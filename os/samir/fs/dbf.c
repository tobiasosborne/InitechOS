/*
 * os/samir/fs/dbf.c -- SAMIR (InitechBase) .dbf header parser + invariants (S1.1).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (samir/pal.h,
 * samir/rt.h, samir/dbf.h) and the LOCKED spec/samir/dbf_format.h. No libc, no
 * int 0x21 -- all OS contact is through the PAL vtable (plan Sec 2.B/2.D).
 *
 * Scope (plan S1.1): parse + validate the 32-byte header, locate the 0x0D
 * field-descriptor terminator (deriving nfields and the +1/+2 form), and assert
 * the three structural invariants from dbf.md sec 8. The field-length byte of
 * each descriptor (offset 0x10) is read ONLY to sum the record-length invariant
 * (1b); names/types/dec are NOT decoded here (that is S1.2).
 *
 * Fail loud (Rule 2): every malformed-input / violated-invariant path returns a
 * negated dbf_err and leaves no half-open handle. A silently-wrong table would
 * corrupt everything built on top of it (Tool of last resort).
 *
 * III+ 1.1 ONLY (plan Sec 2.C): dBASE IV version bytes (0x04/0x8B and anything
 * not 0x03/0x83) are rejected with DBF_ERR_BAD_VERSION, not half-supported.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2,3,4,8,9.
 *   - spec/samir/dbf_format.h (every offset/size constant; no hardcoded offsets).
 *   - docs/plans/SAMIR-implementation-plan.md S1.1 contract.
 *   - os/samir/include/samir/pal.h (open/read/seek/close + seek=filesize idiom).
 */

#include <stdint.h>

#include "samir/pal.h"
#include "samir/rt.h"
#include "samir/dbf.h"

#include "samir/dbf_format.h"   /* LOCKED spec-data, on -Ispec/samir or -Ispec */

/* ---- Opaque table layout (S1.1 fields + the handle S1.2 will build on) ---- */
struct dbf_table {
    samir_pal_t *pal;        /* the PAL this table was opened through */
    pal_fd       fd;         /* OPEN handle; S1.2/S1.3 read the record area */
    void        *mark;       /* arena mark taken before alloc; dbf_close unwinds */

    uint16_t     header_length;   /* offset 0x08, u16 LE */
    uint16_t     record_length;   /* offset 0x0A, u16 LE */
    uint32_t     nrec;            /* offset 0x04, u32 LE */
    uint16_t     nfields;         /* derived: scan-to-0x0D / DBF_DESC_STRIDE */
    uint8_t      version;         /* offset 0x00 */
    uint8_t      year;            /* offset 0x01 (year - 1900) */
    uint8_t      month;           /* offset 0x02 */
    uint8_t      day;             /* offset 0x03 */
    uint8_t      term_extra;      /* 1 (+1 form) or 2 (+2 form) */
};

/* ---- little-endian readers (header binary is LE; dbf.md sec 1/2) ---- */
static uint16_t rd_u16le(const uint8_t *b)
{
    return (uint16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
}

static uint32_t rd_u32le(const uint8_t *b)
{
    return (uint32_t)b[0]
         | ((uint32_t)b[1] << 8)
         | ((uint32_t)b[2] << 16)
         | ((uint32_t)b[3] << 24);
}

/*
 * read_exact: read exactly `n` bytes into `buf` at absolute offset `off`.
 * Returns DBF_OK or a negated dbf_err. A short read is an I/O error here (the
 * header / descriptors are fixed-size regions that MUST be fully present).
 * Ref: pal.h read contract (a short read returns the real count, never a full).
 */
static int read_exact(samir_pal_t *pal, pal_fd fd, int32_t off,
                      void *buf, uint32_t n)
{
    int32_t pos;
    int32_t got;

    pos = pal->seek(pal, fd, off, PAL_SEEK_SET);
    if (pos < 0 || pos != off)
        return -DBF_ERR_IO;

    got = pal->read(pal, fd, buf, n);
    if (got < 0)
        return -DBF_ERR_IO;
    if ((uint32_t)got != n)
        return -DBF_ERR_SHORT;   /* fail loud: region not fully present */
    return DBF_OK;
}

/* close the fd and unwind the arena; returns the PAL close result (negated). */
static int teardown(dbf_table *tbl, int rc)
{
    samir_pal_t *pal = tbl->pal;
    void *mark = tbl->mark;
    int close_rc = 0;

    if (tbl->fd >= 0)
        close_rc = pal->close(pal, tbl->fd);
    pal->reset(pal, mark);   /* frees the table itself (allocated after mark) */

    if (rc != DBF_OK)
        return rc;            /* preserve the original error */
    return (close_rc < 0) ? -DBF_ERR_IO : DBF_OK;
}

int dbf_open(samir_pal_t *pal, const char *name, dbf_table **out)
{
    uint8_t hdr[DBF_HDR_SIZE];
    uint8_t desc[DBF_DESC_STRIDE];
    void *mark;
    dbf_table *tbl;
    pal_fd fd;
    int rc;
    uint8_t ver;
    uint16_t header_length, record_length;
    uint32_t nrec;
    uint16_t nfields;
    uint32_t sum_lens;     /* 1 + sum(field lengths) accumulator (invariant 1b) */
    int32_t  filesz;
    uint32_t body;         /* header_length + nrec*record_length */
    int32_t  off;
    uint32_t descbytes;    /* DBF_DESC_STRIDE * nfields */
    int32_t  term_off;     /* offset of the 0x0D terminator */
    uint32_t hdr_extra;    /* header_length - (DBF_HDR_SIZE + descbytes) */

    if (out)
        *out = (dbf_table *)0;
    if (!pal || !name || !out)
        return -DBF_ERR_IO;

    /* Allocate the table from the PAL arena; mark first so dbf_close unwinds it.
     * Fail loud on NULL (arena exhausted), Rule 2. */
    mark = pal->alloc(pal, 0);                 /* current arena mark (zero alloc) */
    tbl = (dbf_table *)pal->alloc(pal, (uint32_t)sizeof(*tbl));
    if (!tbl)
        return -DBF_ERR_NOMEM;
    rt_memset(tbl, 0, (uint32_t)sizeof(*tbl));
    tbl->pal  = pal;
    tbl->mark = mark;
    tbl->fd   = -1;

    /* Open for reading; the handle stays open for the record area (S1.2/S1.3). */
    fd = pal->open(pal, name, PAL_RD);
    if (fd < 0)
        return teardown(tbl, -DBF_ERR_IO);
    tbl->fd = fd;

    /* --- 32-byte header (dbf.md sec 2). --- */
    rc = read_exact(pal, fd, 0, hdr, DBF_HDR_SIZE);
    if (rc != DBF_OK)
        return teardown(tbl, rc);

    /* Version (dbf.md sec 3). III+ 1.1 ONLY: only 0x03/0x83 are valid; IV bytes
     * (0x04/0x8B) and anything else fail loud (plan Sec 2.C). */
    ver = hdr[DBF_HDR_VERSION_OFF];
    if (ver != DBF_VERSION_NO_MEMO && ver != DBF_VERSION_WITH_MEMO)
        return teardown(tbl, -DBF_ERR_BAD_VERSION);

    header_length = rd_u16le(hdr + DBF_HDR_HEADER_LEN_OFF);
    record_length = rd_u16le(hdr + DBF_HDR_RECORD_LEN_OFF);
    nrec          = rd_u32le(hdr + DBF_HDR_NREC_OFF);

    /* header_length must at least cover the 32-byte header + one descriptor +
     * the lone terminator. Fail loud otherwise (a structurally impossible file). */
    if (header_length < (uint16_t)(DBF_HDR_SIZE + DBF_DESC_STRIDE + 1))
        return teardown(tbl, -DBF_ERR_BAD_HDRLEN);

    /*
     * --- Field count + terminator form (dbf.md sec 4/8). ---
     * Scan 32-byte descriptors from offset DBF_HDR_SIZE, accumulating the
     * field-length byte (descriptor offset 0x10) for invariant 1b, until the
     * NEXT descriptor slot's first byte is the 0x0D terminator. dbf.md sec 8/9:
     * the descriptor scan -- NOT arithmetic on header_length -- is authoritative,
     * because both +1 and +2 terminator conventions exist.
     */
    sum_lens = (uint32_t)DBF_REC_FLAG_SIZE;   /* the delete-flag byte (dbf.md sec 6) */
    nfields  = 0;
    off      = (int32_t)DBF_HDR_SIZE;
    term_off = -1;

    for (;;) {
        uint8_t lead;

        /* Read one byte at `off`: either a descriptor's name[0] or the 0x0D term. */
        rc = read_exact(pal, fd, off, &lead, 1);
        if (rc != DBF_OK)
            return teardown(tbl, (rc == -DBF_ERR_SHORT) ? -DBF_ERR_NO_TERM : rc);

        if (lead == DBF_DESC_TERMINATOR) {
            term_off = off;
            break;
        }

        if (nfields >= DBF_MAX_FIELDS)
            return teardown(tbl, -DBF_ERR_TOO_MANY);

        /* A full 32-byte descriptor must be present (fail loud on truncation). */
        rc = read_exact(pal, fd, off, desc, DBF_DESC_STRIDE);
        if (rc != DBF_OK)
            return teardown(tbl, (rc == -DBF_ERR_SHORT) ? -DBF_ERR_NO_TERM : rc);

        /* Invariant 1b sum: read ONLY the field-length byte (offset 0x10).
         * S1.2 decodes name/type/dec; S1.1 needs just the length. */
        sum_lens += (uint32_t)desc[DBF_DESC_FIELD_LEN_OFF];

        nfields++;
        off += (int32_t)DBF_DESC_STRIDE;
    }

    if (nfields == 0)
        return teardown(tbl, -DBF_ERR_NO_TERM);   /* a table needs >= 1 field */

    /*
     * --- Invariant 1 (dbf.md sec 8): the gap between header_length and the end
     * of the descriptor array (32 + 32*nfields) is the terminator region, and
     * must be exactly 1 (lone 0x0D, "+1") or 2 (0x0D 0x00, "+2").
     * Cross-check: the terminator we located sits at header_length - extra.
     */
    descbytes = (uint32_t)DBF_DESC_STRIDE * (uint32_t)nfields;
    if ((uint32_t)header_length < (uint32_t)DBF_HDR_SIZE + descbytes)
        return teardown(tbl, -DBF_ERR_BAD_HDRLEN);
    hdr_extra = (uint32_t)header_length - ((uint32_t)DBF_HDR_SIZE + descbytes);
    if (hdr_extra != 1u && hdr_extra != 2u)
        return teardown(tbl, -DBF_ERR_BAD_HDRLEN);

    /* The 0x0D we scanned to must be the FIRST byte of the terminator region,
     * i.e. at offset (DBF_HDR_SIZE + descbytes) == header_length - hdr_extra. */
    if ((uint32_t)term_off != (uint32_t)DBF_HDR_SIZE + descbytes)
        return teardown(tbl, -DBF_ERR_NO_TERM);

    /*
     * --- Invariant 1b (dbf.md sec 8): record_length == 1 + sum(field lengths).
     * `sum_lens` already includes the +1 delete-flag byte. This is also how the
     * absence of a III+ NULL bitmap is confirmed (dbf.md sec 5).
     */
#ifdef DBF_MUTATE_RECLEN
    /* MUTATION HOOK (Rule 6): off-by-one in the invariant-1b check. Comparing
     * against (sum_lens + 1) instead of sum_lens makes a VALID golden -- whose
     * record_length equals exactly 1 + sum(field lengths) -- FAIL dbf_open with
     * DBF_ERR_BAD_RECLEN, turning the mutant build RED. Exactly one perturbed
     * branch; restored when the macro is undefined. */
    if ((uint32_t)record_length != sum_lens + 1u)
        return teardown(tbl, -DBF_ERR_BAD_RECLEN);
#else
    if ((uint32_t)record_length != sum_lens)
        return teardown(tbl, -DBF_ERR_BAD_RECLEN);
#endif

    /*
     * --- Invariant 2 (dbf.md sec 8): file_size == header_length +
     * nrec*record_length [+ 0 or 1 EOF byte]. The hard structural requirement a
     * reader needs is that the record area is FULLY present:
     *     file_size >= header_length + nrec*record_length.
     * dbf.md sec 8 "BANK special case" + sec 9: III+ does not truncate the file
     * on delete/PACK, so file_size may be LARGER (ghost data) -- a reader uses
     * nrec and ignores everything past the logical EOF. The optional 0x1A EOF
     * byte (present in TAX/UNIVERSD, absent in CLIENTS/TOURS/TRAVEL) is the +1.
     * A file SHORTER than the declared record area is corrupt -> fail loud.
     * The file-size primitive is seek(fd,0,PAL_SEEK_END) (pal.h; ADR-0008 DEC-02).
     */
    filesz = pal->seek(pal, fd, 0, PAL_SEEK_END);
    if (filesz < 0)
        return teardown(tbl, -DBF_ERR_IO);
    body = (uint32_t)header_length + nrec * (uint32_t)record_length;
    if ((uint32_t)filesz < body)
        return teardown(tbl, -DBF_ERR_BAD_FILESZ);

    /* All invariants hold: capture the parsed values. */
    tbl->version       = ver;
    tbl->year          = hdr[DBF_HDR_YEAR_OFF];
    tbl->month         = hdr[DBF_HDR_MONTH_OFF];
    tbl->day           = hdr[DBF_HDR_DAY_OFF];
    tbl->nrec          = nrec;
    tbl->header_length = header_length;
    tbl->record_length = record_length;
    tbl->nfields       = nfields;
    tbl->term_extra    = (uint8_t)hdr_extra;

    *out = tbl;
    return DBF_OK;
}

int dbf_close(dbf_table *tbl)
{
    if (!tbl)
        return DBF_OK;
    return teardown(tbl, DBF_OK);
}

/* ---- accessors (pure reads of values captured at open) ---- */

uint8_t  dbf_version(const dbf_table *t)       { return t->version; }
uint8_t  dbf_year(const dbf_table *t)          { return t->year; }
uint8_t  dbf_month(const dbf_table *t)         { return t->month; }
uint8_t  dbf_day(const dbf_table *t)           { return t->day; }
uint32_t dbf_nrec(const dbf_table *t)          { return t->nrec; }
uint16_t dbf_header_length(const dbf_table *t) { return t->header_length; }
uint16_t dbf_record_length(const dbf_table *t) { return t->record_length; }
uint16_t dbf_nfields(const dbf_table *t)       { return t->nfields; }
uint8_t  dbf_term_extra(const dbf_table *t)    { return t->term_extra; }
int      dbf_has_memo(const dbf_table *t)
{
    return (t->version & 0x80u) ? 1 : 0;   /* dbf.md sec 3: bit 7 = has memo */
}

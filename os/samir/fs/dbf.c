/*
 * os/samir/fs/dbf.c -- SAMIR (InitechBase) .dbf codec: header (S1.1) +
 *                      field-descriptor array (S1.2).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (samir/pal.h,
 * samir/rt.h, samir/dbf.h) and the LOCKED spec/samir/dbf_format.h. No libc, no
 * int 0x21 -- all OS contact is through the PAL vtable (plan Sec 2.B/2.D).
 *
 * S1.1 scope: parse + validate the 32-byte header, locate the 0x0D
 * field-descriptor terminator (deriving nfields and the +1/+2 form), assert the
 * three structural invariants from dbf.md sec 8. The field-length byte of each
 * descriptor (offset 0x10) is read during the scan loop for invariant-1b sum.
 *
 * S1.2 scope (docs/plans/SAMIR-implementation-plan.md S1.2): after S1.1's scan
 * completes, re-read each descriptor in a second pass to decode the full
 * name/type/length/dec metadata into an arena-allocated dbf_field_t array.
 * S1.1 invariants are preserved; the field-count comes from S1.1's scan-to-0x0D
 * (NOT re-derived independently). The mutation hook (DBF_MUTATE_STRIDE) perturbs
 * the descriptor stride to the dBASE-7 48-byte value, shifting every field after
 * the first so decoded names/types/lengths mismatch the golden -> tests go RED.
 *
 * Fail loud (Rule 2): every malformed-input / violated-invariant path returns a
 * negated dbf_err and leaves no half-open handle. A silently-wrong table would
 * corrupt everything built on top of it (Tool of last resort).
 *
 * III+ 1.1 ONLY (plan Sec 2.C): dBASE IV version bytes (0x04/0x8B and anything
 * not 0x03/0x83) are rejected with DBF_ERR_BAD_VERSION, not half-supported.
 * Unknown field type bytes (not C/N/D/L/M) fail loud with DBF_ERR_BAD_TYPE.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2,3,4,5,8,9.
 *     sec 4 worked example: CLIENTS FIRSTNAME at offset 0x20
 *       46 49 52 53 54 4E 41 4D 45 00 00 / 43 / 23 00 2C 40 / 14 / 00 / ...
 *       name="FIRSTNAME", type='C', len=20, dec=0.
 *     sec 9: "name = bytes to first NUL; trailing bytes may be garbage."
 *   - spec/samir/dbf_format.h (every offset/size constant; no hardcoded offsets):
 *     DBF_DESC_NAME_OFF, DBF_DESC_TYPE_OFF, DBF_DESC_FIELD_LEN_OFF,
 *     DBF_DESC_DEC_COUNT_OFF, DBF_DESC_STRIDE=32, DBF_DESC_NAME_SIZE=11.
 *   - docs/plans/SAMIR-implementation-plan.md S1.1+S1.2 contract.
 *   - os/samir/include/samir/pal.h (open/read/seek/close + seek=filesize idiom).
 */

#include <stdint.h>

#include "samir/pal.h"
#include "samir/rt.h"
#include "samir/dbf.h"

#include "samir/dbf_format.h"   /* LOCKED spec-data, on -Ispec/samir or -Ispec */

/*
 * S1.2 MUTATION HOOK (Rule 6): when -DDBF_MUTATE_STRIDE is defined, the
 * descriptor decode pass uses a 48-byte stride (the dBASE-7 descriptor size)
 * instead of the correct 32-byte III+ stride. This shifts the decode offset of
 * every descriptor after the first, so names/types/lengths read from wrong bytes
 * and diverge from the golden -> the test goes RED. The S1.1 scan-to-0x0D still
 * uses the correct DBF_DESC_STRIDE (it runs before this constant is used), so the
 * table still opens and nfields is still correct; only the S1.2 field-decode pass
 * is perturbed. Exactly one constant is changed.
 * Ref: docs/plans/SAMIR-implementation-plan.md S1.2 mutant (dBASE-7 stride -> RED).
 */
#ifdef DBF_MUTATE_STRIDE
#  define DECODE_STRIDE  48u   /* dBASE-7 descriptor size; NOT the III+ 32-byte form */
#else
#  define DECODE_STRIDE  ((uint32_t)DBF_DESC_STRIDE)   /* correct: 32 bytes (dbf.md sec 4) */
#endif

/* ---- Opaque table layout (S1.1 fields + S1.2 field array) ---- */
struct dbf_table {
    samir_pal_t  *pal;        /* the PAL this table was opened through */
    pal_fd        fd;         /* OPEN handle; S1.3 reads the record area */
    void         *mark;       /* arena mark taken before alloc; dbf_close unwinds */

    dbf_field_t  *fields;     /* S1.2: arena-allocated array of nfields decoded
                               * field descriptors; NULL until S1.2 decode pass runs.
                               * dbf_field(tbl,i) reads from this array (0-based).
                               * Ref: dbf.md sec 4 + spec/samir/dbf_format.h. */

    uint16_t      header_length;   /* offset 0x08, u16 LE */
    uint16_t      record_length;   /* offset 0x0A, u16 LE */
    uint32_t      nrec;            /* offset 0x04, u32 LE */
    uint16_t      nfields;         /* derived: scan-to-0x0D / DBF_DESC_STRIDE */
    uint8_t       version;         /* offset 0x00 */
    uint8_t       year;            /* offset 0x01 (year - 1900) */
    uint8_t       month;           /* offset 0x02 */
    uint8_t       day;             /* offset 0x03 */
    uint8_t       term_extra;      /* 1 (+1 form) or 2 (+2 form) */
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

    /* All S1.1 invariants hold: capture the parsed values. */
    tbl->version       = ver;
    tbl->year          = hdr[DBF_HDR_YEAR_OFF];
    tbl->month         = hdr[DBF_HDR_MONTH_OFF];
    tbl->day           = hdr[DBF_HDR_DAY_OFF];
    tbl->nrec          = nrec;
    tbl->header_length = header_length;
    tbl->record_length = record_length;
    tbl->nfields       = nfields;
    tbl->term_extra    = (uint8_t)hdr_extra;

    /*
     * --- S1.2: field-descriptor array decode ---
     *
     * Now that nfields is known (from S1.1's scan-to-0x0D), allocate and fill
     * a dbf_field_t array. We re-read each descriptor from the PAL (the fd is
     * still open at an arbitrary position after the invariant checks). This is a
     * second pass over the same bytes; the S1.1 scan-loop results (nfields, the
     * record-length sum) are authoritative and are NOT re-derived here.
     *
     * For each descriptor at (DBF_HDR_SIZE + i * DECODE_STRIDE):
     *   - name   : 11 bytes (DBF_DESC_NAME_OFF) -> decode to first 0x00, store
     *              NUL-terminated in dbf_field_t.name (dbf.md sec 4 / sec 9).
     *   - type   : 1 byte  (DBF_DESC_TYPE_OFF)  -> ASCII char 'C'/'N'/'D'/'L'/'M';
     *              fail loud (DBF_ERR_BAD_TYPE) on any other value (dbf.md sec 5).
     *   - len    : 1 byte  (DBF_DESC_FIELD_LEN_OFF).
     *   - dec    : 1 byte  (DBF_DESC_DEC_COUNT_OFF).
     *
     * The DECODE_STRIDE constant is 32 normally; when built with
     * -DDBF_MUTATE_STRIDE it is 48 (the dBASE-7 stride) so every field after
     * the first is decoded from shifted bytes -> mismatch with the golden -> RED
     * (mutation hook, Rule 6).
     *
     * Ref: dbf.md sec 4/5/9; spec/samir/dbf_format.h (all offset constants);
     *      plan S1.2 contract.
     */
    if (nfields > 0) {
        uint32_t n       = (uint32_t)nfields;
        uint32_t fsize   = n * (uint32_t)sizeof(dbf_field_t);
        dbf_field_t *fa  = (dbf_field_t *)tbl->pal->alloc(tbl->pal, fsize);
        uint32_t fi;

        if (!fa)
            return teardown(tbl, -DBF_ERR_NOMEM);

        rt_memset(fa, 0, fsize);
        tbl->fields = fa;

        for (fi = 0u; fi < n; fi++) {
            int32_t doff = (int32_t)((uint32_t)DBF_HDR_SIZE + fi * DECODE_STRIDE);
            uint8_t dbuf[DBF_DESC_STRIDE];   /* always read 32 bytes (the real stride) */
            uint8_t ttype;
            uint32_t k;
            int nul_found;

            rc = read_exact(pal, fd, doff, dbuf, (uint32_t)DBF_DESC_STRIDE);
            if (rc != DBF_OK)
                return teardown(tbl, (rc == -DBF_ERR_SHORT) ? -DBF_ERR_SHORT : rc);

            /* Name: decode to first 0x00 in the 11-byte slot; ignore trailing
             * bytes (dbf.md sec 4 "Field name: read to first NUL; ignore
             * trailing garbage", sec 9). Store NUL-terminated in .name[]. */
            nul_found = 0;
            for (k = 0u; k < (uint32_t)DBF_DESC_NAME_SIZE; k++) {
                if (dbuf[DBF_DESC_NAME_OFF + k] == 0x00u) {
                    nul_found = 1;
                    break;
                }
                fa[fi].name[k] = (char)dbuf[DBF_DESC_NAME_OFF + k];
            }
            fa[fi].name[nul_found ? k : (uint32_t)DBF_DESC_NAME_SIZE] = '\0';

            /* Type: must be one of C/N/D/L/M (III+ only; dbf.md sec 5).
             * Any other byte fails loud (plan Sec 2.C; Rule 2). */
            ttype = dbuf[DBF_DESC_TYPE_OFF];
            if (ttype != (uint8_t)'C' && ttype != (uint8_t)'N' &&
                ttype != (uint8_t)'D' && ttype != (uint8_t)'L' &&
                ttype != (uint8_t)'M')
                return teardown(tbl, -DBF_ERR_BAD_TYPE);
            fa[fi].type = (char)ttype;

            /* Length and decimal count (dbf.md sec 4, offsets 0x10/0x11). */
            fa[fi].field_len  = dbuf[DBF_DESC_FIELD_LEN_OFF];
            fa[fi].dec_count  = dbuf[DBF_DESC_DEC_COUNT_OFF];
        }
    }

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

/*
 * dbf_field: return a pointer to the i-th decoded field descriptor (S1.2).
 *
 * Returns NULL for out-of-range i (i < 0 or i >= nfields). This signals a
 * programming error (fail loud at the call site: callers must not pass a NULL
 * return as a valid descriptor, Rule 2).
 *
 * The returned pointer is owned by the table and is valid until dbf_close.
 * The fields array is populated during dbf_open (S1.2 decode pass); if a table
 * has zero fields (theoretically impossible after the nfields==0 invariant in
 * S1.1, but defensively handled) tbl->fields is NULL and this returns NULL.
 *
 * Ref: dbf.h dbf_field_t; dbf.md sec 4/9; plan S1.2 contract.
 */
const dbf_field_t *dbf_field(const dbf_table *t, int i)
{
    if (!t || !t->fields)
        return (const dbf_field_t *)0;
    if (i < 0 || (uint32_t)i >= (uint32_t)t->nfields)
        return (const dbf_field_t *)0;
    return &t->fields[i];
}

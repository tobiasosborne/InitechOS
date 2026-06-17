/*
 * os/samir/fs/dbf.c -- SAMIR (InitechBase) .dbf codec: header (S1.1),
 *                      field-descriptor array (S1.2), record read (S1.3).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers (samir/pal.h,
 * samir/rt.h, samir/value.h, samir/dbf.h) and the LOCKED spec/samir/dbf_format.h.
 * No libc, no int 0x21 -- all OS contact is through the PAL vtable (plan Sec
 * 2.B/2.D).
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
 * S1.3 scope (docs/plans/SAMIR-implementation-plan.md S1.3): dbf_read_rec reads
 * a single record by 1-based record number and decodes each field into an xb_val:
 *   C -> xb_c (raw bytes in arena record buffer; trimming is evaluator's job)
 *   N -> xb_n(dec_parse(raw, len)) (all-spaces -> 0.0)
 *   D -> xb_d(jdn_from_ymd(y,m,d)) for valid date; xb_u() for blank/00000000
 *   L -> xb_l(1/0) for T/Y/F/N; xb_u() for '?'/space; fail loud otherwise
 *   M -> xb_m(raw10, 10) for block>0; xb_u() for block 0 / blanks
 * Delete flag: 0x20 -> *deleted=0, 0x2A -> *deleted=1; other bytes -> fail loud.
 * The mutation hook (DBF_MUTATE_RECOFF) shifts the record offset by +1 (skips
 * the delete-flag byte) so every decoded field value is wrong -> tests go RED.
 * Ref: dbf.md sec 6 (record layout) and sec 5 (field type encodings).
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
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2,3,4,5,6,8,9.
 *     sec 4 worked example: CLIENTS FIRSTNAME at offset 0x20
 *       46 49 52 53 54 4E 41 4D 45 00 00 / 43 / 23 00 2C 40 / 14 / 00 / ...
 *       name="FIRSTNAME", type='C', len=20, dec=0.
 *     sec 5: field type on-disk encodings (C space-padded; N right-just; D YYYYMMDD;
 *       L T/F/Y/N/?; M 10-byte ASCII block#).
 *     sec 6: record layout (1 delete-flag byte + packed field bytes, no separator).
 *     sec 9: "name = bytes to first NUL; trailing bytes may be garbage."
 *   - spec/samir/dbf_format.h (every offset/size constant; no hardcoded offsets):
 *     DBF_DESC_NAME_OFF, DBF_DESC_TYPE_OFF, DBF_DESC_FIELD_LEN_OFF,
 *     DBF_DESC_DEC_COUNT_OFF, DBF_DESC_STRIDE=32, DBF_DESC_NAME_SIZE=11,
 *     DBF_REC_DELETE_LIVE=0x20, DBF_REC_DELETE_DELETED=0x2A, DBF_REC_FLAG_SIZE=1.
 *   - docs/plans/SAMIR-implementation-plan.md S1.1+S1.2+S1.3 contract.
 *   - os/samir/include/samir/pal.h (open/read/seek/close + seek=filesize idiom).
 *   - os/samir/include/samir/value.h (xb_val, xb_c/xb_n/xb_d/xb_l/xb_m/xb_u).
 *   - os/samir/include/samir/rt.h (dec_parse, jdn_from_ymd).
 *   - harness/diff/dbf_diff/dbf_ref.py decode_field() (independent reference;
 *     our typed values agree with it field-by-field).
 */

#include <stdint.h>

#include "samir/pal.h"
#include "samir/rt.h"
#include "samir/value.h"   /* xb_val, xb_c/xb_n/xb_d/xb_l/xb_m/xb_u */
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

/*
 * S1.3 MUTATION HOOK (Rule 6): when -DDBF_MUTATE_RECOFF is defined,
 * dbf_read_rec reads the record starting one byte PAST the correct offset.
 * This effectively skips the delete-flag byte and reads field data shifted by
 * one, so decoded field values mismatch the golden -> Tier-0 checks go RED.
 * The perturbation is a single additive offset; exactly one constant changes.
 * The mutant still reads record_length bytes (not record_length-1), so the
 * data it decodes comes from the next byte onward -- every field is shifted.
 * Ref: docs/plans/SAMIR-implementation-plan.md S1.3 mutant (recoff -> RED).
 */
#ifdef DBF_MUTATE_RECOFF
#  define REC_OFFSET_EXTRA  1u   /* skip 1 extra byte -> field data is shifted */
#else
#  define REC_OFFSET_EXTRA  0u   /* correct: no extra offset */
#endif

/* ---- Opaque table layout (S1.1 fields + S1.2 field array + S1.3 record buf) ---- */
struct dbf_table {
    samir_pal_t  *pal;        /* the PAL this table was opened through */
    pal_fd        fd;         /* OPEN handle; S1.3 reads the record area */
    void         *mark;       /* arena mark taken before alloc; dbf_close unwinds */

    dbf_field_t  *fields;     /* S1.2: arena-allocated array of nfields decoded
                               * field descriptors; NULL until S1.2 decode pass runs.
                               * dbf_field(tbl,i) reads from this array (0-based).
                               * Ref: dbf.md sec 4 + spec/samir/dbf_format.h. */

    uint8_t      *rec_buf;    /* S1.3: arena-allocated record buffer, record_length
                               * bytes. Allocated at open time; reused by every
                               * dbf_read_rec call. xb_c/xb_m pointers in the
                               * decoded xb_val array point into this buffer.
                               * LIFETIME: valid until the next dbf_read_rec call
                               * on the same table. NULL until dbf_open allocates it.
                               * Ref: dbf.h S1.3 "Record buffer lifetime" note. */

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
     * --- S1.3: allocate the reusable record buffer ---
     * Allocate record_length bytes from the PAL arena at open time.  Every call
     * to dbf_read_rec reuses this buffer (overwriting it), so xb_c/xb_m
     * pointers that reference it are invalidated by the next dbf_read_rec call.
     * Ref: dbf.h S1.3 "Record buffer lifetime" + dbf.md sec 6 (record_length).
     */
    {
        uint8_t *rb = (uint8_t *)pal->alloc(pal, (uint32_t)record_length);
        if (!rb)
            return teardown(tbl, -DBF_ERR_NOMEM);
        tbl->rec_buf = rb;
    }

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

/*
 * dbf_read_rec: read and decode one record by 1-based record number (S1.3).
 *
 * 1-based convention: recno 1 is the first data record, stored at byte offset
 * header_length + 0*record_length (i.e. the 0-based index is recno-1).
 * Confirmed by dbf_ref.py iter_records (yields i=0 for the first record).
 *
 * Record layout (dbf.md sec 6):
 *   byte 0            : delete flag (0x20 live, 0x2A deleted)
 *   bytes 1..reclen-1 : field data packed in descriptor order, no separators
 *
 * Field decode (dbf.md sec 5; dbf_ref.py decode_field() for each type):
 *   C  -> xb_c(ptr, field_len) -- raw bytes in rec_buf including trailing spaces.
 *   N  -> xb_n(dec_parse(raw, field_len)) -- dec_parse handles spaces -> 0.0.
 *   D  -> xb_d(jdn_from_ymd) for valid date; xb_u() for blank(8 spaces)/"00000000".
 *   L  -> xb_l(1) T/t/Y/y; xb_l(0) F/f/N/n; xb_u() '?'/space; else BAD_REC.
 *   M  -> xb_m(ptr, 10) block>0; xb_u() block==0 / blanks. S2.1 reads .dbt.
 *
 * Mutation hook (DBF_MUTATE_RECOFF): adds REC_OFFSET_EXTRA (=1 when mutant) to
 * the file seek offset, so field data is read from byte 1 of the record onward
 * (the delete-flag byte is consumed into field 0 rather than the flag slot, and
 * every subsequent field byte is shifted). All decoded values diverge -> RED.
 * The mutant build still opens correctly (dbf_open is unchanged); only
 * dbf_read_rec is perturbed. The extra offset is applied to the byte address
 * passed to read_exact, not to the record-length count.
 *
 * Ref (Law 1):
 *   - dbf.md sec 5 (C/N/D/L/M on-disk encodings; blank date; L uninitialised).
 *   - dbf.md sec 6 (record layout; delete flag 0x20/0x2A).
 *   - spec/samir/dbf_format.h (DBF_REC_DELETE_LIVE, DBF_REC_DELETE_DELETED,
 *     DBF_REC_FLAG_SIZE=1).
 *   - rt.h dec_parse (all-spaces -> 0.0), jdn_from_ymd (Y,M,D -> JDN).
 *   - value.h xb_c/xb_n/xb_d/xb_l/xb_m/xb_u.
 *   - dbf_ref.py decode_field() for C/N/D/L/M field-by-field agreement.
 */
int dbf_read_rec(dbf_table *tbl, uint32_t recno,
                 xb_val *out, int *deleted)
{
    int32_t     rec_off;     /* byte offset of this record's first byte (delete flag) */
    int32_t     field_off;   /* running offset into rec_buf for each field */
    uint8_t     del_flag;
    uint32_t    fi;
    int         rc;

    if (!tbl || !out)
        return -DBF_ERR_IO;

    /* Validate 1-based recno: must be in [1, nrec]. dbf.h S1.3 contract.
     * Ref: dbf.md sec 9 "header nrec is authoritative for reading". */
    if (recno < 1u || recno > tbl->nrec)
        return -DBF_ERR_BAD_RECNO;

    /* Compute the byte offset of this record's first byte (the delete flag).
     * recno 1 -> header_length + 0*record_length; recno k -> + (k-1)*record_length.
     * Ref: dbf.md sec 6 "Record k begins at header_length + k*record_length" (0-based k).
     * The REC_OFFSET_EXTRA mutation shifts this by +1 byte (DBF_MUTATE_RECOFF). */
    rec_off = (int32_t)((uint32_t)tbl->header_length
                        + (recno - 1u) * (uint32_t)tbl->record_length
                        + REC_OFFSET_EXTRA);

    /* Read the entire record into the reusable rec_buf.
     * read_exact seeks to rec_off and reads record_length bytes. */
    rc = read_exact(tbl->pal, tbl->fd, rec_off, tbl->rec_buf,
                    (uint32_t)tbl->record_length);
    if (rc != DBF_OK)
        return rc;

    /* Delete flag is the first byte of the record (rec_buf[0]).
     * 0x20 = live, 0x2A = deleted; any other byte is corrupt data -> fail loud.
     * Ref: dbf.md sec 6; spec/samir/dbf_format.h DBF_REC_DELETE_LIVE/_DELETED. */
    del_flag = tbl->rec_buf[0];
    if (del_flag == (uint8_t)DBF_REC_DELETE_LIVE) {
        if (deleted) *deleted = 0;
    } else if (del_flag == (uint8_t)DBF_REC_DELETE_DELETED) {
        if (deleted) *deleted = 1;
    } else {
        /* Corrupt record: delete flag is neither 0x20 nor 0x2A. Fail loud
         * (Rule 2). A silently-wrong delete-flag interpretation corrupts pack/zap. */
        return -DBF_ERR_BAD_REC;
    }

    /* Decode each field in descriptor order.
     * Field data starts at rec_buf[DBF_REC_FLAG_SIZE] (= rec_buf[1]).
     * Each field occupies exactly field_len bytes, packed with no separator.
     * Ref: dbf.md sec 6. */
    field_off = (int32_t)DBF_REC_FLAG_SIZE;

    for (fi = 0u; fi < (uint32_t)tbl->nfields; fi++) {
        const dbf_field_t *fd = &tbl->fields[fi];
        uint8_t *raw = tbl->rec_buf + (uint32_t)field_off;
        uint8_t  flen = fd->field_len;
        char     ftype = fd->type;

        switch ((unsigned char)ftype) {

        case (unsigned char)'C':
            /*
             * Character: raw bytes in rec_buf, left-justified, space-padded.
             * We expose the FULL field_len bytes (including trailing spaces)
             * as xb_c so that the round-trip (S1.4) is lossless. Trimming is
             * the evaluator's job (S3.3). Pointer into rec_buf; valid until
             * the next dbf_read_rec call. Ref: dbf.md sec 5 C; dbf_ref.py
             * decode_field('C') -> rstrip but that is display-only.
             */
            out[fi] = xb_c((const char *)raw, (uint16_t)flen);
            break;

        case (unsigned char)'N':
            /*
             * Numeric: right-justified ASCII decimal. dec_parse skips leading/
             * trailing spaces and returns 0.0 for all-spaces (blank N field).
             * All-asterisk overflow fields: non-digit char -> dec_parse returns
             * 0.0 (rt.h: "malformed input -> some double"). This matches the
             * blank/overflow fallback of dbf_ref.py decode_field('N') -> None.
             * Ref: dbf.md sec 5 N; rt.h dec_parse; dbf_ref.py.
             */
            out[fi] = xb_n(dec_parse((const char *)raw, (int)flen));
            break;

        case (unsigned char)'D':
            /*
             * Date: 8 ASCII chars YYYYMMDD. Blank (8 spaces) or "00000000" ->
             * xb_u(). Valid non-blank date -> xb_d(jdn_from_ymd(y,m,d)).
             * Ref: dbf.md sec 5 D "8 spaces = blank/empty; 00000000 not valid".
             * dbf_ref.py decode_field('D') returns "" for blank; xb_u() is the
             * typed-model equivalent (no blank-date sentinel in xb_type).
             *
             * Blank detection: if raw[0] is a space (0x20) we treat the whole
             * field as blank (all 8 spaces; a partial-blank date is malformed
             * but we treat it as blank rather than failing to avoid unnecessary
             * halt on uncommon edge cases). "00000000" check: if all 8 chars
             * are '0', also treat as xb_u() (dbf.md sec 5 D explicit rule).
             */
            {
                int32_t y, m, d;
                int is_blank, is_zero;
                int32_t jdn;

                /* Blank check: first byte is a space. */
                is_blank = (raw[0] == (uint8_t)' ');
                /* Zero-date check: "00000000" -- all ASCII zeros. */
                is_zero = (raw[0] == (uint8_t)'0' &&
                           raw[1] == (uint8_t)'0' &&
                           raw[2] == (uint8_t)'0' &&
                           raw[3] == (uint8_t)'0' &&
                           raw[4] == (uint8_t)'0' &&
                           raw[5] == (uint8_t)'0' &&
                           raw[6] == (uint8_t)'0' &&
                           raw[7] == (uint8_t)'0');

                if (is_blank || is_zero) {
                    out[fi] = xb_u();
                } else {
                    /* Parse YYYYMMDD from ASCII digits. dec_parse on 4-char
                     * slice gives the integer year; same for month and day. */
                    y = (int32_t)dec_parse((const char *)raw,     4);
                    m = (int32_t)dec_parse((const char *)raw + 4, 2);
                    d = (int32_t)dec_parse((const char *)raw + 6, 2);
                    jdn = jdn_from_ymd(y, m, d);
                    out[fi] = xb_d((double)jdn);
                }
            }
            break;

        case (unsigned char)'L':
            /*
             * Logical: 1 byte. T/t/Y/y -> xb_l(1); F/f/N/n -> xb_l(0);
             * '?'/space (0x3F/0x20) -> xb_u() (uninitialised).
             * Any other byte is corrupt -> fail loud (Rule 2).
             * Ref: dbf.md sec 5 L; dbf_ref.py decode_field('L').
             * Note: corpus fixtures only exhibit T and F (no '?' seen in
             * TRAVEL.DBF PAID column), but we handle all defined values.
             */
            {
                uint8_t lb = raw[0];
                if (lb == (uint8_t)'T' || lb == (uint8_t)'t' ||
                    lb == (uint8_t)'Y' || lb == (uint8_t)'y') {
                    out[fi] = xb_l(1);
                } else if (lb == (uint8_t)'F' || lb == (uint8_t)'f' ||
                           lb == (uint8_t)'N' || lb == (uint8_t)'n') {
                    out[fi] = xb_l(0);
                } else if (lb == (uint8_t)'?' || lb == (uint8_t)' ') {
                    out[fi] = xb_u();   /* uninitialised L field */
                } else {
                    /* Unknown L byte: corrupt data. Fail loud (Rule 2). */
                    return -DBF_ERR_BAD_REC;
                }
            }
            break;

        case (unsigned char)'M':
            /*
             * Memo pointer: 10-byte right-justified ASCII decimal block number.
             * S1.3->S2.1 boundary: the .dbt is NOT read here.
             * Block 0 or all-blank field -> xb_u() (no memo for this record).
             * Block > 0 -> xb_m(raw, 10): pointer into rec_buf; S2.1 uses
             *   dec_parse(ptr, 10) to recover the block number and open .dbt.
             * Ref: dbf.md sec 5 M "block 0 or blanks = no memo";
             *      dbf_ref.py decode_field('M') -> 0 for blank/zero.
             *
             * GATED: "10 blanks" vs right-justified "0" both mean no-memo;
             * both are treated as xb_u() here. [dbf.md sec 5 M note]
             */
            {
                double block_num = dec_parse((const char *)raw, 10);
                if (block_num <= 0.5) {   /* block 0 or blank (dec_parse -> 0.0) */
                    out[fi] = xb_u();
                } else {
                    out[fi] = xb_m((const char *)raw, 10u);
                }
            }
            break;

        default:
            /* Should never reach here: dbf_open rejects non-C/N/D/L/M types
             * with DBF_ERR_BAD_TYPE. If somehow reached, fail loud (Rule 2). */
            return -DBF_ERR_BAD_REC;
        }

        field_off += (int32_t)flen;
    }

    return DBF_OK;
}

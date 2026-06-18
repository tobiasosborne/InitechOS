/*
 * os/samir/fs/dbf.c -- SAMIR (InitechBase) .dbf codec: header (S1.1),
 *                      field-descriptor array (S1.2), record read (S1.3),
 *                      deterministic write + round-trip (S1.4).
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
 * S1.4 scope (docs/plans/SAMIR-implementation-plan.md S1.4): deterministic
 * write + round-trip. dbf_create builds the header + descriptors + opens the
 * file; dbf_append_rec formats typed values into an in-arena record region;
 * dbf_flush serializes the whole image (version 0x03/0x83, injected last-update
 * date, +1 terminator, records, trailing 0x1A) in one deterministic byte stream
 * (Rule 11). Every NORMALIZE byte (spec/samir/dbf_normalization.json) is written
 * 0x00 (esp. the field RAM address 0x0C, work-area 0x14, LDID 0x1D). The mutation
 * hook (DBF_MUTATE_VERSION) drops the 0x80 memo bit so a memo schema round-trips
 * with has_memo=false -> RED. Ref: dbf.md sec 2/3/4/5/6/8.
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

/*
 * S1.4 MUTATION HOOK (Rule 6): when -DDBF_MUTATE_VERSION is defined, dbf_flush
 * writes 0x03 in the version byte EVEN WHEN a memo (M) field is present (it
 * drops the 0x80 memo bit). A round-trip of a memo-bearing schema then reads
 * back has_memo=false (and version 0x03), so the round-trip oracle goes RED.
 * Exactly one decision is perturbed: the has-memo -> memo-bit mapping. The
 * no-memo case (0x03) is unaffected, so a no-memo round-trip still passes --
 * only the memo schema bites, which is the point. Ref: plan S1.4 mutant
 * ("emit 0x03 where 0x83 required -> RED").
 */
#ifdef DBF_MUTATE_VERSION
#  define DBF_MEMO_VERSION_BIT  0x00u   /* mutant: drop the 0x80 memo bit */
#else
#  define DBF_MEMO_VERSION_BIT  0x80u   /* correct: dbf.md sec 3 bit 7 = has memo */
#endif

/*
 * 7az.16 MUTATION HOOK (Rule 6): when -DDBF_MUTATE_OPENRW_RO is defined,
 * dbf_open_rw opens the file PAL_RDWR + loads the record region (as normal) but
 * leaves tbl->writable = 0 (the read-only state dbf_open produces). Every S1.5
 * mutation verb (dbf_replace/dbf_append_blank/dbf_delete/...) and dbf_flush then
 * reject the table with -DBF_ERR_IO, so a plain-USE-then-REPLACE persists NOTHING
 * -> the RW-USE oracle goes RED. Exactly one decision (the writable flag) is
 * perturbed; the read path (dbf_read_rec) still works, so a read-only assertion
 * would still pass -- the mutant bites precisely the write capability 7az.16 adds.
 * Ref: dbf.h dbf_open_rw; dbf.c dbf_replace/dbf_flush writable guards.
 */

/*
 * Upper bound on appended records for an in-arena built table (S1.4). Keeps the
 * record region from being asked to grow without bound; well above any S1.4
 * round-trip fixture. A genuine III+ table allows up to ~1e9 records, but the
 * S1.4 build-in-arena writer is for creating small test/seed tables, not bulk
 * loads. Ref: plan S1.4 ("minimal ... to put records before flush").
 */
#define DBF_MAX_RECS  65535u

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

    uint8_t      *rec_region; /* S1.4: arena-allocated contiguous record region for
                               * a dbf_create()'d table; record_length * cap bytes.
                               * Each dbf_append_rec formats one record here. NULL
                               * for a dbf_open()'d (read-only) table. */
    uint32_t      rec_cap;    /* S1.4: capacity of rec_region in records. */
    uint8_t       writable;   /* S1.4: 1 if created by dbf_create (flushable),
                               * 0 if opened read-only by dbf_open. */

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

/* ---- little-endian writers (S1.4; mirror the readers above) ---- */
static void wr_u16le(uint8_t *b, uint16_t v)
{
    b[0] = (uint8_t)(v & 0xFFu);
    b[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void wr_u32le(uint8_t *b, uint32_t v)
{
    b[0] = (uint8_t)(v & 0xFFu);
    b[1] = (uint8_t)((v >> 8) & 0xFFu);
    b[2] = (uint8_t)((v >> 16) & 0xFFu);
    b[3] = (uint8_t)((v >> 24) & 0xFFu);
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

/*
 * dbf_open_common: the shared open+parse path behind both dbf_open (read-only)
 * and dbf_open_rw (read-write; 7az.16). It opens `name` with `open_mode`, parses
 * + validates the header/descriptors/record-area exactly the same way for both
 * callers (one parse path, no duplicate-and-drift), allocates the field array +
 * the record-read buffer, and -- when `want_writable` -- loads every on-disk
 * record into the in-arena rec_region and sets the writable flag so the S1.5
 * mutation verbs (dbf_replace/dbf_append_blank/dbf_delete/recall/pack/zap) and
 * dbf_flush operate on the opened table, NOT only on a dbf_create()'d one.
 *
 * Why the record region is loaded for the RW path (Law 1 / dbf.c S1.5 model):
 * the mutation verbs all patch tbl->rec_region in-arena and dbf_flush rewrites
 * the WHOLE file from that region (build-in-arena/flush-to-disk). A table opened
 * RW with an empty rec_region would flush an empty file. So dbf_open_rw must
 * prime rec_region from the existing records before any verb runs. We read the
 * record bytes directly through read_exact (the same primitive dbf_read_rec uses)
 * so no record is decoded/re-encoded -- the bytes round-trip verbatim.
 *
 * `open_mode`     : PAL_RD (read-only) or PAL_RDWR (read-write).
 * `want_writable` : 0 -> read-only table (dbf_open); 1 -> load rec_region +
 *                   set writable=1 (dbf_open_rw). When 1, open_mode MUST be
 *                   PAL_RDWR so dbf_flush's write path has a writable fd.
 *
 * Ref: dbf.md sec 2/3/4/5/6/8; dbf.h S1.1-S1.5; spec/samir/dbf_format.h.
 */
static int dbf_open_common(samir_pal_t *pal, const char *name, dbf_table **out,
                           int open_mode, int want_writable)
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

    /* Open per the caller's mode; the handle stays open for the record area
     * (S1.2/S1.3) and, for the RW path, for dbf_flush's write-back. */
    fd = pal->open(pal, name, open_mode);
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

    /*
     * --- 7az.16: writable (read-write) USE path ---
     * For a read-write open (dbf_open_rw, want_writable=1) load every existing
     * record into the in-arena rec_region so the S1.5 mutation verbs + dbf_flush
     * (which operate on rec_region, not on disk-in-place) can run after a plain
     * USE. The bytes are copied verbatim from disk via read_exact (the same
     * primitive dbf_read_rec uses) -- no decode/re-encode round-trip, so the
     * records flush back byte-for-byte unless a verb changed them.
     * The DBF_MUTATE_OPENRW_RO mutation hook drops the writable flag so the verbs
     * reject the table -> the RW-USE oracle goes RED (Rule 6; see hook below).
     */
    if (want_writable) {
#ifdef DBF_MUTATE_OPENRW_RO
        tbl->writable = 0u;          /* MUTANT: opened RW but flagged read-only */
#else
        tbl->writable = 1u;
#endif
        /* Load the existing records from the ORIGINAL header_length offset (the
         * value parsed from disk; the records start there regardless of the
         * +1/+2 terminator form). Read BEFORE the header_length normalization
         * below so the offset is the on-disk one. */
        if (nrec > 0u) {
            uint32_t rlen   = (uint32_t)record_length;
            uint32_t total  = nrec * rlen;
            uint8_t *region = (uint8_t *)pal->alloc(pal, total);
            if (!region)
                return teardown(tbl, -DBF_ERR_NOMEM);
            rc = read_exact(pal, fd, (int32_t)header_length, region, total);
            if (rc != DBF_OK)
                return teardown(tbl, rc);
            tbl->rec_region = region;
            tbl->rec_cap    = nrec;
        } else {
            tbl->rec_region = (uint8_t *)0;
            tbl->rec_cap    = 0u;
        }

        /*
         * Normalize the header geometry to the +1 (lone 0x0D) terminator form
         * that dbf_flush ALWAYS emits, so a subsequent flush stays
         * self-consistent: dbf_flush writes header + descriptors + ONE 0x0D
         * + records, placing records at DBF_HDR_SIZE + 32*nfields + 1. If we
         * kept a +2-form header_length (genuine III+ files exist: TEST1C/2C.DBF),
         * the flushed records would no longer sit at the stored header_length and
         * a re-open would read garbage. We already captured the records above
         * from the original offset, so re-pointing header_length is loss-free.
         * Ref: dbf.md sec 4 (+1/+2 forms); dbf_flush (+1 emitter); dbf_create.
         */
        tbl->header_length = (uint16_t)((uint32_t)DBF_HDR_SIZE + descbytes + 1u);
        tbl->term_extra    = 1u;
    }

    *out = tbl;
    return DBF_OK;
}

/*
 * dbf_open: open an existing .dbf read-only. Thin wrapper over dbf_open_common
 * (PAL_RD, want_writable=0). The shared parse path keeps dbf_open and dbf_open_rw
 * from drifting (Rule 3 / 7az.16 contract).
 */
int dbf_open(samir_pal_t *pal, const char *name, dbf_table **out)
{
    return dbf_open_common(pal, name, out, PAL_RD, /*want_writable=*/0);
}

/*
 * dbf_open_rw: open an existing .dbf read-write (7az.16). Same parse + validation
 * as dbf_open, but opens PAL_RDWR, loads the on-disk records into rec_region, and
 * sets the writable flag so REPLACE/APPEND/DELETE/RECALL/PACK/ZAP + dbf_flush work
 * after a plain USE -- no dbf_create / wa_adopt_table seam required.
 * Fails loud (Rule 2) on a missing/unopenable file (teardown -> -DBF_ERR_IO) or a
 * malformed header, identically to dbf_open.
 */
int dbf_open_rw(samir_pal_t *pal, const char *name, dbf_table **out)
{
    return dbf_open_common(pal, name, out, PAL_RDWR, /*want_writable=*/1);
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

/* ======================================================================== */
/* S1.4: deterministic write + round-trip                                   */
/*                                                                          */
/* Write model: build-in-arena, flush-to-disk (dbf.h S1.4 note). dbf_create  */
/* lays out the header + descriptor metadata + opens the file; records are    */
/* formatted into an arena record region by dbf_append_rec; dbf_flush         */
/* serializes the whole image in one deterministic byte stream (Rule 11).     */
/*                                                                          */
/* Determinism (Rule 11): no malloc-address or wall-clock leakage. The only   */
/* date is pal->today() (INJECTABLE). Every NORMALIZE byte                    */
/* (spec/samir/dbf_normalization.json) is written 0x00. Same inputs + same    */
/* injected date => byte-identical file every run.                            */
/* ======================================================================== */

/*
 * fmt_field: format one xb_val `v` into `dst[0..flen-1]` per the field type
 * `ftype`/`dec` (dbf.md sec 5 on-disk encodings). `dst` is exactly flen bytes.
 *
 *   C : raw bytes of an xb_c/xb_m payload, left-justified, space-padded to flen
 *       (truncated if longer). xb_u / wrong type -> all spaces.
 *   N : right-justified ASCII via dec_format(value, flen, dec) (overflow ->
 *       dec_format fills '*'). xb_u / wrong type -> all spaces (blank N).
 *   D : 8-char YYYYMMDD from the JDN in an xb_d. xb_u / wrong type -> 8 spaces
 *       (blank date). (flen is always 8 for D, asserted at create.)
 *   L : 'T' for a true xb_l, 'F' for false. xb_u -> '?'. (flen is always 1.)
 *   M : 10-byte right-justified ASCII block number. xb_m payload carries the
 *       raw 10-byte pointer (copied verbatim); xb_n carries a numeric block
 *       number (formatted right-justified, 0-padded? no -- space-padded per
 *       dbf.md sec 5 M "right-justified, space-padded"). xb_u / 0 -> 10 spaces
 *       (no memo). (flen is always 10 for M.)
 *
 * Ref: dbf.md sec 5 (C/N/D/L/M encodings); rt.h dec_format; value.h xb_val.
 */
static void fmt_field(uint8_t *dst, uint8_t flen, char ftype, uint8_t dec,
                      const xb_val *v)
{
    uint32_t i;

    switch ((unsigned char)ftype) {

    case (unsigned char)'C': {
        /* Left-justified, space-padded (dbf.md sec 5 C). */
        rt_memset(dst, ' ', (uint32_t)flen);
        if (v && (v->t == XB_C || v->t == XB_M) && v->u.c.p) {
            uint32_t n = (uint32_t)v->u.c.len;
            if (n > (uint32_t)flen)
                n = (uint32_t)flen;   /* truncate to field width */
            rt_memcpy(dst, v->u.c.p, n);
        }
        break;
    }

    case (unsigned char)'N': {
        /* Right-justified ASCII; dec_format writes exactly flen bytes and
         * '*'-fills on overflow (rt.h; dbf.md sec 5 N). Blank/wrong -> spaces. */
        if (v && v->t == XB_N) {
            (void)dec_format(v->u.n, (int)flen, (int)dec, (char *)dst);
        } else {
            rt_memset(dst, ' ', (uint32_t)flen);   /* blank numeric */
        }
        break;
    }

    case (unsigned char)'D': {
        /* 8-char YYYYMMDD from the JDN; blank (8 spaces) for xb_u/wrong type.
         * dbf.md sec 5 D. flen is 8 (create-asserted). */
        if (v && v->t == XB_D) {
            int32_t y, m, d;
            char    buf[8];
            ymd_from_jdn((int32_t)v->u.d, &y, &m, &d);
            /* YYYY MM DD as zero-padded ASCII (no separators). dec_format would
             * right-justify with spaces; dates need leading zeros, so emit
             * digits directly. */
            buf[0] = (char)('0' + (int)((y / 1000) % 10));
            buf[1] = (char)('0' + (int)((y / 100) % 10));
            buf[2] = (char)('0' + (int)((y / 10) % 10));
            buf[3] = (char)('0' + (int)(y % 10));
            buf[4] = (char)('0' + (int)((m / 10) % 10));
            buf[5] = (char)('0' + (int)(m % 10));
            buf[6] = (char)('0' + (int)((d / 10) % 10));
            buf[7] = (char)('0' + (int)(d % 10));
            for (i = 0; i < 8u && i < (uint32_t)flen; i++)
                dst[i] = (uint8_t)buf[i];
            for (; i < (uint32_t)flen; i++)
                dst[i] = (uint8_t)' ';
        } else {
            rt_memset(dst, ' ', (uint32_t)flen);   /* blank date */
        }
        break;
    }

    case (unsigned char)'L': {
        /* One byte: 'T'/'F'; xb_u -> '?'. dbf.md sec 5 L. flen is 1. */
        uint8_t b;
        if (v && v->t == XB_L)
            b = v->u.l ? (uint8_t)'T' : (uint8_t)'F';
        else
            b = (uint8_t)'?';   /* uninitialised */
        rt_memset(dst, ' ', (uint32_t)flen);
        if (flen >= 1u)
            dst[0] = b;
        break;
    }

    case (unsigned char)'M': {
        /* 10-byte right-justified ASCII block number; blank (10 spaces) for
         * xb_u / no memo. dbf.md sec 5 M. flen is 10 (create-asserted).
         *   xb_m: copy the raw 10-byte pointer verbatim (round-trips the
         *         on-disk pointer the reader handed back).
         *   xb_n: format the numeric block number right-justified.
         *   xb_u/other: 10 spaces (no memo). */
        rt_memset(dst, ' ', (uint32_t)flen);
        if (v && v->t == XB_M && v->u.c.p) {
            uint32_t n = (uint32_t)v->u.c.len;
            if (n > (uint32_t)flen)
                n = (uint32_t)flen;
            /* right-justify the raw pointer bytes (already right-justified on
             * disk, but copy into the low `n` bytes to be safe). */
            rt_memcpy(dst + ((uint32_t)flen - n), v->u.c.p, n);
        } else if (v && v->t == XB_N && v->u.n >= 0.5) {
            (void)dec_format(v->u.n, (int)flen, 0, (char *)dst);
        }
        /* else: 10 spaces -> no memo */
        break;
    }

    default:
        /* dbf_create rejects non-C/N/D/L/M types; fail safe with spaces. */
        rt_memset(dst, ' ', (uint32_t)flen);
        break;
    }
}

int dbf_create(samir_pal_t *pal, const char *name,
               const dbf_field_spec *fields, int nfields, dbf_table **out)
{
    void       *mark;
    dbf_table  *tbl;
    pal_fd      fd;
    uint32_t    sum_lens;     /* 1 + sum(field lengths) */
    uint32_t    hdr_len;      /* DBF_HDR_SIZE + 32*nfields + 1 (the +1 form) */
    int         has_memo;
    int         fi;

    if (out)
        *out = (dbf_table *)0;
    if (!pal || !name || !fields || !out)
        return -DBF_ERR_IO;
    if (nfields < 1 || nfields > DBF_MAX_FIELDS)
        return -DBF_ERR_TOO_MANY;

    /* Validate the schema BEFORE touching the arena/file (fail loud, Rule 2). */
    sum_lens = (uint32_t)DBF_REC_FLAG_SIZE;
    has_memo = 0;
    for (fi = 0; fi < nfields; fi++) {
        char    t   = fields[fi].type;
        uint8_t len = fields[fi].field_len;
        uint8_t dec = fields[fi].dec;

        switch ((unsigned char)t) {
        case (unsigned char)'C':
            if (len < 1u || dec != 0u) return -DBF_ERR_BAD_TYPE;
            break;
        case (unsigned char)'N':
            if (len < 1u) return -DBF_ERR_BAD_RECLEN;
            /* dec must leave room for at least one integer digit + '.' when >0;
             * dbf.md sec 5 N: width includes sign + '.'. Be permissive on the
             * exact split (dec_format '*'-fills overflow), require dec < len. */
            if (dec != 0u && (uint32_t)dec + 1u >= (uint32_t)len)
                return -DBF_ERR_BAD_RECLEN;
            break;
        case (unsigned char)'D':
            if (len != 8u || dec != 0u) return -DBF_ERR_BAD_TYPE;
            break;
        case (unsigned char)'L':
            if (len != 1u || dec != 0u) return -DBF_ERR_BAD_TYPE;
            break;
        case (unsigned char)'M':
            if (len != 10u || dec != 0u) return -DBF_ERR_BAD_TYPE;
            has_memo = 1;
            break;
        default:
            return -DBF_ERR_BAD_TYPE;   /* III+ only (plan Sec 2.C) */
        }
        sum_lens += (uint32_t)len;
    }

    /* record_length and header_length must fit in u16 (the header stores them
     * as u16 LE; dbf.md sec 2). */
    if (sum_lens > 0xFFFFu)
        return -DBF_ERR_BAD_RECLEN;
    hdr_len = (uint32_t)DBF_HDR_SIZE + (uint32_t)DBF_DESC_STRIDE * (uint32_t)nfields + 1u;
    if (hdr_len > 0xFFFFu)
        return -DBF_ERR_BAD_HDRLEN;

    /* Allocate the table from the arena; mark first so teardown unwinds it. */
    mark = pal->alloc(pal, 0);
    tbl  = (dbf_table *)pal->alloc(pal, (uint32_t)sizeof(*tbl));
    if (!tbl)
        return -DBF_ERR_NOMEM;
    rt_memset(tbl, 0, (uint32_t)sizeof(*tbl));
    tbl->pal  = pal;
    tbl->mark = mark;
    tbl->fd   = -1;

    /* Capture header values. Version 0x83 iff any memo field, else 0x03
     * (dbf.md sec 3). NORMALIZE bytes are emitted at flush time as 0x00. */
    tbl->version       = has_memo ? (uint8_t)DBF_VERSION_WITH_MEMO
                                  : (uint8_t)DBF_VERSION_NO_MEMO;
    tbl->nrec          = 0u;
    tbl->record_length = (uint16_t)sum_lens;
    tbl->header_length = (uint16_t)hdr_len;
    tbl->nfields       = (uint16_t)nfields;
    tbl->term_extra    = 1u;   /* S1.4 always emits the +1 (lone 0x0D) form */
    tbl->writable      = 1u;

    /* Injected last-update date (INJECTABLE clock; Rule 11). */
    pal->today(pal, &tbl->year, &tbl->month, &tbl->day);

    /* Decode the schema into the dbf_field_t array (same shape S1.2 builds). */
    {
        uint32_t fsize  = (uint32_t)nfields * (uint32_t)sizeof(dbf_field_t);
        dbf_field_t *fa = (dbf_field_t *)pal->alloc(pal, fsize);
        if (!fa)
            return teardown(tbl, -DBF_ERR_NOMEM);
        rt_memset(fa, 0, fsize);
        tbl->fields = fa;

        for (fi = 0; fi < nfields; fi++) {
            const char *nm = fields[fi].name;
            uint32_t k;
            /* Copy name to first NUL, max DBF_DESC_NAME_SIZE-1 usable bytes, then
             * NUL-terminate. dbf.md sec 4 (up to 10 usable + NUL). */
            for (k = 0u; nm && nm[k] != '\0' &&
                         k < (uint32_t)(DBF_DESC_NAME_SIZE - 1); k++)
                fa[fi].name[k] = nm[k];
            fa[fi].name[k] = '\0';
            fa[fi].type      = fields[fi].type;
            fa[fi].field_len = fields[fi].field_len;
            fa[fi].dec_count = fields[fi].dec;
        }
    }

    /* Allocate the reusable record-read buffer (record_length bytes) so the
     * table can be read back (dbf_read_rec) after flush without re-open. */
    {
        uint8_t *rb = (uint8_t *)pal->alloc(pal, (uint32_t)tbl->record_length);
        if (!rb)
            return teardown(tbl, -DBF_ERR_NOMEM);
        tbl->rec_buf = rb;
    }

    /* Open the file for read/write, creating/truncating it (PAL contract). */
    fd = pal->open(pal, name, PAL_RDWR | PAL_CREATE | PAL_TRUNC);
    if (fd < 0)
        return teardown(tbl, -DBF_ERR_IO);
    tbl->fd = fd;

    /* The record region is allocated lazily on the first append (we do not know
     * the record count at create time). rec_region stays NULL until then. */
    tbl->rec_region = (uint8_t *)0;
    tbl->rec_cap    = 0u;

    *out = tbl;
    return DBF_OK;
}

int dbf_append_rec(dbf_table *tbl, const xb_val *in, int deleted)
{
    uint8_t  *rec;
    uint32_t  field_off;
    uint32_t  fi;

    if (!tbl || !in)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;   /* read-only (dbf_open) table */
    if (tbl->nrec >= DBF_MAX_RECS)
        return -DBF_ERR_TOO_MANY;

    /*
     * Grow the record region if needed. We bump-allocate from the arena; since
     * the arena is a pure bump allocator we must allocate the region contiguous.
     * Strategy: allocate the region once with a growable doubling policy. On the
     * first append, reserve an initial capacity; if exhausted, allocate a new
     * (larger) region and copy. This keeps the region contiguous for the single
     * flush write. (Arena waste is acceptable for S1.4's small test tables.)
     */
    if (tbl->nrec >= tbl->rec_cap) {
        uint32_t new_cap = (tbl->rec_cap == 0u) ? 16u : (tbl->rec_cap * 2u);
        uint32_t bytes   = new_cap * (uint32_t)tbl->record_length;
        uint8_t *nr      = (uint8_t *)tbl->pal->alloc(tbl->pal, bytes);
        if (!nr)
            return -DBF_ERR_NOMEM;
        if (tbl->rec_region && tbl->nrec > 0u)
            rt_memcpy(nr, tbl->rec_region,
                      tbl->nrec * (uint32_t)tbl->record_length);
        tbl->rec_region = nr;
        tbl->rec_cap    = new_cap;
    }

    rec = tbl->rec_region + tbl->nrec * (uint32_t)tbl->record_length;

    /* Delete flag (dbf.md sec 6). */
    rec[0] = deleted ? (uint8_t)DBF_REC_DELETE_DELETED
                     : (uint8_t)DBF_REC_DELETE_LIVE;

    /* Format each field in descriptor order, packed with no separators. */
    field_off = (uint32_t)DBF_REC_FLAG_SIZE;
    for (fi = 0u; fi < (uint32_t)tbl->nfields; fi++) {
        const dbf_field_t *f = &tbl->fields[fi];
        fmt_field(rec + field_off, f->field_len, f->type, f->dec_count, &in[fi]);
        field_off += (uint32_t)f->field_len;
    }

    tbl->nrec++;
    return DBF_OK;
}

int dbf_flush(dbf_table *tbl)
{
    uint8_t   hdr[DBF_HDR_SIZE];
    uint8_t   desc[DBF_DESC_STRIDE];
    uint8_t   term;
    uint8_t   eof;
    int32_t   pos;
    int32_t   wr;
    uint32_t  fi;
    uint8_t   memo_bit;

    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;   /* read-only (dbf_open) table is not flushable */

    /* Seek to the start; we always rewrite the whole file (TRUNC at create). */
    pos = tbl->pal->seek(tbl->pal, tbl->fd, 0, PAL_SEEK_SET);
    if (pos != 0)
        return -DBF_ERR_IO;

    /* --- 32-byte header (dbf.md sec 2). EVERY non-MEANINGFUL byte is 0x00
     * (spec/samir/dbf_normalization.json): 0x0C..0x1F all stay zero from the
     * rt_memset; we only set the MEANINGFUL fields. --- */
    rt_memset(hdr, 0, DBF_HDR_SIZE);

    /* Version: 0x03 or 0x83. The memo bit comes from DBF_MEMO_VERSION_BIT so the
     * DBF_MUTATE_VERSION mutant drops it (Rule 6). tbl->version already encodes
     * has_memo, but we recompute the bit so the mutant perturbation is local. */
    memo_bit = (tbl->version & 0x80u) ? DBF_MEMO_VERSION_BIT : 0x00u;
    hdr[DBF_HDR_VERSION_OFF] = (uint8_t)(DBF_VERSION_NO_MEMO | memo_bit);

    hdr[DBF_HDR_YEAR_OFF]  = tbl->year;   /* from pal->today() (Rule 11) */
    hdr[DBF_HDR_MONTH_OFF] = tbl->month;
    hdr[DBF_HDR_DAY_OFF]   = tbl->day;
    wr_u32le(hdr + DBF_HDR_NREC_OFF,       tbl->nrec);
    wr_u16le(hdr + DBF_HDR_HEADER_LEN_OFF, tbl->header_length);
    wr_u16le(hdr + DBF_HDR_RECORD_LEN_OFF, tbl->record_length);
    /* hdr[0x0C..0x1F] remain 0x00 (NORMALIZE: reserved/MDX/LDID/multiuser). */

    wr = tbl->pal->write(tbl->pal, tbl->fd, hdr, DBF_HDR_SIZE);
    if (wr != (int32_t)DBF_HDR_SIZE)
        return -DBF_ERR_IO;

    /* --- field descriptors (dbf.md sec 4). NORMALIZE bytes (RAM addr 0x0C,
     * work-area 0x14, all reserved) stay 0x00 from the rt_memset. --- */
    for (fi = 0u; fi < (uint32_t)tbl->nfields; fi++) {
        const dbf_field_t *f = &tbl->fields[fi];
        uint32_t k;

        rt_memset(desc, 0, DBF_DESC_STRIDE);

        /* Name: bytes up to first NUL, NUL-padded (rt_memset already zeroed). */
        for (k = 0u; f->name[k] != '\0' &&
                     k < (uint32_t)DBF_DESC_NAME_SIZE; k++)
            desc[DBF_DESC_NAME_OFF + k] = (uint8_t)f->name[k];

        desc[DBF_DESC_TYPE_OFF]      = (uint8_t)f->type;
        /* desc 0x0C..0x0F (field RAM address): 0x00000000 (NORMALIZE). */
        desc[DBF_DESC_FIELD_LEN_OFF] = f->field_len;
        desc[DBF_DESC_DEC_COUNT_OFF] = f->dec_count;
        /* desc 0x14 work-area id, 0x12/0x15/0x17/0x18/0x1F: 0x00 (NORMALIZE). */

        wr = tbl->pal->write(tbl->pal, tbl->fd, desc, DBF_DESC_STRIDE);
        if (wr != (int32_t)DBF_DESC_STRIDE)
            return -DBF_ERR_IO;
    }

    /* --- the lone 0x0D terminator (the +1 form; dbf.md sec 4). --- */
    term = (uint8_t)DBF_DESC_TERMINATOR;
    wr = tbl->pal->write(tbl->pal, tbl->fd, &term, 1u);
    if (wr != 1)
        return -DBF_ERR_IO;

    /* --- the record region: nrec records, record_length bytes each. --- */
    if (tbl->nrec > 0u) {
        uint32_t total = tbl->nrec * (uint32_t)tbl->record_length;
        wr = tbl->pal->write(tbl->pal, tbl->fd, tbl->rec_region, total);
        if (wr != (int32_t)total)
            return -DBF_ERR_IO;
    }

    /* --- trailing 0x1A EOF byte (S1.4 decision: emit it; dbf.md sec 8 optional;
     * NOT counted in header_length/record_length; a reader must not require it). */
    eof = (uint8_t)DBF_EOF_MARKER;
    wr = tbl->pal->write(tbl->pal, tbl->fd, &eof, 1u);
    if (wr != 1)
        return -DBF_ERR_IO;

    /* Reposition to the file start so the table can be read back without
     * re-open (the open handle is RDWR). */
    pos = tbl->pal->seek(tbl->pal, tbl->fd, 0, PAL_SEEK_SET);
    if (pos != 0)
        return -DBF_ERR_IO;

    return DBF_OK;
}

/* ======================================================================== */
/* S1.5: record mutation verbs                                               */
/*                                                                          */
/* Record-cursor model: EXPLICIT recno arguments (1-based, matching         */
/* dbf_read_rec). Cleaner for testing; S5.1 adds a work-area current-recno. */
/*                                                                          */
/* Write model: for dbf_create()'d (writable) tables all verbs patch the    */
/* in-arena rec_region directly; the caller calls dbf_flush() afterwards to */
/* persist the changes. This is consistent with S1.4's build-in-arena/flush */
/* model and keeps a single code path for the deterministic write.          */
/*                                                                          */
/* Blank-field fill (dbf.md sec 5 + plan S1.5):                            */
/*   C/M -> spaces; N -> spaces; D -> spaces; L -> '?' (uninitialised).    */
/*                                                                          */
/* Assignment coercion (xbase_coercion.json assignment_coercion, self-      */
/* contained; no eval.c dependency per plan S1.5):                          */
/*   C<-C ok (truncate/pad); N<-N ok (dec_format, stars on overflow);       */
/*   D<-D ok; L<-L ok; cross-type -> -DBF_ERR_MISMATCH (fail loud).        */
/* ======================================================================== */

/*
 * S1.5 MUTATION HOOK (Rule 6): when -DDBF_MUTATE_DELFLAG is defined,
 * dbf_delete writes 0x20 (live) instead of 0x2A (deleted). The mutation
 * perturbs a single constant: the byte written to the delete-flag slot.
 * The test asserts that after dbf_delete the flag byte reads back as 0x2A;
 * with the mutant it reads 0x20, so every test that checks the deleted state
 * goes RED. Exactly one constant is changed; dbf_recall is unaffected.
 *
 * Why this bites hardest: the delete flag is the gate for dbf_pack. If
 * dbf_delete silently writes 0x20, pack cannot identify deleted records and
 * the nrec check, survivor-count check, and content checks all fail.
 * Ref: plan S1.5 mutant spec "dbf_delete writes 0x20 instead of 0x2A";
 *      dbf.md sec 6; spec/samir/dbf_format.h DBF_REC_DELETE_DELETED=0x2A.
 */
#ifdef DBF_MUTATE_DELFLAG
#  define DELETE_FLAG_BYTE  ((uint8_t)DBF_REC_DELETE_LIVE)   /* mutant: 0x20 */
#else
#  define DELETE_FLAG_BYTE  ((uint8_t)DBF_REC_DELETE_DELETED) /* correct: 0x2A */
#endif

/*
 * rec_ptr: return a pointer to the start of the record at 1-based recno in
 * the writable rec_region (recno 1 -> rec_region[0]).
 * NO range-checking here; callers validate recno before calling.
 */
static uint8_t *rec_ptr(dbf_table *tbl, uint32_t recno)
{
    return tbl->rec_region + (recno - 1u) * (uint32_t)tbl->record_length;
}

/*
 * field_offset_in_rec: return the byte offset of field `fi` (0-based) within
 * a record buffer (i.e. from the record's first byte = the delete flag). This
 * is DBF_REC_FLAG_SIZE + sum(field_len[0..fi-1]).
 */
static uint32_t field_offset_in_rec(const dbf_table *tbl, int fi)
{
    uint32_t off = (uint32_t)DBF_REC_FLAG_SIZE;
    int k;
    for (k = 0; k < fi; k++)
        off += (uint32_t)tbl->fields[k].field_len;
    return off;
}

/*
 * fmt_blank_field: fill `dst[0..flen-1]` with the blank value for `ftype`.
 *   C/M/N/D -> spaces (0x20).
 *   L       -> '?' (0x3F) -- the III+ uninitialised logical value.
 * Ref: dbf.md sec 5 (L uninitialized = '?'); plan S1.5 blank-field spec.
 */
static void fmt_blank_field(uint8_t *dst, uint8_t flen, char ftype)
{
    if ((unsigned char)ftype == (unsigned char)'L' && flen >= 1u) {
        rt_memset(dst, ' ', (uint32_t)flen);
        dst[0] = (uint8_t)'?';
    } else {
        rt_memset(dst, ' ', (uint32_t)flen);
    }
}

int dbf_append_blank(dbf_table *tbl)
{
    uint8_t  *rec;
    uint32_t  fi;
    uint32_t  field_off;

    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;   /* opened read-only; S5.1 handles RDWR open */
    if (tbl->nrec >= DBF_MAX_RECS)
        return -DBF_ERR_TOO_MANY;

    /* Grow the record region if needed (mirrors dbf_append_rec logic). */
    if (tbl->nrec >= tbl->rec_cap) {
        uint32_t new_cap = (tbl->rec_cap == 0u) ? 16u : (tbl->rec_cap * 2u);
        uint32_t bytes   = new_cap * (uint32_t)tbl->record_length;
        uint8_t *nr      = (uint8_t *)tbl->pal->alloc(tbl->pal, bytes);
        if (!nr)
            return -DBF_ERR_NOMEM;
        if (tbl->rec_region && tbl->nrec > 0u)
            rt_memcpy(nr, tbl->rec_region,
                      tbl->nrec * (uint32_t)tbl->record_length);
        tbl->rec_region = nr;
        tbl->rec_cap    = new_cap;
    }

    rec = rec_ptr(tbl, tbl->nrec + 1u);   /* zero-based: nrec is count BEFORE bump */

    /* Delete flag: 0x20 (live). Ref: dbf.md sec 6. */
    rec[0] = (uint8_t)DBF_REC_DELETE_LIVE;

    /* Blank-fill each field in descriptor order. */
    field_off = (uint32_t)DBF_REC_FLAG_SIZE;
    for (fi = 0u; fi < (uint32_t)tbl->nfields; fi++) {
        const dbf_field_t *f = &tbl->fields[fi];
        fmt_blank_field(rec + field_off, f->field_len, f->type);
        field_off += (uint32_t)f->field_len;
    }

    tbl->nrec++;
    return DBF_OK;
}

/*
 * coerce_and_write_field: apply assignment coercion and write the formatted
 * bytes into `dst[0..flen-1]` for the given field type/dec. Returns DBF_OK
 * on success or -DBF_ERR_MISMATCH on a cross-type incompatibility.
 *
 * Assignment coercion contract (xbase_coercion.json assignment_coercion; self-
 * contained per plan S1.5 -- no eval.c):
 *   C <- C : truncate to flen if too long; space-pad if too short.
 *   N <- N : dec_format(v->u.n, flen, dec, dst). Overflow -> '*'-fill; DBF_OK.
 *   D <- D : write YYYYMMDD; xb_u -> blank (8 spaces).
 *   L <- L : 'T' for 1, 'F' for 0; xb_u -> '?'.
 *   M <- C : memo text stored as C (xbase_coercion.json "target:M,expr:C,ok").
 *   cross-type (C<-N, N<-C, N<-D, D<-C, L<-N, etc.) -> -DBF_ERR_MISMATCH.
 *
 * Ref: xbase_coercion.json assignment_coercion; dbf.md sec 5; rt.h dec_format,
 *      ymd_from_jdn; plan S1.5.
 */
static int coerce_and_write_field(uint8_t *dst, uint8_t flen, char ftype,
                                  uint8_t dec, const xb_val *v)
{
    uint32_t i;

    switch ((unsigned char)ftype) {

    case (unsigned char)'C': {
        /* C <- C: ok. Truncate/pad. Ref: xbase_coercion.json target:C, expr:C. */
        if (!v || v->t != XB_C)
            return -DBF_ERR_MISMATCH;
        rt_memset(dst, ' ', (uint32_t)flen);
        if (v->u.c.p) {
            uint32_t n = (uint32_t)v->u.c.len;
            if (n > (uint32_t)flen) n = (uint32_t)flen;  /* truncate */
            rt_memcpy(dst, v->u.c.p, n);
        }
        return DBF_OK;
    }

    case (unsigned char)'N': {
        /* N <- N: ok; dec_format handles overflow by '*'-fill.
         * Ref: xbase_coercion.json target:N, expr:N, on_overflow:stars_fill. */
        if (!v || v->t != XB_N)
            return -DBF_ERR_MISMATCH;
        (void)dec_format(v->u.n, (int)flen, (int)dec, (char *)dst);
        return DBF_OK;
    }

    case (unsigned char)'D': {
        /* D <- D: ok. xb_u -> 8 spaces (blank date).
         * Ref: xbase_coercion.json target:D, expr:D. dbf.md sec 5 D. */
        if (!v || (v->t != XB_D && v->t != XB_U))
            return -DBF_ERR_MISMATCH;
        if (v->t == XB_D) {
            int32_t y, m, d;
            ymd_from_jdn((int32_t)v->u.d, &y, &m, &d);
            i = 0u;
            dst[i++] = (uint8_t)('0' + (int)((y / 1000) % 10));
            dst[i++] = (uint8_t)('0' + (int)((y / 100) % 10));
            dst[i++] = (uint8_t)('0' + (int)((y / 10) % 10));
            dst[i++] = (uint8_t)('0' + (int)(y % 10));
            dst[i++] = (uint8_t)('0' + (int)((m / 10) % 10));
            dst[i++] = (uint8_t)('0' + (int)(m % 10));
            dst[i++] = (uint8_t)('0' + (int)((d / 10) % 10));
            dst[i]   = (uint8_t)('0' + (int)(d % 10));
        } else {
            /* xb_u -> blank date (8 spaces). flen is 8 (asserted at create). */
            rt_memset(dst, ' ', (uint32_t)flen);
        }
        return DBF_OK;
    }

    case (unsigned char)'L': {
        /* L <- L: ok. 'T'/'F'. xb_u -> '?'.
         * Ref: xbase_coercion.json target:L, expr:L. dbf.md sec 5 L. */
        if (!v || (v->t != XB_L && v->t != XB_U))
            return -DBF_ERR_MISMATCH;
        rt_memset(dst, ' ', (uint32_t)flen);
        if (flen >= 1u) {
            if (v->t == XB_L)
                dst[0] = v->u.l ? (uint8_t)'T' : (uint8_t)'F';
            else
                dst[0] = (uint8_t)'?';
        }
        return DBF_OK;
    }

    case (unsigned char)'M': {
        /* M <- C: ok (xbase_coercion.json target:M, expr:C, ok, "memo stores text").
         * Write as left-justified, space-padded C-style into the 10-byte M slot
         * (the .dbt block pointer slot). The full .dbt integration is S2.2;
         * here we treat an M field like C for the field bytes. */
        if (!v || v->t != XB_C)
            return -DBF_ERR_MISMATCH;
        rt_memset(dst, ' ', (uint32_t)flen);
        if (v->u.c.p) {
            uint32_t n = (uint32_t)v->u.c.len;
            if (n > (uint32_t)flen) n = (uint32_t)flen;
            rt_memcpy(dst, v->u.c.p, n);
        }
        return DBF_OK;
    }

    default:
        return -DBF_ERR_MISMATCH;
    }
}

int dbf_replace(dbf_table *tbl, uint32_t recno, int field, const xb_val *v)
{
    uint8_t  *rec;
    uint8_t  *dst;
    uint32_t  foff;
    const dbf_field_t *f;

    if (!tbl || !v)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;

    /* Validate recno (1-based, [1..nrec]). Fail loud. */
    if (recno < 1u || recno > tbl->nrec)
        return -DBF_ERR_BAD_RECNO;

    /* Validate field index (0-based, [0..nfields-1]). Fail loud. */
    if (field < 0 || (uint32_t)field >= (uint32_t)tbl->nfields)
        return -DBF_ERR_BAD_RECNO;   /* reuse BAD_RECNO for out-of-range field */

    f    = &tbl->fields[field];
    foff = field_offset_in_rec(tbl, field);
    rec  = rec_ptr(tbl, recno);
    dst  = rec + foff;

    /* Apply assignment coercion and write the formatted bytes.
     * Returns DBF_OK or -DBF_ERR_MISMATCH for cross-type. */
    return coerce_and_write_field(dst, f->field_len, f->type, f->dec_count, v);
}

int dbf_delete(dbf_table *tbl, uint32_t recno)
{
    uint8_t *rec;

    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;
    if (recno < 1u || recno > tbl->nrec)
        return -DBF_ERR_BAD_RECNO;

    rec = rec_ptr(tbl, recno);
    /* Write the delete flag. DELETE_FLAG_BYTE is 0x2A normally; 0x20 with
     * -DDBF_MUTATE_DELFLAG (the mutation hook; Rule 6). */
    rec[0] = DELETE_FLAG_BYTE;
    return DBF_OK;
}

int dbf_recall(dbf_table *tbl, uint32_t recno)
{
    uint8_t *rec;

    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;
    if (recno < 1u || recno > tbl->nrec)
        return -DBF_ERR_BAD_RECNO;

    rec = rec_ptr(tbl, recno);
    rec[0] = (uint8_t)DBF_REC_DELETE_LIVE;   /* clear the delete flag */
    return DBF_OK;
}

int dbf_pack(dbf_table *tbl)
{
    uint8_t  *new_region;
    uint32_t  new_nrec;
    uint32_t  r;
    uint32_t  rlen;

    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;

    rlen    = (uint32_t)tbl->record_length;
    new_nrec = 0u;

    /* Count survivors first so we can allocate the exact size. */
    for (r = 0u; r < tbl->nrec; r++) {
        uint8_t *rec = tbl->rec_region + r * rlen;
        if (rec[0] == (uint8_t)DBF_REC_DELETE_LIVE)
            new_nrec++;
    }

    if (new_nrec == 0u) {
        /* All records deleted (or table was already empty). Just zap. */
        tbl->nrec = 0u;
        return DBF_OK;
    }

    /* Allocate a fresh region for the survivors. */
    new_region = (uint8_t *)tbl->pal->alloc(tbl->pal, new_nrec * rlen);
    if (!new_region)
        return -DBF_ERR_NOMEM;

    /* Copy survivors in original record order (deterministic; Rule 11).
     * A deleted record (flag == 0x2A) is skipped. Any other flag byte
     * that survived earlier validation is treated as live (defensive). */
    {
        uint32_t dst_idx = 0u;
        for (r = 0u; r < tbl->nrec; r++) {
            uint8_t *rec = tbl->rec_region + r * rlen;
            if (rec[0] == (uint8_t)DBF_REC_DELETE_LIVE) {
                rt_memcpy(new_region + dst_idx * rlen, rec, rlen);
                dst_idx++;
            }
        }
    }

    tbl->rec_region = new_region;
    tbl->rec_cap    = new_nrec;
    tbl->nrec       = new_nrec;
    return DBF_OK;
}

int dbf_zap(dbf_table *tbl)
{
    if (!tbl)
        return -DBF_ERR_IO;
    if (!tbl->writable)
        return -DBF_ERR_IO;

    /* Reset the record count to 0. The rec_region is left as-is in the arena
     * (bump allocator; no free needed). dbf_flush will write 0 records. */
    tbl->nrec = 0u;
    return DBF_OK;
}

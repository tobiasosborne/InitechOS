/*
 * os/samir/include/samir/dbf.h -- SAMIR (InitechBase) .dbf table contract.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib) alongside the shipped SAMIR engine. Depends ONLY on <stdint.h>
 * and samir/pal.h (which itself depends only on <stdint.h>). No libc headers.
 *
 * S1.1 (docs/plans/SAMIR-implementation-plan.md): the dBASE III PLUS 1.1 .dbf
 * header parser + structural invariants.
 *   - dbf_open  : open a .dbf via the PAL, read + validate the 32-byte header,
 *                 derive the field count and terminator form, assert the three
 *                 structural invariants (fail loud, Rule 2), keep the handle
 *                 open for the record area.
 *   - dbf_close : release the table + its PAL handle.
 *   - accessors : every header field S1.1 parses (version, date, nrec,
 *                 header_length, record_length, nfields, term_extra, has_memo).
 *
 * S1.2 (docs/plans/SAMIR-implementation-plan.md): field-descriptor array decoder.
 *   - Decodes name/type/length/dec for all nfields descriptors at open time.
 *   - dbf_field(tbl, i) : return the i-th field descriptor (0-based), or NULL
 *                         for out-of-range i (fail loud: returns NULL only when
 *                         i >= nfields or i < 0 -- callers must check).
 *   - dbf_nfields(tbl)  : already exposed by S1.1; reused here.
 *   - dbf_field_t       : public struct holding name/type/length/dec.
 *
 * S1.1 reads ONLY the per-descriptor field-length byte (descriptor offset 0x10)
 * during its scan loop; S1.2 decodes all fields fully on the second pass (same
 * open call, second seek). S1.1 invariants are preserved; S1.2 adds no new
 * error codes (a bad field type is caught by DBF_ERR_NO_TERM is insufficient --
 * we reuse DBF_ERR_SHORT to signal a structural failure in the descriptor decode).
 *
 * S1.3 (docs/plans/SAMIR-implementation-plan.md): record read -> typed values.
 *   - dbf_read_rec : read a single record by 1-based record number, decode each
 *                    field into an xb_val, and report the delete flag.
 *   - Requires value.h (xb_val, xb_c/xb_n/xb_d/xb_l/xb_m/xb_u) and rt.h
 *                    (dec_parse, jdn_from_ymd). Both are freestanding headers.
 *
 *   Type decisions (S1.3 boundary; all cited to dbf.md sec 5 + dbf_ref.py):
 *
 *   C field  -> xb_c(ptr, field_len). Pointer into the arena record buffer;
 *               raw bytes including trailing spaces. Trimming is the evaluator's
 *               job (S3.3); raw bytes enable round-trip (S1.4). dbf.md sec 5 C:
 *               "left-justified, space-padded". Lifetime: valid until the NEXT
 *               call to dbf_read_rec on the SAME table (the record buffer is
 *               reused per call -- callers that need stable storage must copy).
 *
 *   N field  -> xb_n(dec_parse(raw, field_len)). All-spaces/empty -> 0.0
 *               (dec_parse contract, rt.h). dbf.md sec 5 N: "right-justified,
 *               space-padded". This matches dbf_ref.py decode_field N logic
 *               (None for all-spaces maps to our 0.0 -- the evaluator handles
 *               the "blank N" semantic; at the codec level 0.0 is returned).
 *               dec_parse ignores leading/trailing spaces and returns 0.0 for
 *               all-spaces; this is consistent with dbf_ref.py's None mapping.
 *               All-asterisk overflow fields: dec_parse on "*..."  returns 0.0
 *               (non-digit, non-sign, non-space char -> 0.0 per rt.h contract).
 *
 *   D field  -> Valid 8-char YYYYMMDD with non-space content: xb_d(jdn) where
 *               jdn = jdn_from_ymd(year, month, day) (rt.h). dbf.md sec 5 D.
 *               Blank date (8 spaces, or "00000000"): xb_u() with *deleted
 *               unchanged. dbf.md sec 5 D: "A blank/empty date is 8 spaces.
 *               '00000000' is not a valid date."
 *               Decision: blank date -> xb_u(). Rationale: dbf_ref.py returns
 *               "" (empty string) for blank; our typed model has no "blank date"
 *               sentinel -- xb_u() is the clean equivalent. GATED: no blank-date
 *               fixture in the corpus to force the decision; confirmed via
 *               dbf_ref.py decode_field D logic (strip -> "" -> return "").
 *
 *   L field  -> T/t/Y/y -> xb_l(1). F/f/N/n -> xb_l(0). '?'/space ->
 *               xb_u() (uninitialised). dbf.md sec 5 L; dbf_ref.py
 *               decode_field L. Decision: '?'/space -> xb_u(), not xb_l(0),
 *               because the evaluator must be able to distinguish "false" from
 *               "not set". Any other byte: fail loud with DBF_ERR_BAD_REC.
 *               Note: the corpus fixtures only exhibit T and F (no '?' seen).
 *               [verified: TRAVEL.DBF PAID column -- T/F only]
 *
 *   M field  -> S1.3->S2.1 boundary: the .dbt is NOT read here. The 10-byte
 *               ASCII block-number pointer (dbf.md sec 5 M) is exposed as
 *               xb_m(ptr, 10) pointing into the arena record buffer (same
 *               lifetime as C -- valid until next dbf_read_rec call). The
 *               caller or S2.1 uses dec_parse(ptr, 10) to recover the block
 *               number. Block 0 or all-blanks -> xb_u() (no memo for this
 *               record). Rationale: xb_m carries the raw 10-byte ASCII so
 *               that S2.1 can parse the block number and open the .dbt without
 *               any duplication of the block-number parsing logic between the
 *               codec and the memo reader. dbf_ref.py returns 0 for no-memo.
 *               Decision: block 0 -> xb_u(); block > 0 -> xb_m(raw10, 10).
 *               GATED: "10 blanks" vs right-justified 0 both mean no-memo
 *               (dbf.md sec 5 M note); both forms are treated as xb_u().
 *
 *   Delete flag: 0x20 -> *deleted=0, 0x2A -> *deleted=1. Any other byte ->
 *               fail loud with DBF_ERR_BAD_REC (Rule 2). dbf.md sec 6.
 *               [verified: byte-checked CLIENTS/TOURS/TRAVEL records: all 0x20]
 *               [documented: 0x2A = deleted, from clicketyclick + dbf_ref.py]
 *
 *   1-based recno: recno 1 is the first data record, stored at byte offset
 *               header_length + 0*record_length. This matches the dBASE
 *               convention (RECNO() returns 1 for the first record). Confirmed
 *               by dbf_ref.py iter_records which yields i=0 for rec0 (0-based
 *               internally); our dbf_read_rec(tbl, 1, ...) maps to i=0.
 *
 *   Record buffer: a PAL-arena-allocated byte array of record_length bytes
 *               allocated at dbf_open time and reused by every dbf_read_rec
 *               call. C and M xb_val.u.c.p pointers point into this buffer.
 *               LIFETIME: the data is valid only until the next dbf_read_rec
 *               call on the SAME table (the buffer is overwritten). Callers
 *               that need stable C/M data must copy it. N/D/L values are
 *               decoded and stored in the xb_val struct (no pointer into the
 *               buffer after decode).
 *
 * III+ 1.1 ONLY (plan Sec 2.C): dbf_open FAILS LOUD on dBASE IV version bytes
 * (0x04, 0x8B, and anything other than 0x03/0x83) with DBF_ERR_BAD_VERSION.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2 (32-byte header),
 *     sec 3 (version byte), sec 4 (descriptor + terminator), sec 5 (field types
 *     C/N/D/L/M -- III+ only), sec 6 (record layout: 1 delete-flag byte then
 *     fields, no NULL bitmap), sec 8 (invariants),
 *     sec 9 (implementer notes: trust header_length / record_length / nrec;
 *            field name = bytes to first NUL; trailing bytes may be garbage).
 *   - spec/samir/dbf_format.h (the LOCKED byte-offset constants):
 *     DBF_DESC_NAME_OFF/TYPE_OFF/FIELD_LEN_OFF/DEC_COUNT_OFF, DBF_DESC_STRIDE=32,
 *     DBF_DESC_NAME_SIZE=11, DBF_DESC_TERMINATOR=0x0D,
 *     DBF_REC_DELETE_LIVE=0x20, DBF_REC_DELETE_DELETED=0x2A.
 *   - docs/plans/SAMIR-implementation-plan.md S1.3 contract; Sec 8.2 dbf.h sketch.
 *   - os/samir/include/samir/pal.h (the ONLY OS surface; seek=filesize idiom).
 *   - os/samir/include/samir/value.h (xb_val; xb_c/xb_n/xb_d/xb_l/xb_m/xb_u).
 *   - os/samir/include/samir/rt.h (dec_parse, jdn_from_ymd).
 *   - harness/diff/dbf_diff/dbf_ref.py decode_field() (independent reference
 *     for field decoding logic; our typed values must agree with it).
 */
#ifndef INITECH_SAMIR_DBF_H
#define INITECH_SAMIR_DBF_H

#include <stdint.h>

#include "samir/pal.h"
#include "samir/value.h"   /* xb_val, xb_c/xb_n/xb_d/xb_l/xb_m/xb_u */

/* ---- Error codes (fail loud; Rule 2) ----
 *
 * dbf_open returns DBF_OK (0) on success or a NEGATED dbf_err on failure
 * (return -DBF_ERR_BAD_VERSION, etc.), matching the PAL convention of negated
 * error codes. The engine compares symbolically, never on magnitude.
 */
typedef enum {
    DBF_OK              = 0,  /* success */
    DBF_ERR_IO          = 1,  /* PAL open/read/seek failure */
    DBF_ERR_NOMEM       = 2,  /* PAL arena exhausted (alloc returned NULL) */
    DBF_ERR_SHORT       = 3,  /* file too short for the 32-byte header / a descriptor */
    DBF_ERR_BAD_VERSION = 4,  /* version byte not 0x03/0x83 (e.g. IV 0x04/0x8B) */
    DBF_ERR_NO_TERM     = 5,  /* no 0x0D field-descriptor terminator where expected */
    DBF_ERR_BAD_HDRLEN  = 6,  /* invariant 1: header_length terminator-extra not in {1,2} */
    DBF_ERR_BAD_RECLEN  = 7,  /* invariant 1b: record_length != 1 + sum(field lengths) */
    DBF_ERR_BAD_FILESZ  = 8,  /* invariant 2: file too short for header + nrec*reclen */
    DBF_ERR_TOO_MANY    = 9,  /* field count exceeds DBF_MAX_FIELDS */
    DBF_ERR_BAD_TYPE    = 10, /* S1.2: field type byte is not one of C/N/D/L/M (III+ only).
                               * Ref: dbf.md sec 5 (III+ types); plan Sec 2.C (fail loud). */
    DBF_ERR_BAD_RECNO   = 11, /* S1.3: recno out of range (< 1 or > nrec). */
    DBF_ERR_BAD_REC     = 12  /* S1.3: malformed record (bad delete flag or bad L byte).
                               * dbf.md sec 6: delete flag must be 0x20 or 0x2A; any other
                               * byte is corrupt data -- fail loud (Rule 2). */
} dbf_err;

/* ---- Opaque table handle ----
 *
 * The full layout lives in os/samir/fs/dbf.c. Callers (and S1.2) reach the
 * parsed values only through the accessors below.
 */
typedef struct dbf_table dbf_table;

/* ---- Open / close ---- */

/*
 * dbf_open: open the .dbf named `name` through `pal`, read and validate its
 * 32-byte header, derive the field count + terminator form, assert the three
 * dbf.md sec 8 structural invariants, and return an opaque handle in *out.
 *
 * On success returns DBF_OK and *out points to an arena-allocated dbf_table
 * whose PAL handle stays OPEN (the record area is read later by S1.2/S1.3).
 * On any failure returns -(dbf_err); *out is set to NULL and any partially
 * opened handle is closed. Fails loud (Rule 2): a malformed header, an
 * unsupported (IV) version, or a violated invariant is an error, never a
 * silently-wrong table.
 *
 * Ref: dbf.md sec 2/3/4/8/9; spec/samir/dbf_format.h.
 */
int dbf_open(samir_pal_t *pal, const char *name, dbf_table **out);

/*
 * dbf_close: close the PAL handle held by `tbl` and reset the arena to the
 * mark taken at open (freeing the table). NULL is a no-op. Returns DBF_OK or
 * -(dbf_err) if the PAL close reported a failure (the table is freed either
 * way).
 */
int dbf_close(dbf_table *tbl);

/* ---- Parsed-header accessors (S1.1 surface) ----
 *
 * All are pure reads of values captured at dbf_open time. Passing NULL is a
 * programming error; the accessors do not guard against it (the engine asserts
 * non-NULL at the call site).
 */

/* Version/flags byte (dbf.md sec 3): 0x03 (no memo) or 0x83 (with memo). */
uint8_t  dbf_version(const dbf_table *tbl);

/* Last-update date, header form (dbf.md sec 2): YY = year-1900, MM 1..12,
 * DD 1..31. Two-digit YY exactly as stored (NOT century-expanded). */
uint8_t  dbf_year(const dbf_table *tbl);   /* year - 1900 (e.g. 0x55 = 85) */
uint8_t  dbf_month(const dbf_table *tbl);
uint8_t  dbf_day(const dbf_table *tbl);

/* Number of logical records (dbf.md sec 2 offset 0x04, u32 LE). Authoritative
 * record count for reading (dbf.md sec 9). */
uint32_t dbf_nrec(const dbf_table *tbl);

/* Header length (dbf.md sec 2 offset 0x08): byte offset where record data
 * begins. Trusted, not recomputed (dbf.md sec 9). */
uint16_t dbf_header_length(const dbf_table *tbl);

/* Record length (dbf.md sec 2 offset 0x0A): bytes per record = 1 delete flag
 * + sum of field lengths. */
uint16_t dbf_record_length(const dbf_table *tbl);

/* Number of field descriptors, found by scanning to the 0x0D terminator
 * (dbf.md sec 4/8 -- the authoritative way; NOT arithmetic on header_length). */
uint16_t dbf_nfields(const dbf_table *tbl);

/* Terminator form: 1 for the "+1" (lone 0x0D) convention, 2 for the "+2"
 * (0x0D 0x00) convention (dbf.md sec 4). Both occur in genuine III+ 1.1. */
uint8_t  dbf_term_extra(const dbf_table *tbl);

/* 1 if the version byte has the memo bit (0x80) set, else 0 (dbf.md sec 3). */
int      dbf_has_memo(const dbf_table *tbl);

/* ---- S1.2: field-descriptor accessor ----
 *
 * dbf_field_t holds the decoded metadata for one field descriptor (S1.2).
 * The name is a NUL-terminated C string (at most DBF_DESC_NAME_SIZE bytes
 * including the terminating NUL) containing the field name decoded to the
 * first 0x00 byte; trailing garbage in the 11-byte name slot is discarded
 * (dbf.md sec 4 "read to first NUL; ignore trailing garbage").
 *
 * Ref (Law 1):
 *   - dbf.md sec 4 (32-byte field descriptor table + worked example CLIENTS
 *     FIRSTNAME); sec 5 (field type codes C/N/D/L/M, III+ only).
 *   - spec/samir/dbf_format.h: DBF_DESC_NAME_OFF, DBF_DESC_TYPE_OFF,
 *     DBF_DESC_FIELD_LEN_OFF, DBF_DESC_DEC_COUNT_OFF, DBF_DESC_NAME_SIZE=11,
 *     DBF_DESC_STRIDE=32.
 *   - docs/plans/SAMIR-implementation-plan.md S1.2 contract.
 */
typedef struct {
    char    name[12];   /* field name, NUL-terminated; up to 10 usable chars +
                         * the terminating NUL (DBF_DESC_NAME_SIZE=11 bytes on
                         * disk, we store one extra byte to guarantee NUL even if
                         * all 11 bytes are non-zero in a malformed file; in
                         * practice III+ always NUL-terminates within 11 bytes).
                         * Ref: dbf.md sec 4 "Field name" row. */
    char    type;       /* field type char: 'C', 'N', 'D', 'L', or 'M' (III+ only).
                         * Ref: dbf.md sec 5; spec/samir/dbf_format.h DBF_DESC_TYPE_OFF. */
    uint8_t field_len;  /* bytes the field occupies in each record (1..255).
                         * For N: includes sign + decimal point.
                         * Ref: dbf.md sec 4 offset 0x10; DBF_DESC_FIELD_LEN_OFF. */
    uint8_t dec_count;  /* decimal digit count; meaningful for N only (0 for C/D/L/M).
                         * Ref: dbf.md sec 4 offset 0x11; DBF_DESC_DEC_COUNT_OFF. */
} dbf_field_t;

/*
 * dbf_field: return a pointer to the decoded i-th field descriptor (0-based).
 *
 * Returns a non-NULL pointer to a dbf_field_t owned by the table (valid until
 * dbf_close) on success, or NULL if i is out of range (i < 0 or i >= nfields).
 * The caller MUST check for NULL (fail-loud contract: do not use a NULL return
 * as a value; it signals a programming error or out-of-bounds access, Rule 2).
 *
 * Ref: dbf.md sec 4; spec/samir/dbf_format.h; plan S1.2 contract.
 */
const dbf_field_t *dbf_field(const dbf_table *tbl, int i);

/* ---- S1.3: record read -> typed values ----
 *
 * dbf_read_rec: read and decode one record into a caller-supplied xb_val array.
 *
 * Parameters:
 *   tbl     : open table returned by dbf_open.
 *   recno   : 1-based record number (recno 1 = first data record, dBASE convention).
 *             Confirmed by dbf_ref.py which yields i=0 (0-based) for the first
 *             record; our recno 1 maps to offset header_length + 0*record_length.
 *   out     : caller-allocated array of at least dbf_nfields(tbl) xb_val slots.
 *             Each slot is filled with the decoded value for the corresponding
 *             field in descriptor order.
 *   deleted : set to 0 if the record's delete flag is 0x20 (live), 1 if 0x2A
 *             (logically deleted). May be NULL (caller does not need the flag).
 *
 * Return value: DBF_OK (0) on success, or a NEGATED dbf_err code on failure:
 *   -DBF_ERR_BAD_RECNO : recno < 1 or recno > dbf_nrec(tbl) (fail loud).
 *   -DBF_ERR_IO        : PAL seek or read failure.
 *   -DBF_ERR_BAD_REC   : delete flag byte is neither 0x20 nor 0x2A (corrupt
 *                         record; fail loud per Rule 2).
 *
 * Per-field decode decisions (S1.3; all cited to dbf.md sec 5 + dbf_ref.py):
 *   C  -> xb_c(ptr, field_len).  Raw bytes in the arena record buffer (see
 *          lifetime note below). No space-trimming -- that is the evaluator's
 *          responsibility (S3.3). Round-trip safe.
 *   N  -> xb_n(dec_parse(raw, field_len)).  All-spaces -> 0.0 (dec_parse
 *          contract, rt.h). Overflow asterisks -> 0.0 (non-digit input).
 *   D  -> Valid non-blank 8-char YYYYMMDD: xb_d(jdn_from_ymd(y, m, d)).
 *          Blank (8 spaces) or "00000000": xb_u().
 *          Ref: dbf.md sec 5 D "blank/empty date is 8 spaces; 00000000 not valid".
 *          dbf_ref.py returns "" for blank -- we use xb_u() as the typed-model
 *          equivalent.
 *   L  -> T/t/Y/y -> xb_l(1); F/f/N/n -> xb_l(0); '?'/space -> xb_u().
 *          Any other byte -> return -DBF_ERR_BAD_REC (fail loud, Rule 2).
 *          Ref: dbf.md sec 5 L; dbf_ref.py decode_field L.
 *   M  -> S1.3->S2.1 boundary: the .dbt is NOT opened here.
 *          Block 0 or all-blank 10-byte field -> xb_u() (no memo).
 *          Block > 0 -> xb_m(ptr, 10) where ptr points into the record buffer
 *          at the 10-byte ASCII field. Caller / S2.1 uses dec_parse(ptr, 10)
 *          to get the block number, then reads the .dbt block.
 *          Ref: dbf.md sec 5 M "10-char ASCII right-justified decimal block
 *          number"; "block 0 or blanks = no memo".
 *
 * Record buffer lifetime:
 *   The C and M xb_val pointers in `out` reference an internal arena buffer
 *   allocated at dbf_open time. The buffer is OVERWRITTEN on each call to
 *   dbf_read_rec for the SAME table. Callers that need stable C/M data must
 *   copy the bytes before calling dbf_read_rec again.
 *
 * Mutation hook (Rule 6):
 *   -DDBF_MUTATE_RECOFF: shifts the record read offset by +1, omitting the
 *   delete-flag byte so field data is read from the wrong position. Every
 *   decoded field value diverges from the golden -> Tier-0 checks go RED.
 *
 * Ref (Law 1):
 *   - dbf.md sec 6 (record layout, delete flag, field byte packing).
 *   - dbf.md sec 5 (per-type on-disk encoding: C/N/D/L/M).
 *   - spec/samir/dbf_format.h (DBF_REC_DELETE_LIVE/DELETED, DBF_REC_FLAG_SIZE).
 *   - rt.h dec_parse, jdn_from_ymd.
 *   - value.h xb_c/xb_n/xb_d/xb_l/xb_m/xb_u.
 *   - harness/diff/dbf_diff/dbf_ref.py decode_field() (independent reference).
 */
int dbf_read_rec(dbf_table *tbl, uint32_t recno,
                 xb_val *out /*[nfields]*/, int *deleted);

/* ---- S1.4: deterministic write + round-trip ----
 *
 * S1.4 (docs/plans/SAMIR-implementation-plan.md): the .dbf creation + write path.
 *   - dbf_create     : create a NEW (empty, nrec=0) table from a field schema,
 *                      building the header + descriptors in the PAL arena. The
 *                      file is NOT written to disk until dbf_flush. The PAL file
 *                      handle is opened (PAL_RDWR|PAL_CREATE|PAL_TRUNC) at create
 *                      time so dbf_flush can write and the same handle can be
 *                      reused for record reads after flush.
 *   - dbf_append_rec : append one record (formatted from a typed-value array)
 *                      into the in-arena record region; bumps nrec. (Minimal
 *                      write-record primitive S1.4 needs for the round-trip; the
 *                      full mutation verbs REPLACE/DELETE/PACK/ZAP are S1.5.)
 *   - dbf_flush      : write the complete deterministic file image (header +
 *                      descriptors + 0x0D terminator + records [+ 0x1A EOF]) to
 *                      disk through the PAL, and re-sync the open handle so the
 *                      table can be read back (dbf_read_rec) without re-open.
 *
 * Write model (S1.4 decision; documented in dbf.c):
 *   Build-in-arena, flush-to-disk. dbf_create allocates the header bytes, the
 *   descriptor bytes, and a growable record region in the PAL arena. Records are
 *   appended into the arena (each is formatted into record_length bytes). One
 *   dbf_flush serializes the whole image with a single deterministic byte layout.
 *   Rationale: a single contiguous write is the simplest path to byte-determinism
 *   (Rule 11) and matches the "deterministic write" S1.4 contract; per-record
 *   seek-and-write (needed for in-place REPLACE/DELETE) is deferred to S1.5.
 *
 * Determinism (Rule 11): identical inputs + identical injected pal->today()
 *   date produce a byte-identical file every run. Specifically:
 *     - version    : 0x83 iff ANY field type is 'M' (memo), else 0x03.
 *     - last-update: from pal->today() (INJECTABLE; never a wall clock).
 *     - header_length = DBF_HDR_SIZE + DBF_DESC_STRIDE*nfields + 1  (the "+1"
 *       lone-0x0D terminator form; S1.4 always emits +1).
 *     - record_length = DBF_REC_FLAG_SIZE + sum(field lengths).
 *     - EVERY NORMALIZE byte (spec/samir/dbf_normalization.json) is emitted as
 *       0x00: header reserved (0x0C..0x0F, 0x10..0x1B multiuser, MDX flag 0x1C,
 *       LDID 0x1D, 0x1E..0x1F); per-descriptor RAM address 0x0C (0x00000000),
 *       work-area id 0x14, and all reserved/flag bytes. There are NO live RAM
 *       pointers or timestamps in SAMIR output (Rule 11) -- the writer is more
 *       normalized than genuine III+, which leaves a live address at desc 0x0C.
 *       Ref: spec/samir/dbf_normalization.json (MEANINGFUL vs NORMALIZE map).
 *
 * 0x1A EOF byte (S1.4 decision): dbf_flush EMITS a trailing 0x1A after the last
 *   record. dbf.md sec 8: the EOF byte is OPTIONAL in III+ (present in TAX/
 *   UNIVERSD, absent in CLIENTS/TOURS/TRAVEL); a reader MUST NOT require it
 *   (dbf_open does not). We emit it because it is harmless, period-authentic
 *   (DCREATE writes it), and bounds the file. It is NOT counted in
 *   header_length or record_length; it sits at offset
 *   header_length + nrec*record_length. The round-trip oracle normalizes the
 *   optional trailing 0x1A before comparing to a golden that lacks one.
 *
 * Memo (M) field write (S1.4->S2.2 boundary): dbf_create accepts an 'M' field
 *   and sets the memo (0x80) version bit, but S1.4 does NOT write a .dbt file.
 *   On dbf_append_rec an 'M' value formats to the 10-byte right-justified block
 *   number from the xb_m/xb_n payload (or 10 spaces for xb_u "no memo"). Writing
 *   the .dbt content + back-patching the block number is S2.2. A memo-bearing
 *   .dbf written by S1.4 has version 0x83 and valid M pointers but no sibling
 *   .dbt -- exercising it as a standalone memo store is the S2.2 contract.
 *
 * Mutation hook (Rule 6):
 *   -DDBF_MUTATE_VERSION: dbf_flush emits 0x03 in the version byte even when a
 *   memo (M) field is present (i.e. drops the 0x80 memo bit). A round-trip of a
 *   memo-bearing schema then reads back has_memo=false -> the oracle goes RED.
 *   Exactly one constant changes; no other byte is perturbed.
 *
 * Ref (Law 1):
 *   - dbf.md sec 2 (header), sec 3 (version byte / memo bit), sec 4 (descriptor +
 *     +1 terminator), sec 5 (C/N/D/L/M on-disk encodings), sec 6 (record layout),
 *     sec 8 (invariants + optional 0x1A EOF).
 *   - spec/samir/dbf_normalization.json (MEANINGFUL vs NORMALIZE byte map).
 *   - spec/samir/dbf_format.h (all offsets; DBF_VERSION_NO_MEMO/_WITH_MEMO).
 *   - rt.h dec_format (N output), jdn/ymd (D output), rt_memset/rt_memcpy.
 *   - pal.h (write/seek/open with PAL_CREATE/PAL_TRUNC; injectable today()).
 *   - docs/plans/SAMIR-implementation-plan.md S1.4 contract; Sec 2.E determinism.
 */

/*
 * dbf_field_spec: the per-field creation input for dbf_create. (Distinct from
 * dbf_field_t, which is the DECODED descriptor from an opened table -- this is
 * the user-supplied schema row.)
 *
 *   name       : NUL-terminated field name. Up to 10 usable chars; truncated to
 *                10 if longer (the 11th byte is the on-disk NUL). dbf.md sec 4.
 *   type       : 'C', 'N', 'D', 'L', or 'M' (III+ only). dbf.md sec 5.
 *   field_len  : bytes the field occupies in each record (1..254). For N this
 *                must include the sign + decimal point. For D it must be 8, for
 *                L it must be 1, for M it must be 10 (dbf_create asserts these).
 *   dec        : decimal-place count; meaningful for N only (must be 0 for
 *                C/D/L/M, asserted). dbf.md sec 4 offset 0x11.
 */
typedef struct {
    const char *name;
    char        type;
    uint8_t     field_len;
    uint8_t     dec;
} dbf_field_spec;

/*
 * dbf_create: create a new, empty (nrec=0) .dbf table from a field schema.
 *
 * Builds the 32-byte header + nfields 32-byte descriptors + the +1 terminator
 * in the PAL arena, opens the file via PAL_RDWR|PAL_CREATE|PAL_TRUNC, and
 * returns the handle in *out (nothing is written to disk until dbf_flush).
 *
 * Validates the schema (fail loud, Rule 2):
 *   - nfields in [1, DBF_MAX_FIELDS]                  -> DBF_ERR_TOO_MANY
 *   - every type is one of C/N/D/L/M                  -> DBF_ERR_BAD_TYPE
 *   - per-type length: D==8, L==1, M==10, C/N>=1      -> DBF_ERR_BAD_RECLEN
 *   - dec==0 for non-N types                          -> DBF_ERR_BAD_TYPE
 *   - record_length and header_length fit in u16      -> DBF_ERR_BAD_RECLEN/HDRLEN
 *
 * Returns DBF_OK and *out != NULL on success; on failure returns -(dbf_err),
 * sets *out = NULL, and leaves no half-open handle.
 *
 * Ref: dbf.md sec 2/3/4/5; spec/samir/dbf_format.h; plan S1.4.
 */
int dbf_create(samir_pal_t *pal, const char *name,
               const dbf_field_spec *fields, int nfields, dbf_table **out);

/*
 * dbf_append_rec: append one record to a table created by dbf_create.
 *
 *   in      : array of at least dbf_nfields(tbl) xb_val, one per field in
 *             descriptor order. Each is formatted to its field bytes per type
 *             (C left-justified space-pad; N right-justified via dec_format;
 *             D YYYYMMDD or 8 spaces for xb_u/xb_d; L 'T'/'F'; M 10-byte
 *             right-justified block number or 10 spaces for xb_u/empty).
 *   deleted : 0 -> delete flag 0x20 (live); non-zero -> 0x2A (deleted).
 *
 * The formatted record is appended into the in-arena record region and nrec is
 * bumped. Type-mismatch tolerance: a value whose type does not match the field
 * is coerced where natural (e.g. xb_u -> blank field) or formatted by the
 * field's own rule; the full assignment-coercion matrix is S5.5's concern.
 * S1.4 round-trip only round-trips well-typed values.
 *
 * Returns DBF_OK or -(dbf_err): -DBF_ERR_NOMEM (record region exhausted),
 * -DBF_ERR_BAD_RECLEN (a field value cannot be formatted into its width -- N
 * overflow is '*'-filled by dec_format, not an error, so this is for structural
 * faults only). Appending past DBF_MAX_RECS is -DBF_ERR_TOO_MANY.
 *
 * Ref: dbf.md sec 5/6; rt.h dec_format; plan S1.4 ("write the records you
 * create for the round-trip").
 */
int dbf_append_rec(dbf_table *tbl, const xb_val *in, int deleted);

/*
 * dbf_flush: write the complete deterministic file image to disk through the PAL.
 *
 * Serializes: 32-byte header (version 0x03/0x83, injected last-update date,
 * nrec, header_length, record_length, all NORMALIZE bytes 0x00) + nfields
 * 32-byte descriptors (name, type, length, dec; RAM addr/work-area/reserved all
 * 0x00) + the lone 0x0D terminator + every appended record + a trailing 0x1A
 * EOF byte. The write is one contiguous deterministic byte stream (Rule 11).
 *
 * After a successful flush the open handle is repositioned to the file start so
 * dbf_read_rec can read the just-written records without re-open.
 *
 * Only valid on a table from dbf_create (a table from dbf_open is read-only in
 * S1.4; flushing it returns -DBF_ERR_IO). Returns DBF_OK or -(dbf_err); a short
 * PAL write is -DBF_ERR_IO (fail loud, Rule 2).
 *
 * Ref: dbf.md sec 2/4/6/8; spec/samir/dbf_normalization.json; plan S1.4.
 */
int dbf_flush(dbf_table *tbl);

#endif /* INITECH_SAMIR_DBF_H */

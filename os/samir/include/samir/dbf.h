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

#endif /* INITECH_SAMIR_DBF_H */

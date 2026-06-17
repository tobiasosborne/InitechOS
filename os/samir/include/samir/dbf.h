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
 * III+ 1.1 ONLY (plan Sec 2.C): dbf_open FAILS LOUD on dBASE IV version bytes
 * (0x04, 0x8B, and anything other than 0x03/0x83) with DBF_ERR_BAD_VERSION.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/specs/file-formats/dbf.md sec 2 (32-byte header),
 *     sec 3 (version byte), sec 4 (descriptor + terminator), sec 5 (field types
 *     C/N/D/L/M -- III+ only), sec 8 (invariants),
 *     sec 9 (implementer notes: trust header_length / record_length / nrec;
 *            field name = bytes to first NUL; trailing bytes may be garbage).
 *   - spec/samir/dbf_format.h (the LOCKED byte-offset constants):
 *     DBF_DESC_NAME_OFF/TYPE_OFF/FIELD_LEN_OFF/DEC_COUNT_OFF, DBF_DESC_STRIDE=32,
 *     DBF_DESC_NAME_SIZE=11, DBF_DESC_TERMINATOR=0x0D.
 *   - docs/plans/SAMIR-implementation-plan.md S1.2 contract; Sec 8.2 dbf.h sketch.
 *   - os/samir/include/samir/pal.h (the ONLY OS surface; seek=filesize idiom).
 */
#ifndef INITECH_SAMIR_DBF_H
#define INITECH_SAMIR_DBF_H

#include <stdint.h>

#include "samir/pal.h"

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
    DBF_ERR_BAD_TYPE    = 10  /* S1.2: field type byte is not one of C/N/D/L/M (III+ only).
                               * Ref: dbf.md sec 5 (III+ types); plan Sec 2.C (fail loud). */
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

#endif /* INITECH_SAMIR_DBF_H */

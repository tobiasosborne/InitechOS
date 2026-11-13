/*
 * spec/samir/dbf_format.h -- dBASE III PLUS 1.1 .dbf byte-offset constants.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8). Source: ../dbase3-decomp/specs/file-formats/dbf.md
 * sections 2 (32-byte file header), 4 (32-byte field descriptor), 6 (record layout),
 * 8 (structural invariants). Every value is byte-verified against six III+ 1.1
 * golden fixtures: BANK, CLIENTS, TOURS, TRAVEL, TAX, UNIVERSD.
 *
 * III+ ONLY. dBASE IV / VFP / dBASE-7 meanings for reserved fields are NOT imported.
 * ASCII-clean (CLAUDE.md Rule 12). No timestamps (Rule 11).
 *
 * Ref: dbf.md ss2 (header table), ss4 (field descriptor + terminator),
 *      ss6 (record layout), ss8 (invariants).
 */

#ifndef SAMIR_DBF_FORMAT_H
#define SAMIR_DBF_FORMAT_H

/* -----------------------------------------------------------------------
 * 32-byte file header (offset 0x00 .. 0x1F)
 * Ref: dbf.md ss2 (table + worked example CLIENTS.DBF 03 55 0A 1E ...)
 * ----------------------------------------------------------------------- */

/* offset 0x00, size 1: version/flags byte.
 * III+ values: 0x03 = table, no memo; 0x83 = table, has .dbt memo.
 * bit 7 (0x80) = has memo file. low nibble 3 = dBASE III family.
 * Ref: dbf.md ss3 */
#define DBF_HDR_VERSION_OFF     0x00
#define DBF_HDR_VERSION_SIZE    1

/* offset 0x01, size 1: last-update year (year - 1900). e.g. 0x55 = 1985.
 * Ref: dbf.md ss2, "Date byte order is YY,MM,DD" (Corion's M/D/Y is WRONG). */
#define DBF_HDR_YEAR_OFF        0x01
#define DBF_HDR_YEAR_SIZE       1

/* offset 0x02, size 1: last-update month (1..12).
 * Ref: dbf.md ss2 */
#define DBF_HDR_MONTH_OFF       0x02
#define DBF_HDR_MONTH_SIZE      1

/* offset 0x03, size 1: last-update day (1..31).
 * Ref: dbf.md ss2 */
#define DBF_HDR_DAY_OFF         0x03
#define DBF_HDR_DAY_SIZE        1

/* offset 0x04, size 4: number of records (nrec), u32 little-endian.
 * Authoritative record count. Ref: dbf.md ss2, ss8 Invariant 2. */
#define DBF_HDR_NREC_OFF        0x04
#define DBF_HDR_NREC_SIZE       4

/* offset 0x08, size 2: header_length, u16 little-endian.
 * Byte offset where record data begins. TRUST this; do not assume +1 or +2.
 * Two real conventions: header_length = 32 + 32*n + 1 (+1 form, four fixtures)
 *                      or 32 + 32*n + 2 (+2 form, BANK/TAX).
 * Ref: dbf.md ss4 "Terminator: 0x0D, optionally followed by extra 0x00", ss8 Inv 1. */
#define DBF_HDR_HEADER_LEN_OFF  0x08
#define DBF_HDR_HEADER_LEN_SIZE 2

/* offset 0x0A, size 2: record_length, u16 little-endian.
 * Bytes per record = 1 (delete flag) + sum(all field lengths). No NULL bitmap.
 * Ref: dbf.md ss6, ss8 Invariant 1b. */
#define DBF_HDR_RECORD_LEN_OFF  0x0A
#define DBF_HDR_RECORD_LEN_SIZE 2

/* offset 0x0C, size 2: reserved. 0x00 0x00 in all III+ fixtures.
 * (dBASE IV reuses these bytes; do NOT import IV meaning for III+.)
 * Ref: dbf.md ss2, "Header bytes 0x0C-0x1F are NOT blank-filled" (they are 0x00). */
#define DBF_HDR_RESERVED_0C_OFF  0x0C
#define DBF_HDR_RESERVED_0C_SIZE 2

/* offset 0x0E, size 1: reserved (IV: incomplete-transaction flag).
 * 0x00 in III+ fixtures. IV-only meaning; do not use in III+. Ref: dbf.md ss2. */
#define DBF_HDR_RESERVED_0E_OFF  0x0E
#define DBF_HDR_RESERVED_0E_SIZE 1

/* offset 0x0F, size 1: reserved (IV: encryption flag).
 * 0x00 in III+ fixtures. IV-only meaning. Ref: dbf.md ss2. */
#define DBF_HDR_RESERVED_0F_OFF  0x0F
#define DBF_HDR_RESERVED_0F_SIZE 1

/* offset 0x10, size 12: reserved for multi-user / LAN.
 * All 0x00 in III+ fixtures. dBASE III+ doc labels 0x14-0x1B "reserved for
 * multi-user dBASE". Ref: dbf.md ss2. */
#define DBF_HDR_MULTIUSER_OFF    0x10
#define DBF_HDR_MULTIUSER_SIZE   12

/* offset 0x1C, size 1: reserved (IV: production .mdx flag).
 * NORMALIZE for III+: 0x00 in 100% of III+ fixtures; III+ has no MDX.
 * Do NOT import the IV "production MDX flag" meaning.
 * Ref: dbf.md ss2 (0x1C row); ADR-0008 DEC-06 (0x1C = NORMALIZE for III+). */
#define DBF_HDR_MDX_FLAG_OFF     0x1C
#define DBF_HDR_MDX_FLAG_SIZE    1

/* offset 0x1D, size 1: language driver / code-page id (LDID).
 * 0x00 in all III+ 1.1 fixtures = "no codepage defined" (CP437 by default).
 * Ref: dbf.md ss7. */
#define DBF_HDR_LDID_OFF         0x1D
#define DBF_HDR_LDID_SIZE        1

/* offset 0x1E, size 2: reserved. 0x00 0x00 in III+ fixtures. Ref: dbf.md ss2. */
#define DBF_HDR_RESERVED_1E_OFF  0x1E
#define DBF_HDR_RESERVED_1E_SIZE 2

/* Total header block size (fixed). Ref: dbf.md ss2. */
#define DBF_HDR_SIZE             32

/* Known version byte values for III+ (only these two are produced by III+ 1.1).
 * Ref: dbf.md ss3. */
#define DBF_VERSION_NO_MEMO      0x03  /* dBASE III/III+ table, no memo file */
#define DBF_VERSION_WITH_MEMO    0x83  /* dBASE III+ table, has .dbt memo file */

/* -----------------------------------------------------------------------
 * 32-byte field descriptor (immediately after the 32-byte header, one per field)
 * Ref: dbf.md ss4 "32-byte field descriptor" table + worked example CLIENTS FIRSTNAME.
 * Stride between descriptors = 32 bytes (byte-verified: all 6 fixtures).
 * ----------------------------------------------------------------------- */

/* offset 0x00 within descriptor, size 11: field name, ASCII, NUL-terminated,
 * NUL-padded. Read name as bytes up to first 0x00; trailing bytes may be garbage.
 * Ref: dbf.md ss4 "Field name: read to first NUL; ignore trailing garbage". */
#define DBF_DESC_NAME_OFF        0x00
#define DBF_DESC_NAME_SIZE       11

/* offset 0x0B, size 1: field type, ASCII char. III+ types: C N D L M.
 * Ref: dbf.md ss4, ss5. */
#define DBF_DESC_TYPE_OFF        0x0B
#define DBF_DESC_TYPE_SIZE       1

/* offset 0x0C, size 4: field data address in memory (RAM pointer).
 * MEANINGLESS on disk -- a live address left by the running program.
 * Ignore on read; emit anything (real III+ leaves a live address).
 * Ref: dbf.md ss4 (0x0C row), ss9 "Field data address ... meaningless on disk". */
#define DBF_DESC_ADDR_OFF        0x0C
#define DBF_DESC_ADDR_SIZE       4

/* offset 0x10, size 1: field length (bytes the field occupies in each record).
 * For N: includes sign + decimal point. Ref: dbf.md ss4 (0x10 row). */
#define DBF_DESC_FIELD_LEN_OFF   0x10
#define DBF_DESC_FIELD_LEN_SIZE  1

/* offset 0x11, size 1: decimal count. Meaningful for N only (0 for C/D/L/M).
 * Ref: dbf.md ss4 (0x11 row). */
#define DBF_DESC_DEC_COUNT_OFF   0x11
#define DBF_DESC_DEC_COUNT_SIZE  1

/* offset 0x12, size 2: reserved (multi-user/LAN). 0x00 0x00 in fixtures.
 * Ref: dbf.md ss4 (0x12 row). */
#define DBF_DESC_RESERVED_12_OFF  0x12
#define DBF_DESC_RESERVED_12_SIZE 2

/* offset 0x14, size 1: work-area id. 0x00 or 0x01 in III+ fixtures.
 * Note: correlates with terminator convention (+1 vs +2) but causality unresolved.
 * Do NOT import the Harbour/IV auto-increment meaning for III+.
 * Ref: dbf.md ss4 (0x14 row), ss9 "work-area id correlation". */
#define DBF_DESC_WORK_AREA_OFF   0x14
#define DBF_DESC_WORK_AREA_SIZE  1

/* offset 0x15, size 2: reserved (multi-user/LAN). 0x00 0x00 in fixtures.
 * Ref: dbf.md ss4 (0x15 row). */
#define DBF_DESC_RESERVED_15_OFF  0x15
#define DBF_DESC_RESERVED_15_SIZE 2

/* offset 0x17, size 1: SET FIELDS flag. 0x00 in all fixtures.
 * Ref: dbf.md ss4 (0x17 row). */
#define DBF_DESC_SET_FIELDS_OFF  0x17
#define DBF_DESC_SET_FIELDS_SIZE 1

/* offset 0x18, size 8: reserved. 0x00 in fixtures.
 * Ref: dbf.md ss4 (0x18 row). */
#define DBF_DESC_RESERVED_18_OFF  0x18
#define DBF_DESC_RESERVED_18_SIZE 8

/* offset 0x1F, size 1: reserved (IV: index-field flag / MDX field flag).
 * NORMALIZE for III+: 0x00 in 100% of III+ fixtures. IV-only meaning.
 * Do NOT import the IV "index-field flag" meaning.
 * Ref: dbf.md ss4 (0x1F row); ADR-0008 DEC-06 (per-descriptor 0x1F = NORMALIZE for III+). */
#define DBF_DESC_INDEX_FLAG_OFF  0x1F
#define DBF_DESC_INDEX_FLAG_SIZE 1

/* Size of one field descriptor. Byte-verified: stride = 32 in all 6 fixtures.
 * (NOT the dBASE-7 48-byte descriptor.) Ref: dbf.md ss4 Verification. */
#define DBF_DESC_STRIDE          32

/* Terminator byte that ends the field-descriptor array. Ref: dbf.md ss4. */
#define DBF_DESC_TERMINATOR      0x0D

/* Maximum number of fields in a III+ table. Ref: dbf.md ss4. */
#define DBF_MAX_FIELDS           128

/* -----------------------------------------------------------------------
 * Record layout
 * Ref: dbf.md ss6.
 * ----------------------------------------------------------------------- */

/* Delete-flag byte: first byte of each record.
 * 0x20 = live/active; 0x2A ('*') = deleted (still present until PACK).
 * Ref: dbf.md ss6. */
#define DBF_REC_DELETE_LIVE      0x20
#define DBF_REC_DELETE_DELETED   0x2A

/* Size of the delete-flag byte that precedes the field data in every record.
 * record_length = DBF_REC_FLAG_SIZE + sum(field_length[i]).
 * Ref: dbf.md ss6, ss8 Invariant 1b. */
#define DBF_REC_FLAG_SIZE        1

/* -----------------------------------------------------------------------
 * Optional EOF marker
 * 0x1A optionally appended after the last record; optional in III+ 1.1.
 * Present in TAX and UNIVERSD fixtures; absent in CLIENTS/TOURS/TRAVEL.
 * A reader MUST NOT require it. Ref: dbf.md ss8 Invariant 2.
 * ----------------------------------------------------------------------- */
#define DBF_EOF_MARKER           0x1A

/* -----------------------------------------------------------------------
 * Arithmetic relationships (for validation; do not hard-code)
 * Ref: dbf.md ss8.
 *
 * header_length - (DBF_HDR_SIZE + DBF_DESC_STRIDE * nfields) in {1, 2}
 * record_length == DBF_REC_FLAG_SIZE + sum(field_lengths[0..nfields-1])
 * file_size     == header_length + nrec * record_length [+ 0 or 1 EOF byte]
 * ----------------------------------------------------------------------- */

#endif /* SAMIR_DBF_FORMAT_H */

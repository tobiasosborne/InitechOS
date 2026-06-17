/*
 * spec/samir/ndx_format.h -- dBASE III PLUS 1.1 .ndx B-tree byte-offset constants.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8). Source: ../dbase3-decomp/specs/file-formats/ndx.md
 * sections 2 (512-byte header/page-0), 3 (B-tree node), 3.1 (key entry / group layout),
 * 4 (key data encoding). Every value is byte-verified against 11 III+ 1.1 golden
 * .ndx files: CNAMES, NAMES, TNAMES, ZIPCODE, CUSTOMER, CHKNO, TOURDATE, AVAL_FLT,
 * FLT_NO, LOCATION, TRIPS.
 *
 * III+ ONLY (.ndx single-file index). dBASE IV .mdx, Clipper .ntx, Fox .idx are
 * different formats and are NOT described here.
 * ASCII-clean (CLAUDE.md Rule 12). No timestamps (Rule 11).
 *
 * Ref: ndx.md ss1 (file geometry), ss2 (header), ss3 (node + group), ss4 (key data).
 */

#ifndef SAMIR_NDX_FORMAT_H
#define SAMIR_NDX_FORMAT_H

/* -----------------------------------------------------------------------
 * File geometry
 * Ref: ndx.md ss1.
 * ----------------------------------------------------------------------- */

/* Every page (block) is 512 bytes. All fixture file sizes are multiples of 512.
 * Page number P is at byte offset P * NDX_PAGE_SIZE.
 * Ref: ndx.md ss1 (piclist BLOCK_SIZE 512 + fixture size check). */
#define NDX_PAGE_SIZE            512

/* Page 0 = the header ("anchor node"). All other pages are B-tree nodes.
 * Ref: ndx.md ss1, ss2. */
#define NDX_HEADER_PAGE          0

/* -----------------------------------------------------------------------
 * Header (page 0, 512 bytes)
 * All integers little-endian. Validated against CNAMES.NDX + all 11 fixtures.
 * Ref: ndx.md ss2 (table), ss2.2 (worked byte dump).
 * ----------------------------------------------------------------------- */

/* offset 0x00, size 4: root_page, uint32 LE.
 * Page number of the current root node. Updated on reindex.
 * CNAMES.NDX: root_page = 6.
 * Ref: ndx.md ss2. */
#define NDX_HDR_ROOT_PAGE_OFF    0x00
#define NDX_HDR_ROOT_PAGE_SIZE   4

/* offset 0x04, size 4: total_pages, uint32 LE.
 * Total page count including the header (= next-page-to-allocate).
 * Always equals filesize / 512. Max 0x7FFFFF.
 * CNAMES.NDX: total_pages = 7.
 * Ref: ndx.md ss2. */
#define NDX_HDR_TOTAL_PAGES_OFF  0x04
#define NDX_HDR_TOTAL_PAGES_SIZE 4

/* offset 0x08, size 4: reserved, uint32 LE.
 * Always 0x00000000 in III+ fixtures. piclist calls it "version (uncertain)".
 * Treat as reserved = 0 for III+.
 * Ref: ndx.md ss2, Open questions (reserved@0x08). */
#define NDX_HDR_RESERVED_OFF     0x08
#define NDX_HDR_RESERVED_SIZE    4

/* offset 0x0C, size 2: key_length, uint16 LE.
 * Length in bytes of the key data within each entry.
 * CNAMES: key_length = 40 (LASTNAME C20 + FIRSTNAME C20).
 * Ref: ndx.md ss2. */
#define NDX_HDR_KEY_LENGTH_OFF   0x0C
#define NDX_HDR_KEY_LENGTH_SIZE  2

/* offset 0x0E, size 2: keys_per_page, uint16 LE.
 * Maximum keys per node = floor((512 - 4) / group_length).
 * CNAMES: keys_per_page = 10 (508 / 48 = 10).
 * Ref: ndx.md ss2 (formula verified for all 11 fixtures). */
#define NDX_HDR_KEYS_PER_PAGE_OFF  0x0E
#define NDX_HDR_KEYS_PER_PAGE_SIZE 2

/* offset 0x10, size 2: key_type, uint16 LE.
 * 0 = character key (space-padded ASCII).
 * 1 = numeric OR date key (8-byte IEEE-754 double LE).
 * CNAMES = 0 (char), CHKNO = 1 (N field), TOURDATE = 1 (D field).
 * Ref: ndx.md ss2, ss4. */
#define NDX_HDR_KEY_TYPE_OFF     0x10
#define NDX_HDR_KEY_TYPE_SIZE    2
#define NDX_KEY_TYPE_CHAR        0
#define NDX_KEY_TYPE_NUMERIC     1   /* also used for Date keys */

/* offset 0x12, size 2: group_length, uint16 LE.
 * Bytes per key entry = ceil4(key_length + 8).
 * ceil4(n) = ((n + 3) / 4) * 4.
 * CNAMES: group_length = 48 (ceil4(40 + 8) = ceil4(48) = 48).
 * Verified: group_length == ceil4(key_length + 8) for all 11 fixtures.
 * Ref: ndx.md ss2. */
#define NDX_HDR_GROUP_LENGTH_OFF  0x12
#define NDX_HDR_GROUP_LENGTH_SIZE 2

/* offset 0x14, size 2: dummy, uint16 LE.
 * Unused/uninitialized; holds garbage (0x0010, 0x0008, 0x000F seen, or 0).
 * No correlation to key_length, key_type, or unique. Writer intent unresolved.
 * Ref: ndx.md ss2, Open questions (dummy@0x14). */
#define NDX_HDR_DUMMY_OFF        0x14
#define NDX_HDR_DUMMY_SIZE       2

/* offset 0x16, size 2: unique_flag, uint16 LE.
 * 0 = not unique; 1 = UNIQUE index (minted NUNIQ.NDX confirms value = 1).
 * All 11 fixtures = 0. III+ has no DESCENDING index (that is IV).
 * Ref: ndx.md ss2, Open questions (unique_flag resolved 2026-06-16 via minting). */
#define NDX_HDR_UNIQUE_FLAG_OFF  0x16
#define NDX_HDR_UNIQUE_FLAG_SIZE 2
#define NDX_UNIQUE_NO            0
#define NDX_UNIQUE_YES           1

/* offset 0x18, size 100: key_expr, NUL-terminated char[].
 * The dBASE index expression (e.g. "LASTNAME + FIRSTNAME ").
 * Stored verbatim AS TYPED (NOT lowercased -- piclist "lowercase" claim is WRONG;
 * byte-verified against CNAMES "LASTNAME + FIRSTNAME " and CHKNO "Chkno ").
 * Span 0x18..0x7B (100 bytes). NUL-terminated; bytes after NUL up to 0x7B are garbage.
 * Ref: ndx.md ss2.1 (key-expression byte span / casing RESOLVED from bytes). */
#define NDX_HDR_KEY_EXPR_OFF     0x18
#define NDX_HDR_KEY_EXPR_SIZE    100

/* offset 0x7C..0x1FF: unused tail (388 bytes). Garbage or zeros.
 * Ref: ndx.md ss2 (table last row). */
#define NDX_HDR_TAIL_OFF         0x7C
#define NDX_HDR_TAIL_SIZE        388

/* -----------------------------------------------------------------------
 * B-tree node (any page 1 .. total_pages - 1)
 * Each node is a 512-byte page: a 4-byte node header + key entries.
 * Ref: ndx.md ss3 (table + byte-check CUSTOMER.NDX, CNAMES.NDX).
 * ----------------------------------------------------------------------- */

/* offset 0x00 within node, size 2: entry_count, uint16 LE.
 * Number of LIVE key entries in this node. Range 0..keys_per_page.
 * A 2-byte count, NOT a 4-byte count (piclist: "count (two bytes)").
 * CUSTOMER: entry_count = 5; CNAMES root: entry_count = 4.
 * Ref: ndx.md ss3. */
#define NDX_NODE_ENTRY_COUNT_OFF  0x00
#define NDX_NODE_ENTRY_COUNT_SIZE 2

/* offset 0x02 within node, size 2: filler, uint16 LE.
 * Pad to 4-byte boundary; uninitialized garbage.
 * CUSTOMER = 0x5543 (garbage); CNAMES root = 0x0000.
 * Ref: ndx.md ss3. */
#define NDX_NODE_FILLER_OFF      0x02
#define NDX_NODE_FILLER_SIZE     2

/* Size of the 4-byte node header (entry_count + filler).
 * keys_per_page = (NDX_PAGE_SIZE - NDX_NODE_HDR_SIZE) / group_length = 508 / group_length.
 * Ref: ndx.md ss3. */
#define NDX_NODE_HDR_SIZE        4

/* offset 0x04: start of key entry array. Each entry is group_length bytes.
 * Ref: ndx.md ss3. */
#define NDX_NODE_ENTRIES_OFF     0x04

/* -----------------------------------------------------------------------
 * Key entry ("group") layout within a node
 * Each group is group_length = ceil4(key_length + 8) bytes.
 * Ref: ndx.md ss3.1 (table + byte-check CUSTOMER leaf, CNAMES branch+leaf).
 * ----------------------------------------------------------------------- */

/* offset 0x00 within group, size 4: child_page, uint32 LE.
 * Lower-level page number. 0 => this is a LEAF entry (no child subtree).
 * Non-zero => BRANCH entry; value is the child page index.
 * Ref: ndx.md ss3.1. */
#define NDX_GRP_CHILD_PAGE_OFF   0x00
#define NDX_GRP_CHILD_PAGE_SIZE  4

/* offset 0x04 within group, size 4: dbf_recno, uint32 LE.
 * DBF record number (1-based). Meaningful in LEAF entries; 0 (ignored) in
 * branch entries.
 * Ref: ndx.md ss3.1. */
#define NDX_GRP_DBF_RECNO_OFF    0x04
#define NDX_GRP_DBF_RECNO_SIZE   4

/* offset 0x08 within group: key_data, key_length bytes.
 * Encoding depends on key_type (see section 4 / NDX_KEY_TYPE_*).
 * Ref: ndx.md ss3.1, ss4. */
#define NDX_GRP_KEY_DATA_OFF     0x08

/* Filler bytes after key_data: group_length - 8 - key_length (0..3 bytes).
 * Garbage or zeros. Example: AVAL_FLT KL=14 -> group 24 -> 2 filler bytes.
 * Ref: ndx.md ss3.1. */

/* Overhead per group entry before/after key_data (child_page + dbf_recno). */
#define NDX_GRP_OVERHEAD         8   /* 4 (child_page) + 4 (dbf_recno) */

/* -----------------------------------------------------------------------
 * Key data encoding
 * Ref: ndx.md ss4.
 * ----------------------------------------------------------------------- */

/* Character key (key_type == NDX_KEY_TYPE_CHAR):
 * key_length bytes of space-padded ASCII (OEM/CP437), left-justified, 0x20-padded.
 * Multi-field key = runtime concatenation of parts, each to fixed width.
 * Collation = unsigned byte value (CP437 byte order; NOT case-insensitive).
 * Ref: ndx.md ss4.1, ss6. */

/* Numeric key (key_type == NDX_KEY_TYPE_NUMERIC, N field):
 * 8-byte IEEE-754 double, little-endian. key_length == 8, group_length == 16.
 * Raw LE double, NO sign-flip, NO order-preserving transform.
 * key_length == NDX_KEY_LEN_DOUBLE for numeric/date keys.
 * Comparison MUST be arithmetic (not byte-wise) because negatives sort wrong
 * with byte order. Minted NCOST.NDX confirmed bytes for -123.45, -1.0, 0.0, 279.0.
 * Ref: ndx.md ss4.2 (minted 2026-06-16, re/mint-results-001.md). */

/* Date key (key_type == NDX_KEY_TYPE_NUMERIC, D field):
 * 8-byte IEEE-754 double LE encoding the Julian Day Number (JDN) of the date.
 * Example: 1985-08-05 -> JDN 2446283 -> 00 00 00 80 E5 A9 42 41.
 * Ref: ndx.md ss4.2 (byte-verified TOURDATE.NDX). */

/* key_length for numeric and date keys (always 8 = sizeof double). */
#define NDX_KEY_LEN_DOUBLE       8

/* group_length for numeric/date keys (ceil4(8 + 8) = 16). */
#define NDX_GROUP_LEN_DOUBLE     16

/* -----------------------------------------------------------------------
 * Derived formulas (for validation; not hard-coded values)
 * Ref: ndx.md ss2, ss3.
 *
 * group_length  = ((key_length + 8 + 3) / 4) * 4   (ceil4(key_length + 8))
 * keys_per_page = (NDX_PAGE_SIZE - NDX_NODE_HDR_SIZE) / group_length
 *               = 508 / group_length                (integer division)
 *
 * Both formulas hold for all 11 fixtures (KL 2,3,5,8,14,28,40 all confirmed).
 *
 * Trailing (N+1th) slot in a branch node (offset NDX_NODE_ENTRIES_OFF + entry_count * group_length):
 *   first 4 bytes = rightmost child pointer (subtree for keys > last separator).
 *   dbf_recno / key_data in this slot are garbage.
 * In a leaf node, this trailing slot is unused garbage.
 * Ref: ndx.md ss3.2.
 * ----------------------------------------------------------------------- */

#endif /* SAMIR_NDX_FORMAT_H */

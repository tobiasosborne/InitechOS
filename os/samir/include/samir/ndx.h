/*
 * os/samir/include/samir/ndx.h -- SAMIR (InitechBase) .ndx index codec contract.
 *                                  Steps S4.1 + S4.2.
 *
 * THE ARTIFACT (CLAUDE.md Law 3): compiled freestanding (-ffreestanding
 * -nostdlib). Includes ONLY <stdint.h> plus the engine headers. All OS contact
 * is through the PAL vtable. No libc, no int 0x21.
 *
 * dBASE III PLUS 1.1 ONLY (docs/plans/SAMIR-implementation-plan.md Sec 2.C):
 * .ndx single-file B-tree index. .mdx (IV), .ntx (Clipper), .idx (Fox) are
 * OUT OF SCOPE for M6 and are not described or handled here.
 *
 * S4.1 scope (STRUCTURE parse only; key decode to typed values is S4.2):
 *   - ndx_open: read page 0, validate the 10 header fields, compute/verify the
 *     ceil4(key_length+8) group formula and (512-4)/group keys-per-page formula.
 *   - ndx_close: release all arena memory.
 *   - Accessors for all 10 header fields (see ndx_format.h for byte offsets).
 *   - ndx_read_node: load a 512-byte page by page number, parse the 2+2 node
 *     header (entry_count + filler), and expose the per-group struct array
 *     {child_page, dbf_recno, key_data_ptr}. key_data_ptr points into an
 *     arena-allocated copy of the page; the bytes are RAW (no typed decode --
 *     that is S4.2). Caller must not read past key_length bytes.
 *   - ndx_node_free: release a node returned by ndx_read_node.
 *
 * S4.2 scope (key decode + collation; added in this step):
 *   - ndx_key_decode: decode raw key_data bytes to an xb_val, dispatching on
 *     key_type. char keys -> XB_C (pointer into caller-supplied buf); type-1
 *     keys -> XB_N holding the raw LE double. S4.2->S5 boundary: the .ndx
 *     header only records key_type 0/1, NOT N-vs-D (both are doubles). The
 *     type-1 result is always XB_N; a caller that knows the field is a D column
 *     (from the .dbf descriptor or the key expression) may reinterpret the
 *     double as a JDN and construct XB_D itself. For B-tree collation (S4.3
 *     SEEK and S4.4 build) the N/D distinction is irrelevant: both are doubles
 *     compared arithmetically by ndx_key_cmp.
 *   - ndx_key_cmp: compare two raw key_data byte arrays for B-tree ordering.
 *     char: unsigned byte compare over key_length (CP437 byte order; ndx.md ss6).
 *     type-1: decode both as LE double, compare arithmetically -- NOT memcmp
 *     (raw IEEE doubles do not byte-sort across the sign boundary; the minted
 *     NCOST.NDX leaf is in true numeric order: -123.45 < -1 < 0 < 279 even
 *     though the sign bit is set in the MSB of negatives). [verified: minted
 *     NCOST.NDX 2026-06-16; re/mint-results-001.md "NDX numeric & date keys"]
 *
 * S4.1/S4.2 boundary (what this header exposes for S4.2 to consume):
 *   - ndx_key_type(idx) returns 0 (char) or 1 (numeric/date) -- S4.2 switches
 *     on this to decide the decode path.
 *   - ndx_key_length(idx) returns the key_length field -- S4.2 knows the byte
 *     width of each key_data region.
 *   - ndx_entry_t.key_data points to raw key_length bytes per entry.
 *   - key_type == NDX_KEY_TYPE_NUMERIC means S4.2 must read 8 bytes as LE
 *     IEEE-754 double and compare ARITHMETICALLY (plan Sec 3.3 / ndx.md ss4.2).
 *     For char keys, comparison is unsigned byte order (CP437, ndx.md ss6).
 *   - The root page, total_pages, and keys_per_page are exposed so S4.3 can
 *     implement B-tree descent without re-reading the header.
 *
 * Fail loud (CLAUDE.md Rule 2): every structural violation (bad group formula,
 * root out of range, file size not a page multiple, entry extends past page
 * boundary) returns a negative error code. A silently-wrong index would corrupt
 * every SEEK / ORDER BY built on top of it.
 *
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 * Freestanding-legal (Law 3): only <stdint.h> and samir/ headers.
 *
 * Ref (Law 1):
 *   - spec/samir/ndx_format.h -- LOCKED byte-offset constants (source of truth
 *     for every offset used in ndx.c).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md -- the byte-verified spec:
 *     ss1 (page geometry), ss2 (header + casing RESOLVED), ss3 (node + 2+2 hdr),
 *     ss3.1 (group layout), ss3.2 (trailing child), ss4 (key encoding),
 *     ss4.1 (char: space-padded CP437), ss4.2 (numeric/date: raw LE double,
 *     NO sign-flip, arithmetic compare -- VERIFIED minted NCOST.NDX 2026-06-16),
 *     ss5 (B-tree ordering), ss6 (collation: char = unsigned byte; numeric =
 *     double arithmetic).
 *   - ../dbase3-decomp/re/mint-results-001.md -- THE authority that settled
 *     numeric keys = raw LE IEEE-754 doubles, NO sign-flip, arithmetic compare.
 *     Table: -123.45 -> cd cc cc cc cc dc 5e c0; -1.0 -> 00..00 f0 bf; 0.0 ->
 *     all zeros; 279.0 -> 00..00 70 71 40. [verified: minted NCOST.NDX 2026-06-16]
 *   - docs/plans/SAMIR-implementation-plan.md S4.1/S4.2 contracts, Sec 3.3.
 *   - os/samir/include/samir/pal.h (byte I/O vtable).
 *   - os/samir/include/samir/rt.h (rt_mem* helpers; jdn_from_ymd for tests).
 *   - os/samir/include/samir/value.h (xb_val, xb_n, xb_c).
 */

#ifndef INITECH_SAMIR_NDX_H
#define INITECH_SAMIR_NDX_H

#include <stdint.h>
#include "samir/pal.h"
#include "samir/value.h"

/* -----------------------------------------------------------------------
 * Error codes
 * Returned negated from int-returning functions (return -NDX_ERR_BAD_SIZE,
 * etc.). NDX_OK == 0 (success). Callers compare symbolically.
 * Ref: ndx.md ss1 (size), ss2 (header fields), ss3 (node).
 * ----------------------------------------------------------------------- */
typedef enum {
    NDX_OK               = 0,  /* success */
    NDX_ERR_IO           = 1,  /* PAL I/O error (short read, open failure) */
    NDX_ERR_BAD_SIZE     = 2,  /* file size not a multiple of 512, or < 512 */
    NDX_ERR_BAD_ROOT     = 3,  /* root_page == 0 or >= total_pages */
    NDX_ERR_BAD_GROUP    = 4,  /* group_length != ceil4(key_length+8) */
    NDX_ERR_BAD_KPP      = 5,  /* keys_per_page != (512-4)/group_length */
    NDX_ERR_BAD_KEYTYPE  = 6,  /* key_type is not 0 or 1 */
    NDX_ERR_BAD_PAGE     = 7,  /* page_no out of range in ndx_read_node */
    NDX_ERR_PAGE_OVF     = 8,  /* entry extends past 512-byte page boundary */
    NDX_ERR_OOM          = 9   /* PAL arena exhausted */
} ndx_err;

/* -----------------------------------------------------------------------
 * Opaque handle
 * ndx_index is opaque; all access is through the accessor functions below.
 * ----------------------------------------------------------------------- */
typedef struct ndx_index ndx_index;

/* -----------------------------------------------------------------------
 * ndx_open / ndx_close
 * ----------------------------------------------------------------------- */

/*
 * ndx_open: open and parse the .ndx at PATH.
 *
 * Reads page 0 (the header), validates all 10 fields, verifies:
 *   group_length  == ceil4(key_length + 8)       [ndx.md ss2 formula]
 *   keys_per_page == (NDX_PAGE_SIZE - 4) / group_length  [ndx.md ss2]
 *   root_page in [1, total_pages)                [ndx.md ss1]
 * Reads the key_expr as NUL-terminated, capped at 100 bytes, verbatim case
 * (NOT lowercased; piclist "lowercase" claim is WRONG -- ndx.md ss2.1).
 *
 * Returns NDX_OK (0) with *out set on success.
 * Returns -ndx_err (negative) with *out == NULL on failure; fails loud per
 * Rule 2 (every structural violation is an explicit error code).
 *
 * Ref: ndx.md ss2, ss2.1, ss2.2; spec/samir/ndx_format.h NDX_HDR_* constants.
 */
int ndx_open(samir_pal_t *pal, const char *name, ndx_index **out);

/*
 * ndx_close: release all arena memory for IDX.
 *
 * Returns NDX_OK. IDX is invalid after this call.
 */
int ndx_close(ndx_index *idx);

/* -----------------------------------------------------------------------
 * Header accessors (all 10 fields from ndx_format.h)
 * Each returns the value parsed from page 0 during ndx_open.
 * Ref: ndx.md ss2 table + ss2.2 byte dump (CNAMES values cited in comments).
 * ----------------------------------------------------------------------- */

/* root_page: page number of the current B-tree root. CNAMES=6. [ndx.md ss2] */
uint32_t ndx_root_page(const ndx_index *idx);

/* total_pages: total page count including the header (= filesize/512).
 * CNAMES=7. [ndx.md ss2] */
uint32_t ndx_total_pages(const ndx_index *idx);

/* reserved: always 0x00000000 in III+ fixtures. Treat as reserved.
 * [ndx.md ss2, open question reserved@0x08] */
uint32_t ndx_reserved(const ndx_index *idx);

/* key_length: byte width of the key data in each group entry.
 * CNAMES=40, CHKNO=8, TOURDATE=8, ZIPCODE=5. [ndx.md ss2] */
uint16_t ndx_key_length(const ndx_index *idx);

/* keys_per_page: maximum live key entries per node = floor(508/group_length).
 * CNAMES=10. [ndx.md ss2] */
uint16_t ndx_keys_per_page(const ndx_index *idx);

/* key_type: 0 = character (space-padded ASCII/CP437); 1 = numeric or date
 * (8-byte LE IEEE-754 double). CNAMES=0, CHKNO=1, TOURDATE=1. [ndx.md ss2] */
uint16_t ndx_key_type(const ndx_index *idx);

/* group_length: bytes per key entry = ceil4(key_length + 8). CNAMES=48.
 * Ref: ndx.md ss2; spec/samir/ndx_format.h NDX_HDR_GROUP_LENGTH_OFF. */
uint16_t ndx_group_length(const ndx_index *idx);

/* dummy: unused/uninitialized field; holds garbage. AVAL_FLT=0x0010,
 * CUSTOMER=0x0008, FLT_NO=0x000F, others 0. [ndx.md ss2 / open question] */
uint16_t ndx_dummy(const ndx_index *idx);

/* unique_flag: 0 = not unique; non-zero = UNIQUE index. All 11 fixtures = 0.
 * [ndx.md ss2; minted NUNIQ.NDX: LE u16 = 0x0100 = 256 for UNIQUE] */
uint16_t ndx_unique_flag(const ndx_index *idx);

/* key_expr: NUL-terminated index expression, verbatim as typed (NOT
 * lowercased). Capped at 100 bytes. CNAMES="LASTNAME + FIRSTNAME ".
 * [ndx.md ss2.1: piclist "lowercase" refuted by byte-check] */
const char *ndx_key_expr(const ndx_index *idx);

/* -----------------------------------------------------------------------
 * Node I/O
 * ----------------------------------------------------------------------- */

/*
 * One key entry ("group") within a node page.
 *
 * Layout per ndx.md ss3.1:
 *   [0..3]  child_page  uint32 LE: 0 => leaf; non-zero => branch (child page #)
 *   [4..7]  dbf_recno   uint32 LE: DBF record number (1-based; 0 in branch)
 *   [8..8+key_length-1] key_data: raw bytes, ndx_key_length(idx) long.
 *     key_data is a pointer into arena memory valid until ndx_node_free().
 *     S4.2 decodes these bytes; S4.1 exposes them raw.
 *
 * key_data encoding hint (for S4.2):
 *   key_type == 0: space-padded ASCII/CP437, unsigned byte collation [ndx.md ss4.1]
 *   key_type == 1: 8-byte LE IEEE-754 double, arithmetic comparison [ndx.md ss4.2]
 *
 * Ref: ndx.md ss3.1; spec/samir/ndx_format.h NDX_GRP_* constants.
 */
typedef struct {
    uint32_t       child_page;  /* 0 = leaf, non-zero = child page number */
    uint32_t       dbf_recno;   /* 1-based DBF record number (leaf); 0 in branch */
    const uint8_t *key_data;    /* raw key bytes, ndx_key_length(idx) long */
} ndx_entry_t;

/*
 * ndx_node_t: parsed node returned by ndx_read_node.
 *
 * entry_count live entries in the entries[] array. The trailing (N+1th) child
 * pointer (rightmost subtree in branch nodes, ndx.md ss3.2) is in trail_child;
 * it is 0 for leaf nodes or when the trailing slot does not fully fit in the
 * 512-byte page.
 *
 * entries[] is allocated from the PAL arena; entries[i].key_data points into
 * the same arena region. All pointers are invalid after ndx_node_free().
 */
typedef struct {
    uint16_t    entry_count;   /* live entries (0..keys_per_page) */
    uint16_t    filler;        /* raw filler word (garbage; exposed for testing) */
    uint32_t    trail_child;   /* rightmost child page (0 if leaf / not present) */
    ndx_entry_t entries[1];    /* [entry_count] entries; flexible via arena alloc */
} ndx_node_t;

/*
 * ndx_read_node: read and parse the 512-byte page PAGE_NO.
 *
 * Allocates an ndx_node_t (plus entry array and key_data bytes) from the PAL
 * arena. The caller MUST call ndx_node_free() when done.
 *
 * Validates:
 *   - page_no in [1, total_pages)         -> NDX_ERR_BAD_PAGE
 *   - each entry group does not extend past offset 512  -> NDX_ERR_PAGE_OVF
 * The trailing child pointer is read only if it fits within the page boundary
 * (ndx.md ss3.2 overflow caveat: a full leaf may not have room for all 4 bytes
 * of the trailing child, so we guard).
 *
 * Returns NDX_OK (0) with *node_out set on success.
 * Returns -ndx_err (negative) with *node_out == NULL on failure.
 *
 * Ref: ndx.md ss3 (node header), ss3.1 (group layout), ss3.2 (trailing child);
 *      spec/samir/ndx_format.h NDX_NODE_* and NDX_GRP_* constants.
 */
int ndx_read_node(ndx_index *idx, uint32_t page_no, ndx_node_t **node_out);

/*
 * ndx_node_free: release arena memory for NODE.
 *
 * Unwinds the bump arena to the mark saved before the node was allocated.
 * NODE is invalid after this call.
 */
void ndx_node_free(ndx_index *idx, ndx_node_t *node);

/* -----------------------------------------------------------------------
 * S4.2: Key decode + collation API
 * ----------------------------------------------------------------------- */

/*
 * ndx_key_decode: decode KEY_DATA bytes to a typed xb_val.
 *
 * Dispatches on ndx_key_type(idx):
 *   key_type == 0 (NDX_KEY_TYPE_CHAR):
 *     Produces XB_C. OUT->u.c.p is set to BUF (the caller's buffer); the raw
 *     key_data bytes (ndx_key_length(idx) of them, including trailing spaces)
 *     are copied into BUF via rt_memcpy. BUF must have room for at least
 *     ndx_key_length(idx) bytes. Ownership: BUF is caller-owned; the xb_val
 *     is valid as long as BUF is valid (value.h "NOT owned" contract).
 *     [ndx.md ss4.1: "key_data is key_length bytes of space-padded ASCII (OEM/
 *     CP437), left-justified, padded on the right with 0x20."]
 *
 *   key_type == 1 (NDX_KEY_TYPE_NUMERIC, covers both N and D fields):
 *     Produces XB_N. The 8 key_data bytes are decoded as a little-endian
 *     IEEE-754 double via rt_memcpy (no aliasing UB) and stored in out->u.n.
 *     BUF is unused (may be NULL).
 *
 *     N-vs-D BOUNDARY NOTE: the .ndx header stores key_type=1 for BOTH numeric
 *     (N field) and date (D field) expressions -- both are raw LE doubles. The
 *     distinction between N and D is NOT in the .ndx file; it comes from the
 *     key expression and the .dbf field descriptor (available to the interpreter
 *     at S5.x). ndx_key_decode therefore always produces XB_N for type-1 keys.
 *     A caller that knows the key is a date column may construct XB_D from the
 *     same double (the JDN value) using xb_d(). For collation (ndx_key_cmp,
 *     S4.3 SEEK, S4.4 build) the N/D distinction is irrelevant: both are
 *     compared arithmetically as doubles.
 *     [ndx.md ss4.2 / re/mint-results-001.md: "key_type=1 for BOTH numeric
 *     and date". Plan Sec 3.3: "NDX numeric keys = raw LE IEEE-754 doubles,
 *     NO sign-flip, compared ARITHMETICALLY."]
 *
 * Returns NDX_OK (0) on success, -NDX_ERR_BAD_KEYTYPE if key_type is not 0/1
 * (should not occur for a successfully opened index, but fail loud: Rule 2).
 *
 * Ref: ndx.md ss4.1 (char), ss4.2 (numeric/date); re/mint-results-001.md;
 *      docs/plans/SAMIR-implementation-plan.md S4.2 contract.
 */
int ndx_key_decode(const ndx_index *idx, const uint8_t *key_data,
                   char *buf, xb_val *out);

/*
 * ndx_key_cmp: compare two raw key_data byte arrays for B-tree collation order.
 *
 * Both A and B must be ndx_key_length(idx) bytes.
 *
 * Returns: < 0 if A < B, 0 if A == B, > 0 if A > B.
 *
 * key_type == 0 (char): unsigned byte compare over the full key_length.
 *   This is CP437 unsigned byte order: uppercase before lowercase; digits
 *   before letters; no case-folding. Matches plain memcmp over unsigned bytes.
 *   [ndx.md ss6: "Character keys collate by unsigned byte value (ASCII/OEM
 *   CP437), left-to-right, space-padded. Verified: CNAMES 'DeBello'(0x42)
 *   sorts before 'Dean'(0x61) because 'B'(0x42) < 'a'(0x61)"]
 *
 * key_type == 1 (numeric/date): decode both A and B as LE IEEE-754 doubles
 *   and compare arithmetically. The double comparison is < / == / > on the
 *   decoded values. THIS IS NOT MEMCMP: raw IEEE doubles do not byte-sort
 *   correctly across the sign boundary (the MSB is the sign bit, making
 *   negatives appear larger than positives in a raw byte compare). The minted
 *   NCOST.NDX leaf is in true numeric order (-123.45 < -1 < 0 < 279) even
 *   though the sign bit is set in the high byte of the negative doubles.
 *   [ndx.md ss4.2 / re/mint-results-001.md: "Key comparison is ARITHMETIC,
 *   not byte-wise, for key_type==1." VERIFIED minted NCOST.NDX 2026-06-16]
 *
 *   Mutation hook: build with -DNDX_MUTATE_KEY_SIGNFLIP to apply the sign-flip
 *   transform that mint-001 DISPROVED (XOR the high bit of byte 7, equivalent
 *   to negating the double for negative values). Under this mutation the
 *   numeric ordering assertions go RED (the leaf traversal will no longer be
 *   monotonically ascending for negatives). This is the canonical mutation for
 *   S4.2: it implements the hypothesis that mint-001 was specifically minted to
 *   refute. [re/mint-results-001.md: "corrects the prior ndx.md speculation
 *   about a sign-flip transform -- there is none."]
 *
 * Ref: ndx.md ss4.2, ss6; re/mint-results-001.md; plan Sec 3.3, S4.2 contract.
 */
int ndx_key_cmp(const ndx_index *idx, const uint8_t *a, const uint8_t *b);

#endif /* INITECH_SAMIR_NDX_H */

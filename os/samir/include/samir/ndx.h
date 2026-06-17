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
    NDX_ERR_OOM          = 9,  /* PAL arena exhausted */
    NDX_ERR_CYCLE        = 10  /* B-tree descent revisited a page / overran the
                                * total_pages depth bound -- a corrupt index
                                * cycle. Fail loud (Rule 2) instead of looping. */
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

/* -----------------------------------------------------------------------
 * S4.3: B-tree traverse + SEEK API
 *
 * The load-bearing B-tree navigation. A silently-wrong descent corrupts every
 * SEEK / ORDER built on the index. The descent rule is fixed by ndx.md ss5 (NOT
 * guessed): the branch separator at entry i is the HIGH key of the subtree
 * rooted at child_page[i]; search descends into the first separator entry whose
 * key >= target, or into the rightmost (trailing) child if target exceeds all
 * separators.
 *
 * Ref (Law 1):
 *   - ndx.md ss5 (B-tree ordering invariant + search algorithm; "Branch
 *     separator = HIGH key of subtree", "scan entries in order until you find
 *     an entry whose key >= K ... descend into that entry's child_page (or the
 *     rightmost child if K exceeds all separators)").
 *   - ndx.md ss8 step 3 (in-order traversal: recurse child before emitting the
 *     separator key (branch); emit (key_data, dbf_recno) at a leaf; after the
 *     last entry, if internal, recurse into the trailing child pointer).
 *   - ndx.md ss3.2 (trailing/rightmost child pointer).
 *   - re/mint-results-002.md (SET EXACT default OFF; char "=" is directional
 *     begins-with: LEFT begins with RIGHT; "ab"="a" -> .T., "a"="ab" -> .F.).
 *   - docs/plans/SAMIR-implementation-plan.md S4.3 contract.
 * ----------------------------------------------------------------------- */

/*
 * ndx_visit_fn: callback invoked once per leaf entry during an in-order
 * traversal, in ascending collation order (ndx.md ss5/ss6).
 *
 *   ctx       opaque caller context passed through ndx_inorder.
 *   key_data  pointer to the leaf entry's raw key_data (ndx_key_length bytes);
 *             valid ONLY for the duration of the callback (it points into the
 *             arena node that is freed after the callback returns). The callback
 *             MUST copy any bytes it needs to retain.
 *   recno     the leaf entry's 1-based dbf_recno.
 *
 * Return 0 to continue the traversal; any non-zero value aborts the traversal
 * immediately and is propagated back as the return value of ndx_inorder
 * (callers use this to stop early or to signal a caller-side failure).
 */
typedef int (*ndx_visit_fn)(void *ctx, const uint8_t *key_data, uint32_t recno);

/*
 * ndx_inorder: in-order traverse the whole B-tree, invoking VISIT for every
 * leaf entry in ascending key order (across all leaves).
 *
 * Descends leftmost-first; at a branch entry it recurses into child_page before
 * the next entry; at the trailing slot of a branch it recurses into the
 * rightmost child (ndx.md ss8 step 3). Leaf separators are never emitted (only
 * leaf entries with child_page == 0 produce a callback).
 *
 * Bounded recursion (Rule 2): a corrupt index could in principle form a cycle;
 * every descent is bounded by ndx_total_pages(idx) and any page revisit or
 * over-deep descent fails loud with -NDX_ERR_CYCLE. Recursion depth is bounded
 * by the same total_pages count.
 *
 * Returns:
 *   NDX_OK (0)            traversal completed; every leaf entry visited.
 *   < 0  (-ndx_err)       structural error (I/O, bad page, cycle, OOM).
 *   > 0                   the value VISIT returned (early abort by the caller).
 *
 * Ref: ndx.md ss5, ss8 step 3; ss3.2 (trailing child).
 */
int ndx_inorder(ndx_index *idx, ndx_visit_fn visit, void *ctx);

/*
 * ndx_seek: B-tree SEEK for KEY, honouring SET EXACT.
 *
 * Descends the tree per ndx.md ss5: at each node it finds the first entry whose
 * key >= KEY (via ndx_key_cmp); for a branch it descends into that entry's
 * child_page, or into the rightmost child if KEY exceeds all separators; for a
 * leaf the first entry with key >= KEY is the LANDING entry.
 *
 * Match semantics at the landing entry (set by *found_out):
 *   key_type 1 (numeric/date):
 *     FOUND iff the landing key compares arithmetically EQUAL to KEY
 *     (ndx_key_cmp == 0). SET EXACT is irrelevant for N/D. KEY must be XB_N or
 *     XB_D; its double is encoded to the 8-byte search key.
 *   key_type 0 (character):
 *     SET_EXACT == 0 (OFF, the III+ default): begins-with / directional match
 *       -- FOUND iff the stored landing key BEGINS WITH the search key over the
 *       search key's significant length (trailing spaces of KEY are trimmed
 *       before comparison; the stored key is left-justified space-padded).
 *       [re/mint-results-002.md: char "=" is directional, LEFT begins with
 *        RIGHT; in SEEK the stored leaf key is LEFT, the search key is RIGHT.]
 *     SET_EXACT != 0 (ON): full-length equality over ndx_key_length bytes
 *       (the search key is space-padded to key_length before comparison).
 *     KEY must be XB_C (or XB_M); shorter than key_length is right-padded with
 *     spaces, longer is truncated to key_length.
 *
 * On a successful descent to a leaf:
 *   *recno_out is set to the landing entry's dbf_recno (the FIRST leaf entry
 *     with key >= KEY). When *found_out is 1 this is the record whose key
 *     matched; when *found_out is 0 it is still set to the landing recno (the
 *     record at the insertion point), so callers implementing FIND-style
 *     positioning can use it. If the landing point is past the last leaf entry
 *     (KEY greater than every key in the index) *recno_out is set to 0 and
 *     *found_out to 0 (EOF: positioned past the end).
 *
 * Either recno_out or found_out may be NULL (the caller may want only one).
 *
 * Returns:
 *   NDX_OK (0)            descent reached a leaf; *found_out / *recno_out set.
 *   -NDX_ERR_BAD_KEYTYPE  KEY's xb_type is incompatible with the index key_type.
 *   < 0  (other -ndx_err) structural error (I/O, bad page, cycle, OOM).
 *
 * Bounded descent (Rule 2): bounded by ndx_total_pages; a cycle or over-deep
 * descent fails loud with -NDX_ERR_CYCLE rather than looping forever.
 *
 * Ref: ndx.md ss5 (search algorithm + HIGH-key separator invariant);
 *      re/mint-results-002.md (SET EXACT OFF default + directional begins-with);
 *      docs/plans/SAMIR-implementation-plan.md S4.3 contract.
 */
int ndx_seek(ndx_index *idx, const xb_val *key, int set_exact,
             uint32_t *recno_out, int *found_out);

/* -----------------------------------------------------------------------
 * S4.4: bulk INDEX ON build
 *
 * Builds a fresh .ndx file that reproduces dBASE III PLUS 1.1's bulk
 * `INDEX ON <expr> TO <file>` / `REINDEX` output BYTE-FOR-BYTE in every
 * MEANINGFUL byte. The build is the canonical "pack-then-parent" algorithm
 * minted from real dBASE (re/mint-results-003.md, ndx.md Open questions
 * "bulk build" RESOLVED 2026-06-16):
 *
 *   1. Collect one key per record, in physical record order (recno 1..nrec),
 *      via the caller-supplied key-provider callback (see ndx_key_provider).
 *   2. STABLE sort the (key, recno) pairs by ndx_key_cmp; ties (equal keys)
 *      keep ASCENDING recno (the physical-order tie-break -- VERIFIED against
 *      minted BIGIDX.NDX: the five "Beman" entries appear in recno order
 *      42,91,140,189,238).
 *   3. PACK LEAVES 100% LEFT-TO-RIGHT: fill leaf 1 to keys_per_page in key
 *      order, leaf 2 to keys_per_page, ..., with the REMAINDER in the LAST
 *      leaf -- NOT a 50/50 split. (245 records, kpp 18 -> 13 full leaves +
 *      1 leaf of 11. VERIFIED BIGIDX.NDX; NCOST.NDX 33 = 31 + 2.)
 *   4. BUILD THE PARENT (root branch) over the leaves' HIGH keys: for L leaves
 *      the root has L-1 separator entries (N keys / N+1 children, ndx.md ss5),
 *      where separator i = the LAST (high) key of leaf i (child_page = leaf i,
 *      recno = 0), and the trailing child = the last leaf. (VERIFIED: BIGIDX
 *      root sep[i] == last key of leaf i+1; NCOST root sep[0]=3295.0 == leaf 1
 *      high key; ZIPCODE root sep[0]="91306" == leaf 1 high key.)
 *
 * Page numbering (VERIFIED minted/pristine): leaves occupy pages 1..L
 * (left-to-right). When L == 1 the single leaf IS the root: root_page = 1,
 * total_pages = 2 (NDATE.NDX: 30 keys, root=1, total=2). When L > 1 the root
 * branch is the LAST page: root_page = L+1, total_pages = L+2 (NCOST.NDX:
 * root=3, total=4; BIGIDX.NDX: root=15, total=16).
 *
 * MEANINGFUL vs NORMALIZE bytes (Rule 11 + ndx.md ss1 "a byte-exact WRITER
 * need not reproduce specific garbage, but a READER must ignore it"):
 *   MEANINGFUL (reproduced exactly): the 10 header fields except dummy/reserved;
 *     the key_expr (verbatim + its NUL); per-node entry_count + the live entry
 *     groups (child_page, dbf_recno, key_data); the branch separators and the
 *     trailing child pointer.
 *   NORMALIZE (written as 0x00, since dBASE leaves stale heap garbage there):
 *     header reserved@0x08 and dummy@0x14; the header tail after the key_expr
 *     NUL (0x18+len+1 .. 511); per-group filler after key_data; the node
 *     filler word@0x02; the unused page tail after the live entries (and the
 *     non-child bytes of the trailing slot). SAMIR is MORE normalized than
 *     genuine III+; the oracle masks these regions before cmp (documented in
 *     test_ndx_build.c).
 *
 * The trailing child pointer in a BRANCH node is MEANINGFUL (the rightmost
 * subtree, ndx.md ss3.2); only its accompanying recno/key bytes are NORMALIZE.
 *
 * DECOUPLING (plan + bead initech-ahu.4): ndx_build does NOT evaluate the key
 * expression itself -- it would otherwise pull core/eval.c into the codec and
 * break every test that links ndx.c without the evaluator. Instead the caller
 * passes a KEY PROVIDER callback that renders each record's key bytes (the
 * Phase-5 interpreter / the test harness owns the dbf + evaluator). fs/ndx.c
 * therefore depends only on pal.h / rt.h / value.h, NOT on core/eval.c.
 * key_expr is passed in verbatim (the caller already has it from the .ndx being
 * rebuilt or from the INDEX ON command text).
 * ----------------------------------------------------------------------- */

/*
 * ndx_key_provider: render the on-disk key bytes for record RECNO.
 *
 *   user     opaque caller context (typically {dbf_table*, parsed AST, ...}).
 *   recno    1-based DBF record number (ndx_build calls it for 1..nrec in
 *            physical order).
 *   key_out  caller writes EXACTLY key_len bytes of the on-disk key here:
 *              char key (key_type 0): left-justified, RIGHT-padded with 0x20
 *                spaces to key_len (ndx.md ss4.1).
 *              numeric/date key (key_type 1): the 8-byte little-endian IEEE-754
 *                double (ndx.md ss4.2; key_len is always 8).
 *   key_len  the index key_length (the exact number of bytes to write).
 *
 * Returns 0 on success; non-zero aborts the build and propagates back as the
 * ndx_build return (callers signal an eval/read failure this way).
 */
typedef int (*ndx_key_provider)(void *user, uint32_t recno,
                                uint8_t *key_out, uint16_t key_len);

/*
 * ndx_build: build a fresh .ndx at OUT_NAME from NREC records' keys.
 *
 *   pal       the PAL (the OUT_NAME file is created PAL_RDWR|PAL_CREATE|PAL_TRUNC
 *             and written sequentially; the engine never touches the OS directly).
 *   out_name  the output .ndx path.
 *   key_type  NDX_KEY_TYPE_CHAR (0) or NDX_KEY_TYPE_NUMERIC (1). Validated.
 *   key_len   the key_length: any value for char; MUST be 8 for type-1.
 *   key_expr  the index expression, verbatim, NUL-terminated. Copied into the
 *             header at 0x18, capped at NDX_HDR_KEY_EXPR_SIZE (100) bytes.
 *   nrec      number of records (the key provider is called for recno 1..nrec).
 *   get_key   the key-provider callback (above). Must be non-NULL.
 *   user      opaque context passed to get_key.
 *
 * Determinism (Rule 11): identical (keys, recnos, key_expr, key_type, key_len)
 * produce a byte-identical file every run -- no clock, no host paths, no RNG.
 * Every NORMALIZE byte is emitted 0x00.
 *
 * The build uses the PAL arena for the key buffer and one 512-byte page buffer;
 * it resets the arena to its entry mark before returning (success or failure).
 *
 * Returns:
 *   NDX_OK (0)             the file was built and fully written.
 *   -NDX_ERR_BAD_KEYTYPE   key_type not 0/1, or type-1 with key_len != 8.
 *   -NDX_ERR_BAD_GROUP     key_len is 0 or so large that group_length (a u16)
 *                          or a single leaf would overflow a 512-byte page
 *                          (keys_per_page would be 0).
 *   -NDX_ERR_OOM           the PAL arena could not hold the key buffer.
 *   -NDX_ERR_IO            a PAL open/seek/write failed, or get_key aborted
 *                          (non-zero) -- in the get_key case the callback's
 *                          non-zero code is returned NEGATED if it is positive,
 *                          else -NDX_ERR_IO. (Callers usually use a sentinel.)
 *   -NDX_ERR_PAGE_OVF      MULTI-LEVEL interior packing required: the number of
 *                          leaves L exceeds keys_per_page + 1, so the L-1 root
 *                          separators do not fit in ONE branch page and an
 *                          interior level above the root would be needed. That
 *                          packing geometry is corpus-OPEN (no minted golden
 *                          for a 3-level III+ .ndx), so the build FAILS LOUD
 *                          rather than guess (Law 1 / Rule 2). The single-leaf
 *                          and single-root-branch (two-level) cases are
 *                          byte-exact and fully supported.
 *
 * Mutation hook (Rule 6 / plan S4.4 "50/50 split"):
 *   Build with -DNDX_MUTATE_SPLIT_5050. Leaves are packed ~50/50 (each leaf
 *   gets ceil(N / (2*L_correct)) ... ) instead of 100%-then-remainder. The
 *   leaf fill levels and the resulting node bytes diverge from the golden ->
 *   the byte-exact rebuild oracle goes RED. [re/mint-results-003.md: bulk build
 *   is 100%-pack, NOT 50/50.]
 *
 * Ref (Law 1):
 *   - ../dbase3-decomp/re/mint-results-003.md (bulk-build pack 100% L->R,
 *     remainder last; N keys / N+1 children root; minted BIGIDX.NDX).
 *   - ../dbase3-decomp/specs/file-formats/ndx.md ss1 (geometry + writer-garbage
 *     note), ss2 (header), ss3/ss3.1/ss3.2 (node + group + trailing child),
 *     ss5 (HIGH-key separator invariant).
 *   - spec/samir/ndx_format.h (LOCKED offsets).
 *   - docs/plans/SAMIR-implementation-plan.md S4.4 contract; Sec 2.E (fixed
 *     APPEND order before bulk INDEX ON; insertion-ordered layout).
 */
int ndx_build(samir_pal_t *pal, const char *out_name,
              uint16_t key_type, uint16_t key_len, const char *key_expr,
              uint32_t nrec, ndx_key_provider get_key, void *user);

#endif /* INITECH_SAMIR_NDX_H */

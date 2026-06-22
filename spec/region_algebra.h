/*
 * spec/region_algebra.h -- ATKINSON region engine: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.2 -- "the load-bearing math").
 * ATKINSON is the SINGLE scanline-inversion-list Boolean set engine that is the
 * shared spine behind BOTH the System-7 QuickDraw Rgn family AND the Win-3.1 GDI
 * HRGN family: clipping, window overlap, visRgn/clipRgn, the update/damage model,
 * and the GDI region calls all reduce to ONE region algebra. BOTH heritages clip
 * through the ONE region_op (Sec 6); NEITHER heritage owns it (ADR-0005 Amendment
 * AM-1, AM-1/AM-2). The two heritage names -- QuickDraw (UnionRgn/SectRgn/...)
 * and GDI (CombineRgn RGN_OR/RGN_AND/...) -- are strict PEER facades over the one
 * neutral primitive (Sec 7 = QuickDraw facade, Sec 7b = GDI facade). This header
 * is the contract BOTH the engine (os/flair/atkinson, the .c sources) and its
 * property suite (harness/proptest/test_region.c) include. The region's INTERNAL
 * byte format is OUR clean-room design (DEC-04a-R1 / ADR-0005); it is NOT a port
 * of QuickDraw's proprietary, unpublished region body NOR of the GDI HRGN body --
 * only the public op NAMES, mode codes, and Rect field order are carried, from
 * both heritages, for period authenticity.
 *
 * Source / Law 1 citations (all local):
 *   PRD Sec 6.2 -- a region is a pixel set R in Z^2, represented per scanline by
 *     a sorted list of INVERSION POINTS x0 < x1 < ... at which membership
 *     toggles (a normalized RLE). Regions over a bounding rectangle form a
 *     Boolean algebra under union/intersect/diff/xor and complement; all ops
 *     are a SCANLINE MERGE of inversion lists. The normal form is the minimal
 *     inversion-point set (no redundant/adjacent toggles). The correctness
 *     oracle is the HOMOMORPHISM property:
 *       rasterize(A OP B) == rasterize(A) OP_set rasterize(B)   (bit-exact),
 *     plus normalize-idempotence and the algebra identities.
 *   docs/adr/ADR-0005-ATKINSON-Region-Engine.md (pending) -- the clean-room
 *     inversion-list decision; the 5 normal-form invariants; the homomorphism
 *     suite as the ENTIRE correctness signal (no external golden -- QuickDraw's
 *     region body format is proprietary/unpublished).
 *   docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md (pending) -- the umbrella
 *     5-layer stack this engine anchors; operator decisions: indexed-8 internal
 *     offscreen depth, 640x480 native resolution (so int16 region coords are
 *     framebuffer-bounded, 640x480 << 32767).
 *   Inside Macintosh I, "QuickDraw" -- the System-7 heritage facade: the PUBLIC
 *     region op names (UnionRgn, SectRgn, DiffRgn, XorRgn, SetRectRgn,
 *     SetEmptyRgn, PtInRgn, RectInRgn, EmptyRgn, EqualRgn) and the Rect field
 *     order (top,left,bottom,right), carried verbatim for authenticity. The
 *     0x7FFF in-band sentinel that QuickDraw's body reportedly uses is UNVERIFIED
 *     lore and is NOT adopted (see GUARDRAIL below).
 *   Win-3.1 "wingdi.h" -- the GDI heritage facade (dual provenance, ADR-0005
 *     Amendment AM-1): the PUBLIC HRGN combine entry CombineRgn(dst,src1,src2,
 *     mode), the RGN_* mode codes (RGN_AND=1, RGN_OR=2, RGN_XOR=3, RGN_DIFF=4,
 *     RGN_COPY=5), the region-type complexity return (ERROR=0, NULLREGION=1,
 *     SIMPLEREGION=2, COMPLEXREGION=3), and the GDI argument orders for
 *     PtInRegion/RectInRegion/GetRgnBox -- carried verbatim for authenticity
 *     (Sec 7b). The GDI body format is likewise proprietary and is NOT ported;
 *     only the names/codes are borrowed.
 *
 * DUAL-COMPILE (the console.c/int21.c pattern): the engine that includes this
 * header compiles BOTH freestanding for the kernel (gcc -m32 -ffreestanding
 * -nostdlib -std=c11) AND hosted for the property suite. The ONLY dependencies
 * here are <stdint.h>/<stddef.h>, which are freestanding-provided. There is NO
 * host malloc: ALL x-data lives in one contiguous, caller-supplied x_pool
 * (arena/static-backed), so the rep is freestanding-legal and its layout is
 * deterministic (Rule 11). Invariant violations FAIL LOUD (Rule 2): the engine
 * panics in-kernel / CHECK-fails hosted (see region_assert_normal).
 *
 * Coordinates are int16 (period-authentic; QuickDraw used 16-bit Rect coords)
 * and framebuffer-bounded to 640x480 (operator decision; 640x480 << 32767, so
 * no in-band coordinate can collide with any chosen sentinel value -- but see
 * the GUARDRAIL: we use an EXPLICIT per-row x_count and therefore need NO
 * in-band sentinel at all).
 *
 * ASCII-clean (Rule 12). No nondeterminism / no timestamps (Rule 11).
 */
#ifndef INITECH_SPEC_REGION_ALGEBRA_H
#define INITECH_SPEC_REGION_ALGEBRA_H

#include <stdint.h>
#include <stddef.h>

/* ===========================================================================
 * 1. TYPES
 * ===========================================================================*/

/* rgn_rect_t -- QuickDraw Rect field order (top,left,bottom,right). int16 is
 * period-authentic and framebuffer-bounded to 640x480. A rect is EMPTY iff
 * (right <= left) || (bottom <= top); the engine treats such a rect as the
 * empty set. The rect spans pixel columns [left, right) and rows [top, bottom)
 * -- half-open, matching the inversion-list convention below (a toggle AT x
 * starts/ends a span, so [left,right) needs inversion points {left, right}). */
typedef struct rgn_rect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} rgn_rect_t;

/* rgn_row_t -- one scanline run of inversion points. `x` points INTO the owning
 * region's x_pool (NOT a per-row malloc -- see region_t). The x-values are
 * strictly increasing and EVEN in count (pairs [x0,x1), [x2,x3), ... are the
 * "inside" spans on this scanline). This row is valid for every scanline y in
 * [y_top, next_row.y_top) -- vertical run-length encoding: a row stands until
 * the next row's y_top supersedes it (the final/closing row carries x_count==0
 * and bounds the last live band from below). */
typedef struct rgn_row {
    int16_t  y_top;    /* first scanline this run applies to                  */
    uint16_t x_count;  /* number of inversion points (EVEN; 0 == closing row) */
    int16_t *x;        /* -> region.x_pool; strictly increasing, length x_count */
} rgn_row_t;

/* region_t -- a region. ALL x-data for ALL rows lives in ONE contiguous x_pool
 * (arena/static-backed; deterministic layout; NO per-row malloc, Rule 11). The
 * bbox mirrors QuickDraw's rgnBBox. is_rect/is_empty are the fast-path flags and
 * the rgnSize==10 analogue (a rectangular region is the QuickDraw 10-byte
 * special case -- bbox alone determines it; the rows are still materialized so
 * every op has a uniform input). */
typedef struct region {
    rgn_rect_t bbox;        /* tight bounding box of the pixel set (rgnBBox)   */
    uint16_t   n_rows;      /* number of live rows (incl. the closing row)     */
    uint16_t   cap_rows;    /* capacity of rows[] (fail-loud on overflow)      */
    rgn_row_t *rows;        /* sorted by y_top, ascending, no duplicate y_top  */
    int16_t   *x_pool;      /* contiguous backing store for every row's x[]    */
    uint32_t   x_pool_used; /* int16 slots consumed in x_pool                  */
    uint32_t   x_pool_cap;  /* capacity of x_pool in int16 slots               */
    uint8_t    is_rect;     /* 1 == rectangular fast-path (rgnSize==10 analogue)*/
    uint8_t    is_empty;    /* 1 == the empty set (canonical: 0 rows, null bbox)*/
} region_t;

/* ===========================================================================
 * 2. THE FIVE NORMAL-FORM INVARIANTS
 * ---------------------------------------------------------------------------
 * region_assert_normal() checks ALL FIVE and is called at the TOP of every op
 * (panic in-kernel / CHECK hosted). A merge fed a NON-normal-form inversion
 * list produces plausible-looking garbage that passes non-overlapping cases --
 * exactly the bug the property suite + these asserts exist to catch (Rule 2,
 * Rule 3 "all bugs are deep"). normalize() establishes all five and is
 * IDEMPOTENT: normalize(normalize(R)) == normalize(R), BIT-EXACT.
 *
 *   (1) STRICTLY INCREASING: within every row, x[0] < x[1] < ... < x[k-1].
 *       (No duplicate or out-of-order inversion points; a duplicate would be a
 *       zero-width span that must collapse away.)
 *
 *   (2) EVEN LENGTH: every row's x_count is even (each "inside" span is a
 *       [start,end) pair). An odd count would mean a span runs to +infinity,
 *       which is impossible in a framebuffer-bounded region.
 *
 *   (3) VERTICAL-RLE COLLAPSE: no two CONSECUTIVE rows share an identical
 *       x-list. If row r and row r+1 have byte-identical inversion lists, r+1 is
 *       redundant (r already covers down to r+1.y_top and beyond) and MUST be
 *       dropped. This is the vertical run-length minimality that makes the form
 *       canonical (and makes region_equal a structural memcmp).
 *
 *   (4) NO REDUNDANT EMPTY ROW: an empty row (x_count==0) is legal ONLY as
 *       (a) a GAP-CLOSING band terminator -- an empty row between two DIFFERENT
 *       non-empty bands, which encodes a vertical hole. This row is LOAD-BEARING:
 *       since a row is valid for [y_top, next.y_top), the ONLY way the model can
 *       represent a gap (e.g. the union of two vertically-disjoint rects, a basic
 *       homomorphism-oracle case) is an empty row at the gap's top; or
 *       (b) the FINAL CLOSING row, which bounds the last live band from below
 *       (its y_top == bbox.bottom).
 *       FORBIDDEN: a LEADING empty row (no live span opened yet, dead space above
 *       the first band) and an EMPTY-UNDER-EMPTY row (already excluded by (3),
 *       vertical-RLE). normalize() drops exactly those. (Reconciled to the
 *       engine + the homomorphism oracle this session: the earlier "no empty
 *       interior row" wording wrongly outlawed the load-bearing gap-closer.)
 *
 *   (5) SORTED, UNIQUE y_top: rows are sorted by y_top ascending with NO
 *       duplicate y_top. (Two rows at the same y_top is a contradiction -- only
 *       one inversion list can apply to a given scanline.)
 *
 * Canonical empty region: is_empty==1, n_rows==0, bbox=={0,0,0,0}. The empty
 * region trivially satisfies (1)-(5).
 * ===========================================================================*/

/* ===========================================================================
 * 3. THE OP TRUTH TABLES
 * ---------------------------------------------------------------------------
 * Every binary op is a y-band sweep over (A.rows + B.rows); within each band,
 * a per-scanline x-merge walks both x-lists in sorted order tracking the state
 * pair (inA, inB) and emits an inversion point ONLY when boolfn(inA,inB)
 * changes. The FOUR ops differ ONLY by boolfn -- this is the entire algebra:
 *
 *   UNION     out = inA OR  inB        (in A, or in B)
 *   INTERSECT out = inA AND inB        (in A, and in B)
 *   DIFF      out = inA AND (NOT inB)  (in A, and NOT in B)        == ANDNOT
 *   XOR       out = inA XOR inB        (in exactly one of A, B)
 *
 * complement(A, frame) is RELATIVE TO AN EXPLICIT FRAME RECT: there is NO
 * infinite universe (a framebuffer-bounded region cannot represent one).
 *   complement(A, frame) := region_op(A, region_of(frame), XOR)
 * i.e. the frame minus A (over the frame). State this everywhere complement is
 * offered: "complement" without a frame is meaningless here.
 * ===========================================================================*/
/* THREE name columns (heritage-symmetric, ADR-0005 Amendment AM-3): the internal
 * rgn_op_t, its System-7 QuickDraw name, and its Win-3.1 GDI CombineRgn mode.
 * Both heritages dispatch to the SAME op; neither owns it (Sec 7 / Sec 7b).
 *
 *   internal rgn_op_t   | QuickDraw name | GDI CombineRgn mode
 *   --------------------+----------------+--------------------
 *   RGN_OP_UNION        | UnionRgn       | RGN_OR
 *   RGN_OP_INTERSECT    | SectRgn        | RGN_AND
 *   RGN_OP_DIFF         | DiffRgn        | RGN_DIFF
 *   RGN_OP_XOR          | XorRgn         | RGN_XOR
 *
 * XOR-decomposition note (the dual-heritage proof point): GDI documents XOR as
 * (src1 SUB src2) UNION (src2 SUB src1) -- it reaches XOR a different SYNTACTIC
 * way -- but that is provably the SAME SET as the one XOR primitive, as the
 * existing _Static_assert at the XOR line (Sec 8, "XOR == UNION minus INTERSECT
 * on the 4-bit truth tables") pins. Two heritages, one set: neither owns the
 * engine. */
typedef enum rgn_op {
    RGN_OP_UNION     = 0,  /* OR(inA, inB)     QuickDraw UnionRgn / GDI RGN_OR  */
    RGN_OP_INTERSECT = 1,  /* AND(inA, inB)    QuickDraw SectRgn  / GDI RGN_AND */
    RGN_OP_DIFF      = 2,  /* ANDNOT(inA,inB)  QuickDraw DiffRgn  / GDI RGN_DIFF*/
    RGN_OP_XOR       = 3   /* XOR(inA, inB)    QuickDraw XorRgn   / GDI RGN_XOR */
} rgn_op_t;

/* The boolfn truth table as data (so a test can assert the engine's boolfn
 * matches the spec, and so the spec is machine-checkable, not prose). Index by
 * (inA*2 + inB): {00, 01, 10, 11} -> output membership bit. */
#define RGN_TRUTH_UNION      0x0Eu  /* bits: 00->0 01->1 10->1 11->1 = 1110b   */
#define RGN_TRUTH_INTERSECT  0x08u  /* bits: 00->0 01->0 10->0 11->1 = 1000b   */
#define RGN_TRUTH_DIFF       0x04u  /* bits: 00->0 01->0 10->1 11->0 = 0100b   */
#define RGN_TRUTH_XOR        0x06u  /* bits: 00->0 01->1 10->1 11->0 = 0110b   */

/* rgn_op_truth(op) -> the 4-bit truth table for that op (LSB = state 00, then
 * 01, 10, 11). region_op's boolfn(inA,inB) MUST equal
 * ((rgn_op_truth(op) >> (inA*2 + inB)) & 1u). A static-assert in the engine
 * pins boolfn to this table; the property suite re-derives it independently. */
static inline uint8_t rgn_op_truth(rgn_op_t op)
{
    switch (op) {
        case RGN_OP_UNION:     return RGN_TRUTH_UNION;
        case RGN_OP_INTERSECT: return RGN_TRUTH_INTERSECT;
        case RGN_OP_DIFF:      return RGN_TRUTH_DIFF;
        case RGN_OP_XOR:       return RGN_TRUTH_XOR;
    }
    return 0u; /* unreachable; caller passes a valid rgn_op_t */
}

/* ===========================================================================
 * 4. COORDINATE / SENTINEL CONVENTION  (our own -- GUARDRAIL)
 * ---------------------------------------------------------------------------
 * Each row carries an EXPLICIT x_count, so the inversion list is self-
 * delimiting. We therefore use NO in-band sentinel value to terminate an
 * x-list. In particular we DO NOT adopt the QuickDraw 0x7FFF in-band row
 * terminator: it is unverified lore about a proprietary format, and worse, it
 * would COLLIDE with a legal coordinate -- 0x7FFF (32767) is a valid int16, and
 * although our regions are framebuffer-bounded to 640x480 today, baking an
 * in-band magic into the LOCKED format would be a latent trap if the bound ever
 * grows. Length-prefixed (x_count) is unambiguous and sentinel-free.
 *
 * Coordinate space: int16, half-open spans [x_even, x_odd). Rows apply to
 * scanlines [y_top, next.y_top). Native resolution is 640x480 (operator
 * decision), well inside int16 -- but the format itself imposes no upper bound
 * beyond int16 and the storage caps below.
 * ===========================================================================*/

/* ===========================================================================
 * 5. STORAGE CAPS  (fail-loud on overflow, Rule 2)
 * ---------------------------------------------------------------------------
 * Fixed per-region caps sized for the common case (rects, chrome, single
 * windows, and the frame's worst window count). A region whose normal form
 * needs more rows or x-entries than these caps is a FAIL-LOUD panic, never a
 * silent truncation. A merge's output AND scratch are bounded a priori:
 *   rows_out <= rowsA + rowsB + 2   (each input band boundary can appear once,
 *                                    plus the opening and closing rows)
 *   x_out    <= xA + xB             (the merge never invents inversion points;
 *                                    a changed-output emit is at an existing
 *                                    A-edge or B-edge)
 * so an op into a region with RGN_ROWS_CAP/RGN_X_POOL_CAP sized to the inputs
 * cannot overflow if the inputs themselves are within caps.
 *
 * Sizing rationale (the frame, PRD Appendix A): the reference desktop shows on
 * the order of a half-dozen overlapping windows plus chrome; a window's update
 * region under a handful of overlaps stays well under a hundred rows. The caps
 * below are generous headroom over that worst case while remaining static-
 * arena-friendly. They are LOCKED (Rule 8): raising them is a deliberate act
 * with a beads issue + worklog note, motivated by a concrete region that needs
 * the room, never a silent bump to make one test pass. */
#define RGN_ROWS_CAP      256u   /* max live rows (incl. closing row) per region */
#define RGN_X_POOL_CAP   1024u   /* max int16 inversion-point slots per region   */

/* A single scanline cannot have more inversion points than this (used to size
 * the per-band x-merge scratch). 640px wide => at most 320 disjoint spans =>
 * 640 inversion points; this cap covers that with headroom. */
#define RGN_ROW_X_MAX     256u

/* ===========================================================================
 * 6. THE ENGINE API  (os/flair/atkinson .c sources -- authored in a later stage)
 * ---------------------------------------------------------------------------
 * These are the INTERNAL primitives (region_* names). The verbatim-QuickDraw
 * wrappers in section 7 are thin shims over region_op / these. Prototypes only;
 * NO implementation lives in this header (it is data + types + contract). The
 * engine .c files are explicitly NOT written in this (specs) stage.
 *
 * dst regions carry their own rows[]/x_pool storage (arena/static-backed); the
 * engine writes INTO that storage and FAILS LOUD if a cap is exceeded.
 * ===========================================================================*/

/* --- 6a (bead initech-jmo): rep + normalize-on-construction ----------------*/

/* Set `r` to the canonical empty region (is_empty=1, n_rows=0, null bbox). */
void region_set_empty(region_t *r);

/* Set `r` to the rectangle `rect`, normalized on construction:
 *   rows = [ (top, [left,right]), (bottom, []) ],  is_rect=1.
 * An empty rect (right<=left || bottom<=top) yields the empty region. */
void region_set_rect(region_t *r, rgn_rect_t rect);

/* Build `r` as the union of `n` rects (repeated union; result normalized). */
void region_from_rects(region_t *r, const rgn_rect_t *rects, uint16_t n);

/* Assert ALL FIVE normal-form invariants on `r` (section 2). Returns 1 if
 * normal; on violation it FAILS LOUD (panic in-kernel / CHECK hosted) -- it is
 * the runtime guard called at the TOP of every op, not a soft predicate. */
int region_assert_normal(const region_t *r);

/* Establish the normal form in place (idempotent, bit-exact): collapse adjacent
 * toggles, drop vertical-RLE-redundant rows, drop empty interior rows, recompute
 * bbox, set is_rect/is_empty. normalize(normalize(R)) == normalize(R). */
void region_normalize(region_t *r);

/* --- 6b (bead initech-b5g): the scanline merge + queries -------------------*/

/* out = A OP B, by y-band sweep + per-band x-merge under boolfn(op), then
 * region_normalize(out). out must have storage for the a-priori bound
 * (rowsA+rowsB+2 rows, xA+xB x-entries); overflow FAILS LOUD. A, B, out may
 * alias only if the engine documents it -- default contract: out is distinct. */
void region_op(region_t *out, const region_t *A, const region_t *B, rgn_op_t op);

/* out = complement of A relative to `frame` (= frame XOR A, over the frame).
 * NO infinite universe -- the frame is explicit (section 3). */
void region_complement(region_t *out, const region_t *A, rgn_rect_t frame);

/* O(1) queries off the maintained bbox / is_empty flag. */
rgn_rect_t region_get_bbox(const region_t *r);   /* returns bbox (rgnBBox)     */
int        region_is_empty(const region_t *r);   /* 1 iff the empty set        */

/* Point membership: binary-search the row covering v, then test h against that
 * row's x-list by parity (inside iff h in [x_2k, x_2k+1) for some k). */
int region_contains_point(const region_t *r, int16_t h, int16_t v);

/* Structural equality. Because the normal form is canonical, two equal regions
 * are byte-identical in (bbox, rows, x-lists) => region_equal is a structural
 * compare; it requires both inputs already normal. */
int region_equal(const region_t *A, const region_t *B);

/* rect FULLY in region (CONTAINMENT): 1 iff every pixel of `rect` is in `r`
 * (bbox reject, then an op-empty test: rect DIFF r == empty). Renamed from
 * region_rect_in_region per ADR-0005 Amendment AM-4; retained for window-clip
 * contexts that genuinely want containment. This is NOT the RectInRgn/
 * RectInRegion semantic -- those are OVERLAP (see region_rect_overlaps). */
int region_rect_fully_in(const region_t *r, rgn_rect_t rect);

/* rect OVERLAPS region: 1 iff `rect` and `r` share at least one pixel (bbox
 * reject, then INTERSECT-non-empty). This is the DOCUMENTED semantic of BOTH
 * heritages' RectInRgn (QuickDraw, "any overlap") and RectInRegion (GDI, "at
 * least partially inside") -- ADR-0005 Amendment AM-4 (the containment->overlap
 * deep-bug fix; binding constraint C-9). An empty rect overlaps nothing. */
int region_rect_overlaps(const region_t *r, rgn_rect_t rect);

/* Do the two regions share any pixel? (bbox reject, then INTERSECT-non-empty.) */
int region_intersects(const region_t *A, const region_t *B);

/* ===========================================================================
 * 7. HERITAGE FACADES -- QUICKDRAW FACADE  (period authenticity)
 * ---------------------------------------------------------------------------
 * The first of TWO strict-peer heritage facades over the ONE neutral region_op
 * (ADR-0005 Amendment AM-1; the GDI facade is Sec 7b). The PUBLIC Inside
 * Macintosh names, carried verbatim for authenticity, in the QuickDraw (srcA,
 * srcB, dstRgn) argument order. These are THIN WRAPPERS over region_op / the
 * section-6 primitives. The internal byte format is NOT a QuickDraw port
 * (section 0); only the names + Rect order are borrowed. NEITHER facade owns the
 * engine; both bottom out in region_op (binding constraint C-7).
 *
 *   UnionRgn(srcA, srcB, dst)  -> region_op(dst, srcA, srcB, RGN_OP_UNION)
 *   SectRgn (srcA, srcB, dst)  -> region_op(dst, srcA, srcB, RGN_OP_INTERSECT)
 *   DiffRgn (srcA, srcB, dst)  -> region_op(dst, srcA, srcB, RGN_OP_DIFF)
 *   XorRgn  (srcA, srcB, dst)  -> region_op(dst, srcA, srcB, RGN_OP_XOR)
 * ===========================================================================*/
void UnionRgn(const region_t *srcA, const region_t *srcB, region_t *dstRgn);
void SectRgn (const region_t *srcA, const region_t *srcB, region_t *dstRgn);
void DiffRgn (const region_t *srcA, const region_t *srcB, region_t *dstRgn);
void XorRgn  (const region_t *srcA, const region_t *srcB, region_t *dstRgn);

void SetRectRgn(region_t *rgn, int16_t left, int16_t top,
                int16_t right, int16_t bottom);   /* QuickDraw l,t,r,b order   */
void SetEmptyRgn(region_t *rgn);                   /* -> region_set_empty       */

int  PtInRgn  (int16_t h, int16_t v, const region_t *rgn); /* QuickDraw Point   */
/* RectInRgn -> region_rect_overlaps: 1 iff rect and rgn OVERLAP (share any
 * pixel), NOT containment. QuickDraw documents this as "any overlap" (ADR-0005
 * Amendment AM-4 / C-9; was wrongly containment pre-AM-4). For containment use
 * region_rect_fully_in. */
int  RectInRgn(rgn_rect_t rect, const region_t *rgn);
int  EmptyRgn (const region_t *rgn);
int  EqualRgn (const region_t *rgnA, const region_t *rgnB);

/* ===========================================================================
 * 7b. HERITAGE FACADES -- GDI/HRGN FACADE  (period authenticity)
 * ---------------------------------------------------------------------------
 * The SECOND strict-peer heritage facade over the ONE neutral region_op (a peer
 * of Sec 7; ADR-0005 Amendment AM-1/AM-2). The PUBLIC Win-3.1 wingdi.h names,
 * mode codes, region-type return convention, and argument orders, carried
 * VERBATIM for authenticity. These are THIN WRAPPERS over region_op + the
 * section-6 primitives + a region-type classifier -- ZERO new region math, ZERO
 * second engine (binding constraint C-7). The HRGN body format is NOT ported;
 * the GDI facade operates on the SAME in-engine region_t representation.
 *
 *   CombineRgn(dst, src1, src2, mode) -> region_op(dst, src1, src2, map(mode));
 *                                        RGN_COPY -> self-union identity copy.
 *   GetRgnBox (rgn, out)              -> *out = bbox; return region-type code.
 *   PtInRegion(rgn, x, y)            -> region_contains_point (GDI arg order).
 *   RectInRegion(rgn, rect)          -> region_rect_overlaps  (OVERLAP, AM-4).
 * ===========================================================================*/

/* GDI CombineRgn mode codes, carried VERBATIM from Win-3.1 wingdi.h. */
#define RGN_AND   1   /* dst = src1 AND src2          -> RGN_OP_INTERSECT       */
#define RGN_OR    2   /* dst = src1 OR  src2          -> RGN_OP_UNION           */
#define RGN_XOR   3   /* dst = src1 XOR src2          -> RGN_OP_XOR             */
#define RGN_DIFF  4   /* dst = src1 AND NOT src2      -> RGN_OP_DIFF            */
#define RGN_COPY  5   /* dst = copy of src1 (normalized; self-union identity)   */

/* GDI region-type / complexity return codes, VERBATIM from wingdi.h. ERROR is
 * for invalid args only; the rest classify the NORMALIZED result. Named with a
 * RGN_TYPE_ prefix on ERROR to avoid colliding with the bare GDI ERROR token. */
#define RGN_TYPE_ERROR   0   /* invalid argument (NULL / bad mode)             */
#define NULLREGION       1   /* the empty region                              */
#define SIMPLEREGION     2   /* a single rectangle (is_rect fast-path)        */
#define COMPLEXREGION    3   /* a multi-span / multi-row region               */

/* map a CombineRgn mode to the internal rgn_op_t -- the SINGLE source of truth
 * for the mode->op wiring, used BOTH by the static-assert mapping oracle below
 * AND by rgn_op_from_combine_mode (so the machine-check exercises the real map).
 * It is a constant-foldable conditional expression so it is usable inside
 * _Static_assert under C11 (a static-inline function call is NOT a constant
 * expression and would not compile in an assert). RGN_COPY has no boolean op
 * (it is the self-union identity copy idiom) and is handled specially by
 * CombineRgn; this map is defined only for the four set ops. */
#define RGN_OP_FROM_MODE(m) \
    ((m) == RGN_AND  ? RGN_OP_INTERSECT : \
     (m) == RGN_OR   ? RGN_OP_UNION     : \
     (m) == RGN_XOR  ? RGN_OP_XOR       : \
     (m) == RGN_DIFF ? RGN_OP_DIFF      : RGN_OP_UNION)

static inline rgn_op_t rgn_op_from_combine_mode(int mode)
{
    return RGN_OP_FROM_MODE(mode);
}

/* CombineRgn(dst, src1, src2, mode): the GDI combine entry. Selects the Boolean
 * op by `mode` (RGN_AND/OR/XOR/DIFF) or copies src1 (RGN_COPY), normalizes the
 * result into dst, and RETURNS the GDI region-type code of the result
 * (NULLREGION/SIMPLEREGION/COMPLEXREGION). NULL args or an invalid mode return
 * RGN_TYPE_ERROR and write nothing. */
int CombineRgn(region_t *dst, const region_t *src1, const region_t *src2, int mode);

/* GetRgnBox(rgn, out): write rgn's bounding box to *out; RETURN the region-type
 * code of rgn. NULL args return RGN_TYPE_ERROR. */
int GetRgnBox(const region_t *rgn, rgn_rect_t *out);

/* PtInRegion(rgn, x, y): 1 iff pixel (x,y) is in rgn (GDI argument order). */
int PtInRegion(const region_t *rgn, int16_t x, int16_t y);

/* RectInRegion(rgn, rect): 1 iff rect OVERLAPS rgn (GDI argument order; OVERLAP,
 * not containment -- ADR-0005 Amendment AM-4 / C-9; GDI "at least partially
 * inside"). -> region_rect_overlaps. */
int RectInRegion(const region_t *rgn, rgn_rect_t rect);

/* --- Build-time GDI-constant oracle (AM-2/AM-3): a swapped wiring or a typo'd
 *     constant is a COMPILE error -- the cheapest oracle in the vector. ------- */

/* Each mode constant pinned VERBATIM to wingdi.h. */
_Static_assert(RGN_AND  == 1, "GDI RGN_AND == 1 (wingdi.h)");
_Static_assert(RGN_OR   == 2, "GDI RGN_OR == 2 (wingdi.h)");
_Static_assert(RGN_XOR  == 3, "GDI RGN_XOR == 3 (wingdi.h)");
_Static_assert(RGN_DIFF == 4, "GDI RGN_DIFF == 4 (wingdi.h)");
_Static_assert(RGN_COPY == 5, "GDI RGN_COPY == 5 (wingdi.h)");

/* Each region-type code pinned VERBATIM to wingdi.h. */
_Static_assert(RGN_TYPE_ERROR == 0, "GDI ERROR == 0 (wingdi.h)");
_Static_assert(NULLREGION     == 1, "GDI NULLREGION == 1 (wingdi.h)");
_Static_assert(SIMPLEREGION   == 2, "GDI SIMPLEREGION == 2 (wingdi.h)");
_Static_assert(COMPLEXREGION  == 3, "GDI COMPLEXREGION == 3 (wingdi.h)");

/* The mode->op mapping is machine-checked via the constant-foldable
 * RGN_OP_FROM_MODE macro (the SAME map rgn_op_from_combine_mode uses): a swapped
 * wiring (e.g. RGN_OR -> INTERSECT) becomes a COMPILE error -- the cheapest
 * oracle in the vector (ADR-0005 Amendment AM-3, Sec 7.5; mutation-proven by
 * deliberately mis-mapping one mode and confirming the build fails). */
_Static_assert(RGN_OP_FROM_MODE(RGN_AND)  == RGN_OP_INTERSECT,
               "map(RGN_AND) == RGN_OP_INTERSECT");
_Static_assert(RGN_OP_FROM_MODE(RGN_OR)   == RGN_OP_UNION,
               "map(RGN_OR) == RGN_OP_UNION");
_Static_assert(RGN_OP_FROM_MODE(RGN_XOR)  == RGN_OP_XOR,
               "map(RGN_XOR) == RGN_OP_XOR");
_Static_assert(RGN_OP_FROM_MODE(RGN_DIFF) == RGN_OP_DIFF,
               "map(RGN_DIFF) == RGN_OP_DIFF");

/* ===========================================================================
 * 8. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ===========================================================================*/

/* The 4-bit truth tables must be the classic boolfns; a typo is a build error.*/
_Static_assert(RGN_TRUTH_UNION     == 0x0Eu, "UNION truth table = 1110b");
_Static_assert(RGN_TRUTH_INTERSECT == 0x08u, "INTERSECT truth table = 1000b");
_Static_assert(RGN_TRUTH_DIFF      == 0x04u, "DIFF (A and not B) truth = 0100b");
_Static_assert(RGN_TRUTH_XOR       == 0x06u, "XOR truth table = 0110b");

/* Set-identity sanity at the bit level: XOR == UNION minus INTERSECT, on the
 * 4-bit truth tables (in exactly one == in either but not in both). */
_Static_assert((RGN_TRUTH_UNION & ~RGN_TRUTH_INTERSECT & 0x0Fu) == RGN_TRUTH_XOR,
               "XOR == UNION minus INTERSECT on the 4-bit truth tables");

/* The Rect is the QuickDraw 8-byte (4 x int16) record, field order t,l,b,r. */
_Static_assert(sizeof(rgn_rect_t) == 8, "rgn_rect_t is 4 x int16 (QuickDraw Rect)");
_Static_assert(offsetof(rgn_rect_t, top)    == 0, "Rect.top @ 0");
_Static_assert(offsetof(rgn_rect_t, left)   == 2, "Rect.left @ 2");
_Static_assert(offsetof(rgn_rect_t, bottom) == 4, "Rect.bottom @ 4");
_Static_assert(offsetof(rgn_rect_t, right)  == 6, "Rect.right @ 6");

/* Caps are mutually consistent (Rule 2 fail-loud is the runtime guard; this is
 * the static sanity that the constants agree). */
_Static_assert(RGN_ROW_X_MAX <= RGN_X_POOL_CAP, "a single row must fit the pool");
_Static_assert(RGN_ROWS_CAP >= 2u, "need room for >=1 live row + closing row");

#endif /* INITECH_SPEC_REGION_ALGEBRA_H */

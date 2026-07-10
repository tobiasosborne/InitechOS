/* test_region.c -- the ATKINSON region engine's property suite (the ORACLE).
 *
 * beads: initech-6dy ("C property suite: homomorphism + identities + shrinker,
 *        mutation-proven"); engine reps initech-jmo / initech-b5g.
 * Ref:   PRD Sec 6.2 -- "the load-bearing math". A region is a pixel set R in
 *        Z^2, represented per scanline by a sorted inversion list; regions over
 *        a bounding rect form a Boolean algebra under union/intersect/diff/xor
 *        and complement. The CORRECTNESS ORACLE is the HOMOMORPHISM property:
 *          rasterize(A OP B) == rasterize(A) OP_set rasterize(B)   (BIT-EXACT)
 *        i.e. rasterization is a Boolean-algebra homomorphism from the region
 *        representation to the powerset of pixels over the bounding box. We
 *        brute-force this against the pixel ground truth -- no proof assistant,
 *        no external golden (QuickDraw's region body is proprietary/unpublished;
 *        the homomorphism IS the entire correctness signal -- ADR-0005).
 * Ref:   spec/region_algebra.h -- the LOCKED contract (types, 5 normal-form
 *        invariants, 4 op truth tables, storage caps, complement-frame).
 * Ref:   CLAUDE.md Law 2 (the oracle is the truth), Rule 1 (RED->GREEN), Rule 6
 *        (golden/oracle mutation-proven), Rule 11 (seeded LCG -> deterministic),
 *        Rule 12 (ASCII).
 *
 * This is the test_mcb.c idiom: TEST_HARNESS()/CHECK, a seeded LCG so the fuzz
 * is reproducible, host-side (malloc the arena storage; the ENGINE itself does
 * NO host malloc -- all x-data lives in caller-supplied pools). Compiles HOSTED
 * and links the same region.c the kernel links freestanding (the dual-compile
 * pattern).
 *
 * The suite, in order of decisiveness:
 *   PRIMARY  -- homomorphism: rasterize(A OP B) == rasterize(A) OP_set raster(B)
 *               for all 4 ops + complement, over thousands of random regions.
 *               Generators include RAW random scanline-span sets (NOT only
 *               rect-unions) so non-rectangular normal-form bugs cannot hide.
 *   SECONDARY-- normalize-idempotence (bit-exact); algebra identities
 *               (commutativity, associativity, De Morgan, A DIFF A = empty,
 *               A XOR A = empty, A UNION comp(A) = frame); rect-fast-path ==
 *               general-path; region_equal structural consistency; normal-form
 *               invariants hold on every produced region.
 *   On failure a SHRINKER bisects the rect/span list and clamps coords to a
 *   MINIMAL counterexample, printed for the human.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>           /* fork, _exit            (the over-cap probe)   */
#include <sys/wait.h>         /* waitpid, WIF* macros                          */
#include <sys/resource.h>     /* setrlimit (suppress the child's core dump)    */
#include <signal.h>           /* SIGABRT                                       */

#include "region_algebra.h"   /* the LOCKED spec (-Ispec)            */
#include "region.h"           /* the engine constructors (-Ios/flair/atkinson) */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed) */

TEST_HARNESS();

/* ===========================================================================
 * The bounded grid + the pixel ground truth (rasterize / OP_set on bitmaps).
 * ---------------------------------------------------------------------------
 * We work over a small GW x GH grid (the homomorphism is bit-exact; a small
 * grid makes thousands of cases cheap). Coordinates stay well inside int16.
 * A bitmap is GW*GH bytes (0/1). rasterize() span-paints a region's rows.
 * OP_set is the pixel-wise Boolean op -- the OTHER side of the homomorphism.
 * ===========================================================================*/
enum { GW = 48, GH = 40 };       /* grid width/height (pixels)            */

/* initech-xi7x: the grid is a small LOCAL CROP, not the origin. rgn_rect_t
 * fields are int16 (spec/region_algebra.h Sec 4: "no upper bound beyond
 * int16 and the storage caps" -- 640x480 there is the operator's NATIVE-
 * RESOLUTION convention, not a domain restriction), and window.c DragWindow
 * applies unclamped int16 deltas (MoveWindow: `(int16_t)(s.left + dh)`, no
 * clamp) -- so live desktop regions routinely carry negative bboxes (every
 * top-left drag) and can reach near INT16_MIN/INT16_MAX. A GWxGH bitmap
 * cannot cover the full int16 plane (65536x65536 cells is infeasible), so
 * instead the crop TRANSLATES: g_win_ox/g_win_oy is the ABSOLUTE (int16-
 * domain) top-left the crop currently represents, and every bitmap helper
 * below takes ABSOLUTE coordinates and translates internally. gen_spec()
 * (below) only ever emits coordinates inside the CURRENT window, so nothing
 * of interest falls outside the crop -- this is an honest extension of the
 * oracle (rasterize over a translated window), never a clamp on the input
 * domain. new_window() (below gen_spec) is what moves the crop. */
static int32_t g_win_ox = 0, g_win_oy = 0;

typedef struct bitmap { uint8_t px[GW * GH]; } bitmap_t;

static void bm_clear(bitmap_t *b) { memset(b->px, 0, sizeof b->px); }

static int bm_get(const bitmap_t *b, int x, int y)
{
    int gx = x - g_win_ox, gy = y - g_win_oy;
    if (gx < 0 || gx >= GW || gy < 0 || gy >= GH) return 0;
    return b->px[gy * GW + gx] ? 1 : 0;
}
static void bm_set(bitmap_t *b, int x, int y, int v)
{
    int gx = x - g_win_ox, gy = y - g_win_oy;
    if (gx < 0 || gx >= GW || gy < 0 || gy >= GH) return;
    b->px[gy * GW + gx] = (uint8_t)(v ? 1 : 0);
}

/* Ground-truth rasterizer driven straight off the region rows/x-lists -- this
 * is INDEPENDENT of region_contains_point (so a bug in one cannot mask a bug in
 * the other). For each row r (valid for scanlines [y_top, next.y_top)), paint
 * the half-open spans [x[2k], x[2k+1]) on every covered scanline. */
static void rasterize(const region_t *r, bitmap_t *out)
{
    bm_clear(out);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0; /* closing row */
        if (r->rows[i].x_count == 0) continue;                   /* empty/closing */
        for (int y = y0; y < y1; y++) {
            if (y - g_win_oy < 0 || y - g_win_oy >= GH) continue; /* outside crop */
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2) {
                int xa = r->rows[i].x[k];
                int xb = r->rows[i].x[k + 1];
                for (int x = xa; x < xb; x++) bm_set(out, x, y, 1);
            }
        }
    }
}

/* OP_set on bitmaps -- the powerset-side operators. op matches rgn_op_t. */
static void bm_op(bitmap_t *out, const bitmap_t *A, const bitmap_t *B, rgn_op_t op)
{
    for (int i = 0; i < GW * GH; i++) {
        int a = A->px[i] ? 1 : 0, b = B->px[i] ? 1 : 0, o = 0;
        switch (op) {
            case RGN_OP_UNION:     o = a | b;        break;
            case RGN_OP_INTERSECT: o = a & b;        break;
            case RGN_OP_DIFF:      o = a & (b ^ 1);  break;
            case RGN_OP_XOR:       o = a ^ b;        break;
        }
        out->px[i] = (uint8_t)o;
    }
}

static int bm_equal(const bitmap_t *A, const bitmap_t *B)
{
    return memcmp(A->px, B->px, sizeof A->px) == 0;
}

/* ===========================================================================
 * Region storage: a host arena per region (the engine never mallocs; the
 * CALLER -- here, the test -- supplies rows[] and x_pool, exactly as the kernel
 * will from static/arena memory). One struct bundles a region with its pools.
 * ===========================================================================*/
typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

/* Build a single-scanline region with `nspans` disjoint, NON-adjacent unit
 * spans [off + 4i, off + 4i + 1) at y=0, plus the empty closing row at y=1.
 * The live row carries 2*nspans inversion points (<= RGN_ROW_X_MAX for the row
 * itself to be a legal normal form). Two such regions built at offsets 0 and 2
 * are mutually disjoint AND non-adjacent, so their XOR/UNION is the interleaved
 * union of ALL their spans -- 2*nspans separate spans, i.e. 4*nspans inversion
 * points on the band. With nspans = RGN_ROW_X_MAX/2 each input row is exactly at
 * the per-row cap and the merge output is TWICE the cap -- the bead initech-mswo
 * repro that would overrun region_op's scratch[RGN_ROW_X_MAX]. Normalized (and
 * hence assert-normal-clean) before return. */
static void build_dense_scanline(rgn_store_t *s, int nspans, int off)
{
    store_attach(s);
    int np = 2 * nspans;
    for (int i = 0; i < nspans; i++) {
        s->pool[2 * i]     = (int16_t)(off + 4 * i);
        s->pool[2 * i + 1] = (int16_t)(off + 4 * i + 1);
    }
    s->rows[0].y_top   = 0;
    s->rows[0].x_count = (uint16_t)np;
    s->rows[0].x       = &s->pool[0];
    s->rows[1].y_top   = 1;
    s->rows[1].x_count = 0;
    s->rows[1].x       = &s->pool[np];
    s->r.n_rows        = 2;
    s->r.x_pool_used   = (uint32_t)np;
    s->r.is_empty      = 0;
    s->r.is_rect       = 0;
    region_normalize(&s->r);
}

/* ===========================================================================
 * The WIDE (640px, OD-3 full-domain) deterministic oracle -- initech-44ab
 * RIDER 4. A 640-wide, 2-tall bitmap ground truth, SEPARATE from the small
 * GW x GH windowed crop above: a full-width dense scanline (320 disjoint
 * spans = 640 inversion points) is the exact worst case the 44ab cap raise
 * (RGN_ROW_X_MAX 256 -> 640 == the [0,640] domain bound) exists to represent,
 * and it cannot fit the GW=48 crop. DETERMINISTIC -- a handful of fixed
 * cases, NOT part of the fuzzed loops (committee rider 4). Coordinates are
 * absolute [0,640) x [0,WH); no window translation.
 * ===========================================================================*/
enum { WW = 640, WH = 2 };
typedef struct wbitmap { uint8_t px[WW * WH]; } wbitmap_t;

static void wbm_clear(wbitmap_t *b) { memset(b->px, 0, sizeof b->px); }

/* Wide ground-truth rasterizer -- same row-driven span-paint as rasterize()
 * above (independent of region_contains_point), on the 640-wide grid. */
static void w_rasterize(const region_t *r, wbitmap_t *out)
{
    wbm_clear(out);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;
        if (r->rows[i].x_count == 0) continue;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= WH) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2)
                for (int x = r->rows[i].x[k]; x < r->rows[i].x[k + 1]; x++)
                    if (x >= 0 && x < WW) out->px[y * WW + x] = 1;
        }
    }
}

static void wbm_op(wbitmap_t *out, const wbitmap_t *A, const wbitmap_t *B, rgn_op_t op)
{
    for (int i = 0; i < WW * WH; i++) {
        int a = A->px[i] ? 1 : 0, b = B->px[i] ? 1 : 0, o = 0;
        switch (op) {
            case RGN_OP_UNION:     o = a | b;        break;
            case RGN_OP_INTERSECT: o = a & b;        break;
            case RGN_OP_DIFF:      o = a & (b ^ 1);  break;
            case RGN_OP_XOR:       o = a ^ b;        break;
        }
        out->px[i] = (uint8_t)o;
    }
}

static int wbm_equal(const wbitmap_t *A, const wbitmap_t *B)
{
    return memcmp(A->px, B->px, sizeof A->px) == 0;
}

/* A full-width "comb": 320 disjoint unit spans [2i+off, 2i+off+1) on the band
 * y in [0,2), plus the closing row -- EXACTLY 640 == RGN_ROW_X_MAX inversion
 * points on one row, every coordinate inside the locked [0,640] OD-3 domain
 * (off=0 -> even columns; off=1 -> odd columns; the two are disjoint and
 * their union is the full-width rect). Normalized before return: this IS the
 * rider-4b at-cap boundary case -- a row with exactly RGN_ROW_X_MAX points
 * must round-trip region_normalize without fail-loud. */
static void build_wide_comb(rgn_store_t *s, int off)
{
    store_attach(s);
    for (int i = 0; i < 320; i++) {
        s->pool[2 * i]     = (int16_t)(2 * i + off);
        s->pool[2 * i + 1] = (int16_t)(2 * i + off + 1);
    }
    s->rows[0].y_top   = 0;
    s->rows[0].x_count = 640;
    s->rows[0].x       = &s->pool[0];
    s->rows[1].y_top   = 2;
    s->rows[1].x_count = 0;
    s->rows[1].x       = &s->pool[640];
    s->r.n_rows        = 2;
    s->r.x_pool_used   = 640;
    s->r.is_empty      = 0;
    s->r.is_rect       = 0;
    region_normalize(&s->r);
}

/* ===========================================================================
 * The generators (Rule 11: a seeded LCG so failures are reproducible).
 * ===========================================================================*/
static uint32_t g_seed = 0x1234567u;
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}
static int rnd(int lo, int hi)   /* inclusive [lo,hi] */
{
    if (hi <= lo) return lo;
    return lo + (int)(lcg() % (uint32_t)(hi - lo + 1));
}

/* --- A "spec" for a random region: either a union of rects, OR a raw set of
 *     scanline spans. Both serialize to a region AND to a ground-truth bitmap,
 *     and both are SHRINKABLE (the shrinker bisects the list of items). ---
 * MAX_ITEMS (6) sizes the DEFAULT generator (gen_spec, below) so the primary
 * 4000-case-per-op homomorphism loop stays cheap. STRESS_MAX_ITEMS (64) sizes
 * the array itself so gen_spec_stress() (below new_window()) can push item
 * counts toward RGN_ROWS_CAP (256, spec/region_algebra.h Sec 5) -- MAX_ITEMS=6
 * alone never exercises the engine's multi-row / x_pool-heavy code paths
 * (initech-xi7x). Every item's coordinates are relative to the CURRENT window
 * (g_win_ox/g_win_oy, set by new_window()) -- item generation is factored into
 * gen_item() so both generators share one windowed coordinate rule. */
enum { MAX_ITEMS = 6, STRESS_MAX_ITEMS = 64 };
typedef struct rgn_spec {
    int  is_raw;                 /* 0 = rects, 1 = raw scanline spans          */
    int  n;                      /* item count                                 */
    /* rect items: [top,left,bottom,right]; raw items: [y, x0, x1, _] (one span
     * on scanline y, columns [x0,x1)). A raw spec is a *bag* of single spans;
     * overlapping/adjacent spans on the same row exercise the normalizer. */
    int  it[STRESS_MAX_ITEMS][4];
} rgn_spec_t;

/* Fill item `i` of `s` with a coordinate ABSOLUTE to the current window
 * (g_win_ox/g_win_oy) but window-LOCAL in range -- so every generated
 * rgn_rect_t field is a valid, in-window int16 regardless of where
 * new_window() last placed the crop (near 0, straddling 0, or near an
 * INT16_MIN/INT16_MAX edge; see new_window()). */
static void gen_item(rgn_spec_t *s, int i)
{
    if (!s->is_raw) {
        int t = g_win_oy + rnd(0, GH - 1), l = g_win_ox + rnd(0, GW - 1);
        int b = g_win_oy + rnd(0, GH),     r = g_win_ox + rnd(0, GW);
        s->it[i][0] = t; s->it[i][1] = l; s->it[i][2] = b; s->it[i][3] = r;
    } else {
        int y  = g_win_oy + rnd(0, GH - 1);
        int x0 = g_win_ox + rnd(0, GW - 1);
        int x1 = g_win_ox + rnd(0, GW);
        s->it[i][0] = y; s->it[i][1] = x0; s->it[i][2] = x1; s->it[i][3] = 0;
    }
}

static void gen_spec(rgn_spec_t *s)
{
    s->is_raw = rnd(0, 1);
    s->n = rnd(1, MAX_ITEMS);
    for (int i = 0; i < s->n; i++) gen_item(s, i);
}

/* High-n stress generator (initech-xi7x item 4): item counts pushed toward
 * RGN_ROWS_CAP so the multi-row/x_pool merge paths get exercised at scale,
 * not just the MAX_ITEMS<=6 default. Same windowed coordinate rule. 64 raw
 * 1-row items union to at most ~2*64=128 live rows (well under the 256 cap;
 * each union step is A-union-A idempotent-copy so rows track the ACCUMULATED
 * disjoint-span count, not a doubling recursion -- see spec_to_region). */
static void gen_spec_stress(rgn_spec_t *s)
{
    s->is_raw = rnd(0, 1);
    s->n = rnd(20, STRESS_MAX_ITEMS);
    for (int i = 0; i < s->n; i++) gen_item(s, i);
}

/* Move the crop (initech-xi7x): pick a new ABSOLUTE window origin, seeded off
 * the SAME LCG as everything else (Rule 11 -- a failing case stays
 * reproducible from g_seed alone). Four bands, each landing every generated
 * coordinate on a valid int16 (checked below):
 *   0: legacy small-nonneg [0,GW)x[0,GH) -- the ORIGINAL generator's entire
 *      domain pre-initech-xi7x; kept live for regression continuity.
 *   1: straddling zero (mixed negative/positive) -- e.g. a window dragged
 *      just past the desktop origin; the task's suggested [-48,48) shape.
 *   2: near INT16_MIN -- window low edge can touch INT16_MIN exactly
 *      (oy + 0 == INT16_MIN) without underflow.
 *   3: near INT16_MAX -- symmetric high edge, window high edge can touch
 *      INT16_MAX exactly (oy + GH == INT16_MAX) without overflow.
 * Band 2/3 bounds are picked so g_win_o{x,y} + [0,GW]/[0,GH] never leaves
 * [INT16_MIN, INT16_MAX] -- gen_item()'s widest item field is o + rnd(0,GW)
 * (rect right/bottom), so the arithmetic below is exact, not a margin guess. */
static void new_window(void)
{
    switch (rnd(0, 3)) {
        case 0:
            g_win_ox = 0;
            g_win_oy = 0;
            break;
        case 1:
            g_win_ox = rnd(-(GW / 2), GW / 2);
            g_win_oy = rnd(-(GH / 2), GH / 2);
            break;
        case 2:
            g_win_ox = rnd(INT16_MIN, INT16_MIN + 8);
            g_win_oy = rnd(INT16_MIN, INT16_MIN + 8);
            break;
        case 3:
            g_win_ox = rnd(INT16_MAX - GW - 8, INT16_MAX - GW);
            g_win_oy = rnd(INT16_MAX - GH - 8, INT16_MAX - GH);
            break;
    }
}

/* Build the region described by `s` INTO store `out` (already attached). The
 * raw path constructs each single span as a 1px-tall rect and unions them, so
 * it shares the engine's union path but FEEDS arbitrary overlapping spans (not
 * tidy disjoint rects) -- that is what surfaces non-rectangular bugs. */
static void spec_to_region(const rgn_spec_t *s, rgn_store_t *out)
{
    store_attach(out);
    rgn_store_t acc, tmp, one;
    store_attach(&acc);
    region_set_empty(&acc.r);
    for (int i = 0; i < s->n; i++) {
        rgn_rect_t rc;
        if (!s->is_raw) {
            rc.top = (int16_t)s->it[i][0]; rc.left  = (int16_t)s->it[i][1];
            rc.bottom = (int16_t)s->it[i][2]; rc.right = (int16_t)s->it[i][3];
        } else {
            rc.top = (int16_t)s->it[i][0];      rc.left  = (int16_t)s->it[i][1];
            rc.bottom = (int16_t)(s->it[i][0] + 1); rc.right = (int16_t)s->it[i][2];
        }
        store_attach(&one);
        region_set_rect(&one.r, rc);
        store_attach(&tmp);
        region_op(&tmp.r, &acc.r, &one.r, RGN_OP_UNION);
        /* copy tmp -> acc (rebind pools) */
        store_attach(&acc);
        region_op(&acc.r, &tmp.r, &tmp.r, RGN_OP_UNION); /* idempotent copy via self-union */
    }
    region_op(&out->r, &acc.r, &acc.r, RGN_OP_UNION);    /* final copy */
}

/* Ground-truth bitmap of a spec, painted DIRECTLY from the spec (NOT via the
 * region) -- the independent oracle of what the region SHOULD be. */
static void spec_to_bitmap(const rgn_spec_t *s, bitmap_t *out)
{
    bm_clear(out);
    for (int i = 0; i < s->n; i++) {
        int t, l, b, r;
        if (!s->is_raw) {
            t = s->it[i][0]; l = s->it[i][1]; b = s->it[i][2]; r = s->it[i][3];
        } else {
            t = s->it[i][0]; l = s->it[i][1]; b = s->it[i][0] + 1; r = s->it[i][2];
        }
        for (int y = t; y < b; y++)
            for (int x = l; x < r; x++) bm_set(out, x, y, 1);
    }
}

/* ===========================================================================
 * The shrinker -- on a failing (specA, specB, op), bisect the item lists and
 * clamp coords toward a MINIMAL counterexample, then print it. The failing
 * predicate is the homomorphism check itself.
 * ===========================================================================*/
typedef int (*fails_fn)(const rgn_spec_t *A, const rgn_spec_t *B, rgn_op_t op);

/* Does the homomorphism FAIL for this (A,B,op)? (1 = fails = counterexample) */
static int hom_fails(const rgn_spec_t *sA, const rgn_spec_t *sB, rgn_op_t op)
{
    static rgn_store_t SA, SB, SO;       /* static: keep the test stack small  */
    static bitmap_t bA, bB, bregion, bset;
    spec_to_region(sA, &SA);
    spec_to_region(sB, &SB);
    store_attach(&SO);
    region_op(&SO.r, &SA.r, &SB.r, op);
    rasterize(&SO.r, &bregion);          /* LHS: rasterize(A OP B)             */
    rasterize(&SA.r, &bA);
    rasterize(&SB.r, &bB);
    bm_op(&bset, &bA, &bB, op);          /* RHS: rasterize(A) OP_set raster(B) */
    return !bm_equal(&bregion, &bset);
}

static void print_spec(const char *name, const rgn_spec_t *s)
{
    fprintf(stderr, "    %s = {%s n=%d", name, s->is_raw ? "raw" : "rects", s->n);
    for (int i = 0; i < s->n; i++)
        fprintf(stderr, " [%d,%d,%d,%d]", s->it[i][0], s->it[i][1],
                s->it[i][2], s->it[i][3]);
    fprintf(stderr, "}\n");
}

/* Try to drop items / clamp coords while the predicate still fails. */
static void shrink(rgn_spec_t *A, rgn_spec_t *B, rgn_op_t op, fails_fn fails)
{
    int progress = 1;
    while (progress) {
        progress = 0;
        /* 1. drop an item from A or B if it still fails. */
        rgn_spec_t *which[2] = { A, B };
        for (int w = 0; w < 2; w++) {
            rgn_spec_t *S = which[w];
            for (int i = 0; i < S->n && S->n > 1; i++) {
                rgn_spec_t cand = *S;
                for (int j = i; j + 1 < cand.n; j++)
                    memcpy(cand.it[j], cand.it[j + 1], sizeof cand.it[j]);
                cand.n--;
                rgn_spec_t a2 = *A, b2 = *B;
                if (w == 0) a2 = cand; else b2 = cand;
                if (fails(&a2, &b2, op)) { *S = cand; progress = 1; i--; }
            }
        }
        /* 2. clamp each coord toward 0 while it still fails. BIDIRECTIONAL
         *    (initech-xi7x): coordinates can now be negative (or near
         *    INT16_MIN), and the original decrement-only loop could never
         *    shrink those -- it would leave a deeply negative counterexample
         *    coordinate untouched. Step toward 0 from whichever side. */
        for (int w = 0; w < 2; w++) {
            rgn_spec_t *S = which[w];
            for (int i = 0; i < S->n; i++)
                for (int c = 0; c < 4; c++) {
                    int dir = (S->it[i][c] > 0) ? -1 : (S->it[i][c] < 0) ? 1 : 0;
                    while (dir != 0 && S->it[i][c] != 0) {
                        rgn_spec_t cand = *S;
                        cand.it[i][c] += dir;
                        rgn_spec_t a2 = *A, b2 = *B;
                        if (w == 0) a2 = cand; else b2 = cand;
                        if (fails(&a2, &b2, op)) { *S = cand; progress = 1; }
                        else break;
                    }
                }
        }
    }
}

/* ===========================================================================
 * Normal-form invariant checker (independent of region_assert_normal so the
 * test cannot be fooled by a buggy assert). Checks all 5 invariants directly.
 * ===========================================================================*/
static int normal_form_holds(const region_t *r)
{
    if (r->is_empty) return r->n_rows == 0;
    if (r->n_rows < 2) return 0;                 /* >=1 live row + closing row */
    if (r->rows[0].x_count == 0) return 0;        /* (4) no LEADING empty row   */
    if (r->rows[r->n_rows - 1].x_count != 0) return 0; /* (4) closing row IS empty */
    for (uint16_t i = 0; i < r->n_rows; i++) {
        const rgn_row_t *row = &r->rows[i];
        if (row->x_count & 1u) return 0;          /* (2) EVEN length           */
        for (uint16_t k = 1; k < row->x_count; k++)
            if (row->x[k] <= row->x[k - 1]) return 0; /* (1) STRICTLY INCREASING */
        if (i + 1 < r->n_rows && row->y_top >= r->rows[i + 1].y_top) return 0; /* (5) */
    }
    /* (3)+(4) VERTICAL-RLE: no two consecutive rows share an identical x-list
     * (forbids the redundant empty-under-empty; a single empty interior row
     * between two DIFFERENT non-empty rows is a legal vertical-gap closer). */
    for (uint16_t i = 0; i + 1 < r->n_rows; i++) {
        const rgn_row_t *a = &r->rows[i], *b = &r->rows[i + 1];
        if (a->x_count == b->x_count) {
            int same = 1;
            for (uint16_t k = 0; k < a->x_count; k++)
                if (a->x[k] != b->x[k]) { same = 0; break; }
            if (same) return 0;
        }
    }
    return 1;
}

/* ===========================================================================
 * MAIN -- the suite.
 * ===========================================================================*/
int main(void)
{
    const rgn_op_t OPS[4] = { RGN_OP_UNION, RGN_OP_INTERSECT,
                              RGN_OP_DIFF, RGN_OP_XOR };
    const char *OPNAME[4] = { "UNION", "INTERSECT", "DIFF", "XOR" };

    /* ---- spec truth-table sanity (the locked header says these bits) ------ */
    CHECK(rgn_op_truth(RGN_OP_UNION)     == 0x0Eu, "spec: UNION truth 1110b");
    CHECK(rgn_op_truth(RGN_OP_INTERSECT) == 0x08u, "spec: INTERSECT truth 1000b");
    CHECK(rgn_op_truth(RGN_OP_DIFF)      == 0x04u, "spec: DIFF truth 0100b");
    CHECK(rgn_op_truth(RGN_OP_XOR)       == 0x06u, "spec: XOR truth 0110b");

    /* ======================================================================
     * OVER-CAP FAIL-LOUD PROBE  (bead initech-mswo; Rule 2, Rule 6)
     * ----------------------------------------------------------------------
     * region_op's per-band xmerge writes into a stack scratch[RGN_ROW_X_MAX].
     * Each input row is normalized to at most RGN_ROW_X_MAX inversion points,
     * so an XOR of two maximally-dense rows can emit up to 2*RGN_ROW_X_MAX
     * points -- TWICE the scratch. Pre-fix xmerge had NO bound on its write
     * count and overran the scratch (silent stack corruption). The fix makes
     * xmerge fail loud (RGN_FAIL_LOUD -> abort() hosted) when the output would
     * exceed cap, BEFORE the (cap+1)-th write.
     *
     * We cannot call region_op on the over-cap inputs directly here: fail-loud
     * is abort(), which would kill the whole suite. So fork a child, run the
     * over-cap XOR there, and observe the child from the parent:
     *   - child killed by SIGABRT  == the bound check fired (fail loud): GOOD.
     *   - child exited 0           == xmerge RETURNED without failing loud, i.e.
     *                                 it overran the scratch and merely got lucky
     *                                 not to crash: the BUG is present.
     *   - any other termination    == corruption detected by another guard
     *                                 (e.g. ASAN's stack-buffer-overflow exit,
     *                                 or a SIGSEGV): NOT a clean fail-loud.
     * Only a clean SIGABRT counts as PASS. Built under -fsanitize=address (the
     * RGN_MUTATE_NO_XMERGE_CAP mutant, Makefile test-region-mutant) the reverted
     * bound lets the OOB write reach ASAN FIRST, which exits(1) rather than
     * SIGABRT -- so this probe goes RED exactly when the bound is removed
     * (mutation-proven, Rule 6). Ref: PRD Sec 6.2; spec/region_algebra.h Sec 5.
     * ====================================================================== */
    {
        static rgn_store_t A, B, O;
        /* cap/2 spans -> exactly-at-cap rows (320 spans -> 640 pts at the
         * post-44ab cap of 640); their XOR wants 2*cap pts -> over-cap. */
        const int NSPANS = (int)(RGN_ROW_X_MAX / 2);
        build_dense_scanline(&A, NSPANS, 0);
        build_dense_scanline(&B, NSPANS, 2);
        CHECK(A.r.rows[0].x_count == RGN_ROW_X_MAX,
              "over-cap probe: dense A row is exactly the per-row cap");
        CHECK(B.r.rows[0].x_count == RGN_ROW_X_MAX,
              "over-cap probe: dense B row is exactly the per-row cap");
        CHECK(normal_form_holds(&A.r) && normal_form_holds(&B.r),
              "over-cap probe: dense inputs are in normal form");

        fflush(stdout); fflush(stderr);     /* don't double-flush in the child */
        pid_t pid = fork();
        if (pid == 0) {
            /* child: no core dump on the abort; run the over-cap XOR. If
             * region_op RETURNS, the write was unbounded (the pre-fix bug) --
             * exit 0 so the parent records the miss. */
            struct rlimit rl = { 0, 0 };
            (void)setrlimit(RLIMIT_CORE, &rl);
            store_attach(&O);
            region_op(&O.r, &A.r, &B.r, RGN_OP_XOR);
            _exit(0);
        }
        CHECK(pid > 0, "over-cap probe: fork() succeeded");
        int status = 0;
        (void)waitpid(pid, &status, 0);
        int fail_loud = (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
        CHECK(fail_loud,
              "over-cap band xmerge FAILS LOUD (SIGABRT) instead of overrunning "
              "scratch[RGN_ROW_X_MAX] (bead initech-mswo)");
    }

    /* ======================================================================
     * T2 (initech-44ab follow-up, committee 2026-07-11): region_from_rects'
     * n==0 / NULL-rects early return happens BEFORE the static working-set
     * guard is acquired -- call each TWICE in a row (a leaked guard would
     * false-panic the second call), then prove real calls + all three query
     * helpers still cycle the guard cleanly.
     * ====================================================================== */
    {
        static rgn_store_t S;
        rgn_rect_t rc0 = { 0, 0, 4, 4 };            /* top,left,bottom,right */
        store_attach(&S);
        region_from_rects(&S.r, (const rgn_rect_t *)0, 3);
        CHECK(region_is_empty(&S.r), "from_rects(NULL rects) -> empty (1st call)");
        region_from_rects(&S.r, (const rgn_rect_t *)0, 3);
        CHECK(region_is_empty(&S.r), "from_rects(NULL rects) -> empty (2nd call; no guard leak)");
        region_from_rects(&S.r, &rc0, 0);
        CHECK(region_is_empty(&S.r), "from_rects(n==0) -> empty (1st call)");
        region_from_rects(&S.r, &rc0, 0);
        CHECK(region_is_empty(&S.r), "from_rects(n==0) -> empty (2nd call; no guard leak)");
        /* real work right after the empty-arg calls: the guard must be free,
         * and each guarded helper must release it for the next one. */
        region_from_rects(&S.r, &rc0, 1);
        CHECK(!region_is_empty(&S.r) && S.r.is_rect,
              "from_rects does real work after the empty-arg calls (guard free)");
        region_from_rects(&S.r, &rc0, 1);
        CHECK(region_rect_fully_in(&S.r, rc0) == 1,
              "rect_fully_in right after from_rects (guard cycled)");
        CHECK(region_rect_overlaps(&S.r, rc0) == 1,
              "rect_overlaps right after rect_fully_in (guard cycled)");
        CHECK(region_intersects(&S.r, &S.r) == 1,
              "region_intersects right after rect_overlaps (guard cycled)");
    }

    /* ======================================================================
     * THE IN-USE GUARD BITES (Rule 6; initech-44ab follow-up): a NESTED
     * working-set acquire must FAIL LOUD. Fork a child -- the parent is NOT
     * inside any guarded call at fork time (committee verification round) --
     * and have it run the hosted-only probe hook: acquire the guard, then
     * call region_from_rects, whose nested acquire must abort(). Only a clean
     * SIGABRT counts; a child that EXITS 0 means the probe RETURNED, i.e. the
     * guard is decoration.
     * ====================================================================== */
    {
        static rgn_store_t S;
        fflush(stdout); fflush(stderr);
        pid_t pid = fork();
        if (pid == 0) {
            struct rlimit rl = { 0, 0 };
            (void)setrlimit(RLIMIT_CORE, &rl);
            store_attach(&S);
            region_engine_reset();                  /* known-clear guard */
            region_engine_probe_nested_acquire(&S.r);
            _exit(0);                               /* returned == guard dead */
        }
        CHECK(pid > 0, "guard probe: fork() succeeded");
        int status = 0;
        (void)waitpid(pid, &status, 0);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT,
              "nested working-set acquire FAILS LOUD (SIGABRT) -- the in-use "
              "guard bites (initech-44ab)");
    }

    /* ======================================================================
     * WIDE 640px DETERMINISTIC LEG (initech-44ab RIDER 4) -- the full OD-3
     * domain, pixel-exact against the independent 640-wide bitmap oracle.
     *  (a) at-cap boundary: a row with EXACTLY RGN_ROW_X_MAX == 640 inversion
     *      points (320 disjoint unit spans across [0,640)) round-trips
     *      region_normalize AND region_op without fail-loud;
     *  (b) full-width homomorphism: all 4 ops over the dense comb pair (A =
     *      even columns, B = odd columns) and over (full-rect, A), graded
     *      PIXEL-EXACT on the 640-wide grid;
     *  (c) the collapse case: A UNION B == the full-width rect -- 1280 input
     *      points collapse to 2 output points (coincident-edge emit logic);
     *  (d) the at-cap OUTPUT case: F DIFF A == B -- a merge whose output row
     *      is exactly at the 640 cap.
     * The cap+2 over-cap side stays covered by the fail-loud probe above.
     * Deterministic; NOT part of the fuzzed loops (committee rider 4).
     * ====================================================================== */
    {
        static rgn_store_t A, B, F, O;
        static wbitmap_t wa, wb, wf, wexp, wgot, ta, tb;

        build_wide_comb(&A, 0);                     /* even columns */
        CHECK(A.r.n_rows == 2 && A.r.rows[0].x_count == RGN_ROW_X_MAX,
              "wide comb A: exactly RGN_ROW_X_MAX points round-trips "
              "region_normalize (at-cap boundary, rider 4b)");
        CHECK(normal_form_holds(&A.r), "wide comb A is in normal form");
        build_wide_comb(&B, 1);                     /* odd columns  */
        CHECK(B.r.rows[0].x_count == RGN_ROW_X_MAX,
              "wide comb B: exactly RGN_ROW_X_MAX points round-trips normalize");
        store_attach(&F);
        rgn_rect_t frect = { 0, 0, (int16_t)WH, (int16_t)WW };
        region_set_rect(&F.r, frect);               /* the full-width rect */

        /* Independent pixel ground truth painted from the GENERATING FORMULA
         * (not via the regions): even / odd columns on rows [0,2). */
        wbm_clear(&ta); wbm_clear(&tb);
        for (int y = 0; y < WH; y++)
            for (int i = 0; i < 320; i++) {
                ta.px[y * WW + 2 * i]     = 1;
                tb.px[y * WW + 2 * i + 1] = 1;
            }
        w_rasterize(&A.r, &wa);
        w_rasterize(&B.r, &wb);
        w_rasterize(&F.r, &wf);
        CHECK(wbm_equal(&wa, &ta), "wide comb A rasterizes to the even columns");
        CHECK(wbm_equal(&wb, &tb), "wide comb B rasterizes to the odd columns");

        /* (b) homomorphism, all 4 ops, pixel-exact, on (A,B) and (F,A). */
        int hom_bad = 0, nf_bad = 0;
        for (int op_i = 0; op_i < 4; op_i++) {
            store_attach(&O);
            region_op(&O.r, &A.r, &B.r, OPS[op_i]);
            if (!normal_form_holds(&O.r)) nf_bad = 1;
            w_rasterize(&O.r, &wgot);
            wbm_op(&wexp, &wa, &wb, OPS[op_i]);
            if (!wbm_equal(&wgot, &wexp)) hom_bad = 1;

            store_attach(&O);
            region_op(&O.r, &F.r, &A.r, OPS[op_i]);
            if (!normal_form_holds(&O.r)) nf_bad = 1;
            w_rasterize(&O.r, &wgot);
            wbm_op(&wexp, &wf, &wa, OPS[op_i]);
            if (!wbm_equal(&wgot, &wexp)) hom_bad = 1;
        }
        CHECK(!hom_bad, "wide 640px homomorphism: all 4 ops on (A,B) and "
                        "(full,A), pixel-exact (rider 4a)");
        CHECK(!nf_bad, "wide 640px op outputs are all in normal form");

        /* (a) at-cap round-trip through region_op: A self-union == A. */
        store_attach(&O);
        region_op(&O.r, &A.r, &A.r, RGN_OP_UNION);
        CHECK(region_equal(&O.r, &A.r) &&
              O.r.rows[0].x_count == RGN_ROW_X_MAX,
              "at-cap row (640 pts) round-trips region_op+normalize "
              "(self-union identity; the 44ab acceptance case)");

        /* (c) the collapse case: A UNION B == the full-width rect (1280
         * candidate emit positions collapse to 2 -- coincident-edge logic). */
        store_attach(&O);
        region_op(&O.r, &A.r, &B.r, RGN_OP_UNION);
        CHECK(region_equal(&O.r, &F.r) && O.r.is_rect,
              "full-width union: 320+320 interleaved unit spans collapse to "
              "the single [0,640) span (2 points)");

        /* (d) the at-cap OUTPUT case: F DIFF A == B (merge output row is
         * exactly at the 640 cap -- the largest legal xmerge emission). */
        store_attach(&O);
        region_op(&O.r, &F.r, &A.r, RGN_OP_DIFF);
        CHECK(region_equal(&O.r, &B.r) &&
              O.r.rows[0].x_count == RGN_ROW_X_MAX,
              "full DIFF evens == odds: merge emits an exactly-at-cap row "
              "(640 pts) without fail-loud");
    }

    /* ---- constructor smoke: empty + single rect rasterize correctly ------- */
    {
        static rgn_store_t S; bitmap_t bm, expect;
        store_attach(&S);
        region_set_empty(&S.r);
        CHECK(region_is_empty(&S.r), "set_empty -> is_empty");
        CHECK(normal_form_holds(&S.r), "empty is normal-form");
        rasterize(&S.r, &bm); bm_clear(&expect);
        CHECK(bm_equal(&bm, &expect), "empty rasterizes to no pixels");

        store_attach(&S);
        rgn_rect_t rc = { 5, 7, 20, 30 };   /* top,left,bottom,right          */
        region_set_rect(&S.r, rc);
        CHECK(S.r.is_rect == 1, "set_rect -> is_rect fast-path flag");
        CHECK(normal_form_holds(&S.r), "set_rect is normal-form");
        rasterize(&S.r, &bm);
        bm_clear(&expect);
        for (int y = 5; y < 20; y++) for (int x = 7; x < 30; x++) bm_set(&expect, x, y, 1);
        CHECK(bm_equal(&bm, &expect), "rect rasterizes to its [l,r)x[t,b) pixels");

        /* empty rect (right<=left) yields the empty region */
        store_attach(&S);
        rgn_rect_t er = { 5, 30, 20, 7 };
        region_set_rect(&S.r, er);
        CHECK(region_is_empty(&S.r), "degenerate rect (r<=l) -> empty region");

        /* initech-xi7x smoke: a rect with an ENTIRELY NEGATIVE bbox rasterizes
         * correctly -- direct check that the oracle's grid/bitmap does not
         * silently assume an origin-(0,0) region (the gap this issue closes).
         * The window is moved to cover [-30,18)x[-30,10) so the rect's pixels
         * fall inside the crop. */
        g_win_ox = -30; g_win_oy = -30;
        store_attach(&S);
        rgn_rect_t nr = { -20, -25, -5, -3 };   /* top,left,bottom,right, all < 0 */
        region_set_rect(&S.r, nr);
        CHECK(normal_form_holds(&S.r), "negative-bbox rect is normal-form");
        rasterize(&S.r, &bm);
        bm_clear(&expect);
        for (int y = -20; y < -5; y++) for (int x = -25; x < -3; x++) bm_set(&expect, x, y, 1);
        CHECK(bm_equal(&bm, &expect),
              "negative-bbox rect rasterizes to its [l,r)x[t,b) pixels (grid is NOT origin-locked)");
        CHECK(region_contains_point(&S.r, (int16_t)-10, (int16_t)-10) != 0,
              "region_contains_point true inside a negative-bbox rect");
        CHECK(region_contains_point(&S.r, (int16_t)10, (int16_t)10) == 0,
              "region_contains_point false outside a negative-bbox rect");
        g_win_ox = 0; g_win_oy = 0;   /* restore default window for what follows */
    }

    /* ---- construction fidelity: a region built from a spec rasterizes EXACTLY
     *      to the spec's directly-painted bitmap (independent oracle: the spec
     *      bitmap is painted from the raw items, NOT via the region) ---------- */
    {
        static rgn_store_t S; bitmap_t fromrgn, fromspec;
        rgn_spec_t s;
        int mism = 0;
        for (int t = 0; t < 500 && !mism; t++) {
            new_window();
            gen_spec(&s);
            spec_to_region(&s, &S);
            rasterize(&S.r, &fromrgn);
            spec_to_bitmap(&s, &fromspec);
            if (!bm_equal(&fromrgn, &fromspec)) mism = 1;
        }
        CHECK(!mism, "region built from spec rasterizes to the spec's pixels");
    }

    /* ---- region_contains_point agrees with rasterize (parity test) -------- */
    {
        static rgn_store_t S; bitmap_t bm;
        rgn_spec_t s;
        int mism = 0;
        for (int t = 0; t < 200 && !mism; t++) {
            new_window();
            gen_spec(&s);
            spec_to_region(&s, &S);
            rasterize(&S.r, &bm);
            for (int y = g_win_oy; y < g_win_oy + GH && !mism; y++)
                for (int x = g_win_ox; x < g_win_ox + GW; x++) {
                    int want = bm_get(&bm, x, y);
                    int got  = region_contains_point(&S.r, (int16_t)x, (int16_t)y) ? 1 : 0;
                    if (want != got) { mism = 1; break; }
                }
        }
        CHECK(!mism, "region_contains_point matches rasterize on every pixel");
    }

    /* ---- region_intersects + region_rect_fully_in + region_rect_overlaps vs
     *      the pixel truth (AM-4: CONTAINMENT and OVERLAP are distinct) ------- */
    {
        int inter_bad = 0, rir_bad = 0, ovl_bad = 0;
        for (int t = 0; t < 800 && !inter_bad && !rir_bad && !ovl_bad; t++) {
            new_window();
            rgn_spec_t sA, sB; gen_spec(&sA); gen_spec(&sB);
            static rgn_store_t SA, SB; bitmap_t bA, bB;
            spec_to_region(&sA, &SA); spec_to_region(&sB, &SB);
            rasterize(&SA.r, &bA); rasterize(&SB.r, &bB);
            /* ground truth: do A and B share any pixel? */
            int share = 0;
            for (int i = 0; i < GW * GH && !share; i++) if (bA.px[i] && bB.px[i]) share = 1;
            if (region_intersects(&SA.r, &SB.r) != share) inter_bad = 1;

            /* region_rect_fully_in (CONTAINMENT): a random rect is fully inside A
             * iff EVERY one of its pixels is set in A's bitmap. Generated in the
             * SAME window as A/B (grid-local, then translated) so it lands where
             * A/B's pixels actually are, including negative/near-INT16 windows. */
            int topL = rnd(0, GH - 1), leftL = rnd(0, GW - 1);
            int botL = rnd(topL + 1, GH), rightL = rnd(leftL + 1, GW);
            int top = g_win_oy + topL, left = g_win_ox + leftL;
            int bot = g_win_oy + botL, right = g_win_ox + rightL;
            int all_in = 1;
            for (int y = top; y < bot && all_in; y++)
                for (int x = left; x < right; x++)
                    if (!bm_get(&bA, x, y)) { all_in = 0; break; }
            rgn_rect_t rc = { (int16_t)top, (int16_t)left, (int16_t)bot, (int16_t)right };
            if (region_rect_fully_in(&SA.r, rc) != all_in) rir_bad = 1;

            /* region_rect_overlaps (OVERLAP, AM-4): the SAME random rect overlaps
             * A iff ANY one of its pixels is set in A's bitmap. Independent pixel
             * ground truth -- this is a DIFFERENT predicate than containment. */
            int any_in = 0;
            for (int y = top; y < bot && !any_in; y++)
                for (int x = left; x < right; x++)
                    if (bm_get(&bA, x, y)) { any_in = 1; break; }
            if (region_rect_overlaps(&SA.r, rc) != any_in) ovl_bad = 1;
        }
        CHECK(!inter_bad, "region_intersects matches pixel-overlap ground truth");
        CHECK(!rir_bad,   "region_rect_fully_in matches pixel-containment truth");
        CHECK(!ovl_bad,   "region_rect_overlaps matches pixel-any-overlap truth");
    }

    /* ======================================================================
     * PRIMARY: the homomorphism, all 4 ops, thousands of random pairs (both
     * rect-union AND raw scanline-span generators). On the FIRST failure,
     * shrink + print, then CHECK(0).
     * ====================================================================== */
    {
        enum { CASES = 4000 };
        int failed = 0;
        for (int op_i = 0; op_i < 4 && !failed; op_i++) {
            rgn_op_t op = OPS[op_i];
            for (int t = 0; t < CASES; t++) {
                new_window();
                rgn_spec_t sA, sB;
                gen_spec(&sA);
                gen_spec(&sB);
                if (hom_fails(&sA, &sB, op)) {
                    fprintf(stderr, "  HOMOMORPHISM FAIL op=%s; shrinking...\n",
                            OPNAME[op_i]);
                    shrink(&sA, &sB, op, hom_fails);
                    print_spec("A", &sA);
                    print_spec("B", &sB);
                    CHECK(0, "homomorphism: rasterize(A OP B) == raster(A) OP raster(B)");
                    failed = 1;
                    break;
                }
            }
            if (!failed)
                CHECK(1, "homomorphism holds for op (4000 random pairs)");
        }
    }

    /* ======================================================================
     * HIGH-N STRESS (initech-xi7x item 4): same homomorphism oracle, but with
     * gen_spec_stress() (20..64 items) instead of gen_spec() (1..6 items) --
     * MAX_ITEMS=6 alone never pushes a region past a handful of rows, so the
     * multi-row / x_pool-heavy merge paths (well below RGN_ROWS_CAP /
     * RGN_X_POOL_CAP, spec/region_algebra.h Sec 5) go untested at scale.
     * Iteration count kept modest (150/op) so this stays cheap.
     * ====================================================================== */
    {
        enum { STRESS_CASES = 150 };
        int failed = 0;
        for (int op_i = 0; op_i < 4 && !failed; op_i++) {
            rgn_op_t op = OPS[op_i];
            for (int t = 0; t < STRESS_CASES; t++) {
                new_window();
                rgn_spec_t sA, sB;
                gen_spec_stress(&sA);
                gen_spec_stress(&sB);
                if (hom_fails(&sA, &sB, op)) {
                    fprintf(stderr, "  HIGH-N STRESS HOMOMORPHISM FAIL op=%s; shrinking...\n",
                            OPNAME[op_i]);
                    shrink(&sA, &sB, op, hom_fails);
                    print_spec("A", &sA);
                    print_spec("B", &sB);
                    CHECK(0, "high-n stress homomorphism: rasterize(A OP B) == raster(A) OP raster(B)");
                    failed = 1;
                    break;
                }
            }
            if (!failed)
                CHECK(1, "high-n stress homomorphism holds for op (150 high-n random pairs)");
        }
    }

    /* ======================================================================
     * The region produced by EVERY op is in normal form, and its bbox is the
     * tight bounding box of its pixels; region_assert_normal agrees.
     * ====================================================================== */
    {
        int nf_bad = 0, bbox_bad = 0, assert_bad = 0;
        for (int t = 0; t < 1500 && !nf_bad && !bbox_bad; t++) {
            new_window();
            rgn_spec_t sA, sB; gen_spec(&sA); gen_spec(&sB);
            static rgn_store_t SA, SB, SO; bitmap_t bm;
            spec_to_region(&sA, &SA); spec_to_region(&sB, &SB);
            for (int op_i = 0; op_i < 4; op_i++) {
                store_attach(&SO);
                region_op(&SO.r, &SA.r, &SB.r, OPS[op_i]);
                if (!normal_form_holds(&SO.r)) { nf_bad = 1; break; }
                if (!region_assert_normal(&SO.r)) { assert_bad = 1; break; }
                rasterize(&SO.r, &bm);
                /* tight bbox of bm, in ABSOLUTE (window-translated) coords; wide
                 * sentinels (not GW/GH/-1) since the window may sit near
                 * INT16_MIN/INT16_MAX. */
                int minx = 1 << 30, miny = 1 << 30, maxx = -(1 << 30), maxy = -(1 << 30), any = 0;
                for (int y = g_win_oy; y < g_win_oy + GH; y++)
                    for (int x = g_win_ox; x < g_win_ox + GW; x++)
                    if (bm_get(&bm, x, y)) {
                        any = 1;
                        if (x < minx) minx = x;
                        if (x + 1 > maxx) maxx = x + 1;
                        if (y < miny) miny = y;
                        if (y + 1 > maxy) maxy = y + 1;
                    }
                rgn_rect_t bb = region_get_bbox(&SO.r);
                if (!any) {
                    if (!region_is_empty(&SO.r)) { bbox_bad = 1; break; }
                } else if (bb.left != minx || bb.top != miny ||
                           bb.right != maxx || bb.bottom != maxy) {
                    bbox_bad = 1; break;
                }
            }
        }
        CHECK(!nf_bad, "every op output is in normal form (5 invariants)");
        CHECK(!assert_bad, "region_assert_normal agrees on every op output");
        CHECK(!bbox_bad, "every op output bbox is the tight pixel bounding box");
    }

    /* ======================================================================
     * normalize-idempotence (BIT-EXACT): normalize(normalize(R)) == norm(R).
     * We normalize an op output, snapshot bytes, normalize again, compare.
     * ====================================================================== */
    {
        int bad = 0;
        for (int t = 0; t < 1500 && !bad; t++) {
            new_window();
            rgn_spec_t sA, sB; gen_spec(&sA); gen_spec(&sB);
            static rgn_store_t SA, SB, SO; spec_to_region(&sA, &SA); spec_to_region(&sB, &SB);
            store_attach(&SO);
            region_op(&SO.r, &SA.r, &SB.r, OPS[t & 3]);
            /* snapshot (rows + x_pool used) */
            rgn_row_t  rows0[RGN_ROWS_CAP];
            int16_t    pool0[RGN_X_POOL_CAP];
            uint16_t   n0 = SO.r.n_rows; uint32_t xu0 = SO.r.x_pool_used;
            rgn_rect_t bb0 = SO.r.bbox;
            memcpy(rows0, SO.rows, sizeof(rgn_row_t) * n0);
            memcpy(pool0, SO.pool, sizeof(int16_t) * xu0);
            region_normalize(&SO.r);
            if (SO.r.n_rows != n0 || SO.r.x_pool_used != xu0) { bad = 1; break; }
            if (memcmp(&SO.r.bbox, &bb0, sizeof bb0) != 0) { bad = 1; break; }
            /* x-lists may be re-laid-out by normalize; compare the *pixels* are
             * identical AND the row count/used stayed (idempotent layout). */
            for (uint16_t i = 0; i < n0; i++)
                if (SO.rows[i].y_top != rows0[i].y_top ||
                    SO.rows[i].x_count != rows0[i].x_count) { bad = 1; break; }
            if (memcmp(SO.pool, pool0, sizeof(int16_t) * xu0) != 0) bad = 1;
        }
        CHECK(!bad, "normalize is idempotent (bit-exact: rows, x_pool, bbox)");
    }

    /* ======================================================================
     * Algebra identities (commutativity, associativity, De Morgan, A DIFF A,
     * A XOR A, A UNION comp(A) = frame, rect-fast-path == general-path).
     * ====================================================================== */
    {
        int comm = 0, assoc = 0, demorgan = 0, selfd = 0, selfx = 0,
            compl_ = 0, fastpath = 0, eqsym = 0;
        for (int t = 0; t < 1500; t++) {
            new_window();
            /* the explicit complement frame -- MUST track the window: the
             * "A UNION comp(A,frame) == frame" identity below only holds when
             * A is WITHIN frame, and gen_spec() bounds every coordinate to the
             * current window, whatever new_window() picked (initech-xi7x: this
             * used to be a fixed {0,0,GH,GW} outside the loop, which silently
             * assumed the legacy origin-0 generator). */
            rgn_rect_t FRAME = { (int16_t)g_win_oy, (int16_t)g_win_ox,
                                  (int16_t)(g_win_oy + GH), (int16_t)(g_win_ox + GW) };
            rgn_spec_t sA, sB, sC; gen_spec(&sA); gen_spec(&sB); gen_spec(&sC);
            static rgn_store_t SA, SB, SC, X, Y, Z, W;
            spec_to_region(&sA, &SA); spec_to_region(&sB, &SB); spec_to_region(&sC, &SC);

            /* commutativity: A UNION B == B UNION A; A INT B == B INT A */
            store_attach(&X); store_attach(&Y);
            region_op(&X.r, &SA.r, &SB.r, RGN_OP_UNION);
            region_op(&Y.r, &SB.r, &SA.r, RGN_OP_UNION);
            if (!region_equal(&X.r, &Y.r)) comm++;
            store_attach(&X); store_attach(&Y);
            region_op(&X.r, &SA.r, &SB.r, RGN_OP_INTERSECT);
            region_op(&Y.r, &SB.r, &SA.r, RGN_OP_INTERSECT);
            if (!region_equal(&X.r, &Y.r)) comm++;

            /* associativity: (A U B) U C == A U (B U C) */
            store_attach(&X); store_attach(&Y); store_attach(&Z); store_attach(&W);
            region_op(&X.r, &SA.r, &SB.r, RGN_OP_UNION);
            region_op(&Y.r, &X.r, &SC.r, RGN_OP_UNION);
            region_op(&Z.r, &SB.r, &SC.r, RGN_OP_UNION);
            region_op(&W.r, &SA.r, &Z.r, RGN_OP_UNION);
            if (!region_equal(&Y.r, &W.r)) assoc++;

            /* De Morgan over FRAME: comp(A U B) == comp(A) INT comp(B) */
            {
                static rgn_store_t cA, cB, lhs, rhs, uAB;
                store_attach(&uAB); region_op(&uAB.r, &SA.r, &SB.r, RGN_OP_UNION);
                store_attach(&lhs); region_complement(&lhs.r, &uAB.r, FRAME);
                store_attach(&cA);  region_complement(&cA.r, &SA.r, FRAME);
                store_attach(&cB);  region_complement(&cB.r, &SB.r, FRAME);
                store_attach(&rhs); region_op(&rhs.r, &cA.r, &cB.r, RGN_OP_INTERSECT);
                if (!region_equal(&lhs.r, &rhs.r)) demorgan++;
            }

            /* A DIFF A == empty; A XOR A == empty */
            store_attach(&X); region_op(&X.r, &SA.r, &SA.r, RGN_OP_DIFF);
            if (!region_is_empty(&X.r)) selfd++;
            store_attach(&Y); region_op(&Y.r, &SA.r, &SA.r, RGN_OP_XOR);
            if (!region_is_empty(&Y.r)) selfx++;

            /* A UNION comp(A, frame) == frame  (over the frame) */
            {
                static rgn_store_t cA, uni, fr;
                store_attach(&cA);  region_complement(&cA.r, &SA.r, FRAME);
                store_attach(&uni); region_op(&uni.r, &SA.r, &cA.r, RGN_OP_UNION);
                store_attach(&fr);  region_set_rect(&fr.r, FRAME);
                /* only meaningful when A is within the frame, which it is (grid
                 * == frame); the union of A and its frame-complement is frame. */
                if (!region_equal(&uni.r, &fr.r)) compl_++;
            }

            /* region_equal symmetry */
            store_attach(&X); region_op(&X.r, &SA.r, &SB.r, RGN_OP_XOR);
            store_attach(&Y); region_op(&Y.r, &SB.r, &SA.r, RGN_OP_XOR);
            if (region_equal(&X.r, &Y.r) != region_equal(&Y.r, &X.r)) eqsym++;
        }

        /* rect-fast-path == general-path: a single-rect region built by
         * set_rect must EQUAL the same rect built via from_rects (general).
         * Purely structural (region_equal, no bitmap) -- no grid/window needed,
         * so this can sample the FULL int16 domain directly, no crop-follow
         * required (initech-xi7x): odd t samples top/left/bot/right anywhere
         * in [INT16_MIN, INT16_MAX], even t keeps the legacy small-nonneg
         * range for regression continuity. */
        for (int t = 0; t < 500; t++) {
            int top, left, bot, right;
            if (t & 1) {
                top  = rnd(INT16_MIN, INT16_MAX - 1);
                left = rnd(INT16_MIN, INT16_MAX - 1);
                bot   = rnd(top + 1, INT16_MAX);
                right = rnd(left + 1, INT16_MAX);
            } else {
                top = rnd(0, GH - 1); left = rnd(0, GW - 1);
                bot = rnd(top + 1, GH); right = rnd(left + 1, GW);
            }
            rgn_rect_t rc = { (int16_t)top, (int16_t)left, (int16_t)bot, (int16_t)right };
            static rgn_store_t F, G;
            store_attach(&F); region_set_rect(&F.r, rc);
            store_attach(&G); region_from_rects(&G.r, &rc, 1);
            if (!region_equal(&F.r, &G.r)) fastpath++;
        }

        CHECK(comm == 0,     "commutativity: A U B == B U A, A INT B == B INT A");
        CHECK(assoc == 0,    "associativity: (A U B) U C == A U (B U C)");
        CHECK(demorgan == 0, "De Morgan: comp(A U B) == comp(A) INT comp(B)");
        CHECK(selfd == 0,    "A DIFF A == empty");
        CHECK(selfx == 0,    "A XOR A == empty");
        CHECK(compl_ == 0,   "A UNION comp(A,frame) == frame");
        CHECK(fastpath == 0, "rect fast-path region == general-path region");
        CHECK(eqsym == 0,    "region_equal is symmetric");
    }

    return TEST_SUMMARY("test_region");
}

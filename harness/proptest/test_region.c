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

typedef struct bitmap { uint8_t px[GW * GH]; } bitmap_t;

static void bm_clear(bitmap_t *b) { memset(b->px, 0, sizeof b->px); }

static int bm_get(const bitmap_t *b, int x, int y)
{
    if (x < 0 || x >= GW || y < 0 || y >= GH) return 0;
    return b->px[y * GW + x] ? 1 : 0;
}
static void bm_set(bitmap_t *b, int x, int y, int v)
{
    if (x < 0 || x >= GW || y < 0 || y >= GH) return;
    b->px[y * GW + x] = (uint8_t)(v ? 1 : 0);
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
            if (y < 0 || y >= GH) continue;
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
 *     and both are SHRINKABLE (the shrinker bisects the list of items). --- */
enum { MAX_ITEMS = 6 };
typedef struct rgn_spec {
    int  is_raw;                 /* 0 = rects, 1 = raw scanline spans          */
    int  n;                      /* item count                                 */
    /* rect items: [top,left,bottom,right]; raw items: [y, x0, x1, _] (one span
     * on scanline y, columns [x0,x1)). A raw spec is a *bag* of single spans;
     * overlapping/adjacent spans on the same row exercise the normalizer. */
    int  it[MAX_ITEMS][4];
} rgn_spec_t;

static void gen_spec(rgn_spec_t *s)
{
    s->is_raw = rnd(0, 1);
    s->n = rnd(1, MAX_ITEMS);
    for (int i = 0; i < s->n; i++) {
        if (!s->is_raw) {
            int t = rnd(0, GH - 1), l = rnd(0, GW - 1);
            int b = rnd(0, GH),     r = rnd(0, GW);
            s->it[i][0] = t; s->it[i][1] = l; s->it[i][2] = b; s->it[i][3] = r;
        } else {
            int y  = rnd(0, GH - 1);
            int x0 = rnd(0, GW - 1);
            int x1 = rnd(0, GW);
            s->it[i][0] = y; s->it[i][1] = x0; s->it[i][2] = x1; s->it[i][3] = 0;
        }
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
        /* 2. clamp each coord toward 0 while it still fails. */
        for (int w = 0; w < 2; w++) {
            rgn_spec_t *S = which[w];
            for (int i = 0; i < S->n; i++)
                for (int c = 0; c < 4; c++) {
                    while (S->it[i][c] > 0) {
                        rgn_spec_t cand = *S;
                        cand.it[i][c]--;
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
    }

    /* ---- construction fidelity: a region built from a spec rasterizes EXACTLY
     *      to the spec's directly-painted bitmap (independent oracle: the spec
     *      bitmap is painted from the raw items, NOT via the region) ---------- */
    {
        static rgn_store_t S; bitmap_t fromrgn, fromspec;
        rgn_spec_t s;
        int mism = 0;
        for (int t = 0; t < 500 && !mism; t++) {
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
            gen_spec(&s);
            spec_to_region(&s, &S);
            rasterize(&S.r, &bm);
            for (int y = 0; y < GH && !mism; y++)
                for (int x = 0; x < GW; x++) {
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
            rgn_spec_t sA, sB; gen_spec(&sA); gen_spec(&sB);
            static rgn_store_t SA, SB; bitmap_t bA, bB;
            spec_to_region(&sA, &SA); spec_to_region(&sB, &SB);
            rasterize(&SA.r, &bA); rasterize(&SB.r, &bB);
            /* ground truth: do A and B share any pixel? */
            int share = 0;
            for (int i = 0; i < GW * GH && !share; i++) if (bA.px[i] && bB.px[i]) share = 1;
            if (region_intersects(&SA.r, &SB.r) != share) inter_bad = 1;

            /* region_rect_fully_in (CONTAINMENT): a random rect is fully inside A
             * iff EVERY one of its pixels is set in A's bitmap. */
            int top = rnd(0, GH - 1), left = rnd(0, GW - 1);
            int bot = rnd(top + 1, GH), right = rnd(left + 1, GW);
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
     * The region produced by EVERY op is in normal form, and its bbox is the
     * tight bounding box of its pixels; region_assert_normal agrees.
     * ====================================================================== */
    {
        int nf_bad = 0, bbox_bad = 0, assert_bad = 0;
        for (int t = 0; t < 1500 && !nf_bad && !bbox_bad; t++) {
            rgn_spec_t sA, sB; gen_spec(&sA); gen_spec(&sB);
            static rgn_store_t SA, SB, SO; bitmap_t bm;
            spec_to_region(&sA, &SA); spec_to_region(&sB, &SB);
            for (int op_i = 0; op_i < 4; op_i++) {
                store_attach(&SO);
                region_op(&SO.r, &SA.r, &SB.r, OPS[op_i]);
                if (!normal_form_holds(&SO.r)) { nf_bad = 1; break; }
                if (!region_assert_normal(&SO.r)) { assert_bad = 1; break; }
                rasterize(&SO.r, &bm);
                /* tight bbox of bm */
                int minx = GW, miny = GH, maxx = -1, maxy = -1, any = 0;
                for (int y = 0; y < GH; y++) for (int x = 0; x < GW; x++)
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
        rgn_rect_t FRAME = { 0, 0, GH, GW };   /* the explicit complement frame */
        for (int t = 0; t < 1500; t++) {
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
         * set_rect must EQUAL the same rect built via from_rects (general). */
        for (int t = 0; t < 500; t++) {
            int top = rnd(0, GH - 1), left = rnd(0, GW - 1);
            int bot = rnd(top + 1, GH), right = rnd(left + 1, GW);
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

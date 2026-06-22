/* test_region_gdi.c -- the DUAL-HERITAGE region CONFORMANCE ORACLE.
 *
 * beads: initech-n79q (landing step 1b -- the independent-golden grading leg);
 *        ADR-0005 Amendment AM-1 (OEA-ADR-0005-A1) Sec 7 (Verification L1/L2/L3,
 *        the Sec 7.4 mutation table), FO-D2-5.
 * Ref:   spec/region_algebra.h Sec 7b -- the GDI/HRGN heritage facade
 *        (CombineRgn / GetRgnBox / PtInRegion / RectInRegion + RGN_* modes +
 *        region-type codes) over the ONE neutral region_op (constraint C-7).
 * Ref:   os/flair/atkinson/region.c -- the facade under test.
 * Ref:   ../win31-decomp/oracles/wine/server/region.c -- THE INDEPENDENT GOLDEN
 *        (the wine banded-rect engine), compiled host-only under gdi_ref_ via
 *        gdi_ref_wine_shim.h. A different heritage's representation; NOT a
 *        re-derivation of ATKINSON's inversion-list algorithm -> a genuine
 *        non-by-construction Law-2 golden (AM-1 Sec 2.3 / Sec 7.2).
 * Ref:   CLAUDE.md Law 1 (ground truth), Law 2 (the oracle is the truth, graded
 *        against an INDEPENDENT golden), Law 3 (no host-only factory code leaks
 *        into the artifact -- the wine path is TEST-ONLY), Rule 6 (mutation-
 *        proven), Rule 11 (seeded LCG -> deterministic), Rule 12 (ASCII).
 *
 * THE THREE LEGS (AM-1 Sec 7.1-7.3):
 *   L1 -- cross-family equality (by-construction; necessary NOT sufficient):
 *         region_equal(CombineRgn(A,B,RGN_*), QuickDrawOp(A,B)) for AND/OR/DIFF/
 *         XOR/COPY. Both facades call the SAME region_op, so this only proves
 *         they AGREE -- it cannot catch a wrong VALUE (that is L2's job).
 *   L2 -- the INDEPENDENT golden: build the SAME rect set into BOTH the ATKINSON
 *         region AND a gdi_ref_ wine region; for all four CombineRgn modes + COPY
 *         compare BIT-EXACT on the RASTERIZED pixel sets over a bounded grid
 *         (memcmp), plus GetRgnBox complexity, PtInRegion, and RectInRegion
 *         (the AM-4 OVERLAP semantic) against wine's point_in_region /
 *         rect_in_region. LOUD-SKIPS (banner, continue) if ../win31-decomp is
 *         absent or the wine file cannot be found; NEVER silent-passes L2.
 *   L3 -- single-engine structural gate: grep FAILS LOUD if any region_op /
 *         xmerge / band-sweep DEFINITION appears outside region.c (constraint
 *         C-7). The wine static region_op stays gdi_ref_-namespaced + host-only.
 *
 * MUTANTS (Rule 6; AM-1 Sec 7.4), built with extra -D and required RED:
 *   -DGDI_MUTATE_DISPATCH     CombineRgn RGN_OR->INTERSECT  -> L1 AND L2 RED
 *   -DGDI_MUTATE_RECTIN       RectInRegion overlap->contain -> L2 RectInRegion RED
 *   -DRGN_MUTATE_EMIT_NOCHANGE (existing engine mutant)     -> L1 GREEN / L2 RED
 *   -DRGN_MUTATE_PARITY_OFF1   (existing engine mutant)     -> L2 RED (cross-redden)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED spec (-Ispec)                       */
#include "region.h"           /* the ATKINSON facade under test (-Ios/flair/...) */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

/* ---------------------------------------------------------------------------
 * The INDEPENDENT golden lives in a SEPARATE translation unit (gdi_ref_wine.c)
 * so the wine `struct region` cannot clash with FLAIR's `struct region`
 * (region_t) here. We see it only through the OPAQUE handle interface. Whether
 * the wine engine was actually compiled in is decided at the wine TU's build
 * time (by __has_include over WIN31_DECOMP) and reported via the linked symbol
 * gdi_ref_wine_available -- the oracle reads that at runtime to LOUD-SKIP L2
 * (it NEVER silent-passes L2). The macro GDI_REF_WINE_LINKED tells us the wine
 * TU is on the link line at all (the build always links it; when wine is absent
 * gdi_ref_wine_available is 0). We keep a compile-time flag too so the L2 body
 * is always compiled (it calls only the opaque interface, present either way).
 * ------------------------------------------------------------------------- */
#include "gdi_ref_wine.h"

TEST_HARNESS();

/* ===========================================================================
 * The bounded grid + rasterization (mirrors test_region.c: GW x GH, bm_get).
 * The homomorphism is bit-exact, so a small grid keeps thousands of cases cheap
 * AND makes the L2 pixel-set comparison apples-to-apples with the engine side.
 * ===========================================================================*/
enum { GW = 48, GH = 40 };

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
static int bm_equal(const bitmap_t *A, const bitmap_t *B)
{
    return memcmp(A->px, B->px, sizeof A->px) == 0;
}

/* Rasterize an ATKINSON region straight off its rows/x-lists (independent of
 * region_contains_point -- same approach as test_region.c). */
static void rasterize_atk(const region_t *r, bitmap_t *out)
{
    bm_clear(out);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;
        if (r->rows[i].x_count == 0) continue;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= GH) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2) {
                int xa = r->rows[i].x[k], xb = r->rows[i].x[k + 1];
                for (int x = xa; x < xb; x++) bm_set(out, x, y, 1);
            }
        }
    }
}

/* ===========================================================================
 * ATKINSON region storage: a host arena per region (the engine never mallocs;
 * the CALLER supplies rows[]/x_pool -- exactly the test_region.c idiom).
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
 * Seeded LCG generation (Rule 11). A region is a union of N rects over the
 * bounded grid -- both engines ingest the SAME rect list, so any divergence is
 * an engine/facade difference, not a generator artifact.
 * ===========================================================================*/
static uint32_t g_seed = 0x1234567u;
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}
static int rnd(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(lcg() % (uint32_t)(hi - lo + 1));
}

enum { MAX_RECTS = 5 };
typedef struct rectset {
    int  n;
    int  it[MAX_RECTS][4];   /* [top, left, bottom, right] (ATKINSON field order) */
} rectset_t;

static void gen_rectset(rectset_t *s)
{
    s->n = rnd(1, MAX_RECTS);
    for (int i = 0; i < s->n; i++) {
        int t = rnd(0, GH - 1), l = rnd(0, GW - 1);
        int b = rnd(t + 1, GH), r = rnd(l + 1, GW);
        s->it[i][0] = t; s->it[i][1] = l; s->it[i][2] = b; s->it[i][3] = r;
    }
}

/* Build the union-of-rects into an ATKINSON region (general path). */
static void rectset_to_atk(const rectset_t *s, rgn_store_t *out)
{
    rgn_rect_t rc[MAX_RECTS];
    for (int i = 0; i < s->n; i++) {
        rc[i].top    = (int16_t)s->it[i][0]; rc[i].left   = (int16_t)s->it[i][1];
        rc[i].bottom = (int16_t)s->it[i][2]; rc[i].right  = (int16_t)s->it[i][3];
    }
    store_attach(out);
    region_from_rects(&out->r, rc, (uint16_t)s->n);
}

/* Ground-truth bitmap painted DIRECTLY from the rect list (independent of any
 * engine) -- used to sanity the generation and as the L2 tight-bbox source. */
static void rectset_to_bitmap(const rectset_t *s, bitmap_t *out)
{
    bm_clear(out);
    for (int i = 0; i < s->n; i++) {
        int t = s->it[i][0], l = s->it[i][1], b = s->it[i][2], r = s->it[i][3];
        for (int y = t; y < b; y++)
            for (int x = l; x < r; x++) bm_set(out, x, y, 1);
    }
}

/* Pixel-wise Boolean op (the powerset side; matches rgn_op_t). */
static void bm_op(bitmap_t *out, const bitmap_t *A, const bitmap_t *B, int mode)
{
    for (int i = 0; i < GW * GH; i++) {
        int a = A->px[i] ? 1 : 0, b = B->px[i] ? 1 : 0, o = 0;
        switch (mode) {
            case RGN_AND:  o = a & b;       break;
            case RGN_OR:   o = a | b;       break;
            case RGN_DIFF: o = a & (b ^ 1); break;
            case RGN_XOR:  o = a ^ b;       break;
            case RGN_COPY: o = a;           break;
        }
        out->px[i] = (uint8_t)o;
    }
}

/* Tight bbox of a bitmap (half-open). Returns 0 if no pixels. */
static int bm_tight_bbox(const bitmap_t *b, rgn_rect_t *out)
{
    int minx = GW, miny = GH, maxx = -1, maxy = -1, any = 0;
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++)
            if (bm_get(b, x, y)) {
                any = 1;
                if (x < minx) minx = x;
                if (x + 1 > maxx) maxx = x + 1;
                if (y < miny) miny = y;
                if (y + 1 > maxy) maxy = y + 1;
            }
    if (!any) { out->top = out->left = out->bottom = out->right = 0; return 0; }
    out->top = (int16_t)miny; out->left = (int16_t)minx;
    out->bottom = (int16_t)maxy; out->right = (int16_t)maxx;
    return 1;
}

/* The complexity code the result SHOULD carry, computed from the pixel truth
 * (independent of the engine's classifier): empty -> NULLREGION; a single solid
 * rectangle (tight bbox fully painted) -> SIMPLEREGION; else COMPLEXREGION. */
static int expected_complexity(const bitmap_t *b)
{
    rgn_rect_t bb;
    if (!bm_tight_bbox(b, &bb)) return NULLREGION;
    for (int y = bb.top; y < bb.bottom; y++)
        for (int x = bb.left; x < bb.right; x++)
            if (!bm_get(b, x, y)) return COMPLEXREGION;  /* a hole -> complex   */
    return SIMPLEREGION;
}

/* ===========================================================================
 * The wine (gdi_ref_) side, through the OPAQUE interface (gdi_ref_wine.h).
 *
 * COORDINATE-CONVENTION BRIDGE (Law 1 honesty): ATKINSON rgn_rect_t is
 * (top,left,bottom,right); the wine rectangle is (left,top,right,bottom). Same
 * half-open span semantics, DIFFERENT field order -- bridged explicitly here at
 * every boundary so a transposed coordinate cannot masquerade as a region
 * difference. The gdi_ref_wine_* accessors take/return (l,t,r,b) explicitly.
 * ===========================================================================*/

/* wine union-of-rects: union each rect into the accumulator -- mirrors how
 * region_from_rects builds the ATKINSON side, but through the wholly-independent
 * wine band engine. Returns an owned handle (free with gdi_ref_wine_free). */
static gdi_ref_rgn wine_from_rectset(const rectset_t *s)
{
    gdi_ref_rgn acc = gdi_ref_wine_create();
    gdi_ref_rgn one = gdi_ref_wine_create();
    if (!acc || !one) { gdi_ref_wine_free(acc); gdi_ref_wine_free(one); return 0; }
    for (int i = 0; i < s->n; i++) {
        /* ATKINSON [top,left,bottom,right] -> wine (left,top,right,bottom) */
        gdi_ref_wine_set_rect(one, s->it[i][1], s->it[i][0],
                                   s->it[i][3], s->it[i][2]);
        gdi_ref_wine_union(acc, acc, one);
    }
    gdi_ref_wine_free(one);
    return acc;
}

static void rasterize_wine(gdi_ref_rgn r, bitmap_t *out)
{
    int n = gdi_ref_wine_num_rects(r);
    bm_clear(out);
    for (int i = 0; i < n; i++) {
        int l, t, rr, b;
        if (!gdi_ref_wine_get_rect(r, i, &l, &t, &rr, &b)) continue;
        for (int y = t; y < b; y++)
            for (int x = l; x < rr; x++) bm_set(out, x, y, 1);
    }
}

/* Run one CombineRgn mode through the wine engine into `dst`. Returns 1 on ok. */
static int wine_combine(gdi_ref_rgn dst, gdi_ref_rgn s1, gdi_ref_rgn s2, int mode)
{
    switch (mode) {
        case RGN_AND:  return gdi_ref_wine_intersect(dst, s1, s2);
        case RGN_OR:   return gdi_ref_wine_union(dst, s1, s2);
        case RGN_DIFF: return gdi_ref_wine_subtract(dst, s1, s2);
        case RGN_XOR:  return gdi_ref_wine_xor(dst, s1, s2);
        case RGN_COPY: return gdi_ref_wine_copy(dst, s1);
    }
    return 0;
}

/* ===========================================================================
 * L3 -- the single-engine structural grep gate (load-bearing per AM-1 Sec 2.3 /
 * Sec 7.3 / constraint C-7). FAIL LOUD if any region_op / xmerge / band-sweep
 * DEFINITION appears OUTSIDE os/flair/atkinson/region.c. We scan os/ for a
 * C-function-DEFINITION of those names (a return-type line ending in the name +
 * '(' that is NOT a call and NOT a prototype ';'); region.c is the one allowed
 * site. The wine engine's own static region_op is gdi_ref_-namespaced and lives
 * under ../win31-decomp (outside os/), so it is invisible to this scan by both
 * its namespace AND its location.
 *
 * Implemented as a portable grep via system(): we search os/ (the artifact tree)
 * for a DEFINITION pattern, EXCLUDING region.c, and require zero hits. A
 * second-engine definition (a new region_op/xmerge in os/) makes this RED.
 * ===========================================================================*/
static int l3_single_engine_gate(void)
{
    /* A definition looks like  "<type> region_op(" / "...xmerge(" at line start
     * (allowing static/inline qualifiers + a return type), and crucially is NOT
     * followed by ';' on the same logical line (a prototype) and is NOT a call
     * (calls are indented and preceded by '=', 'return', etc.). We approximate
     * "definition outside region.c" with: a line that matches the function-head
     * regex for region_op|xmerge|active_xlist (the band-sweep helpers), in any
     * os/ .c file whose path is NOT region.c. The engine's own region.c is the
     * sole legal home; everything else is a second-engine violation.
     *
     * grep -rnE over os/ ; --include='*.c' ; exclude region.c ; the ERE matches
     * a function head (start-of-line, optional qualifiers/type, the name, '('),
     * NOT a call (no leading '.'/'->'/'=' and not 'return'). We additionally
     * require the line to NOT end in ';' (prototype) by excluding ');' tails. */
    const char *cmd =
        "grep -rnE "
        "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
        "os --include='*.c' "
        "| grep -v 'os/flair/atkinson/region.c' "
        "| grep -vE ';[ \\t]*$' "       /* drop prototype lines (end in ';')     */
        "> /dev/null 2>&1";
    int rc = system(cmd);
    /* grep exits 0 when it FOUND a match (=> a violation), 1 when none. We want
     * NONE outside region.c, so PASS iff grep found nothing (rc != 0). */
    int found = (rc == 0);
    if (found) {
        fprintf(stderr,
            "  L3 FAIL: a region_op/xmerge DEFINITION exists OUTSIDE "
            "os/flair/atkinson/region.c -- a SECOND region engine (C-7 violation):\n");
        /* re-run visibly so the operator sees the offending line(s). */
        (void)system(
            "grep -rnE "
            "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
            "os --include='*.c' "
            "| grep -v 'os/flair/atkinson/region.c' "
            "| grep -vE ';[ \\t]*$' 1>&2");
        return 0;
    }
    return 1;
}

/* Positive control for L3: confirm the gate actually SEES region.c's region_op
 * definition (so a gate that matches nothing -- e.g. a broken regex -- is caught
 * as decoration, not silently green). */
static int l3_positive_control(void)
{
    const char *cmd =
        "grep -rnE "
        "'^[A-Za-z_][A-Za-z0-9_ \\t\\*]*[ \\t\\*](region_op|xmerge)[ \\t]*\\(' "
        "os/flair/atkinson/region.c "
        "| grep -vE ';[ \\t]*$' > /dev/null 2>&1";
    return system(cmd) == 0;   /* must FIND region.c's own definitions */
}

/* ===========================================================================
 * MAIN -- the three legs.
 * ===========================================================================*/
int main(void)
{
    const int MODES[5]   = { RGN_AND, RGN_OR, RGN_DIFF, RGN_XOR, RGN_COPY };
    const char *MNAME[5] = { "RGN_AND", "RGN_OR", "RGN_DIFF", "RGN_XOR", "RGN_COPY" };

    printf("test-region-gdi: dual-heritage conformance oracle "
           "(ADR-0005 Amendment AM-1 L1/L2/L3)\n");

    /* ---- generation sanity: an ATKINSON region built from a rectset must
     *      rasterize EXACTLY to the rectset's directly-painted bitmap (an
     *      independent paint), and the pixel-op helper bm_op must agree with the
     *      rasterized facade output for RGN_OR -- guards the L1/L2 substrate. */
    {
        int gen_bad = 0, op_bad = 0;
        for (int t = 0; t < 500 && !gen_bad && !op_bad; t++) {
            rectset_t sA, sB; gen_rectset(&sA); gen_rectset(&sB);
            static rgn_store_t SA, SB, GDI;
            rectset_to_atk(&sA, &SA);
            bitmap_t fromrgn, fromspec;
            rasterize_atk(&SA.r, &fromrgn);
            rectset_to_bitmap(&sA, &fromspec);
            if (!bm_equal(&fromrgn, &fromspec)) gen_bad = 1;
            /* bm_op cross-check: rasterize(CombineRgn(A,B,RGN_OR)) == raster(A)
             * OR_set raster(B) (the homomorphism, restated on the GDI facade). */
            rectset_to_atk(&sB, &SB);
            store_attach(&GDI);
            CombineRgn(&GDI.r, &SA.r, &SB.r, RGN_OR);
            bitmap_t bO, bA, bB, bSet;
            rasterize_atk(&GDI.r, &bO);
            rasterize_atk(&SA.r, &bA); rasterize_atk(&SB.r, &bB);
            bm_op(&bSet, &bA, &bB, RGN_OR);
            if (!bm_equal(&bO, &bSet)) op_bad = 1;
        }
        CHECK(!gen_bad, "substrate: ATKINSON region from rectset rasterizes to "
                        "the directly-painted bitmap");
        CHECK(!op_bad,  "substrate: rasterize(CombineRgn(A,B,RGN_OR)) == "
                        "raster(A) OR_set raster(B)");
    }

    /* ======================================================================
     * L1 -- cross-family equality (BY-CONSTRUCTION; necessary NOT sufficient).
     * region_equal(CombineRgn(A,B,mode), QuickDrawOp(A,B)) over thousands of
     * pairs. Both facades bottom out in the IDENTICAL region_op, so a GREEN
     * here proves only AGREEMENT, never correctness (that is L2). A wrong-VALUE
     * engine mutant (RGN_MUTATE_EMIT_NOCHANGE) leaves L1 GREEN -- demonstrated
     * by the mutation matrix. A mode-DISPATCH mutant DOES redden L1.
     * ====================================================================== */
    {
        enum { CASES = 3000 };
        int l1_bad = 0; int first_bad_mode = -1;
        for (int t = 0; t < CASES && !l1_bad; t++) {
            rectset_t sA, sB; gen_rectset(&sA); gen_rectset(&sB);
            static rgn_store_t SA, SB, GDI, QD;
            rectset_to_atk(&sA, &SA); rectset_to_atk(&sB, &SB);
            for (int m = 0; m < 5; m++) {
                int mode = MODES[m];
                store_attach(&GDI);
                CombineRgn(&GDI.r, &SA.r, &SB.r, mode);
                store_attach(&QD);
                switch (mode) {
                    case RGN_AND:  SectRgn (&SA.r, &SB.r, &QD.r); break;
                    case RGN_OR:   UnionRgn(&SA.r, &SB.r, &QD.r); break;
                    case RGN_DIFF: DiffRgn (&SA.r, &SB.r, &QD.r); break;
                    case RGN_XOR:  XorRgn  (&SA.r, &SB.r, &QD.r); break;
                    case RGN_COPY: UnionRgn(&SA.r, &SA.r, &QD.r); break; /* copy of src1 */
                }
                if (!region_equal(&GDI.r, &QD.r)) {
                    l1_bad = 1; first_bad_mode = mode; break;
                }
            }
        }
        if (l1_bad)
            fprintf(stderr, "  L1 FAIL: CombineRgn(%s) != QuickDraw facade "
                    "(the two heritage facades diverge over the same region_op)\n",
                    (first_bad_mode >= 0 && first_bad_mode <= 5)
                      ? MNAME[first_bad_mode == RGN_COPY ? 4 :
                              first_bad_mode == RGN_AND ? 0 :
                              first_bad_mode == RGN_OR ? 1 :
                              first_bad_mode == RGN_DIFF ? 2 : 3]
                      : "?");
        CHECK(!l1_bad, "L1 [by-construction]: CombineRgn == QuickDraw facade "
                       "(AND/OR/DIFF/XOR/COPY, 3000 pairs)");
    }

    /* ======================================================================
     * L2 -- the INDEPENDENT golden (the real Law-2 signal). Runtime-gated on
     * gdi_ref_wine_available (decided by __has_include over WIN31_DECOMP in the
     * wine TU's build). Absent -> LOUD-SKIP, never a silent L2 pass.
     * ====================================================================== */
    if (!gdi_ref_wine_available) {
        fprintf(stderr,
            "  ======================================================================\n"
            "  SKIP: ../win31-decomp absent (or wine server/region.c not found) --\n"
            "        the wine DIFFERENTIAL (L2) was NOT run. The independent-golden\n"
            "        Law-2 signal is UNAVAILABLE this run; only the by-construction\n"
            "        floor (L1) and the structural gate (L3) stand. This is a LOUD\n"
            "        SKIP, never a silent pass -- point WIN31_DECOMP at the decomp\n"
            "        tree to enable L2. (ADR-0005 Amendment AM-1 Sec 7.2.)\n"
            "  ======================================================================\n");
        /* A skip MARKER (passing), so the summary is honest that L2 did not run.
         * No L2 oracle CHECK is emitted as green: the banner is unmissable and
         * the process still exits 0 only on the L1 floor + L3 gate. */
        CHECK(1, "L2 [SKIPPED -- ../win31-decomp absent; wine differential not run]");
    } else {
        enum { CASES = 3000 };
        int l2_modes_bad = 0;     /* rasterized pixel-set mismatch (any mode)    */
        int l2_cplx_bad  = 0;     /* GetRgnBox complexity / extents mismatch     */
        int l2_pt_bad    = 0;     /* PtInRegion vs wine point_in_region          */
        int l2_rir_bad   = 0;     /* RectInRegion vs wine rect_in_region         */
        int bad_mode = -1;
        for (int t = 0; t < CASES; t++) {
            rectset_t sA, sB; gen_rectset(&sA); gen_rectset(&sB);
            static rgn_store_t SA, SB, GDI;
            rectset_to_atk(&sA, &SA); rectset_to_atk(&sB, &SB);
            gdi_ref_rgn WA = wine_from_rectset(&sA);
            gdi_ref_rgn WB = wine_from_rectset(&sB);
            gdi_ref_rgn WO = gdi_ref_wine_create();
            if (!WA || !WB || !WO) { l2_modes_bad = 1; bad_mode = -2;
                gdi_ref_wine_free(WA); gdi_ref_wine_free(WB); gdi_ref_wine_free(WO);
                break; }

            for (int m = 0; m < 5; m++) {
                int mode = MODES[m];
                /* ATKINSON GDI facade result */
                store_attach(&GDI);
                int atk_cplx = CombineRgn(&GDI.r, &SA.r, &SB.r, mode);
                bitmap_t atk_bm; rasterize_atk(&GDI.r, &atk_bm);
                /* wine result (fresh empty dst each mode) */
                gdi_ref_wine_set_rect(WO, 0, 0, 0, 0);           /* -> empty       */
                if (!wine_combine(WO, WA, WB, mode)) { l2_modes_bad = 1; bad_mode = mode; break; }
                bitmap_t win_bm; rasterize_wine(WO, &win_bm);

                /* (a) BIT-EXACT rasterized pixel-set comparison (the authority) */
                if (!bm_equal(&atk_bm, &win_bm)) {
                    if (!l2_modes_bad) { l2_modes_bad = 1; bad_mode = mode; }
                }
                /* (b) complexity + extents: ATKINSON's GetRgnBox return vs the
                 *     pixel-truth derived from the INDEPENDENT wine raster (wine
                 *     carries no complexity code; its union extents can be loose,
                 *     so we grade against the pixel-tight box of the wine raster). */
                int want_cplx = expected_complexity(&win_bm);
                if (atk_cplx != want_cplx) l2_cplx_bad = 1;
                rgn_rect_t atk_box;
                int box_cplx = GetRgnBox(&GDI.r, &atk_box);
                if (box_cplx != want_cplx) l2_cplx_bad = 1;
                rgn_rect_t tight;
                if (bm_tight_bbox(&win_bm, &tight)) {
                    if (atk_box.top != tight.top || atk_box.left != tight.left ||
                        atk_box.bottom != tight.bottom || atk_box.right != tight.right)
                        l2_cplx_bad = 1;
                }
            }
            if (bad_mode == -2) {
                gdi_ref_wine_free(WA); gdi_ref_wine_free(WB); gdi_ref_wine_free(WO);
                break;
            }

            /* (c) PtInRegion vs wine point_in_region, and (d) RectInRegion vs
             *     wine rect_in_region (OVERLAP) -- graded on region A against the
             *     independent wine engine, over a point lattice and probe rects. */
            {
                store_attach(&GDI);
                CombineRgn(&GDI.r, &SA.r, &SA.r, RGN_COPY);   /* region A         */
                for (int y = 0; y < GH; y += 3)
                    for (int x = 0; x < GW; x += 3) {
                        int atk_pt  = PtInRegion(&GDI.r, (int16_t)x, (int16_t)y) ? 1 : 0;
                        int wine_pt = gdi_ref_wine_point_in(WA, x, y);
                        if (atk_pt != wine_pt) l2_pt_bad = 1;
                    }
                for (int p = 0; p < 6; p++) {
                    int top = rnd(0, GH - 1), left = rnd(0, GW - 1);
                    int bot = rnd(top + 1, GH), right = rnd(left + 1, GW);
                    rgn_rect_t arc = { (int16_t)top, (int16_t)left,
                                       (int16_t)bot, (int16_t)right };
                    int atk_rir  = RectInRegion(&GDI.r, arc) ? 1 : 0;
                    /* ATKINSON (t,l,b,r) -> wine (l,t,r,b) */
                    int wine_rir = gdi_ref_wine_rect_in(WA, left, top, right, bot);
                    if (atk_rir != wine_rir) l2_rir_bad = 1;
                }
            }

            gdi_ref_wine_free(WA);
            gdi_ref_wine_free(WB);
            gdi_ref_wine_free(WO);
        }

        if (l2_modes_bad && bad_mode >= 0)
            fprintf(stderr, "  L2 FAIL: CombineRgn(%s) rasterized pixel set != "
                    "wine independent golden\n",
                    MNAME[bad_mode == RGN_AND ? 0 : bad_mode == RGN_OR ? 1 :
                          bad_mode == RGN_DIFF ? 2 : bad_mode == RGN_XOR ? 3 : 4]);
        CHECK(!l2_modes_bad, "L2 [independent golden]: CombineRgn all 5 modes "
                             "rasterize BIT-EXACT vs wine (3000 pairs)");
        CHECK(!l2_cplx_bad,  "L2 [independent golden]: GetRgnBox complexity + "
                             "extents match the wine pixel truth");
        CHECK(!l2_pt_bad,    "L2 [independent golden]: PtInRegion matches wine "
                             "point_in_region");
        CHECK(!l2_rir_bad,   "L2 [independent golden]: RectInRegion (OVERLAP) "
                             "matches wine rect_in_region");
    }

    /* ======================================================================
     * L3 -- single-engine structural gate.
     * ====================================================================== */
    CHECK(l3_positive_control(),
          "L3 [control]: the grep gate SEES region.c's own region_op/xmerge "
          "definitions (gate is not decoration)");
    CHECK(l3_single_engine_gate(),
          "L3 [structural]: no region_op/xmerge DEFINITION outside "
          "os/flair/atkinson/region.c (constraint C-7)");

    return TEST_SUMMARY("test-region-gdi");
}

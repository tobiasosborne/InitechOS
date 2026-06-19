/* test_blitter.c -- the FLAIR region-clipped blitter's property suite (ORACLE).
 *
 * beads: initech-i50 ("Blitter with region clipping (visRgn/clipRgn)"). The
 *        blitter is the load-bearing CLIPPING primitive every Manager + the
 *        window drag draws through; a wrong clip corrupts the whole Toolbox
 *        (CLAUDE.md Rule 3 "all bugs are deep"). This is its mechanical oracle
 *        (Law 2).
 *
 * Ref:   PRD Sec 6.2/6.3 (region algebra is the clip; the Toolbox draws through
 *        a region-clipped surface). ADR-0004 D-1/D-2 (ALL drawing clipped by
 *        visRgn INTERSECT clipRgn; the ONE surface module is the sole writer).
 *        spec/region_algebra.h (the LOCKED region contract); os/flair/blitter.h
 *        (the primitive under test); os/flair/surface.h (the ONE pixel writer);
 *        harness/proptest/test_region.c (the property idiom + seeded LCG +
 *        rgn_store_t arena + rasterize ground truth this suite MIRRORS).
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 11 (seeded LCG -> deterministic), Rule 12 ASCII.
 *
 * THE PROPERTY (homomorphism-style):
 *   A region-clipped blit/fill changes destination pixel (x,y) IF AND ONLY IF
 *   that pixel is (a) WITHIN the blit/fill rect AND (b) INSIDE the clip region.
 *
 *   We verify it independently of the blitter's internals:
 *     - Buffer A: run the REAL blitter into a fresh destination.
 *     - Buffer B: the EXPECTED destination, computed WITHOUT the blitter --
 *       start from the same pre-fill, then for every pixel: if it is in the rect
 *       AND inside the clip (membership decided by an INDEPENDENT rasterize of
 *       the clip region, NOT via region_contains_point or the blitter), apply the
 *       source pixel; else leave the pre-fill untouched.
 *     - Assert A == B, BYTE-EXACT, over the whole destination buffer.
 *   The pre-fill is a distinct sentinel so an out-of-clip WRITE (mutant) shows up
 *   as a changed byte, and an in-clip MISS shows up as an unchanged byte.
 *
 * COVERAGE (the cases ad-hoc clipping gets wrong): full-clip (clip == the whole
 * frame => same as unclipped), empty-clip (nothing drawn), NON-RECTANGULAR clip
 * (multiple spans per scanline + vertical holes -- the load-bearing case), and
 * thousands of random partial-overlap (clip, rect) pairs. Both 8bpp and 32bpp
 * destinations (both surface_put_pixel branches). Fill AND copy-blit paths.
 *
 * MUTANTS (Rule 6): BLITTER_MUTATE_IGNORE_CLIP and BLITTER_MUTATE_OFF_BY_ONE,
 * each compiled via -D; the Makefile gate asserts each goes RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED spec (-Ispec)                       */
#include "region.h"           /* the engine constructors (-Ios/flair/atkinson)  */
#include "surface.h"          /* bitmap_t + surface writers (-Ios/flair)        */
#include "blitter.h"          /* the primitive under test (-Ios/flair)          */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)       */

TEST_HARNESS();

/* ===========================================================================
 * The bounded destination + a pixel-set ground truth (rasterize the clip).
 * ---------------------------------------------------------------------------
 * Small GW x GH grid (the property is bit-exact; a small grid makes thousands
 * of cases cheap). Coordinates stay well inside int16. We test an 8bpp dst (one
 * byte/pixel = the value) and a 32bpp dst (one dword/pixel). For both, the
 * EXPECTED buffer is built directly from the spec/rect, never via the blitter.
 * ===========================================================================*/
enum { GW = 40, GH = 32 };       /* grid width/height (pixels)            */

/* A clip-membership bitmap, rasterized from a region row/x-list DIRECTLY (the
 * test_region.c rasterize idiom: independent of region_contains_point, so a bug
 * in one cannot mask a bug in the other). 1 = inside the clip. */
typedef struct cbm { uint8_t in[GW * GH]; } cbm_t;

static void cbm_clear(cbm_t *b) { memset(b->in, 0, sizeof b->in); }
static int  cbm_get(const cbm_t *b, int x, int y)
{
    if (x < 0 || x >= GW || y < 0 || y >= GH) return 0;
    return b->in[y * GW + x] ? 1 : 0;
}
static void cbm_set(cbm_t *b, int x, int y)
{
    if (x < 0 || x >= GW || y < 0 || y >= GH) return;
    b->in[y * GW + x] = 1;
}

/* Rasterize a region into a membership bitmap straight off its rows/x-lists. */
static void rasterize_clip(const region_t *r, cbm_t *out)
{
    cbm_clear(out);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;  /* closing */
        if (r->rows[i].x_count == 0) continue;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= GH) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2) {
                int xa = r->rows[i].x[k];
                int xb = r->rows[i].x[k + 1];
                for (int x = xa; x < xb; x++) cbm_set(out, x, y);
            }
        }
    }
}

/* ===========================================================================
 * Region storage (the test_region.c rgn_store arena: the engine never mallocs;
 * the CALLER supplies rows[] and x_pool). Built from a spec via repeated union.
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
 * Seeded LCG (Rule 11: reproducible fuzz), mirroring test_region.c.
 * ===========================================================================*/
static uint32_t g_seed = 0x0B1177u;  /* (a distinct seed from test_region)     */
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

/* A random clip region: a UNION of up to MAX_RECTS rects. Multiple overlapping
 * and vertically-disjoint rects produce NON-RECTANGULAR normal forms (multiple
 * spans per scanline + vertical holes) -- exactly the case ad-hoc per-pixel
 * clipping mishandles. Built via region_from_rects (the engine's union path). */
enum { MAX_RECTS = 5 };
static void gen_clip(rgn_store_t *out)
{
    store_attach(out);
    int n = rnd(1, MAX_RECTS);
    rgn_rect_t rects[MAX_RECTS];
    for (int i = 0; i < n; i++) {
        int t = rnd(0, GH - 1), l = rnd(0, GW - 1);
        int b = rnd(t + 1, GH), r = rnd(l + 1, GW);
        rects[i].top = (int16_t)t; rects[i].left = (int16_t)l;
        rects[i].bottom = (int16_t)b; rects[i].right = (int16_t)r;
    }
    region_from_rects(&out->r, rects, (uint16_t)n);
}

/* ===========================================================================
 * 8bpp destination harness. The surface value for 8bpp is the low byte, so the
 * expected buffer is trivially the byte values. PREFILL is the untouched
 * sentinel; INK is the fill/copy value -- distinct, so a stray write shows.
 * ===========================================================================*/
enum { PREFILL8 = 0x11u, INK8 = 0xA5u };

static void mk_dst8(bitmap_t *bm, uint8_t *buf)
{
    memset(bm, 0, sizeof *bm);
    bm->base = buf;
    bm->bpp = 8u;
    bm->bytes_per_pixel = 1u;
    bm->width = GW;
    bm->height = GH;
    bm->pitch = GW;          /* tight 8bpp pitch */
    memset(buf, PREFILL8, (size_t)GW * GH);
}

/* Expected 8bpp buffer for a FILL of `rect` clipped to clip-membership `cm`. */
static void expect_fill8(uint8_t *exp, rgn_rect_t rect, const cbm_t *cm,
                         int use_clip)
{
    memset(exp, PREFILL8, (size_t)GW * GH);
    for (int y = rect.top; y < rect.bottom; y++) {
        if (y < 0 || y >= GH) continue;
        for (int x = rect.left; x < rect.right; x++) {
            if (x < 0 || x >= GW) continue;
            if (!use_clip || cbm_get(cm, x, y)) exp[y * GW + x] = INK8;
        }
    }
}

/* ===========================================================================
 * 32bpp destination harness (the other surface_put_pixel branch). The surface
 * stores a dword per pixel; we read it back via surface_get_pixel for the diff.
 * ===========================================================================*/
enum { PREFILL32 = 0x00112233u, INK32 = 0x00ABCDEFu };

static void mk_dst32(bitmap_t *bm, uint8_t *buf)
{
    memset(bm, 0, sizeof *bm);
    bm->base = buf;
    bm->bpp = 32u;
    bm->bytes_per_pixel = 4u;
    bm->width = GW;
    bm->height = GH;
    bm->pitch = GW * 4u;     /* tight 32bpp pitch */
    for (int i = 0; i < GW * GH; i++) ((uint32_t *)buf)[i] = PREFILL32;
}

static uint32_t get32(const bitmap_t *bm, int x, int y)
{
    return surface_get_pixel(bm, (uint32_t)y * bm->pitch +
                                 (uint32_t)x * bm->bytes_per_pixel);
}

/* Does the 32bpp dst match the expected (rect INTERSECT clip => INK32)? */
static int dst32_matches_fill(const bitmap_t *bm, rgn_rect_t rect,
                              const cbm_t *cm, int use_clip)
{
    for (int y = 0; y < GH; y++)
        for (int x = 0; x < GW; x++) {
            int want_ink = (y >= rect.top && y < rect.bottom &&
                            x >= rect.left && x < rect.right &&
                            (!use_clip || cbm_get(cm, x, y)));
            uint32_t want = want_ink ? INK32 : PREFILL32;
            if (get32(bm, x, y) != want) return 0;
        }
    return 1;
}

/* ===========================================================================
 * Copy-blit harness (8bpp): a source block whose pixels vary per cell, so a
 * misaligned or wrong copy is caught (not just a uniform fill). The expected
 * dst takes src[(y-y0)*w + (x-x0)] where (x,y) is in the dst rect AND in clip.
 * ===========================================================================*/
static void expect_blit8(uint8_t *exp, const uint32_t *src, int sx0, int sy0,
                         int w, int h, const cbm_t *cm, int use_clip)
{
    memset(exp, PREFILL8, (size_t)GW * GH);
    for (int dy = sy0; dy < sy0 + h; dy++) {
        if (dy < 0 || dy >= GH) continue;
        for (int dx = sx0; dx < sx0 + w; dx++) {
            if (dx < 0 || dx >= GW) continue;
            if (!use_clip || cbm_get(cm, dx, dy)) {
                uint32_t sp = src[(dy - sy0) * w + (dx - sx0)];
                exp[dy * GW + dx] = (uint8_t)(sp & 0xFFu);
            }
        }
    }
}

/* A random rect over the grid (half-open; non-empty). */
static rgn_rect_t gen_rect(void)
{
    int t = rnd(0, GH - 1), l = rnd(0, GW - 1);
    int b = rnd(t + 1, GH), r = rnd(l + 1, GW);
    rgn_rect_t rc = { (int16_t)t, (int16_t)l, (int16_t)b, (int16_t)r };
    return rc;
}

/* ===========================================================================
 * MAIN -- the suite.
 * ===========================================================================*/
int main(void)
{
    /* ---- directed: FULL clip (clip == whole frame) == unclipped fill ------ */
    {
        static rgn_store_t C; cbm_t cm;
        rgn_rect_t frame = { 0, 0, GH, GW };
        store_attach(&C); region_set_rect(&C.r, frame);
        rasterize_clip(&C.r, &cm);

        uint8_t dst[GW * GH], exp[GW * GH], unclipped[GW * GH];
        bitmap_t bm;
        rgn_rect_t rect = { 4, 6, 20, 30 };

        mk_dst8(&bm, dst);
        blitter_fill_rect_clipped(&bm, rect, INK8, &C.r);
        expect_fill8(exp, rect, &cm, 1);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "full-clip fill == rect-only fill");

        /* and a NULL clip == full frame */
        mk_dst8(&bm, unclipped);
        blitter_fill_rect_clipped(&bm, rect, INK8, 0);
        CHECK(memcmp(unclipped, exp, sizeof exp) == 0,
              "NULL clip == unclipped fill (== full-frame clip)");
    }

    /* ---- directed: EMPTY clip => nothing drawn --------------------------- */
    {
        static rgn_store_t C;
        store_attach(&C); region_set_empty(&C.r);
        uint8_t dst[GW * GH], exp[GW * GH];
        bitmap_t bm; rgn_rect_t rect = { 2, 2, 28, 36 };
        mk_dst8(&bm, dst);
        memset(exp, PREFILL8, sizeof exp);
        blitter_fill_rect_clipped(&bm, rect, INK8, &C.r);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "empty clip => nothing written (all prefill)");
    }

    /* ---- directed: NON-RECTANGULAR clip (two spans on a scanline + a hole)
     * The case ad-hoc per-pixel/single-span clipping gets wrong. Two disjoint
     * vertical rects whose rows overlap => some scanlines have TWO inside spans;
     * the band between them is a vertical hole. ------------------------------ */
    {
        static rgn_store_t C; cbm_t cm;
        rgn_rect_t two[2] = {
            { 4,  3, 26, 12 },   /* left column   */
            { 8, 22, 30, 33 }    /* right column, offset vertically => hole + 2-span band */
        };
        store_attach(&C); region_from_rects(&C.r, two, 2);
        rasterize_clip(&C.r, &cm);
        /* sanity: there IS a scanline with two spans (rows 8..26 cover both). */
        int multi = 0;
        for (int y = 8; y < 26; y++)
            if (cbm_get(&cm, 5, y) && cbm_get(&cm, 25, y) && !cbm_get(&cm, 17, y))
                multi = 1;
        CHECK(multi, "fixture: non-rectangular clip has a 2-span scanline + hole");

        uint8_t dst[GW * GH], exp[GW * GH];
        bitmap_t bm; rgn_rect_t rect = { 0, 0, GH, GW }; /* fill everything */
        mk_dst8(&bm, dst);
        blitter_fill_rect_clipped(&bm, rect, INK8, &C.r);
        expect_fill8(exp, rect, &cm, 1);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "non-rectangular clip fill == rasterized-clip membership");
    }

    /* ======================================================================
     * PRIMARY: the IFF property, thousands of random (clip, rect) pairs, 8bpp
     * FILL. A blitter pixel is changed IFF it is in the rect AND in the clip.
     * Buffer A = real blitter; buffer B = independent rasterize+rect+src.
     * ====================================================================== */
    {
        enum { CASES = 4000 };
        int bad = 0;
        for (int t = 0; t < CASES && !bad; t++) {
            static rgn_store_t C; cbm_t cm;
            gen_clip(&C);
            rasterize_clip(&C.r, &cm);
            rgn_rect_t rect = gen_rect();

            uint8_t dst[GW * GH], exp[GW * GH];
            bitmap_t bm; mk_dst8(&bm, dst);
            blitter_fill_rect_clipped(&bm, rect, INK8, &C.r);
            expect_fill8(exp, rect, &cm, 1);
            if (memcmp(dst, exp, sizeof dst) != 0) {
                fprintf(stderr,
                  "  FILL-IFF FAIL case=%d rect=[t%d l%d b%d r%d]\n",
                  t, rect.top, rect.left, rect.bottom, rect.right);
                bad = 1;
            }
        }
        CHECK(!bad, "8bpp fill: pixel changed IFF in rect AND in clip (4000 cases)");
    }

    /* ======================================================================
     * 32bpp FILL: the other surface_put_pixel branch, same IFF property.
     * ====================================================================== */
    {
        enum { CASES = 2000 };
        int bad = 0;
        for (int t = 0; t < CASES && !bad; t++) {
            static rgn_store_t C; cbm_t cm;
            gen_clip(&C);
            rasterize_clip(&C.r, &cm);
            rgn_rect_t rect = gen_rect();

            uint8_t dst[GW * GH * 4]; bitmap_t bm; mk_dst32(&bm, dst);
            blitter_fill_rect_clipped(&bm, rect, INK32, &C.r);
            if (!dst32_matches_fill(&bm, rect, &cm, 1)) {
                fprintf(stderr, "  FILL32-IFF FAIL case=%d\n", t);
                bad = 1;
            }
        }
        CHECK(!bad, "32bpp fill: pixel changed IFF in rect AND in clip (2000 cases)");
    }

    /* ======================================================================
     * COPY-BLIT (8bpp): a varying source block; pixel copied IFF in dst rect
     * AND in clip. Buffer A = real blitter_blit_clipped; buffer B independent.
     * ====================================================================== */
    {
        enum { CASES = 3000 };
        int bad = 0;
        for (int t = 0; t < CASES && !bad; t++) {
            static rgn_store_t C; cbm_t cm;
            gen_clip(&C);
            rasterize_clip(&C.r, &cm);

            /* random destination block (top-left + size, kept within grid) */
            int x0 = rnd(0, GW - 1), y0 = rnd(0, GH - 1);
            int w  = rnd(1, GW - x0), h = rnd(1, GH - y0);

            /* varying source: each cell a distinct, non-prefill, non-zero byte */
            static uint32_t src[GW * GH];
            for (int i = 0; i < w * h; i++)
                src[i] = (uint32_t)(((i * 7 + 3) & 0x7Fu) | 0x80u); /* != PREFILL8 */

            uint8_t dst[GW * GH], exp[GW * GH];
            bitmap_t bm; mk_dst8(&bm, dst);
            blitter_blit_clipped(&bm, src, (uint32_t)w,
                                 (int32_t)x0, (int32_t)y0,
                                 (uint32_t)w, (uint32_t)h, &C.r);
            expect_blit8(exp, src, x0, y0, w, h, &cm, 1);
            if (memcmp(dst, exp, sizeof dst) != 0) {
                fprintf(stderr,
                  "  BLIT-IFF FAIL case=%d at (%d,%d) %dx%d\n", t, x0, y0, w, h);
                bad = 1;
            }
        }
        CHECK(!bad, "8bpp copy-blit: pixel copied IFF in dst rect AND in clip (3000)");
    }

    /* ======================================================================
     * COPY-BLIT off-grid: negative origin + extent past the right/bottom edge,
     * proving bounds-checking (no write past the dst buffer, Rule 2) AND the
     * clip IFF still holds for the on-grid intersection.
     * ====================================================================== */
    {
        static rgn_store_t C; cbm_t cm;
        rgn_rect_t frame = { 0, 0, GH, GW };
        store_attach(&C); region_set_rect(&C.r, frame);     /* full clip */
        rasterize_clip(&C.r, &cm);

        int w = 12, h = 10;
        static uint32_t src[GW * GH];
        for (int i = 0; i < w * h; i++) src[i] = (uint32_t)((i & 0x7Fu) | 0x80u);

        uint8_t dst[GW * GH], exp[GW * GH];
        bitmap_t bm; mk_dst8(&bm, dst);
        /* origin partly off the top-left */
        blitter_blit_clipped(&bm, src, (uint32_t)w, -4, -3,
                             (uint32_t)w, (uint32_t)h, &C.r);
        expect_blit8(exp, src, -4, -3, w, h, &cm, 1);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "copy-blit clipped at top-left edge (no underflow write)");

        /* origin partly off the bottom-right */
        mk_dst8(&bm, dst);
        blitter_blit_clipped(&bm, src, (uint32_t)w,
                             (int32_t)(GW - 5), (int32_t)(GH - 4),
                             (uint32_t)w, (uint32_t)h, &C.r);
        expect_blit8(exp, src, GW - 5, GH - 4, w, h, &cm, 1);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "copy-blit clipped at bottom-right edge (no overflow write)");
    }

    /* ======================================================================
     * Negative-coordinate FILL: rect.left/top negative; the on-grid intersection
     * with the clip is filled, nothing off-grid is written.
     * ====================================================================== */
    {
        static rgn_store_t C; cbm_t cm;
        gen_clip(&C); rasterize_clip(&C.r, &cm);
        uint8_t dst[GW * GH], exp[GW * GH];
        bitmap_t bm; mk_dst8(&bm, dst);
        rgn_rect_t rect = { -5, -7, 18, 25 };
        blitter_fill_rect_clipped(&bm, rect, INK8, &C.r);
        expect_fill8(exp, rect, &cm, 1);
        CHECK(memcmp(dst, exp, sizeof dst) == 0,
              "fill with negative origin clips to grid + clip region");
    }

    return TEST_SUMMARY("test_blitter");
}

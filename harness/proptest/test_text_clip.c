/*
 * harness/proptest/test_text_clip.c -- text never paints outside its clip
 * (bead initech-tdnl.68, Font Manager slice 2).
 *
 * THE PROPERTY (grafport.h CLIPPING INVARIANT, "ALL DRAWING IS CLIPPED BY
 * visRgn INTERSECT clipRgn"; blitter.h THE CLIP RULE): for every pixel P,
 *     clipped_run(P) == (P in A AND P in B AND P in box) ? free_run(P) : untouched
 * where free_run is the SAME text_draw_run call with no clip. So the clip may
 * only REMOVE pixels, never move or add one, and nothing lands outside it --
 * the opaque box, the styled ink (italic overhangs past the box included) and
 * the underline alike.
 *
 * INDEPENDENCE (Law 2): membership in A and B is computed HERE by rasterizing
 * the regions straight off their row / inversion lists (the test_blitter.c /
 * test_region.c idiom), never through region_contains_point, which is what the
 * artifact uses. The regions are random unions / differences of rectangles
 * built with the ATKINSON engine (seeded LCG, Rule 11), often non-rectangular,
 * with holes; strings, fonts, faces, pen positions (partly off the bitmap
 * included) and the box are random too.
 *
 * Mutant (Rule 6): TEXT_MUT_CLIP_IGNORE (text_draw_run ignores the regions and
 * the box) must go RED. FACTORY CODE (hosted C, Law 3). ASCII-clean (Rule 12).
 */
#include "../oracle_policy.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "text.h"
#include "region_algebra.h"

static int g_checks, g_fails;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fails++; \
    if (g_fails <= 20) { printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } } } while (0)

enum { W = 160, H = 48 };
#define SENT 0x00123456u

typedef struct rgn_store {
    region_t  r;
    rgn_row_t rows[RGN_ROWS_CAP];
    int16_t   pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows = s->rows;
    s->r.cap_rows = RGN_ROWS_CAP;
    s->r.x_pool = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

/* membership straight off the row / x lists (NOT region_contains_point) */
static void rasterize(const region_t *r, uint8_t in[H][W])
{
    memset(in, 0, (size_t)H * W);
    if (r->is_empty || r->n_rows == 0) return;
    for (uint16_t i = 0; i < r->n_rows; i++) {
        int y0 = r->rows[i].y_top;
        int y1 = (i + 1 < r->n_rows) ? r->rows[i + 1].y_top : y0;
        for (int y = y0; y < y1; y++) {
            if (y < 0 || y >= H) continue;
            for (uint16_t k = 0; k + 1 < r->rows[i].x_count; k += 2)
                for (int x = r->rows[i].x[k]; x < r->rows[i].x[k + 1]; x++)
                    if (x >= 0 && x < W) in[y][x] = 1;
        }
    }
}

static uint32_t g_seed = 0x7E47C1u;
static int rnd(int lo, int hi)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return lo + (int)(((g_seed >> 16) & 0x7FFFu) % (uint32_t)(hi - lo + 1));
}

static void random_region(rgn_store_t *out)
{
    static rgn_store_t a, t;
    int n = rnd(1, 4);
    store_attach(out);
    for (int k = 0; k < n; k++) {
        rgn_rect_t rc;
        rc.left = (int16_t)rnd(-10, W - 5);
        rc.top = (int16_t)rnd(-10, H - 4);
        rc.right = (int16_t)(rc.left + rnd(2, 90));
        rc.bottom = (int16_t)(rc.top + rnd(2, 30));
        store_attach(&a);
        region_set_rect(&a.r, rc);
        store_attach(&t);
        region_op(&t.r, &out->r, &a.r, (k > 0 && rnd(0, 2) == 0) ? RGN_OP_DIFF : RGN_OP_UNION);
        store_attach(out);
        region_op(&out->r, &t.r, &t.r, RGN_OP_UNION);       /* out := t */
    }
}

static uint32_t g_free[W * H], g_clip[W * H];
static uint8_t g_ina[H][W], g_inb[H][W];

static void bm_on(bitmap_t *bm, uint32_t *px)
{
    for (int i = 0; i < W * H; i++) px[i] = SENT;
    bm->base = (volatile uint8_t *)px;
    bm->pitch = W * 4u;
    bm->bpp = 32u;
    bm->bytes_per_pixel = 4u;
    bm->width = W;
    bm->height = H;
}

int main(void)
{
    static const char *strs[] = { "Saving tables to disk", "gyp jq", "Hello",
        "PC LOAD LETTER", "Ww Mm", "\xC9", "Bold Italic Underline", "j" };
    static rgn_store_t ra, rb;
    long clipped = 0, kept = 0, overhang_clipped = 0;
    printf(">>> test-text-clip: text_draw_run writes only inside rgnA ^ rgnB ^ box (bead initech-tdnl.68)\n");
    for (int iter = 0; iter < 3000; iter++) {
        bitmap_t bf, bc;
        text_clip_t k;
        const char *s = strs[rnd(0, (int)(sizeof strs / sizeof strs[0]) - 1)];
        text_font_t font = (text_font_t)rnd(0, 1);
        unsigned face = (unsigned)rnd(0, 7);
        int x = rnd(-20, W - 10), y = rnd(-8, H - 6);
        int use_a = rnd(0, 3) != 0, use_b = rnd(0, 2) == 0, use_box = rnd(0, 1);
        int bad = 0;

        random_region(&ra);
        random_region(&rb);
        rasterize(&ra.r, g_ina);
        rasterize(&rb.r, g_inb);
        k.rgn_a = use_a ? &ra.r : 0;
        k.rgn_b = use_b ? &rb.r : 0;
        if (use_box) {
            k.left = rnd(-5, W - 20); k.top = rnd(-5, H - 8);
            k.right = k.left + rnd(1, 120); k.bottom = k.top + rnd(1, 30);
        } else {
            k.left = k.top = k.right = k.bottom = 0;
        }
        bm_on(&bf, g_free);
        bm_on(&bc, g_clip);
        text_draw_run(&bf, x, y, s, font, face, 0x000000u, 0xFFFFFFu, 0);
        text_draw_run(&bc, x, y, s, font, face, 0x000000u, 0xFFFFFFu, &k);
        for (int py = 0; py < H; py++)
            for (int px = 0; px < W; px++) {
                int in = (!use_a || g_ina[py][px]) && (!use_b || g_inb[py][px]) &&
                         (!use_box || (px >= k.left && px < k.right &&
                                       py >= k.top && py < k.bottom));
                uint32_t f = g_free[py * W + px], c = g_clip[py * W + px];
                uint32_t want = in ? f : SENT;
                if (c != want) bad++;
                if (f != SENT) {
                    if (in) kept++;
                    else {
                        clipped++;
                        /* ink painted outside the run's own box (an overhang) */
                        if (f == 0u && (px < x || py < y)) overhang_clipped++;
                    }
                }
            }
        CHECK(bad == 0, "iter %d \"%s\" font %d face %u at (%d,%d) A=%d B=%d box=%d:"
              " %d px outside the rule", iter, s, font, face, x, y, use_a, use_b,
              use_box, bad);
    }
    /* the property was exercised, not vacuous */
    CHECK(clipped > 10000 && kept > 10000, "coverage: %ld px clipped, %ld kept", clipped, kept);
    CHECK(overhang_clipped > 0, "coverage: no italic overhang pixel was ever clipped");
    printf("    %ld run px removed by the clip, %ld kept, %ld overhang ink px clipped\n",
           clipped, kept, overhang_clipped);
    printf(">>> test-text-clip: %d checks, %d failed\n", g_checks, g_fails);
    if (g_fails) { printf("!!! test-text-clip FAIL\n"); return 1; }
    printf(">>> test-text-clip: green\n");
    return 0;
}

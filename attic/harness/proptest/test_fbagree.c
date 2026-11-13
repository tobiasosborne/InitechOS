/* harness/proptest/test_fbagree.c -- fb-agree oracle (FO-1 / AM-2).
 *
 * beads: initech-k8o5. Ref: ADR-0004 D-2 ("exactly one surface module; no second
 *        pixel writer"); ADR-0004 FO-1 ("fb-agree gate must be GREEN and
 *        MUTATION-PROVEN before any Manager code"); ADR-0004 AM-2 ("fb-agree is
 *        a hard pass/fail gate in the oracle vector").
 * Ref:   CLAUDE.md Law 2 (oracle is truth; never false-green), Rule 1 (RED->GREEN
 *        -> refactor), Rule 6 (mutation-proven: at least one named mutant must
 *        drive the oracle RED, then restoring it drives it GREEN), Rule 11
 *        (seeded LCG -- reproducible; no timestamps), Rule 12 (ASCII-clean).
 *
 * ONE-SURFACE INVARIANT PROOF: render identical content two ways into two equal-
 * sized host framebuffers, then assert BYTE-IDENTICAL output.
 *   Path A: via the console API (console_init / console_draw_glyph / console_putc
 *           / console_puts).
 *   Path B: via surface primitives directly (surface_blit / surface_fill_span
 *           / surface_put_pixel).
 * If the two framebuffers agree byte-for-byte then the console's pixel writes and
 * the surface primitives are functionally identical -- the one-surface invariant
 * holds. Any divergence (wrong byte order, off-by-one, wrong bpp branch) is
 * detected immediately.
 *
 * NAMED MUTANT (Rule 6 proof): define FBAGREE_MUTATE_SECOND_PATH at compile
 * time to inject a one-byte offset error in the surface_fill_span writer used
 * on path B. This MUST drive the oracle RED (the two buffers disagree). See
 * the `#ifndef FBAGREE_MUTATE_SECOND_PATH` guard around the fill in path B's
 * clear step. The Makefile recipe for `test-fbagree-mutant` adds
 * -DFBAGREE_MUTATE_SECOND_PATH; the normal `test-fbagree` recipe omits it.
 *
 * Tests (across bpp 8, 24, 32):
 *   - fb-agree-clear: both paths produce an all-bg framebuffer after init/clear.
 *   - fb-agree-glyph: console_draw_glyph agrees with surface_blit on a solid
 *       glyph, an empty glyph, and a left-only glyph (the MSB-left bit).
 *   - fb-agree-puts: console_puts on a short string agrees with the equivalent
 *       surface_blit sequence (glyph by glyph).
 *   - fb-agree-span: surface_fill_span agrees with a pixel-by-pixel put_pixel
 *       loop on a random-color horizontal run (seeded LCG ensures reproducibility).
 *   - fb-agree-scroll: both paths produce identical bytes after a scroll (sentinel
 *       row moves up one CONSOLE_CELL_H band, last row cleared to bg).
 *
 * FACTORY host test (CLAUDE.md Law 3: factory is C, libc OK on the host).
 * Uses the TEST_HARNESS()/CHECK/TEST_SUMMARY idiom from seed/test_assert.h.
 * ASCII-clean (Rule 12). No timestamps (Rule 11).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "test_assert.h"      /* -Iseed                                    */
#include "console.h"          /* -Ios/milton                               */
#include "../os/flair/surface.h"  /* -I. (project root)                   */

TEST_HARNESS();

/* ---- Seeded LCG (Rule 11 -- reproducible fuzz) --------------------------- */
static uint32_t g_seed = 0xFBA9EE7u;  /* named constant (FO-1 seed); not 0 */
static uint32_t lcg_next(void)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    return g_seed;
}

/* ---- Fake framebuffer allocation (MAP_32BIT, matching test_console.c) ---- */
#define FB_W 640
#define FB_H 480

static void *alloc_low(size_t n)
{
    void *p = MAP_FAILED;
#ifdef MAP_32BIT
    p = mmap(NULL, n, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
#endif
    if (p == MAP_FAILED) return NULL;
    if ((uintptr_t)p > 0xFFFFFFFFu) { munmap(p, n); return NULL; }
    return p;
}

/* ---- Synthetic glyphs (identical to test_console.c) ---------------------- */
#define GLYPH_SOLID  'S'
#define GLYPH_EMPTY  'E'
#define GLYPH_LEFT   'L'

static void build_font(uint8_t font[CONSOLE_FONT_BYTES])
{
    memset(font, 0, CONSOLE_FONT_BYTES);
    for (int r = 0; r < CONSOLE_CELL_H; r++) {
        font[(unsigned)GLYPH_SOLID * CONSOLE_CELL_H + r] = 0xFF;
        font[(unsigned)GLYPH_EMPTY * CONSOLE_CELL_H + r] = 0x00;
        font[(unsigned)GLYPH_LEFT  * CONSOLE_CELL_H + r] = 0x80;
    }
}

/* ---- make_bi: boot_info pointing at a host buffer ----------------------- */
static boot_info_t make_bi(void *fb, uint32_t bpp, void *font)
{
    boot_info_t bi;
    bi.lfb_addr   = (uint32_t)(uintptr_t)fb;
    bi.lfb_bpp    = bpp;
    bi.lfb_pitch  = (uint32_t)FB_W * (bpp / 8u);
    bi.lfb_width  = FB_W;
    bi.lfb_height = FB_H;
    bi.font_addr  = (uint32_t)(uintptr_t)font;
    return bi;
}

/* ---- make_bm: bitmap_t over a raw host buffer ---------------------------- */
static bitmap_t make_bm(uint8_t *buf, uint32_t bpp)
{
    bitmap_t bm;
    bm.base            = (volatile uint8_t *)buf;
    bm.pitch           = (uint32_t)FB_W * (bpp / 8u);
    bm.bpp             = bpp;
    bm.bytes_per_pixel = bpp / 8u;
    bm.width           = FB_W;
    bm.height          = FB_H;
    return bm;
}

/* ---- fb_bytes_equal: compare two raw framebuffer byte arrays ------------- */
static int fb_bytes_equal(const uint8_t *a, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return 0;
    }
    return 1;
}

/* ===========================================================================
 * run_bpp: run the full fb-agree suite for one bpp value.
 * =========================================================================*/
static void run_bpp(uint8_t *font, uint32_t bpp)
{
    size_t bpx      = bpp / 8u;
    size_t fb_bytes = (size_t)FB_W * FB_H * bpx;
    char   label[128];

    uint8_t *fb_a = alloc_low(fb_bytes);  /* path A: console API  */
    uint8_t *fb_b = alloc_low(fb_bytes);  /* path B: surface direct */
    snprintf(label, sizeof label, "alloc fb_a %ubpp", bpp);
    CHECK(fb_a != NULL, label);
    snprintf(label, sizeof label, "alloc fb_b %ubpp", bpp);
    CHECK(fb_b != NULL, label);
    if (!fb_a || !fb_b) return;

    /* ------------------------------------------------------------------ */
    /* fb-agree-clear: after init (path A) vs after surface clear (path B) */
    /* ------------------------------------------------------------------ */
    boot_info_t bi = make_bi(fb_a, bpp, font);
    console_t con;
    int rc = console_init(&con, &bi);
    snprintf(label, sizeof label, "fb-agree-clear %ubpp: console_init OK", bpp);
    CHECK(rc == CONSOLE_OK, label);

    /* Path B: fill fb_b with bg using surface_fill_span (per row). */
    uint32_t bg = con.bg;
    uint32_t fg = con.fg;
    bitmap_t bm_b = make_bm(fb_b, bpp);

#ifndef FBAGREE_MUTATE_SECOND_PATH
    for (uint32_t y = 0; y < FB_H; y++) {
        surface_fill_span(&bm_b, 0u, y, FB_W, bg);
    }
#else
    /* MUTANT: perturb the fill on path B by writing bg+1 to one span,
     * causing a single-byte mismatch that the byte-compare MUST detect.
     * This proves the oracle bites (Rule 6: a golden that never caught
     * a regression is decoration). */
    for (uint32_t y = 0; y < FB_H; y++) {
        uint32_t color = (y == 0) ? (bg + 1u) : bg;  /* corrupt first row */
        surface_fill_span(&bm_b, 0u, y, FB_W, color);
    }
#endif

    snprintf(label, sizeof label, "fb-agree-clear %ubpp: both paths identical after clear", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    /* ------------------------------------------------------------------ */
    /* fb-agree-glyph: console_draw_glyph agrees with surface_blit        */
    /* ------------------------------------------------------------------ */
    /* Reset both buffers to bg. */
    memset(fb_a, 0, fb_bytes); console_clear(&con);
    memset(fb_b, 0, fb_bytes);
    for (uint32_t y = 0; y < FB_H; y++) {
        surface_fill_span(&bm_b, 0u, y, FB_W, bg);
    }

    /* Solid glyph at cell (3, 2). */
    const uint8_t *glyph_s = font + (uint32_t)GLYPH_SOLID * CONSOLE_CELL_H;
    const uint8_t *glyph_e = font + (uint32_t)GLYPH_EMPTY * CONSOLE_CELL_H;
    const uint8_t *glyph_l = font + (uint32_t)GLYPH_LEFT  * CONSOLE_CELL_H;

    /* Path A: console API. */
    console_draw_glyph(&con, 3u, 2u, GLYPH_SOLID, fg, bg);
    console_draw_glyph(&con, 5u, 4u, GLYPH_EMPTY, fg, bg);
    console_draw_glyph(&con, 7u, 6u, GLYPH_LEFT,  fg, bg);

    /* Path B: surface_blit directly. */
    surface_blit(&bm_b, 3u*CONSOLE_CELL_W, 2u*CONSOLE_CELL_H,
                 glyph_s, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);
    surface_blit(&bm_b, 5u*CONSOLE_CELL_W, 4u*CONSOLE_CELL_H,
                 glyph_e, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);
    surface_blit(&bm_b, 7u*CONSOLE_CELL_W, 6u*CONSOLE_CELL_H,
                 glyph_l, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);

    snprintf(label, sizeof label,
             "fb-agree-glyph %ubpp: console_draw_glyph == surface_blit (solid+empty+left)", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    /* ------------------------------------------------------------------ */
    /* fb-agree-puts: console_puts agrees with per-glyph surface_blit     */
    /* ------------------------------------------------------------------ */
    /* Clear both then write "SLE" (the three synthetic glyphs) via puts/blit. */
    console_clear(&con);
    memset(fb_b, 0, fb_bytes);
    for (uint32_t y = 0; y < FB_H; y++) {
        surface_fill_span(&bm_b, 0u, y, FB_W, bg);
    }
    /* Place cursor at origin. */
    con.cur_col = 0; con.cur_row = 0;

    /* Path A: console_puts writes three characters at cols 0,1,2 row 0. */
    const char str[4] = { (char)GLYPH_SOLID, (char)GLYPH_LEFT, (char)GLYPH_EMPTY, '\0' };
    console_puts(&con, str);

    /* Path B: the same three glyphs via surface_blit at the same pixel coords. */
    surface_blit(&bm_b, 0u*CONSOLE_CELL_W, 0u*CONSOLE_CELL_H,
                 glyph_s, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);
    surface_blit(&bm_b, 1u*CONSOLE_CELL_W, 0u*CONSOLE_CELL_H,
                 glyph_l, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);
    surface_blit(&bm_b, 2u*CONSOLE_CELL_W, 0u*CONSOLE_CELL_H,
                 glyph_e, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);

    snprintf(label, sizeof label,
             "fb-agree-puts %ubpp: console_puts agrees with surface_blit sequence", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    /* ------------------------------------------------------------------ */
    /* fb-agree-span: surface_fill_span agrees with pixel-by-pixel loop   */
    /* ------------------------------------------------------------------ */
    /* Use a fresh buffer pair cleared to bg. Fill row 5 from x=10 len=20
     * with a random packed color, two ways: fill_span vs put_pixel loop. */
    console_clear(&con);
    memset(fb_b, 0, fb_bytes);
    for (uint32_t y = 0; y < FB_H; y++) {
        surface_fill_span(&bm_b, 0u, y, FB_W, bg);
    }

    /* Pick a random packed color (LCG seeded above for reproducibility). */
    uint32_t span_color = lcg_next() & 0x00FFFFFFu;
    uint32_t span_x = 10u, span_y = 5u, span_len = 20u;

    /* Path A: surface_fill_span into fb_a via bitmap wrapper. */
    bitmap_t bm_a = make_bm(fb_a, bpp);
    surface_fill_span(&bm_a, span_x, span_y, span_len, span_color);

    /* Path B: explicit surface_put_pixel loop. */
    {
        uint32_t off = span_y * bm_b.pitch + span_x * bm_b.bytes_per_pixel;
        for (uint32_t i = 0; i < span_len; i++) {
            surface_put_pixel(&bm_b, off, span_color);
            off += bm_b.bytes_per_pixel;
        }
    }

    snprintf(label, sizeof label,
             "fb-agree-span %ubpp: surface_fill_span == pixel loop", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    /* ------------------------------------------------------------------ */
    /* fb-agree-scroll: after scroll, both paths byte-identical           */
    /* ------------------------------------------------------------------ */
    /* Write a solid-glyph sentinel at the last row (col 0), then scroll.
     * Both buffers must match after the scroll. */
    console_clear(&con);
    memset(fb_b, 0, fb_bytes);
    for (uint32_t y = 0; y < FB_H; y++) {
        surface_fill_span(&bm_b, 0u, y, FB_W, bg);
    }
    /* Both start from the same bg-cleared state. */
    /* Path A: draw sentinel at row 24 via console. */
    console_draw_glyph(&con, 0u, (uint32_t)(con.rows - 1u), GLYPH_SOLID, fg, bg);
    /* Path B: draw same sentinel via surface_blit. */
    surface_blit(&bm_b, 0u,
                 (con.rows - 1u) * CONSOLE_CELL_H,
                 glyph_s, CONSOLE_CELL_W, CONSOLE_CELL_H, fg, bg);

    snprintf(label, sizeof label,
             "fb-agree-scroll %ubpp: fb_a==fb_b after sentinel draw", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    /* Path A: console_scroll. Path B: replicate scroll by hand via surface. */
    console_scroll(&con);

    {
        /* Replicate scroll on bm_b: copy rows CELL_H..text_h-1 to 0..text_h-CELL_H-1,
         * then clear the freed bottom band. This is the same logic as console_scroll,
         * but exercised independently -- any deviation in the console path would show
         * up as a byte mismatch. */
        uint32_t text_h = con.rows * CONSOLE_CELL_H;
        if (text_h > FB_H) text_h = FB_H;
        uint32_t move_rows = (text_h >= CONSOLE_CELL_H) ? (text_h - CONSOLE_CELL_H) : 0u;
        for (uint32_t y = 0; y < move_rows; y++) {
            uint32_t dst = y * bm_b.pitch;
            uint32_t src = (y + CONSOLE_CELL_H) * bm_b.pitch;
            for (uint32_t x = 0; x < bm_b.width; x++) {
                uint32_t px = surface_get_pixel(&bm_b, src + x * bm_b.bytes_per_pixel);
                surface_put_pixel(&bm_b, dst + x * bm_b.bytes_per_pixel, px);
            }
        }
        for (uint32_t y = move_rows; y < text_h; y++) {
            surface_fill_span(&bm_b, 0u, y, bm_b.width, bg);
        }
    }

    snprintf(label, sizeof label,
             "fb-agree-scroll %ubpp: byte-identical after scroll", bpp);
    CHECK(fb_bytes_equal(fb_a, fb_b, fb_bytes), label);

    munmap(fb_a, fb_bytes);
    munmap(fb_b, fb_bytes);
}

/* ===========================================================================
 * main
 * =========================================================================*/
int main(void)
{
    uint8_t *font = alloc_low(CONSOLE_FONT_BYTES);
    CHECK(font != NULL, "alloc fake font in low 4 GiB (MAP_32BIT)");
    if (!font) return TEST_SUMMARY("test_fbagree");
    build_font(font);

    run_bpp(font, 32u);
    run_bpp(font, 24u);
    run_bpp(font, 8u);

    munmap(font, CONSOLE_FONT_BYTES);
    return TEST_SUMMARY("test_fbagree");
}

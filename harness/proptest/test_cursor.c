/*
 * test_cursor.c -- independent host oracle for FLAIR CursorMgr.
 *
 * beads: initech-tdnl.1 (GUI remediation R0.1).
 * Ref: PRD Sec 6.3 (FLAIR Event/Window Managers and event-trace oracles);
 *      docs/plans/GUI-remediation-plan.md R0.1 (16x16 CURS, hotspot,
 *      save-under, ShieldCursor); Inside Macintosh: Imaging With QuickDraw,
 *      cursor ('CURS') resource layout (16 data words, 16 mask words, MSB-left,
 *      then hotspot). CLAUDE.md Law 2 / Rules 1, 6, 11, and 12.
 *
 * INDEPENDENCE (Law 2): the canonical-position golden below has its own literal
 * strike and a deliberately trivial reference blitter. It never reads the
 * implementation tables and never calls CursorMgr to compute expected pixels.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cursor.h"
#include "test_assert.h"

TEST_HARNESS();

enum { FB_W = 48, FB_H = 40, CURS_DIM = 16 };

/* Independent copy of the clean-room R0.1 golden. '#' = black data bit.
 * The mask-only white outline is carried separately below. */
static const uint16_t golden_data[CURS_DIM] = {
    0x0000u, /* ................ */
    0x4000u, /* .#.............. */
    0x6000u, /* .##............. */
    0x7000u, /* .###............ */
    0x7800u, /* .####........... */
    0x7C00u, /* .#####.......... */
    0x7E00u, /* .######......... */
    0x7F00u, /* .#######........ */
    0x7F80u, /* .########....... */
    0x7FC0u, /* .#########...... */
    0x7C00u, /* .#####.......... */
    0x6C00u, /* .##.##.......... */
    0x4C00u, /* .#..##.......... */
    0x0C00u, /* ....##.......... */
    0x0C00u, /* ....##.......... */
    0x0000u  /* ................ */
};

static const uint16_t golden_mask[CURS_DIM] = {
    0xE000u, /* ooo............. */
    0xF000u, /* o#oo............ */
    0xF800u, /* o##oo........... */
    0xFC00u, /* o###oo.......... */
    0xFE00u, /* o####oo......... */
    0xFF00u, /* o#####oo........ */
    0xFF80u, /* o######oo....... */
    0xFFC0u, /* o#######oo...... */
    0xFFE0u, /* o########oo..... */
    0xFFE0u, /* o#########o..... */
    0xFFE0u, /* o#####ooooo..... */
    0xFE00u, /* o##o##o......... */
    0xFE00u, /* o#oo##o......... */
    0xFE00u, /* oooo##o......... */
    0x1E00u, /* ...o##o......... */
    0x1E00u  /* ...oooo......... */
};

static uint32_t rng_state = 0xC0750A11u;

static uint32_t rng_next(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static uint32_t bytes_per_pixel(uint32_t bpp)
{
    return bpp / 8u;
}

static void reference_put(uint8_t *fb, uint32_t pitch, uint32_t bpp,
                          int x, int y, int white)
{
    uint8_t *p = fb + (uint32_t)y * pitch +
                 (uint32_t)x * bytes_per_pixel(bpp);
    uint8_t v = white ? 0xFFu : 0x00u;

    if (bpp == 8u) {
        p[0] = white ? 1u : 0u;
    } else if (bpp == 24u) {
        p[0] = v; p[1] = v; p[2] = v;
    } else {
        p[0] = v; p[1] = v; p[2] = v; p[3] = 0u;
    }
}

/* Trivial independent CURS blitter: fixed 16x16 loops, literal hotspot (1,1),
 * mask 0 leaves the destination unchanged, data selects black vs white. */
static void reference_draw(uint8_t *fb, uint32_t pitch, uint32_t bpp,
                           int w, int h, int x, int y)
{
    int ox = x - 1;
    int oy = y - 1;
    for (int row = 0; row < CURS_DIM; row++) {
        for (int col = 0; col < CURS_DIM; col++) {
            int px = ox + col;
            int py = oy + row;
            uint16_t bit = (uint16_t)(0x8000u >> col);
            if (px < 0 || py < 0 || px >= w || py >= h ||
                !(golden_mask[row] & bit)) {
                continue;
            }
            reference_put(fb, pitch, bpp, px, py,
                          !(golden_data[row] & bit));
        }
    }
}

static void fill_random(uint8_t *fb, size_t n)
{
    for (size_t i = 0; i < n; i++) fb[i] = (uint8_t)rng_next();
}

static void property_show_hide(void)
{
    static const int edge_x[8] = { 0, FB_W - 1, 0, FB_W - 1,
                                   FB_W / 2, 0, FB_W - 1, FB_W / 2 };
    static const int edge_y[8] = { 0, 0, FB_H - 1, FB_H - 1,
                                   0, FB_H / 2, FB_H / 2, FB_H - 1 };

    for (int i = 0; i < 500; i++) {
        uint32_t bpp = (i % 3 == 0) ? 8u : (i % 3 == 1) ? 24u : 32u;
        uint32_t pitch = (uint32_t)FB_W * bytes_per_pixel(bpp) + 8u;
        size_t size = (size_t)pitch * FB_H;
        uint8_t *fb = (uint8_t *)malloc(size);
        uint8_t *before = (uint8_t *)malloc(size);
        int x = (i < 8) ? edge_x[i] : (int)(rng_next() % FB_W);
        int y = (i < 8) ? edge_y[i] : (int)(rng_next() % FB_H);

        if (!fb || !before) {
            fprintf(stderr, "test_cursor: OOM\n");
            exit(2);
        }
        fill_random(fb, size);
        memcpy(before, fb, size);
        flair_cursor_init(fb, pitch, FB_W, FB_H, bpp);
        flair_cursor_show(x, y);
        flair_cursor_hide();
        CHECK(memcmp(fb, before, size) == 0,
              "show+hide restores every framebuffer and padding byte");
        free(before);
        free(fb);
    }
}

static void canonical_golden_and_hotspot(void)
{
    const uint32_t bpp = 32u;
    const uint32_t pitch = FB_W * 4u + 8u;
    const size_t size = (size_t)pitch * FB_H;
    const int x = 18;
    const int y = 12;
    uint8_t *fb = (uint8_t *)malloc(size);
    uint8_t *expected = (uint8_t *)malloc(size);
    int min_x = FB_W, min_y = FB_H, max_x = -1, max_y = -1;

    if (!fb || !expected) exit(2);
    memset(fb, 0x5Au, size);
    memcpy(expected, fb, size);
    reference_draw(expected, pitch, bpp, FB_W, FB_H, x, y);

    flair_cursor_init(fb, pitch, FB_W, FB_H, bpp);
    flair_cursor_show(x, y);
    CHECK(memcmp(fb, expected, size) == 0,
          "CursorMgr matches the independent canonical-position golden");

    for (int py = 0; py < FB_H; py++) {
        for (int px = 0; px < FB_W; px++) {
            const uint8_t *a = fb + (size_t)py * pitch + (size_t)px * 4u;
            if (a[0] != 0x5Au || a[1] != 0x5Au ||
                a[2] != 0x5Au || a[3] != 0x5Au) {
                if (px < min_x) min_x = px;
                if (px > max_x) max_x = px;
                if (py < min_y) min_y = py;
                if (py > max_y) max_y = py;
            }
        }
    }
    CHECK(min_x == x - 1, "hotspot places the left mask edge at x-1");
    CHECK(max_x == x + 9, "hotspot places the right mask edge at x+9");
    CHECK(min_y == y - 1, "hotspot places the top mask edge at y-1");
    CHECK(max_y == y + 14, "hotspot places the bottom mask edge at y+14");

    flair_cursor_hide();
    free(expected);
    free(fb);
}

static void shield_nesting(void)
{
    const uint32_t bpp = 24u;
    const uint32_t pitch = FB_W * 3u + 8u;
    const size_t size = (size_t)pitch * FB_H;
    uint8_t *fb = (uint8_t *)malloc(size);
    uint8_t *base = (uint8_t *)malloc(size);
    uint8_t *shown = (uint8_t *)malloc(size);

    if (!fb || !base || !shown) exit(2);
    fill_random(fb, size);
    memcpy(base, fb, size);
    flair_cursor_init(fb, pitch, FB_W, FB_H, bpp);
    flair_cursor_show(20, 15);
    memcpy(shown, fb, size);

    flair_cursor_shield();
    CHECK(memcmp(fb, base, size) == 0, "first shield erases the cursor");
    flair_cursor_shield();
    CHECK(memcmp(fb, base, size) == 0, "nested shield remains erased");
    flair_cursor_unshield();
    CHECK(memcmp(fb, base, size) == 0,
          "first unshield does not redraw through the outer shield");
    flair_cursor_unshield();
    CHECK(memcmp(fb, shown, size) == 0,
          "final unshield redraws the cursor byte-identically");
    flair_cursor_hide();
    CHECK(memcmp(fb, base, size) == 0,
          "hide after nested shield cycle restores the original surface");

    free(shown);
    free(base);
    free(fb);
}

static void move_restores_old_site(void)
{
    const uint32_t bpp = 32u;
    const uint32_t pitch = FB_W * 4u + 8u;
    const size_t size = (size_t)pitch * FB_H;
    uint8_t *fb = (uint8_t *)malloc(size);
    uint8_t *base = (uint8_t *)malloc(size);
    uint8_t *expected = (uint8_t *)malloc(size);

    if (!fb || !base || !expected) exit(2);
    fill_random(fb, size);
    memcpy(base, fb, size);
    memcpy(expected, fb, size);
    reference_draw(expected, pitch, bpp, FB_W, FB_H, 32, 22);

    flair_cursor_init(fb, pitch, FB_W, FB_H, bpp);
    flair_cursor_show(8, 8);
    flair_cursor_move(32, 22);
    CHECK(memcmp(fb, expected, size) == 0,
          "move restores the old site and draws only at the new site");
    flair_cursor_hide();
    CHECK(memcmp(fb, base, size) == 0,
          "hide after move restores the whole buffer byte-identically");

    free(expected);
    free(base);
    free(fb);
}

int main(void)
{
    property_show_hide();
    canonical_golden_and_hotspot();
    shield_nesting();
    move_restores_old_site();
    return TEST_SUMMARY("test_cursor");
}

/*
 * harness/proptest/test_text.c -- oracle for os/flair/text.{h,c}.
 *
 * Ref: ADR-0004 D-7 ("proportional NFNT text measurement; text width = sum
 *      of per-glyph advances; no fixed-pitch assumption"); Rule 6 (golden
 *      files / oracles are mutation-proven); PRD Sec 6.4 (font resources).
 *      os/flair/text.h (text_measure, text_draw, text_center_in).
 *      spec/assets/geneva9.h (GENEVA9_ADVANCE[], per-glyph advance widths).
 *      spec/assets/chicago8x16.h (CHICAGO_CELL_W, fixed-cell advance).
 *      os/flair/surface.h (bitmap_t, surface_blit declaration).
 *      CLAUDE.md Rule 6 (mutation-proven: TEXT_MUTATE_FIXED_PITCH must
 *      drive proportional checks RED), Rule 12 (ASCII-clean), Rule 11
 *      (reproducible -- no nondeterminism in this harness).
 *
 * ASSERTIONS (all MUST be GREEN in normal build; MUST be RED under mutant):
 *
 *   1. PROPORTIONAL MEASUREMENT:
 *      a. advance('W') > advance('i')         -- W is wide, i is narrow
 *      b. text_measure("Hi") == adv('H') + adv('i')  -- exact sum of advances
 *      c. text_measure("")   == 0
 *      d. text_measure(NULL) == 0
 *
 *   2. PIXEL PLACEMENT:
 *      Blit "Hi" into a host framebuffer; verify that glyph pixels land at
 *      cumulative advance positions (x=0 for H, x=adv(H) for i).
 *
 *   3. CENTERING MATH:
 *      text_center_in(rect_w, str, font) == (rect_w - text_measure(font, str)) / 2
 *      text_center_in(rect_w, str, font) >= 0 when rect_w >= 0 (never negative)
 *      text_center_in with a wide string clamped to 0 (left-justified fallback)
 *
 * NAMED MUTANT -- TEXT_MUTATE_FIXED_PITCH (Rule 6):
 *   Compile with -DTEXT_MUTATE_FIXED_PITCH=1. text_measure uses a fixed
 *   6px advance for all glyphs regardless of font. The assertions in group 1
 *   and 2 MUST fail (non-zero exit). This proves the oracle catches fixed-
 *   pitch drift.
 *
 *   Build GREEN:  gcc ... harness/proptest/test_text.c os/flair/text.c -o /tmp/test_text
 *   Build MUTANT: gcc ... -DTEXT_MUTATE_FIXED_PITCH=1 ... (must exit non-zero)
 *
 * SURFACE STUBS:
 *   This test harness supplies its own surface_blit and surface_fill_span
 *   implementations that write to a host uint8_t buffer. These stubs mirror
 *   the surface.c logic (MSB=leftmost pixel, set=ink) so the pixel-placement
 *   assertions are a faithful test of text_draw's advance arithmetic.
 *
 * ASCII-clean (Rule 12). No nondeterminism (Rule 11). FLAIR_HOSTED=1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Pull in the text API (includes surface.h, chicago8x16.h, geneva9.h). */
#include "text.h"

/* ===========================
 * TEST HARNESS INFRASTRUCTURE
 * =========================== */

static int g_tests  = 0;
static int g_failed = 0;

#define CHECK(cond, msg)                                         \
    do {                                                          \
        ++g_tests;                                                \
        if (!(cond)) {                                            \
            ++g_failed;                                           \
            fprintf(stderr, "FAIL [%s:%d] %s\n",                 \
                    __FILE__, __LINE__, (msg));                   \
        }                                                         \
    } while (0)

/* ===========================
 * SURFACE STUBS
 * ===========================
 * surface_blit and surface_fill_span are DECLARED in surface.h but their
 * definitions live in os/flair/surface.c (not linked here to keep the test
 * build command simple as specified). These stubs provide a faithful
 * pixel-writing implementation for the test buffer.
 *
 * The stub uses the same MSB-first bit interpretation as surface.c:
 *   glyph[row] bit 0x80 = column 0 (leftmost); set bit = fg pixel.
 * This makes the pixel-placement assertions a true test of text_draw's
 * x-advance arithmetic.
 *
 * Ref: os/flair/surface.c (surface_blit inner loop; "MSB=0x80=leftmost
 * pixel, set bit = ink") -- boot-to-text-ground-truth.md Sec 1.2.
 */

void surface_fill_span(const bitmap_t *bm,
                       uint32_t x, uint32_t y,
                       uint32_t len, uint32_t px)
{
    uint32_t i;
    if (!bm || !bm->base) return;
    if (y >= bm->height || x >= bm->width) return;
    if (len > bm->width - x) len = bm->width - x;
    for (i = 0; i < len; i++) {
        uint32_t off = y * bm->pitch + (x + i) * bm->bytes_per_pixel;
        /* 8bpp stub: write low 8 bits of px as palette index. */
        bm->base[off] = (uint8_t)(px & 0xFFu);
    }
}

void surface_blit(const bitmap_t *bm,
                  uint32_t x0, uint32_t y0,
                  const uint8_t *glyph,
                  uint32_t cell_w, uint32_t cell_h,
                  uint32_t fg, uint32_t bg)
{
    uint32_t gy, gx;
    uint32_t rows, cols;

    if (!bm || !bm->base || !glyph) return;
    if (y0 >= bm->height || x0 >= bm->width) return;
    rows = cell_h;
    cols = cell_w;
    if (rows > bm->height - y0) rows = bm->height - y0;
    if (cols > bm->width  - x0) cols = bm->width  - x0;

    for (gy = 0; gy < rows; gy++) {
        uint8_t bits = glyph[gy];
        uint32_t off = (y0 + gy) * bm->pitch + x0 * bm->bytes_per_pixel;
        for (gx = 0; gx < cols; gx++) {
            uint8_t mask = (uint8_t)(0x80u >> gx);
            uint32_t color = (bits & mask) ? fg : bg;
            bm->base[off] = (uint8_t)(color & 0xFFu);
            off += bm->bytes_per_pixel;
        }
    }
}

/* ===========================
 * HELPER: bitmap on the stack
 * =========================== */

/*
 * make_bitmap -- fill in a bitmap_t descriptor for a caller-supplied buffer.
 * 8bpp, 1 byte per pixel, pitch == width.
 */
static void make_bitmap(bitmap_t *bm, volatile uint8_t *buf,
                        uint32_t w, uint32_t h)
{
    bm->base            = buf;
    bm->pitch           = w;
    bm->bpp             = 8;
    bm->bytes_per_pixel = 1;
    bm->width           = w;
    bm->height          = h;
}

/* ===========================
 * TEST GROUP 1: PROPORTIONAL MEASUREMENT
 * =========================== */

static void test_proportional_measure(void)
{
    int adv_W, adv_i, adv_H;
    int m_Hi, m_empty;

    /* Geneva 9 per-glyph advances (from spec/assets/geneva9.h).
     * Ref: ADR-0004 D-7; GENEVA9_ADVANCE[]; 'W' adv=9, 'i' adv=4, 'H' adv=7. */
    adv_W = (int)geneva9_advance_w('W');
    adv_i = (int)geneva9_advance_w('i');
    adv_H = (int)geneva9_advance_w('H');

    /* 1a. Width property: advance('W') > advance('i') -- W must be wider than i */
    CHECK(adv_W > adv_i,
          "PROPORTIONAL: advance('W') must be > advance('i') in Geneva 9");

    /* 1b. text_measure("Hi") == advance('H') + advance('i') (sum of per-glyph) */
    m_Hi = text_measure(FONT_GENEVA9, "Hi");
    CHECK(m_Hi == adv_H + adv_i,
          "PROPORTIONAL: text_measure(FONT_GENEVA9, \"Hi\") == adv(H) + adv(i)");

    /* 1c. Empty string -> 0 */
    m_empty = text_measure(FONT_GENEVA9, "");
    CHECK(m_empty == 0,
          "PROPORTIONAL: text_measure(FONT_GENEVA9, \"\") == 0");

    /* 1d. NULL -> 0 */
    CHECK(text_measure(FONT_GENEVA9, (const char *)0) == 0,
          "PROPORTIONAL: text_measure(FONT_GENEVA9, NULL) == 0");

    /* Also check Chicago (fixed cell): all advances == CHICAGO_CELL_W.
     * Chicago text_measure("Hi") == 2 * CHICAGO_CELL_W. */
    {
        int chi = text_measure(FONT_CHICAGO, "Hi");
        CHECK(chi == 2 * (int)CHICAGO_CELL_W,
              "CHICAGO: text_measure(\"Hi\") == 2 * CHICAGO_CELL_W");
    }

    /* Additional spot checks on Geneva proportionality:
     * 'l' (adv=3) < 'i' (adv=4) < 'a' (adv=6) < 'M' (adv=8) < 'W' (adv=9). */
    {
        int adv_l = (int)geneva9_advance_w('l');
        int adv_a = (int)geneva9_advance_w('a');
        int adv_M = (int)geneva9_advance_w('M');
        CHECK(adv_l < adv_i,   "PROPORTIONAL: adv('l') < adv('i')");
        CHECK(adv_i < adv_a,   "PROPORTIONAL: adv('i') < adv('a')");
        CHECK(adv_a < adv_M,   "PROPORTIONAL: adv('a') < adv('M')");
        CHECK(adv_M < adv_W,   "PROPORTIONAL: adv('M') < adv('W')");
    }

    /* text_measure sums correctly for a longer string. */
    {
        /* "Wi" = adv('W') + adv('i') */
        int m_Wi   = text_measure(FONT_GENEVA9, "Wi");
        int m_iW   = text_measure(FONT_GENEVA9, "iW");
        CHECK(m_Wi == adv_W + adv_i,
              "PROPORTIONAL: text_measure(\"Wi\") == adv(W) + adv(i)");
        CHECK(m_iW == adv_i + adv_W,
              "PROPORTIONAL: text_measure(\"iW\") == adv(i) + adv(W)");
        /* Order symmetry: same glyphs, same sum, different order. */
        CHECK(m_Wi == m_iW,
              "PROPORTIONAL: text_measure(\"Wi\") == text_measure(\"iW\")");
    }
}

/* ===========================
 * TEST GROUP 2: PIXEL PLACEMENT
 * =========================== */

/*
 * test_pixel_placement -- blit "Hi" (Geneva 9) into a host buffer and
 * verify that the glyph pixels land at the correct cumulative advance offsets.
 *
 * 'H' advance = 7 px, 'i' advance = 4 px (Geneva 9, spec/assets/geneva9.h).
 *
 * After blitting "Hi":
 *   - The 'H' glyph occupies columns [0, 7) in the buffer.
 *   - The 'i' glyph occupies columns [7, 11) in the buffer.
 *
 * We probe a specific row of each glyph to verify ink lands in the right column.
 * The probe row is chosen where the glyph has a known ink pixel.
 *
 * Ref: ADR-0004 D-7 (per-glyph advances drive x offset in text_draw);
 *      spec/assets/geneva9.h (H: row 4 = 0xF8 = bits 11111... at col 0-4;
 *                              i: row 3 = 0xC0 = bits 11...... at col 0-1).
 */
static void test_pixel_placement(void)
{
    /* Buffer large enough for "Hi" at Geneva 9: 7 + 4 = 11 px wide, 11 tall. */
    enum { BUF_W = 32, BUF_H = 16 };
    uint8_t backing[BUF_W * BUF_H];
    volatile uint8_t vbuf[BUF_W * BUF_H];
    bitmap_t bm;
    int adv_H;

    memset(backing, 0, sizeof(backing));
    memcpy((void *)vbuf, backing, sizeof(backing));

    make_bitmap(&bm, vbuf, BUF_W, BUF_H);

    /* fg=0xFF (ink), bg=0x00 (background) -- 8bpp palette-index style. */
    text_draw(&bm, 0, 0, "Hi", FONT_GENEVA9, 0xFFu, 0x00u);

    adv_H = (int)geneva9_advance_w('H');

    /*
     * Check 'H' glyph: row 4 of geneva9['H'-0x20] = 0xF8 = 1111 1000.
     * Bits: col0=1, col1=1, col2=1, col3=1, col4=1, col5=0, col6=0, col7=0.
     * At x=0, row=4 in the buffer: pixel (0,4) and (4,4) should be ink (0xFF);
     * pixel (5,4) should be background (0x00).
     * Ref: spec/assets/geneva9.h glyph 'H' row r4 = {0xF8} = ####### (col0-4).
     */
    {
        /* 'H' glyph row 4 in spec/assets/geneva9.h: 0xF8 = bits 11111000
         * => columns 0,1,2,3,4 are ink, columns 5,6,7 are background. */
        uint8_t pix_H_col0_r4 = (uint8_t)vbuf[4 * BUF_W + 0]; /* row4, col0 */
        uint8_t pix_H_col4_r4 = (uint8_t)vbuf[4 * BUF_W + 4]; /* row4, col4 */
        uint8_t pix_H_col5_r4 = (uint8_t)vbuf[4 * BUF_W + 5]; /* row4, col5 */
        CHECK(pix_H_col0_r4 == 0xFF,
              "PIXEL: 'H' col0 row4 should be ink (glyph row4 bit7=1)");
        CHECK(pix_H_col4_r4 == 0xFF,
              "PIXEL: 'H' col4 row4 should be ink (glyph row4 bit3=1)");
        CHECK(pix_H_col5_r4 == 0x00,
              "PIXEL: 'H' col5 row4 should be background (glyph row4 bit2=0)");
    }

    /*
     * Check 'i' glyph: starts at x=adv_H (=7).
     * spec/assets/geneva9.h glyph 'i' row 1: 0xC0 = bits 11000000
     * => columns 0,1 are ink (relative to glyph origin); rest background.
     * In the buffer: glyph origin = (adv_H, 0) = (7, 0).
     * pixel (7+0, 1) = ink; pixel (7+1, 1) = ink; pixel (7+2, 1) = background.
     */
    {
        uint8_t pix_i_col0_r1 = (uint8_t)vbuf[1 * BUF_W + adv_H + 0];
        uint8_t pix_i_col1_r1 = (uint8_t)vbuf[1 * BUF_W + adv_H + 1];
        uint8_t pix_i_col2_r1 = (uint8_t)vbuf[1 * BUF_W + adv_H + 2];
        CHECK(pix_i_col0_r1 == 0xFF,
              "PIXEL: 'i' col0 row1 (at x=adv_H) should be ink");
        CHECK(pix_i_col1_r1 == 0xFF,
              "PIXEL: 'i' col1 row1 (at x=adv_H+1) should be ink");
        CHECK(pix_i_col2_r1 == 0x00,
              "PIXEL: 'i' col2 row1 (at x=adv_H+2) should be background");
    }

    /*
     * Sanity: the 'H' glyph should NOT have drawn ink at x=adv_H (where 'i'
     * starts) on a row where the 'H' glyph is also background. Check row 0
     * (empty for both 'H' and 'i' in Geneva 9): all pixels in row 0 = bg.
     * spec/assets/geneva9.h: both 'H' and 'i' have row 0 = 0x00 (empty).
     */
    {
        int col;
        int all_bg = 1;
        for (col = 0; col < 12; col++) {
            if (vbuf[0 * BUF_W + col] != 0x00) { all_bg = 0; break; }
        }
        CHECK(all_bg, "PIXEL: row 0 should be all background (both glyphs have empty row 0)");
    }
}

/* ===========================
 * TEST GROUP 3: CENTERING MATH
 * =========================== */

static void test_centering(void)
{
    int rect_w, str_w, center_x;

    /* 3a. Centered: text_center_in(rect_w, str, font) == (rect_w - str_w) / 2 */
    rect_w  = 100;
    str_w   = text_measure(FONT_GENEVA9, "Hi");
    center_x = text_center_in(rect_w, "Hi", FONT_GENEVA9);
    CHECK(center_x == (rect_w - str_w) / 2,
          "CENTER: text_center_in(100, \"Hi\", GENEVA9) == (100 - str_w) / 2");

    /* 3b. Result is non-negative for any rect_w >= str_w */
    CHECK(center_x >= 0,
          "CENTER: text_center_in result must be >= 0 for rect_w >= str_w");

    /* 3c. Empty string: center = rect_w / 2 */
    center_x = text_center_in(rect_w, "", FONT_GENEVA9);
    CHECK(center_x == rect_w / 2,
          "CENTER: text_center_in(100, \"\", GENEVA9) == 50");

    /* 3d. NULL string: text_measure returns 0, so center = rect_w / 2 */
    center_x = text_center_in(rect_w, (const char *)0, FONT_GENEVA9);
    CHECK(center_x == rect_w / 2,
          "CENTER: text_center_in(100, NULL, GENEVA9) == 50");

    /* 3e. String wider than rect -> clamped to 0 (left-justified fallback) */
    center_x = text_center_in(1, "Wide string that is definitely wider", FONT_GENEVA9);
    CHECK(center_x == 0,
          "CENTER: text_center_in clamped to 0 when string > rect_w");

    /* 3f. rect_w == str_w -> center = 0 */
    str_w   = text_measure(FONT_CHICAGO, "X");
    center_x = text_center_in(str_w, "X", FONT_CHICAGO);
    CHECK(center_x == 0,
          "CENTER: text_center_in(str_w, \"X\", CHICAGO) == 0 when rect_w == str_w");

    /* 3g. Odd split: integer division truncates toward zero. */
    {
        /* Choose a rect_w where (rect_w - str_w) is odd. */
        int sw = text_measure(FONT_GENEVA9, "W"); /* adv=9 */
        /* rect_w = sw + 3 -> (3/2) = 1 */
        center_x = text_center_in(sw + 3, "W", FONT_GENEVA9);
        CHECK(center_x == 1,
              "CENTER: integer-division truncation: (sw+3-sw)/2 == 1");
    }

    /* 3h. Chicago centering: text_center_in(20, "X", CHICAGO) = (20-8)/2 = 6.
     * CHICAGO_CELL_W = 8. */
    {
        int chi_x = text_center_in(20, "X", FONT_CHICAGO);
        int expected = (20 - (int)CHICAGO_CELL_W) / 2;
        CHECK(chi_x == expected,
              "CENTER: text_center_in(20, \"X\", CHICAGO) == (20-8)/2 = 6");
    }
}

/* ===========================
 * MAIN
 * =========================== */

int main(void)
{
    printf("test_text: running proportional-text oracle...\n");

    test_proportional_measure();
    test_pixel_placement();
    test_centering();

    printf("test_text: %d checks, %d failure(s).\n", g_tests, g_failed);
    if (g_failed > 0) {
        fprintf(stderr, "test_text: ORACLE RED -- %d check(s) failed.\n",
                g_failed);
        return 1;
    }
    printf("test_text: ORACLE GREEN.\n");
    return 0;
}

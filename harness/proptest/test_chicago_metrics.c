/*
 * harness/proptest/test_chicago_metrics.c -- the INDEPENDENT Chicago 12 metrics
 * oracle (bead initech-tdnl.33, slice 1).
 *
 * WHAT IT GRADES: the artifact's Chicago text path (os/flair/text.h --
 * text_measure / text_draw / text_cell_height, the functions every Chicago
 * consumer in FLAIR calls) against the REAL System 7.0.1 Chicago 12 strike's
 * metrics, read at run time from the Apple resource itself:
 *     $(SYSTEM7_DECOMP)/goldens/resources/NFNT_5478.bin
 * NOT against the artifact's own advance table (Law 2 / ADR-0010 HER-02: an
 * oracle that computes its expected values from the table the artifact renders
 * from agrees by construction). Every expected number below is parsed here from
 * the NFNT bytes; this file includes text.h only to CALL the code under test.
 *
 * Ref: ADR-0004 D-7 ("Text width is the sum of per-glyph advances; no
 *      fixed-pitch assumption"); ../system7-decomp/specs/fonts/chicago.md
 *      ("FontRec header", "Advance-width table", "Verification recipe" steps 1
 *      and 3); specs/fonts/font-manager.md Sec 1.1 (owTable entry: high byte lb,
 *      low byte aw; 0xFFFF = missing), Sec 1.2 (owTable byte offset = 16 +
 *      2*owTLoc), Sec 3.2 (StringWidth = sum of aw), Sec 7.1 (ascent / descent /
 *      leading at header bytes 18 / 20 / 22).
 *
 * LEGS (each failure names the glyph / string and both numbers):
 *   A  header sanity: firstChar 0 <= 0x20, lastChar >= 0x7E, owTable in range.
 *   B  per-glyph ADVANCE, 0x20..0x7E: text_measure("c") == owTable aw(c).
 *   C  per-glyph DRAWN pen advance and LEFT BEARING: text_draw paints exactly
 *      the columns [x, x+aw) (the opaque cell IS the advance), its leftmost ink
 *      column is x + lb(c), and no ink falls outside the cell.
 *   D  STRING widths: text_measure(s) == sum of aw over s, for the strings the
 *      desktop actually shows (menu titles, items, window titles, tenant text),
 *      and text_draw's painted run for s spans exactly that many columns.
 *   E  VERTICAL metrics: text_cell_height == ascent + descent + leading; flat-
 *      bottomed glyphs end on the baseline row (ascent - 1); descenders reach
 *      the last descent row (ascent + descent - 1); no ink in the leading row.
 *   F  MISSING glyph: every code the real strike leaves undefined (owTable
 *      0xFFFF) measures the NFNT missing-glyph advance.
 *
 * Absent corpus: LOUD-SKIP, exit 0 (the test-clut pattern) -- EXCEPT when built
 * with -DCHICAGO_METRICS_REQUIRE_GOLDEN (the mutant target), where an absent
 * golden is a FAIL, so a missing corpus can never make the mutant look RED for
 * the wrong reason or the real gate look GREEN by skipping.
 *
 * FACTORY CODE (hosted C, Law 3). ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"

#ifndef SYSTEM7_DECOMP
#define SYSTEM7_DECOMP "../system7-decomp"
#endif
#define NFNT_PATH SYSTEM7_DECOMP "/goldens/resources/NFNT_5478.bin"

static int g_checks, g_fails;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fails++; \
    printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- the NFNT, parsed independently ------------------------------------- */
static unsigned char g_nfnt[8192];
static long g_nfnt_len;
static int g_first, g_last, g_ascent, g_descent, g_leading, g_ow_off;

static int be16s(long off)
{
    return (int)(int16_t)(((unsigned)g_nfnt[off] << 8) | g_nfnt[off + 1]);
}

/* owTable entry for code c: lb (high byte) and aw (low byte); returns 0 when
 * the entry is the 0xFFFF missing sentinel or c is outside the strike. */
static int nfnt_ow(int c, int *lb, int *aw)
{
    long e;
    if (c < g_first || c > g_last) return 0;
    e = g_ow_off + 2L * (c - g_first);
    if (g_nfnt[e] == 0xFF && g_nfnt[e + 1] == 0xFF) return 0;
    *lb = (int)(signed char)g_nfnt[e];
    *aw = (int)g_nfnt[e + 1];
    return 1;
}

static int nfnt_str_w(const char *s)
{
    int w = 0, lb, aw;
    for (; *s; s++) {
        if (!nfnt_ow((unsigned char)*s, &lb, &aw)) {
            printf("  FAIL: string glyph 0x%02X has no owTable entry\n",
                   (unsigned char)*s);
            g_fails++;
            return -1;
        }
        w += aw;
    }
    return w;
}

/* ---- a hosted 32bpp canvas --------------------------------------------- */
#define CW 640
#define CH 24
static uint32_t g_px[CW * CH];
static bitmap_t g_bm;
#define SENTINEL 0x00123456u
#define INK      0x00000000u
#define PAPER    0x00FFFFFFu
#define PEN_X    8
#define PEN_Y    4

static void canvas_reset(void)
{
    for (int i = 0; i < CW * CH; i++) g_px[i] = SENTINEL;
    g_bm.base = (volatile uint8_t *)g_px;
    g_bm.pitch = CW * 4u;
    g_bm.bpp = 32u;
    g_bm.bytes_per_pixel = 4u;
    g_bm.width = CW;
    g_bm.height = CH;
}

static uint32_t px_at(int x, int y) { return g_px[y * CW + x] & 0x00FFFFFFu; }

/* Painted column extent (any non-sentinel pixel), ink column extent, ink row
 * extent, relative to the pen. Returns 0 if nothing was painted. */
typedef struct { int pl, pr, il, ir, it, ib, any_ink, bad; } extent_t;

static extent_t measure_canvas(void)
{
    extent_t e = { CW, -1, CW, -1, CH, -1, 0, 0 };
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            uint32_t p = px_at(x, y);
            if (p == SENTINEL) continue;
            if (p != INK && p != PAPER) e.bad++;
            if (x < e.pl) e.pl = x;
            if (x > e.pr) e.pr = x;
            if (p == INK) {
                e.any_ink = 1;
                if (x < e.il) e.il = x;
                if (x > e.ir) e.ir = x;
                if (y < e.it) e.it = y;
                if (y > e.ib) e.ib = y;
            }
        }
    return e;
}

static extent_t draw_one(const char *s)
{
    canvas_reset();
    text_draw(&g_bm, PEN_X, PEN_Y, s, FONT_CHICAGO, INK, PAPER);
    return measure_canvas();
}

/* The strings the desktop shows (Finder bar + items, the HELLO/Photoshop bar,
 * window titles, the tenant fixture's text and bar). Width EXPECTATIONS come
 * from the NFNT only. */
static const char *const STRINGS[] = {
    "File", "Edit", "View", "Special", "Help", "Image", "Layer", "Select",
    "Window", "Fixture", "About This Computer", "Empty Trash", "Clean Up",
    "Close Window", "Get Info", "New Folder", "Open", "Print", "Quit",
    "Undo", "Cut", "Copy", "Paste", "Clear", "Select All", "Show Clipboard",
    "NOTES", "HELLO", "TenantFix", "Loaded from disk", "Click here to quit",
    "Saving tables to disk...", "INITECH", "Macintosh HD", "Untitled",
    "^Q", "^W", "^M", "OK", "Cancel", "Apple", "About",
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~",
};

int main(void)
{
    FILE *f = fopen(NFNT_PATH, "rb");
    printf(">>> test-chicago-metrics: FLAIR Chicago vs the REAL NFNT 5478 metrics\n");
    if (!f) {
#ifdef CHICAGO_METRICS_REQUIRE_GOLDEN
        printf("!!! test-chicago-metrics FAIL: golden %s absent and REQUIRED\n",
               NFNT_PATH);
        return 2;
#else
        printf("test-chicago-metrics: LOUD-SKIP -- %s absent (gitignored Apple "
               "resource; set SYSTEM7_DECOMP)\n", NFNT_PATH);
        return 0;
#endif
    }
    g_nfnt_len = (long)fread(g_nfnt, 1, sizeof g_nfnt, f);
    fclose(f);

    /* --- A: header (font-manager.md Sec 1.2, Sec 7.1) --- */
    CHECK(g_nfnt_len == 3098, "A NFNT_5478.bin is %ld bytes, chicago.md says 3098",
          g_nfnt_len);
    if (g_nfnt_len < 26) { printf("  short file\n"); return 1; }
    g_first   = be16s(2);
    g_last    = be16s(4);
    g_ow_off  = 16 + 2 * be16s(16);
    g_ascent  = be16s(18);
    g_descent = be16s(20);
    g_leading = be16s(22);
    CHECK(g_first <= 0x20 && g_last >= 0x7E, "A strike covers 0x20..0x7E "
          "(first %d last %d)", g_first, g_last);
    CHECK(g_ow_off + 2L * (g_last - g_first + 3) <= g_nfnt_len,
          "A owTable (byte %d) lies inside the file", g_ow_off);
    if (g_fails) return 1;
    printf("  NFNT 5478: first %d last %d ascent %d descent %d leading %d "
           "owTable @%d\n", g_first, g_last, g_ascent, g_descent, g_leading,
           g_ow_off);

    /* --- B + C: every printable glyph --- */
    for (int c = 0x20; c <= 0x7E; c++) {
        char s[2] = { (char)c, 0 };
        int lb, aw;
        if (!nfnt_ow(c, &lb, &aw)) {
            CHECK(0, "B NFNT has no owTable entry for 0x%02X", c);
            continue;
        }
        CHECK(text_measure(FONT_CHICAGO, s) == aw,
              "B advance '%c' (0x%02X): text_measure %d, NFNT aw %d",
              c, c, text_measure(FONT_CHICAGO, s), aw);
        extent_t e = draw_one(s);
        CHECK(e.bad == 0, "C '%c': %d pixels neither fg nor bg", c, e.bad);
        CHECK(e.pl == PEN_X && e.pr == PEN_X + aw - 1,
              "C drawn cell '%c' (0x%02X): painted x[%d,%d], NFNT advance wants "
              "[%d,%d]", c, c, e.pl, e.pr, PEN_X, PEN_X + aw - 1);
        if (c != ' ') {
            CHECK(e.any_ink, "C glyph '%c' (0x%02X) has no ink", c, c);
            if (e.any_ink) {
                CHECK(e.il - PEN_X == lb,
                      "C left bearing '%c' (0x%02X): first ink column %d, NFNT "
                      "lb %d", c, c, e.il - PEN_X, lb);
                CHECK(e.ir < PEN_X + aw,
                      "C '%c' ink at column %d overhangs its advance %d",
                      c, e.ir - PEN_X, aw);
            }
        } else {
            CHECK(!e.any_ink, "C the space glyph carries ink");
        }
        /* E: no ink in the leading row; ink stays inside ascent+descent. */
        if (e.any_ink)
            CHECK(e.it >= PEN_Y && e.ib < PEN_Y + g_ascent + g_descent,
                  "E '%c' ink rows [%d,%d] leave the ascent+descent band [0,%d)",
                  c, e.it - PEN_Y, e.ib - PEN_Y, g_ascent + g_descent);
    }

    /* --- D: strings --- */
    for (size_t k = 0; k < sizeof STRINGS / sizeof STRINGS[0]; k++) {
        const char *s = STRINGS[k];
        int want = nfnt_str_w(s);
        int got = text_measure(FONT_CHICAGO, s);
        CHECK(got == want, "D StringWidth(\"%s\"): text_measure %d, NFNT sum %d",
              s, got, want);
        if (want > 0 && PEN_X + want < CW) {
            extent_t e = draw_one(s);
            CHECK(e.pl == PEN_X && e.pr == PEN_X + want - 1,
                  "D drawn run \"%s\": painted x[%d,%d], NFNT wants [%d,%d]",
                  s, e.pl, e.pr, PEN_X, PEN_X + want - 1);
        }
    }
    {
        const char *chk[] = { "File", "Edit", "View", "OK" };
        const int  want[] = { 23, 25, 32, 17 };   /* chicago.md recipe step 3 */
        for (int i = 0; i < 4; i++)
            CHECK(nfnt_str_w(chk[i]) == want[i], "D the parser disagrees with "
                  "chicago.md's worked example \"%s\" = %d", chk[i], want[i]);
    }

    /* --- E: vertical metrics --- */
    CHECK(text_cell_height(FONT_CHICAGO) == g_ascent + g_descent + g_leading,
          "E line height %d != ascent+descent+leading %d",
          text_cell_height(FONT_CHICAGO), g_ascent + g_descent + g_leading);
    {
        const char *flat = "EFHILTZhiklmnrxz1";
        for (const char *p = flat; *p; p++) {
            char s[2] = { *p, 0 };
            extent_t e = draw_one(s);
            CHECK(e.any_ink && e.ib - PEN_Y == g_ascent - 1,
                  "E '%c' sits on row %d, the NFNT baseline is row %d "
                  "(ascent - 1)", *p, e.ib - PEN_Y, g_ascent - 1);
        }
        const char *desc = "gjpqy";
        for (const char *p = desc; *p; p++) {
            char s[2] = { *p, 0 };
            extent_t e = draw_one(s);
            CHECK(e.any_ink && e.ib - PEN_Y == g_ascent + g_descent - 1,
                  "E descender '%c' ends on row %d, NFNT descent ends on row %d",
                  *p, e.ib - PEN_Y, g_ascent + g_descent - 1);
        }
    }

    /* --- F: codes the REAL strike leaves undefined (owTable 0xFFFF) take the
     * missing-glyph advance, owTable[lastChar - firstChar + 1].aw (font-
     * manager.md Sec 5). FLAIR covers only 0x20..0x7E, so this is graded only
     * where the real font itself has no glyph. --- */
    {
        long me = g_ow_off + 2L * (g_last - g_first + 1);
        int miss_aw = (int)g_nfnt[me + 1], nmiss = 0;
        for (int c = 1; c <= 0xFF; c++) {
            int lb, aw;
            char s[2] = { (char)c, 0 };
            if (c >= 0x20 && c <= 0x7E) continue;
            if (nfnt_ow(c, &lb, &aw)) continue;
            nmiss++;
            CHECK(text_measure(FONT_CHICAGO, s) == miss_aw,
                  "F undefined code 0x%02X: text_measure %d, NFNT missing-glyph "
                  "aw %d", c, text_measure(FONT_CHICAGO, s), miss_aw);
        }
        CHECK(nmiss > 0, "F the NFNT has no undefined codes to grade");
    }

    printf("  %d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) {
        printf("!!! test-chicago-metrics FAIL\n");
        return 1;
    }
    printf(">>> test-chicago-metrics: all %d checks green\n", g_checks);
    return 0;
}

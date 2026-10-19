/*
 * harness/proptest/test_geneva12_metrics.c -- the INDEPENDENT Geneva 12
 * metrics + ink-extent oracle (bead initech-uelk; the test_chicago_metrics.c
 * pattern).
 *
 * WHAT IT GRADES: the artifact's Geneva 12 text path (os/flair/text.h --
 * text_measure / text_draw / text_cell_height / text_baseline_row /
 * text_font_from_txfont_size, the functions TEXTDRAW and TextEdit call)
 * against the REAL Mac OS 8.1 Geneva 12 strike, read at run time from the
 * Apple resource itself:
 *     $(SYSTEM7_DECOMP)/mint/work/geneva12/NFNT_12112.bin
 * (FOND 3 "Geneva", FAT entry size 12 style 0, extracted from
 * goldens/work/boot81.img System Folder:Fonts:Geneva; md5
 * 3e289fee44b6f1b9b89ac5343647ef2b; mint/work/geneva12/mint-results-011.md).
 * NOT against the artifact's own tables (Law 2 / ADR-0010 HER-02): every
 * expected number is parsed here from the NFNT bytes, or is a [MEAS] figure
 * of the SimpleText capture; this file includes text.h only to CALL the code
 * under test.
 *
 * Ref: ../system7-decomp specs/fonts/font-manager.md Sec 1.1 (owTable: high
 *      byte lb, low byte aw; 0xFFFF missing), Sec 1.2 (owTable at 16 +
 *      2*owTLoc; bitImage at 26, rowWords*2 bytes a row, fRectHeight rows;
 *      locationTable after it), Sec 7.1 (ascent/descent/leading at header
 *      bytes 18/20/22); specs/sys8/simpletext-stdfile.md Sec 1.2 [MEAS]
 *      ("Saving " 45 px, "tables" 39 px, "PC LOAD LETTER" 101 px; line pitch
 *      16 = 12 + 3 + 1).
 *
 * LEGS (each failure names the glyph / string and both numbers):
 *   A  header sanity: firstChar <= 0x20, lastChar >= 0x7E, sections in range.
 *   B  per-glyph ADVANCE, 0x20..0x7E: text_measure("c") == owTable aw(c).
 *   C  per-glyph DRAWN cell and INK BOX: text_draw paints exactly the columns
 *      [x, x+aw) and the 16 cell rows, and the ink bounding box (columns from
 *      the pen, rows from the cell top) EQUALS the real strike's glyph ink
 *      box (its image columns placed at lb, its rows at ascent - 12). This is
 *      the extents-only diff oracle the hand-authored art was drawn against
 *      (geneva12.art provenance) -- the glyph SHAPES are not compared.
 *   D  STRING widths: text_measure(s) == sum of NFNT aw and == the SimpleText
 *      [MEAS] widths, and text_draw's painted run spans exactly that width.
 *   E  VERTICAL metrics: text_cell_height == ascent + descent + leading (16);
 *      text_baseline_row == ascent (12); no ink in the leading row.
 *   F  MISSING glyph: a byte above the strike (0xFF > lastChar) measures the
 *      NFNT missing-glyph advance (owTable index lastChar + 1).
 *   G  txFont/txSize mapping: (applFont 1, 12) -> FONT_GENEVA12; (1, 0) ->
 *      Geneva 9; (0, 0) -> Chicago 12 (the old one-argument mapping holds).
 *
 * Absent corpus: FAIL LOUD; SKIP_SYSTEM7_DECOMP=1 is the explicit opt-out.
 * Mutant (test-geneva12-metrics-mutant): the Makefile flips one paper pixel
 * of 'S' row 2 (one row above its cap line) to ink in a COPY of the .art,
 * transcribes it into a mutant geneva12.h and compiles this unchanged file
 * against it: leg C must go RED for 'S' (ink box y 2..11 vs NFNT 3..11).
 *
 * FACTORY CODE (hosted C, Law 3). ASCII-clean (Rule 12).
 */
#include "../oracle_policy.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"

#ifndef SYSTEM7_DECOMP
#define SYSTEM7_DECOMP "../system7-decomp"
#endif
#define NFNT_PATH SYSTEM7_DECOMP "/mint/work/geneva12/NFNT_12112.bin"

static int g_checks, g_fails;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fails++; \
    printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- the NFNT, parsed independently ------------------------------------- */
static unsigned char g_nfnt[16384];
static long g_nfnt_len;
static int g_first, g_last, g_ascent, g_descent, g_leading, g_frect_h;
static long g_ow_off, g_img_off, g_loc_off, g_row_bytes;

static int be16s(long off)
{
    return (int)(int16_t)(((unsigned)g_nfnt[off] << 8) | g_nfnt[off + 1]);
}
static unsigned be16u(long off)
{
    return ((unsigned)g_nfnt[off] << 8) | g_nfnt[off + 1];
}

static int nfnt_ow(int c, int *lb, int *aw)
{
    long e;
    if (c < g_first || c > g_last + 1) return 0;
    e = g_ow_off + 2L * (c - g_first);
    if (g_nfnt[e] == 0xFF && g_nfnt[e + 1] == 0xFF) return 0;
    *lb = (int)(signed char)g_nfnt[e];
    *aw = (int)g_nfnt[e + 1];
    return 1;
}

static int nfnt_bit(long x, int y)
{
    return (g_nfnt[g_img_off + (long)y * g_row_bytes + x / 8] >> (7 - (x % 8))) & 1;
}

/* The real glyph's ink box in CELL coordinates (x from the pen, y from the
 * cell top = 12 - ascent rows above the strike's row 0). 0 = no ink. */
static int nfnt_box(int c, int *l, int *r, int *t, int *b)
{
    int lb, aw, any = 0;
    long x0, x1;
    if (!nfnt_ow(c, &lb, &aw)) return 0;
    x0 = (long)be16u(g_loc_off + 2L * (c - g_first));
    x1 = (long)be16u(g_loc_off + 2L * (c - g_first) + 2);
    *l = 99; *r = -1; *t = 99; *b = -1;
    for (long x = x0; x < x1; x++)
        for (int y = 0; y < g_frect_h; y++)
            if (nfnt_bit(x, y)) {
                int cx = lb + (int)(x - x0), cy = y + (12 - g_ascent);
                any = 1;
                if (cx < *l) *l = cx;
                if (cx > *r) *r = cx;
                if (cy < *t) *t = cy;
                if (cy > *b) *b = cy;
            }
    return any;
}

static int nfnt_str_w(const char *s)
{
    int w = 0, lb, aw;
    for (; *s; s++) {
        if (!nfnt_ow((unsigned char)*s, &lb, &aw)) return -1;
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

typedef struct { int pl, pr, pt, pb, il, ir, it, ib, ink, bad; } extent_t;
static extent_t measure_canvas(void)
{
    extent_t e = { CW, -1, CH, -1, CW, -1, CH, -1, 0, 0 };
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            uint32_t p = px_at(x, y);
            if (p == SENTINEL) continue;
            if (p != INK && p != PAPER) e.bad++;
            if (x < e.pl) e.pl = x;
            if (x > e.pr) e.pr = x;
            if (y < e.pt) e.pt = y;
            if (y > e.pb) e.pb = y;
            if (p == INK) {
                e.ink++;
                if (x < e.il) e.il = x;
                if (x > e.ir) e.ir = x;
                if (y < e.it) e.it = y;
                if (y > e.ib) e.ib = y;
            }
        }
    return e;
}

static void load_nfnt(void)
{
    FILE *f = fopen(NFNT_PATH, "rb");
    if (!f) {
        oracle_missing("SYSTEM7_DECOMP", "%s absent (extract it: mint/work/geneva12 recipe)\n", NFNT_PATH);
        printf(">>> test-geneva12-metrics: SKIPPED by explicit opt-out\n");
        exit(0);
    }
    g_nfnt_len = (long)fread(g_nfnt, 1, sizeof g_nfnt, f);
    fclose(f);
}

int main(void)
{
    static const char *const strs[] = {
        "Saving ", "tables", "PC LOAD LETTER", "Saving tables to disk", "PC LOAD",
        "untitled", "The quick brown fox jumps over the lazy dog 0123456789",
    };
    /* [MEAS] simpletext-stdfile.md Sec 1.2 / 1.4 -- independent literals */
    static const struct { const char *s; int w; } meas[] = {
        { "Saving ", 45 }, { "tables", 39 }, { "PC LOAD LETTER", 101 },
    };
    int lb, aw, rows, rowwords;

    load_nfnt();

    /* ---- A: header ---------------------------------------------------- */
    CHECK(g_nfnt_len > 26, "NFNT too short (%ld bytes)", g_nfnt_len);
    if (g_nfnt_len <= 26) goto done;
    g_first = be16s(2);
    g_last = be16s(4);
    g_frect_h = be16s(14);
    g_ow_off = 16 + 2L * be16s(16);
    g_ascent = be16s(18);
    g_descent = be16s(20);
    g_leading = be16s(22);
    rowwords = be16s(24);
    g_row_bytes = 2L * rowwords;
    g_img_off = 26;
    rows = g_frect_h;
    g_loc_off = g_img_off + g_row_bytes * rows;
    CHECK(g_first <= 0x20 && g_last >= 0x7E, "A firstChar %d lastChar %d", g_first, g_last);
    CHECK(g_ow_off + 2L * (g_last - g_first + 2) <= g_nfnt_len &&
          g_loc_off + 2L * (g_last - g_first + 3) <= g_nfnt_len,
          "A sections overrun the %ld-byte resource", g_nfnt_len);
    CHECK(g_frect_h == g_ascent + g_descent, "A fRectHeight %d != ascent %d + descent %d",
          g_frect_h, g_ascent, g_descent);
    if (g_fails) goto done;

    /* ---- B + C: per glyph --------------------------------------------- */
    for (int c = 0x20; c <= 0x7E; c++) {
        char s[2] = { (char)c, 0 };
        int l, r, t, b, real;
        extent_t e;
        if (!nfnt_ow(c, &lb, &aw)) { CHECK(0, "B 0x%02X missing in the NFNT", c); continue; }
        CHECK(text_measure(FONT_GENEVA12, s) == aw, "B advance '%c' (0x%02X): text_measure %d, NFNT aw %d",
              c, c, text_measure(FONT_GENEVA12, s), aw);
        canvas_reset();
        text_draw(&g_bm, PEN_X, PEN_Y, s, FONT_GENEVA12, INK, PAPER);
        e = measure_canvas();
        CHECK(e.bad == 0, "C '%c': %d pixels neither ink nor paper", c, e.bad);
        CHECK(e.pl == PEN_X && e.pr == PEN_X + aw - 1 && e.pt == PEN_Y && e.pb == PEN_Y + 15,
              "C '%c' (0x%02X): painted cell x %d..%d y %d..%d, want x %d..%d y %d..%d",
              c, c, e.pl, e.pr, e.pt, e.pb, PEN_X, PEN_X + aw - 1, PEN_Y, PEN_Y + 15);
        real = nfnt_box(c, &l, &r, &t, &b);
        if (!real) {
            CHECK(e.ink == 0, "C '%c' (0x%02X): %d ink px, the real glyph has none", c, c, e.ink);
            continue;
        }
        CHECK(e.ink > 0 && e.il - PEN_X == l && e.ir - PEN_X == r && e.it - PEN_Y == t && e.ib - PEN_Y == b,
              "C ink box '%c' (0x%02X): drawn x %d..%d y %d..%d, NFNT 12112 x %d..%d y %d..%d",
              c, c, e.il - PEN_X, e.ir - PEN_X, e.it - PEN_Y, e.ib - PEN_Y, l, r, t, b);
        CHECK(e.ib - PEN_Y < 15, "E '%c' ink in the leading row", c);
    }

    /* ---- D: strings ----------------------------------------------------- */
    for (unsigned i = 0; i < sizeof strs / sizeof strs[0]; i++) {
        int want = nfnt_str_w(strs[i]);
        extent_t e;
        CHECK(want > 0 && text_measure(FONT_GENEVA12, strs[i]) == want,
              "D \"%s\": text_measure %d, NFNT sum %d", strs[i], text_measure(FONT_GENEVA12, strs[i]), want);
        canvas_reset();
        text_draw(&g_bm, PEN_X, PEN_Y, strs[i], FONT_GENEVA12, INK, PAPER);
        e = measure_canvas();
        CHECK(e.pl == PEN_X && e.pr - e.pl + 1 == want, "D \"%s\": painted %d columns, want %d",
              strs[i], e.pr - e.pl + 1, want);
    }
    for (unsigned i = 0; i < sizeof meas / sizeof meas[0]; i++)
        CHECK(text_measure(FONT_GENEVA12, meas[i].s) == meas[i].w,
              "D [MEAS] SimpleText \"%s\" is %d px, text_measure %d", meas[i].s, meas[i].w,
              text_measure(FONT_GENEVA12, meas[i].s));

    /* ---- E: vertical ---------------------------------------------------- */
    CHECK(text_cell_height(FONT_GENEVA12) == g_ascent + g_descent + g_leading,
          "E cell height %d, NFNT ascent+descent+leading %d", text_cell_height(FONT_GENEVA12),
          g_ascent + g_descent + g_leading);
    CHECK(text_cell_height(FONT_GENEVA12) == 16, "E [MEAS] the SimpleText line is 16 px, cell %d",
          text_cell_height(FONT_GENEVA12));
    CHECK(text_baseline_row(FONT_GENEVA12) == g_ascent, "E baseline row %d, NFNT ascent %d",
          text_baseline_row(FONT_GENEVA12), g_ascent);

    /* ---- F: missing ----------------------------------------------------- */
    if (nfnt_ow(g_last + 1, &lb, &aw)) {
        char s[2] = { (char)0xFF, 0 };
        CHECK(0xFF > g_last, "F 0xFF is inside the strike (lastChar %d)", g_last);
        CHECK(text_measure(FONT_GENEVA12, s) == aw, "F missing glyph: text_measure %d, NFNT missing aw %d",
              text_measure(FONT_GENEVA12, s), aw);
    } else {
        CHECK(0, "F the NFNT has no missing-glyph entry");
    }

    /* ---- G: mapping ------------------------------------------------------ */
    CHECK(text_font_from_txfont_size(1, 12) == FONT_GENEVA12, "G (applFont, 12) is not Geneva 12");
    CHECK(text_font_from_txfont_size(1, 0) == FONT_GENEVA9, "G (applFont, 0) is not Geneva 9");
    CHECK(text_font_from_txfont_size(0, 0) == FONT_CHICAGO, "G (systemFont, 0) is not Chicago 12");

done:
    printf("test_geneva12_metrics: %d checks, %d failures (NFNT 12112: ascent %d descent %d leading %d)\n",
           g_checks, g_fails, g_ascent, g_descent, g_leading);
    if (g_fails) { printf("!!! test-geneva12-metrics FAIL\n"); return 1; }
    printf(">>> test-geneva12-metrics: green (95 advances, 95 ink boxes, strings, vertical metrics vs the real Mac OS 8.1 NFNT 12112)\n");
    return 0;
}

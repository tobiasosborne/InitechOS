/*
 * harness/proptest/test_font_styles.c -- the INDEPENDENT oracle of the Font
 * Manager's synthesized text styles (bead initech-tdnl.68, slice 2).
 *
 * WHAT IT GRADES: the artifact's txFace synthesis (os/flair/text.h --
 * text_synth_row, text_italic_shift, text_face_advance, text_measure_face,
 * text_draw_run) against sources the artifact never renders from (Law 2 /
 * ADR-0010 HER-02):
 *   - the REAL System 7.0.1 strikes, parsed here at run time from the Apple
 *     resources: NFNT 5478 (Chicago 12), NFNT 5415 (Geneva 9), NFNT 6475
 *     (Geneva 9 italic) in $(SYSTEM7_DECOMP)/goldens/resources/;
 *   - real System 7.5.3 screendump crops of bold Geneva 9 and of the Chicago
 *     ellipsis (harness/proptest/font_styles_golden.h, transcribed from
 *     goldens/captures/s7_get_info.png, s7_about.png, s7_menu_apple.png).
 *
 * LEGS (each failure names the glyph / string / row and both values):
 *   A   the three NFNT headers parse (firstChar, lastChar, ascent, owTable).
 *   G1  BOLD vs the golden: the ARTIFACT's text_synth_row + text_face_advance
 *       applied to the REAL plain Geneva 9 strike reproduce, pixel for pixel
 *       (paper margins included), the bold labels System 7.5.3 drew.
 *   G2  ELLIPSIS vs the golden: the artifact's Chicago 0xC9 cell (its own
 *       hand-authored art, drawn by text_draw_run) equals the ellipsis of
 *       "About This Macintosh..." in s7_menu_apple.png; its aw/lb equal the
 *       NFNT 5478 owTable entry 0xC9.
 *   M   METRICS: every Chicago advance in every face combination == the NFNT
 *       5478 aw + 1 when bold (font-manager.md Sec 6: synth_aw = plain_aw +
 *       FMOutRec.bold; italic / underline add nothing); styled string widths
 *       == the sums; Geneva 9 styled advances == plain + 1 when bold (the v0
 *       Geneva advances are hand-authored, so only the DELTA is graded).
 *   I   ITALIC vs the measurement: the per-row MODAL shift between NFNT 6475
 *       and NFNT 5415 is recomputed here; text_italic_shift must equal it on
 *       every row with >= 5 samples, and shearing the real plain strike with
 *       the artifact's text_synth_row must reproduce the real italic strike
 *       on >= 76% of its inked rows (measured 447/581 = 76.9% for the 1:5
 *       rule; the next-best single ratio, 1:4, scores 73.5%; no shear at all
 *       58.3% -- Apple hand-retouched the rest, so 100% is not attainable by
 *       ANY shear; the floor sits just under the measured value).
 *   R   RENDER STRUCTURE on the artifact's own strikes, both fonts, every
 *       printable glyph, every face combination: text_draw_run's ink is
 *       exactly this oracle's own rule applied to the plain ink (shear by
 *       floor(d/5), smear +1, underline row baseline+1 minus a 1-px halo),
 *       its opaque box is exactly the styled advance wide, nothing else paints.
 *       Plus strings with descenders (the underline gaps).
 *
 * MUTANTS (Rule 6; Makefile test-font-styles-mutant): TEXT_MUT_SMEAR_OFF,
 * TEXT_MUT_BOLD_ADVANCE_PLAIN, TEXT_MUT_UL_OFFSET, TEXT_MUT_ITALIC_RUN4 -- each
 * must go RED on the leg named in its expected failure line.
 *
 * Absent corpus: FAIL LOUD (oracle_policy.h). FACTORY CODE (hosted C, Law 3).
 * ASCII-clean (Rule 12).
 */
#include "../oracle_policy.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "region_algebra.h"
#include "font_styles_golden.h"

#ifndef SYSTEM7_DECOMP
#define SYSTEM7_DECOMP "../system7-decomp"
#endif
#define RES SYSTEM7_DECOMP "/goldens/resources/"

static int g_checks, g_fails;
#define CHECK(cond, ...) do { g_checks++; if (!(cond)) { g_fails++; \
    if (g_fails <= 40) { printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } } } while (0)

/* ---- an NFNT, parsed independently --------------------------------------- */
typedef struct nfnt {
    unsigned char d[8192];
    long len;
    int first, last, kern, ascent, descent, frh, rowwords, ow, loc, bits;
} nfnt_t;

static int be16(const nfnt_t *f, long o)
{
    return (int)(int16_t)(((unsigned)f->d[o] << 8) | f->d[o + 1]);
}

static int nfnt_load(nfnt_t *f, const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    f->len = (long)fread(f->d, 1, sizeof f->d, fp);
    fclose(fp);
    if (f->len < 26) return 0;
    f->first = be16(f, 2);
    f->last = be16(f, 4);
    f->kern = be16(f, 8);
    f->frh = be16(f, 14);
    f->ow = 16 + 2 * be16(f, 16);
    f->ascent = be16(f, 18);
    f->descent = be16(f, 20);
    f->rowwords = be16(f, 24);
    f->bits = 26;
    f->loc = 26 + f->rowwords * 2 * f->frh;
    return 1;
}

/* owTable lb/aw for code c (the missing entry for undefined codes) and the
 * glyph's image extent [x0, x1) in the strike. Returns 0 when c is outside. */
static int nfnt_glyph(const nfnt_t *f, int c, int *lb, int *aw, int *x0, int *x1)
{
    int i;
    if (c < f->first || c > f->last) return 0;
    i = c - f->first;
    if (f->d[f->ow + 2 * i] == 0xFF && f->d[f->ow + 2 * i + 1] == 0xFF)
        i = f->last - f->first + 1;
    *lb = (int)(signed char)f->d[f->ow + 2 * i];
    *aw = (int)f->d[f->ow + 2 * i + 1];
    *x0 = (be16(f, f->loc + 2 * i) & 0xFFFF);
    *x1 = (be16(f, f->loc + 2 * i + 2) & 0xFFFF);
    return 1;
}

static int nfnt_pix(const nfnt_t *f, int x, int y)
{
    unsigned char b = f->d[f->bits + y * f->rowwords * 2 + x / 8];
    return (b >> (7 - x % 8)) & 1;
}

/* The REAL plain row of glyph c as a TEXT_OVERHANG mask (bit 31 - (col +
 * TEXT_OVERHANG) = ink at pen + col; col = kernMax + lb + image column). */
static uint32_t nfnt_row_mask(const nfnt_t *f, int c, int r)
{
    int lb, aw, x0, x1;
    uint32_t m = 0u;
    if (!nfnt_glyph(f, c, &lb, &aw, &x0, &x1)) return 0u;
    for (int x = x0; x < x1; x++)
        if (nfnt_pix(f, x, r)) {
            int col = f->kern + lb + (x - x0);
            int b = 31 - (col + TEXT_OVERHANG);
            if (b >= 0 && b < 32) m |= 1u << b;
        }
    return m;
}

static nfnt_t g_chi, g_gen, g_genit;

/* ---- a hosted 32bpp canvas ---------------------------------------------- */
#define CW 256
#define CH 40
#define SENT  0x00123456u
#define INK   0x00000000u
#define PAPER 0x00FFFFFFu
static uint32_t g_px[CW * CH];
static bitmap_t g_bm;

static void canvas(uint32_t fill)
{
    for (int i = 0; i < CW * CH; i++) g_px[i] = fill;
    g_bm.base = (volatile uint8_t *)g_px;
    g_bm.pitch = CW * 4u;
    g_bm.bpp = 32u;
    g_bm.bytes_per_pixel = 4u;
    g_bm.width = CW;
    g_bm.height = CH;
}
static uint32_t px(int x, int y) { return g_px[y * CW + x] & 0x00FFFFFFu; }

/* ===========================================================================
 * G1 -- bold vs the System 7.5.3 captures
 * ===========================================================================*/
static void leg_g1(void)
{
    int legfail = g_fails;
    for (int k = 0; k < FSG_BOLD_N; k++) {
        const fsg_crop_t *g = &FSG_BOLD[k];
        int ox = 16, oy = 4;          /* the pen origin on the canvas */
        int pen = 0, bad = 0, fx = -1, fy = -1;
        canvas(PAPER);
        for (const char *s = g->text; *s; s++) {
            int c = (unsigned char)*s, lb, aw, x0, x1;
            if (!nfnt_glyph(&g_gen, c, &lb, &aw, &x0, &x1)) { CHECK(0, "G1 no glyph %02X", c); return; }
            for (int r = 0; r < g_gen.frh; r++) {
                uint32_t m = text_synth_row(nfnt_row_mask(&g_gen, c, r),
                                            TEXT_FACE_BOLD, g_gen.ascent - r);
                for (int b = 31; b >= 0; b--)
                    if (m & (1u << b)) {
                        int x = ox + pen + (31 - b) - TEXT_OVERHANG;
                        if (x >= 0 && x < CW) g_px[(oy + r) * CW + x] = INK;
                    }
            }
            /* the artifact's bold delta, on the REAL plain advance */
            pen += aw + (text_face_advance(FONT_GENEVA9, TEXT_FACE_BOLD, c) -
                         text_face_advance(FONT_GENEVA9, 0u, c));
        }
        CHECK(pen == g->w, "G1 '%s' (%s): bold StringWidth %d, the capture's %d",
              g->text, g->capture, pen, g->w);
        for (int r = 0; r < FSG_GENEVA_ROWS; r++)
            for (int col = 0; col < g->w + 2 * FSG_MARGIN; col++) {
                int want = g->rows[r][col] == '#';
                int got = px(ox - FSG_MARGIN + col, oy + r) == INK;
                if (want != got) { if (!bad) { fx = col; fy = r; } bad++; }
            }
        CHECK(bad == 0, "G1 '%s' (%s pen_x %d top %d): %d px differ from the capture"
              " (first at crop col %d row %d)", g->text, g->capture, g->pen_x,
              g->top, bad, fx, fy);
    }
    printf("  leg G1 (bold vs s7_get_info/s7_about, %d labels): %s\n",
           FSG_BOLD_N, g_fails == legfail ? "ok" : "RED");
}

/* ===========================================================================
 * G2 -- the ellipsis glyph vs s7_menu_apple.png
 * ===========================================================================*/
static void leg_g2(void)
{
    int legfail = g_fails, lb, aw, x0, x1, bad = 0;
    char e[2] = { (char)0xC9, 0 };
    CHECK(nfnt_glyph(&g_chi, 0xC9, &lb, &aw, &x0, &x1), "G2 NFNT 5478 has no 0xC9");
    CHECK(chicago_advance(0xC9) == aw, "G2 ellipsis advance %d, NFNT aw %d", chicago_advance(0xC9), aw);
    CHECK(chicago_bearing(0xC9) == lb, "G2 ellipsis bearing %d, NFNT lb %d", chicago_bearing(0xC9), lb);
    CHECK(text_measure(FONT_CHICAGO, "About This Macintosh") + 24 == FSG_ELLIPSIS_PEN_X,
          "G2 prefix width %d + 24 != the capture's ellipsis pen %d",
          text_measure(FONT_CHICAGO, "About This Macintosh"), FSG_ELLIPSIS_PEN_X);
    canvas(PAPER);
    text_draw_run(&g_bm, 10, 2, e, FONT_CHICAGO, 0u, INK, PAPER, 0);
    for (int r = 0; r < FSG_ELLIPSIS_ROWS; r++)
        for (int c = 0; c < FSG_ELLIPSIS_COLS; c++) {
            int want = FSG_ELLIPSIS[r][c] == '#';
            int got = px(10 + c, 2 + r) == INK;
            if (want != got) bad++;
        }
    CHECK(bad == 0, "G2 the 0xC9 cell differs from s7_menu_apple.png (%d px)", bad);
    printf("  leg G2 (ellipsis 0xC9 vs s7_menu_apple + NFNT 5478): %s\n",
           g_fails == legfail ? "ok" : "RED");
}

/* ===========================================================================
 * M -- styled advances vs NFNT 5478 (+ the Geneva delta)
 * ===========================================================================*/
static void leg_m(void)
{
    int legfail = g_fails;
    static const char *strs[] = { "File", "Edit", "Bold Italic Underline",
        "The quick brown fox jumps over the lazy dog.", "Saving tables to disk", 0 };
    for (unsigned face = 0; face < 8u; face++) {
        int bold = (face & TEXT_FACE_BOLD) ? 1 : 0;
        for (int c = 0x20; c <= 0x7F; c++) {
            int code = (c == 0x7F) ? 0xC9 : c, lb, aw, x0, x1;
            if (!nfnt_glyph(&g_chi, code, &lb, &aw, &x0, &x1)) continue;
            CHECK(text_face_advance(FONT_CHICAGO, face, code) == aw + bold,
                  "M Chicago face %u '%c' (0x%02X): advance %d, NFNT aw %d + bold %d",
                  face, code < 0x7F ? code : '?', code,
                  text_face_advance(FONT_CHICAGO, face, code), aw, bold);
            CHECK(text_face_advance(FONT_GENEVA9, face, code) ==
                  text_face_advance(FONT_GENEVA9, 0u, code) + bold,
                  "M Geneva face %u 0x%02X: styled advance is not plain + %d",
                  face, code, bold);
        }
        for (int k = 0; strs[k]; k++) {
            int want = 0, lb, aw, x0, x1;
            for (const char *s = strs[k]; *s; s++)
                if (nfnt_glyph(&g_chi, (unsigned char)*s, &lb, &aw, &x0, &x1))
                    want += aw + bold;
            CHECK(text_measure_face(FONT_CHICAGO, face, strs[k]) == want,
                  "M Chicago face %u \"%s\": width %d, NFNT sum %d", face, strs[k],
                  text_measure_face(FONT_CHICAGO, face, strs[k]), want);
        }
    }
    CHECK(text_measure_face(FONT_CHICAGO, 0u, "Edit") == text_measure(FONT_CHICAGO, "Edit"),
          "M plain text_measure_face != text_measure");
    printf("  leg M (styled advances vs NFNT 5478, 8 faces): %s\n",
           g_fails == legfail ? "ok" : "RED");
}

/* ===========================================================================
 * I -- the italic shear vs Apple's Geneva 9 italic strike
 * ===========================================================================*/
static void leg_i(void)
{
    int legfail = g_fails;
    int tally[24][7];      /* [d + 8][shift + 3] */
    int agree = 0, total = 0;
    memset(tally, 0, sizeof tally);
    for (int c = 0x21; c <= 0x7E; c++)
        for (int r = 0; r < g_gen.frh; r++) {
            uint32_t a = nfnt_row_mask(&g_gen, c, r);
            uint32_t b = nfnt_row_mask(&g_genit, c, r);
            int d = g_gen.ascent - r;
            if (a == 0u && b == 0u) continue;
            total++;
            if (text_synth_row(a, TEXT_FACE_ITALIC, d) == b) agree++;
            for (int s = -3; s <= 3; s++) {
                uint32_t sh = (s >= 0) ? (a >> s) : (a << -s);
                if (sh == b) { tally[d + 8][s + 3]++; break; }
            }
        }
    for (int d = -2; d <= 10; d++) {
        int n = 0, best = 0, bs = 0;
        for (int s = 0; s < 7; s++) {
            n += tally[d + 8][s];
            if (tally[d + 8][s] > best) { best = tally[d + 8][s]; bs = s - 3; }
        }
        if (n < 5) continue;
        CHECK(text_italic_shift(d) == bs,
              "I row d=%d above the baseline: artifact shear %d, Apple's modal shift %d"
              " (%d of %d rows)", d, text_italic_shift(d), bs, best, n);
    }
    CHECK(agree * 100 >= total * 76,
          "I sheared real plain == real italic on %d of %d rows (%d%%), floor 76%%",
          agree, total, total ? agree * 100 / total : 0);
    printf("  leg I (italic shear vs NFNT 6475/5415 modal shift; %d/%d rows exact): %s\n",
           agree, total, g_fails == legfail ? "ok" : "RED");
}

/* ===========================================================================
 * R -- render structure on the artifact's strikes (this oracle's own rules)
 * ===========================================================================*/
static int o_shift(int d) { return (d >= 0) ? d / 5 : -((-d + 4) / 5); }

#define RX 40
#define RY 8
static uint8_t g_plain[CH][CW], g_want[CH][CW];

static void ink_of(uint8_t out[CH][CW])
{
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++)
            out[y][x] = (px(x, y) == INK);
}

/* Render s styled, and compare with the rule applied to the plain render. */
static void render_case(text_font_t font, unsigned face, const char *s)
{
    int base = (font == FONT_CHICAGO) ? 12 : 8;   /* first row below baseline */
    int h = (font == FONT_CHICAGO) ? 16 : 11;
    int wp, ws, want_w = 0, bad = 0, fx = -1, fy = -1;
    int bold = (face & TEXT_FACE_BOLD) != 0;

    /* plain, glyph by glyph at the STYLED pens (so the rules apply per glyph) */
    canvas(SENT);
    {
        int pen = RX;
        char one[2] = { 0, 0 };
        for (const char *p = s; *p; p++) {
            one[0] = *p;
            text_draw_run(&g_bm, pen, RY, one, font, 0u, INK, PAPER, 0);
            pen += text_measure(font, one) + bold;
        }
        want_w = pen - RX;
    }
    ink_of(g_plain);
    memset(g_want, 0, sizeof g_want);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            if (!g_plain[y][x]) continue;
            int d = base - (y - RY);
            int nx = (face & TEXT_FACE_ITALIC) ? x + o_shift(d) : x;
            g_want[y][nx] = 1;
            if (bold) g_want[y][nx + 1] = 1;
        }
    if (face & TEXT_FACE_UNDERLINE) {
        int uy = RY + base + 1;
        uint8_t ink[CW];
        memcpy(ink, g_want[uy], sizeof ink);   /* the glyph ink alone */
        for (int x = RX; x < RX + want_w; x++) {
            int gap = ink[x - 1] || ink[x] || ink[x + 1];
            if (!gap) g_want[uy][x] = 1;
        }
    }
    canvas(SENT);
    ws = text_draw_run(&g_bm, RX, RY, s, font, face, INK, PAPER, 0);
    wp = text_measure(font, s);
    CHECK(ws == want_w && ws == wp + (bold ? (int)strlen(s) : 0),
          "R font %d face %u \"%s\": run width %d, want %d", font, face, s, ws, want_w);
    for (int y = 0; y < CH; y++)
        for (int x = 0; x < CW; x++) {
            uint32_t p = px(x, y);
            int in_box = x >= RX && x < RX + want_w && y >= RY && y < RY + h;
            int ok;
            if (g_want[y][x]) ok = (p == INK);
            else ok = in_box ? (p == PAPER) : (p == SENT);
            if (!ok) { if (!bad) { fx = x - RX; fy = y - RY; } bad++; }
        }
    CHECK(bad == 0, "R font %d face %u \"%s\": %d px off the rule (first at pen+%d, row %d)",
          font, face, s, bad, fx, fy);
}

static void leg_r(void)
{
    int legfail = g_fails;
    char one[2] = { 0, 0 };
    /* the underline's own placement, stated once (documented reading): the
     * ONLY row an underline adds ink to is the one below the baseline row,
     * for both fonts ("mmm" has no descender, so nothing breaks it) */
    for (int f = 0; f < 2; f++) {
        static uint8_t plain[CH][CW];
        int want = (f == 0) ? 13 : 9, rows = 0, row = -1, n = 0;
        canvas(SENT);
        text_draw_run(&g_bm, RX, RY, "mmm", (text_font_t)f, 0u, INK, PAPER, 0);
        ink_of(plain);
        canvas(SENT);
        text_draw_run(&g_bm, RX, RY, "mmm", (text_font_t)f, TEXT_FACE_UNDERLINE, INK, PAPER, 0);
        for (int y = 0; y < CH; y++) {
            int diff = 0;
            for (int x = 0; x < CW; x++)
                if ((px(x, y) == INK) != plain[y][x]) { diff++; n++; }
            if (diff) { rows++; row = y - RY; }
        }
        CHECK(rows == 1 && row == want && n == text_measure((text_font_t)f, "mmm"),
              "R underline (font %d): adds %d px on %d row(s), last row %d; want one"
              " full-width row %d (baseline row + ulOffset 1)", f, n, rows, row, want);
    }
    for (int f = 0; f < 2; f++)
        for (unsigned face = 1; face < 8u; face++) {
            for (int c = 0x21; c <= 0x7E; c++) {
                one[0] = (char)c;
                render_case((text_font_t)f, face, one);
            }
            render_case((text_font_t)f, face, "Hey jumpy fox, quickly");
            render_case((text_font_t)f, face, "gyp jq");
        }
    printf("  leg R (render structure, 2 fonts x 7 faces x 94 glyphs + strings): %s\n",
           g_fails == legfail ? "ok" : "RED");
}

int main(void)
{
    printf(">>> test-font-styles: txFace synthesis vs NFNT 5478/5415/6475 + System 7.5.3 captures (bead initech-tdnl.68)\n");
    if (!nfnt_load(&g_chi, RES "NFNT_5478.bin") || !nfnt_load(&g_gen, RES "NFNT_5415.bin") ||
        !nfnt_load(&g_genit, RES "NFNT_6475.bin")) {
        printf("!!! test-font-styles FAIL: cannot read the NFNT resources under %s"
               " (SYSTEM7_DECOMP; the corpus is required -- fail loud)\n", RES);
        return 1;
    }
    CHECK(g_chi.first <= 0x20 && g_chi.last >= 0xC9 && g_chi.ascent == 12, "A NFNT 5478 header");
    CHECK(g_gen.ascent == 10 && g_gen.frh == FSG_GENEVA_ROWS && g_gen.kern == 0, "A NFNT 5415 header");
    CHECK(g_genit.ascent == 10 && g_genit.frh == 12 && g_genit.kern == -1, "A NFNT 6475 header");
    leg_g1();
    leg_g2();
    leg_m();
    leg_i();
    leg_r();
    printf(">>> test-font-styles: %d checks, %d failed\n", g_checks, g_fails);
    if (g_fails) {
        printf("!!! test-font-styles FAIL\n");
        return 1;
    }
    printf(">>> test-font-styles: green\n");
    return 0;
}

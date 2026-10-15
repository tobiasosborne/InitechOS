/*
 * tools/ppm_flair_fonts_check.c -- the screendump grader of test-flair-fonts
 * (bead initech-tdnl.68, Font Manager slice 2). FACTORY CODE (hosted C,
 * Law 3). ASCII-clean (Rule 12).
 *
 * WHAT IT GRADES, on a real QEMU screendump of the flagship desktop running
 * the FONTS specimen tenant (os/apps/fonts/fonts.c; geometry derived in
 * spec/flair_fonts_traces.mk):
 *
 *   window <ppm>
 *     W1 STYLES   each specimen line (plain / bold / italic / underline /
 *                 bold+italic+underline) is, pixel for pixel over its whole
 *                 16-row band of the content, EXACTLY this grader's own model:
 *                 the locked glyph art (spec/assets/chicago12.h) placed with
 *                 the REAL NFNT 5478 advances and bearings (spec/
 *                 chicago12_nfnt_golden.h, proven against the Apple resource
 *                 by test-chicago-metrics leg G) and the synthesis rules this
 *                 file implements independently of os/flair/text.h -- bold:
 *                 OR with a 1-px-right copy, advance +1 (font-manager.md Sec
 *                 6.1, golden-verified by test-font-styles G1); italic: row
 *                 shift floor(d/5), d = rows above the baseline (measured off
 *                 NFNT 6475, test-font-styles I); underline: cell row 13 under
 *                 the run, broken 1 px around ink on that row (documented).
 *     W2 CLIP     the bold line wider than the window is the model CLIPPED at
 *                 the content's right edge (x 499), and the 21 columns right
 *                 of it (the scroll bar and frame, x 499..519) are byte-equal
 *                 on the line's rows to the same columns 32 rows lower, where
 *                 no text is drawn -- not one text pixel escaped the window.
 *     W3 TITLE    the title "Font Manager Styles Specimen" does not fit its
 *                 stripe run; the title band's ink is exactly the longest
 *                 prefix + the Chicago ellipsis 0xC9 that fits the measured
 *                 rule (centered on the whole bar, gap pads 6/5 inside the run
 *                 [321,482); spec/chrome_metrics.json platinum_title_text_
 *                 layout), and the ellipsis's three 2x2 dots are there.
 *   menu <ppm>
 *     M1 STYLED ITEMS  the held Style menu's Bold, Italic and Underline rows
 *                 are each exactly the model in their own face (the Plain row
 *                 is left to the menu gates: the cursor arrow, at the Style
 *                 title, overlaps its top rows).
 *     M2 WIDTH    the panel's right frame sits where the STYLED widths put it
 *                 (20 + widest styled item + 12; menu.h ITEM_LPAD / RPAD).
 *
 * Usage: ppm_flair_fonts_check window|menu <screendump.ppm>; exit 0 iff green.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chicago12.h"             /* the locked glyph ART only */
#include "chicago12_nfnt_golden.h" /* the REAL metrics           */

/* ---- the screendump ------------------------------------------------------ */
static unsigned char *g_rgb;
static int g_w, g_h;

static int load_ppm(const char *path)
{
    FILE *f = fopen(path, "rb");
    int maxv;
    if (!f) return 0;
    if (fscanf(f, "P6 %d %d %d", &g_w, &g_h, &maxv) != 3 || maxv != 255) { fclose(f); return 0; }
    fgetc(f);
    g_rgb = malloc((size_t)g_w * g_h * 3);
    if (!g_rgb || fread(g_rgb, 3, (size_t)g_w * g_h, f) != (size_t)g_w * g_h) { fclose(f); return 0; }
    fclose(f);
    return 1;
}
static uint32_t px(int x, int y)
{
    const unsigned char *p = g_rgb + ((size_t)y * g_w + x) * 3;
    return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
}
static int is_ink(int x, int y) { return px(x, y) == 0x000000u; }

/* ---- the independent model ----------------------------------------------- */
#define F_BOLD 1u
#define F_ITAL 2u
#define F_UL   4u
#define CELL   16
#define BASE   12            /* NFNT 5478 ascent: the first row below the baseline */
#define OV     4             /* model columns left of the pen */
#define MW     420
static uint8_t g_m[CELL][MW];
static int g_mw;             /* the run's advance width */

static int shift_of(int d) { return (d >= 0) ? d / 5 : -((-d + 4) / 5); }

/* golden metrics: printable ASCII and 0xC9 */
static int aw_of(int c) { return nfnt5478_aw(c); }
static int lb_of(int c) { return nfnt5478_lb(c); }

static void model(const char *s, unsigned face)
{
    int pen = 0, bold = (face & F_BOLD) ? 1 : 0;
    memset(g_m, 0, sizeof g_m);
    for (; *s; s++) {
        int c = (unsigned char)*s;
        const uint16_t *rows = chicago12_rows[chicago12_index(c)];
        for (int r = 0; r < CELL; r++)
            for (int i = 0; i < 16; i++) {
                if (!(rows[r] & (0x8000u >> i))) continue;
                int x = pen + lb_of(c) + i + OV;
                if (face & F_ITAL) x += shift_of(BASE - r);
                if (x >= 0 && x < MW) g_m[r][x] = 1;
                if (bold && x + 1 < MW) g_m[r][x + 1] = 1;
            }
        pen += aw_of(c) + bold;
    }
    g_mw = pen;
    if (face & F_UL) {
        uint8_t ink[MW];
        memcpy(ink, g_m[BASE + 1], sizeof ink);
        for (int col = 0; col < g_mw; col++) {
            int x = col + OV;
            if (!ink[x - 1] && !ink[x] && !(x + 1 < MW && ink[x + 1]))
                g_m[BASE + 1][x] = 1;
        }
    }
}

static int model_at(int col, int r)   /* col relative to the pen */
{
    int x = col + OV;
    if (r < 0 || r >= CELL || x < 0 || x >= MW) return 0;
    return g_m[r][x];
}

/* Grade rows [y0 + r0, y0 + r1) x columns [xa, xb) of the screen against the
 * model drawn with its pen at (px0, y0). Returns the mismatch count. */
static int grade(int px0, int y0, int r0, int r1, int xa, int xb, int *fx, int *fy)
{
    int bad = 0;
    for (int r = r0; r < r1; r++)
        for (int x = xa; x < xb; x++)
            if (is_ink(x, y0 + r) != model_at(x - px0, r)) {
                if (!bad) { *fx = x; *fy = y0 + r; }
                bad++;
            }
    return bad;
}

/* ---- window -------------------------------------------------------------- */
/* RE-KEYED 2026-10-16 (bead initech-tdnl.51, stated): the content starts
 * inside the left body rail, frame.left 300 + 6 = 306 (was 301). */
#define CONT_L 306
#define CONT_T 192
#define CONT_R 499           /* exclusive: column 499 is the scroll bar's line */
#define LINE_X (CONT_L + 6)

static int check_window(void)
{
    static const struct { int y; const char *s; unsigned face; const char *name; } L[] = {
        { 2,  "Hamburger", 0u,                    "plain" },
        { 20, "Hamburger", F_BOLD,                "bold" },
        { 38, "Hamburger", F_ITAL,                "italic" },
        { 56, "jumpy gyp", 0u,                    "plain (descenders)" },
        { 74, "jumpy gyp", F_UL,                  "underline" },
        { 92, "Hamburger", F_BOLD | F_ITAL | F_UL, "bold+italic+underline" },
    };
    int fails = 0, fx = 0, fy = 0;

    /* W1 */
    for (unsigned k = 0; k < sizeof L / sizeof L[0]; k++) {
        int y0 = CONT_T + L[k].y, bad, ink = 0;
        model(L[k].s, L[k].face);
        bad = grade(LINE_X, y0, 0, CELL, CONT_L, CONT_R, &fx, &fy);
        for (int r = 0; r < CELL; r++) for (int x = CONT_L; x < CONT_R; x++) ink += is_ink(x, y0 + r);
        if (bad) {
            printf("!!! W1 STYLES: line \"%s\" %s at (%d,%d): %d px differ from the model (first at %d,%d)\n",
                   L[k].s, L[k].name, LINE_X, y0, bad, fx, fy);
            fails++;
        } else {
            printf(">>> W1 STYLES: \"%s\" %-22s exact (%d ink px, run %d px)\n", L[k].s, L[k].name, ink, g_mw);
        }
    }

    /* W2 */
    {
        const char *s = "Saving tables to disk... PC LOAD LETTER";
        int y0 = CONT_T + 116, bad, esc = 0;
        model(s, F_BOLD);
        bad = grade(LINE_X, y0, 0, CELL, CONT_L, CONT_R, &fx, &fy);
        for (int r = 0; r < CELL; r++)
            for (int x = CONT_R; x < CONT_R + 21; x++)
                if (px(x, y0 + r) != px(x, y0 + r + 32)) esc++;
        if (LINE_X + g_mw <= CONT_R + 21) {
            printf("!!! W2 CLIP: the long line (%d px) does not reach past the window -- the leg is vacuous\n", g_mw);
            fails++;
        } else if (bad || esc) {
            printf("!!! W2 CLIP: the %d-px bold line: %d px differ inside the content, %d px changed right of x=%d\n",
                   g_mw, bad, esc, CONT_R);
            fails++;
        } else {
            printf(">>> W2 CLIP: the %d-px bold line is the model cut at x=%d; columns %d..%d unchanged on its rows\n",
                   g_mw, CONT_R, CONT_R, CONT_R + 20);
        }
    }

    /* W3 */
    {
        static const char title[] = "Font Manager Styles Specimen";
        const int left = 300, right = 520, safe_l = left + 21, safe_r = right - 38;
        const int ty = 170 + 3;
        char cand[64];
        int n = (int)strlen(title), got = -1, tx = 0;
        for (int k = n - 1; k >= 1 && got < 0; k--) {
            int lo = 9999, hi = -1, x;
            memcpy(cand, title, (size_t)k);
            cand[k] = (char)0xC9;
            cand[k + 1] = 0;
            model(cand, 0u);
            for (int r = 0; r < CELL; r++)
                for (int c = -OV; c < g_mw; c++)
                    if (model_at(c, r)) { if (c < lo) lo = c; if (c > hi) hi = c; }
            x = left + ((right - left) - g_mw) / 2;
            if (x + lo - 6 >= safe_l && x + hi + 1 + 5 <= safe_r) { got = k; tx = x; }
        }
        if (got < 0) {
            printf("!!! W3 TITLE: no prefix fits (model error)\n");
            return 1;
        }
        {
            int bad = grade(tx, ty, 0, CELL, safe_l, safe_r, &fx, &fy), dots = 0;
            int ex = tx + g_mw - aw_of(0xC9) + lb_of(0xC9);    /* the ellipsis ink start */
            for (int d = 0; d < 3; d++)
                dots += is_ink(ex + 4 * d, ty + 10) && is_ink(ex + 4 * d + 1, ty + 11) &&
                        !is_ink(ex + 4 * d + 2, ty + 10);
            if (bad || dots != 3) {
                printf("!!! W3 TITLE: \"%.*s\" + 0xC9 at x=%d: %d px differ, %d of 3 ellipsis dots (first diff %d,%d)\n",
                       got, title, tx, bad, dots, fx, fy);
                fails++;
            } else {
                printf(">>> W3 TITLE: \"%.*s\" + the 0xC9 ellipsis at x=%d, exact over [%d,%d); 3 dots at x=%d,%d,%d\n",
                       got, title, tx, safe_l, safe_r, ex, ex + 4, ex + 8);
            }
        }
    }
    return fails;
}

/* ---- menu ---------------------------------------------------------------- */
static int check_menu(void)
{
    static const struct { const char *s; unsigned face; } I[] = {
        { "Plain", 0u }, { "Bold", F_BOLD }, { "Italic", F_ITAL }, { "Underline", F_UL },
    };
    /* band 2 (y_top 20): panel top 20 + 20 - 1 = 39, rows from 41 (+16 each);
     * the Style slot starts at x74; item text at panel.left+19, its cell
     * one row above its row (menu.c, CHICAGO_CAP_TOP). */
    const int pl = 74, ytop = 20; /* re-key2026-10-15: Apple33 + File23 + pads18 */
    int fails = 0, fx = 0, fy = 0, widest = 0, pr;
    for (unsigned k = 0; k < 4; k++) {
        model(I[k].s, I[k].face);
        if (g_mw > widest) widest = g_mw;
    }
    pr = pl + 19 + widest + 12; /* sampled ink20 minus NFNT bearing1 */
    for (unsigned k = 1; k < 4; k++) {
        int row = ytop + 19 + 2 + 16 * (int)k, bad;
        model(I[k].s, I[k].face);
        bad = grade(pl + 19, row - 1, 1, CELL, pl + 2, pr - 2, &fx, &fy);
        if (bad) {
            printf("!!! M1 STYLED ITEMS: \"%s\" (face %u) row %d: %d px differ (first at %d,%d)\n",
                   I[k].s, I[k].face, row, bad, fx, fy);
            fails++;
        } else {
            printf(">>> M1 STYLED ITEMS: \"%s\" drawn in face %u, exact (row %d)\n", I[k].s, I[k].face, row);
        }
    }
    {
        int ok = 1;
        for (int y = ytop + 22; y < ytop + 19 + 16 * 4; y++)   /* above the bottom bevel */
            ok = ok && is_ink(pr - 1, y) && !is_ink(pr - 3, y);
        if (!ok) {
            printf("!!! M2 WIDTH: the panel's right frame is not at x=%d (19 + widest styled %d + 12)\n", pr - 1, widest);
            fails++;
        } else {
            printf(">>> M2 WIDTH: panel right frame at x=%d = 74 + 19 + %d (widest styled item) + 12\n", pr - 1, widest);
        }
    }
    return fails;
}

int main(int argc, char **argv)
{
    int fails;
    if (argc != 3 || !load_ppm(argv[2])) {
        fprintf(stderr, "usage: ppm_flair_fonts_check window|menu <screendump.ppm> (readable P6)\n");
        return 2;
    }
    if (strcmp(argv[1], "window") == 0) fails = check_window();
    else if (strcmp(argv[1], "menu") == 0) fails = check_menu();
    else { fprintf(stderr, "unknown mode %s\n", argv[1]); return 2; }
    return fails ? 1 : 0;
}

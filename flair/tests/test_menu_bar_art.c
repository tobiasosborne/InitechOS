/* test_menu_bar_art.c -- the idle menu bar DrawMenuBar paints, against
 * expectations that do not come from DrawMenuBar or the packed font.
 *
 * flair/desktop/tests/test_shell.c compares the composed top band with a
 * second DrawMenuBar call, so a bug in DrawMenuBar is in both sides. This
 * program supplies the independent side (CLAUDE.md, "The constitution",
 * point 4):
 *
 * BAR   every column x=15..624: row 0 #FFFFFF, rows 1 and 17 #E7E7E7, row 18
 *       #B3B3B3, row 19 #000000. system7-decomp specs/sys8/menus.md sec 1.1
 *       (vscan x=300 in five captures; the strip is full width).
 * CORNER the 15 x 9 grid of menus.md sec 1.2, transcribed from
 *       s8_desktop_menubar.png x=0..14 y=0..8; the right corner is its mirror.
 * TITLES the Finder titles File Edit View Special: ink width 21/23/30/44 and
 *       ink rows 5..13 (Special 5..15), measured in s7_desktop.png (System
 *       7.5.3, Chicago 12; specs/desktop/menu-bar.md title table). File's
 *       first ink is x=43 in both s7_desktop.png and s8_desktop_menubar.png.
 *       Ink is the title ink (wctb part2 #000000).
 * ART   each title's pixels equal the approved strike chicago12.art (the
 *       hand-authored source the packed header is generated from), read here
 *       by this file's own parser and laid out cell by cell from the title's
 *       first ink column; the strike's top row is bar row 2 (its cap rows 3..11
 *       land on rows 5..13, as in s7_desktop.png). The glyph shapes are
 *       authored, not Apple's (CLAUDE.md, "Hazards that are still true"), so
 *       the art, not a capture, is their authority.
 * ORDER the InitechPaint bar carries the frame's eight titles in the frame's
 *       order, File Edit Image Layer Select View Window Help
 *       (goldens/frame/transcription.tsv MB.1..MB.8, all certain).
 *
 * Title positions after File are not asserted: the product's spacing is
 * wider than both captures (initech-vhgid), and the captured positions are
 * the acceptance test of that fix.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"
#include "menu.h"
#include "test_assert.h"

TEST_HARNESS();

#ifndef CHICAGO12_ART
#error "CHICAGO12_ART must name the approved strike"
#endif

/* ---- the approved strike, read independently of tools/font_art2h.c ---- */
typedef struct { int lb, aw; char rows[16][24]; int ok; } art_glyph;
static art_glyph g_art[128];

static int load_art(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];
    int code = -1, row = 0;
    if (!f) return 0;
    while (fgets(line, sizeof line, f)) {
        unsigned c; int lb, aw;
        if (line[0] == ';' || line[0] == '\n') continue;
        if (sscanf(line, "glyph 0x%x lb %d aw %d", &c, &lb, &aw) == 3) {
            code = c < 128u && aw > 0 && aw < 24 ? (int)c : -1;
            row = 0;
            if (code >= 0) { g_art[code].lb = lb; g_art[code].aw = aw; }
            continue;
        }
        if (strncmp(line, "glyph", 5) == 0) { code = -1; continue; }
        if (code < 0 || row >= 16) continue;
        int n = (int)strcspn(line, "\r\n");
        if (n != g_art[code].aw) { fclose(f); return 0; }
        memcpy(g_art[code].rows[row], line, (size_t)n);
        if (++row == 16) g_art[code].ok = 1;
    }
    fclose(f);
    return 1;
}

/* ---- literal references ---- */
static const uint32_t ROW_RGB[20] = {
    0xFFFFFF, 0xE7E7E7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xE7E7E7, 0xB3B3B3, 0x000000 };
static const char *const CORNER[9] = {   /* menus.md 1.2, x=0..14 */
    "KKKKKsceWWWWWWW", "KKKscWWWeeeeeee", "KKseWeeeeeeeeee",
    "KseWeeeeeeeeeee", "KcWeeeeeeeeeeee", "sWeeeeeeeeeeeee",
    "cWeeeeeeeeeeeee", "eWeeeeeeeeeeeee", "Weeeeeeeeeeeeee" };
static uint32_t corner_rgb(char c)
{
    switch (c) {
    case 'K': return 0x000000; case 's': return 0x777777;
    case 'c': return 0xC0C0C0; case 'e': return 0xE7E7E7;
    default:  return 0xFFFFFF;
    }
}
typedef struct { const char *t; int w, y0, y1; } title_box;
static const title_box FINDER[4] = {          /* s7_desktop.png */
    { "File", 21, 5, 13 }, { "Edit", 23, 5, 13 },
    { "View", 30, 5, 13 }, { "Special", 44, 5, 15 } };
static const char *const PAINT[8] = {         /* frame MB.1..MB.8 */
    "File", "Edit", "Image", "Layer", "Select", "View", "Window", "Help" };

/* ---- the bar under test ---- */
static MenuItem g_item[] = { { "New", 0, 'N', 0, 1, 0 } };
static MenuInfo g_finder[4], g_paint[8];
static MenuBar g_bar;
static void draw(GrafPort *p) { DrawMenuBar(p, &g_bar, -1, NULL); }

static uint32_t px(render_ctx_t *r, int x, int y)
{
    return render_pixel_rgb(r, (uint32_t)x, (uint32_t)y) & 0xFFFFFFu;
}

/* Ink groups in rows 2..16, x=33..511 (titles only: the mark ends at 31,
 * the clock is not attached). A gap of 8 or more columns ends a title. */
static int groups(render_ctx_t *r, int g[][4], int cap)
{
    int n = 0, x = 33;
    while (x < 512) {
        int ink = 0;
        for (int y = 2; y <= 16; y++) ink |= px(r, x, y) != 0xE7E7E7;
        if (!ink) { x++; continue; }
        if (n && x - g[n - 1][1] - 1 < 8) g[n - 1][1] = x;
        else if (n < cap) { g[n][0] = g[n][1] = x; n++; }
        else return cap + 1;
        x++;
    }
    for (int k = 0; k < n; k++) {
        g[k][2] = 99; g[k][3] = -1;
        for (int xx = g[k][0]; xx <= g[k][1]; xx++)
            for (int y = 2; y <= 16; y++)
                if (px(r, xx, y) != 0xE7E7E7) {
                    if (y < g[k][2]) g[k][2] = y;
                    if (y > g[k][3]) g[k][3] = y;
                }
    }
    return n;
}

/* Pixels of one title against the art, from its first ink column. */
static int art_mismatch(render_ctx_t *r, const char *t, int first_ink, int *cells)
{
    int bad = 0, pen = first_ink - g_art[(unsigned char)t[0]].lb;
    *cells = 0;
    for (const char *c = t; *c; c++) {
        const art_glyph *a = &g_art[(unsigned char)*c];
        if (!a->ok) return -1;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < a->aw; x++) {
                uint32_t want = a->rows[y][x] == '#' ? 0x000000 : 0xE7E7E7;
                bad += px(r, pen + x, 2 + y) != want;
                (*cells)++;
            }
        pen += a->aw;
    }
    return bad;
}

static void run_depth(unsigned bpp)
{
    char msg[200];
    render_ctx_t r;
    render_boot_info_t b = { 0, 0, bpp, 640, 480 };
    int g[9][4];
    if (render_ctx_init(&r, &b)) { CHECK(0, "render context"); return; }

    g_bar = (MenuBar){ g_finder, 4, 1 };
    render_run(&r, draw);

    int bad = 0;
    for (int x = 15; x < 625; x++)
        for (int y = 0; y < 20; y++)
            if (y < 2 || y > 16) bad += px(&r, x, y) != ROW_RGB[y];
    snprintf(msg, sizeof msg, "%u bpp: bar rows 0/1/17/18/19 at x=15..624 (menus.md 1.1)", bpp);
    CHECK(bad == 0, msg);
    bad = 0;
    for (int y = 0; y < 9; y++)
        for (int x = 0; x < 15; x++) {
            uint32_t want = corner_rgb(CORNER[y][x]);
            bad += px(&r, x, y) != want;
            bad += px(&r, 639 - x, y) != want;
        }
    snprintf(msg, sizeof msg, "%u bpp: both rounded corners equal menus.md 1.2", bpp);
    CHECK(bad == 0, msg);

    int n = groups(&r, g, 9);
    snprintf(msg, sizeof msg, "%u bpp: four Finder titles drawn (%d ink groups)", bpp, n);
    CHECK(n == 4, msg);
    snprintf(msg, sizeof msg, "%u bpp: File first ink x=43 (both captures)", bpp);
    CHECK(n >= 1 && g[0][0] == 43, msg);
    for (int k = 0; k < 4 && k < n; k++) {
        int cells, w = g[k][1] - g[k][0] + 1;
        snprintf(msg, sizeof msg, "%u bpp: %s ink %d px wide, rows %d..%d (s7_desktop.png %d, %d..%d)",
                 bpp, FINDER[k].t, w, g[k][2], g[k][3], FINDER[k].w, FINDER[k].y0, FINDER[k].y1);
        CHECK(w == FINDER[k].w && g[k][2] == FINDER[k].y0 && g[k][3] == FINDER[k].y1, msg);
        int m = art_mismatch(&r, FINDER[k].t, g[k][0], &cells);
        snprintf(msg, sizeof msg, "%u bpp: %s equals chicago12.art, %d/%d cells differ",
                 bpp, FINDER[k].t, m, cells);
        CHECK(m == 0, msg);
    }

    g_bar = (MenuBar){ g_paint, 8, 1 };
    render_run(&r, draw);
    n = groups(&r, g, 9);
    snprintf(msg, sizeof msg, "%u bpp: eight InitechPaint titles drawn (%d ink groups)", bpp, n);
    CHECK(n == 8, msg);
    for (int k = 0; k < 8 && k < n; k++) {
        int cells, m = art_mismatch(&r, PAINT[k], g[k][0], &cells);
        snprintf(msg, sizeof msg, "%u bpp: title %d is \"%s\" in chicago12.art, %d/%d cells differ",
                 bpp, k + 1, PAINT[k], m, cells);
        CHECK(m == 0, msg);
    }
    render_ctx_free(&r);
}

int main(void)
{
    CHECK(load_art(CHICAGO12_ART), "chicago12.art reads with 16 rows of aw columns per glyph");
    for (int k = 0; k < 4; k++)
        g_finder[k] = (MenuInfo){ (int16_t)(512 + k), FINDER[k].t, g_item, 1, 0 };
    for (int k = 0; k < 8; k++)
        g_paint[k] = (MenuInfo){ (int16_t)(256 + k), PAINT[k], g_item, 1, 0 };
    run_depth(32u);
    run_depth(8u);
    return TEST_SUMMARY("test-menu-bar-art");
}

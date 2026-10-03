/*
 * tools/ppm_flair_app_launch_check.c -- the R3.7 APP LAUNCH screendump grader
 * (factory C; bead initech-tdnl.14).
 *
 * Grades ONE 640x480 P6 dump taken after FLAIR_APP_LAUNCH_SHOW_SPEC (the disk
 * tenant TENANTFX.EXE launched, resident, frontmost; pointer hidden). Every
 * expected value is derived HERE, independently of the renderer:
 *   - GEOMETRY from the fixture's own NEWWINDOW arguments (os/apps/tenantfx.asm
 *     WIN_* = (100,160)..(400,340)) and the locked chrome metrics arithmetic
 *     (spec/flair_app_launch_traces.mk "GEOMETRY"): content (101,182)..(379,339);
 *     TEXTDRAW (16,16) and (16,40) content-local -> global (117,198), (117,222);
 *   - COLOURS from the independent canon (spec/assets/color_canon.h
 *     flair_canon_rgb, ADR-0010 -- never the renderer's palette);
 *   - INK COUNTS from the LOCKED Chicago strike (spec/assets/chicago8x16.h) --
 *     the asset is spec-data, not the renderer's code: what is graded is that
 *     the gate put EXACTLY those glyphs, in black on white, at EXACTLY the
 *     content-local origin the tenant asked for (a 1-px shift, a clip error, a
 *     wrong colour token or a dropped call all change the count or the tones).
 * Legs (argv[1]): "show" (the window + its text, above), and the three BAND-2
 * legs of bead initech-cnpm (SETMBAR, a disk tenant's own menu bar):
 *   "bar-tenant"     band 2 is EXACTLY TENANTFX's own bar (File Edit Fixture);
 *   "bar-finder"     band 2 is EXACTLY the Finder's bar (the bar a clean exit
 *                    must restore in the R3.7 scene: the Finder was foreground
 *                    when the tenant launched);
 *   "bar-photoshop"  band 2 is EXACTLY the shell fallback bar (a tenant whose
 *                    SETMBAR was REFUSED has no bar of its own).
 * Each band-2 leg is a FULL-PIXEL differential over band 2's rows [20,40) and
 * columns [20,624) -- every pixel's expected tone is derived HERE (see
 * grade_bar), never read off the renderer. ASCII-clean (Rule 12). Host libc is
 * fine in the factory (Law 3).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */
#include "chicago8x16.h"   /* the LOCKED strike (-Ispec/assets)        */

#define TOL 2

static unsigned char *g_buf;
static long g_w, g_h;
static int g_fail;

static int read_uint(FILE *f, long *out)
{
    int c;
    long v;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    v = 0;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static unsigned int IDX(int i)
{
    return (unsigned int)(flair_canon_rgb((unsigned char)i) & 0x00FFFFFFu);
}

static int is_idx(int x, int y, int idx)
{
    const unsigned char *p = g_buf + ((long)y * g_w + x) * 3;
    unsigned int e = IDX(idx);
    return abs((int)p[0] - (int)((e >> 16) & 0xFFu)) <= TOL &&
           abs((int)p[1] - (int)((e >> 8) & 0xFFu)) <= TOL &&
           abs((int)p[2] - (int)(e & 0xFFu)) <= TOL;
}

static void expect(int x, int y, int idx, const char *what)
{
    if (!is_idx(x, y, idx)) {
        const unsigned char *p = g_buf + ((long)y * g_w + x) * 3;
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- %s at (%d,%d): "
                "RGB(%d,%d,%d), expected canon idx %d\n",
                what, x, y, p[0], p[1], p[2], idx);
        g_fail = 1;
    }
}

/* Popcount of the locked strike over `s` -- the ink a correct TEXTDRAW puts
 * down (8x16 cells, CHICAGO_CELL_W advance). */
static long strike_ink(const char *s)
{
    long n = 0;
    for (; *s; s++) {
        const unsigned char *g = chicago8x16_glyph((int)(unsigned char)*s);
        for (int r = 0; r < CHICAGO_CELL_H; r++)
            for (int c = 0; c < 8; c++)
                if (g[r] & (0x80u >> c)) n++;
    }
    return n;
}

/* A text band: exactly `want` black pixels, every other pixel white. */
static void text_band(const char *label, const char *s, int x0, int y0)
{
    long black = 0, white = 0, other = 0;
    long want = strike_ink(s);
    int x1 = x0 + (int)strlen(s) * CHICAGO_CELL_W;
    for (int y = y0; y < y0 + CHICAGO_CELL_H; y++)
        for (int x = x0; x < x1; x++) {
            if (is_idx(x, y, CIDX_BLACK)) black++;
            else if (is_idx(x, y, CIDX_WHITE)) white++;
            else other++;
        }
    if (black != want || other != 0) {
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- %s \"%s\" band "
                "(%d,%d)-(%d,%d): black=%ld (want %ld from the locked strike), "
                "white=%ld, other=%ld (want 0)\n",
                label, s, x0, y0, x1, y0 + CHICAGO_CELL_H, black, want, white,
                other);
        g_fail = 1;
    } else {
        printf("  %-6s PASS -- \"%s\": %ld ink px exactly, black-on-white\n",
               label, s, black);
    }
}

/* Every pixel of a rect is `idx`. */
static void solid(const char *label, int x0, int y0, int x1, int y1, int idx)
{
    long bad = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (!is_idx(x, y, idx)) bad++;
    if (bad != 0) {
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- %s (%d,%d)-(%d,%d): "
                "%ld pixel(s) are not canon idx %d\n", label, x0, y0, x1, y1,
                bad, idx);
        g_fail = 1;
    } else {
        printf("  %-6s PASS -- (%d,%d)-(%d,%d) all idx %d\n", label, x0, y0,
               x1, y1, idx);
    }
}

/* ---------------------------------------------------------------------------
 * THE BAND-2 GRADER (bead initech-cnpm).
 *
 * Every expected pixel is DERIVED here from four independent, hand-carried
 * sources -- the renderer (os/flair/menu.c DrawMenuBar, text.h text_draw,
 * flair_look's PART table) is never consulted:
 *   1. THE BAR PROFILE, sampled off real Mac OS 8.1 captures:
 *      ../system7-decomp/specs/sys8/menus.md Sec 1.1 -- bar row 0 #FFFFFF
 *      (top highlight), rows 1..17 #E7E7E7 (face), row 18 #B3B3B3 (bottom
 *      shadow), row 19 #000000 (baseline hairline, full width). The canon
 *      indices are the gray-ramp entries whose RGB IS that sampled value
 *      (spec/assets/color_canon.h _Static_asserts CIDX_PLAT_FACE == #E7E7E7,
 *      CIDX_PLAT_FRAME_SHADOW == #B3B3B3).
 *   2. THE STACKING: band 2 is the second 20-px bar, rows [20,40)
 *      (GetMBarHeight 20, spec/chrome_metrics.h; two stacked bars, PRD
 *      Appendix A -- the frame's chimera).
 *   3. THE TITLE LAYOUT, os/flair/menu.h Sec 5 re-derived by hand (the same
 *      arithmetic tools/ppm_flair_disk_windows_check.c carries for the
 *      Finder): first slot at x = 20 (FLAIR_MENU_APPLE_W) when the bar has
 *      the Apple slot, else 0; each slot is 8*len + 2*7 wide (Chicago is a
 *      FIXED 8-px cell, spec/assets/chicago8x16.h; pad 7 each side); the
 *      title's ink cell starts at slot + 7, row band-top + 2 (the 16-px cell
 *      centred in the 20-px bar: (20-16)/2).
 *   4. THE GLYPHS, the LOCKED Chicago strike (spec/assets/chicago8x16.h --
 *      spec-data, the same asset the "show" leg counts ink from): a set bit is
 *      black title ink, a clear bit is the face.
 * And the TITLE WORDS are hand-carried per bar below, each from its own
 * statement of record -- never from the bar structure the kernel drew.
 *
 * GRADED: rows 20..39 (all twenty) x columns [20,624). Column 20 starts past
 * the Apple slot (an Apple glyph is not what tells two app bars apart) and
 * past the rounded left corner; 624 stops before the rounded RIGHT corner
 * (menus.md Sec 1.2), the same stop ppm_flair_disk_windows_check uses. Every
 * graded pixel must be EXACTLY its derived tone: a different bar, a missing
 * swap, a shifted title or a hilite left behind all go RED.
 * ------------------------------------------------------------------------- */
#define BAND2_TOP     20   /* the second stacked bar (rows [20,40))          */
#define BAR_H         20   /* GetMBarHeight                                  */
#define BAR_APPLE_W   20   /* FLAIR_MENU_APPLE_W (menu.h Sec 1)              */
#define BAR_PAD        7   /* FLAIR_MENU_TITLE_PAD                           */
#define BAR_VPAD       2   /* (20 - 16) / 2                                  */
#define BAR_GX0       20
#define BAR_GX1      624

typedef struct BarSpec {
    const char        *name;
    const char *const *titles;
    int                n;
    int                has_apple;
} BarSpec;

/* TENANTFX's own bar -- os/apps/tenantfx.asm, the `mbar` resource it hands
 * SETMBAR (has_apple 1; titles File / Edit / Fixture). Transcribed by hand. */
static const char *const TENANT_TITLES[] = { "File", "Edit", "Fixture" };
/* The Finder's bar -- docs/design/GUI-remediation-R3-finder-design.md F4.2
 * (Apple slot + File Edit View Special Help), the same five words
 * ppm_flair_disk_windows_check's finderbar leg carries. */
static const char *const FINDER_TITLES[] = {
    "File", "Edit", "View", "Special", "Help"
};
/* The shell fallback bar -- the FROZEN canon string "File Edit Image Layer
 * Select View Window Help" (ADR-0004 D-3 / AM-4; spec/assets/menu_canon.h),
 * no Apple slot (the Photoshop bar has none). Transcribed by hand, NOT read
 * from menu_canon.h, so a corrupted canon would go RED here. */
static const char *const PHOTOSHOP_TITLES[] = {
    "File", "Edit", "Image", "Layer", "Select", "View", "Window", "Help"
};

static const BarSpec BAR_TENANT = { "TENANTFX", TENANT_TITLES, 3, 1 };
static const BarSpec BAR_FINDER = { "FINDER", FINDER_TITLES, 5, 1 };
static const BarSpec BAR_PHOTOSHOP = { "PHOTOSHOP", PHOTOSHOP_TITLES, 8, 0 };

/* The derived canon index of band-2 pixel (x,y) for bar `b`. */
static int bar_expected(const BarSpec *b, int x, int y)
{
    int r = y - BAND2_TOP;          /* bar-local row 0..19 */
    int slot = b->has_apple ? BAR_APPLE_W : 0;

    if (r == 0) return CIDX_WHITE;              /* top highlight          */
    if (r == BAR_H - 2) return CIDX_PLAT_FRAME_SHADOW;   /* #B3B3B3       */
    if (r == BAR_H - 1) return CIDX_BLACK;      /* baseline hairline      */
    for (int k = 0; k < b->n; k++) {
        int len = (int)strlen(b->titles[k]);
        int tx = slot + BAR_PAD;
        int gr = r - BAR_VPAD;
        if (x >= tx && x < tx + len * CHICAGO_CELL_W &&
            gr >= 0 && gr < CHICAGO_CELL_H) {
            const unsigned char *g =
                chicago8x16_glyph((int)(unsigned char)b->titles[k][(x - tx) / 8]);
            if (g[gr] & (0x80u >> ((x - tx) % 8))) return CIDX_BLACK;
        }
        slot += len * CHICAGO_CELL_W + 2 * BAR_PAD;
    }
    return CIDX_PLAT_FACE;                      /* #E7E7E7 face           */
}

static void grade_bar(const BarSpec *b)
{
    long bad = 0, ink = 0, want_ink = 0;
    int fx = -1, fy = -1, fe = -1;

    for (int y = BAND2_TOP; y < BAND2_TOP + BAR_H; y++)
        for (int x = BAR_GX0; x < BAR_GX1; x++) {
            int e = bar_expected(b, x, y);
            int is_text_row = (y - BAND2_TOP) > 0 && (y - BAND2_TOP) < BAR_H - 2;
            if (e == CIDX_BLACK && is_text_row) want_ink++;
            if (is_text_row && is_idx(x, y, CIDX_BLACK)) ink++;
            if (!is_idx(x, y, e)) {
                if (bad == 0) { fx = x; fy = y; fe = e; }
                bad++;
            }
        }
    if (bad != 0) {
        const unsigned char *p = g_buf + ((long)fy * g_w + fx) * 3;
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- band 2 is NOT the "
                "%s bar: %ld of %d graded pixel(s) differ from the derived bar "
                "(first at (%d,%d): RGB(%d,%d,%d), expected canon idx %d); title "
                "ink in band 2 = %ld px, the %s bar has %ld\n",
                b->name, bad, BAR_H * (BAR_GX1 - BAR_GX0), fx, fy, p[0], p[1],
                p[2], fe, ink, b->name, want_ink);
        g_fail = 1;
    } else {
        printf("  BAND2  PASS -- rows [%d,%d) x [%d,%d): all %d pixels are the "
               "derived %s bar (%ld title-ink px)\n", BAND2_TOP,
               BAND2_TOP + BAR_H, BAR_GX0, BAR_GX1,
               BAR_H * (BAR_GX1 - BAR_GX0), b->name, want_ink);
    }
}

int main(int argc, char **argv)
{
    FILE *f;
    long maxv;
    char magic[3] = {0, 0, 0};
    const BarSpec *bar = NULL;

    if (argc == 3 && strcmp(argv[1], "bar-tenant") == 0) bar = &BAR_TENANT;
    else if (argc == 3 && strcmp(argv[1], "bar-finder") == 0) bar = &BAR_FINDER;
    else if (argc == 3 && strcmp(argv[1], "bar-photoshop") == 0)
        bar = &BAR_PHOTOSHOP;
    if (argc != 3 || (bar == NULL && strcmp(argv[1], "show") != 0)) {
        fprintf(stderr, "usage: %s show|bar-tenant|bar-finder|bar-photoshop "
                "dump.ppm\n", argv[0]);
        return 2;
    }
    f = fopen(argv[2], "rb");
    if (f == NULL || fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0 ||
        read_uint(f, &g_w) || read_uint(f, &g_h) || read_uint(f, &maxv) ||
        g_w != 640 || g_h != 480 || maxv != 255) {
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- %s is not a "
                "640x480x255 P6\n", argv[2]);
        return 1;
    }
    g_buf = (unsigned char *)malloc((size_t)(g_w * g_h * 3));
    if (g_buf == NULL ||
        fread(g_buf, 1, (size_t)(g_w * g_h * 3), f) != (size_t)(g_w * g_h * 3)) {
        fprintf(stderr, "ppm_flair_app_launch_check: FAIL -- short raster\n");
        return 1;
    }
    fclose(f);

    if (bar != NULL) {
        printf(">>> ppm_flair_app_launch_check [%s]: %s\n", argv[1], argv[2]);
        grade_bar(bar);
        printf(">>> ppm_flair_app_launch_check [%s]: %s\n", argv[1],
               g_fail ? "RED" : "green");
        return g_fail ? 1 : 0;
    }

    printf(">>> ppm_flair_app_launch_check [show]: %s\n", argv[2]);
    /* The window exists where the tenant asked: its outer frame corners. */
    expect(100, 160, CIDX_BLACK, "tenant frame top-left corner");
    expect(399, 160, CIDX_BLACK, "tenant frame top-right corner");
    expect(100, 339, CIDX_BLACK, "tenant frame bottom-left corner");
    /* FILLRECT painted the content white -- including (300,300), which is
     * NOTES' gray content, and (200,320), which is bare teal desktop, when the
     * tenant window is NOT in front of them. */
    expect(105, 186, CIDX_WHITE, "content just inside the top-left");
    expect(300, 300, CIDX_WHITE, "content over NOTES (the tenant is FRONT)");
    expect(200, 320, CIDX_WHITE, "content over bare desktop");
    /* The two TEXTDRAW runs, exactly. */
    text_band("LINE1", "Loaded from disk", 117, 198);
    text_band("LINE2", "Click here to quit", 117, 222);
    /* No stray ink anywhere else in the content: the gap between the lines and
     * everything below the second one. */
    solid("GAP", 101, 214, 379, 222, CIDX_WHITE);
    solid("BELOW", 101, 238, 379, 339, CIDX_WHITE);
    solid("LEFT", 101, 182, 117, 214, CIDX_WHITE);

    if (g_fail) {
        printf(">>> ppm_flair_app_launch_check [show]: RED\n");
        return 1;
    }
    printf(">>> ppm_flair_app_launch_check [show]: green\n");
    return 0;
}

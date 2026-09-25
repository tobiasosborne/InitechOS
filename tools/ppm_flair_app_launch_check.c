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
 * Legs (argv[1]): "show".
 * ASCII-clean (Rule 12). Host libc is fine in the factory (Law 3).
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

int main(int argc, char **argv)
{
    FILE *f;
    long maxv;
    char magic[3] = {0, 0, 0};

    if (argc != 3 || strcmp(argv[1], "show") != 0) {
        fprintf(stderr, "usage: %s show dump.ppm\n", argv[0]);
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

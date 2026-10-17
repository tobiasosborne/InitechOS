/*
 * ppm_flair_boot_screen_check.c -- the FLAIR BOOT SCREEN pixel oracle
 * (bead initech-tdnl.105).
 *
 * Usage: ppm_flair_boot_screen_check <dump.ppm> <s8_startup_welcome.png>
 *
 * Grades a 640x480 screendump of the boot screen against the INDEPENDENT
 * golden ../system7-decomp/goldens/captures/s8_startup_welcome.png (the real
 * Mac OS 8.1 "Welcome to Mac OS" panel, minted under Basilisk II, mint-009) and
 * the logo cell against the locked strike spec/assets/boot_logo.h (itself
 * graded against the logo geometry by test-boot-logo). Zero tolerance.
 *
 * Legs (all must pass):
 *   GRAY    every pixel outside the panel (x 109..530, y 47..369) equals the
 *           capture: the 50 % gray and the rounded screen corners.
 *   CHROME  every panel pixel outside the well interior (x 146..492, y
 *           75..294) and the title band (x 111..528, y 314..329) equals the
 *           capture: frame, bevel, face, the inset well's shadow/frame/
 *           highlight.
 *   TITLE   the title band holds only face and black; its ink rows are
 *           exactly the capture's ink rows (the cap rows 317..325) and its ink
 *           box centre is within 1 px of the capture's (the panel centre).
 *   WELL    the well interior is white outside the logo cell.
 *   LOGO    the logo cell (x 196..440, y 98..265) equals the strike, each role
 *           in its DESIGN-REFERENCE Sec 7.0 colour (tops #8DDCDC desktop canon,
 *           dark #131313, light #D4D4D4, clear = the well white).
 *
 * Ref: re/mint-results-009.md Sec 2 (geometry), os/milton/boot_screen.h (the
 * painter, which this file does NOT include: its numbers are re-stated here
 * from the capture ledger). CLAUDE.md Law 2, Rule 6, Rule 12.
 */
#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "boot_logo.h"

typedef struct { int w, h; unsigned char *rgb; } img_t;

static int load_ppm(const char *path, img_t *im)
{
    FILE *f = fopen(path, "rb");
    int maxv;
    if (!f) return 0;
    if (fscanf(f, "P6 %d %d %d", &im->w, &im->h, &maxv) != 3 || maxv != 255) { fclose(f); return 0; }
    fgetc(f);
    im->rgb = malloc((size_t)im->w * im->h * 3);
    if (!im->rgb || fread(im->rgb, 3, (size_t)im->w * im->h, f) != (size_t)im->w * im->h) { fclose(f); return 0; }
    fclose(f);
    return 1;
}

static int load_png(const char *path, img_t *im)
{
    png_image p;
    memset(&p, 0, sizeof p);
    p.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&p, path)) return 0;
    p.format = PNG_FORMAT_RGB;
    im->w = (int)p.width; im->h = (int)p.height;
    im->rgb = malloc(PNG_IMAGE_SIZE(p));
    return im->rgb && png_image_finish_read(&p, NULL, im->rgb, 0, NULL) != 0;
}

static unsigned rgb(const img_t *im, int x, int y)
{
    const unsigned char *p = &im->rgb[3 * (y * im->w + x)];
    return ((unsigned)p[0] << 16) | ((unsigned)p[1] << 8) | p[2];
}

static int in_rect(int x, int y, int l, int t, int r, int b)
{
    return x >= l && x <= r && y >= t && y <= b;
}

static long fails;

static void report(const char *leg, long bad, int x, int y, unsigned got, unsigned want)
{
    if (bad) {
        printf("  FAIL %-6s %ld pixels; first at (%d,%d) got #%06X want #%06X\n",
               leg, bad, x, y, got, want);
        fails++;
    } else {
        printf("  ok   %s\n", leg);
    }
}

int main(int argc, char **argv)
{
    img_t d, g;
    if (argc != 3) { fprintf(stderr, "usage: %s <dump.ppm> <s8_startup_welcome.png>\n", argv[0]); return 2; }
    if (!load_ppm(argv[1], &d) || d.w != 640 || d.h != 480) { fprintf(stderr, "bad dump %s\n", argv[1]); return 2; }
    if (!load_png(argv[2], &g) || g.w != 640 || g.h != 480) { fprintf(stderr, "bad golden %s (mint-009 required)\n", argv[2]); return 2; }
    printf("ppm_flair_boot_screen_check: %s vs %s\n", argv[1], argv[2]);

    /* GRAY + CHROME: zero-tolerance equality with the capture. */
    {
        long bg = 0, bc = 0;
        int gx = 0, gy = 0, cx = 0, cy = 0;
        unsigned gg = 0, gw = 0, cg = 0, cw = 0;
        for (int y = 0; y < 480; y++) {
            for (int x = 0; x < 640; x++) {
                unsigned a = rgb(&d, x, y), b = rgb(&g, x, y);
                if (!in_rect(x, y, 109, 47, 530, 369)) {
                    if (a != b && !bg++) { gx = x; gy = y; gg = a; gw = b; }
                } else if (!in_rect(x, y, 146, 75, 492, 294) &&
                           !in_rect(x, y, 111, 314, 528, 329)) {
                    if (a != b && !bc++) { cx = x; cy = y; cg = a; cw = b; }
                }
            }
        }
        report("GRAY", bg, gx, gy, gg, gw);
        report("CHROME", bc, cx, cy, cg, cw);
    }

    /* TITLE: face + black only; ink rows == capture ink rows; centred. */
    {
        long bad = 0, dn = 0, gn = 0;
        int bx = 0, by = 0, rows_ok = 1, dl = 640, dr = -1, gl = 640, gr = -1;
        unsigned bgot = 0;
        for (int y = 314; y <= 329; y++) {
            int dink = 0, gink = 0;
            for (int x = 111; x <= 528; x++) {
                unsigned a = rgb(&d, x, y), b = rgb(&g, x, y);
                if (a != 0x000000u && a != 0xDDDDDDu && !bad++) { bx = x; by = y; bgot = a; }
                if (a == 0x000000u) { dink = 1; dn++; if (x < dl) dl = x; if (x > dr) dr = x; }
                if (b == 0x000000u) { gink = 1; gn++; if (x < gl) gl = x; if (x > gr) gr = x; }
            }
            if (dink != gink) rows_ok = 0;
        }
        report("TITLE", bad, bx, by, bgot, 0xDDDDDDu);
        if (!dn || !gn || !rows_ok) {
            printf("  FAIL TITLE   ink rows differ from the capture's (dump ink %ld px, capture %ld px)\n", dn, gn);
            fails++;
        } else {
            /* Ink BOX centre (the capture's "Welcome to Mac OS" spans x
             * 258..380: centre 319.0, the panel centre 319.5 rounded down). */
            double dc = (dl + dr) / 2.0, gc = (gl + gr) / 2.0;
            int ok = dc - gc <= 1.0 && gc - dc <= 1.0;
            printf("  %s TITLE  ink box x %d..%d (centre %.1f) vs capture %d..%d (centre %.1f)\n",
                   ok ? "ok  " : "FAIL", dl, dr, dc, gl, gr, gc);
            if (!ok) fails++;
        }
    }

    /* WELL + LOGO. */
    {
        static const unsigned role_rgb[4] = {0xFFFFFFu, 0x8DDCDCu, 0x131313u, 0xD4D4D4u};
        static unsigned char cell[BOOT_LOGO_H][BOOT_LOGO_W];
        const int lx = 196, ly = 98;
        long bw = 0, bl = 0, inked = 0;
        int wx = 0, wy = 0, lxx = 0, lyy = 0;
        unsigned wg = 0, lg = 0, lw = 0;
        int k = 0;
        for (int j = 0; j < BOOT_LOGO_H; j++) {
            int i = 0;
            while (i < BOOT_LOGO_W) {
                int code = boot_logo_rle[k] >> 6, len = (boot_logo_rle[k] & 63) + 1;
                k++;
                memset(&cell[j][i], code, (size_t)len);
                i += len;
            }
        }
        for (int y = 75; y <= 294; y++) {
            for (int x = 146; x <= 492; x++) {
                unsigned a = rgb(&d, x, y);
                if (in_rect(x, y, lx, ly, lx + BOOT_LOGO_W - 1, ly + BOOT_LOGO_H - 1)) {
                    unsigned want = role_rgb[cell[y - ly][x - lx]];
                    if (cell[y - ly][x - lx]) inked++;
                    if (a != want && !bl++) { lxx = x; lyy = y; lg = a; lw = want; }
                } else if (a != 0xFFFFFFu && !bw++) {
                    wx = x; wy = y; wg = a;
                }
            }
        }
        report("WELL", bw, wx, wy, wg, 0xFFFFFFu);
        report("LOGO", bl, lxx, lyy, lg, lw);
        printf("       logo cell %dx%d at (%d,%d), %ld inked strike pixels\n",
               BOOT_LOGO_W, BOOT_LOGO_H, lx, ly, inked);
    }

    if (fails) {
        printf("ppm_flair_boot_screen_check: FAIL (%ld legs)\n", fails);
        return 1;
    }
    printf("ppm_flair_boot_screen_check: PASS\n");
    return 0;
}

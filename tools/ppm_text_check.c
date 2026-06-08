/*
 * ppm_text_check.c -- boot-to-text framebuffer oracle (factory, C-only).
 *
 * beads: initech-bea ("InitechDOS banner + boot gate"). Ref: CLAUDE.md Law 2
 *        ("the oracle is the truth"), Rule 2 (fail fast/loud), Law 3 (factory
 *        is C), Law 4 (it must look like the frame -- text really rendered).
 *        docs/research/boot-to-text-ground-truth.md Sec 4.2 (screendump pixel
 *        assertion). PRD Sec 8 (the QMP screendump is a debug signal we assert).
 *
 * The STRONGER companion to tools/ppm_seafoam_check.c. The pure-seafoam grid
 * check is necessarily SUPERSEDED once the kernel blits the InitechDOS banner
 * (the banner inks fg pixels into the top rows, so a uniform-seafoam assertion
 * would go red). Rather than weaken anything (STOP CONDITION), this tool asserts
 * a STRICTLY STRONGER property: the banner was actually rendered AND the desktop
 * is still seafoam away from the text (so it is NOT a solid fill or a blank
 * screen). Three assertions, all must hold (exit 0 iff all pass):
 *
 *   (A) BANNER RENDERED: within the known banner band -- line 1 at cell row 0
 *       (y in [0,16)) across the columns where "InitechDOS  Version 3.30"
 *       renders (cols 0..23 => x in [0,192)) -- there exist at least
 *       MIN_FG_PIXELS foreground-colored pixels. The console fg is light gray
 *       RGB(0xC0,0xC0,0xC0) (os/milton/console.c CONSOLE_FG_*). Zero fg pixels
 *       => the blit never happened => RED.
 *
 *   (B) FIRST GLYPH PRESENT: the top-left glyph cell (0,0) -- where 'I' of
 *       "InitechDOS" renders -- contains at least one fg pixel. This pins the
 *       banner to its KNOWN origin (Sec 4.2(b)): a banner drawn somewhere else
 *       would miss this.
 *
 *   (C) BACKGROUND INTACT: a region well below the two-row banner band
 *       (y in [BG_Y0, height)) sampled on a coarse grid is still seafoam within
 *       tolerance. This proves the screen is the seafoam desktop with text on
 *       top -- not a solid fg fill, not a black screen, not garbage.
 *
 * Usage: ppm_text_check <screendump.ppm>
 * The expected colors, the banner band, and the background region are compiled
 * in (single source of truth, mirrored from os/milton/console.c CONSOLE_FG_*
 * and CONSOLE_BG_* and the 8x16 cell grid: cell (col,row) covers
 * x in [col*8, col*8+8), y in [row*16, row*16+16)).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Console foreground (ink) = light gray; mirrored from os/milton/console.c
 * CONSOLE_FG_R/G/B. The banner glyphs are inked in this color. */
#define FG_R 0xC0
#define FG_G 0xC0
#define FG_B 0xC0

/* Console background = SEAFOAM; mirrored from os/milton/console.c
 * CONSOLE_BG_R/G/B (== stage2.asm SEAFOAM_R/G/B). */
#define BG_R 0x6F
#define BG_G 0xA0
#define BG_B 0x8E

#define TOL 8 /* per-channel tolerance for bpp rounding (matches seafoam check) */

/* 8x16 cell grid. Banner line 1 = "InitechDOS  Version 3.30" (24 chars) at
 * cell row 0 => y in [0,16), cols 0..23 => x in [0,192). */
#define CELL_W 8
#define CELL_H 16
#define LINE1_COLS 24
#define LINE1_X0 0
#define LINE1_X1 (LINE1_COLS * CELL_W) /* 192 */
#define LINE1_Y0 0
#define LINE1_Y1 CELL_H                 /* 16 */

/* Empirically (a clean boot) line 1 inks ~600 fg pixels; require a healthy
 * fraction so a stray-pixel artifact cannot satisfy the assertion, while the
 * margin stays generous against font choice. The 'I' cell alone has >=20. */
#define MIN_FG_PIXELS 80

/* Background region: everything from y=BG_Y0 down is below the two-row banner
 * (rows 0..1 => y<32), so it must be pure seafoam desktop. We start well clear
 * of the banner to avoid any descender/edge effects. */
#define BG_Y0 240

/* Skip PPM whitespace and '#' comment lines (same as ppm_seafoam_check). */
static int read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        c = fgetc(f);
    }
    *out = v;
    return 0;
}

static int is_color(const unsigned char *p, int r, int g, int b)
{
    return abs((int)p[0] - r) <= TOL &&
           abs((int)p[1] - g) <= TOL &&
           abs((int)p[2] - b) <= TOL;
}

int main(int argc, char **argv)
{
    /* argv[1] = the PPM. OPTIONAL (backward-compatible): argv[2..4] =
     * band_y0 band_y1 min_fg -- additionally assert that the horizontal band
     * y in [band_y0, band_y1) contains >= min_fg foreground pixels. test-boot
     * calls with 1 arg (banner only); test-fs passes the proto-DIR band so the
     * gate BITES if the directory listing fails to render on screen (Rule 6 --
     * the gate previously false-passed on the banner alone). */
    long band_y0 = -1, band_y1 = -1, band_min = 0;
    if (argc == 5) {
        band_y0  = strtol(argv[2], NULL, 10);
        band_y1  = strtol(argv[3], NULL, 10);
        band_min = strtol(argv[4], NULL, 10);
    } else if (argc != 2) {
        fprintf(stderr, "usage: %s <screendump.ppm> [band_y0 band_y1 min_fg]\n",
                argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "ppm_text_check: cannot open %s\n", argv[1]);
        return 2;
    }

    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "ppm_text_check: not a P6 PPM (got '%s')\n", magic);
        fclose(f);
        return 2;
    }

    long w = 0, h = 0, maxv = 0;
    if (read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_text_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (w <= 0 || h <= 0 || maxv != 255) {
        fprintf(stderr,
                "ppm_text_check: unexpected dims/maxval w=%ld h=%ld max=%ld\n",
                w, h, maxv);
        fclose(f);
        return 2;
    }
    /* read_uint() already consumed the single whitespace byte after maxval, so
     * the position is exactly at the start of the raster (same invariant as
     * ppm_seafoam_check -- do NOT skip another byte). */

    long npix = w * h;
    unsigned char *buf = malloc((size_t)npix * 3);
    if (!buf) {
        fprintf(stderr, "ppm_text_check: OOM\n");
        fclose(f);
        return 2;
    }
    size_t got = fread(buf, 3, (size_t)npix, f);
    fclose(f);
    if (got != (size_t)npix) {
        fprintf(stderr, "ppm_text_check: short raster (%zu of %ld pixels)\n",
                got, npix);
        free(buf);
        return 2;
    }

    /* The banner band must fit on screen (defensive; the LFB is 640x480). */
    if (w < LINE1_X1 || h <= BG_Y0) {
        fprintf(stderr,
                "ppm_text_check: image %ldx%ld too small for banner band/bg region\n",
                w, h);
        free(buf);
        return 2;
    }

    /* ---- (A) BANNER RENDERED: count fg pixels in the line-1 band. ---- */
    long fg_line1 = 0;
    for (long y = LINE1_Y0; y < LINE1_Y1; y++) {
        for (long x = LINE1_X0; x < LINE1_X1; x++) {
            if (is_color(buf + (y * w + x) * 3, FG_R, FG_G, FG_B)) {
                fg_line1++;
            }
        }
    }

    /* ---- (B) FIRST GLYPH PRESENT: fg pixel(s) in cell (0,0). ---- */
    long fg_cell00 = 0;
    for (long y = 0; y < CELL_H; y++) {
        for (long x = 0; x < CELL_W; x++) {
            if (is_color(buf + (y * w + x) * 3, FG_R, FG_G, FG_B)) {
                fg_cell00++;
            }
        }
    }

    /* ---- (C) BACKGROUND INTACT: coarse seafoam grid below the banner. ---- */
    int bg_checked = 0, bg_bad = 0;
    long fbx = -1, fby = -1;
    int br = 0, bg = 0, bb = 0;
    for (long y = BG_Y0; y < h; y += 7) {
        for (long x = 0; x < w; x += 7) {
            unsigned char *p = buf + (y * w + x) * 3;
            bg_checked++;
            if (!is_color(p, BG_R, BG_G, BG_B)) {
                if (bg_bad == 0) {
                    fbx = x; fby = y; br = p[0]; bg = p[1]; bb = p[2];
                }
                bg_bad++;
            }
        }
    }
    /* ---- (D) OPTIONAL BAND: fg text in a caller-specified band (e.g. the
     * proto-DIR listing). Counts fg pixels across the full width in
     * [band_y0, band_y1). ---- */
    long fg_band = 0;
    int band_checked = 0;
    if (band_y0 >= 0 && band_y1 > band_y0 && band_y1 <= h) {
        band_checked = 1;
        for (long y = band_y0; y < band_y1; y++) {
            for (long x = 0; x < w; x++) {
                if (is_color(buf + (y * w + x) * 3, FG_R, FG_G, FG_B)) {
                    fg_band++;
                }
            }
        }
    }
    free(buf);

    printf("ppm_text_check: %ldx%ld -- banner band fg=%ld (>=%d req), "
           "cell(0,0) fg=%ld (>=1 req), bg seafoam %d/%d below y=%d",
           w, h, fg_line1, MIN_FG_PIXELS, fg_cell00,
           bg_checked - bg_bad, bg_checked, BG_Y0);
    if (band_checked) {
        printf(", band[%ld,%ld) fg=%ld (>=%ld req)",
               band_y0, band_y1, fg_band, band_min);
    }
    printf("\n");

    int fail = 0;
    if (band_checked && fg_band < band_min) {
        fprintf(stderr,
                "ppm_text_check: FAIL [D] -- only %ld fg pixels in band "
                "[%ld,%ld) (need >=%ld); the expected text did NOT render there\n",
                fg_band, band_y0, band_y1, band_min);
        fail = 1;
    }
    if (fg_line1 < MIN_FG_PIXELS) {
        fprintf(stderr,
                "ppm_text_check: FAIL [A] -- only %ld fg pixels in the banner "
                "band (need >=%d); the banner was NOT rendered\n",
                fg_line1, MIN_FG_PIXELS);
        fail = 1;
    }
    if (fg_cell00 < 1) {
        fprintf(stderr,
                "ppm_text_check: FAIL [B] -- no fg pixels in the top-left glyph "
                "cell (0,0); the banner is not at its known origin\n");
        fail = 1;
    }
    if (bg_bad != 0) {
        fprintf(stderr,
                "ppm_text_check: FAIL [C] -- %d/%d bg samples below y=%d are not "
                "seafoam; first at (%ld,%ld) = RGB(%d,%d,%d) -- solid fill / "
                "blank / garbage, not the seafoam desktop\n",
                bg_bad, bg_checked, BG_Y0, fbx, fby, br, bg, bb);
        fail = 1;
    }
    if (fail) {
        return 1;
    }

    printf("ppm_text_check: PASS -- banner rendered at the known origin AND the "
           "desktop is still seafoam below it\n");
    return 0;
}

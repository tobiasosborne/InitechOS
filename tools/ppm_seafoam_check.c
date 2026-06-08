/*
 * ppm_seafoam_check.c -- tracer-boot framebuffer oracle (factory, C-only).
 *
 * beads: initech-f8v.2 ("Tracer boot ..."). Ref: CLAUDE.md Law 2 ("the
 *        oracle is the truth"), Rule 2 (fail fast/loud), Law 3 (factory is C).
 *        PRD Sec 8 (QMP screendump is a debug signal we assert against).
 *
 * Parses a QEMU QMP screendump (binary PPM, "P6") and asserts that a sample
 * of pixels equals the seafoam color SEAFOAM_RGB = (0x6F,0xA0,0x8E), within a
 * per-channel tolerance (bpp rounding when the LFB is 24/32bpp can perturb the
 * low bits a touch; QEMU's PPM dump is exact for our XRGB8888 path, so a small
 * tolerance is generous). Exits 0 if seafoam, non-zero otherwise -- this is
 * the visual half of the boot oracle.
 *
 * Usage: ppm_seafoam_check <screendump.ppm>
 * The expected RGB and tolerance are compiled in (single source of truth,
 * mirrored from os/boot/stage2.asm SEAFOAM_R/G/B).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mirror of os/boot/stage2.asm SEAFOAM_R/G/B (documented placeholder). */
#define SEAFOAM_R 0x6F
#define SEAFOAM_G 0xA0
#define SEAFOAM_B 0x8E
#define TOL 8 /* per-channel tolerance for bpp rounding */

/* Skip PPM whitespace and '#' comment lines. */
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

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <screendump.ppm>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "ppm_seafoam_check: cannot open %s\n", argv[1]);
        return 2;
    }

    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "ppm_seafoam_check: not a P6 PPM (got '%s')\n", magic);
        fclose(f);
        return 2;
    }

    long w = 0, h = 0, maxv = 0;
    if (read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_seafoam_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (w <= 0 || h <= 0 || maxv != 255) {
        fprintf(stderr,
                "ppm_seafoam_check: unexpected dims/maxval w=%ld h=%ld max=%ld\n",
                w, h, maxv);
        fclose(f);
        return 2;
    }
    /* read_uint() already consumed the single whitespace byte that follows
     * maxval (it reads one byte past the last digit to detect end-of-number),
     * so the file position is now exactly at the start of the raster. Do NOT
     * skip another byte here -- that would drop the first pixel's red channel. */

    long npix = w * h;
    unsigned char *buf = malloc((size_t)npix * 3);
    if (!buf) {
        fprintf(stderr, "ppm_seafoam_check: OOM\n");
        fclose(f);
        return 2;
    }
    size_t got = fread(buf, 3, (size_t)npix, f);
    fclose(f);
    if (got != (size_t)npix) {
        fprintf(stderr,
                "ppm_seafoam_check: short raster (%zu of %ld pixels)\n",
                got, npix);
        free(buf);
        return 2;
    }

    /* Sample a grid of points across the image; every one must be seafoam. */
    const int GRID = 9; /* 9x9 = 81 sample points */
    int checked = 0, bad = 0;
    int first_bad_x = -1, first_bad_y = -1, br = 0, bg = 0, bb = 0;
    for (int gy = 0; gy < GRID; gy++) {
        for (int gx = 0; gx < GRID; gx++) {
            long x = (long)((gx + 0.5) / GRID * w);
            long y = (long)((gy + 0.5) / GRID * h);
            if (x >= w) x = w - 1;
            if (y >= h) y = h - 1;
            unsigned char *p = buf + (y * w + x) * 3;
            int r = p[0], g = p[1], b = p[2];
            checked++;
            if (abs(r - SEAFOAM_R) > TOL || abs(g - SEAFOAM_G) > TOL ||
                abs(b - SEAFOAM_B) > TOL) {
                if (bad == 0) {
                    first_bad_x = (int)x; first_bad_y = (int)y;
                    br = r; bg = g; bb = b;
                }
                bad++;
            }
        }
    }
    free(buf);

    printf("ppm_seafoam_check: %ldx%ld, sampled %d points, "
           "expect RGB(%d,%d,%d) tol +/-%d\n",
           w, h, checked, SEAFOAM_R, SEAFOAM_G, SEAFOAM_B, TOL);
    if (bad != 0) {
        fprintf(stderr,
                "ppm_seafoam_check: FAIL -- %d/%d samples not seafoam; "
                "first bad at (%d,%d) = RGB(%d,%d,%d)\n",
                bad, checked, first_bad_x, first_bad_y, br, bg, bb);
        return 1;
    }
    printf("ppm_seafoam_check: PASS -- all %d samples are seafoam\n", checked);
    return 0;
}

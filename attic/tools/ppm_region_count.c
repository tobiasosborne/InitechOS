/*
 * tools/ppm_region_count.c -- count the pixels of a rectangle of a 640x480 P6
 * screendump that differ from / equal a REFERENCE pixel of the same dump
 * (factory C; bead initech-w96l / initech-8zii / initech-9u8w).
 *
 * The disk-tenant gates (test-flair-ctenant, test-flair-i123) grade WHERE a
 * tenant's drawing landed and THAT it reached the screen, not glyph shapes:
 * the glyph shapes are the VGA BIOS's own ROM strike (spec/toolbox_gate.h Sec
 * 7a), which the factory does not hold. The reference pixel is sampled from
 * the dump itself (a known-blank content point, or a known-filled bar), so
 * the role colours (spec Sec 7: tokens, never RGB) are never hard-coded here.
 *
 * usage: ppm_region_count dump.ppm L T R B REFX REFY
 *   prints "ne=<n> eq=<n> ref=<rrggbb>" for the pixels (x,y), L<=x<R, T<=y<B,
 *   that are != / == the pixel at (REFX,REFY). Exit 0 (the gate judges), 2 on
 *   a malformed dump or arguments (fail loud).
 * ASCII-clean (Rule 12). Deterministic.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_uint(FILE *f, unsigned *out)
{
    int c;
    unsigned v = 0;
    do {
        c = fgetc(f);
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); }
    } while (c == ' ' || c == '\n' || c == '\r' || c == '\t');
    if (c < '0' || c > '9') return 1;
    while (c >= '0' && c <= '9') { v = v * 10u + (unsigned)(c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

int main(int argc, char **argv)
{
    char magic[3] = { 0, 0, 0 };
    unsigned w, h, maxv, ne = 0, eq = 0;
    long a[6];
    unsigned char *buf;
    const unsigned char *ref;
    FILE *f;

    if (argc != 8) {
        fprintf(stderr, "usage: %s dump.ppm L T R B REFX REFY\n", argv[0]);
        return 2;
    }
    for (int i = 0; i < 6; i++) a[i] = strtol(argv[2 + i], NULL, 10);
    f = fopen(argv[1], "rb");
    if (f == NULL || fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0 ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != 640 || h != 480 || maxv != 255) {
        fprintf(stderr, "ppm_region_count: %s is not a 640x480x255 P6\n", argv[1]);
        return 2;
    }
    buf = (unsigned char *)malloc((size_t)w * h * 3u);
    if (buf == NULL || fread(buf, 1, (size_t)w * h * 3u, f) != (size_t)w * h * 3u) {
        fprintf(stderr, "ppm_region_count: short raster\n");
        return 2;
    }
    fclose(f);
    if (a[0] < 0 || a[1] < 0 || a[2] > (long)w || a[3] > (long)h || a[0] >= a[2] ||
        a[1] >= a[3] || a[4] < 0 || a[4] >= (long)w || a[5] < 0 || a[5] >= (long)h) {
        fprintf(stderr, "ppm_region_count: rectangle or reference off the 640x480 dump\n");
        return 2;
    }
    ref = buf + ((size_t)a[5] * w + (size_t)a[4]) * 3u;
    for (long y = a[1]; y < a[3]; y++)
        for (long x = a[0]; x < a[2]; x++) {
            const unsigned char *p = buf + ((size_t)y * w + (size_t)x) * 3u;
            if (p[0] == ref[0] && p[1] == ref[1] && p[2] == ref[2]) eq++;
            else ne++;
        }
    printf("ne=%u eq=%u ref=%02x%02x%02x\n", ne, eq, ref[0], ref[1], ref[2]);
    free(buf);
    return 0;
}

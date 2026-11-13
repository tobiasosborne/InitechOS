/*
 * ppm_flair_cursor_check.c -- R0.1 booted CursorMgr pixel oracle.
 *
 * beads: initech-tdnl.1. The test-flair-cursor scaffold injects two (+90,+70)
 * moves after FLAIR-LIVE-READY, taking Event Manager's center (320,240) to the
 * known hotspot (500,380), then captures after FLAIR-CURSOR x=500 y=380.
 *
 * M009 (initech-tdnl.98): grade ALL 256 cells at zero tolerance against
 * the real Mac OS 8 arrow crop, using the same independent capture loader
 * as the host suite. Clear cells must retain the canon seafoam desktop
 * (#8DDCDC, spec/assets/color_canon.json); no renderer tables are included.
 * Ref: audit 2026-10-15-look-pass6/REPORT.md M009; PRD Sec 6.3.
 *
 * Usage: ppm_flair_cursor_check dump.ppm s8_alert_modal.png s8_desktop_menubar.png
 *                               [hotspot_x hotspot_y [reference.ppm]]
 *
 * bead initech-vzjy (test-flair-cursor-boot): the optional hotspot grades the
 * SAME 256 cells at another position -- the Mac OS 8.1 boot position (320,240)
 * (../system7-decomp/re/mint-results-009.md Sec 1; arrow crop origin (319,239)
 * in s8_startup_panel/last/after.png and s8_icons_boot.png). The optional
 * reference PPM is a cursor-free still of the SAME desktop (the arrow tracked
 * elsewhere by a second boot): clear cells must then equal the reference pixel
 * instead of the bare seafoam, because the boot position lies over whatever the
 * desktop composited there. Without a reference the seafoam rule stands.
 * ASCII-only; deterministic; factory C (ADR-0002 / CLAUDE.md Law 3).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cursor_capture.h"

enum {
    EXPECT_W = 640,
    EXPECT_H = 480,
    CURSOR_X = 500,
    CURSOR_Y = 380
};

typedef struct image {
    int w;
    int h;
    unsigned char *pixels;
} image_t;

static int read_ppm(const char *path, image_t *im)
{
    FILE *f = fopen(path, "rb");
    char magic[3] = { 0, 0, 0 };
    int maxv = 0;
    size_t count;

    if (!f) {
        fprintf(stderr, "ppm_flair_cursor_check: cannot open %s\n", path);
        return 1;
    }
    if (fscanf(f, "%2s %d %d %d", magic, &im->w, &im->h, &maxv) != 4 ||
        strcmp(magic, "P6") != 0 || maxv != 255 ||
        im->w < EXPECT_W || im->h < EXPECT_H ||
        im->w > 4096 || im->h > 4096) {
        fprintf(stderr,
                "ppm_flair_cursor_check: %s is not a sane >=640x480 P6 PPM\n",
                path);
        fclose(f);
        return 1;
    }
    (void)fgetc(f);
    count = (size_t)im->w * (size_t)im->h * 3u;
    im->pixels = (unsigned char *)malloc(count);
    if (!im->pixels) {
        fprintf(stderr, "ppm_flair_cursor_check: OOM\n");
        fclose(f);
        return 1;
    }
    if (fread(im->pixels, 1u, count, f) != count) {
        fprintf(stderr, "ppm_flair_cursor_check: short PPM payload\n");
        free(im->pixels);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

int main(int argc, char **argv)
{
    image_t im = { 0, 0, NULL };
    image_t ref = { 0, 0, NULL };
    char cells[256];
    int bad = 0;
    int hx = CURSOR_X;
    int hy = CURSOR_Y;
    if (argc != 4 && argc != 6 && argc != 7) {
        fprintf(stderr, "usage: %s dump.ppm s8_alert_modal.png s8_desktop_menubar.png "
                        "[hotspot_x hotspot_y [reference.ppm]]\n", argv[0]);
        return 2;
    }
    if (argc >= 6) {
        hx = atoi(argv[4]);
        hy = atoi(argv[5]);
        /* The whole 16x16 crop (origin = hotspot - 1) must lie on the panel. */
        if (hx < 1 || hy < 1 || hx + 15 > EXPECT_W || hy + 15 > EXPECT_H) {
            fprintf(stderr, "ppm_flair_cursor_check: hotspot (%d,%d) puts the crop off-screen\n", hx, hy);
            return 2;
        }
    }
    if (cursor_capture_cells(argv[2], argv[3], cells) || read_ppm(argv[1], &im)) return 2;
    if (argc == 7) {
        if (read_ppm(argv[6], &ref)) { free(im.pixels); return 2; }
        if (ref.w != im.w || ref.h != im.h) {
            fprintf(stderr, "ppm_flair_cursor_check: reference %s is %dx%d, dump is %dx%d\n",
                    argv[6], ref.w, ref.h, im.w, im.h);
            free(im.pixels);
            free(ref.pixels);
            return 2;
        }
    }
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            char cell = cells[y * 16 + x];
            size_t at = 3u * ((size_t)(hy - 1 + y) * (size_t)im.w + (size_t)(hx - 1 + x));
            const unsigned char *p = im.pixels + at;
            unsigned want;
            unsigned got = p[0] * 65536u + p[1] * 256u + p[2];
            if (cell == '#') want = 0;
            else if (cell == 'o') want = 0xFFFFFFu;
            else if (ref.pixels) want = ref.pixels[at] * 65536u + ref.pixels[at + 1] * 256u + ref.pixels[at + 2];
            else want = 0x8DDCDCu;
            if (got != want) {
                if (!bad) fprintf(stderr, "cursor M009 first difference cell=(%d,%d): #%06X expected #%06X\n",
                                  x, y, got, want);
                bad++;
            }
        }
    }
    printf("ppm_flair_cursor_check: %s M009 hotspot=(%d,%d) %d/256 differing cells "
           "(zero tolerance; capture ink=54 paper=43 clear=159%s)\n", bad ? "RED" : "PASS", hx, hy, bad,
           ref.pixels ? "; clear cells vs reference still" : "");
    free(im.pixels);
    free(ref.pixels);
    return bad ? 1 : 0;
}

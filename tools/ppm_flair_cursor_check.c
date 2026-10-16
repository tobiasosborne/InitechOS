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
    char cells[256];
    int bad = 0;
    if (argc != 4) {
        fprintf(stderr, "usage: %s dump.ppm s8_alert_modal.png s8_desktop_menubar.png\n", argv[0]);
        return 2;
    }
    if (cursor_capture_cells(argv[2], argv[3], cells) || read_ppm(argv[1], &im)) return 2;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            char cell = cells[y * 16 + x];
            unsigned want = cell == '#' ? 0 : cell == 'o' ? 0xFFFFFFu : 0x8DDCDCu;
            const unsigned char *p = im.pixels +
                3u * ((size_t)(CURSOR_Y - 1 + y) * (size_t)im.w + CURSOR_X - 1 + x);
            unsigned got = p[0] * 65536u + p[1] * 256u + p[2];
            if (got != want) {
                if (!bad) fprintf(stderr, "cursor M009 first difference cell=(%d,%d): #%06X expected #%06X\n",
                                  x, y, got, want);
                bad++;
            }
        }
    }
    printf("ppm_flair_cursor_check: %s M009 hotspot=(500,380) %d/256 differing cells "
           "(zero tolerance; capture ink=54 paper=43 clear=159)\n", bad ? "RED" : "PASS", bad);
    free(im.pixels);
    return bad ? 1 : 0;
}

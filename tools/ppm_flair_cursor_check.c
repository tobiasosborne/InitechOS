/*
 * ppm_flair_cursor_check.c -- R0.1 booted CursorMgr pixel oracle.
 *
 * beads: initech-tdnl.1. The test-flair-cursor scaffold injects one (+80,+40)
 * move after FLAIR-LIVE-READY, taking Event Manager's center (320,240) to the
 * known hotspot (400,280), then captures after FLAIR-CURSOR x=400 y=280.
 *
 * INDEPENDENT GOLDEN (CLAUDE.md Law 2): expected pixels and coordinates are
 * hard-coded here from the clean-room strike review, not read from cursor.c or
 * cursor.h. With CURS hotspot (1,1), black data row 1/col 1 lands exactly at
 * (400,280); the mask-only pixel row 0/col 1 lands at (400,279) and is white.
 * Ref: Inside Macintosh: Imaging With QuickDraw, CURS data/mask/hotspot layout;
 *      docs/plans/GUI-remediation-plan.md R0.1; PRD Sec 6.3.
 *
 * Usage: ppm_flair_cursor_check <dump.ppm>
 * ASCII-only; deterministic; factory C (ADR-0002 / CLAUDE.md Law 3).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    EXPECT_W = 640,
    EXPECT_H = 480,
    CURSOR_X = 400,
    CURSOR_Y = 280,
    TOL = 2
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

static int close_channel(int got, int expected)
{
    int d = got - expected;
    if (d < 0) d = -d;
    return d <= TOL;
}

static int probe_rgb(const image_t *im, int x, int y,
                     int er, int eg, int eb, const char *what)
{
    const unsigned char *p = im->pixels +
        3u * ((size_t)y * (size_t)im->w + (size_t)x);
    if (!close_channel((int)p[0], er) ||
        !close_channel((int)p[1], eg) ||
        !close_channel((int)p[2], eb)) {
        fprintf(stderr,
                "ppm_flair_cursor_check: FAIL %s at (%d,%d): "
                "sampled #%02X%02X%02X expected #%02X%02X%02X (+/-%d)\n",
                what, x, y, p[0], p[1], p[2], er, eg, eb, TOL);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    image_t im;
    int failed = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <dump.ppm>\n", argv[0]);
        return 2;
    }
    im.pixels = (unsigned char *)0;
    if (read_ppm(argv[1], &im)) return 2;

    failed |= probe_rgb(&im, CURSOR_X, CURSOR_Y, 0, 0, 0,
                        "black arrow-tip data pixel is absent");
    failed |= probe_rgb(&im, CURSOR_X, CURSOR_Y - 1, 255, 255, 255,
                        "white mask-only outline pixel is absent");
    if (!failed) {
        printf("ppm_flair_cursor_check: PASS hotspot=(400,280) "
               "black tip + white outline\n");
    }
    free(im.pixels);
    return failed ? 1 : 0;
}

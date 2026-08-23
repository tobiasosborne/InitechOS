/* ppm_flair_window_ops_check.c -- R1 zoom/grow/collapse screendump oracle.
 *
 * Factory C only. Pixel VALUES come from the independent color canon; expected
 * operation endpoints come from the locked trace arithmetic and the tenant
 * scene contract, never from WindowMgr state. Ref: GUI-remediation-plan.md
 * R1.2; beads initech-tdnl.2/tbef/ci4o/cjfr; CLAUDE.md Law 2/3/12.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"

typedef struct Img {
    int w;
    int h;
    unsigned char *buf;
} Img;

static int read_ppm(const char *path, Img *im)
{
    FILE *f = fopen(path, "rb");
    int maxv;
    size_t n;
    if (!f) {
        fprintf(stderr, "window_ops_check: cannot open %s\n", path);
        return 1;
    }
    if (fscanf(f, "P6 %d %d %d", &im->w, &im->h, &maxv) != 3 ||
        im->w <= 0 || im->h <= 0 || maxv != 255) {
        fprintf(stderr, "window_ops_check: %s is not a sane P6 PPM\n", path);
        fclose(f);
        return 1;
    }
    if (fgetc(f) == EOF) {
        fclose(f);
        return 1;
    }
    n = (size_t)im->w * (size_t)im->h * 3u;
    im->buf = (unsigned char *)malloc(n);
    if (!im->buf || fread(im->buf, 1u, n, f) != n) {
        fprintf(stderr, "window_ops_check: %s truncated\n", path);
        free(im->buf);
        fclose(f);
        return 1;
    }
    fclose(f);
    return 0;
}

static int probe(const Img *im, int x, int y, unsigned char idx,
                 const char *label)
{
    uint32_t rgb;
    const unsigned char *p;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    if (x < 0 || y < 0 || x >= im->w || y >= im->h) {
        fprintf(stderr, "FAIL %s: probe (%d,%d) outside %dx%d\n",
                label, x, y, im->w, im->h);
        return 1;
    }
    rgb = flair_canon_rgb(idx);
    r = (unsigned char)((rgb >> 16) & 0xFFu);
    g = (unsigned char)((rgb >> 8) & 0xFFu);
    b = (unsigned char)(rgb & 0xFFu);
    p = im->buf + ((size_t)y * (size_t)im->w + (size_t)x) * 3u;
    if (p[0] != r || p[1] != g || p[2] != b) {
        fprintf(stderr,
                "FAIL %s at (%d,%d): got #%02X%02X%02X want #%02X%02X%02X\n",
                label, x, y, p[0], p[1], p[2], r, g, b);
        return 1;
    }
    return 0;
}

/* Locked scene endpoint: HELLO frame [60,360)x[60,260). */
static int zoom_toggle(const Img *im)
{
    int bad = 0;
    bad |= probe(im, 100, 60, CIDX_BLACK,
                 "zoom_toggle restored HELLO top frame");
    bad |= probe(im, 100, 100, CIDX_WHITE,
                 "zoom_toggle restored HELLO content");
    bad |= probe(im, 359, 100, CIDX_BLACK,
                 "zoom_toggle restored HELLO right frame");
    bad |= probe(im, 500, 60, CIDX_DESKTOP,
                 "zoom_toggle removed standard-state right extension");
    bad |= probe(im, 10, 100, CIDX_DESKTOP,
                 "zoom_toggle removed standard-state left extension");
    if (!bad)
        printf("window ops zoom_toggle PASS: exact user frame restored after in/out\n");
    return bad;
}

/* Locked grow trace shrinks HELLO 300x200 -> clamped 96x64 at (60,60). */
static int grow(const Img *im)
{
    int bad = 0;
    bad |= probe(im, 100, 60, CIDX_BLACK,
                 "grow resized HELLO top frame");
    bad |= probe(im, 155, 90, CIDX_BLACK,
                 "grow resized HELLO right frame");
    bad |= probe(im, 156, 90, CIDX_BLACK,
                 "grow resized HELLO right shadow");
    bad |= probe(im, 100, 100, CIDX_WHITE,
                 "grow repainted resized HELLO content");
    bad |= probe(im, 100, 130, CIDX_DESKTOP,
                 "grow exposed old body to desktop");
    bad |= probe(im, 200, 100, CIDX_DESKTOP,
                 "grow exposed old right body to desktop");
    if (!bad)
        printf("window ops grow PASS: HELLO is 96x64 with old body exposed\n");
    return bad;
}

/* Locked collapse endpoint: HELLO frame [60,360)x[60,82), shadow row y=82. */
static int collapse(const Img *im)
{
    int bad = 0;
    bad |= probe(im, 100, 60, CIDX_BLACK,
                 "collapse title top frame");
    bad |= probe(im, 100, 81, CIDX_BLACK,
                 "collapse title bottom frame");
    bad |= probe(im, 100, 82, CIDX_BLACK,
                 "collapse bottom shadow");
    bad |= probe(im, 100, 83, CIDX_DESKTOP,
                 "collapse pixel below shadow is desktop");
    bad |= probe(im, 100, 100, CIDX_DESKTOP,
                 "collapse removed full old body");
    if (!bad)
        printf("window ops collapse PASS: title band only; body fully exposed\n");
    return bad;
}

int main(int argc, char **argv)
{
    Img im;
    int rc;
    if (argc != 3) {
        fprintf(stderr,
                "usage: ppm_flair_window_ops_check <zoom_toggle|grow|collapse> <dump.ppm>\n");
        return 2;
    }
    if (read_ppm(argv[2], &im)) return 2;
    if (strcmp(argv[1], "zoom_toggle") == 0)
        rc = zoom_toggle(&im);
    else if (strcmp(argv[1], "grow") == 0)
        rc = grow(&im);
    else if (strcmp(argv[1], "collapse") == 0)
        rc = collapse(&im);
    else {
        fprintf(stderr, "window_ops_check: unknown leg %s\n", argv[1]);
        rc = 2;
    }
    free(im.buf);
    return rc;
}

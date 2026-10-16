/* Independent runtime oracle input for initech-bf2 (Law 2): the Mac OS 8.1
 * Platinum determinate progress bar, read from the PNG captures themselves.
 * Ref: ../system7-decomp/re/mint-results-s8-progress.md;
 *      goldens/captures/s8_progress_determinate{_start,,_full}.png, 640x480.
 * The bar's etched rect is x=190..377, y=133..146 in all three captures
 * (interior L=192..R=375, rows 135..144; fill edge E=193 / 274 / 375).
 *
 * NEVER include artifact drawing tables here (spec/progress_platinum.h is the
 * artifact's transcription; this header only ever reads the PNG pixels).
 * Expected pixels for a bar of ANY width are built by COLUMN-CLASS extension:
 * every expected value is copied from a golden column of the same class
 * (frame/etch ends, fill left columns, body, cap relative to E, track
 * relative to T, the last track column), then the clut-208 lavender rungs are
 * mapped through the ratified lavender->teal rule (DEC-10 OQ-2/OQ-3; the SAME
 * H/P/S/R mapping spec/scrollbar_look_golden.h sl_mapped_rgb grades, plus the
 * #000077 cap -> the menu title's dark edge). Factory C, hosted. ASCII. */
#ifndef INITECH_HARNESS_PROGRESS_GOLDEN_H
#define INITECH_HARNESS_PROGRESS_GOLDEN_H

#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PG_X0 190
#define PG_Y0 133
#define PG_W  188
#define PG_H  14
#define PG_L  192
#define PG_R  375
#define PG_NONE 0xFFFFFFFFu

typedef struct {
    unsigned char *start, *partial, *full;  /* RGB 640x480 each */
    int e_start, e_partial, e_full;         /* measured fill edges */
} pg_golden_t;

static unsigned char *pg_load_one(const char *name)
{
    const char *root = getenv("SYSTEM7_DECOMP");
    char path[1024];
    png_image im;
    unsigned char *pixels;
    if (!root || !*root) root = "../system7-decomp";
    snprintf(path, sizeof path, "%s/goldens/captures/%s", root, name);
    memset(&im, 0, sizeof im);
    im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_file(&im, path)) {
        fprintf(stderr, "FAIL independent progress capture: %s: %s\n", path, im.message);
        png_image_free(&im);
        return NULL;
    }
    im.format = PNG_FORMAT_RGB;
    if (im.width != 640 || im.height != 480) {
        fprintf(stderr, "FAIL independent progress capture dimensions: %s\n", path);
        png_image_free(&im);
        return NULL;
    }
    pixels = malloc(PNG_IMAGE_SIZE(im));
    if (!pixels || !png_image_finish_read(&im, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "FAIL independent progress capture decode: %s\n", path);
        free(pixels);
        pixels = NULL;
    }
    png_image_free(&im);
    return pixels;
}

static uint32_t pg_rgb(const unsigned char *p, int x, int y)
{
    const unsigned char *q = p + 3 * (y * 640 + x);
    return (uint32_t)q[0] * 65536u + (uint32_t)q[1] * 256u + q[2];
}

/* The fill edge on interior row 4 (the white highlight row): the first black
 * column inside the interior, or -1. */
static int pg_edge(const unsigned char *p)
{
    for (int x = PG_L; x <= PG_R; x++)
        if (pg_rgb(p, x, PG_Y0 + 6) == 0x000000u) return x;
    return -1;
}

/* Load and sanity-check the three captures: the black frame must be where the
 * ledger says, the edges must be the ledger's, and every body column of the
 * partial capture must be identical (the class-extension premise). */
static int pg_golden_load(pg_golden_t *g)
{
    memset(g, 0, sizeof *g);
    g->start = pg_load_one("s8_progress_determinate_start.png");
    g->partial = pg_load_one("s8_progress_determinate.png");
    g->full = pg_load_one("s8_progress_determinate_full.png");
    if (!g->start || !g->partial || !g->full) return 0;
    const unsigned char *all[3] = { g->start, g->partial, g->full };
    for (int i = 0; i < 3; i++) {
        for (int x = PG_X0 + 1; x <= PG_X0 + PG_W - 2; x++)
            if (pg_rgb(all[i], x, PG_Y0 + 1) != 0 || pg_rgb(all[i], x, PG_Y0 + PG_H - 2) != 0) {
                fprintf(stderr, "FAIL progress golden %d: black frame not at the ledger rect\n", i);
                return 0;
            }
    }
    g->e_start = pg_edge(g->start);
    g->e_partial = pg_edge(g->partial);
    g->e_full = pg_edge(g->full);
    if (g->e_start != 193 || g->e_partial != 274 || g->e_full != 375) {
        fprintf(stderr, "FAIL progress golden edges %d/%d/%d != ledger 193/274/375\n",
                g->e_start, g->e_partial, g->e_full);
        return 0;
    }
    for (int x = PG_L + 3; x <= g->e_partial - 4; x++)
        for (int r = 0; r < 10; r++)
            if (pg_rgb(g->partial, x, PG_Y0 + 2 + r) != pg_rgb(g->partial, PG_L + 2, PG_Y0 + 2 + r)) {
                fprintf(stderr, "FAIL progress golden body column %d differs\n", x);
                return 0;
            }
    return 1;
}

static void pg_golden_free(pg_golden_t *g)
{
    free(g->start); free(g->partial); free(g->full);
    memset(g, 0, sizeof *g);
}

/* lavender (clut 208) -> the ratified teal roles; grays unchanged.
 * depth 8: the locked indexed palette's tones (S/R #3F3F3F, cap black);
 * otherwise direct colour (S/R #4E9BA3, cap #003F45). */
static uint32_t pg_map(uint32_t rgb, int depth)
{
    switch (rgb) {
    case 0xDADAFFu: return 0xFFFFFFu;
    case 0xB3B3FFu: return 0x8DDCDCu;
    case 0x8787DAu: case 0x5454B3u: return depth == 8 ? 0x3F3F3Fu : 0x4E9BA3u;
    case 0x000077u: return depth == 8 ? 0x000000u : 0x003F45u;
    default: return rgb;
    }
}

/* Expected RGB of rect-relative pixel (dx,dy) of a w-wide, PG_H-tall bar
 * whose fill count (edge included) is f; PG_NONE where nothing is drawn. */
static uint32_t pg_expected(const pg_golden_t *g, int w, int f, int dx, int dy, int depth)
{
    int h = PG_H, gx, gy = PG_Y0 + dy;
    const unsigned char *src = g->start;
    if ((dx == w - 1 && dy == 0) || (dx == 0 && dy == h - 1)) return PG_NONE;
    if (dy <= 1 || dy >= h - 2 || dx <= 1 || dx >= w - 2) {
        gx = dx <= 1 ? PG_X0 + dx : dx >= w - 2 ? PG_X0 + PG_W - (w - dx) : PG_X0 + 100;
        return pg_map(pg_rgb(g->start, gx, gy), depth);
    }
    int L = 2, R = w - 3, x = dx;
    int E = L + f - 1, T = f > 0 ? E + 1 : L;
    if (f > 0 && x == E) {
        gx = g->e_start;
    } else if (f > 0 && x < E) {
        if (f <= 6) { gx = PG_L; src = g->start; }
        else {
            src = f == R - L + 1 ? g->full : g->partial;
            int ge = f == R - L + 1 ? g->e_full : g->e_partial;
            if (x <= L + 1) gx = PG_L + (x - L);
            else if (x >= E - 3) gx = ge - (E - x);
            else gx = PG_L + 2;
        }
    } else {
        int gt = g->e_start + 1;          /* the start capture's track T */
        if (x == T) gx = gt;
        else if (x == T + 1) gx = gt + 1;
        else if (x == R) gx = PG_R;
        else gx = gt + 10;
    }
    return pg_map(pg_rgb(src, gx, gy), depth);
}

#endif

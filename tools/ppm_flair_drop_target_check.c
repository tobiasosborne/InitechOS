/*
 * tools/ppm_flair_drop_target_check.c -- grade the MID-DRAG frame of the R3.4a
 * Finder drag (bead initech-34dh): README.TXT held over the APPS folder in the
 * unmoved root disk window, button still down (spec/flair_file_ops_traces.mk
 * trace 5, FLAIR_FILEOPS_HOVER_SPEC). Factory C tool (Law 3).
 *
 * WHAT IS ASSERTED (every expectation hand-derived; nothing read back from the
 * renderer -- Law 2 / HER-02):
 *   1. THE TARGET IS HIGHLIGHTED. The APPS folder sprite at (107,86) shows the
 *      highlighted tones of os/flair/finder_icon.h :: finder_icon_draw_hilite:
 *        row 10 col 10 (a 'w' body pixel of the folder ASCII map,
 *          spec/assets/finder_icons.h) == #777777  (body, darkened)
 *        row 25 col 10 (a 'g' flap pixel)            == #3F3F3F  (detail, darkened)
 *        row  7 col 10 (the '#' flap edge)           == #000000  (ink unchanged)
 *      The RGB values are the two EXISTING canon ramp rows named in
 *      spec/assets/color_canon.h (CIDX_HILITE_FRAME 119 "#777777",
 *      CIDX_PLAT_DARK_RING 63 "#3F3F3F") -- Inside Macintosh VI p. 2-19..2-20
 *      "a black-and-white icon turns gray when selected".
 *   2. ITS NAME IS INVERTED: inside the APPS label band sub-rect
 *      x [111,135) y [121,131) black outnumbers white (the selected look).
 *   3. NOTHING ELSE IS LIT: README.TXT (39,86), DESKTOP.DB (175,86) (both DOC,
 *      row 10 col 12 'w') and TRASH (243,86) (FOLDER, row 10 col 10 'w') keep a
 *      WHITE #FFFFFF body.
 *   4. THE GRAY OUTLINE IS ON SCREEN: README's cell translated by (+68,0) has
 *      its top edge on row y=86; at (110,86) and (115,86) -- APPS's transparent
 *      rows, window-content white without a drag -- the pixel is the outline
 *      gray #777777 (FLAIR_PART_PLAT_INACTIVE_FRAME, color_canon.h 119).
 * Probes avoid the arrow's 16x16 footprint at (123,102)..(139,118).
 *
 * usage: ppm_flair_drop_target_check <dump.ppm>   exit 0 PASS, 1 FAIL, 2 usage
 * Ref: os/flair/finder_ops.h ("WHAT A USER SEES" 1 and 2); CLAUDE.md Law 2,
 *      Rule 12.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCRW 640
#define SCRH 480
#define TOL  2

static unsigned char *g_buf;
static int g_fail;

static const unsigned char *at(int x, int y)
{
    return g_buf + ((long)y * SCRW + x) * 3;
}

static int is_rgb(int x, int y, unsigned int rgb)
{
    const unsigned char *p = at(x, y);
    return abs((int)p[0] - (int)((rgb >> 16) & 0xFFu)) <= TOL &&
           abs((int)p[1] - (int)((rgb >> 8) & 0xFFu)) <= TOL &&
           abs((int)p[2] - (int)(rgb & 0xFFu)) <= TOL;
}

static void want(int x, int y, unsigned int rgb, const char *what)
{
    if (!is_rgb(x, y, rgb)) {
        const unsigned char *p = at(x, y);
        fprintf(stderr, "ppm_flair_drop_target_check: FAIL -- %s\n"
                "    at (%d,%d): sampled #%02X%02X%02X, expected #%06X\n",
                what, x, y, p[0], p[1], p[2], rgb);
        g_fail = 1;
    }
}

static int read_uint(FILE *f, long *out)
{
    int c;
    long v = 0;
    do {
        c = fgetc(f);
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); }
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    if (c < '0' || c > '9') return -1;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long w, h, maxv;
    size_t need;
    long black = 0, white = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: ppm_flair_drop_target_check <dump.ppm>\n");
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (f == NULL) { fprintf(stderr, "ppm_flair_drop_target_check: cannot open %s\n", argv[1]); return 2; }
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0 ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr, "ppm_flair_drop_target_check: not a %dx%d P6/255 dump\n", SCRW, SCRH);
        fclose(f);
        return 2;
    }
    need = (size_t)SCRW * SCRH * 3u;
    g_buf = (unsigned char *)malloc(need);
    if (g_buf == NULL || fread(g_buf, 1, need, f) != need) {
        fprintf(stderr, "ppm_flair_drop_target_check: short raster\n");
        fclose(f);
        free(g_buf);
        return 2;
    }
    fclose(f);

    /* 1. the target, highlighted */
    want(107 + 10, 86 + 10, 0x777777u, "APPS body pixel is the HIGHLIGHTED #777777");
    want(107 + 10, 86 + 25, 0x3F3F3Fu, "APPS flap pixel is the HIGHLIGHTED #3F3F3F");
    want(107 + 10, 86 + 7,  0x000000u, "APPS ink edge stays black");

    /* 2. its name inverted */
    for (int y = 121; y < 131; y++)
        for (int x = 111; x < 135; x++) {
            if (is_rgb(x, y, 0x000000u)) black++;
            else if (is_rgb(x, y, 0xFFFFFFu)) white++;
        }
    if (!(black > white && white > 0)) {
        fprintf(stderr, "ppm_flair_drop_target_check: FAIL -- the APPS label "
                "band is not inverted (black=%ld white=%ld)\n", black, white);
        g_fail = 1;
    }

    /* 3. nothing else lit */
    want(39 + 12,  86 + 10, 0xFFFFFFu, "README.TXT (the dragged icon) keeps a white body");
    want(175 + 12, 86 + 10, 0xFFFFFFu, "DESKTOP.DB is not a target: white body");
    want(243 + 10, 86 + 10, 0xFFFFFFu, "TRASH folder is not under the pointer: white body");

    /* 4. the gray outline */
    want(110, 86, 0x777777u, "the drag outline's top edge (README cell +68,0)");
    want(115, 86, 0x777777u, "the drag outline's top edge (README cell +68,0)");

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_drop_target_check: FAILED on %s\n", argv[1]);
        return 1;
    }
    printf("ppm_flair_drop_target_check: PASS on %s (APPS highlighted #777777/"
           "#3F3F3F + inverted name; 3 non-targets white; gray outline present)\n",
           argv[1]);
    return 0;
}

/*
 * ppm_flair_trash_check.c -- the EMPTY TRASH screendump STRUCTURE oracle
 *                            (factory, C).
 *
 * bead: initech-6k12 (Special > Empty Trash: the confirm alert, the FULL Trash
 *       icon and its swap back).
 *
 * Legs (one QEMU screendump, P6 PPM 640x480, of the booted $(FLAIRTENANTS_IMG)):
 *   alert    README.TXT staged, Special > Empty Trash chosen, the alert UP:
 *              the alert frame at the corpus geometry, the message in three
 *              inked lines in the text column, OK with the default ring and
 *              Cancel without, and the desktop Trash drawn FULL.
 *   emptied  after OK: the alert gone (its footprint repainted by what lies
 *              beneath) and the desktop Trash drawn EMPTY again.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2):
 *   - the alert geometry: MEASURED off the corpus capture
 *     ../system7-decomp/goldens/captures/s8_alert_modal.png (frame x=133..506
 *     y=87..190; text ink from x 211, cap tops y 100/116/132; Cancel
 *     x=363..421, OK x=435..493, ring x=432..496, buttons y=158..177 --
 *     specs/sys8/INDEX-sys8.md agrees), restated here by hand;
 *   - the Trash strikes: hand-read off the ASCII maps in
 *     spec/assets/desk_icons.h (this file never includes it);
 *   - the colours: flair_canon_rgb (spec/assets/color_canon.h), the
 *     independently decomp-graded canon, never the renderer's palette.
 *   - the dBoxProc frame art is the existing Dialog Manager's (a 7-px black
 *     band, FLAIR_CHROME_DIALOG_BORDER); the Platinum alert frame is bead
 *     initech-81ft's, so only "a black band where the frame is" is graded.
 *
 * Usage: ppm_flair_trash_check <alert|emptied> <dump.ppm>
 * Exit 0 iff every assertion passes; otherwise a loud message per failure.
 * Ref: CLAUDE.md Law 1, Law 2, Law 4, Rule 2, Rule 6, Rule 12.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */

#define TOL 2
enum { SCRW = 640, SCRH = 480 };

/* The alert, measured (see the banner). */
#define AL_L 133
#define AL_T  87
#define AL_R 507             /* exclusive: the capture's last column is 506   */
#define AL_B 191
#define AL_BORDER 7          /* dBoxProc band (FLAIR_CHROME_DIALOG_BORDER)     */
#define TXT_X0 211
#define TXT_X1 496           /* 211 + the 285-px column                        */
#define TXT_Y0  97           /* line k occupies [97+16k, 113+16k)               */
#define OK_L 435
#define OK_R 494
#define CA_L 363
#define CA_R 422
#define BT_T 158
#define BT_B 178

/* The desktop Trash sprite origin (spec/flair_trash_traces.mk: (584,404)). */
#define TR_X 584
#define TR_Y 404

static unsigned char *g_buf;
static int g_fail;
static const char *g_leg;

static int read_uint(FILE *f, long *out)
{
    int c;
    long v = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static int is_idx(int x, int y, int idx)
{
    const unsigned char *p = g_buf + ((long)y * SCRW + x) * 3;
    unsigned int e = (unsigned int)(flair_canon_rgb((unsigned char)idx) & 0x00FFFFFFu);
    return abs((int)p[0] - (int)((e >> 16) & 0xFFu)) <= TOL &&
           abs((int)p[1] - (int)((e >> 8) & 0xFFu)) <= TOL &&
           abs((int)p[2] - (int)(e & 0xFFu)) <= TOL;
}

static void want(int x, int y, int idx, const char *what)
{
    if (!is_idx(x, y, idx)) {
        const unsigned char *p = g_buf + ((long)y * SCRW + x) * 3;
        fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- %s at (%d,%d): "
                "sampled #%02X%02X%02X, want canon idx %d\n",
                g_leg, what, x, y, p[0], p[1], p[2], idx);
        g_fail = 1;
    }
}

static long ink(int x0, int y0, int x1, int y1)
{
    long n = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (is_idx(x, y, CIDX_BLACK)) n++;
    return n;
}

static void want_ink(int x0, int y0, int x1, int y1, int some, const char *what)
{
    long n = ink(x0, y0, x1, y1);
    printf("    %-52s [%d,%d)x[%d,%d): %ld ink px\n", what, x0, x1, y0, y1, n);
    if (some ? (n == 0) : (n != 0)) {
        fprintf(stderr, "ppm_flair_trash_check: FAIL leg %s -- %s: %ld ink "
                "pixels in [%d,%d)x[%d,%d), want %s\n",
                g_leg, what, n, x0, x1, y0, y1, some ? "some" : "none");
        g_fail = 1;
    }
}

/* The FULL strike (desk_icons.h FLAIR_DESK_ICON_TRASH_FULL map), by hand:
 *   row 2 col 10 '#' (the lid, ajar)      row 3 col 12 'g' (lid face)
 *   row 6 col  8 'w' (paper)              row 2 col  7 '.' (clear)
 *   row 5 col 11 '.' (between the balls)
 * The EMPTY strike (FLAIR_DESK_ICON_TRASH map), by hand:
 *   row 2 col 10 '.' (only the handle, cols 13..18, on row 2)
 *   row 2 col 13 '#'   row 4 col 7 '#' (lid slab)   row 5 col 10 'g' */
static void trash_full(void)
{
    want(TR_X + 10, TR_Y + 2, CIDX_BLACK,   "FULL Trash: lid ink (row 2 col 10)");
    want(TR_X + 12, TR_Y + 3, CIDX_CONTROL, "FULL Trash: lid face shade (row 3 col 12)");
    want(TR_X +  8, TR_Y + 6, CIDX_WHITE,   "FULL Trash: crumpled paper (row 6 col 8)");
    want(TR_X +  7, TR_Y + 2, CIDX_DESKTOP, "FULL Trash: clear left of the ajar lid (row 2 col 7)");
    want(TR_X + 11, TR_Y + 5, CIDX_DESKTOP, "FULL Trash: clear between paper balls (row 5 col 11)");
}

static void trash_empty(void)
{
    want(TR_X + 10, TR_Y + 2, CIDX_DESKTOP, "EMPTY Trash: no lid on row 2 (row 2 col 10 clear)");
    want(TR_X + 13, TR_Y + 2, CIDX_BLACK,   "EMPTY Trash: the handle (row 2 col 13)");
    want(TR_X +  7, TR_Y + 4, CIDX_BLACK,   "EMPTY Trash: lid slab left end (row 4 col 7)");
    want(TR_X + 10, TR_Y + 5, CIDX_CONTROL, "EMPTY Trash: lid face shade (row 5 col 10)");
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long w = 0, h = 0, maxv = 0;
    size_t n = (size_t)SCRW * SCRH * 3u;

    if (argc != 3 || (strcmp(argv[1], "alert") && strcmp(argv[1], "emptied") &&
                     strcmp(argv[1], "locked") && strcmp(argv[1], "locked-full"))) {
        fprintf(stderr, "usage: %s <alert|emptied|locked|locked-full> <dump.ppm>\n", argv[0]);
        return 2;
    }
    g_leg = argv[1];
    f = fopen(argv[2], "rb");
    if (!f || fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6' ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr, "ppm_flair_trash_check: %s is not a 640x480/255 P6 PPM\n", argv[2]);
        if (f) fclose(f);
        return 2;
    }
    g_buf = (unsigned char *)malloc(n);
    if (!g_buf || fread(g_buf, 1, n, f) != n) {
        fprintf(stderr, "ppm_flair_trash_check: short raster\n");
        fclose(f);
        return 2;
    }
    fclose(f);
    printf("ppm_flair_trash_check: leg %s on %s\n", g_leg, argv[2]);

    if (strcmp(g_leg, "alert") == 0 || strcmp(g_leg, "locked") == 0 ||
        strcmp(g_leg, "locked-full") == 0) {
        /* the dBoxProc band on all four sides, mid-edge */
        want(AL_L + AL_BORDER / 2, 140, CIDX_BLACK, "alert frame band, left");
        want(AL_R - 1 - AL_BORDER / 2, 140, CIDX_BLACK, "alert frame band, right");
        want(300, AL_T + AL_BORDER / 2, CIDX_BLACK, "alert frame band, top");
        want(300, AL_B - 1 - AL_BORDER / 2, CIDX_BLACK, "alert frame band, bottom");
        want(AL_L - 1, 140, CIDX_WHITE, "just outside the alert (root window content)");
        want(300, 150, CIDX_PLAT_FACE, "alert face between the message and the buttons");
        /* the message: three inked lines in the text column, nothing below */
        for (int k = 0; k < 3; k++) {
            char what[64];
            snprintf(what, sizeof what, "message line %d", k + 1);
            want_ink(TXT_X0, TXT_Y0 + 16 * k, TXT_X1, TXT_Y0 + 16 * (k + 1), 1, what);
        }
        want_ink(TXT_X0, TXT_Y0 + 48, CA_L - 3, BT_B, 0, "no fourth message line");
        want_ink(AL_L + AL_BORDER + 1, TXT_Y0, TXT_X0 - 2, TXT_Y0 + 48, 0,
                 "nothing left of the text column");
        /* the buttons: labels inked; OK carries the default ring, Cancel not */
        want_ink(OK_L + 4, BT_T + 3, OK_R - 4, BT_B - 2, 1, "OK button label");
        want_ink(CA_L + 4, BT_T + 3, CA_R - 4, BT_B - 2,
                 strcmp(g_leg, "alert") == 0, "Cancel only in confirmation");
        want((OK_L + OK_R) / 2, BT_T - 3, CIDX_BLACK, "OK default ring (InsetRect -3)");
        want((CA_L + CA_R) / 2, BT_T - 3, CIDX_PLAT_FACE, "Cancel has NO ring");
        if (strcmp(g_leg, "locked") == 0) trash_empty(); else trash_full();
    } else {
        want(AL_L + AL_BORDER / 2, 140, CIDX_WHITE,
             "the alert is gone: its left band repainted by the Trash window content");
        want(300, AL_B - 1 - AL_BORDER / 2, CIDX_WHITE,
             "the alert is gone: its bottom band repainted by the Trash window content");
        trash_empty();
    }
    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_trash_check: leg %s FAILED\n", g_leg);
        return 1;
    }
    printf("ppm_flair_trash_check: leg %s PASS\n", g_leg);
    return 0;
}

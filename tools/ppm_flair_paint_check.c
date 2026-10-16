/*
 * tools/ppm_flair_paint_check.c -- InitechPaint screendump grader (factory C,
 * host; bead initech-tdnl.20, the bar-carrier half). The BAR itself is graded
 * by ppm_flair_app_launch_check bar-photoshop (band 2 pixel-exact against the
 * hand-carried canon titles in the real NFNT 5478 metrics); this tool grades:
 *
 *   canvas dump.ppm  the blank document: the canvas content (global x 340..600,
 *                    y 180..370: inside PAINT's (330,150)-(630,400) window,
 *                    clear of its frame, title bar and scroll bars) is
 *                    entirely white -- InitechPaint opens an EMPTY canvas.
 *   menu dump.ppm    the Edit pull-down at FLAIR-MENU-XDROP (button held, no
 *                    item tracked): the panel hangs from the Edit title slot
 *                    (x = StringWidth("File") + 2*9, the real NFNT advances,
 *                    menu.h Sec 5 re-derived by hand), and its five item rows
 *                    carry DISABLED ink only (#A5A5A5, sys8/menus.md Sec 2.3 /
 *                    menu_look) -- not one black glyph pixel: every Edit item
 *                    is period-grayed.
 *
 * Exit 0 PASS, 1 named FAIL, 2 unusable input. ASCII-clean (Rule 12).
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "chicago12_nfnt_golden.h"

static unsigned char pixels[640 * 480 * 3];
static uint32_t px(int x, int y)
{
    if (x < 0 || x >= 640 || y < 0 || y >= 480) return 0xFFFFFFFFu;
    const unsigned char *p = pixels + 3 * (y * 640 + x);
    return (uint32_t)p[0] * 65536u + (uint32_t)p[1] * 256u + p[2];
}
static int sw(const char *s)
{
    int w = 0;
    for (; *s; s++) w += NFNT5478_AW[(unsigned char)*s - NFNT5478_FIRST];
    return w;
}

static int canvas(void)
{
    int bad = 0;
    for (int y = 180; y < 370; y++)
        for (int x = 340; x < 600; x++) bad += px(x, y) != 0xFFFFFFu;
    if (bad) {
        fprintf(stderr, "FAIL paint CANVAS: %d non-white pixels in the blank document\n", bad);
        return 1;
    }
    printf("paint CANVAS PASS: the document is a blank white canvas\n");
    return 0;
}

static int menu(void)
{
    int L = sw("File") + 2 * 9;          /* the Edit slot, no Apple slot   */
    int R = -1, black = 0, gray = 0;
    for (int y = 45; y < 115; y++)
        if (px(L, y) != 0x000000u) {
            fprintf(stderr, "FAIL paint MENU_PANEL: no Edit panel frame at x=%d (y=%d #%06X)\n",
                    L, y, (unsigned)px(L, y));
            return 1;
        }
    for (int x = L + 20; x < 400 && R < 0; x++) {
        int col = 1;
        for (int y = 45; y < 115 && col; y++) col = px(x, y) == 0x000000u;
        if (col) R = x;
    }
    if (R < 0) {
        fprintf(stderr, "FAIL paint MENU_PANEL: no right frame for the Edit panel\n");
        return 1;
    }
    /* Rows 50..118: inside the frame (bottom frame row 41 + 5*16 - 1), below
     * the held arrow cursor (hotspot (62,30), its 16x16 image ends y=46). */
    for (int y = 50; y < 41 + 5 * 16 - 2; y++)
        for (int x = L + 2; x <= R - 2; x++) {
            uint32_t c = px(x, y);
            black += c == 0x000000u;
            gray += c == 0xA5A5A5u;
        }
    if (black != 0 || gray < 40) {
        fprintf(stderr, "FAIL paint MENU_GRAYED: Edit panel x=%d..%d holds %d black (want 0) and "
                "%d disabled-ink px (want >= 40)\n", L, R, black, gray);
        return 1;
    }
    printf("paint MENU PASS: Edit panel x=%d..%d, %d disabled-ink px, 0 enabled ink\n", L, R, gray);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *f;
    int w, h, max;
    if (argc != 3 || (strcmp(argv[1], "canvas") && strcmp(argv[1], "menu"))) {
        fprintf(stderr, "usage: ppm_flair_paint_check canvas|menu dump.ppm\n");
        return 2;
    }
    f = fopen(argv[2], "rb");
    if (!f || fscanf(f, "P6 %d %d %d", &w, &h, &max) != 3 || w != 640 || h != 480 ||
        max != 255 || fgetc(f) == EOF || fread(pixels, 1, sizeof pixels, f) != sizeof pixels) {
        fprintf(stderr, "FAIL paint: missing or malformed 640x480 PPM %s\n", argv[2]);
        if (f) fclose(f);
        return 2;
    }
    fclose(f);
    return !strcmp(argv[1], "canvas") ? canvas() : menu();
}

/*
 * ppm_flair_shutdown_check.c -- the SHUT-DOWN SCREEN screendump oracle
 *                               (factory, C).
 *
 * bead: initech-tdnl.78 (Special > Shut Down: put everything away, then the
 *       period "safe to switch off" screen, then halt).
 *
 * Leg (one QEMU screendump, P6 PPM 640x480, of the booted $(FLAIRTENANTS_IMG)
 * after Special > Shut Down and a probe click + key that nothing may answer):
 *   safe   the screen is DARK everywhere outside one alert box; the box
 *          carries the 32x32 shut-down icon, three lines of message text and
 *          a Restart push button with the default ring.
 *
 * WHERE THE EXPECTED VALUES COME FROM (Law 1 / Law 2):
 *   - "the screen will become dark, and a dialog box will appear with a
 *     Restart button": Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf
 *     PDF p. 215 (book p. 195), the Macintosh Plus / SE (no soft power) path.
 *   - the icon bitmap and the element positions: the System Error Handler's
 *     alert table, resource 'DSAT' 2 of the System file on the System 7.0.1
 *     boot disk ../system7-decomp/goldens/work/boot701.dsk (System 7.5.3's
 *     boot753.img carries the same strings): alert 20001 (the switch-off
 *     alert with a Restart button) = text 0xB176 at (v 94, h 114), icon
 *     0xBFF1 at rect (81,60)-(113,92), button list 0xAA0F = one button
 *     "Restart" at rect (154,404)-(174,463). Those are 512x342 screen
 *     coordinates; this screen centres that frame on 640x480, (+64,+69).
 *     The ICON BELOW IS HAND-TYPED from a render of those 128 bytes, NOT read
 *     from the kernel's array (os/milton/kmain.c FSD_ICON), so a slip in
 *     either transcription is caught (Law 2 / HER-02).
 *   - the BOX rectangle is AUTHORED (the DSAT positions its items inside the
 *     low-memory DSAlertRect, which the resource does not carry -- no local
 *     reference), restated here by hand from the kernel's stated choice:
 *     (96,133)..(546,259) = DSAT-space (32,64)..(482,190).
 *   - the frame art is the existing dBoxProc (a 7-px black band) and the face
 *     the Platinum E7 (CIDX_PLAT_FACE), exactly what ppm_flair_trash_check
 *     grades for the Empty Trash alert; colours via flair_canon_rgb (the
 *     independently decomp-graded canon), never the renderer's palette.
 *
 * Usage: ppm_flair_shutdown_check safe <dump.ppm>
 * Exit 0 iff every assertion passes; otherwise a loud message per failure.
 * Ref: CLAUDE.md Law 1, Law 2, Law 4, Rule 2, Rule 6, Rule 12.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb + CIDX_* (-Ispec/assets) */

#define TOL 2
enum { SCRW = 640, SCRH = 480 };

/* The box (authored, see the banner) and its dBoxProc band. */
#define BX_L  96
#define BX_T 133
#define BX_R 546            /* exclusive */
#define BX_B 259            /* exclusive */
#define BX_BORDER 7
/* DSAT 2 elements, +64,+69. */
#define IC_X 124            /* icon rect (81,60)-(113,92) -> x 124, y 150 */
#define IC_Y 150
#define TX_X0 178           /* text pen h 114 -> 178                       */
#define TX_X1 (BX_R - BX_BORDER)
#define TX_Y0 151           /* line cells of 16 px; baseline v 94 -> 163   */
#define BT_L 468            /* button (154,404)-(174,463) -> (468,223)..   */
#define BT_T 223
#define BT_R 527
#define BT_B 243
/* The cursor PARK rect of the trace (spec/flair_power_traces.mk). */
#define PK_X 620
#define PK_Y 460

/* DSAT 2 ICON 0xBFF1, 32 rows x 32 columns, hand-typed from a
 * render of the resource bytes: '#' = ink (black), '.' = clear (white). */
static const char *const ICON[32] = {
    "################################",
    "#........#######################",
    "#........#######################",
    "#........#######################",
    "#........#######################",
    "#........#########......########",
    "#...#....#######..........######",
    "#...#....######............#####",
    "#...#....#####..............####",
    "#........####................###",
    "#........####................###",
    "#........###..................##",
    "#........###...###.###.###....##",
    "#........###..................##",
    "#........###..................##",
    "#........###...###.###.#.#....##",
    "#........###..................##",
    "#........###..................##",
    "#........###...###.#.###......##",
    "#........###..................##",
    "#....#######..................##",
    "#......#####...####.###.##....##",
    "#......#####.................###",
    "#......#####.................###",
    "#......#####................####",
    "#......####................#####",
    "#...#####................#######",
    "#......#########################",
    "#......#########################",
    "#......#########################",
    "#......#########################",
    "################################"
};

static unsigned char *g_buf;
static int g_fail;

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
        fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- %s at (%d,%d): "
                "sampled #%02X%02X%02X, want canon idx %d\n",
                what, x, y, p[0], p[1], p[2], idx);
        g_fail = 1;
    }
}

/* Black pixels in [x0,x1)x[y0,y1), and the rightmost inked column (-1 if none). */
static long ink(int x0, int y0, int x1, int y1, int *right)
{
    long n = 0;
    int r = -1;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (is_idx(x, y, CIDX_BLACK)) { n++; if (x > r) r = x; }
    if (right) *right = r;
    return n;
}

static void want_ink(int x0, int y0, int x1, int y1, int some, const char *what)
{
    long n = ink(x0, y0, x1, y1, NULL);
    printf("    %-52s [%d,%d)x[%d,%d): %ld ink px\n", what, x0, x1, y0, y1, n);
    if (some ? (n == 0) : (n != 0)) {
        fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- %s: %ld ink pixels "
                "in [%d,%d)x[%d,%d), want %s\n",
                what, n, x0, x1, y0, y1, some ? "some" : "none");
        g_fail = 1;
    }
}

static int in_box(int x, int y)
{
    return x >= BX_L && x < BX_R && y >= BX_T && y < BX_B;
}

static int in_park(int x, int y)
{
    return x >= PK_X - 2 && y >= PK_Y - 2;     /* the parked arrow + slack */
}

int main(int argc, char **argv)
{
    FILE *f;
    char magic[3] = {0, 0, 0};
    long w = 0, h = 0, maxv = 0;
    size_t n = (size_t)SCRW * SCRH * 3u;
    long lit = 0;
    int right[3];

    if (argc != 3 || strcmp(argv[1], "safe") != 0) {
        fprintf(stderr, "usage: %s safe <dump.ppm>\n", argv[0]);
        return 2;
    }
    f = fopen(argv[2], "rb");
    if (!f || fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6' ||
        read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv) ||
        w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr, "ppm_flair_shutdown_check: %s is not a 640x480/255 P6 PPM\n", argv[2]);
        if (f) fclose(f);
        return 2;
    }
    g_buf = (unsigned char *)malloc(n);
    if (!g_buf || fread(g_buf, 1, n, f) != n) {
        fprintf(stderr, "ppm_flair_shutdown_check: short raster\n");
        fclose(f);
        return 2;
    }
    fclose(f);
    printf("ppm_flair_shutdown_check: leg safe on %s\n", argv[2]);

    /* (1) THE SCREEN IS DARK outside the box: every pixel, not a sample --
     * a menu bar, a window or a desktop pixel left behind is the failure a
     * person sees first. Only the parked arrow is exempt. */
    for (int y = 0; y < SCRH; y++)
        for (int x = 0; x < SCRW; x++)
            if (!in_box(x, y) && !in_park(x, y) && !is_idx(x, y, CIDX_BLACK)) lit++;
    printf("    %-52s %ld non-black px\n", "outside the box (the dark screen)", lit);
    if (lit != 0) {
        fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- the screen is not "
                "dark outside the box: %ld lit pixels\n", lit);
        g_fail = 1;
    }

    /* (2) the box: the dBoxProc frame and the Platinum face.
     * RE-KEY 2026-10-16 (L023 / initech-rgz1, main 08cf239): the heritage
     * 7-px black slab became the capture-derived Platinum alert edge --
     * inward from top/left: black, #FFB3B3, white, face; inward from
     * bottom/right: black, #FF8787, #B3B3B3, face (s8_alert_modal.png frame
     * x133..506 y87..190; harness/render/alert_capture.h; dialog.c Sec 42).
     * The old probes sat 3 px inside the edge and now read face. Each edge
     * is graded pixel by pixel through the band, so a slab relapse (all
     * black) and a flat frame (no tint rows) both go RED. */
    want(BX_L,     196, CIDX_BLACK,             "box frame left, outline");
    want(BX_L + 1, 196, CIDX_ALERT_LIGHT,       "box frame left, tint row");
    want(BX_L + 2, 196, CIDX_WHITE,             "box frame left, light row");
    want(BX_L + 3, 196, CIDX_PLAT_FACE,         "box frame left, face begins");
    want(BX_R - 1, 196, CIDX_BLACK,             "box frame right, outline");
    want(BX_R - 2, 196, CIDX_ALERT_SHADOW,      "box frame right, tint row");
    want(BX_R - 3, 196, CIDX_PLAT_FRAME_SHADOW, "box frame right, shadow row");
    want(BX_R - 4, 196, CIDX_PLAT_FACE,         "box frame right, face begins");
    want(320, BX_T,     CIDX_BLACK,             "box frame top, outline");
    want(320, BX_T + 1, CIDX_ALERT_LIGHT,       "box frame top, tint row");
    want(320, BX_T + 2, CIDX_WHITE,             "box frame top, light row");
    want(320, BX_T + 3, CIDX_PLAT_FACE,         "box frame top, face begins");
    want(320, BX_B - 1, CIDX_BLACK,             "box frame bottom, outline");
    want(320, BX_B - 2, CIDX_ALERT_SHADOW,      "box frame bottom, tint row");
    want(320, BX_B - 3, CIDX_PLAT_FRAME_SHADOW, "box frame bottom, shadow row");
    want(320, BX_B - 4, CIDX_PLAT_FACE,         "box frame bottom, face begins");
    want(BX_L + BX_BORDER + 4, 240, CIDX_PLAT_FACE, "box face, lower left");
    want(300, 233, CIDX_PLAT_FACE, "box face, left of the button row");
    want(IC_X + 16, IC_Y - 4, CIDX_PLAT_FACE, "box face, above the icon");

    /* (3) the icon, pixel-exact against the hand-typed DSAT strike. */
    {
        int bad = 0;
        for (int r = 0; r < 32; r++)
            for (int c = 0; c < 32; c++) {
                int idx = (ICON[r][c] == '#') ? CIDX_BLACK : CIDX_WHITE;
                if (!is_idx(IC_X + c, IC_Y + r, idx)) {
                    if (bad < 4)
                        fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- icon "
                                "row %d col %d is not %s\n", r, c,
                                idx == CIDX_BLACK ? "ink" : "clear");
                    bad++;
                }
            }
        printf("    %-52s %d wrong px of 1024\n", "the DSAT 0xBFF1 icon at (124,150)", bad);
        if (bad) g_fail = 1;
    }

    /* (4) the message: three lines in the text column; the third ("press
     * Restart.") is SHORT and the first two are long; no fourth line. */
    for (int k = 0; k < 3; k++) {
        char what[64];
        long nk = ink(TX_X0, TX_Y0 + 16 * k, TX_X1, TX_Y0 + 16 * (k + 1), &right[k]);
        snprintf(what, sizeof what, "message line %d", k + 1);
        printf("    %-52s [%d,%d)x[%d,%d): %ld ink px, right edge x=%d\n", what,
               TX_X0, TX_X1, TX_Y0 + 16 * k, TX_Y0 + 16 * (k + 1), nk, right[k]);
        if (nk == 0) {
            fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- message line %d is not drawn\n", k + 1);
            g_fail = 1;
        }
    }
    if (!(right[0] > TX_X0 + 200 && right[1] > TX_X0 + 200 &&
          right[2] > TX_X0 && right[2] < TX_X0 + 120)) {
        fprintf(stderr, "ppm_flair_shutdown_check: FAIL -- the line lengths are "
                "not long/long/short (right edges %d %d %d)\n",
                right[0], right[1], right[2]);
        g_fail = 1;
    }
    want_ink(TX_X0, TX_Y0 + 48, BT_L - 6, BX_B - BX_BORDER, 0, "no fourth line, nothing under the text");
    want_ink(BX_L + BX_BORDER + 1, BX_T + BX_BORDER + 1, IC_X - 1, BX_B - BX_BORDER - 1, 0,
             "nothing left of the icon");

    /* (5) the Restart button: its label, and the default ring around it. */
    want_ink(BT_L + 4, BT_T + 3, BT_R - 4, BT_B - 2, 1, "Restart button label");
    want((BT_L + BT_R) / 2, BT_T - 3, CIDX_BLACK, "Restart default ring (InsetRect -3), top");
    want(BT_L - 3, (BT_T + BT_B) / 2, CIDX_BLACK, "Restart default ring, left");
    want(BT_L - 8, (BT_T + BT_B) / 2, CIDX_PLAT_FACE, "face left of the ring");

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_shutdown_check: leg safe FAILED\n");
        return 1;
    }
    printf("ppm_flair_shutdown_check: leg safe PASS\n");
    return 0;
}

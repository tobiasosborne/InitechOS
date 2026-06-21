/*
 * ppm_flair_check.c -- the FLAIR LIVE DESKTOP screendump oracle (factory, C-only).
 *
 * beads: initech-re30.3 (LANE 2 -- the screendump oracle). Ref: CLAUDE.md Law 2
 *        ("the oracle is the truth"), Law 4 ("it must look like the frame"),
 *        Rule 2 (fail fast/loud), Rule 6 (the gate must BITE -- mutation-proven
 *        by test-flair-desktop-mutant), Rule 11 (deterministic), Rule 12
 *        (ASCII-clean), Law 3 (factory is C). PRD Sec 1/6.3/6.5/Appendix A.
 *
 * The STRUCTURAL companion to harness/proptest/test_shell.c. test_shell.c probes
 * the SAME composed Office Space scene in 8bpp INDEX space on the HOST; this tool
 * probes the SAME scene as PRESENTED to the live LFB (build/flair_desktop.img,
 * kmain -DBOOT_FLAIR_SHELL) and captured as a QEMU 32bpp screendump (a P6 PPM).
 * The live present path writes flair_palette_rgb(idx) (spec/assets/palette.h) which
 * is a byte-for-byte port of the host render_palette_rgb, so EVERY probe reads the
 * EXACT RGB that test_shell.c's index probe maps to -- the live desktop and the
 * host oracle agree BY CONSTRUCTION (Law 2). Colors are therefore EXACT; the
 * tolerance is kept TIGHT (+/-2) so the checker actually discriminates (a wrong
 * scene reads wrong colors and the gate BITES).
 *
 * FOUR structural assertions, all must hold (exit 0 iff all pass; non-zero + a
 * fail-loud message naming the assertion + sampled-vs-expected RGB otherwise):
 *
 *   (a) SEAFOAM DESKTOP -- bare-desktop sample points (corners/edges clear of the
 *       windows + the modal + the two menu bars) read INITECH_DESKTOP_BG_RGB
 *       (flair_palette_rgb index 2). test_shell.c assertion (1).
 *
 *   (b) TWO MENU BARS -- band [0,20) is the System-7 bar, band [20,40) is the
 *       Photoshop bar. Both bands paint the menubar-gray fill (idx 3) at a far-
 *       right non-title x AND a black (idx 0) baseline; both carry TITLE INK
 *       (idx 0) glyphs. THE CHIMERA TELL: the System-7 band has a dense Apple
 *       slot at x in [0,20) (a filled ink square, >= APPLE_INK_MIN px) while the
 *       Photoshop band does NOT (no Apple slot); AND the Photoshop band carries
 *       title ink far to the right (x in [240,260)) where the System-7 band -- a
 *       short "File Edit View Special" -- has none. A ONE-bar render leaves the
 *       Photoshop band as bare desktop (no fill, no ink) -> RED. test_shell.c
 *       assertion (2).
 *
 *   (c) WINDOW CHROME -- the front window's pinstripe title bar ALTERNATES with
 *       period 2 between idx 7 (#6B6B74) and idx 8 (#8A8A93) at adjacent y; the
 *       1 px right frame column is painted (non-desktop) and the pixel just
 *       outside it is bare seafoam (frame is exactly 1 px); the body just below
 *       the title bar is window white (idx 1). test_shell.c assertion (3).
 *
 *   (d) MODAL FILE COPY -- the centered dBoxProc box's thick black (idx 0) border
 *       on all four sides, white (idx 1) interior just inside the 7 px border, the
 *       "Saving tables to disk..." text ink band (a non-white pixel inside the
 *       text rect), AND it OCCLUDES the windows (z-order): a probe point inside
 *       the modal border that ALSO lies over document window 0 reads the modal's
 *       BLACK border, and a modal-interior probe over window 0 reads modal WHITE
 *       -- NOT window content. test_shell.c assertions (4) + (5).
 *
 * The expected colors are the flair_palette_rgb(idx) values (single source of
 * truth, spec/assets/palette.h, -Ispec/assets); the probe coordinates mirror
 * test_shell.c's scene geometry (W0/W1 bounds, the {140,200,500,280} centered
 * modal, the two 20px bar bands). Tightly calibrated against the live render.
 *
 * Usage: ppm_flair_check <screendump.ppm>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palette.h"   /* flair_palette_rgb + INITECH_*_RGB (-Ispec/assets) */

/* Tight per-channel tolerance. The live present path writes flair_palette_rgb
 * values directly and QEMU's XRGB8888 -> P6 dump is exact, so the colors are
 * EXACT; +/-2 only absorbs any 1-LSB rounding and still discriminates wrong
 * scenes (the palette entries differ by far more than 2 per channel). */
#define TOL 2

/* ---- The composed-scene geometry (mirrors test_shell.c). ----------------- */
enum { SCRW = 640, SCRH = 480 };

/* Menu bars: each FLAIR_MENUBAR_H = 20 px. Band 0 = System-7 [0,20); band 1 =
 * Photoshop [20,40). (Hardcoded as 20 to keep the tool freestanding-header-free
 * beyond palette.h; test-chrome locks FLAIR_MENUBAR_H == 20 against the JSON.) */
#define MENUBAR_H   20

/* Front document window 1 (lower-right; fully visible, clear of the modal):
 * T120 L300 B360 R560 (test_shell.c W1_*). 1 px frame, 19 px title bar. */
#define W1_T 120
#define W1_L 300
#define W1_B 360
#define W1_R 560
#define FRAME       1
#define TITLEBAR_H  19

/* Back document window 0 (upper-left; overlapped by the centered modal):
 * T80 L60 B300 R360 (test_shell.c W0_*) -- used for the z-order occlusion probe. */
#define W0_T 80
#define W0_L 60
#define W0_B 300
#define W0_R 360

/* The centered FILE COPY modal: {dl,dt,dr,db} = {140,200,500,280} (centered on
 * 640x480), dBoxProc border = 7 px. */
#define DL 140
#define DT 200
#define DR 500
#define DB 280
#define DBORDER     7

/* The Apple-slot density tell: the System-7 band's x in [0,20) is a filled ink
 * square (the Apple menu slot); the Photoshop band has no such slot. Calibrated:
 * System-7 band0 x[0,20) inks ~240 px; Photoshop band1 x[0,20) inks ~39 (glyphs
 * only). Threshold splits them with a wide margin. */
#define APPLE_INK_MIN   150

/* Title-ink presence threshold per probed column block (16x20 cell-ish). The
 * Photoshop bar's title ink must reach the far-right column block [240,260)
 * where the short System-7 string never paints (the two-bar tell). */
#define TITLE_INK_MIN   8

/* ---- PPM P6 reader (same invariant as ppm_seafoam_check / ppm_text_check). - */
static int read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') {
        v = v * 10 + (c - '0');
        c = fgetc(f);
    }
    *out = v;
    return 0;
}

static unsigned char *g_buf;
static long g_w, g_h;

static const unsigned char *at(int x, int y)
{
    return g_buf + ((long)y * g_w + x) * 3;
}

/* Match a pixel against an expected 0x00RRGGBB within TOL. */
static int is_rgb(int x, int y, unsigned int rgb)
{
    const unsigned char *p = at(x, y);
    int r = (int)((rgb >> 16) & 0xFFu);
    int g = (int)((rgb >> 8) & 0xFFu);
    int b = (int)(rgb & 0xFFu);
    return abs((int)p[0] - r) <= TOL &&
           abs((int)p[1] - g) <= TOL &&
           abs((int)p[2] - b) <= TOL;
}

/* idx -> expected RGB (the single point of truth, palette.h). */
static unsigned int IDX(int i)
{
    return (unsigned int)(flair_palette_rgb((unsigned char)i) & 0x00FFFFFFu);
}

/* Count idx-0 (black ink) pixels in a rectangle [x0,x1) x [y0,y1). */
static long ink_in(int x0, int y0, int x1, int y1)
{
    long n = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (is_rgb(x, y, IDX(0)))
                n++;
    return n;
}

static int g_fail = 0;

/* Assert that pixel (x,y) equals palette index `idx`; fail loud naming the
 * assertion + the sampled-vs-expected RGB. */
static void assert_idx(int x, int y, int idx, const char *what)
{
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_check: FAIL %s\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected idx %d = RGB(%d,%d,%d)=#%06X (tol +/-%d)\n",
                what, x, y, p[0], p[1], p[2], p[0], p[1], p[2],
                idx, (e >> 16) & 0xFF, (e >> 8) & 0xFF, e & 0xFF, e, TOL);
        g_fail = 1;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <screendump.ppm>\n", argv[0]);
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "ppm_flair_check: cannot open %s\n", argv[1]);
        return 2;
    }
    char magic[3] = {0};
    if (fread(magic, 1, 2, f) != 2 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "ppm_flair_check: not a P6 PPM (got '%s')\n", magic);
        fclose(f);
        return 2;
    }
    long w = 0, h = 0, maxv = 0;
    if (read_uint(f, &w) || read_uint(f, &h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_check: malformed PPM header\n");
        fclose(f);
        return 2;
    }
    if (w != SCRW || h != SCRH || maxv != 255) {
        fprintf(stderr,
                "ppm_flair_check: unexpected dims/maxval w=%ld h=%ld max=%ld "
                "(expect %dx%d, 255)\n", w, h, maxv, SCRW, SCRH);
        fclose(f);
        return 2;
    }
    /* read_uint already consumed the single whitespace byte after maxval, so the
     * file position is exactly at the raster (same invariant as the sibling
     * checkers -- do NOT skip another byte). */
    long npix = w * h;
    g_buf = malloc((size_t)npix * 3);
    if (!g_buf) {
        fprintf(stderr, "ppm_flair_check: OOM\n");
        fclose(f);
        return 2;
    }
    size_t got = fread(g_buf, 3, (size_t)npix, f);
    fclose(f);
    if (got != (size_t)npix) {
        fprintf(stderr, "ppm_flair_check: short raster (%zu of %ld pixels)\n",
                got, npix);
        free(g_buf);
        return 2;
    }
    g_w = w;
    g_h = h;

    printf("ppm_flair_check: %ldx%ld P6, tol +/-%d -- expected indices: "
           "desktop=#%06X menubar=#%06X white=#%06X ink=#%06X "
           "pinstripe L=#%06X D=#%06X\n",
           w, h, TOL, IDX(2), IDX(3), IDX(1), IDX(0), IDX(7), IDX(8));

    /* ======================================================================
     * (a) SEAFOAM DESKTOP -- bare-desktop sample points, every one clear of the
     * two bars (y>=40), window 0 (L60-360,T80-300), window 1 (L300-560,T120-360)
     * and the modal (L140-500,T200-280). test_shell.c assertion (1).
     * ====================================================================== */
    assert_idx( 20, 460, 2, "(a) bare desktop bottom-left reads seafoam");
    assert_idx(600, 460, 2, "(a) bare desktop bottom-right reads seafoam");
    assert_idx( 20,  60, 2, "(a) bare desktop above-left reads seafoam");
    assert_idx(620,  60, 2, "(a) bare desktop top-right reads seafoam");
    assert_idx(620, 400, 2, "(a) bare desktop right edge reads seafoam");
    assert_idx( 40, 400, 2, "(a) bare desktop left edge reads seafoam");

    /* ======================================================================
     * (b) TWO MENU BARS + the chimera tell. test_shell.c assertion (2).
     * ====================================================================== */
    /* Both bands: a far-right non-title x reads the menubar-gray fill (idx 3),
     * and the band baseline (bottom row) reads black ink (idx 0). */
    assert_idx(600, 5,              3, "(b) System-7 bar fill (rows [0,20)) is menubar gray");
    assert_idx(300, MENUBAR_H - 1,  0, "(b) System-7 bar baseline (y=19) is black ink");
    assert_idx(600, MENUBAR_H + 5,  3, "(b) Photoshop bar fill (rows [20,40)) is menubar gray");
    assert_idx(300, 2 * MENUBAR_H - 1, 0, "(b) Photoshop bar baseline (y=39) is black ink");

    /* THE CHIMERA TELL #1: the System-7 band has a dense Apple slot at x[0,20)
     * (a filled ink square), the Photoshop band does NOT. */
    {
        long sys_apple = ink_in(0, 2, 20, MENUBAR_H - 2);
        long ps_apple  = ink_in(0, MENUBAR_H + 2, 20, 2 * MENUBAR_H - 2);
        if (sys_apple < APPLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) System-7 bar Apple slot missing -- "
                    "x[0,20) ink=%ld (need >=%d); the top bar is not the System-7 bar\n",
                    sys_apple, APPLE_INK_MIN);
            g_fail = 1;
        }
        if (ps_apple >= APPLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar has an Apple slot -- "
                    "x[0,20) ink=%ld (must be <%d); the two bands are not DISTINCT bars\n",
                    ps_apple, APPLE_INK_MIN);
            g_fail = 1;
        }
        printf("    (b) Apple-slot tell: System-7 x[0,20) ink=%ld (>=%d req), "
               "Photoshop x[0,20) ink=%ld (<%d req)\n",
               sys_apple, APPLE_INK_MIN, ps_apple, APPLE_INK_MIN);
    }

    /* THE CHIMERA TELL #2 (the load-bearing two-bar assertion): the Photoshop
     * bar carries TITLE INK far to the right (x[240,260)) where the short
     * System-7 string ("File Edit View Special") never paints. A ONE-bar render
     * (SHELL_MUTATE_ONE_MENUBAR) leaves the Photoshop band as bare desktop -> no
     * fill, no baseline ink, no far-right title ink -> all three RED above + here. */
    {
        long ps_far = ink_in(240, MENUBAR_H + 2, 260, 2 * MENUBAR_H - 2);
        long sys_far = ink_in(240, 2, 260, MENUBAR_H - 2);
        if (ps_far < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar title ink missing at "
                    "the far-right (x[240,260) ink=%ld, need >=%d) -- the long "
                    "'File Edit Image Layer Select View Window Help' chimera bar "
                    "is absent (one-bar render?)\n",
                    ps_far, TITLE_INK_MIN);
            g_fail = 1;
        }
        printf("    (b) two-bar tell: Photoshop far-right x[240,260) title ink=%ld "
               "(>=%d req), System-7 far-right ink=%ld (short bar, expected ~0)\n",
               ps_far, TITLE_INK_MIN, sys_far);
        /* Also assert each band carries SOME title ink to the right of the slot
         * region (proves both bars are rendered, not just baselines). */
        long sys_title = ink_in(20, 2, 220, MENUBAR_H - 2);
        long ps_title  = ink_in(0, MENUBAR_H + 2, 220, 2 * MENUBAR_H - 2);
        if (sys_title < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) System-7 bar has no title glyph ink "
                    "(x[20,220) ink=%ld, need >=%d)\n", sys_title, TITLE_INK_MIN);
            g_fail = 1;
        }
        if (ps_title < TITLE_INK_MIN) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (b) Photoshop bar has no title glyph ink "
                    "(x[0,220) ink=%ld, need >=%d)\n", ps_title, TITLE_INK_MIN);
            g_fail = 1;
        }
    }

    /* ======================================================================
     * (c) WINDOW CHROME (front window 1). test_shell.c assertion (3).
     * ====================================================================== */
    {
        const int title_top = W1_T + FRAME;          /* 121 */
        const int title_bot = title_top + TITLEBAR_H; /* 140 */
        const int mid_x = (W1_L + W1_R) / 2;          /* 430 */
        const int body_row = (title_bot + (W1_B - FRAME)) / 2;

        /* Pinstripe ALTERNATES with period 2 between idx 7 and idx 8. Each row in
         * the title bar must be idx 7 OR idx 8, must differ from its neighbor,
         * and equal the row 2 above (period 2). */
        int alt_ok = 1, period_ok = 1, shade_ok = 1;
        int shades[TITLEBAR_H];
        for (int k = 0; k < TITLEBAR_H; k++) {
            int y = title_top + k;
            if      (is_rgb(mid_x, y, IDX(7))) shades[k] = 7;
            else if (is_rgb(mid_x, y, IDX(8))) shades[k] = 8;
            else { shades[k] = -1; shade_ok = 0; }
        }
        for (int k = 1; k < TITLEBAR_H; k++)
            if (shades[k] == shades[k - 1]) alt_ok = 0;
        for (int k = 2; k < TITLEBAR_H; k++)
            if (shades[k] != shades[k - 2]) period_ok = 0;
        if (!shade_ok) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window title bar at x=%d is not "
                    "the pinstripe shade pair (idx 7 #%06X / idx 8 #%06X)\n",
                    mid_x, IDX(7), IDX(8));
            g_fail = 1;
        }
        if (!alt_ok) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window title-bar pinstripe does "
                    "NOT alternate (two equal adjacent rows at x=%d)\n", mid_x);
            g_fail = 1;
        }
        if (!period_ok) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window pinstripe period != 2 at "
                    "x=%d\n", mid_x);
            g_fail = 1;
        }
        printf("    (c) pinstripe at x=%d: %s, period-2 %s (idx7 #%06X / idx8 #%06X)\n",
               mid_x, alt_ok ? "alternates" : "FLAT", period_ok ? "ok" : "BAD",
               IDX(7), IDX(8));

        /* The row just below the title bar is the white body (idx 1) -> the title
         * bar is EXACTLY TITLEBAR_H tall. */
        assert_idx(mid_x, title_bot, 1,
                   "(c) front window row below title bar is white body (title height exact)");
        assert_idx(mid_x, body_row, 1,
                   "(c) front window body fill is window white");

        /* Frame: exactly 1 px. The right frame column is painted (non-desktop);
         * the pixel just outside is bare seafoam. */
        if (is_rgb(W1_R - 1, body_row, IDX(2))) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (c) front window right frame column "
                    "(x=%d) reads bare seafoam -- the frame is not painted\n", W1_R - 1);
            g_fail = 1;
        }
        assert_idx(W1_R, body_row, 2,
                   "(c) pixel just right of the 1 px frame is bare seafoam");
    }

    /* ======================================================================
     * (d) MODAL FILE COPY + z-order occlusion. test_shell.c assertions (4)+(5).
     * ====================================================================== */
    {
        /* Centered (sanity). */
        if ((DL + DR) / 2 != SCRW / 2 || (DT + DB) / 2 != SCRH / 2) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (d) modal bounds not centered on %dx%d\n",
                    SCRW, SCRH);
            g_fail = 1;
        }
        const int cy = (DT + DB) / 2;  /* 240 */
        const int cx = (DL + DR) / 2;  /* 320 */

        /* The 7-px dBoxProc border is black on all four sides. */
        assert_idx(DL + 3, cy, 0, "(d) modal left border is black (dBoxProc 7 px)");
        assert_idx(DR - 4, cy, 0, "(d) modal right border is black (dBoxProc 7 px)");
        assert_idx(cx, DT + 3, 0, "(d) modal top border is black (dBoxProc 7 px)");
        assert_idx(cx, DB - 4, 0, "(d) modal bottom border is black (dBoxProc 7 px)");

        /* First content row just inside the 7-px border is white (idx 1) -> the
         * border is EXACTLY 7 px. */
        assert_idx(cx, DT + DBORDER, 1,
                   "(d) modal interior just inside the 7 px border is white");

        /* The "Saving tables to disk..." static text is RENDERED as ink: the text
         * rect (left=154, top=212, 16 px tall) contains a non-white pixel. */
        {
            int tx0 = 154, ty0 = 212;
            int text_ink = 0;
            for (int x = tx0; x < tx0 + 300 && !text_ink; x++)
                for (int y = ty0; y < ty0 + 16; y++)
                    if (!is_rgb(x, y, IDX(1))) { text_ink = 1; break; }
            if (!text_ink) {
                fprintf(stderr,
                        "ppm_flair_check: FAIL (d) 'Saving tables to disk...' text "
                        "did NOT render (no ink in the text band) -- modal absent?\n");
                g_fail = 1;
            }
        }

        /* The progress bar (left=154, top=236, right=486, bottom=256): its left
         * border is black and its interior (value 0) is white. */
        assert_idx(154, 246, 0, "(d) progress bar left border is painted (black)");
        assert_idx(300, 246, 1, "(d) progress bar interior (value 0) is white");

        /* Z-ORDER: the modal occludes the windows. Probe (143,240) lies BOTH
         * inside the modal's 7-px LEFT border (x in [140,147)) AND inside window 0
         * (x in [60,360), y in [80,300)). Correct z-order -> modal BLACK border. */
        int in_modal_border = (143 >= DL && 143 < DL + DBORDER && 240 >= DT && 240 < DB);
        int in_window0      = (143 >= W0_L && 143 < W0_R && 240 >= W0_T && 240 < W0_B);
        if (!(in_modal_border && in_window0)) {
            fprintf(stderr,
                    "ppm_flair_check: FAIL (d) z-order probe (143,240) is not over "
                    "BOTH the modal border and window 0 (test would be vacuous)\n");
            g_fail = 1;
        }
        assert_idx(143, 240, 0,
                   "(d) modal OCCLUDES the window behind it -- modal black border on top (z-order)");
        assert_idx(160, 240, 1,
                   "(d) modal interior occludes the window behind it (modal white on top)");
    }

    free(g_buf);

    if (g_fail) {
        fprintf(stderr, "ppm_flair_check: FAIL -- the live FLAIR desktop screendump "
                        "is NOT the Office Space frame structure\n");
        return 1;
    }
    printf("ppm_flair_check: PASS -- seafoam desktop + TWO stacked menu bars + "
           "window chrome + centered FILE COPY modal occluding the windows\n");
    return 0;
}

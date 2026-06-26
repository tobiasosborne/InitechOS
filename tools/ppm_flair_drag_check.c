/*
 * ppm_flair_drag_check.c -- the FO-9 EMU drag oracle's screendump grader (HOST,
 * C-only). beads initech-5l5z FO-9 (ADR-0006 E-D5(B) Tier-A damage law + Tier-B
 * shifted-WDEF geometry). It grades the POST-DRAG screendump of the booted
 * BOOT_FLAIR_LIVE desktop after the locked trace dragged window 1 ("Saving
 * tables to disk") by (+40,+30): from struct origin (300,120) to (340,150).
 *
 * THE TWO DIFFERENTIALS (each independently catches the HER-14 drag-noop mutant,
 * the "static frame dressed as interactive" heresy; ADR-0006 M1 / BC-9):
 *
 *   LEG A -- CHROME AT THE NEW POSITION.  The window's System-7 title-bar chrome
 *     is now at the SHIFTED rect {T150 L340 B390 R600}.  We grade the chrome
 *     GEOMETRY at the shifted coords (a WDEF-scan at the new position; ADR-0006
 *     E-D5 Tier-B): the new struct TOP-frame row (y=150) is idx0 frame ink; the
 *     19px title band's 15-row pinstripe interior (y[152,167)) at a CLEAN column
 *     (x=560, right of the centered title text) is the period-2 phase-locked
 *     System-7 racing stripe -- every row is shade idx7 (#F3F3F3) or idx8
 *     (#969696), BOTH shades appear, and >=1 adjacent-equal (phase-lock) pair
 *     exists; and the content just below (y=170) is idx1 window white.  This
 *     column was BARE TEAL before the drag (x=560 is the OLD window's right edge,
 *     just outside its half-open [300,560) struct), so a window that did NOT move
 *     leaves teal here and LEG A goes RED.
 *
 *   LEG B -- THE VACATED AREA READS BARE TEAL (the D-5 damage law erased the old
 *     position; ADR-0006 E-D5 Tier-A).  Two points in the OLD title band that the
 *     new window does NOT cover and no other window/modal re-occupies -- (450,130)
 *     and (520,130), both above the new window's top (y=150), right of window 0's
 *     right edge (x=360), above the modal (y=200) -- read EXACTLY canon idx2
 *     Initech teal (#8DDCDC).  A window that did NOT move leaves its OLD title
 *     chrome here (non-teal) and LEG B goes RED.
 *
 *   LEG C -- a bare-desktop corner sanity anchor (the canon teal is really teal).
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0006 E-D5/BC-5): the expected colors are the
 * canon flair_canon_rgb(idx) values (spec/assets/color_canon.h), the SAME
 * independently-decomp-graded canon test-color-canon vouches for -- never the
 * render source flair_palette_rgb, never preview.webp.  The pinstripe SHADES and
 * the title-bar GEOMETRY (19px band / 1px frame / 15 stripe rows) come from
 * spec/chrome_metrics.h, the System-7 decomp listing -- NOT from the artifact's
 * render.  The load-bearing assertions are GEOMETRIC differentials (chrome at the
 * SHIFTED rect; teal at the VACATED rect) that flip under the drag-noop mutant.
 *
 * Usage: ppm_flair_drag_check <screendump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * locked trace's (+40,+30) delta and the test_shell.c W1 geometry.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb (-Ispec/assets) -- the canon the
                            * INDEPENDENT test-color-canon grades, not us.       */

/* Per-channel tolerance: capture-noise only (the XRGB8888 -> P6 dump is exact;
 * canon entries differ by far more than 2/channel), mirroring ppm_flair_check. */
#define TOL 2

/* ---- canon palette indices (spec/assets; the values are flair_canon_rgb) ---- */
#define CIDX_FRAME     0   /* black frame / ink                 */
#define CIDX_WHITE     1   /* window/content white              */
#define CIDX_TEAL      2   /* Initech teal #8DDCDC (desktop bg)  */
#define CIDX_PIN_LIGHT 7   /* pinstripe light #F3F3F3           */
#define CIDX_PIN_DARK  8   /* pinstripe dark  #969696           */

/* ---- the post-drag geometry (test_shell.c W1 + the locked (+40,+30) delta). --
 * Old W1 struct: T120 L300 B360 R560.  New W1 struct: T150 L340 B390 R600.
 * Title band = 19px; frame = 1px; pinstripe interior = 15 rows (chrome_metrics). */
#define NEW_T      150
#define NEW_L      340
#define NEW_R      600
/* a CLEAN pinstripe column: right of the centered title text, left of the new
 * right frame (x=599), and == the OLD window's right edge (560) so it was bare
 * teal pre-drag -- the LEG-A differential anchor. */
#define PIN_X      560
/* pinstripe interior band at the new position: top-frame(1) + bevel-hi(1) = +2,
 * then 15 stripe rows -> y[NEW_T+2, NEW_T+2+15) = [152,167). */
#define STRIPE_TOP (NEW_T + 2)
#define STRIPE_BOT (STRIPE_TOP + 15)

/* ---- PPM P6 reader (the ppm_flair_check invariant). ---------------------- */
static unsigned char *g_buf;
static long g_w, g_h;

static int read_uint(FILE *f, long *out)
{
    int c;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) return -1;
        if (c == '#') { while (c != '\n' && c != EOF) c = fgetc(f); continue; }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        break;
    }
    if (c < '0' || c > '9') return -1;
    long v = 0;
    while (c >= '0' && c <= '9') { v = v * 10 + (c - '0'); c = fgetc(f); }
    *out = v;
    return 0;
}

static const unsigned char *at(int x, int y)
{
    return g_buf + ((long)y * g_w + x) * 3;
}

static unsigned int IDX(int i)
{
    return (unsigned int)(flair_canon_rgb((unsigned char)i) & 0x00FFFFFFu);
}

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

/* classify pixel (x,y) as pinstripe shade 7 (light) or 8 (dark), else -1. */
static int classify_pin(int x, int y)
{
    if (is_rgb(x, y, IDX(CIDX_PIN_LIGHT))) return 7;
    if (is_rgb(x, y, IDX(CIDX_PIN_DARK)))  return 8;
    return -1;
}

static int g_fail = 0;

static void assert_idx(int x, int y, int idx, const char *what)
{
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_drag_check: FAIL %s\n"
                "    at (%d,%d): sampled RGB(%d,%d,%d)=#%02X%02X%02X, "
                "expected idx %d = #%06X (tol +/-%d)\n",
                what, x, y, p[0], p[1], p[2], p[0], p[1], p[2], idx, e, TOL);
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
    if (!f) { fprintf(stderr, "ppm_flair_drag_check: cannot open %s\n", argv[1]); return 2; }
    int c0 = fgetc(f), c1 = fgetc(f);
    if (c0 != 'P' || c1 != '6') { fprintf(stderr, "ppm_flair_drag_check: not a P6 PPM\n"); return 2; }
    long maxv;
    if (read_uint(f, &g_w) || read_uint(f, &g_h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_drag_check: bad PPM header\n"); return 2;
    }
    if (g_w < 640 || g_h < 480) {
        fprintf(stderr, "ppm_flair_drag_check: screendump %ldx%ld < 640x480\n", g_w, g_h);
        return 2;
    }
    g_buf = (unsigned char *)malloc((size_t)g_w * g_h * 3);
    if (!g_buf) { fprintf(stderr, "ppm_flair_drag_check: OOM\n"); return 2; }
    if (fread(g_buf, 1, (size_t)g_w * g_h * 3, f) != (size_t)g_w * g_h * 3) {
        fprintf(stderr, "ppm_flair_drag_check: short read\n"); free(g_buf); fclose(f); return 2;
    }
    fclose(f);

    printf("ppm_flair_drag_check: grading the post-drag frame "
           "(W1 (300,120)->(340,150), the locked +40,+30 trace)\n");

    /* ---- LEG A: CHROME AT THE NEW POSITION (the WDEF-scan at shifted coords) -- */
    /* A1: the new struct top-frame row is idx0 frame ink. */
    assert_idx(PIN_X, NEW_T, CIDX_FRAME,
               "LEG A: new title-bar TOP frame (y=NEW_T) is idx0 frame ink");
    /* A2: the 15-row pinstripe interior at a clean column is the System-7 racing
     *     stripe: every row shade 7/8, both shades present, a phase-lock pair. */
    {
        int shade_ok = 1, saw_light = 0, saw_dark = 0, doubled = 0, prev = -2;
        for (int y = STRIPE_TOP; y < STRIPE_BOT; y++) {
            int s = classify_pin(PIN_X, y);
            if (s < 0) {
                shade_ok = 0;
                const unsigned char *p = at(PIN_X, y);
                fprintf(stderr, "    LEG A: (%d,%d) not a pinstripe shade: "
                        "RGB(%d,%d,%d)\n", PIN_X, y, p[0], p[1], p[2]);
            }
            if (s == 7) saw_light = 1;
            if (s == 8) saw_dark = 1;
            if (y > STRIPE_TOP && s == prev && s >= 0) doubled = 1;
            prev = s;
        }
        if (!(shade_ok && saw_light && saw_dark && doubled)) {
            fprintf(stderr, "ppm_flair_drag_check: FAIL LEG A -- new-position "
                    "pinstripe NOT present at x=%d, y[%d,%d) "
                    "(shade_ok=%d light=%d dark=%d phaselock=%d): the window chrome "
                    "did NOT move here (drag-noop?)\n",
                    PIN_X, STRIPE_TOP, STRIPE_BOT, shade_ok, saw_light, saw_dark, doubled);
            g_fail = 1;
        } else {
            printf("    LEG A: System-7 pinstripe chrome IS at the NEW position "
                   "(x=%d, y[%d,%d); period-2 phase-locked, both shades)\n",
                   PIN_X, STRIPE_TOP, STRIPE_BOT);
        }
    }
    /* A3: the content just below the new title band is idx1 window white. */
    assert_idx(PIN_X, NEW_T + 20, CIDX_WHITE,
               "LEG A: content below the new title band (y=NEW_T+20) is idx1 white");

    /* ---- LEG B: THE VACATED AREA READS BARE TEAL (the D-5 damage law) -------- */
    assert_idx(450, 130, CIDX_TEAL,
               "LEG B: vacated old-title point (450,130) is bare idx2 teal");
    assert_idx(520, 130, CIDX_TEAL,
               "LEG B: vacated old-title point (520,130) is bare idx2 teal");
    if (!g_fail) {
        printf("    LEG B: the vacated old-title area reads bare Initech teal "
               "(the minimal-repaint damage law erased the old chrome)\n");
    }

    /* ---- LEG C: a bare-desktop corner sanity anchor ------------------------- */
    assert_idx(20, 460, CIDX_TEAL,
               "LEG C: bare-desktop corner (20,460) is idx2 teal");

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_drag_check: FAIL -- the live drag did not move "
                "the chrome to the new rect and/or erase the old rect (Law 4: the "
                "desktop is not actually interactive)\n");
        return 1;
    }
    printf("ppm_flair_drag_check: PASS -- chrome at the NEW rect {T%d L%d B.. R%d}, "
           "vacated area is bare teal; the booted desktop is LIVE and draggable "
           "(FO-7/8; ADR-0006 E-D5 Tier-A damage law + Tier-B shifted geometry)\n",
           NEW_T, NEW_L, NEW_R);
    return 0;
}

/*
 * ppm_flair_dc4v_check.c -- the initech-dc4v EMU survival oracle's screendump
 * grader (HOST, C-only). beads initech-dc4v (the RED half of the compositor fix
 * initech-pipa). It grades the POST-DRAG screendump of the booted BOOT_FLAIR_LIVE
 * desktop (show_modal=1 -- the ONLY scene with the modal) after the locked dc4v
 * trace dragged window 1 ("Saving tables to disk") ~260 px LEFT: from struct
 * origin (300,120) to (40,120). That drag's VACATED area sweeps the modal's right
 * half; the pre-fix compositor seafoam-ERASES the modal there (and the moved
 * window would overpaint the modal's left half) because the two menu bars + the
 * modal are painted ONCE by shell_render and are NOT WindowMgr damage-tracked
 * participants. The initech-pipa fold (wm->overlay_rgn into window.c fronts_union)
 * makes those layers occlude, so they SURVIVE from the initial shell_render.
 *
 * THE SURVIVAL DIFFERENTIAL (each independently flips under the pre-fix / mutant
 * -DWINDOW_MUTATE_IGNORE_OVERLAY compositor; the pipa erase case):
 *
 *   LEG M -- THE MODAL SURVIVES.  The dragged window's OLD struct {300,120,560,360}
 *     overlapped the modal {140,200,500,280}; after the ~260 px LEFT drag the
 *     modal's right half is VACATED.  Post-fix it is untouched from shell_render:
 *       - the moveable-titled modal's right frame (499,240) reads canon idx0
 *         black (was the dBoxProc 7px border's (496,240) before initech-zvo6;
 *         the modal chrome is now a pinstripe title bar + a PLAIN 1px frame);
 *       - the modal interior / progress bar (450,240) reads canon idx1 white;
 *       - the right-half band x[366,498] y[220,278] (below the 19px title band
 *         -- was y[206,278] under the old 7px-border layout) is >=80% canon
 *         white/black (the modal box + text + progress bar) and <=5% canon teal.
 *     Pre-fix the whole band was seafoam-ERASED -> ~100% teal -> LEG M goes RED.
 *
 *   LEG B -- THE PHOTOSHOP MENU BAR SURVIVES.  A run across the second (Photoshop)
 *     bar at y=30, x[70,350], is dominated by the canon menu-bar white (idx3 ==
 *     #FFFFFF) with black title ink, and shows essentially NO teal -- the bars are
 *     an always-on-top layer the compositor must never let a window/erase touch.
 *
 *   LEG A -- a bare-desktop corner sanity anchor (20,460) reads canon idx2 teal
 *     (the desktop really is the canon teal, so the LEG-M teal check is meaningful).
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0010 / the ppm_flair_drag_check pattern): the
 * expected colors are the canon flair_canon_rgb(idx) values (spec/assets/
 * color_canon.h) -- the SAME independently-decomp-graded canon test-color-canon
 * vouches for -- NEVER the render source flair_palette_rgb, NEVER preview.webp.
 * The modal geometry (moveable-titled movableDBoxProc chrome, beads
 * initech-zvo6; bounds {140,200,500,280}) comes from os/flair/dialog.c
 * FILECOPY_* + spec/chrome_metrics.h + the drag delta, NOT from the artifact's
 * render.
 *
 * Usage: ppm_flair_dc4v_check <screendump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * locked (-260,0) delta and the test_shell.c W1/modal geometry.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color_canon.h"   /* flair_canon_rgb (-Ispec/assets) -- the canon the
                            * INDEPENDENT test-color-canon grades, not us.       */

/* Per-channel tolerance: capture-noise only (the XRGB8888 -> P6 dump is exact),
 * mirroring ppm_flair_drag_check / ppm_flair_check. */
#define TOL 2

/* ---- canon palette indices (spec/assets; the values are flair_canon_rgb) ---- */
#define CIDX_FRAME     0   /* black frame / ink                 */
#define CIDX_WHITE     1   /* window/content white              */
#define CIDX_TEAL      2   /* Initech teal #8DDCDC (desktop bg)  */
#define CIDX_MENUBAR   3   /* menu-bar white #FFFFFF             */

/* ---- the post-drag modal geometry (dialog.c FILECOPY_* -- INDEPENDENT of the
 * render). Bounds {left=140, top=200, right=500, bottom=280}; dBoxProc border 7px.
 * The ~260 px LEFT drag (W1 300->40) vacates the modal's right half; the fix keeps
 * it from being seafoam-erased. Probe the RIGHT half only (defect a, the empirically
 * -proven erase case): all coords are x in [360,500) y in [200,280). */
#define MODAL_R        500
#define MODAL_RB_X     499   /* right frame column (the PLAIN 1px frame; was
                              * 496 -- inside the OLD 7px black band -- before
                              * initech-zvo6's moveable-titled chrome)          */
#define MODAL_MID_Y    240
#define MODAL_INT_X    450   /* interior / progress bar (white content)           */

/* the right-half survival band (entirely inside the modal AND inside the vacated
 * seafoam-erase zone x[360,560)y[120,360) of the pre-fix build). BAND_Y0 starts
 * BELOW the 19px pinstripe title band (content_top = DT+TITLEBAR_H = 219; was
 * 206 under the old 7px-border layout, where content began at DT+7=207) so the
 * >=80% white/black check isn't diluted by legitimate non-white/black pinstripe
 * (idx7/8) / bevel (idx2/4) pixels in the title band -- those are NOT erased
 * teal either way, but they are also not "white/black chrome". */
#define BAND_X0        366
#define BAND_X1        498
#define BAND_Y0        220
#define BAND_Y1        278

/* the Photoshop (second) bar run: y=30 is rows [20,40); x[70,350] crosses titles. */
#define BAR_Y          30
#define BAR_X0         70
#define BAR_X1         350

/* ---- PPM P6 reader (the ppm_flair_drag_check invariant). ------------------- */
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

static int g_fail = 0;

static void assert_idx(int x, int y, int idx, const char *what)
{
    if (!is_rgb(x, y, IDX(idx))) {
        const unsigned char *p = at(x, y);
        unsigned int e = IDX(idx);
        fprintf(stderr,
                "ppm_flair_dc4v_check: FAIL %s\n"
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
    if (!f) { fprintf(stderr, "ppm_flair_dc4v_check: cannot open %s\n", argv[1]); return 2; }
    int c0 = fgetc(f), c1 = fgetc(f);
    if (c0 != 'P' || c1 != '6') { fprintf(stderr, "ppm_flair_dc4v_check: not a P6 PPM\n"); return 2; }
    long maxv;
    if (read_uint(f, &g_w) || read_uint(f, &g_h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_dc4v_check: bad PPM header\n"); return 2;
    }
    if (g_w < 640 || g_h < 480) {
        fprintf(stderr, "ppm_flair_dc4v_check: screendump %ldx%ld < 640x480\n", g_w, g_h);
        return 2;
    }
    g_buf = (unsigned char *)malloc((size_t)g_w * g_h * 3);
    if (!g_buf) { fprintf(stderr, "ppm_flair_dc4v_check: OOM\n"); return 2; }
    if (fread(g_buf, 1, (size_t)g_w * g_h * 3, f) != (size_t)g_w * g_h * 3) {
        fprintf(stderr, "ppm_flair_dc4v_check: short read\n"); free(g_buf); fclose(f); return 2;
    }
    fclose(f);

    printf("ppm_flair_dc4v_check: grading the post-drag frame "
           "(W1 (300,120)->(40,120), the locked -260,0 dc4v trace; modal+bars SURVIVE?)\n");

    /* ---- LEG A: bare-desktop anchor is canon teal (makes the teal checks mean
     * something). ---------------------------------------------------------- */
    assert_idx(20, 460, CIDX_TEAL,
               "LEG A: bare-desktop corner (20,460) is canon idx2 teal");

    /* ---- LEG M: THE MODAL SURVIVES the drag across it. --------------------- */
    assert_idx(MODAL_RB_X, MODAL_MID_Y, CIDX_FRAME,
               "LEG M: modal right frame (499,240) is canon idx0 black (not erased)");
    assert_idx(MODAL_INT_X, MODAL_MID_Y, CIDX_WHITE,
               "LEG M: modal interior (450,240) is canon idx1 white (not erased)");
    {
        long tot = 0, chrome = 0, teal = 0;
        for (int y = BAND_Y0; y < BAND_Y1; y++)
            for (int x = BAND_X0; x < BAND_X1; x++) {
                tot++;
                if (is_rgb(x, y, IDX(CIDX_WHITE)) || is_rgb(x, y, IDX(CIDX_FRAME)))
                    chrome++;   /* modal box / text / progress-bar pixels        */
                if (is_rgb(x, y, IDX(CIDX_TEAL)))
                    teal++;     /* seafoam-erased pixels (the pre-fix bug)        */
            }
        int chrome_ok = (chrome * 100 >= tot * 80);
        int teal_ok   = (teal * 100 <= tot * 5);
        if (!(chrome_ok && teal_ok)) {
            fprintf(stderr,
                    "ppm_flair_dc4v_check: FAIL LEG M -- modal right-half band "
                    "x[%d,%d) y[%d,%d) NOT intact: white/black=%ld/%ld (%.1f%%, "
                    "need >=80%%), teal=%ld/%ld (%.1f%%, need <=5%%): the drag "
                    "ERASED the modal (pipa pre-fix compositor / IGNORE_OVERLAY)\n",
                    BAND_X0, BAND_X1, BAND_Y0, BAND_Y1,
                    chrome, tot, 100.0 * (double)chrome / (double)tot,
                    teal, tot, 100.0 * (double)teal / (double)tot);
            g_fail = 1;
        } else {
            printf("    LEG M: the modal SURVIVED the drag "
                   "(band x[%d,%d)y[%d,%d): %.1f%% white/black, %.1f%% teal)\n",
                   BAND_X0, BAND_X1, BAND_Y0, BAND_Y1,
                   100.0 * (double)chrome / (double)tot,
                   100.0 * (double)teal / (double)tot);
        }
    }

    /* ---- LEG B: THE PHOTOSHOP MENU BAR SURVIVES (never overpainted/erased). - */
    {
        long tot = 0, white = 0, teal = 0;
        for (int x = BAR_X0; x < BAR_X1; x++) {
            tot++;
            if (is_rgb(x, BAR_Y, IDX(CIDX_MENUBAR)))  /* == idx1 white #FFFFFF   */
                white++;
            if (is_rgb(x, BAR_Y, IDX(CIDX_TEAL)))
                teal++;
        }
        int white_ok = (white * 100 >= tot * 40);  /* white bg dominates the run */
        int teal_ok  = (teal * 100 <= tot * 5);
        if (!(white_ok && teal_ok)) {
            fprintf(stderr,
                    "ppm_flair_dc4v_check: FAIL LEG B -- Photoshop bar run y=%d "
                    "x[%d,%d) NOT intact: menubar-white=%ld/%ld (%.1f%%, need "
                    ">=40%%), teal=%ld/%ld (%.1f%%, need <=5%%): the bar was "
                    "overpainted/erased (pipa)\n",
                    BAR_Y, BAR_X0, BAR_X1,
                    white, tot, 100.0 * (double)white / (double)tot,
                    teal, tot, 100.0 * (double)teal / (double)tot);
            g_fail = 1;
        } else {
            printf("    LEG B: the Photoshop menu bar SURVIVED "
                   "(run y=%d x[%d,%d): %.1f%% menubar-white, %.1f%% teal)\n",
                   BAR_Y, BAR_X0, BAR_X1,
                   100.0 * (double)white / (double)tot,
                   100.0 * (double)teal / (double)tot);
        }
    }

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_dc4v_check: FAIL -- dragging a window across the "
                "modal + bars ERASED/overpainted them (the non-WindowMgr always-on-"
                "top layers are not damage-occluded; initech-pipa)\n");
        return 1;
    }
    printf("ppm_flair_dc4v_check: PASS -- the modal FILE COPY box + the menu bars "
           "SURVIVED a window dragged across them (initech-pipa: wm->overlay_rgn "
           "folded into fronts_union; ADR-0005 region spine, Law 2/4)\n");
    return 0;
}

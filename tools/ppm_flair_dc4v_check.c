/*
 * ppm_flair_dc4v_check.c -- the initech-dc4v EMU survival oracle's screendump
 * grader (HOST, C-only). beads initech-dc4v (the RED half of the compositor fix
 * initech-pipa). It grades the POST-DRAG screendump of the booted BOOT_FLAIR_LIVE
 * desktop (show_modal=1 -- the ONLY scene with the modal) after the locked R1.4
 * trace dragged the FILE COPY modal 140 px LEFT, from {L140,T200} to {L0,T200},
 * across window 1. The direction is now MODAL-over-window: the modal must remain
 * intact at its new rect and the old right-half footprint must restore the
 * document content below it.
 *
 * RULE-8 RE-KEY (initech-zn61, orchestrator rework): the old trace dragged a
 * document window from behind the frontmost modal, encoding zn61's input leak as
 * a golden. Correct modality blocks that gesture. The original initech-pipa
 * WINDOW_MUTATE_IGNORE_OVERLAY proof moves to test-flair-shell-mutant's forward
 * window-across-overlay host scene, where the mechanism still intersects. This
 * reverse emu scene is mutation-proven by FLAIR_LIVE_MUTATE_MODAL_NO_RESTORE.
 *
 * THE REVERSE SURVIVAL DIFFERENTIAL:
 *
 *   LEG M -- THE MODAL SURVIVES AT ITS NEW RECT. Right frame (359,240) is black,
 *     interior (330,240) is E7, and the clear body band x[226,358),y[222,247)
 *     is >=80% E7/white/black and <=5% teal.
 *
 *   LEG V -- THE VACATED WINDOW CONTENT IS RESTORED. The old-modal-only band
 *     x[366,498),y[222,278) lies inside window 1 content after the modal moves.
 *     It must be >=90% white and <=5% teal. MODAL_NO_RESTORE leaves stale E7/
 *     progress pixels there and goes RED.
 *
 *   LEG B -- THE PHOTOSHOP MENU BAR SURVIVES.  A run across the second (Photoshop)
 *     bar at y=30, x[70,350], is dominated by sampled Platinum face (idx231 ==
 *     #E7E7E7) with black title ink, and shows essentially NO teal -- the bars are
 *     an always-on-top layer the compositor must never let a window/erase touch.
 *
 *   LEG A -- a bare-desktop corner sanity anchor (20,460) reads canon idx2 teal
 *     (the desktop really is the canon teal, so the LEG-M teal check is meaningful).
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0010 / the ppm_flair_drag_check pattern): the
 * expected colors are the canon flair_canon_rgb(idx) values (spec/assets/
 * color_canon.h) -- the SAME independently-decomp-graded canon test-color-canon
 * vouches for -- NEVER the render source flair_palette_rgb, NEVER preview.webp.
 * Modal geometry is hardcoded from the locked old rect plus the trace delta,
 * not read from dialog/chrome constants or the artifact render.
 *
 * Usage: ppm_flair_dc4v_check <screendump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * locked (-140,0) delta and the test_shell.c W1/modal geometry.
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
#define CIDX_FACE      231 /* sampled Platinum dialog/menu face #E7E7E7 */

/* ---- post-drag modal geometry: old {140,200,500,280} + (-140,0) =>
 * new {0,200,360,280}. Hardcoded independent golden. */
#define MODAL_R        360
#define MODAL_RB_X     359
#define MODAL_MID_Y    240
#define MODAL_INT_X    330

/* Clear modal-body band below the title and above the progress item. */
#define BAND_X0        226
#define BAND_X1        358
#define BAND_Y0        222
#define BAND_Y1        247

/* Old-modal-only band now exposing window 1 content. */
#define VAC_X0         366
#define VAC_X1         498
#define VAC_Y0         222
#define VAC_Y1         278

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

    printf("ppm_flair_dc4v_check: reverse mode -- FILE COPY modal "
           "(140,200)->(0,200), locked -140,0 trace across window 1\n");

    /* ---- LEG A: bare-desktop anchor is canon teal (makes the teal checks mean
     * something). ---------------------------------------------------------- */
    assert_idx(20, 460, CIDX_TEAL,
               "LEG A: bare-desktop corner (20,460) is canon idx2 teal");

    /* ---- LEG M: THE MODAL SURVIVES AT ITS NEW POSITION. ------------------- */
    assert_idx(MODAL_RB_X, MODAL_MID_Y, CIDX_FRAME,
               "LEG M: moved modal right frame (359,240) is black");
    assert_idx(MODAL_INT_X, MODAL_MID_Y, CIDX_FACE,
               "LEG M: moved modal interior (330,240) is sampled E7");
    {
        long tot = 0, chrome = 0, teal = 0;
        for (int y = BAND_Y0; y < BAND_Y1; y++)
            for (int x = BAND_X0; x < BAND_X1; x++) {
                tot++;
                if (is_rgb(x, y, IDX(CIDX_FACE)) ||
                    is_rgb(x, y, IDX(CIDX_WHITE)) ||
                    is_rgb(x, y, IDX(CIDX_FRAME)))
                    chrome++;   /* modal box / text / progress-bar pixels        */
                if (is_rgb(x, y, IDX(CIDX_TEAL)))
                    teal++;     /* seafoam-erased pixels (the pre-fix bug)        */
            }
        int chrome_ok = (chrome * 100 >= tot * 80);
        int teal_ok   = (teal * 100 <= tot * 5);
        if (!(chrome_ok && teal_ok)) {
            fprintf(stderr,
                    "ppm_flair_dc4v_check: FAIL LEG M -- moved modal body band "
                    "x[%d,%d) y[%d,%d) NOT intact: E7/white/black=%ld/%ld (%.1f%%, "
                    "need >=80%%), teal=%ld/%ld (%.1f%%, need <=5%%): the drag "
                    "is not intact at the new position\n",
                    BAND_X0, BAND_X1, BAND_Y0, BAND_Y1,
                    chrome, tot, 100.0 * (double)chrome / (double)tot,
                    teal, tot, 100.0 * (double)teal / (double)tot);
            g_fail = 1;
        } else {
            printf("    LEG M: the modal is intact at the new position "
                   "(band x[%d,%d)y[%d,%d): %.1f%% E7/white/black, %.1f%% teal)\n",
                   BAND_X0, BAND_X1, BAND_Y0, BAND_Y1,
                   100.0 * (double)chrome / (double)tot,
                   100.0 * (double)teal / (double)tot);
        }
    }

    /* ---- LEG V: OLD FOOTPRINT RESTORES WINDOW-1 CONTENT. ------------------ */
    {
        long tot = 0, white = 0, teal = 0;
        for (int y = VAC_Y0; y < VAC_Y1; y++)
            for (int x = VAC_X0; x < VAC_X1; x++) {
                tot++;
                if (is_rgb(x, y, IDX(CIDX_WHITE))) white++;
                if (is_rgb(x, y, IDX(CIDX_TEAL))) teal++;
            }
        if (!(white * 100 >= tot * 90 && teal * 100 <= tot * 5)) {
            fprintf(stderr,
                    "ppm_flair_dc4v_check: FAIL LEG V -- vacated window-1 band "
                    "x[%d,%d)y[%d,%d) not restored: white=%ld/%ld (%.1f%%, "
                    "need >=90%%), teal=%ld/%ld (%.1f%%, need <=5%%); stale "
                    "modal pixels or a desktop-fill hole remain\n",
                    VAC_X0, VAC_X1, VAC_Y0, VAC_Y1,
                    white, tot, 100.0 * (double)white / (double)tot,
                    teal, tot, 100.0 * (double)teal / (double)tot);
            g_fail = 1;
        } else {
            printf("    LEG V: vacated window-1 band restored (%.1f%% white, "
                   "%.1f%% teal)\n",
                   100.0 * (double)white / (double)tot,
                   100.0 * (double)teal / (double)tot);
        }
    }

    /* ---- LEG B: THE PHOTOSHOP MENU BAR SURVIVES (never overpainted/erased). - */
    {
        long tot = 0, face = 0, teal = 0;
        for (int x = BAR_X0; x < BAR_X1; x++) {
            tot++;
            if (is_rgb(x, BAR_Y, IDX(CIDX_FACE)))
                face++;
            if (is_rgb(x, BAR_Y, IDX(CIDX_TEAL)))
                teal++;
        }
        int face_ok = (face * 100 >= tot * 40);  /* E7 face dominates the run */
        int teal_ok  = (teal * 100 <= tot * 5);
        if (!(face_ok && teal_ok)) {
            fprintf(stderr,
                    "ppm_flair_dc4v_check: FAIL LEG B -- Photoshop bar run y=%d "
                    "x[%d,%d) NOT intact: menu-E7=%ld/%ld (%.1f%%, need "
                    ">=40%%), teal=%ld/%ld (%.1f%%, need <=5%%): the bar was "
                    "overpainted/erased (pipa)\n",
                    BAR_Y, BAR_X0, BAR_X1,
                    face, tot, 100.0 * (double)face / (double)tot,
                    teal, tot, 100.0 * (double)teal / (double)tot);
            g_fail = 1;
        } else {
            printf("    LEG B: the Photoshop menu bar SURVIVED "
                   "(run y=%d x[%d,%d): %.1f%% menu-E7, %.1f%% teal)\n",
                   BAR_Y, BAR_X0, BAR_X1,
                   100.0 * (double)face / (double)tot,
                   100.0 * (double)teal / (double)tot);
        }
    }

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_dc4v_check: FAIL -- moved modal/vacated "
                "window/menu-bar reverse-survival contract broken\n");
        return 1;
    }
    printf("ppm_flair_dc4v_check: PASS -- moved modal intact, vacated window "
           "content restored, menu bars intact (reverse survival; Law 2/4)\n");
    return 0;
}

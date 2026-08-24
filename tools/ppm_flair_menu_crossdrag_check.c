/*
 * ppm_flair_menu_crossdrag_check.c -- the CROSS-MENU DRAG live-redraw oracle's
 * screendump grader (HOST, C-only). beads initech-9op1 (found during
 * initech-rl4v): flair_live_do_menu's per-tick redraw used to freeze `mi` at the
 * click and re-hilite via MenuInfo_item_at(bar, mi, ...) with that frozen `mi`
 * for the WHOLE drag -- so even though menu.c's flair_menu_track (initech-rl4v)
 * already re-hits the bar per tracked point and MenuSelect's FINAL result was
 * correct, the ON-SCREEN drop lagged: the originally-clicked menu's panel stayed
 * visible while the cursor was over a DIFFERENT title.
 *
 * This grades the screendump of the booted BOOT_FLAIR_LIVE desktop AT the
 * FLAIR-MENU-XDROP marker during the locked CROSS-MENU trace: click the
 * System-7 "File" title (menu 0), DROP, then drag SIDEWAYS along the bar band
 * onto the "Edit" title (menu 1), staying in the bar band the whole time. Fixed
 * behaviour at that mid-track marker: "File"'s panel is ERASED and "Edit"'s
 * panel is DROPPED with no item hilited. The later release remains in the title
 * bar and the Makefile independently requires the final sel=0 marker.
 * Buggy (frozen-mi) behaviour: "File"'s panel STAYS on screen and "Edit"'s is
 * never drawn.
 *
 * THE DIFFERENTIAL (independently catches the KMAIN_MUT_MENU_NO_REHIT mutant,
 * which freezes `mi` in the live loop -- see test-flair-menu-crossdrag-mutant):
 *
 *   LEG A -- "FILE" PANEL ERASED.  A column UNIQUE to File's panel footprint
 *     (File panel {T19 L20 B53 R112}; Edit starts at L66 -- x=20..65 is
 *     File-only) sampled in the teal zone (y=46, below BOTH 20px menu bars) must
 *     be bare Initech teal again -- the panel that WAS there (a black frame post-
 *     drop) is gone. Under the mutant, File's panel is never erased, so this
 *     stays the black frame -> LEG A RED.
 *
 *   LEG B -- "EDIT" PANEL DROPPED (frame).  A column UNIQUE to Edit's panel
 *     footprint (Edit panel {T19 L66 B53 R158}; the right frame column x=157 is
 *     Edit-only) sampled in the teal zone (y=46) must
 *     be the canon BLACK frame. Under the mutant, Edit's panel is never drawn, so
 *     this stays bare teal -> LEG B RED.
 *
 *   LEG C -- "EDIT" PANEL BODY (sampled E7). The un-hilited "About" row
 *     (y[21,37)) of Edit's panel, sampled at its right gutter (x=152, Edit-only,
 *     clear of glyphs) must be sampled #E7E7E7.
 *
 *   LEG D -- a bare-desktop corner sanity anchor (the canon teal is really teal).
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0006 E-D5/BC-5): the expected colors are the
 * canon flair_canon_rgb(idx) values (spec/assets/color_canon.h) -- the SAME
 * independently-decomp-graded canon test-color-canon vouches for, never the
 * render source flair_palette_rgb. The panel GEOMETRY (File left=20=Apple slot
 * width; Edit left=66=File's slot width 46 later; the fixed 20px-bar-relative
 * item rows) comes from os/flair/menu.h + the test_menu-graded
 * MenuInfo_panel_rect for the kmain.c System-7 sys-menu bar (4 titles, 2 items
 * each: "About"/"Quit"), NOT from the artifact's render.
 *
 * Usage: ppm_flair_menu_crossdrag_check <screendump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * locked FLAIR_MENU_CROSSDRAG_SPEC trace and the kmain.c System-7 bar geometry.
 */
#include <stdio.h>
#include <stdlib.h>

#include "color_canon.h"   /* flair_canon_rgb (-Ispec/assets) -- the canon the
                            * INDEPENDENT test-color-canon grades, not us.       */

/* Per-channel tolerance: capture-noise only, mirroring ppm_flair_menu_check. */
#define TOL 2

/* ---- canon palette indices (spec/assets; the values are flair_canon_rgb) ---- */
#define CIDX_FRAME     0   /* black frame / ink / hilite band   */
#define CIDX_TEAL      2   /* Initech teal #8DDCDC (desktop bg)  */
#define CIDX_FACE      231 /* sampled Platinum face #E7E7E7      */

/* ---- the kmain.c System-7 bar geometry (4 titles File/Edit/View/Special, each
 * with 2 items "About"/"Quit"; os/flair/menu.h + MenuInfo_panel_rect, host-graded
 * by a throwaway geometry dump against the SAME sys-menu bar construction kmain.c
 * builds -- FLAIR_MENU_APPLE_W=20, title slot widths from FONT_CHICAGO's
 * text_measure). File panel = {T19 L20 B53 R112}; Edit panel =
 * {T19 L66 B53 R158}. The right edge expands for the rendered "^Q" command
 * column (sys8/menus.md Sec 2.2/2.3; bead initech-sjvq). Edit's title
 * starts exactly at File's slot width, 46px, after the L20 Apple slot). Row
 * geometry (fixed item heights, not text-width) mirrors ppm_flair_menu_check.c:
 *   About row y[21,37), Quit row y[37,53), bottom frame y=52, shadow y=53.
 * ---------------------------------------------------------------------------- */
#define FILE_PANEL_L      20    /* File panel left frame column                 */
#define FILE_UNIQUE_X     30    /* a column in File's footprint, NOT Edit's      */
#define EDIT_PANEL_L      66    /* Edit panel left frame column                 */
#define EDIT_PANEL_R1     157   /* Edit panel right frame column (right-1)      */
#define EDIT_RPAD_X       152   /* Edit right gutter, clear of glyphs            */
/* a y inside the TEAL zone (below both 20px bars, above the y>=60 windows) --
 * inside BOTH panels' "Quit" row [37,53). */
#define QUIT_Y            46
/* a y inside the un-hilited "About" row [21,37). */
#define ABOUT_Y           28

/* ---- PPM P6 reader (the ppm_flair_check family invariant). ------------------ */
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
                "ppm_flair_menu_crossdrag_check: FAIL %s\n"
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
    if (!f) {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: cannot open %s\n", argv[1]);
        return 2;
    }
    int c0 = fgetc(f), c1 = fgetc(f);
    if (c0 != 'P' || c1 != '6') {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: not a P6 PPM\n");
        return 2;
    }
    long maxv;
    if (read_uint(f, &g_w) || read_uint(f, &g_h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: bad PPM header\n");
        return 2;
    }
    if (g_w < 640 || g_h < 480) {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: screendump %ldx%ld < 640x480\n",
                g_w, g_h);
        return 2;
    }
    g_buf = (unsigned char *)malloc((size_t)g_w * g_h * 3);
    if (!g_buf) { fprintf(stderr, "ppm_flair_menu_crossdrag_check: OOM\n"); return 2; }
    if (fread(g_buf, 1, (size_t)g_w * g_h * 3, f) != (size_t)g_w * g_h * 3) {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: short read\n");
        free(g_buf); fclose(f); return 2;
    }
    fclose(f);

    printf("ppm_flair_menu_crossdrag_check: grading the cross-menu XDROP "
           "(click File, drag along the bar into Edit while held) -- File's "
           "panel {T19 L20 B53 R112} must be ERASED and "
           "Edit's panel {T19 L66 B53 R158} must be DROPPED (initech-9op1)\n");

    /* ---- LEG A: "File" panel ERASED (black frame -> teal again) ------------ */
    assert_idx(FILE_PANEL_L, QUIT_Y, CIDX_TEAL,
               "LEG A: File's panel LEFT frame column (x=20) is bare teal again "
               "(the stale panel was erased on the title switch)");
    assert_idx(FILE_UNIQUE_X, QUIT_Y, CIDX_TEAL,
               "LEG A: a File-only column (x=30) is bare teal again "
               "(no stale File panel survives the switch to Edit)");
    if (!g_fail) {
        printf("    LEG A: the stale 'File' panel is gone -- bare teal where its "
               "black frame was (the live redraw does not lag)\n");
    }

    /* ---- LEG B: "Edit" panel DROPPED (frame) -------------------------------- */
    assert_idx(EDIT_PANEL_L, QUIT_Y, CIDX_FRAME,
               "LEG B: Edit's panel LEFT frame column (x=66) is idx0 black");
    assert_idx(EDIT_PANEL_R1, QUIT_Y, CIDX_FRAME,
               "LEG B: Edit's panel RIGHT frame column (x=157, Edit-only) is "
               "idx0 black");
    if (!g_fail) {
        printf("    LEG B: the 'Edit' panel's black frame is present where bare "
               "teal (and no stale File panel) was\n");
    }

    /* ---- LEG C: "Edit" panel BODY (sampled E7) ------------------------------- */
    assert_idx(EDIT_RPAD_X, ABOUT_Y, CIDX_FACE,
               "LEG C: Edit's un-hilited 'About' row body (x=152, Edit-only) is "
               "sampled idx231 E7");
    if (!g_fail) {
        printf("    LEG C: the 'Edit' panel's sampled E7 body fill is present "
               "(the panel really dropped, not just a frame)\n");
    }

    /* ---- LEG D: a bare-desktop corner sanity anchor ------------------------- */
    assert_idx(20, 460, CIDX_TEAL,
               "LEG D: bare-desktop corner (20,460) is idx2 teal");

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_menu_crossdrag_check: FAIL -- the live drop "
                "did not follow the cross-menu drag (Law 4: the on-screen menu "
                "lagged behind the tracked selection; initech-9op1)\n");
        return 1;
    }
    printf("ppm_flair_menu_crossdrag_check: PASS -- 'File' erased + 'Edit' "
           "dropped live as the cursor crossed titles (the on-screen drop "
           "follows the drag, not just the final MenuSelect result; "
           "initech-9op1)\n");
    return 0;
}

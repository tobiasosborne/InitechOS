/*
 * ppm_flair_menu_check.c -- the FO-8b EMU menu oracle's screendump grader (HOST,
 * C-only). beads initech-5l5z FO-8b (ADR-0004 D-3 / ADR-0006 FO-8 -- inMenuBar ->
 * MenuSelect). It grades the screendump of the booted BOOT_FLAIR_LIVE desktop
 * AT the FLAIR-MENU-DROP marker: the pump has drawn and presented the Platinum
 * "File" pull-down while the button is held, before tracking has hilited an item.
 * The serial half of the gate independently follows the completed gesture and
 * requires MenuSelect item 2 ("Quit"). This mid-track capture keeps the oracle
 * structural without requiring the live artifact to leave a menu open after
 * mouseUp (bead initech-b3hl; CLAUDE.md Law 2).
 *
 * THE DIFFERENTIAL (each leg independently catches the HER-14 "menus do not work"
 * heresy mutant FLAIR_LIVE_MUTATE_MENU_NOOP, which emits FLAIR-MENU sel=0 but
 * drops NO panel -- the desktop under the title stays bare):
 *
 *   LEG A -- THE 1px BLACK PANEL FRAME where BARE TEAL was. The dropped "File"
 *     panel rect is {T19 L20 B53 R112}, footprint {T19 L20 B54 R113}
 *     (MenuInfo_panel_rect/footprint_rect, host-graded by
 *     test_menu).  Its frame is canon BLACK (idx0).  We sample the LEFT frame
 *     column (x=20), the RIGHT frame column (x=111), and the BOTTOM frame row
 *     (y=52) at y/x INSIDE the teal zone (y>=40, below BOTH 20px menu bars and
 *     above the windows at y>=60) -- bare Initech teal (idx2 #8DDCDC) pre-drop.
 *     A menu that did NOT drop leaves teal here and LEG A goes RED.
 *
 *   LEG B -- THE UNHILITED SECOND-ROW BODY is painted. At DROP time no tracking
 *     point has hilited "Quit" yet, so its row y[37,53) remains E7 face.
 *     We sample the mark column (x=28) and right pad (x=106), clear of glyphs.
 *     Both were bare teal pre-drop; a no-drop path leaves teal -> LEG B RED.
 *
 *   LEG C -- THE E7 PANEL BODY fill is present. The un-hilited first row
 *     ("About", y[21,37)) shows sampled #E7E7E7 (idx231). We sample its right
 *     pad (x=106,y=28), clear of glyphs. A no-drop path leaves the underlying
 *     band-2 pixels and therefore differs.
 *
 *   LEG E -- sampled Platinum fidelity on metal: title-free bar rows are
 *     white/E7/B3/black, the pulled File block is teal with white title ink,
 *     and the menu shadow is the distinct #3F3F3F row (not window black).
 *
 *   LEG D -- a bare-desktop corner sanity anchor (the canon teal is really teal).
 *
 * INDEPENDENT GOLDEN (Law 2; ADR-0006 E-D5/BC-5): the expected colors are the
 * canon flair_canon_rgb(idx) values (spec/assets/color_canon.h), the SAME
 * independently-decomp-graded canon test-color-canon vouches for -- never the
 * render source flair_palette_rgb, never preview.webp.  The panel GEOMETRY
 * (left=20 = MenuBar_title_x(File); the 20px-bar-relative rows; the 1px frame)
 * comes from os/flair/menu.h + the test_menu-graded MenuInfo_panel_rect, NOT from
 * the artifact's render.  The load-bearing assertions are GEOMETRIC differentials
 * (panel chrome where bare teal / menubar-white was) that flip under the menu-noop
 * mutant.
 *
 * Usage: ppm_flair_menu_check <screendump.ppm>
 * Exit 0 = PASS; non-zero = a named FAIL (the assertion + sampled-vs-expected RGB).
 *
 * ASCII-clean (Rule 12). Deterministic (Rule 11): fixed probe coords from the
 * locked trace's selection and the test_shell.c first-band "File" geometry.
 */
#include <stdio.h>
#include <stdlib.h>

#include "color_canon.h"   /* flair_canon_rgb (-Ispec/assets) -- the canon the
                            * INDEPENDENT test-color-canon grades, not us.       */

/* Per-channel tolerance: capture-noise only (the XRGB8888 -> P6 dump is exact;
 * canon entries differ by far more than 2/channel), mirroring ppm_flair_check. */
#define TOL 2

/* ---- canon palette indices (spec/assets; the values are flair_canon_rgb) ---- */
#define CIDX_FRAME     0   /* black frame / ink / hilite band   */
#define CIDX_TEAL      2   /* Initech teal #8DDCDC (desktop bg)  */
#define CIDX_WHITE     1   /* sampled bar/panel highlight #FFFFFF */
#define CIDX_FACE      231 /* sampled Platinum face #E7E7E7      */
#define CIDX_B3        179 /* sampled profile/panel shadow         */
#define CIDX_DROP      63  /* sampled menu-only shadow #3F3F3F    */

/* ---- the dropped "File" pull-down geometry (os/flair/menu.h + the test_shell.c
 * first bar; MenuInfo_panel_rect(bar_sys,0), test_menu-graded). -----------------
 * Panel rect {T19 L20 B53 R112}; shadow footprint ends R113/B54. Rows remain:
 * About y[21,37), Quit y[37,53). Bottom frame y=52; shadow y=53.
 * Width grows because Quit now renders the right-aligned "^Q" column.
 * Ref: sys8/menus.md Sec 2.1-2.3; bead initech-sjvq. */
#define PANEL_L      20    /* left frame column (MenuBar_title_x(File)=APPLE_W) */
#define PANEL_R1     111   /* right frame column (panel.right-1)                */
#define PANEL_SH_X   112   /* one-pixel dark-gray right shadow                  */
#define PANEL_BOTY   52    /* bottom frame row (panel.bottom-1)                 */
#define PANEL_SH_Y   53    /* one-pixel dark-gray bottom shadow                 */
/* a y inside the TEAL zone (below both 20px bars, above the y>=60 windows) that
 * lands in the selected "Quit" row [37,53): bare teal pre-drop. */
#define QUIT_Y       46
/* a y inside the un-hilited "About" row [21,37). */
#define ABOUT_Y      28
/* clean x columns (mark column / right pad) clear of the item glyph cells. */
#define MARK_X       28    /* mark column x[21,37): no glyph                    */
#define RPAD_X       106   /* right gutter, clear of text + caret-letter glyphs */

/* ---- PPM P6 reader (the ppm_flair_check / ppm_flair_drag_check invariant). -- */
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
                "ppm_flair_menu_check: FAIL %s\n"
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
    if (!f) { fprintf(stderr, "ppm_flair_menu_check: cannot open %s\n", argv[1]); return 2; }
    int c0 = fgetc(f), c1 = fgetc(f);
    if (c0 != 'P' || c1 != '6') { fprintf(stderr, "ppm_flair_menu_check: not a P6 PPM\n"); return 2; }
    long maxv;
    if (read_uint(f, &g_w) || read_uint(f, &g_h) || read_uint(f, &maxv)) {
        fprintf(stderr, "ppm_flair_menu_check: bad PPM header\n"); return 2;
    }
    if (g_w < 640 || g_h < 480) {
        fprintf(stderr, "ppm_flair_menu_check: screendump %ldx%ld < 640x480\n", g_w, g_h);
        return 2;
    }
    g_buf = (unsigned char *)malloc((size_t)g_w * g_h * 3);
    if (!g_buf) { fprintf(stderr, "ppm_flair_menu_check: OOM\n"); return 2; }
    if (fread(g_buf, 1, (size_t)g_w * g_h * 3, f) != (size_t)g_w * g_h * 3) {
        fprintf(stderr, "ppm_flair_menu_check: short read\n"); free(g_buf); fclose(f); return 2;
    }
    fclose(f);

    printf("ppm_flair_menu_check: grading the held Platinum 'File' pull-down "
           "at FLAIR-MENU-DROP (no item hilited yet; panel "
           "{T19 L20 B53 R112}, shadow footprint {T19 L20 B54 R113})\n");

    /* ---- LEG A: THE 1px BLACK PANEL FRAME where bare teal was (teal->black) -- */
    assert_idx(PANEL_L, QUIT_Y, CIDX_FRAME,
               "LEG A: panel LEFT frame (x=20) in the teal zone is idx0 black");
    assert_idx(PANEL_R1, QUIT_Y, CIDX_FRAME,
               "LEG A: panel RIGHT frame (x=89) in the teal zone is idx0 black");
    assert_idx(MARK_X, PANEL_BOTY, CIDX_FRAME,
               "LEG A: panel BOTTOM frame (y=52) is idx0 black");
    assert_idx(RPAD_X, PANEL_BOTY, CIDX_FRAME,
               "LEG A: panel BOTTOM frame (y=52, right) is idx0 black");
    if (!g_fail) {
        printf("    LEG A: the 1px black panel frame is present where bare teal "
               "was (the pull-down dropped below the menu bar)\n");
    }

    /* ---- LEG B: THE UNHILITED 'Quit' BODY is sampled E7 --------------------- */
    assert_idx(MARK_X, QUIT_Y, CIDX_FACE,
               "LEG B: held-panel 'Quit' row mark column is idx231 E7 face "
               "before tracking hilites an item");
    assert_idx(RPAD_X, QUIT_Y, CIDX_FACE,
               "LEG B: held-panel 'Quit' row right pad is idx231 E7 face "
               "before tracking hilites an item");
    if (!g_fail) {
        printf("    LEG B: the held panel's second-row body is sampled E7 "
               "where bare teal was (no premature hilite at DROP)\n");
    }

    /* ---- LEG C: THE E7 PANEL BODY fill --------------------------------------- */
    assert_idx(RPAD_X, ABOUT_Y, CIDX_FACE,
               "LEG C: un-hilited 'About' row body is sampled idx231 E7 face");
    if (!g_fail) {
        printf("    LEG C: the sampled E7 panel body fill is present (the panel "
               "really covers the bar)\n");
    }

    /* ---- LEG D: a bare-desktop corner sanity anchor ------------------------- */
    assert_idx(20, 460, CIDX_TEAL,
               "LEG D: bare-desktop corner (20,460) is idx2 teal");

    /* ---- LEG E: refreshed Platinum bar/title/shadow expectations ------------- */
    assert_idx(300, 0, CIDX_WHITE,
               "LEG E: title-free menu-bar row 0 is sampled white");
    assert_idx(300, 5, CIDX_FACE,
               "LEG E: title-free menu-bar face is sampled E7");
    assert_idx(300, 18, CIDX_B3,
               "LEG E: title-free menu-bar row 18 is sampled B3");
    assert_idx(300, 19, CIDX_FRAME,
               "LEG E: title-free menu-bar row 19 is black baseline");
    assert_idx(18, 5, CIDX_TEAL,
               "LEG E: pulled File title block is ratified teal while held");
    assert_idx(28, 4, CIDX_WHITE,
               "LEG E: pulled File title text is white");
    assert_idx(PANEL_SH_X, QUIT_Y, CIDX_DROP,
               "LEG E: panel right shadow is distinct sampled 3F");
    assert_idx(RPAD_X, PANEL_SH_Y, CIDX_DROP,
               "LEG E: panel bottom shadow is distinct sampled 3F");

    free(g_buf);
    if (g_fail) {
        fprintf(stderr, "ppm_flair_menu_check: FAIL -- the live menu did not drop "
                "an unhilited held pull-down where bare teal was "
                "(Law 4: the menus do not actually work)\n");
        return 1;
    }
    printf("ppm_flair_menu_check: PASS -- the Platinum 'File' pull-down DROPPED "
           "live while held (profile + teal title + bevel/shadow + E7 body) over the "
           "previously-teal desktop; the booted desktop has WORKING MENUS "
           "(FO-8b; ADR-0004 D-3 / ADR-0006 FO-8)\n");
    return 0;
}

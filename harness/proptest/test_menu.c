/* test_menu.c -- the FLAIR Menu Manager property suite (THE ORACLE; Law 2).
 *
 * beads: initech-n3e ("FLAIR Menu Manager: pull-down menus + the menu bar,
 *        Photoshop-exact"). The mechanical oracle for the Menu Manager (ADR-0004
 *        D-3): the proportional bar layout, the canon InitechPaint menu bar
 *        (Law 4), the deterministic MenuSelect pull-down tracking, MenuKey, and
 *        the sampled Platinum bar/panel pixel layout. Bead initech-sjvq extends
 *        the same suite with independent sys8/menus.md fidelity legs.
 *
 * Ref:   ADR-0004 D-3 (MenuInfo + the Photoshop-exact bar + MenuSelect ->
 *          (menuID<<16|item)); D-7 (proportional text -- sum of advances); D-1/D-2
 *          (draw through a GrafPort clipped by a region). spec/assets/menu_canon.h
 *          (the FROZEN canon string -- asserted byte-exact). spec/chrome_metrics.h
 *          (FLAIR_CHROME_MENUBAR_H = 20). sys8/menus.md Sec 1-2 (SAMPLED
 *          values). os/flair/menu.h (the unit under test).
 *          harness/render/render.h (the host render skeleton -- the dual-compile
 *          path that runs the SAME freestanding menu.c on a host offscreen).
 *          harness/proptest/test_window.c + test_chrome.c (the harness idiom +
 *          seeded LCG this suite mirrors). CLAUDE.md Law 2/4, Rule 6 (mutation-
 *          proven), Rule 11 (seeded LCG -> deterministic), Rule 12 ASCII.
 *
 * THE PROPERTIES (in order of decisiveness):
 *
 *  1. BAR LAYOUT.  Each title's x-position == the cumulative sum of earlier
 *     PROPORTIONAL title widths (PAD + text_measure + PAD), computed INDEPENDENTLY
 *     here (not by calling the same internal). The bar is 20px. MenuBar_hit maps a
 *     point back to the title whose half-open slot contains it.
 *
 *  2. CANON.  The InitechPaint bar built from menu_canon.h matches the FROZEN
 *     "File Edit Image Layer Select View Window Help" string EXACTLY, item-by-item
 *     and as a flat string (Law 4 -- the canon is the spec, not a bug).
 *
 *  3. MENUSELECT TRACKING.  A recorded point sequence (click 'Edit', move down to
 *     item 3, release) yields the DETERMINISTIC (menuID<<16|item); release outside
 *     -> 0; a disabled item / a divider -> not selectable (0); MenuKey(cmd) -> the
 *     right packing. Deterministic across runs (Rule 11).
 *
 *  4. DRAW/FIDELITY. Render the bar + an open menu and assert the sampled
 *     white/E7/B3/black profile, exact corner transcription, teal pulled title,
 *     black/white/B3/3F panel anatomy, shadow footprint, 6px A5/white separator,
 *     +20 item text origin, caret-letter command column, A5 disabled ink, and
 *     retained classic tracking invert. The footprint tooth locks live restore
 *     geometry to every temporary panel pixel.
 *
 *  5. APPLE GLYPH (initech-yx4v).  The Apple slot renders the hand-authored
 *     apple_glyph.h strike, NOT a solid filled square: rendered ink count in
 *     the slot == the strike's own ink count exactly; ink coverage is 25%-60%
 *     of the slot; a named "bite" cell recedes to background a few rows below
 *     ink at the same column (the bite notch); the leaf-tip row has ink and the
 *     slot's top-left corner is background (not a filled rect); the widest body
 *     row is denser than the leaf-tip row. Properties are DERIVED from the
 *     authored strike (apple_glyph.h) but GRADED against the rendered pixels
 *     (Law 2 -- the render must match the strike, not just "look plausible").
 *
 * MUTANTS (Rule 6), each driven RED by the Makefile gate:
 *   MENU_MUTATE_FIXED_WIDTH     -- titles/items laid out fixed-width, not
 *                                  proportional => property 1 (x-positions) RED.
 *   MENU_MUTATE_SELECT_DISABLED -- a disabled item/divider becomes selectable
 *                                  => property 3 (selectability) RED.
 *   MENU_MUT_APPLE_SQUARE       -- the Apple slot reverts to a solid filled
 *                                  square (the ORIGINAL initech-yx4v bug) =>
 *                                  property 5 (Apple glyph) RED.
 *   MENU_MUT_BAR_FLAT           -- sampled profile/corners -> property 4 RED.
 *   MENU_MUT_NO_PANEL_BEVEL     -- white/B3 inner bevel -> property 4 RED.
 *   MENU_MUT_SEP_PLAIN          -- etched A5/white groove -> property 4 RED.
 *   MENU_MUT_DISABLED_NORMAL_INK -- disabled A5 ink -> property 4 RED.
 *   MENU_MUT_TITLE_NO_HILITE    -- pulled teal/white title -> property 4 RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "menu.h"               /* the Menu Manager under test (-Ios/flair)    */
#include "menu_canon.h"         /* FROZEN canon string (-Ispec/assets)         */
#include "apple_glyph.h"        /* APPLE_GLYPH_* -- hand-authored strike, the
                                 * PROPERTY 5 oracle's own reference (-Ispec/
                                 * assets); initech-yx4v                       */
#include "chrome_metrics.h"     /* FLAIR_CHROME_MENUBAR_H (-Ispec)             */
#include "test_assert.h"        /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)    */

TEST_HARNESS();

/* ===========================================================================
 * INDEPENDENT layout model (re-derives the proportional widths WITHOUT calling
 * the menu.c internals -- so a layout bug cannot mask itself; test_window idiom).
 * Mirrors menu.h's constants but recomputes the cumulative x from text_measure.
 * ===========================================================================*/
static int indep_title_w(const char *title)
{
    return text_measure(FONT_CHICAGO, title) + 2 * FLAIR_MENU_TITLE_PAD;
}
static int indep_title_x(const MenuBar *bar, int i)
{
    int x = bar->has_apple ? (int)FLAIR_MENU_APPLE_W : 0;
    for (int k = 0; k < i; k++)
        x += indep_title_w(bar->menus[k].title);
    return x;
}

/* ===========================================================================
 * Build the canon InitechPaint menu bar from menu_canon.h (do NOT re-author the
 * string; consume the frozen array). Each canon title becomes a MenuInfo with a
 * small deterministic item list so the pull-down has something to drop.
 * ===========================================================================*/
enum { CANON_N = FLAIR_CANON_PHOTOSHOP_MENU_COUNT };  /* 8 */

/* A fixed item set per canon menu: 4 normal items + a divider + a disabled item,
 * deterministic so the tracking test is reproducible (Rule 11). */
static MenuItem g_items[CANON_N][6];
static MenuInfo g_menus[CANON_N];
static MenuBar  g_bar;

static void build_canon_bar(void)
{
    for (int mi = 0; mi < CANON_N; mi++) {
        /* item 1..4: enabled normal items */
        g_items[mi][0] = (MenuItem){ "New",   0, 'N', 0, 1, 0 };
        g_items[mi][1] = (MenuItem){ "Open",  0, 'O', 0, 1, 0 };
        g_items[mi][2] = (MenuItem){ "Close", 0, 'W', 0, 1, 0 };
        g_items[mi][3] = (MenuItem){ "Save",  0, 'S', 0, 1, 0 };
        /* item 5: a divider (never selectable) */
        g_items[mi][4] = (MenuItem){ "-",     0,  0,  0, 0, 1 };
        /* item 6: a DISABLED item (never selectable) */
        g_items[mi][5] = (MenuItem){ "Revert",0,  0,  0, 0, 0 };

        g_menus[mi].menuID    = (int16_t)(128 + mi);  /* IM app menus start ~128 */
        g_menus[mi].title     = FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[mi];
        g_menus[mi].items     = g_items[mi];
        g_menus[mi].n_items   = 6;
        g_menus[mi].menuWidth = 0;
    }
    g_bar.menus     = g_menus;
    g_bar.n_menus   = CANON_N;
    g_bar.has_apple = 1;          /* the canon bar carries the Apple-menu slot */
}

/* ===========================================================================
 * Seeded LCG (Rule 11), mirroring test_window/test_region.
 * ===========================================================================*/
static uint32_t g_seed = 0x4D454E55u;     /* "MENU" */
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}
static int rnd(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(lcg() % (uint32_t)(hi - lo + 1));
}

/* ===========================================================================
 * The draw program: bar + the open 'Edit' panel hiliting item index 2 (Close).
 * (render_run takes a no-arg draw fn; the bar/menu come from file statics.)
 * ===========================================================================*/
#define OPEN_MI       1   /* 'Edit' */
#define HILITE_IT     2   /* item index 2 (0-based) -> "Close" */

/* Independent sampled-domain menu goldens. These values are transcribed from
 * ../system7-decomp/specs/sys8/menus.md Sec 1.1/1.2/2.1/2.3, never derived from
 * flair_look or menu.c. Teal is the DEC-10 OQ-2 authored substitution. */
enum {
    MG_BLACK = 0,
    MG_WHITE = 1,
    MG_TEAL = 2,
    MG_TEXT_BLACK = 4,
    MG_DARK_3F = 63,
    MG_GRAY_77 = 119,
    MG_GRAY_A5 = 165,
    MG_GRAY_B3 = 179,
    MG_GRAY_C0 = 192,
    MG_FACE_E7 = 231
};
#define MG_TEAL_LIGHT_RGB   0x8DDCDCu
#define MG_TEAL_SHADOW_RGB  0x4E9BA3u
static void draw_bar_and_panel(GrafPort *port)
{
    DrawMenuBar(port, &g_bar, OPEN_MI, NULL);
    flair_draw_menu_panel(port, &g_bar, OPEN_MI, HILITE_IT, NULL);
}

int main(int argc, char **argv)
{
    build_canon_bar();

    /* ======================================================================
     * PROPERTY 2: CANON -- the bar string matches menu_canon.h EXACTLY (Law 4).
     * (Run first: the rest of the suite stands on the canon titles.)
     * ====================================================================== */
    {
        CHECK(g_bar.n_menus == 8, "canon bar has exactly 8 menus (menu_canon.h)");

        /* Flat string assembled from the bar == the frozen flat literal. */
        char flat[128]; flat[0] = 0;
        size_t pos = 0;
        for (int k = 0; k < (int)g_bar.n_menus; k++) {
            const char *t = g_bar.menus[k].title;
            size_t tl = strlen(t);
            if (k) flat[pos++] = ' ';
            memcpy(flat + pos, t, tl); pos += tl;
        }
        flat[pos] = 0;
        CHECK_STR_EQ(flat, FLAIR_CANON_PHOTOSHOP_MENUBAR_EXPECTED,
                     "canon bar flat string == FROZEN menu_canon.h string (Law 4)");
        CHECK((int)strlen(flat) == FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN,
              "canon bar flat string length == FLAIR_CANON_PHOTOSHOP_MENUBAR_LEN");

        /* Item-by-item against the frozen array. */
        int item_bad = 0;
        for (int k = 0; k < (int)g_bar.n_menus; k++)
            if (strcmp(g_bar.menus[k].title,
                       FLAIR_CANON_PHOTOSHOP_MENU_ITEMS[k]) != 0)
                item_bad = 1;
        CHECK(!item_bad, "each canon title matches menu_canon.h item-by-item");

        /* The two chimera tells (Layer at 3, View at 5) -- canon, not a bug. */
        CHECK(strcmp(g_bar.menus[3].title, "Layer") == 0,
              "canon item 3 is 'Layer' (the chimera signal -- NOT 'Mode')");
        CHECK(strcmp(g_bar.menus[5].title, "View") == 0,
              "canon item 5 is 'View' (the chimera signal -- NOT 'Filter')");
    }

    /* ======================================================================
     * PROPERTY 1: BAR LAYOUT -- x-positions == cumulative proportional widths.
     * Verified against an INDEPENDENT model + a randomized hit-test round-trip.
     * ====================================================================== */
    {
        /* The bar height is the locked 20px (chrome_metrics). */
        CHECK((int)FLAIR_MENUBAR_H == FLAIR_CHROME_MENUBAR_H &&
              FLAIR_MENUBAR_H == 20,
              "menu bar height is FLAIR_CHROME_MENUBAR_H == 20 (locked spec)");

        /* Each title slot is at the independently-computed cumulative x, and its
         * width is the proportional PAD+text+PAD. */
        int x_bad = 0, w_bad = 0;
        for (int k = 0; k < (int)g_bar.n_menus; k++) {
            if (MenuBar_title_x(&g_bar, k) != indep_title_x(&g_bar, k)) x_bad = 1;
            if (MenuBar_title_w(&g_bar, k) != indep_title_w(g_bar.menus[k].title))
                w_bad = 1;
        }
        CHECK(!x_bad, "MenuBar_title_x == cumulative proportional widths (D-7)");
        CHECK(!w_bad, "MenuBar_title_w == PAD + text_measure + PAD (proportional)");

        /* The first title starts AFTER the Apple slot. */
        CHECK(MenuBar_title_x(&g_bar, 0) == (int)FLAIR_MENU_APPLE_W,
              "first title starts after the Apple-menu glyph slot");

        /* Proportional, NOT fixed-pitch: 'Image' (5 wide chars) is a different
         * width than 'File' (4 chars) -- distinct slot widths prove it. (Under
         * the FIXED_WIDTH mutant these collapse to equal and the next checks
         * disagree with the independent model.) */
        CHECK(MenuBar_title_w(&g_bar, 0) != MenuBar_title_w(&g_bar, 2) ||
              text_measure(FONT_CHICAGO, "File") ==
              text_measure(FONT_CHICAGO, "Image"),
              "title widths are proportional to text (File vs Image differ)");

        /* Hit-test round trip: a point in the middle of each title slot maps back
         * to that menu index; a point in the Apple slot is -1. */
        int hit_bad = 0;
        for (int k = 0; k < (int)g_bar.n_menus; k++) {
            int mx = indep_title_x(&g_bar, k) +
                     indep_title_w(g_bar.menus[k].title) / 2;
            if (MenuBar_hit(&g_bar, mx) != k) hit_bad = 1;
        }
        CHECK(!hit_bad, "MenuBar_hit maps a mid-slot x back to its menu index");
        CHECK(MenuBar_hit(&g_bar, (int)FLAIR_MENU_APPLE_W / 2) == -1,
              "a point in the Apple slot is NOT a title hit");
        CHECK(MenuBar_hit(&g_bar, 100000) == -1,
              "a point past the last title is NOT a title hit");
    }

    /* ======================================================================
     * PROPERTY 3: MENUSELECT TRACKING -- deterministic selection + selectability.
     * ====================================================================== */
    {
        /* Click 'Edit' (menu index 1), move down to item index 2 ("Close"),
         * release there. The bar y is within [0,20); item rows start at panel+2. */
        int mi = 1;                         /* 'Edit' */
        int tx = indep_title_x(&g_bar, mi) +
                 indep_title_w(g_bar.menus[mi].title) / 2;
        flair_point_t click = { (int16_t)(FLAIR_MENUBAR_H / 2), (int16_t)tx };

        /* item index 2's row center (screen coords). The panel left aligns with
         * the title slot left; pick an x well inside the panel body. */
        rgn_rect_t panel = MenuInfo_panel_rect(&g_bar, mi);
        int px = (panel.left + panel.right) / 2;
        int row2_top = panel.top + FLAIR_MENU_PANEL_INSET +
                       2 * FLAIR_MENU_ITEM_H;    /* items 0,1 above; item 2 next */
        int row2_y = row2_top + FLAIR_MENU_ITEM_H / 2;

        flair_point_t seq[3] = {
            { (int16_t)(FLAIR_MENUBAR_H + 1), (int16_t)px },  /* enter panel    */
            { (int16_t)(row2_top + 1),        (int16_t)px },  /* hover item 2   */
            { (int16_t)row2_y,                (int16_t)px }    /* release item 2 */
        };
        int hi = -99;
        uint32_t r = flair_menu_track(&g_bar, click, seq, 3, &hi);

        uint32_t want = MenuResult(g_bar.menus[mi].menuID, 3); /* item 2 -> 1based 3 */
        CHECK(r == want,
              "MenuSelect(click Edit, drag to item 3, release) == (menuID<<16|3)");
        CHECK(MenuResultID(r) == g_bar.menus[mi].menuID,
              "result high word is the menuID");
        CHECK(MenuResultItem(r) == 3, "result low word is the 1-based item (3)");
        CHECK(hi == HILITE_IT, "final hilited item is the released item index");

        /* DETERMINISM (Rule 11): the same inputs yield the same result. */
        uint32_t r2 = flair_menu_track(&g_bar, click, seq, 3, NULL);
        CHECK(r == r2, "MenuSelect is deterministic across runs (Rule 11)");

        /* RELEASE OUTSIDE any item -> 0. Release far to the left of the panel. */
        flair_point_t outside[1] = { { (int16_t)row2_y, (int16_t)(panel.left - 50) } };
        CHECK(flair_menu_track(&g_bar, click, outside, 1, NULL) == 0,
              "release outside any item row -> 0 (nothing chosen)");

        /* CLICK NOT ON A TITLE (in the Apple slot) -> 0, no menu drops. */
        flair_point_t apple_click = { (int16_t)(FLAIR_MENUBAR_H / 2),
                                      (int16_t)(FLAIR_MENU_APPLE_W / 2) };
        CHECK(flair_menu_track(&g_bar, apple_click, seq, 3, NULL) == 0,
              "click in the Apple slot drops no menu -> 0");

        /* DISABLED item / DIVIDER not selectable. Item index 4 is a divider,
         * item index 5 is disabled. Releasing on either -> 0. */
        int row4_top = panel.top + FLAIR_MENU_PANEL_INSET +
                       4 * FLAIR_MENU_ITEM_H;            /* after items 0..3     */
        flair_point_t on_div[1] = { { (int16_t)(row4_top + FLAIR_MENU_DIV_H / 2),
                                      (int16_t)px } };
        CHECK(flair_menu_track(&g_bar, click, on_div, 1, NULL) == 0,
              "release on a DIVIDER row -> 0 (never selectable)");

        int row5_top = row4_top + FLAIR_MENU_DIV_H;      /* divider is short     */
        flair_point_t on_dis[1] = { { (int16_t)(row5_top + FLAIR_MENU_ITEM_H / 2),
                                      (int16_t)px } };
        uint32_t rd = flair_menu_track(&g_bar, click, on_dis, 1, NULL);
        CHECK(rd == 0, "release on a DISABLED item -> 0 (never selectable)");

        /* The selectability predicate is decisive (the mutant subverts THIS). */
        CHECK(MenuInfo_item_selectable(&g_bar, mi, 0) == 1,
              "an enabled normal item is selectable");
        CHECK(MenuInfo_item_selectable(&g_bar, mi, 4) == 0,
              "a divider is NOT selectable");
        CHECK(MenuInfo_item_selectable(&g_bar, mi, 5) == 0,
              "a disabled item is NOT selectable");
    }

    /* ======================================================================
     * PROPERTY 3c: CROSS-MENU DRAG (initech-rl4v).  Inside Macintosh Vol I
     * "Menu Manager" MenuSelect: while the mouse is down, whichever title is
     * CURRENTLY under the cursor is the one dropped -- dragging out of the
     * clicked title into a different one closes the old pull-down and opens
     * the new one; releasing inside a panel selects THAT menu's item (docs/
     * research/gui-ground-truth.md Sec 3.2, "Inside Macintosh: Macintosh
     * Toolbox Essentials" Menu Manager citation). Before this fix, menu.c:297
     * captured `mi = MenuBar_hit(bar, startPt.h)` ONCE and the tracking loop /
     * release check (menu.c:305-307, :318-321) always re-used that frozen mi
     * -- MenuBar_hit was never called again, so a cross-menu drag was
     * structurally impossible (initech-rl4v bead evidence, verbatim repro
     * below).
     * ====================================================================== */
    {
        int mi0 = 0;   /* 'File' -- the menu FIRST clicked                    */
        int mi1 = 1;   /* 'Edit' -- the menu dragged INTO                     */

        int t0x = indep_title_x(&g_bar, mi0) +
                  indep_title_w(g_bar.menus[mi0].title) / 2;
        int t1x = indep_title_x(&g_bar, mi1) +
                  indep_title_w(g_bar.menus[mi1].title) / 2;

        flair_point_t click0 = { (int16_t)(FLAIR_MENUBAR_H / 2), (int16_t)t0x };

        rgn_rect_t panel0 = MenuInfo_panel_rect(&g_bar, mi0);
        rgn_rect_t panel1 = MenuInfo_panel_rect(&g_bar, mi1);
        /* The canon bar's per-menu item sets are IDENTICAL (build_canon_bar),
         * so both panels share one width; since panel1 drops directly under
         * title1 (strictly right of title0), panel1.left > panel0.left --
         * there is a distinguishing x that lands in panel0 but not panel1.
         * Assert the fixture assumption holds so this oracle cannot silently
         * degrade into a no-op. */
        CHECK(panel1.left > panel0.left,
              "fixture: panel1 starts strictly right of panel0 (distinguishing x exists)");

        int row2_top = panel1.top + FLAIR_MENU_PANEL_INSET +
                       2 * FLAIR_MENU_ITEM_H;             /* item index 2 row  */
        int row2_y    = row2_top + FLAIR_MENU_ITEM_H / 2;
        int panel1_px = (panel1.left + panel1.right) / 2;

        /* (i) Drag from title0 into title1 (still in the bar band, no panel
         * entered yet) -- this must SWITCH the tracked menu to mi1 and close
         * mi0's panel. Probe with a point at panel0's own x (but an item-row
         * y): if the switch happened, that x is OUTSIDE panel1 -> -1. If mi
         * stayed frozen at mi0 (the bug), that point is squarely inside
         * panel0's own item row 2 and resolves to a valid (wrong) item -- so
         * this probe DISTINGUISHES fixed from buggy behavior. */
        {
            flair_point_t probe[2] = {
                { (int16_t)(FLAIR_MENUBAR_H / 2), (int16_t)t1x },  /* enter menu1 title */
                { (int16_t)row2_y,                (int16_t)panel0.left } /* menu0's old x */
            };
            int hi = -99;
            uint32_t r = flair_menu_track(&g_bar, click0, probe, 2, &hi);
            CHECK(hi == -1,
                  "dragging into menu1's title CLOSES menu0's panel (old panel-space is dead, initech-rl4v)");
            CHECK(r == 0,
                  "no selection while merely hovering the switched-to title (initech-rl4v)");
        }

        /* (ii) Continue the drag: title0 -> title1 -> menu1's own panel ->
         * release on item index 2. The result MUST be menu1's item, not 0 and
         * not menu0's (the bead's verbatim repro: "flair_menu_track startPt on
         * menu 0 ('File'), pts ending inside menu 1 ('Edit') panel -> result
         * 0, out_hi -1" -- this asserts the FIXED contract instead). */
        {
            flair_point_t seq[3] = {
                { (int16_t)(FLAIR_MENUBAR_H / 2), (int16_t)t1x },      /* enter menu1 title */
                { (int16_t)(row2_top + 1),        (int16_t)panel1_px },/* hover item 2      */
                { (int16_t)row2_y,                (int16_t)panel1_px } /* release item 2    */
            };
            int hi = -99;
            uint32_t r = flair_menu_track(&g_bar, click0, seq, 3, &hi);
            uint32_t want = MenuResult(g_bar.menus[mi1].menuID, 3); /* item idx2 -> 1based 3 */
            CHECK(r == want,
                  "cross-menu drag: click File, drag into Edit's panel, release item 3 -> Edit's result (initech-rl4v)");
            CHECK(MenuResultID(r) == g_bar.menus[mi1].menuID,
                  "cross-menu drag: result menuID is the FINAL (dragged-to) menu, not the clicked one");
            CHECK(hi == 2,
                  "cross-menu drag: out_hi is the item index in the FINAL menu's panel");
        }
    }

    /* ======================================================================
     * PROPERTY 3b: MenuKey -- command-key equivalents pack the right result.
     * ====================================================================== */
    {
        /* 'N' (New) is item 0 of EVERY menu; MenuKey returns the FIRST match
         * (menu 0, item 1). Case-insensitive. */
        uint32_t rn = MenuKey(&g_bar, 'N');
        CHECK(rn == MenuResult(g_bar.menus[0].menuID, 1),
              "MenuKey('N') -> (menu0 << 16 | 1) (first enabled match)");
        uint32_t rl = MenuKey(&g_bar, 'n');   /* lowercase folds to 'N' */
        CHECK(rl == rn, "MenuKey is case-insensitive for letters (IM Command-key)");

        uint32_t ro = MenuKey(&g_bar, 'O');   /* Open is item 1 -> 1based 2 */
        CHECK(ro == MenuResult(g_bar.menus[0].menuID, 2),
              "MenuKey('O') -> (menu0 << 16 | 2)");

        CHECK(MenuKey(&g_bar, 'Z') == 0,
              "MenuKey for an unbound key -> 0");
        CHECK(MenuKey(&g_bar, 0) == 0, "MenuKey(0) -> 0 (no command key)");
    }

    /* ======================================================================
     * PROPERTY 4: DRAW -- render the bar + open panel; assert the painted layout.
     * Uses the host render skeleton (AM-1: geometry is a runtime parameter).
     * ====================================================================== */
    {
        render_boot_info_t boot;
        memset(&boot, 0, sizeof boot);
        boot.lfb_addr = 0xE0000000u;
        boot.lfb_pitch = 0u;             /* tight */
        boot.lfb_bpp = 8u;               /* indexed-8 (OD-2) */
        boot.lfb_width = 640u;
        boot.lfb_height = 480u;

        render_ctx_t ctx;
        int rc = render_ctx_init(&ctx, &boot);
        CHECK(rc == 0, "render_ctx_init(8bpp) must succeed (AM-1 geometry param)");
        if (rc == 0) {
            render_run(&ctx, draw_bar_and_panel);

            /* (a) Exact bar profile at a title-free column: white row 0, E7
             * rows 1..17, B3 row 18, black row 19. sys8/menus.md Sec 1.1. */
            CHECK(render_pixel_index(&ctx, 500u, 0u) == MG_WHITE,
                  "MENU FIDELITY bar row 0 is sampled white");
            CHECK(render_pixel_index(&ctx, 500u, 1u) == MG_FACE_E7 &&
                  render_pixel_index(&ctx, 500u, 9u) == MG_FACE_E7 &&
                  render_pixel_index(&ctx, 500u, 17u) == MG_FACE_E7,
                  "MENU FIDELITY bar rows 1..17 are sampled E7 face");
            CHECK(render_pixel_index(&ctx, 500u, 18u) == MG_GRAY_B3,
                  "MENU FIDELITY bar row 18 is sampled B3 shadow");
            CHECK(render_pixel_index(&ctx, 500u, 19u) == MG_BLACK,
                  "MENU FIDELITY bar row 19 is black baseline");

            /* Exact left-corner transcription plus one mirrored probe.
             * sys8/menus.md Sec 1.2, grid x=0..14 y=0..8. */
            CHECK(render_pixel_index(&ctx, 0u, 0u) == MG_BLACK &&
                  render_pixel_index(&ctx, 5u, 0u) == MG_GRAY_77 &&
                  render_pixel_index(&ctx, 6u, 0u) == MG_GRAY_C0 &&
                  render_pixel_index(&ctx, 7u, 0u) == MG_FACE_E7 &&
                  render_pixel_index(&ctx, 8u, 0u) == MG_WHITE,
                  "MENU FIDELITY rounded top-left row 0 is KKKKKsceW");
            CHECK(render_pixel_index(&ctx, 0u, 4u) == MG_BLACK &&
                  render_pixel_index(&ctx, 1u, 4u) == MG_GRAY_C0 &&
                  render_pixel_index(&ctx, 2u, 4u) == MG_WHITE &&
                  render_pixel_index(&ctx, 3u, 4u) == MG_FACE_E7,
                  "MENU FIDELITY rounded top-left row 4 is KcWe");
            CHECK(render_pixel_index(&ctx, 639u, 0u) == MG_BLACK &&
                  render_pixel_index(&ctx, 634u, 0u) == MG_GRAY_77,
                  "MENU FIDELITY top-right corner mirrors the sampled left");

            /* (b) Idle title ink lands in the File slot. FLAIR_PART_TEXT is the
             * distinct black-valued canon slot 4 at indexed depth. */
            int s0x = indep_title_x(&g_bar, 0) + FLAIR_MENU_TITLE_PAD;
            int s0w = text_measure(FONT_CHICAGO, "File");
            int ink_in_slot = 0;
            for (int x = s0x; x < s0x + s0w; x++)
                for (int y = FLAIR_MENU_TITLE_VPAD;
                     y < FLAIR_MENU_TITLE_VPAD + 16; y++)
                    if (render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y) ==
                        MG_TEXT_BLACK)
                        ink_in_slot = 1;
            CHECK(ink_in_slot, "title text ink lands inside the 'File' title slot");

            /* The pulled Edit block is 10px either side of the idle ink run,
             * accent-filled through row 18, white text, baseline unchanged. */
            {
                int tx = indep_title_x(&g_bar, OPEN_MI);
                int tw = text_measure(FONT_CHICAGO, g_bar.menus[OPEN_MI].title);
                int want_l = tx + FLAIR_MENU_TITLE_PAD - 10;
                int want_r = tx + FLAIR_MENU_TITLE_PAD + tw + 10;
                rgn_rect_t hr = MenuBar_hilite_rect(&g_bar, OPEN_MI);
                CHECK(hr.left == want_l && hr.right == want_r &&
                      hr.top == 0 && hr.bottom == 19,
                      "MENU FIDELITY pulled title block has 10px ink-side pads");
                CHECK(render_pixel_index(&ctx, (uint32_t)(hr.left + 1), 0u) ==
                      MG_TEAL &&
                      render_pixel_index(&ctx, (uint32_t)(hr.left + 1), 8u) ==
                      MG_TEAL &&
                      render_pixel_index(&ctx, (uint32_t)(hr.left + 1), 18u) ==
                      MG_TEAL,
                      "MENU FIDELITY indexed pulled-title rows use existing teal slot");
                CHECK(render_pixel_index(&ctx, (uint32_t)(tx +
                      FLAIR_MENU_TITLE_PAD + 1), 4u) == MG_WHITE,
                      "MENU FIDELITY pulled title text is white");
                CHECK(render_pixel_index(&ctx, (uint32_t)(hr.left + 1), 19u) ==
                      MG_BLACK,
                      "MENU FIDELITY pulled title leaves black baseline unchanged");
            }

            /* (c) Panel proper + temporary-ink footprint. */
            rgn_rect_t panel = MenuInfo_panel_rect(&g_bar, OPEN_MI);
            rgn_rect_t footprint = MenuInfo_panel_footprint_rect(&g_bar, OPEN_MI);
            CHECK(panel.right > panel.left && panel.bottom > panel.top,
                  "open menu panel rect is non-empty");
            CHECK(panel.top == FLAIR_MENUBAR_H - 1 &&
                  footprint.left == panel.left && footprint.top == panel.top &&
                  footprint.right == panel.right + 1 &&
                  footprint.bottom == panel.bottom + 1,
                  "MENU FIDELITY panel shares baseline and footprint adds +1 shadow");

            /* The panel is the ONLY drawing below the menu bar in this host
             * scene. Scan every pixel there and independently recover the
             * non-desktop bounding box; it must equal the helper rect on all
             * four edges. This is stronger than corner probes: if the drawer
             * grows or shrinks on any side while MenuInfo_panel_rect stays
             * fixed (or vice versa), the restore footprint would drift and
             * this tooth goes RED (beads initech-b3hl/-j0vt). */
            {
                int found = 0;
                int min_x = (int)boot.lfb_width;
                int min_y = (int)boot.lfb_height;
                int max_x = -1;
                int max_y = -1;
                for (int y = FLAIR_MENUBAR_H; y < (int)boot.lfb_height; y++) {
                    for (int x = 0; x < (int)boot.lfb_width; x++) {
                        if (render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y) ==
                            (uint32_t)RENDER_DESKTOP_INDEX)
                            continue;
                        found = 1;
                        if (x < min_x) min_x = x;
                        if (x > max_x) max_x = x;
                        if (y < min_y) min_y = y;
                        if (y > max_y) max_y = y;
                    }
                }
                CHECK(found,
                      "drawn panel has a non-empty painted extent below the bar");
                CHECK(found && min_x == footprint.left &&
                      min_y == FLAIR_MENUBAR_H &&
                      max_x + 1 == footprint.right &&
                      max_y + 1 == footprint.bottom,
                      "drawn extent below bar == panel shadow footprint "
                      "(restore geometry cannot drift)");
            }
            CHECK(render_pixel_index(&ctx, (uint32_t)panel.left,
                                     (uint32_t)panel.top) == MG_BLACK,
                  "panel top-left frame corner is painted ink");
            /* pixel just LEFT of the panel (below the bar) is the bare desktop. */
            if (panel.left - 1 >= 0) {
                CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left - 1),
                                         (uint32_t)(panel.top + 4))
                      == (uint32_t)RENDER_DESKTOP_INDEX,
                      "pixel just left of the panel is the bare desktop (no bleed)");
            }

            /* Exact frame / inner bevel / distinct drop shadow. */
            CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left + 3),
                      (uint32_t)(panel.top + 1)) == MG_WHITE &&
                  render_pixel_index(&ctx, (uint32_t)(panel.left + 1),
                      (uint32_t)(panel.top + 4)) == MG_WHITE,
                  "MENU FIDELITY panel inner top/left highlight is white");
            CHECK(render_pixel_index(&ctx, (uint32_t)(panel.right - 2),
                      (uint32_t)(panel.top + 4)) == MG_GRAY_B3 &&
                  render_pixel_index(&ctx, (uint32_t)(panel.left + 3),
                      (uint32_t)(panel.bottom - 2)) == MG_GRAY_B3,
                  "MENU FIDELITY panel inner bottom/right shadow is B3");
            CHECK(render_pixel_index(&ctx, (uint32_t)panel.right,
                      (uint32_t)(panel.top + 4)) == MG_DARK_3F &&
                  render_pixel_index(&ctx, (uint32_t)(panel.left + 3),
                      (uint32_t)panel.bottom) == MG_DARK_3F,
                  "MENU FIDELITY panel drop shadow is distinct sampled 3F");

            /* (d) Classic item inversion stays bounded to the tracked row. */
            int row2_top = panel.top + FLAIR_MENU_PANEL_INSET +
                           HILITE_IT * FLAIR_MENU_ITEM_H;
            int band_y = row2_top + FLAIR_MENU_ITEM_H / 2;
            int interior_x = panel.left + FLAIR_MENU_PANEL_INSET + 1;
            CHECK(render_pixel_index(&ctx, (uint32_t)interior_x,
                                     (uint32_t)band_y) == MG_BLACK,
                  "hilited item band is painted (inverted fg) at the item row");

            /* A NON-hilited item row interior is the E7 panel face, NOT the
             * inverted band -- proves the hilite is bounded to one row. */
            int row0_top = panel.top + FLAIR_MENU_PANEL_INSET;
            int row0_y = row0_top +
                         FLAIR_MENU_ITEM_H / 2;
            int body_x = panel.right - FLAIR_MENU_PANEL_INSET - 2;
            CHECK(render_pixel_index(&ctx, (uint32_t)body_x,
                                     (uint32_t)row0_y) == MG_FACE_E7,
                  "a non-hilited item row is E7 panel face, not inverted");

            /* Text begins at panel+20. The N strike's first ink is cell x+1 on
             * glyph row 2, providing an exact rendered left-edge tooth. */
            CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left + 20),
                      (uint32_t)(row0_top + 4)) == MG_FACE_E7 &&
                  render_pixel_index(&ctx, (uint32_t)(panel.left + 21),
                      (uint32_t)(row0_top + 4)) == MG_TEXT_BLACK,
                  "MENU FIDELITY item text cell begins at panel left +20");

            /* Command-key substitution: '^N', right-aligned 16px from the panel
             * right. Caret row 2 has ink at cell columns 3/4. */
            {
                int cmd_x = panel.right - 16 - 2 * CHICAGO_CELL_W;
                CHECK(render_pixel_index(&ctx, (uint32_t)(cmd_x + 3),
                          (uint32_t)(row0_top + 4)) == MG_TEXT_BLACK,
                      "MENU FIDELITY command column renders caret-letter at right");
            }

            /* Separator is 6px high with full-span A5/white rows +1/+2. */
            {
                int sep_top = panel.top + FLAIR_MENU_PANEL_INSET +
                              4 * FLAIR_MENU_ITEM_H;
                CHECK(FLAIR_MENU_DIV_H == 6,
                      "MENU FIDELITY separator metric is locked 6px");
                CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left + 2),
                          (uint32_t)(sep_top + 1)) == MG_GRAY_A5 &&
                      render_pixel_index(&ctx, (uint32_t)(panel.right - 3),
                          (uint32_t)(sep_top + 1)) == MG_GRAY_A5 &&
                      render_pixel_index(&ctx, (uint32_t)(panel.left + 2),
                          (uint32_t)(sep_top + 2)) == MG_WHITE &&
                      render_pixel_index(&ctx, (uint32_t)(panel.right - 3),
                          (uint32_t)(sep_top + 2)) == MG_WHITE,
                      "MENU FIDELITY etched separator is A5 then white full-span");

                /* Disabled Revert: R row 2, first ink at text cell x+1. */
                int dis_top = sep_top + FLAIR_MENU_DIV_H;
                CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left + 21),
                          (uint32_t)(dis_top + 4)) == MG_GRAY_A5,
                      "MENU FIDELITY disabled item ink is sampled A5");
            }

            /* ==================================================================
             * PROPERTY 5: APPLE GLYPH -- the Apple slot renders the hand-
             * authored apple-with-bite glyph (spec/assets/apple_glyph.h), NOT a
             * solid filled square (initech-yx4v). Properties are derived from
             * the AUTHORED STRIKE (apple_glyph.h) but graded against the
             * RENDERED pixels, so a wrong render (the old solid-square bug)
             * goes RED even though this oracle never looks at a screenshot.
             * ================================================================== */
            {
                int ax0 = (int)FLAIR_MENU_TITLE_VPAD;  /* slot left (apple.left) */
                int ay0 = (int)FLAIR_MENU_TITLE_VPAD;  /* slot top  (apple.top)  */

                /* (i) INK COVERAGE: rendered ink count in the slot == the
                 * strike's OWN ink count exactly (the render must reproduce
                 * apple_glyph.h bit for bit), AND within the 25%-60% band (a
                 * glyph, not a solid square: a square renders 240/240 = 100%,
                 * failing both checks). */
                int strike_ink = 0;
                for (int gy = 0; gy < APPLE_GLYPH_H; gy++)
                    for (int gx = 0; gx < APPLE_GLYPH_W; gx++)
                        if (APPLE_GLYPH_ROWS[gy] & (uint16_t)(0x8000u >> gx))
                            strike_ink++;

                int rendered_ink = 0;
                for (int gy = 0; gy < APPLE_GLYPH_H; gy++)
                    for (int gx = 0; gx < APPLE_GLYPH_W; gx++)
                        if (render_pixel_index(&ctx, (uint32_t)(ax0 + gx),
                                               (uint32_t)(ay0 + gy)) == 0u)
                            rendered_ink++;

                CHECK(rendered_ink == strike_ink,
                      "Apple slot rendered ink count == the authored strike's "
                      "ink count exactly (the render reproduces apple_glyph.h)");

                {
                    double ratio = (double)rendered_ink /
                                   (double)(APPLE_GLYPH_W * APPLE_GLYPH_H);
                    CHECK(ratio >= 0.25 && ratio <= 0.60,
                          "Apple slot ink coverage is 25%-60% of the slot (a "
                          "glyph, not a solid square -- initech-yx4v)");
                }

                /* (ii) BITE NOTCH: the named probe column is ink a few rows
                 * above the bite (the pre-bite body, row 4) but recedes to
                 * background AT the bite row (row 6) -- a solid square paints
                 * ink at BOTH, so this differential is what a "some ink, some
                 * bg" check alone would have missed. */
                CHECK(render_pixel_index(&ctx,
                          (uint32_t)(ax0 + APPLE_GLYPH_BITE_COL),
                          (uint32_t)(ay0 + APPLE_GLYPH_PREBITE_ROW)) == 0u,
                      "pre-bite body column is ink (apple_glyph.h row 4)");
                CHECK(render_pixel_index(&ctx,
                          (uint32_t)(ax0 + APPLE_GLYPH_BITE_COL),
                          (uint32_t)(ay0 + APPLE_GLYPH_BITE_ROW)) != MG_BLACK,
                      "the SAME column recedes to background at the bite notch "
                      "row -- the apple has a bite, not a solid square");

                /* (iii) LEAF ABOVE BODY: the leaf tip (row 0) is ink, but the
                 * slot's top-left corner is background -- a solid square would
                 * paint the corner ink too. */
                CHECK(render_pixel_index(&ctx,
                          (uint32_t)(ax0 + APPLE_GLYPH_LEAF_COL),
                          (uint32_t)(ay0 + APPLE_GLYPH_LEAF_ROW)) == 0u,
                      "leaf-tip pixel (row 0) is ink -- the leaf sits above the "
                      "body");
                CHECK(render_pixel_index(&ctx, (uint32_t)ax0, (uint32_t)ay0)
                      != MG_BLACK,
                      "the slot's top-left corner is background -- NOT a solid "
                      "filled square (initech-yx4v)");

                /* (iv) the widest body row (apple_glyph.h row 9) is denser than
                 * the leaf-tip row (row 0) -- the leaf tapers, the body doesn't. */
                {
                    int leaf_row_ink = 0, widest_row_ink = 0;
                    for (int gx = 0; gx < APPLE_GLYPH_W; gx++) {
                        if (render_pixel_index(&ctx, (uint32_t)(ax0 + gx),
                                (uint32_t)(ay0 + APPLE_GLYPH_LEAF_ROW)) == 0u)
                            leaf_row_ink++;
                        if (render_pixel_index(&ctx, (uint32_t)(ax0 + gx),
                                (uint32_t)(ay0 + APPLE_GLYPH_WIDEST_ROW)) == 0u)
                            widest_row_ink++;
                    }
                    CHECK(widest_row_ink > leaf_row_ink,
                          "the widest body row has more ink than the leaf-tip "
                          "row (leaf above body, not a uniform block)");
                }
            }

            /* optional PPM dump for human audit (Law 4). */
            if (argc > 1) {
                if (render_write_ppm(&ctx, argv[1]) == 0)
                    printf("    wrote rendered menu PPM to %s\n", argv[1]);
            }
            render_ctx_free(&ctx);
        }
    }

    /* Direct-color accent fidelity: indexed mode deliberately uses the retained
     * idx2 teal fallback for both authored teal rows; 32bpp proves the two canon
     * rows themselves reach pixels without a new palette row (DEC-10 OQ-2/OQ-3). */
    {
        render_boot_info_t boot;
        memset(&boot, 0, sizeof boot);
        boot.lfb_addr = 0xE0000000u;
        boot.lfb_bpp = 32u;
        boot.lfb_width = 640u;
        boot.lfb_height = 480u;
        render_ctx_t ctx;
        int rc = render_ctx_init(&ctx, &boot);
        CHECK(rc == 0, "render_ctx_init(32bpp) succeeds for menu accent fidelity");
        if (rc == 0) {
            render_run(&ctx, draw_bar_and_panel);
            rgn_rect_t hr = MenuBar_hilite_rect(&g_bar, OPEN_MI);
            CHECK((render_pixel_rgb(&ctx, (uint32_t)(hr.left + 1), 0u) &
                   0x00FFFFFFu) == MG_TEAL_LIGHT_RGB,
                  "MENU FIDELITY pulled-title top is authored teal light");
            CHECK((render_pixel_rgb(&ctx, (uint32_t)(hr.left + 1), 8u) &
                   0x00FFFFFFu) == MG_TEAL_SHADOW_RGB &&
                  (render_pixel_rgb(&ctx, (uint32_t)(hr.left + 1), 18u) &
                   0x00FFFFFFu) == MG_TEAL_SHADOW_RGB,
                  "MENU FIDELITY pulled-title face/shadow use authored teal shadow");
            render_ctx_free(&ctx);
        }
    }

    /* A randomized determinism sweep: random clicks + random release points over
     * the canon bar never crash and always return either 0 or a well-formed
     * result with a valid menuID and 1-based item in range (Rule 2 / Rule 11). */
    {
        int bad = 0;
        for (int t = 0; t < 4000 && !bad; t++) {
            int hv = rnd(0, 639), vv = rnd(0, FLAIR_MENUBAR_H - 1);
            flair_point_t click = { (int16_t)vv, (int16_t)hv };
            flair_point_t rel[1] = { { (int16_t)rnd(0, 200),
                                       (int16_t)rnd(0, 639) } };
            uint32_t r = flair_menu_track(&g_bar, click, rel, 1, NULL);
            if (r != 0) {
                int id = MenuResultID(r);
                int it1 = MenuResultItem(r);
                /* id must be one of the canon menuIDs; item 1-based in range. */
                int ok = 0;
                for (int k = 0; k < (int)g_bar.n_menus; k++)
                    if (g_bar.menus[k].menuID == id &&
                        it1 >= 1 && it1 <= (int)g_bar.menus[k].n_items) {
                        /* and the chosen item must actually be selectable. */
                        if (MenuInfo_item_selectable(&g_bar, k, it1 - 1)) ok = 1;
                    }
                if (!ok) bad = 1;
            }
        }
        CHECK(!bad, "random track: any non-zero result is a selectable in-range item");
    }

    return TEST_SUMMARY("test-menu");
}

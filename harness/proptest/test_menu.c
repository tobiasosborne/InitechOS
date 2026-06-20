/* test_menu.c -- the FLAIR Menu Manager property suite (THE ORACLE; Law 2).
 *
 * beads: initech-n3e ("FLAIR Menu Manager: pull-down menus + the menu bar,
 *        Photoshop-exact"). The mechanical oracle for the Menu Manager (ADR-0004
 *        D-3): the proportional bar layout, the canon InitechPaint menu bar
 *        (Law 4), the deterministic MenuSelect pull-down tracking, MenuKey, and
 *        the rendered bar/panel pixel layout.
 *
 * Ref:   ADR-0004 D-3 (MenuInfo + the Photoshop-exact bar + MenuSelect ->
 *          (menuID<<16|item)); D-7 (proportional text -- sum of advances); D-1/D-2
 *          (draw through a GrafPort clipped by a region). spec/assets/menu_canon.h
 *          (the FROZEN canon string -- asserted byte-exact). spec/chrome_metrics.h
 *          (FLAIR_CHROME_MENUBAR_H = 20). os/flair/menu.h (the unit under test).
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
 *  4. DRAW.  Render the bar + an open menu into a host offscreen via the render
 *     skeleton and assert: the bar background occupies rows [0,20); the title
 *     text pixels land inside the title slot; the dropped panel rect matches the
 *     layout; the hilited item band is painted where the layout says.
 *
 * MUTANTS (Rule 6), each driven RED by the Makefile gate:
 *   MENU_MUTATE_FIXED_WIDTH     -- titles/items laid out fixed-width, not
 *                                  proportional => property 1 (x-positions) RED.
 *   MENU_MUTATE_SELECT_DISABLED -- a disabled item/divider becomes selectable
 *                                  => property 3 (selectability) RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render.h"             /* host render skeleton (-Iharness/render)     */
#include "menu.h"               /* the Menu Manager under test (-Ios/flair)    */
#include "menu_canon.h"         /* FROZEN canon string (-Ispec/assets)         */
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
static void draw_bar_and_panel(GrafPort *port)
{
    uint32_t fg = 0u;                          /* black ink (idx 0)            */
    uint32_t bg = 0x00FFFFFFu;                 /* white (idx 1 via palette)    */
    /* Use indices directly for the 8bpp pass; the skeleton's palette maps 0->black
     * and 1->white. We pass the index in the low byte (surface 8bpp). */
    DrawMenuBar(port, &g_bar, 0u, 1u, NULL);
    flair_draw_menu_panel(port, &g_bar, OPEN_MI, HILITE_IT, 0u, 1u, NULL);
    (void)fg; (void)bg;
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
         * release there. The bar y is within [0,20); the item rows start at 20. */
        int mi = 1;                         /* 'Edit' */
        int tx = indep_title_x(&g_bar, mi) +
                 indep_title_w(g_bar.menus[mi].title) / 2;
        flair_point_t click = { (int16_t)(FLAIR_MENUBAR_H / 2), (int16_t)tx };

        /* item index 2's row center (screen coords). The panel left aligns with
         * the title slot left; pick an x well inside the panel body. */
        rgn_rect_t panel = MenuInfo_panel_rect(&g_bar, mi);
        int px = (panel.left + panel.right) / 2;
        int row2_top = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME +
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
        int row4_top = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME +
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

            /* (a) The bar background fills rows [0,20): the row just below the
             * bar baseline is NOT bar ink at a non-title x left of any panel. */
            uint32_t bar_bg = render_pixel_index(&ctx, 300u, 5u);
            CHECK(bar_bg == 1u, "bar background row (y=5) is the bar fill (idx 1)");

            /* The 1px baseline at y=19 is ink (idx 0). */
            CHECK(render_pixel_index(&ctx, 300u, (uint32_t)(FLAIR_MENUBAR_H - 1))
                  == 0u, "menu-bar baseline (y=19) is painted ink");

            /* (b) Title text ink lands inside the 'File' slot (menu 0). There is
             * SOME ink in the slot interior at a title row. */
            int s0x = indep_title_x(&g_bar, 0) + FLAIR_MENU_TITLE_PAD;
            int s0w = text_measure(FONT_CHICAGO, "File");
            int ink_in_slot = 0;
            for (int x = s0x; x < s0x + s0w; x++)
                for (int y = FLAIR_MENU_TITLE_VPAD;
                     y < FLAIR_MENU_TITLE_VPAD + 16; y++)
                    if (render_pixel_index(&ctx, (uint32_t)x, (uint32_t)y) == 0u)
                        ink_in_slot = 1;
            CHECK(ink_in_slot, "title text ink lands inside the 'File' title slot");

            /* (c) The dropped panel rect matches the layout: the panel frame is
             * painted at its computed left/top, and the desktop just OUTSIDE the
             * panel left edge is bare. */
            rgn_rect_t panel = MenuInfo_panel_rect(&g_bar, OPEN_MI);
            CHECK(panel.right > panel.left && panel.bottom > panel.top,
                  "open menu panel rect is non-empty");
            /* panel top-left frame corner is ink. */
            CHECK(render_pixel_index(&ctx, (uint32_t)panel.left,
                                     (uint32_t)panel.top) == 0u,
                  "panel top-left frame corner is painted ink");
            /* pixel just LEFT of the panel (below the bar) is the bare desktop. */
            if (panel.left - 1 >= 0) {
                CHECK(render_pixel_index(&ctx, (uint32_t)(panel.left - 1),
                                         (uint32_t)(panel.top + 4))
                      == (uint32_t)RENDER_DESKTOP_INDEX,
                      "pixel just left of the panel is the bare desktop (no bleed)");
            }

            /* (d) The hilited item band (item index 2) is painted ink across the
             * panel interior at that row (the inverted band fills fg). */
            int row2_top = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME +
                           HILITE_IT * FLAIR_MENU_ITEM_H;
            int band_y = row2_top + FLAIR_MENU_ITEM_H / 2;
            int interior_x = panel.left + FLAIR_MENU_PANEL_FRAME + 1;
            CHECK(render_pixel_index(&ctx, (uint32_t)interior_x,
                                     (uint32_t)band_y) == 0u,
                  "hilited item band is painted (inverted fg) at the item row");

            /* A NON-hilited item row interior is the panel body (idx 1), NOT the
             * inverted band -- proves the hilite is bounded to one row. */
            int row0_y = FLAIR_MENUBAR_H + FLAIR_MENU_PANEL_FRAME +
                         FLAIR_MENU_ITEM_H / 2;
            /* sample a body pixel in the right gutter of row 0 (clear of text). */
            int body_x = panel.right - FLAIR_MENU_PANEL_FRAME - 2;
            CHECK(render_pixel_index(&ctx, (uint32_t)body_x,
                                     (uint32_t)row0_y) == 1u,
                  "a non-hilited item row is the panel body (idx 1), not inverted");

            /* optional PPM dump for human audit (Law 4). */
            if (argc > 1) {
                if (render_write_ppm(&ctx, argv[1]) == 0)
                    printf("    wrote rendered menu PPM to %s\n", argv[1]);
            }
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

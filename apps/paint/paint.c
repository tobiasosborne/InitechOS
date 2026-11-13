/*
 * os/apps/paint/paint.c -- PAINT.EXE, InitechPaint (THE ARTIFACT; bead
 * initech-tdnl.20, the bar-carrier half). PRD Sec 6.5: "InitechPaint --
 * exists primarily to carry the Photoshop-exact menu bar so the frame is
 * self-consistent"; Appendix A: top menu bar "File Edit Image Layer Select
 * View Window Help".
 *
 * A disk-launched C tenant under the App Contract (ADR-0013; os/apps/tbx):
 *   entry      REGISTER, SETMBAR (the canon bar, NO Apple slot -- the frame's
 *              Photoshop bar has none; spec/assets/menu_canon.h), one blank
 *              white canvas window, return (resident).
 *   updateEvt  the canvas is white (a blank document).
 *   File>Quit  EXIT(0); the close box ends the tenant (goAway 1).
 * Every other item is period-GRAYED (enabled 0): the Menu Manager draws it in
 * the disabled ink and never delivers it -- InitechPaint carries the bar, it
 * does not paint (PRD Sec 6.5).
 *
 * Titles: the FROZEN canon (spec/assets/menu_canon.h FLAIR_CANON_MENU_ITEM_*,
 * locked by ADR-0004 D-3 / AM-4 -- never "corrected" to a real Photoshop).
 * Item WORDS and the canvas title/rect are AUTHORED product policy in the
 * period's paint-program vocabulary; the film does not show any pulled menu,
 * so no item is claimed recovered from the frame (Law 1 honesty).
 * PAINT_MUT_BAR_TEXT / PAINT_MUT_ITEMS_ENABLED are the gate's named mutants.
 * ASCII-clean (Rule 12). Freestanding, deterministic (Rule 11).
 */
#include "tbx.h"
#include "assets/menu_canon.h"

const char tbx_app_name[] = "PAINT";

#ifdef PAINT_MUT_ITEMS_ENABLED
#define G 1   /* NAMED MUTANT: every item live */
#else
#define G 0   /* period-grayed */
#endif
#define ITEM(t) { t, 0, 0, 0, G, 0, { 0, 0, 0 } }

static const tbx_menuitem_t file_items[] = {
    ITEM("New..."), ITEM("Open..."), ITEM("Close"), ITEM("Save"),
    { "Quit", 0, 'Q', 0, 1, 0, { 0, 0, 0 } }
};
static const tbx_menuitem_t edit_items[] = {
    ITEM("Undo"), ITEM("Cut"), ITEM("Copy"), ITEM("Paste"), ITEM("Clear")
};
static const tbx_menuitem_t image_items[] = {
    ITEM("Adjust"), ITEM("Image Size..."), ITEM("Canvas Size...")
};
static const tbx_menuitem_t layer_items[] = {
    ITEM("New Layer..."), ITEM("Delete Layer"), ITEM("Flatten Image")
};
static const tbx_menuitem_t select_items[] = {
    ITEM("All"), ITEM("None"), ITEM("Inverse")
};
static const tbx_menuitem_t view_items[] = {
    ITEM("Zoom In"), ITEM("Zoom Out"), ITEM("Show Rulers")
};
static const tbx_menuitem_t window_items[] = {
    ITEM("Show Brushes"), ITEM("Show Layers")
};
static const tbx_menuitem_t help_items[] = {
    ITEM("Paint Help")
};
#define N(a) ((uint16_t)(sizeof(a) / sizeof((a)[0])))

#ifdef PAINT_MUT_BAR_TEXT
#define T_LAYER "Layers"   /* NAMED MUTANT: a "corrected" title */
#else
#define T_LAYER FLAIR_CANON_MENU_ITEM_3
#endif
enum { PAINT_FILE = 128 };
static const tbx_menuinfo_t menus[] = {
    { 128, 0, FLAIR_CANON_MENU_ITEM_0, file_items,   N(file_items),   0 },
    { 129, 0, FLAIR_CANON_MENU_ITEM_1, edit_items,   N(edit_items),   0 },
    { 130, 0, FLAIR_CANON_MENU_ITEM_2, image_items,  N(image_items),  0 },
    { 131, 0, T_LAYER,                 layer_items,  N(layer_items),  0 },
    { 132, 0, FLAIR_CANON_MENU_ITEM_4, select_items, N(select_items), 0 },
    { 133, 0, FLAIR_CANON_MENU_ITEM_5, view_items,   N(view_items),   0 },
    { 134, 0, FLAIR_CANON_MENU_ITEM_6, window_items, N(window_items), 0 },
    { 135, 0, FLAIR_CANON_MENU_ITEM_7, help_items,   N(help_items),   0 }
};
static const tbx_menubar_t mbar = { menus, N(menus), 0 /* no Apple slot */, 0 };

/* The canvas, GLOBAL (l,t,r,b): below both stacked bars, clear of the APPS
 * window's first three icon rows so the launch trace stays short. Authored. */
#define CANVAS_L 330
#define CANVAS_T 150
#define CANVAS_R 630
#define CANVAS_B 400
static int32_t canvas TBX_IN_IMAGE = 0;

int tbx_app_main(void)
{
    if (tbx_register() < 0 || tbx_set_mbar(&mbar) < 0) return 1;
    canvas = tbx_new_window(CANVAS_L, CANVAS_T, CANVAS_R, CANVAS_B, TBX_GOAWAY_CLOSE);
    if (canvas < 0 || tbx_set_wtitle(canvas, "Untitled-1") < 0) { (void)tbx_exit(1); return 1; }
    return 0;
}

void tbx_app_event(const tbx_event_t *e)
{
    if (!e) return;
    if (e->what == updateEvt) {
        (void)tbx_fill_rect(canvas, 0, 0, CANVAS_R - CANVAS_L, CANVAS_B - CANVAS_T,
                            TBX_COLOR_WHITE);
        return;
    }
    if (e->what == TBX_EVT_MENU && e->message == TBX_MENU_WORD(PAINT_FILE, 5))
        (void)tbx_exit(0);
}

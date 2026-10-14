/*
 * os/apps/fonts/fonts.c -- FONTS, the Font Manager specimen tenant (THE
 * ARTIFACT; bead initech-tdnl.68, Font Manager slice 2).
 *
 * A small disk application whose window shows what slice 2 gives every
 * application: the synthesized txFace styles through TEXTDRAW (spec/
 * toolbox_gate.h Sec 7, the fg bits 8..15 extension), a window title too long
 * for its title bar (the Window Manager ends it in the Chicago ellipsis), a
 * line too long for the window (the run is clipped at the content edge), and
 * a Style menu whose items are drawn in their own styles -- the way the Mac
 * Menu Manager draws SimpleText's Style menu (menu-manager.md Sec 2.2, the
 * item style byte). It is the fixture of the emulator gate test-flair-fonts
 * and the fonts_styles clip (Rule 14); it ships on that gate's own data
 * volume, never on the flagship volume.
 *
 * Behaviour:
 *   entry      REGISTER, SETMBAR (File: Quit ^Q; Style: Plain, Bold, Italic,
 *              Underline), one narrow window, the long title, return.
 *   updateEvt  white field + the specimen lines below.
 *   File>Quit  EXIT(0). Style items: no-ops (the specimen is fixed).
 *
 * Every number the gate checks is derived in spec/flair_fonts_traces.mk.
 * Ref: os/apps/tbx/tbx.h; spec/toolbox_gate.h Sec 3/6a/7/9; os/flair/text.h
 * (the synthesis rules). ASCII-clean (Rule 12).
 */
#include "tbx.h"

const char tbx_app_name[] = "FONTS";

/* Window frame, GLOBAL (l,t,r,b): right of and below the APPS window's icon
 * row so the APPS icons stay clickable, narrow enough that the title cannot
 * fit its stripe run. */
#define WIN_L 300
#define WIN_T 170
#define WIN_R 520
#define WIN_B 400

#define MENU_FILE  129
#define MENU_STYLE 130
#define ITEM_QUIT  1

static const tbx_menuitem_t file_items[] = {
    { "Quit", 0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
/* the item style byte is the QuickDraw txFace (bold 1, italic 2, underline 4) */
static const tbx_menuitem_t style_items[] = {
    { "Plain",     0, 0, 0x00, 1, 0, { 0, 0, 0 } },
    { "Bold",      0, 0, 0x01, 1, 0, { 0, 0, 0 } },
    { "Italic",    0, 0, 0x02, 1, 0, { 0, 0, 0 } },
    { "Underline", 0, 0, 0x04, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE,  0, "File",  file_items,  1, 0 },
    { MENU_STYLE, 0, "Style", style_items, 4, 0 },
};
static const tbx_menubar_t mbar = { menus, 2, 1, 0 };

static int32_t g_win TBX_IN_IMAGE = 0;

#define INK TBX_COLOR_BLACK
static void line(int32_t y, const char *s, uint32_t face)
{
    (void)tbx_draw_text(g_win, 6, y, s, INK | face, TBX_COLOR_WHITE);
}

static void paint(void)
{
    (void)tbx_fill_rect(g_win, 0, 0, 400, 400, TBX_COLOR_WHITE);
    line(2,   "Hamburger", 0u);
    line(20,  "Hamburger", TBX_TEXT_BOLD);
    line(38,  "Hamburger", TBX_TEXT_ITALIC);
    line(56,  "jumpy gyp", 0u);
    line(74,  "jumpy gyp", TBX_TEXT_UNDERLINE);
    line(92,  "Hamburger", TBX_TEXT_BOLD | TBX_TEXT_ITALIC | TBX_TEXT_UNDERLINE);
    /* wider than the content: the Font Manager clips it at the edge */
    line(116, "Saving tables to disk... PC LOAD LETTER", TBX_TEXT_BOLD);
}

int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    (void)tbx_set_mbar(&mbar);
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, 1);
    if (g_win <= 0) return 1;
    (void)tbx_set_wtitle(g_win, "Font Manager Styles Specimen");
    return 0;
}

void tbx_app_event(const tbx_event_t *ev)
{
    switch (ev->what) {
    case updateEvt:
        paint();
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) (void)tbx_exit(0);
        break;
    default:
        break;
    }
}

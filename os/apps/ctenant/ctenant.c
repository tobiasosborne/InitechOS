/*
 * os/apps/ctenant/ctenant.c -- CTENANT.EXE, the smallest disk-launched
 * application written in C (THE ARTIFACT; bead initech-w96l, and the
 * emulator half of bead initech-8zii).
 *
 * It exists to prove the C path (os/apps/tbx: the binding, the link, the
 * InitechMZ wrap) end to end before Initech 123 rides on it, and to be the
 * fixture for "a tenant's drawing in response to a plain keyDown or
 * mouseDown reaches the screen" (initech-8zii): the gate types one key and
 * clicks once, and greps the pixels the tenant drew for those events.
 *
 * Behaviour (the C twin of os/apps/tenantfx.asm, plus input):
 *   entry      REGISTER, SETMBAR (File > Quit, Ctrl-Q), one window, return.
 *   updateEvt  white field + one proportional text line.
 *   keyDown    draws "Key <c>" (proportional, TEXTDRAW) and the same in the
 *              fixed 8x16 cell font (DRAWCELLS) -- NOT inside an update.
 *   mouseDown  in the content: draws "Click" (cells) -- NOT inside an update.
 *   File>Quit  EXIT(0).
 * Ref: os/apps/tbx/tbx.h; spec/toolbox_gate.h Sec 3/6a/7/9.
 * ASCII-clean (Rule 12).
 */
#include "tbx.h"

const char tbx_app_name[] = "CTENANT";

/* Window frame, GLOBAL (l,t,r,b): below the APPS window's icon row (sprites
 * y 106..138, labels to 152) so every APPS icon stays clickable, and close to
 * them so the gate's mouse hops stay short (FLAIR_TEN_TICK_BUDGET). */
#define WIN_L 180
#define WIN_T 170
#define WIN_R 470
#define WIN_B 330

#define MENU_FILE 129
#define ITEM_QUIT 1

static const tbx_menuitem_t file_items[] = {
    { "Quit", 0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, 1, 0 },
};
static const tbx_menubar_t mbar = { menus, 1, 1, 0 };

static int32_t g_win TBX_IN_IMAGE = 0;
static uint32_t g_keys TBX_IN_IMAGE = 0;
static char g_keyline[16] TBX_IN_IMAGE = "Key ?";

static void paint(void)
{
    (void)tbx_fill_rect(g_win, 0, 0, 400, 400, TBX_COLOR_WHITE);
    (void)tbx_draw_text(g_win, 8, 8, "A C application, from disk",
                        TBX_COLOR_BLACK, TBX_COLOR_WHITE);
}

int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    (void)tbx_set_mbar(&mbar);
    g_win = tbx_new_window(WIN_L, WIN_T, WIN_R, WIN_B, 1);
    if (g_win <= 0) return 1;
    (void)tbx_set_wtitle(g_win, "C Tenant");
    return 0;
}

void tbx_app_event(const tbx_event_t *ev)
{
    switch (ev->what) {
    case updateEvt:
        paint();
        break;
    case keyDown: {
        char c = (char)(ev->message & 0xFF);
        if (c < ' ' || c > '~') c = '?';
        g_keys++;
        g_keyline[4] = c;
        (void)tbx_draw_text(g_win, 8, 40, g_keyline, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
        (void)tbx_draw_cells(g_win, 8, 64, g_keyline, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
        break;
    }
    case mouseDown:
        (void)tbx_draw_cells(g_win, 8, 96, "Click", TBX_COLOR_WHITE, TBX_COLOR_NAVY);
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) (void)tbx_exit(0);
        break;
    default:
        break;
    }
}

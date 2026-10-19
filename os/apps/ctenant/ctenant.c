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
 *   updateEvt  render(): white field, the startup line, and every readout
 *              the retained state holds (below).
 *   keyDown    retains the key, then render(): "Key <c>" (proportional,
 *              TEXTDRAW) and the same in the fixed 8x16 cell font
 *              (DRAWCELLS) -- drawn at once, NOT inside an update.
 *   mouseDown  in the content: retains the click, then render(): "Click"
 *              (cells) -- drawn at once, NOT inside an update.
 *   File>Quit  EXIT(0).
 * RETAINED STATE (bead initech-6qsk, audit L117): the readouts live in the
 * app's own globals and ONE renderer paints all of them, so a window drag or
 * an obscure/reveal -- each an updateEvt -- redraws what was there. Ref:
 * Inside Macintosh: Macintosh Toolbox Essentials, Window Manager ("Updating
 * the Content of a Window") and Event Manager (update events): the Window
 * Manager only tells the application WHERE to redraw; the application must
 * be able to redraw its content from its own data at any time. The tenant
 * gets its updateEvt through the gate per ADR-0013 (the App Contract).
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
static uint32_t g_keys TBX_IN_IMAGE = 0;    /* keyDowns seen; 0 = no readout */
static uint32_t g_clicks TBX_IN_IMAGE = 0;  /* content clicks; 0 = no readout */
static char g_keyline[16] TBX_IN_IMAGE = "Key ?";  /* the LAST key, both lines */

/* The startup paint alone: the clear and the one proportional line. */
static void paint_field(void)
{
    (void)tbx_fill_rect(g_win, 0, 0, 400, 400, TBX_COLOR_WHITE);
    (void)tbx_draw_text(g_win, 8, 8, "A C application, from disk",
                        TBX_COLOR_BLACK, TBX_COLOR_WHITE);
}

/* THE ONE RENDERER (initech-6qsk): the field, then every readout the
 * retained state holds -- the last key on both lines, the click. */
static void render(void)
{
    paint_field();
    if (g_keys != 0) {
        (void)tbx_draw_text(g_win, 8, 40, g_keyline, TBX_COLOR_BLACK, TBX_COLOR_WHITE);
        (void)tbx_draw_cells(g_win, 8, 64, g_keyline, TBX_COLOR_WHITE, TBX_COLOR_NAVY);
    }
    if (g_clicks != 0)
        (void)tbx_draw_cells(g_win, 8, 96, "Click", TBX_COLOR_WHITE, TBX_COLOR_NAVY);
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
#ifndef CTENANT_MUT_NO_RETAIN
        render();
#else
        /* MUTANT CTENANT_MUT_NO_RETAIN (Rule 6; test-flair-ctenant-mutant
         * ONLY, built into its own fixture volume): the pre-6qsk update
         * paint -- the clear and the startup line, no readouts -- so a drag
         * or a reveal erases them. NEVER in the shipped CTENANT.EXE. */
        paint_field();
#endif
        break;
    case keyDown: {
        char c = (char)(ev->message & 0xFF);
        if (c < ' ' || c > '~') c = '?';
        g_keys++;
        g_keyline[4] = c;
        render();
        break;
    }
    case mouseDown:
        g_clicks++;
        render();
        break;
    case TBX_EVT_MENU:
        /* Test-only declining tenant (initech-cqej): tbx_ask_quit uses the
         * SAME File > Quit handler. Staying resident declines the request,
         * as kmain.c finder_power_ask_apps specifies (Inside Macintosh VI,
         * PDF p.345 Listing 6-8). Never defined for shipped CTENANT.EXE. */
#ifndef CTENANT_DECLINE_QUIT
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) (void)tbx_exit(0);
#endif
        break;
    default:
        break;
    }
}

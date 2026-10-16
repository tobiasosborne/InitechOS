/*
 * os/apps/calc/calc.c -- CALC.EXE, the Calculator desk accessory (THE
 * ARTIFACT; bead initech-tdnl.17, GUI remediation R4.3). A C disk tenant
 * (os/apps/tbx) launched from the Apple menu (os/flair/apple_menu.c) or by
 * double-click in APPS.
 *
 * Behaviour (ground truth: calc_core.h, the real Mac OS 8.1 Calculator):
 *   entry      REGISTER, SETMBAR (File: Close ^W), the rounded rDocProc
 *              window at the capture's own place (spec/calc/calc_layout.h)
 *              titled "Calculator", return.
 *   updateEvt  the whole face (calc_view_paint).
 *   mouseDown  in a key: THAT key's command (calc_view_hit/_click_cmd).
 *   keyDown    the key map's command (calc_cmd_from_key).
 *              BOTH routes end in the ONE calc_press + readout repaint --
 *              click/key parity is structural (and graded, test_calc C4).
 *   close box / File > Close: EXIT(0).
 * Every repaint is FILLRECTs of calc_view's rects; no string reaches the
 * gate, so the engine's state may live in .bss.
 *
 * The emu gate follows the readout through the kernel's per-call trace:
 * every command repaints the readout interior, one
 *   TENANT-GATE ax=0x0031 win=1 l=8 t=8 r=92 b=26 c=0 -> 0
 * line (calc_view_paint_readout's white fill) per command.
 *
 * Gaps (recorded, not invented): the key press flash (the real Calculator
 * inverts a key while it is held, s8_calc_key7_pressed.png) needs mouseUp /
 * keyUp delivery, which the App Contract does not route to tenants
 * (os/flair/process.c); the Edit menu's Copy/Paste need a tenant Scrap call.
 *
 * ASCII-clean (Rule 12).
 */
#include "tbx.h"
#include "calc_core.h"
#include "calc_view.h"
#include "calc/calc_layout.h"

const char tbx_app_name[] = "CALC";

#define MENU_FILE  129
#define MENU_EDIT  130
#define ITEM_CLOSE 1

static const tbx_menuitem_t file_items[] = {
    { "Close", 0, 'W', 0, 1, 0, { 0, 0, 0 } },
};
/* The DA's Edit menu: present (the capture's bar is File Edit Help), every
 * row dimmed until a tenant Scrap call exists (recorded gap). */
static const tbx_menuitem_t edit_items[] = {
    { "Undo",  0, 'Z', 0, 0, 0, { 0, 0, 0 } },
    { "-",     0, 0,   0, 0, 1, { 0, 0, 0 } },
    { "Cut",   0, 'X', 0, 0, 0, { 0, 0, 0 } },
    { "Copy",  0, 0,   0, 0, 0, { 0, 0, 0 } },
    { "Paste", 0, 'V', 0, 0, 0, { 0, 0, 0 } },
    { "Clear", 0, 0,   0, 0, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, 1, 0 },
    { MENU_EDIT, 0, "Edit", edit_items, 6, 0 },
};
static const tbx_menubar_t mbar = { menus, 2, 1, 0 };

static int32_t g_win TBX_IN_IMAGE = 0;
static calc_t  g_calc;                  /* .bss: never handed to the gate */

static void gate_rect(void *user, int l, int t, int r, int b, int tok)
{
    (void)user;
    (void)tbx_fill_rect(g_win, l, t, r, b, (uint32_t)tok);
}

static void command(calc_cmd_t k)
{
    if (k == CALC_NONE) return;
    calc_press(&g_calc, k);
    calc_view_paint_readout(calc_display(&g_calc), gate_rect, (void *)0);
}

int tbx_app_main(void)
{
    if (tbx_register() != TBX_OK) return 1;
    (void)tbx_set_mbar(&mbar);
    calc_init(&g_calc);
    g_win = tbx_new_window(CALC_WIN_L, CALC_WIN_T, CALC_WIN_L + CALC_WIN_W,
                           CALC_WIN_T + CALC_WIN_H,
                           (int32_t)(TBX_GOAWAY_CLOSE | TBX_WDEF_RDOC));
    if (g_win <= 0) return 1;
    (void)tbx_set_wtitle(g_win, "Calculator");
    return 0;
}

void tbx_app_event(const tbx_event_t *ev)
{
    switch (ev->what) {
    case updateEvt:
        calc_view_paint(calc_display(&g_calc), gate_rect, (void *)0);
        break;
    case mouseDown: {
        int32_t org = tbx_content_origin(g_win);
        if (org < 0) break;
        int x = (int)ev->whereh - (int)(org & 0xFFFF);
        int y = (int)ev->wherev - (int)((uint32_t)org >> 16);
        command(calc_view_click_cmd(calc_view_hit(x, y)));
        break;
    }
    case keyDown:
    case autoKey:
        command(calc_cmd_from_key((uint32_t)ev->message));
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_CLOSE)) (void)tbx_exit(0);
        break;
    default:
        break;
    }
}

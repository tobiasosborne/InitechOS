/*
 * os/apps/calcda/calcda.c -- CALCULAT.EXE, the Calculator desk accessory (THE
 * ARTIFACT; bead initech-tdnl.17, GUI remediation R4.3). A C disk tenant
 * (os/apps/tbx) launched from the Apple menu (os/flair/apple_menu.c) or by
 * double-click in APPS.
 *
 * Behaviour (ground truth: calc_core.h, the real Mac OS 8.1 Calculator):
 *   entry      REGISTER, SETMBAR (File: Close ^W / Quit ^Q), the rounded rDocProc
 *              window at the capture's own place (spec/calcda/calcda_layout.h)
 *              titled "Calculator", return.
 *   updateEvt  the whole face (calc_view_paint).
 *   mouseDown  in a key: THAT key's command (calc_view_hit/_click_cmd).
 *   keyDown    the key map's command (calc_cmd_from_key).
 *              BOTH routes end in the ONE calc_press + readout repaint --
 *              click/key parity is structural (and graded, test_calcda C4).
 *   close box / File > Close / Quit (including shutdown request): EXIT(0).
 * Every repaint is FILLRECTs of calc_view's rects; no string reaches the
 * gate, so the engine's state may live in .bss.
 *
 * The emu gate follows the readout through the kernel's per-call trace:
 * every command repaints the readout interior, one
 *   TENANT-GATE ax=0x0031 win=1 l=8 t=8 r=92 b=26 c=0 -> 0
 * line (calc_view_paint_readout's white fill) per command.
 *
 * The held key (initech-m6v0): a press on a key inverts it and asks the
 * shell to track (TBX_TRACKMOUSE); the command runs on a release over the
 * key, as the real Calculator does (s8_calc_key7_pressed: 7 held, the
 * readout still 15; s8_calc_after_click7: released, 7). Leaving the key
 * shows it idle, coming back re-inverts it, and a release off it does
 * nothing (Inside Macintosh I-323, TrackControl). Deactivation ends a press
 * without acting (authored). Keyboard keys act at once and do not flash:
 * the App Contract delivers no keyUp (os/flair/process.c).
 *
 * ASCII-clean (CLAUDE.md, The constitution).
 */
#include "tbx.h"
#include "calc_core.h"
#include "calc_view.h"
#include "calcda/calcda_layout.h"

const char tbx_app_name[] = "CALCULAT";

#define MENU_FILE  129
#define MENU_EDIT  130
#define ITEM_CLOSE 1
#define ITEM_QUIT  2

static const tbx_menuitem_t file_items[] = {
    { "Close", 0, 'W', 0, 1, 0, { 0, 0, 0 } },
    /* initech-0xs6: opt into tbx_ask_quit's existing explicit Quit contract.
     * Same documentless close action (Mac guide printed p.125), retaining W.
     * Never infer permission to terminate document tenants from a close box. */
    { "Quit", 0, 'Q', 0, 1, 0, { 0, 0, 0 } },
};
/* Authored readout binding of Copy (Mac System Software Guide v6 p.118),
 * and menu binding of the captured C/Clear reset (calc_core.h p00/s10/s11).
 * No clipboard numeric parser or arithmetic history is advertised. */
static const tbx_menuitem_t edit_items[] = {
    { "Copy",  0, 0,   0, 1, 0, { 0, 0, 0 } },
    { "Clear", 0, 0,   0, 1, 0, { 0, 0, 0 } },
};
static const tbx_menuinfo_t menus[] = {
    { MENU_FILE, 0, "File", file_items, sizeof file_items / sizeof file_items[0], 0 },
    { MENU_EDIT, 0, "Edit", edit_items, 2, 0 },
};
static const tbx_menubar_t mbar = { menus, 2, 1, 0 };

static int32_t g_win TBX_IN_IMAGE = 0;
static calc_t  g_calc;                  /* .bss: never handed to the gate */

static void gate_rect(void *user, int l, int t, int r, int b, int tok)
{
    (void)user;
    (void)tbx_fill_rect(g_win, l, t, r, b, (uint32_t)tok);
}

/* The held key: its index, or -1; and whether the pointer is over it. */
static int g_held TBX_IN_IMAGE = -1;
static int g_over TBX_IN_IMAGE = 0;

static int local_key(const tbx_event_t *ev)
{
    int32_t org = tbx_content_origin(g_win);
    if (org < 0) return -1;
    return calc_view_hit((int)ev->whereh - (int)(org & 0xFFFF),
                         (int)ev->wherev - (int)((uint32_t)org >> 16));
}

static void paint_held(int pressed)
{
    if (g_held >= 0) calc_view_paint_key(g_held, pressed, gate_rect, (void *)0);
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
    g_held = -1; g_over = 0;
    g_win = tbx_new_window(CALC_WIN_L, 20, CALC_WIN_L + CALC_WIN_W,
                           20 + CALC_WIN_H,
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
        if (g_over) paint_held(1);
        break;
    case mouseDown: {
        int k = local_key(ev);
        if (calc_view_click_cmd(k) == CALC_NONE) break;
        if (tbx_track_mouse() != TBX_OK) { command(calc_view_click_cmd(k)); break; }
        g_held = k; g_over = 1;
        paint_held(1);
        break;
    }
    case TBX_EVT_TRACK: {
        if (g_held < 0) break;
        int over = local_key(ev) == g_held;
        if (ev->message == 1) {             /* the release */
            int k = g_held;
            paint_held(0);
            g_held = -1; g_over = 0;
            if (over) command(calc_view_click_cmd(k));
        } else if (over != g_over) {
            g_over = over;
            paint_held(over);
        }
        break;
    }
    case activateEvt:
        if (!(ev->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) && g_held >= 0) {
            paint_held(0);
            g_held = -1; g_over = 0;
        }
        break;
    case keyDown:
    case autoKey:
        if (!(ev->modifiers & FLAIR_EVT_MOD_CONTROL_KEY))
            command(calc_cmd_from_key((uint32_t)ev->message));
        break;
    case TBX_EVT_MENU:
        if (ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_CLOSE) ||
            ev->message == TBX_MENU_WORD(MENU_FILE, ITEM_QUIT)) (void)tbx_exit(0);
        else if (ev->message == TBX_MENU_WORD(MENU_EDIT, 1)) {
            const char *s = calc_display(&g_calc);
            uint32_t n = 0;
            while (s[n]) n++;
            if (tbx_scrap_put(s, n) != TBX_OK)
                (void)tbx_alert("The readout could not be copied.", "OK", 0, 0);
        } else if (ev->message == TBX_MENU_WORD(MENU_EDIT, 2)) command(CALC_CLEAR);
        break;
    default:
        break;
    }
}

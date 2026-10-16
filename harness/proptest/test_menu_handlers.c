/*
 * test_menu_handlers.c -- THE HANDLER GUARD (HOST oracle, C).
 *
 * beads: initech-tdnl.36 (+ .37, .38, .50, .67): "no command may be enabled
 *        and do nothing" (audit 2026-10-03 F03/F05/F07/F15, pass 2 G10).
 *
 * THE RULE: a menu command is either IMPLEMENTED or DRAWN DISABLED. The audit
 * found eight commands enabled, selectable, logged -- and inert, all through
 * one mechanism: finder_dispatch returned after the shell hook, whose default
 * arm returned in silence. This file holds every menu row the live system can
 * draw to the rule, structurally, so the class cannot come back:
 *
 *   H1 every row of the Finder bar (File/Edit/View/Special/Help) and its Apple
 *      menu that can be ENABLED -- refreshed under the all-live FinderCtx
 *      (selection, a selected folder, a front disk window, a non-empty Trash)
 *      AND the all-zero one -- resolves to a command-table row whose command
 *      the shell's execution table implements (finder_shell_implements, the
 *      SAME table fw_exec dispatches through).
 *   H2 every row of the shell-owned band-1 bar (os/flair/shell_menus.h) that is
 *      enabled is on the STATED exception list -- today only "Quit" (see that
 *      header: graying it would loosen test-flair-menu). Nothing in the pump
 *      executes a band-1 selection, so any other enabled row is a fake.
 *   H3 the fallback really fires: a command the shell does not implement,
 *      dispatched with the hook bound, prints FINDER-NYI (never silence).
 *   H4 the positive list: the commands the bar CAN light are exactly the ones
 *      hand-typed here (New Folder, Open, Close Window, Select All, by Icons,
 *      Clean Up, Arrange, and -- since bead initech-6k12 -- Empty Trash) -- so
 *      a command that loses its implementation is caught too, not only one
 *      that gains a fake enable.
 *
 * Expectations are HAND-TYPED (Law 2): the exception list, the positive list,
 * the FINDER-NYI line. The walk reads the live resource (that is what is under
 * test) but never derives what it expects from it.
 *
 * MUTANTS (Rule 6; -D on the implementation TUs):
 *   FINDER_MENU_MUT_STUB_ENABLED  -- Get Info lit by a selection again (H1 RED)
 *   SHELL_MENUS_MUT_ABOUT_LIVE    -- band-1 About enabled again (H2 RED)
 *   FINDER_CMD_MUT_EXEC_SWALLOWS  -- dispatch returns after the hook whatever
 *                                    it says, the audited root cause (H3 RED)
 *   FINDER_WIN_MUT_EXIT_DOS_UNWIRED -- the Special > Exit to InitechDOS row
 *                                    stays lit but the shell's execution
 *                                    table lost it (H5 RED)
 *
 * H5 (bead initech-3p1a) -- THE EXIT-TO-DOS ROUTE: Special item 10 "Exit to
 *   InitechDOS" (after a divider, item 9) is lit at rest and when all-live,
 *   resolves to FCMD_EXIT_DOS (serial name EXIT_DOS), the shell implements it,
 *   a mouse dispatch prints the FINDER-CMD line and leaves the outcome the
 *   kernel services (EXIT_DOS, OK), and the confirmation alert says, wrapped
 *   at the 285-px Chicago 12 column, exactly the two lines hand-derived below.
 *
 * Ref: os/flair/finder_cmd.h (FinderCtx.exec returns handled),
 *      os/flair/finder_windows.h (finder_shell_implements),
 *      os/flair/finder_menu.c (the rule's banner), os/flair/shell_menus.h.
 * ASCII-clean (Rule 12).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "finder_menu.h"
#include "finder_cmd.h"
#include "finder_windows.h"
#include "shell_menus.h"
#include "test_assert.h"

TEST_HARNESS();

static FinderCtx mk(uint16_t sel, uint8_t open, uint8_t front, uint8_t trash)
{
    FinderCtx fx;
    memset(&fx, 0, sizeof fx);
    fx.selection_count    = sel;
    fx.selection_openable = open;
    fx.front_is_diskwin   = front;
    fx.trash_nonempty     = trash;
    return fx;
}

/* H4's hand-typed positive list: the commands the Finder bar may light. */
static const finder_cmd_id CAN_LIGHT[] = {
    FCMD_NEW_FOLDER, FCMD_OPEN, FCMD_CLOSE_WINDOW, FCMD_SELECT_ALL,
    FCMD_VIEW_ICONS, FCMD_CLEANUP, FCMD_ARRANGE_BY_NAME,
    FCMD_EMPTY_TRASH,         /* bead initech-6k12: implemented, lit iff full */
    FCMD_RESTART, FCMD_SHUTDOWN, /* bead initech-tdnl.78: implemented, always lit */
    FCMD_EXIT_DOS                /* bead initech-3p1a: implemented, always lit */
};
#define N_CAN_LIGHT ((int)(sizeof CAN_LIGHT / sizeof CAN_LIGHT[0]))

static int in_can_light(finder_cmd_id id)
{
    for (int i = 0; i < N_CAN_LIGHT; i++) if (CAN_LIGHT[i] == id) return 1;
    return 0;
}

static int g_lit[64];

static void walk_menu(const MenuInfo *m, const char *state)
{
    char what[200];
    for (int k = 0; k < (int)m->n_items; k++) {
        const MenuItem *it = &m->items[k];
        const finder_cmd_t *c;
        if (it->is_divider || !it->enabled) continue;
        c = finder_cmd_lookup(m->menuID, (uint16_t)(k + 1));
        snprintf(what, sizeof what,
                 "H1 [%s] enabled row menu=%d item=%d \"%s\" has a REAL handler",
                 state, (int)m->menuID, k + 1, it->text ? it->text : "?");
        CHECK(c != NULL && finder_shell_implements(c->id), what);
        if (c != NULL && (int)c->id >= 0 && (int)c->id < 64) g_lit[c->id] = 1;
        snprintf(what, sizeof what,
                 "H4 [%s] \"%s\" is on the hand-typed list of commands that may light",
                 state, it->text ? it->text : "?");
        CHECK(c != NULL && in_can_light(c->id), what);
    }
}

static void leg_finder_bar(void)
{
    MenuBar *bar = finder_menu_bar();
    FinderCtx live = mk(2, 1, 1, 1);
    FinderCtx rest = mk(0, 0, 0, 0);

    memset(g_lit, 0, sizeof g_lit);
    finder_menu_refresh_enables(&live);
    for (int mi = 0; mi < (int)bar->n_menus; mi++)
        walk_menu(&bar->menus[mi], "all-live");
    walk_menu(finder_menu_apple(), "all-live");
    for (int i = 0; i < N_CAN_LIGHT; i++)
        CHECK(g_lit[CAN_LIGHT[i]] == 1,
              "H4 every command on the positive list really is lit in the all-live state");

    finder_menu_refresh_enables(&rest);
    for (int mi = 0; mi < (int)bar->n_menus; mi++)
        walk_menu(&bar->menus[mi], "rest");
    walk_menu(finder_menu_apple(), "rest");

    /* The six audited fakes, by name, in the all-live state. */
    finder_menu_refresh_enables(&live);
    CHECK(bar->menus[FINDER_MENU_IX_FILE].items[5].enabled == 0u,
          "H1 File > Get Info is drawn disabled even with a selection (F03)");
    CHECK(bar->menus[FINDER_MENU_IX_FILE].items[6].enabled == 0u,
          "H1 File > Duplicate is drawn disabled even with a selection (F03)");
    /* RE-KEY (bead initech-tdnl.78, stated): F05's two fakes are REAL now --
     * lit in EVERY state, and the shell implements both (walk_menu above
     * already holds each lit row to a real handler; this names them). */
    CHECK(bar->menus[FINDER_MENU_IX_SPECIAL].items[5].enabled == 1u &&
          bar->menus[FINDER_MENU_IX_SPECIAL].items[6].enabled == 1u &&
          finder_shell_implements(FCMD_RESTART) &&
          finder_shell_implements(FCMD_SHUTDOWN),
          "H1 Special > Restart / Shut Down are lit and the shell implements both (F05, tdnl.78)");
    {
        FinderCtx doc = mk(1, 0, 1, 0);
        finder_menu_refresh_enables(&doc);
        CHECK(bar->menus[FINDER_MENU_IX_FILE].items[1].enabled == 0u,
              "H1 File > Open is disabled when only a DOCUMENT is selected (F07)");
        /* bead initech-6k12 (audit F04): Empty Trash moved from the
         * drawn-disabled list to a REAL handler -- lit exactly while the
         * Trash holds something, dark on an empty Trash. */
        CHECK(bar->menus[FINDER_MENU_IX_SPECIAL].items[1].enabled == 0u,
              "H1 Special > Empty Trash is disabled while the Trash is empty");
        finder_menu_refresh_enables(&live);
        CHECK(bar->menus[FINDER_MENU_IX_SPECIAL].items[1].enabled == 1u &&
              finder_shell_implements(FCMD_EMPTY_TRASH),
              "H1 Special > Empty Trash is lit with a full Trash, and the shell implements it");
    }
}

static void leg_shell_bar(void)
{
    char what[160];
    for (int k = 0; k < SHELL_SYS_MENU_ITEMS_N; k++) {
        const MenuItem *it = &SHELL_SYS_MENU_ITEMS[k];
        if (it->is_divider || !it->enabled) continue;
        snprintf(what, sizeof what,
                 "H2 band-1 enabled row \"%s\" is on the STATED exception list "
                 "(no band-1 selection is executed)", it->text);
        CHECK(strcmp(it->text, "Quit") == 0, what);
    }
    CHECK(strcmp(SHELL_SYS_MENU_ITEMS[0].text, "About") == 0 &&
          SHELL_SYS_MENU_ITEMS[0].enabled == 0u,
          "H2 band-1 About is drawn disabled until R4.1 (F15)");
}

/* H3: the FINDER-NYI fallback. A zeroed shell is a valid hook target (fw_exec
 * only needs a non-NULL cookie to look the command up). */
static char g_last[160];
static int  g_lines;
static void sink(void *u, const char *line)
{
    (void)u;
    g_lines++;
    strncpy(g_last, line, sizeof g_last - 1);
}

static void leg_fallback(void)
{
    static finder_shell_t sh;
    FinderCtx fx = mk(1, 0, 0, 0);
    uint32_t get_info = ((uint32_t)512u << 16) | 6u;

    memset(&sh, 0, sizeof sh);
    fx.trace = sink;
    finder_shell_bind_ctx(&sh, &fx);
    fx.selection_count = 1;              /* bind re-synced it; force the row live */
    CHECK(fx.exec != NULL, "H3 (setup) the shell hook is bound");

    g_lines = 0; g_last[0] = '\0';
    finder_dispatch(&fx, get_info, "mouse");
    CHECK(g_lines == 2 && strcmp(g_last, "FINDER-NYI name=GET_INFO") == 0,
          "H3 an unimplemented command reaching dispatch says FINDER-NYI (never silence)");
}

/* H5 (bead initech-3p1a). The alert wording is the operator's (2026-10-16);
 * the wrap is hand-derived from the System 7.0.1 Chicago 12 advances typed
 * from ../system7-decomp specs/fonts/chicago.md "Advance-width table" (NOT
 * read from the artifact's table -- Law 2), space 4:
 *   Are A8 r6 e8 = 22 | you y8 o8 u8 = 24 | sure s7 u8 r6 e8 = 29 |
 *   want w12 a8 n8 t6 = 34 | to t6 o8 = 14 | exit e8 x8 i4 t6 = 26 |
 *   InitechDOS? I6 n8 i4 t6 e8 c7 h8 D8 O8 S7 ?8 = 78
 *   "Are you sure you want to exit to" = 22+24+29+24+34+14+26+14 + 7*4 = 215;
 *   + " InitechDOS?" = 215 + 4 + 78 = 297 > 285 -> the last word wraps. */
static void leg_exit_dos(void)
{
    static finder_shell_t sh;
    MenuBar *bar = finder_menu_bar();
    const MenuInfo *sp = &bar->menus[FINDER_MENU_IX_SPECIAL];
    FinderCtx rest = mk(0, 0, 0, 0), live = mk(2, 1, 1, 1);
    const finder_cmd_t *c;
    char lines[FINDER_ALERT_LINES][FINDER_ALERT_LINE_MAX];
    const finder_cmd_outcome_t *o;
    char first[160];
    int n;

    CHECK(sp->n_items == 10u && sp->items[8].is_divider &&
          sp->items[9].text != NULL &&
          strcmp(sp->items[9].text, "Exit to InitechDOS") == 0,
          "H5 Special item 10 is \"Exit to InitechDOS\", after a divider (item 9)");
    finder_menu_refresh_enables(&rest);
    CHECK(sp->items[9].enabled == 1u, "H5 Exit to InitechDOS is lit at rest");
    finder_menu_refresh_enables(&live);
    CHECK(sp->items[9].enabled == 1u, "H5 Exit to InitechDOS is lit all-live");
    c = finder_cmd_lookup(515, 10u);
    CHECK(c != NULL && c->id == FCMD_EXIT_DOS && c->serial_name != NULL &&
          strcmp(c->serial_name, "EXIT_DOS") == 0,
          "H5 lookup(515,10) is FCMD_EXIT_DOS, serial name EXIT_DOS");
    CHECK(finder_shell_implements(FCMD_EXIT_DOS),
          "H5 Special > Exit to InitechDOS is implemented by the shell's execution table");

    memset(&sh, 0, sizeof sh);
    live.trace = sink;
    finder_shell_bind_ctx(&sh, &live);
    g_lines = 0; g_last[0] = '\0'; first[0] = '\0';
    finder_dispatch(&live, ((uint32_t)515u << 16) | 10u, "mouse");
    if (g_lines >= 1) strncpy(first, g_last, sizeof first - 1);
    o = finder_shell_take_outcome(&sh);
    CHECK(g_lines == 1 &&
          strcmp(first, "FINDER-CMD id=16 name=EXIT_DOS src=mouse sel=0") == 0,
          "H5 a mouse Exit to InitechDOS prints exactly its FINDER-CMD line (no FINDER-NYI)");
    CHECK(o != NULL && o->id == FCMD_EXIT_DOS && o->status == FINDER_WIN_OK &&
          o->moved == 0,
          "H5 the shell leaves the EXIT_DOS outcome (OK, no open view) for the kernel");

    n = finder_exit_dos_alert_text(lines);
    CHECK(n == 2 && strcmp(lines[0], "Are you sure you want to exit to") == 0 &&
          strcmp(lines[1], "InitechDOS?") == 0,
          "H5 the alert reads \"Are you sure you want to exit to\" / \"InitechDOS?\"");
}

int main(void)
{
    leg_finder_bar();
    leg_shell_bar();
    leg_fallback();
    leg_exit_dos();
    return TEST_SUMMARY("test-menu-handlers");
}

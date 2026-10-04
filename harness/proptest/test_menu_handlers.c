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
    FCMD_EMPTY_TRASH          /* bead initech-6k12: implemented, lit iff full */
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
    CHECK(bar->menus[FINDER_MENU_IX_SPECIAL].items[5].enabled == 0u &&
          bar->menus[FINDER_MENU_IX_SPECIAL].items[6].enabled == 0u,
          "H1 Special > Restart / Shut Down are drawn disabled (F05)");
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

int main(void)
{
    leg_finder_bar();
    leg_shell_bar();
    leg_fallback();
    return TEST_SUMMARY("test-menu-handlers");
}

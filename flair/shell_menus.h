/*
 * os/flair/shell_menus.h -- the ITEM LIST of the shell-owned band-1 bar
 * (THE ARTIFACT; header-only constant data).
 *
 * beads: initech-tdnl.50 (audit 2026-10-03 F15), initech-tdnl.36 (the rule).
 *
 * Band 1 is the static System-7 bar of the canon chimera frame (shell.h Sec 1
 * "TWO STACKED MENU BARS"); its titles are the frame's and stay (Law 4: the
 * chimera bar is part of the canon frame). Each title drops the same two-row
 * list, authored HERE so the live kernel (os/milton/kmain.c copies it into
 * every band-1 MenuInfo) and the handler guard
 * (harness/proptest/test_menu_handlers.c) read ONE list.
 *
 * THE RULE: a command is either implemented or drawn disabled. Nothing in the
 * pump executes a band-1 selection (flair_live_do_menu discards the result), so
 *   About -- authored 0 (grayed) until R4.1 / initech-tdnl.15 gives it a
 *            dialog; it was enabled and did nothing (F15).
 *   Quit  -- STAYS 1, a STATED, NAMED EXCEPTION: it is equally a no-op today,
 *            but it is the selection target of test-flair-menu and
 *            test-flair-menu-crossdrag (they require MenuSelect to return
 *            "Quit"), so graying it would re-key those oracles to accept
 *            "nothing selected" -- a loosening (CLAUDE.md stop condition). The
 *            guard lists it explicitly; giving Quit a real meaning (quit the
 *            foreground tenant) is follow-up work for the operator to rule on.
 *
 * Field order is menu.h Sec 3: text, mark, cmdChar, style, enabled, is_divider.
 * ASCII-clean (Rule 12).
 */
#ifndef INITECH_OS_FLAIR_SHELL_MENUS_H
#define INITECH_OS_FLAIR_SHELL_MENUS_H

#include "menu.h"

#define SHELL_SYS_MENU_ITEMS_N 2

static const MenuItem SHELL_SYS_MENU_ITEMS[SHELL_SYS_MENU_ITEMS_N]
    __attribute__((unused)) = {
#if !defined(SHELL_MENUS_MUT_ABOUT_LIVE)
    { "About", 0, 0,   0, 0, 0 },   /* grayed until R4.1 (tdnl.15)          */
#else
    /* MUTANT (Rule 6; test-menu-handlers): the pre-fix live About -- enabled,
     * no handler. NEVER in a real build. */
    { "About", 0, 0,   0, 1, 0 },
#endif
    { "Quit",  0, 'Q', 0, 1, 0 }    /* the stated exception (banner)        */
};

#endif /* INITECH_OS_FLAIR_SHELL_MENUS_H */

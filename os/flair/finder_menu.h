/*
 * os/flair/finder_menu.h -- THE FINDER MENU BAR RESOURCE (THE ARTIFACT).
 *
 * beads: initech-tdnl.12 (GUI remediation R3.5 "the real Finder menu bar",
 *        stage 1). The F4.2 layout expressed as static MenuBar / MenuInfo /
 *        MenuItem data, plus the ONE routine that recomputes the predicate-
 *        driven enable bytes before every DrawMenuBar / track.
 *
 * WHAT THIS IS (docs/design/GUI-remediation-R3-finder-design.md F4.1-F4.3):
 *   F4-2: "The full Apple/File/Edit/View/Special/Help layout above is expressed
 *   as static MenuBar/MenuInfo/MenuItem data with predicate-recomputed enable
 *   bits; Label submenu omitted (no submenu support, D3-46); dynamic graying is
 *   data maintenance, not new rendering."
 *
 *   So: no new rendering, no new tracking, no new dispatch. The Menu Manager
 *   (os/flair/menu.h) draws this bar, MenuSelect/MenuKey pick items out of it,
 *   and os/flair/finder_cmd.c turns the resulting (menuID<<16|item) word into a
 *   command. THIS file is the DATA between them.
 *
 * STORAGE (menu.h Sec 3, the flair_ten_notes_bar idiom in os/milton/kmain.c):
 *   caller-supplied static arrays; no malloc. The MenuItem arrays are MUTABLE
 *   because finder_menu_refresh_enables writes their `enabled` bytes -- that is
 *   the ONLY field any code mutates after load; text/mark/cmdChar/style/
 *   is_divider are authored once here and never touched (F4.4: "the menu
 *   resource stays static data; only the enable bits mutate"). MenuInfo is
 *   likewise mutable only because menu.c caches menuWidth into it.
 *
 * MENU IDS 512..516 (F4-1). Disjoint from the shell System-7 bar's 128..131,
 * the Photoshop bar's 256..263 and NOTES's 384..386 -- the occupied ranges are
 * documented in spec/flair_tenants_demo.h (the NOTES block, lines 59-67) and in
 * os/flair/shell.c. The Apple-slot entries carry menu id 0 (F4.4).
 *
 * THE APPLE MENU IS DATA HERE, NOT A BAR MENU -- AND SAY WHY (Law 1 honesty):
 *   menu.h's MenuBar has an `has_apple` FLAG that draws the Apple glyph in a
 *   fixed FLAIR_MENU_APPLE_W slot at the far left; MenuBar_hit returns -1 for
 *   any x inside that slot (menu.h Sec 5), so the Menu Manager has no route
 *   from an Apple-slot click to a pull-down. Putting the Apple menu into
 *   bar->menus[] instead would lay it out as an ORDINARY TITLED menu -- it
 *   would draw its title text after the glyph slot and shift File/Edit/View/
 *   Special/Help right by its slot width, which is a Law-4 regression, not a
 *   feature. So the Apple menu ships as a standalone MenuInfo the resource
 *   exports (finder_menu_apple), the finder_cmd table already carries its rows
 *   (menu_id 0), and the Apple-slot hit/drop seam in menu.c is left for the
 *   bead that adds it. Data present, route absent, stated -- not silently
 *   dropped.
 *
 * OMIT-NOT-GRAY, APPLIED (ruling F4-3):
 *   "Apple-menu launchables are omit-if-not-installed; context-dependent
 *   commands are visible-and-grayed." The nine unbuilt desk accessories
 *   (Scrapbook / Key Caps / Chooser / Control Panels / Find File / Jigsaw
 *   Puzzle / Stickies / SimpleSound) are OMITTED per F4.2. THE SAME RULE THEN
 *   REACHES CALCULATOR AND NOTE PAD: F4.2 lists them as "launchable (target on
 *   volume)", but NO application exists on the flagship data volume until bead
 *   initech-tdnl.14 (R4) puts one there, so shipping them now would be two rows
 *   that cannot launch -- exactly what F4-3 forbids. They are OMITTED THIS
 *   STAGE and return with tdnl.14, at which point the Apple menu regains its
 *   divider + the two launch rows and finder_cmd.c's FCMD_LAUNCH_APPLE rows
 *   (Apple items 3 and 4) become reachable. THE COMMAND TABLE IS UNCHANGED:
 *   its rows still name items 3/4, which simply do not exist in the resource
 *   yet -- the seam, left open on purpose and documented on both sides.
 *
 * Label (File menu) is omitted for the reason F4-2 gives: no submenu support
 * (D3 row 46). Its absence is a layout choice, not a graying choice.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.1 (band 2 + the id
 *        range), F4.2 (THE layout -- every row below is read off it), F4.3
 *        (the omit-not-gray ruling), F4.4 (the predicate list + the command
 *        table this resource is the front end of).
 *      os/flair/menu.h Sec 3 (MenuItem/MenuInfo/MenuBar storage conventions),
 *        Sec 5 (bar title layout + the Apple slot), Sec 8 (MenuSelect/MenuKey).
 *      os/flair/finder_cmd.h Sec 4 (the exported predicates -- ONE source of
 *        truth for graying, per F4.4).
 *      spec/flair_tenants_demo.h (the occupied menu-id ranges).
 *      CLAUDE.md Law 1, Law 2 (the oracle hand-authors the expected structure;
 *        this file is never read back into it), Law 3 (freestanding artifact,
 *        dual-compile), Law 4, Rule 8, Rule 11, Rule 12 (ASCII-clean).
 *
 * ARTIFACT code: freestanding C (ADR-0002). No libc, no malloc. Compiles BOTH
 * under the kernel flags AND hosted for harness/proptest/test_finder_menu.c.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_FINDER_MENU_H
#define INITECH_OS_FLAIR_FINDER_MENU_H

#include <stdint.h>

#include "menu.h"          /* MenuBar / MenuInfo / MenuItem (-Ios/flair)      */
#include "finder_cmd.h"    /* FinderCtx + the F4.4 predicates                 */

/* ===========================================================================
 * 1. THE MENU IDS  (design F4-1: 512..516, disjoint from 128..131 / 256..263 /
 *    384..386 -- spec/flair_tenants_demo.h documents the occupied ranges)
 * ===========================================================================*/
#define FINDER_MENU_ID_APPLE     0     /* the has_apple slot entries (F4.4)   */
#define FINDER_MENU_ID_FILE    512
#define FINDER_MENU_ID_EDIT    513
#define FINDER_MENU_ID_VIEW    514
#define FINDER_MENU_ID_SPECIAL 515
#define FINDER_MENU_ID_HELP    516

/* The number of TITLED menus in the bar (Apple is the glyph slot, not a title;
 * see the banner). File, Edit, View, Special, Help. */
#define FINDER_MENU_COUNT        5

/* Bar-index constants so callers and the oracle name the same slots. */
#define FINDER_MENU_IX_FILE      0
#define FINDER_MENU_IX_EDIT      1
#define FINDER_MENU_IX_VIEW      2
#define FINDER_MENU_IX_SPECIAL   3
#define FINDER_MENU_IX_HELP      4

/* The bar titles, named so the oracle and the pixel grader cite ONE spelling.
 * Rightmost Help is period-GUI-suite Sec 2a, cited by F4.2. */
#define FINDER_MENU_TITLE_FILE     "File"
#define FINDER_MENU_TITLE_EDIT     "Edit"
#define FINDER_MENU_TITLE_VIEW     "View"
#define FINDER_MENU_TITLE_SPECIAL  "Special"
#define FINDER_MENU_TITLE_HELP     "Help"

/*
 * The MARK character for the checked view mode (F4.2 View row 1, "by Icons
 * (mark char)"). The hand-authored Chicago strike spans 0x20..0x7A
 * (spec/assets/chicago8x16.h CHICAGO_FIRST/CHICAGO_LAST) and carries NO
 * check-mark glyph -- the period MacRoman check is 0x12, outside it. Rather
 * than invent a strike here (CLAUDE.md: "Glyphs are hand-authored, not
 * pixel-extracted" -- authoring one is a deliberate act, not a side effect of
 * a menu resource) or mark the item with a character that draws nothing, the
 * resource uses an ASCII SUBSTITUTION, exactly the posture menu.h already
 * takes for the command-key column (FLAIR_MENU_CMD_CHARS: "period-plausible
 * '^X' substitution"). The Platinum check glyph is bead initech-sjvq's ink.
 */
#define FINDER_MENU_MARK_CHECK   'v'

/* ===========================================================================
 * 2. THE RESOURCE
 * ===========================================================================*/

/*
 * finder_menu_bar -- THE Finder's MenuBar (has_apple = 1; five titled menus).
 * A singleton over this file's static storage: the SAME pointer every call, so
 * kmain can hand it to FlairApp.menubar once at launch and menu.c's cached
 * menuWidth survives.
 */
MenuBar *finder_menu_bar(void);

/*
 * finder_menu_apple -- the Apple-slot menu as DATA (menu id 0). NOT part of
 * finder_menu_bar()'s menus[] array; see the banner for why. Exported so the
 * host oracle can grade its rows and so the bead that teaches menu.c to drop
 * the Apple slot has something to drop.
 */
MenuInfo *finder_menu_apple(void);

/*
 * finder_menu_refresh_enables -- recompute the PREDICATE-DRIVEN enable bytes.
 *
 * F4.4: "Recomputed into the static MenuItem.enabled bytes before every
 * DrawMenuBar/track -- the menu resource stays static data; only the enable
 * bits mutate."
 *
 * It touches ONLY the items this resource declares predicate-driven (Open,
 * Close Window, Get Info, Duplicate, Empty Trash -- the F4.4 predicate list).
 * Every other byte is authored once in finder_menu.c and left alone:
 *   - the permanently-inert V1 decoration stays 0 (Print, Make Alias, Put
 *     Away, Find, Page Setup, Sharing, Eject, the Edit scrap rows, the eight
 *     non-icon view modes, Erase Disk, Sleep, both Help rows, About This
 *     Computer),
 *   - the always-live commands stay 1 (New Folder, Select All, by Icons,
 *     Clean Up in both menus, Arrange, Restart, Shut Down).
 * A NULL fx evaluates every predicate against NULL, which the finder_cmd
 * predicates answer with 0 (finder_cmd.c) -- i.e. "no context, nothing live",
 * which is the safe direction (Rule 2).
 */
void finder_menu_refresh_enables(const FinderCtx *fx);

#endif /* INITECH_OS_FLAIR_FINDER_MENU_H */

/*
 * os/flair/finder_menu.c -- THE FINDER MENU BAR RESOURCE (THE ARTIFACT).
 *
 * beads: initech-tdnl.12 (GUI remediation R3.5, stage 1). See
 * os/flair/finder_menu.h for the full banner: the F4.2 layout as static data,
 * the Apple-slot ruling, the omit-not-gray application to Calculator/Note Pad,
 * and the one refresh routine.
 *
 * EVERY ROW BELOW IS READ OFF design F4.2 IN ORDER. The item numbers the
 * command table (os/flair/finder_cmd.c, its own F4.2 transcription) uses are
 * 1-BASED and COUNT DIVIDERS, so the k-th entry of an items[] array here is
 * item k+1 there. The two transcriptions are INDEPENDENT and the host oracle
 * (harness/proptest/test_finder_menu.c) grades a THIRD, hand-authored one
 * against both -- a real differential, never agreement by construction
 * (Law 2 / HER-02).
 *
 * MUTANT KNOBS (Rule 6; -D on this TU only, from the Makefile):
 *   FINDER_MENU_MUT_ENABLE_STUCK -- refresh writes 1 for every predicate-driven
 *                                   item instead of the predicate's answer, so
 *                                   nothing ever grays. The truth table goes
 *                                   RED (this is design F4.4's PRED_STUCK_
 *                                   ENABLED applied to the RESOURCE half).
 *   FINDER_MENU_MUT_CMDCHAR_DUP  -- File > Open takes 'N', duplicating New
 *                                   Folder's command key. The structural
 *                                   no-duplicate-cmdChar leg goes RED, and so
 *                                   does MenuKey routing (menu.c returns the
 *                                   FIRST enabled match, so Cmd-N would still
 *                                   be New Folder but Cmd-O would find nothing).
 *   FINDER_MENU_MUT_DEAD_ITEM    -- Special > Clean Up ships DEAD: its enable
 *                                   byte is 0 and no predicate ever revives it,
 *                                   so menu.c's MenuInfo_item_selectable
 *                                   refuses it, MenuSelect returns 0 and the
 *                                   command can never be dispatched from the
 *                                   bar. THE EMU MUTANT of bead initech-tdnl.12
 *                                   (also bites the host structure leg): the
 *                                   booted Special > Clean Up gesture loses its
 *                                   FINDER-CMD id=9 line entirely -- a menu
 *                                   item that draws perfectly and does nothing,
 *                                   which is exactly the failure a human
 *                                   looking at the frame cannot see.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.2 (the layout),
 *        F4.3 (omit-not-gray), F4.4 (predicates + the command table).
 *      os/flair/menu.h Sec 3 (MenuItem field order: text, mark, cmdChar,
 *        style, enabled, is_divider).
 *      os/flair/finder_cmd.h Sec 4 (the exported predicates).
 *      CLAUDE.md Law 1, Law 2, Law 3, Rule 8, Rule 11, Rule 12.
 *
 * FREESTANDING (Law 3): includes only finder_menu.h (menu.h + finder_cmd.h).
 * No libc, no malloc, no static initialiser that is not a constant expression.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include "finder_menu.h"

/* Shorthand for the two constant MenuItem shapes this file repeats, so the
 * tables below read as the F4.2 lists they transcribe rather than as a wall of
 * braces. Field order is menu.h Sec 3: text, mark, cmdChar, style, enabled,
 * is_divider.
 *   FM_ITEM(text, cmd, on) -- an ordinary row.
 *   FM_DIV                 -- a separator row (never selectable, menu.c).      */
#define FM_ITEM(txt, cmd, on)   { (txt), 0, (char)(cmd), 0u, (uint8_t)(on), 0u }
#define FM_MARKED(txt, cmd, on) { (txt), FINDER_MENU_MARK_CHECK, (char)(cmd), \
                                  0u, (uint8_t)(on), 0u }
#define FM_DIV                  { "-", 0, 0, 0u, 0u, 1u }

/* The V1 rest state of a PREDICATE-DRIVEN item. It is authored 0 and then
 * overwritten by finder_menu_refresh_enables before every draw/track; 0 is the
 * honest boot value because at boot there is no selection and no disk window
 * front, so every one of the five predicates answers 0 anyway. */
#define FM_PRED_REST  0

/* ===========================================================================
 * THE APPLE SLOT  (menu id 0; F4.2 Apple table)
 * ---------------------------------------------------------------------------
 *   1  About This Computer   -- "grayed until R4.1 (serial marker on attempt)"
 *   -- the divider and the two launchables (Calculator, Note Pad) are OMITTED
 *      THIS STAGE under ruling F4-3; see finder_menu.h. They return with bead
 *      initech-tdnl.14, restoring items 2/3/4 that finder_cmd.c's Apple rows
 *      already name.
 *
 * About This Computer is authored 0 (grayed), not 1, even though its
 * finder_cmd row carries no predicate: F4.2 says "grayed until R4.1"
 * explicitly, and a specific ruling beats the general "no predicate == always
 * live" shape. It is not predicate-driven, so refresh never touches it.
 * ===========================================================================*/
static MenuItem finder_apple_items[] = {
    FM_ITEM("About This Computer", 0, 0)
};

static MenuInfo finder_apple_menu = {
    (int16_t)FINDER_MENU_ID_APPLE, "", finder_apple_items, 1u, 0
};

/* ===========================================================================
 * FILE  (512)  -- F4.2: New Folder N | Open O | Print (grayed) | Close Window W
 *   | div | Get Info I | Duplicate D | Make Alias (grayed) | Put Away Y
 *   (grayed) | div | Find F (grayed) | Page Setup (grayed) | Sharing (grayed)
 *   | Label (OMITTED -- no submenu support, D3-46) | Eject (grayed V1)
 *
 * Put Away is authored 0 and is NOT predicate-driven. os/flair/finder_cmd.c
 * says so at its row: "Put Away's V1 rest-graying (F4.2) is a menu-resource
 * enabled byte owned by tdnl.12; the SEMANTIC predicate -- the one dispatch
 * enforces -- is 'there is something selected to put away'." Both halves are
 * therefore true at once: the RESOURCE keeps it dark in V1, and if some future
 * route reaches the command anyway, dispatch still gates it on a selection.
 * ===========================================================================*/
static MenuItem finder_file_items[] = {
    FM_ITEM("New Folder",   'N', 1),              /* FCMD_NEW_FOLDER  (always) */
#ifndef FINDER_MENU_MUT_CMDCHAR_DUP
    FM_ITEM("Open",         'O', FM_PRED_REST),   /* FCMD_OPEN  has_selection  */
#else
    /* MUTANT FINDER_MENU_MUT_CMDCHAR_DUP (Rule 6): Open steals New Folder's
     * command key. Two items with cmdChar 'N' can be enabled at once, which is
     * exactly the structural property the oracle forbids. NEVER in a real
     * build. */
    FM_ITEM("Open",         'N', FM_PRED_REST),
#endif
    FM_ITEM("Print",          0, 0),              /* inert V1 decoration       */
    FM_ITEM("Close Window", 'W', FM_PRED_REST),   /* FCMD_CLOSE_WINDOW  front  */
    FM_DIV,
    FM_ITEM("Get Info",     'I', FM_PRED_REST),   /* FCMD_GET_INFO  has_sel    */
    FM_ITEM("Duplicate",    'D', FM_PRED_REST),   /* FCMD_DUPLICATE has_sel    */
    FM_ITEM("Make Alias",     0, 0),              /* inert V1 decoration       */
    FM_ITEM("Put Away",     'Y', 0),              /* grayed V1 (F4.2, F1-5)    */
    FM_DIV,
    FM_ITEM("Find",         'F', 0),              /* inert V1 decoration       */
    FM_ITEM("Page Setup",     0, 0),
    FM_ITEM("Sharing",        0, 0),
    FM_ITEM("Eject",          0, 0)
};

/* ===========================================================================
 * EDIT  (513)  -- F4.2: Undo Z (grayed) | Cut X / Copy C / Paste V (grayed
 *   until the R4.2 scrap wiring, ww9c) | Clear (grayed) | Select All A
 *   (FUNCTIONAL) | div | Show Clipboard (grayed until R4) | div | Preferences
 *   (grayed, bottom per the Mac OS 8 HIG standardization)
 * ===========================================================================*/
static MenuItem finder_edit_items[] = {
    FM_ITEM("Undo",           'Z', 0),
    FM_ITEM("Cut",            'X', 0),
    FM_ITEM("Copy",           'C', 0),
    FM_ITEM("Paste",          'V', 0),
    FM_ITEM("Clear",            0, 0),
    FM_ITEM("Select All",     'A', 1),            /* FCMD_SELECT_ALL (always)  */
    FM_DIV,
    FM_ITEM("Show Clipboard",   0, 0),
    FM_DIV,
    FM_ITEM("Preferences",      0, 0)
};

/* ===========================================================================
 * VIEW  (514)  -- F4.2: by Icons (mark char, FUNCTIONAL) | by Small Icon
 *   (grayed) | by Name/Size/Kind/Label/Date (grayed -- list view deferred per
 *   plan R3.3) | as Buttons (grayed) | as Pop-up Window (grayed) | div |
 *   Clean Up (FUNCTIONAL) | Arrange (by Name) (FUNCTIONAL)
 * ===========================================================================*/
static MenuItem finder_view_items[] = {
    FM_MARKED("by Icons",         0, 1),          /* FCMD_VIEW_ICONS (always)  */
    FM_ITEM("by Small Icon",      0, 0),
    FM_ITEM("by Name",            0, 0),
    FM_ITEM("by Size",            0, 0),
    FM_ITEM("by Kind",            0, 0),
    FM_ITEM("by Label",           0, 0),
    FM_ITEM("by Date",            0, 0),
    FM_ITEM("as Buttons",         0, 0),
    FM_ITEM("as Pop-up Window",   0, 0),
    FM_DIV,
    FM_ITEM("Clean Up",           0, 1),          /* FCMD_CLEANUP    (always)  */
    FM_ITEM("Arrange (by Name)",  0, 1)           /* FCMD_ARRANGE_BY_NAME      */
};

/* ===========================================================================
 * SPECIAL  (515)  -- F4.2: Clean Up (FUNCTIONAL, same cmd id as View's) |
 *   Empty Trash (FUNCTIONAL, enabled iff Trash non-empty) | div | Erase Disk
 *   (grayed) | div | Restart (FUNCTIONAL) | Shut Down (FUNCTIONAL) | Sleep
 *   (grayed)
 * ===========================================================================*/
static MenuItem finder_special_items[] = {
#ifndef FINDER_MENU_MUT_DEAD_ITEM
    FM_ITEM("Clean Up",     0, 1),                /* FCMD_CLEANUP   (always)   */
#else
    /* MUTANT FINDER_MENU_MUT_DEAD_ITEM (Rule 6): the row is present and draws,
     * but its enable byte is 0 forever, so it can never be chosen. NEVER in a
     * real build. */
    FM_ITEM("Clean Up",     0, 0),
#endif
    FM_ITEM("Empty Trash",  0, FM_PRED_REST),     /* FCMD_EMPTY_TRASH  trash   */
    FM_DIV,
    FM_ITEM("Erase Disk",   0, 0),
    FM_DIV,
    FM_ITEM("Restart",      0, 1),                /* FCMD_RESTART   (always)   */
    FM_ITEM("Shut Down",    0, 1),                /* FCMD_SHUTDOWN  (always)   */
    FM_ITEM("Sleep",        0, 0)
};

/* ===========================================================================
 * HELP  (516, rightmost)  -- F4.2: About Help (grayed V1) | Show Balloons
 *   (grayed). BOTH rows are inert decoration in V1; neither carries a
 *   finder_cmd row, so a route to either would be FINDER-CMD-UNKNOWN -- which
 *   is the fail-loud posture, not a silent swallow (Rule 2).
 * ===========================================================================*/
static MenuItem finder_help_items[] = {
    FM_ITEM("About Help",    0, 0),
    FM_ITEM("Show Balloons", 0, 0)
};

/* The five TITLED menus, in bar order (Help rightmost, F4.2). menuWidth 0 ==
 * "not yet computed"; menu.c caches into it. */
static MenuInfo finder_menus[FINDER_MENU_COUNT] = {
    { (int16_t)FINDER_MENU_ID_FILE,    FINDER_MENU_TITLE_FILE,
      finder_file_items,
      (uint16_t)(sizeof finder_file_items / sizeof finder_file_items[0]), 0 },
    { (int16_t)FINDER_MENU_ID_EDIT,    FINDER_MENU_TITLE_EDIT,
      finder_edit_items,
      (uint16_t)(sizeof finder_edit_items / sizeof finder_edit_items[0]), 0 },
    { (int16_t)FINDER_MENU_ID_VIEW,    FINDER_MENU_TITLE_VIEW,
      finder_view_items,
      (uint16_t)(sizeof finder_view_items / sizeof finder_view_items[0]), 0 },
    { (int16_t)FINDER_MENU_ID_SPECIAL, FINDER_MENU_TITLE_SPECIAL,
      finder_special_items,
      (uint16_t)(sizeof finder_special_items / sizeof finder_special_items[0]), 0 },
    { (int16_t)FINDER_MENU_ID_HELP,    FINDER_MENU_TITLE_HELP,
      finder_help_items,
      (uint16_t)(sizeof finder_help_items / sizeof finder_help_items[0]), 0 }
};

/* has_apple = 1 (F4.2 "Apple (has_apple slot)"). The glyph is drawn by
 * DrawMenuBar; the Apple MENU's items live in finder_apple_menu (banner). */
static MenuBar finder_bar = { finder_menus, (uint16_t)FINDER_MENU_COUNT, 1u };

/* ===========================================================================
 * THE PREDICATE MAP  (design F4.4's predicate list, one row per graying item)
 * ---------------------------------------------------------------------------
 * (bar-menu index, 0-based item index, predicate). Authored HERE against the
 * F4.2 layout, using the predicate functions os/flair/finder_cmd.c exports so
 * the bar grays by EXACTLY the rule dispatch enforces (finder_cmd.h Sec 4:
 * "one source of truth for graying").
 *
 * The 1-based item numbers in the comments are the ones finder_cmd.c's rows
 * carry, so a drift between the two transcriptions is visible by eye here and
 * caught mechanically by the oracle.
 * ===========================================================================*/
typedef struct fm_pred_row {
    uint8_t             menu_ix;   /* index into finder_menus[]                */
    uint8_t             item_ix;   /* 0-based index into that menu's items[]   */
    finder_cmd_pred_fn  pred;      /* the F4.4 predicate                       */
} fm_pred_row_t;

static const fm_pred_row_t FINDER_MENU_PREDS[] = {
    { FINDER_MENU_IX_FILE,    1u, finder_pred_has_selection    }, /* Open   #2 */
    { FINDER_MENU_IX_FILE,    3u, finder_pred_front_is_diskwin }, /* Close  #4 */
    { FINDER_MENU_IX_FILE,    5u, finder_pred_has_selection    }, /* Info   #6 */
    { FINDER_MENU_IX_FILE,    6u, finder_pred_has_selection    }, /* Dupl   #7 */
    { FINDER_MENU_IX_SPECIAL, 1u, finder_pred_trash_nonempty   }  /* Trash  #2 */
};

static const uint16_t FINDER_MENU_PREDS_N =
    (uint16_t)(sizeof FINDER_MENU_PREDS / sizeof FINDER_MENU_PREDS[0]);

/* ===========================================================================
 * THE API
 * ===========================================================================*/

MenuBar *finder_menu_bar(void)
{
    return &finder_bar;
}

MenuInfo *finder_menu_apple(void)
{
    return &finder_apple_menu;
}

void finder_menu_refresh_enables(const FinderCtx *fx)
{
    uint16_t i;

    for (i = 0; i < FINDER_MENU_PREDS_N; i++) {
        const fm_pred_row_t *r = &FINDER_MENU_PREDS[i];
        /* The MenuInfo's items[] is a const pointer (menu.h Sec 3) because the
         * Menu Manager never writes it; the STORAGE is this file's own mutable
         * array, so the resource writes its own bytes through it. Casting away
         * the const on a pointer to genuinely-mutable storage is defined, and
         * it keeps the Menu Manager's read-only view intact for everyone else. */
        MenuItem *items = (MenuItem *)finder_menus[r->menu_ix].items;
#ifndef FINDER_MENU_MUT_ENABLE_STUCK
        items[r->item_ix].enabled = (uint8_t)(r->pred(fx) ? 1u : 0u);
#else
        /* MUTANT FINDER_MENU_MUT_ENABLE_STUCK (Rule 6; design F4.4
         * PRED_STUCK_ENABLED applied to the resource half): every
         * predicate-driven row reads LIVE, so the bar never grays -- Open with
         * no selection, Close Window with no window, Empty Trash on an empty
         * Trash all look selectable and MenuKey hands them out. NEVER in a
         * real build. */
        (void)fx;
        items[r->item_ix].enabled = 1u;
#endif
    }
}

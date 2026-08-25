/* test_finder_menu.c -- the HOST oracle for THE FINDER MENU BAR RESOURCE.
 *
 * beads: initech-tdnl.12 (GUI remediation R3.5 "the real Finder menu bar",
 *        stage 1; docs/design/GUI-remediation-R3-finder-design.md F4.1-F4.4).
 *
 * WHAT IT GRADES: os/flair/finder_menu.c -- the EXACT arrays the live Finder
 * bar is drawn from and the EXACT refresh routine the pump calls before every
 * DrawMenuBar/track, plus the routing those arrays produce through the LOCKED
 * Menu Manager (os/flair/menu.c :: MenuKey / MenuInfo_item_selectable) and the
 * LOCKED command table (os/flair/finder_cmd.c).
 *
 * HAND-AUTHORED GOLDEN (Law 2 / Revocation Record HER-02). Every expectation
 * below is TYPED OUT BY HAND from design F4.2 -- the menu order, the ids, the
 * per-item text, the command keys, the divider positions, the rest-state
 * enable bytes, the enable vectors per FinderCtx state, and the bar's title
 * x-positions. NONE of it is read back off the resource. An oracle that asked
 * finder_menu.c what finder_menu.c contains would agree by construction and is
 * forbidden.
 *
 * THREE INDEPENDENT TRANSCRIPTIONS, GRADED AGAINST EACH OTHER (the point of
 * this file):
 *   (1) os/flair/finder_menu.c   -- the RESOURCE (item order, text, cmd keys)
 *   (2) os/flair/finder_cmd.c    -- the COMMAND TABLE (1-based item numbers)
 *   (3) THIS FILE                -- the design F4.2 layout, typed fresh
 * Leg E walks (1) against (2): every selectable row of the resource must
 * resolve to a table row, and every table row of menus 512..516 must name a
 * real, non-divider row of the resource. A drift in either transcription --
 * an inserted item, a moved divider, a renumbered table row -- is RED.
 *
 * THE TITLE-LAYOUT LEG IS LOAD-BEARING BEYOND THIS FILE (leg B). The booted
 * band-2 pixel probe in tools/ppm_flair_disk_windows_check.c (leg finderbar)
 * probes HAND-DERIVED columns; those columns are derived by the SAME arithmetic
 * asserted here, so if menu.h's padding or the Chicago cell width ever moves,
 * this host gate goes RED first and loudly, instead of the emulator gate going
 * mysteriously red an hour later.
 *
 * MUTATION-PROVEN (Rule 6). The mutants are -D knobs on the IMPLEMENTATION TU
 * (os/flair/finder_menu.c), never on this golden:
 *   FINDER_MENU_MUT_ENABLE_STUCK -- refresh never grays: leg C's truth table
 *     and leg D's disabled-cmdChar rows go RED.
 *   FINDER_MENU_MUT_CMDCHAR_DUP  -- File > Open takes New Folder's 'N': leg A's
 *     hand-authored cmdChar column, leg D's Cmd-O routing and leg F's
 *     no-duplicate-among-enabled structural check all go RED.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.1 (band 2, ids
 *        512..516), F4.2 (THE layout), F4.3 (omit-not-gray), F4.4 (the
 *        predicates + the command table + the both-paths-converge rule).
 *      os/flair/menu.h Sec 3/4/5/8 (records, result packing, bar layout,
 *        MenuKey's "FIRST ENABLED, non-divider item whose cmdChar matches").
 *      os/flair/finder_menu.h, os/flair/finder_cmd.h.
 *      CLAUDE.md Law 1, Law 2, Rule 6, Rule 11, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "menu.h"           /* MenuBar/MenuInfo/MenuItem + MenuKey (-Ios/flair) */
#include "finder_menu.h"    /* THE resource under test                          */
#include "finder_cmd.h"     /* the command table the resource must agree with    */
#include "test_assert.h"    /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)          */

TEST_HARNESS();

/* ===========================================================================
 * THE HAND-AUTHORED F4.2 LAYOUT  (typed from the design, not read back)
 * ---------------------------------------------------------------------------
 * `en` is the REST-STATE enabled byte, i.e. what the resource must hold with
 * no refresh applied (and, for the five predicate-driven rows, what refresh
 * must write back for the all-zero FinderCtx). `div` marks a separator row.
 * ===========================================================================*/
typedef struct want_item {
    const char *text;
    char        cmd;
    char        mark;
    int         div;
    int         en;
} want_item_t;

/* Apple slot, menu id 0 (F4.2). THIS STAGE: About This Computer alone -- the
 * divider + Calculator + Note Pad are omitted under ruling F4-3 because no
 * application exists on the volume until bead initech-tdnl.14. Typed here so
 * that when tdnl.14 restores them, THIS golden is the thing that must be
 * deliberately edited (Rule 8), not a silently-passing test. */
static const want_item_t WANT_APPLE[] = {
    { "About This Computer", 0, 0, 0, 0 }
};

/* File, 512 (F4.2). Label is omitted -- no submenu support (D3-46). */
static const want_item_t WANT_FILE[] = {
    { "New Folder",   'N', 0, 0, 1 },
    { "Open",         'O', 0, 0, 0 },
    { "Print",          0, 0, 0, 0 },
    { "Close Window", 'W', 0, 0, 0 },
    { "-",              0, 0, 1, 0 },
    { "Get Info",     'I', 0, 0, 0 },
    { "Duplicate",    'D', 0, 0, 0 },
    { "Make Alias",     0, 0, 0, 0 },
    { "Put Away",     'Y', 0, 0, 0 },
    { "-",              0, 0, 1, 0 },
    { "Find",         'F', 0, 0, 0 },
    { "Page Setup",     0, 0, 0, 0 },
    { "Sharing",        0, 0, 0, 0 },
    { "Eject",          0, 0, 0, 0 }
};

/* Edit, 513 (F4.2). Preferences sits at the BOTTOM (Mac OS 8 HIG). */
static const want_item_t WANT_EDIT[] = {
    { "Undo",           'Z', 0, 0, 0 },
    { "Cut",            'X', 0, 0, 0 },
    { "Copy",           'C', 0, 0, 0 },
    { "Paste",          'V', 0, 0, 0 },
    { "Clear",            0, 0, 0, 0 },
    { "Select All",     'A', 0, 0, 1 },
    { "-",                0, 0, 1, 0 },
    { "Show Clipboard",   0, 0, 0, 0 },
    { "-",                0, 0, 1, 0 },
    { "Preferences",      0, 0, 0, 0 }
};

/* View, 514 (F4.2). by Icons carries the mark char. */
static const want_item_t WANT_VIEW[] = {
    { "by Icons",          0, FINDER_MENU_MARK_CHECK, 0, 1 },
    { "by Small Icon",     0, 0, 0, 0 },
    { "by Name",           0, 0, 0, 0 },
    { "by Size",           0, 0, 0, 0 },
    { "by Kind",           0, 0, 0, 0 },
    { "by Label",          0, 0, 0, 0 },
    { "by Date",           0, 0, 0, 0 },
    { "as Buttons",        0, 0, 0, 0 },
    { "as Pop-up Window",  0, 0, 0, 0 },
    { "-",                 0, 0, 1, 0 },
    { "Clean Up",          0, 0, 0, 1 },
    { "Arrange (by Name)", 0, 0, 0, 1 }
};

/* Special, 515 (F4.2). */
static const want_item_t WANT_SPECIAL[] = {
    { "Clean Up",    0, 0, 0, 1 },
    { "Empty Trash", 0, 0, 0, 0 },
    { "-",           0, 0, 1, 0 },
    { "Erase Disk",  0, 0, 0, 0 },
    { "-",           0, 0, 1, 0 },
    { "Restart",     0, 0, 0, 1 },
    { "Shut Down",   0, 0, 0, 1 },
    { "Sleep",       0, 0, 0, 0 }
};

/* Help, 516 (F4.2) -- rightmost; BOTH rows inert decoration in V1. */
static const want_item_t WANT_HELP[] = {
    { "About Help",    0, 0, 0, 0 },
    { "Show Balloons", 0, 0, 0, 0 }
};

typedef struct want_menu {
    int16_t            id;
    const char        *title;
    const want_item_t *items;
    int                n;
} want_menu_t;

#define WN(a) ((int)(sizeof (a) / sizeof (a)[0]))

static const want_menu_t WANT_BAR[] = {
    { 512, "File",    WANT_FILE,    WN(WANT_FILE)    },
    { 513, "Edit",    WANT_EDIT,    WN(WANT_EDIT)    },
    { 514, "View",    WANT_VIEW,    WN(WANT_VIEW)    },
    { 515, "Special", WANT_SPECIAL, WN(WANT_SPECIAL) },
    { 516, "Help",    WANT_HELP,    WN(WANT_HELP)    }
};

/* ===========================================================================
 * A -- STRUCTURE: the resource IS the F4.2 layout
 * ===========================================================================*/
static void leg_structure(void)
{
    MenuBar  *bar = finder_menu_bar();
    MenuInfo *ap  = finder_menu_apple();
    int mi, k;

    CHECK(bar != NULL && ap != NULL, "A the resource exposes a bar and an Apple menu");
    CHECK(bar == finder_menu_bar(),
          "A the bar is a SINGLETON (kmain binds it once; menu.c caches into it)");
    CHECK(bar->has_apple == 1u,
          "A has_apple == 1 -- the Apple glyph slot at the far left (F4.2)");
    CHECK(bar->n_menus == (uint16_t)WN(WANT_BAR),
          "A five TITLED menus: File Edit View Special Help (Apple is the glyph slot)");
    CHECK(bar->n_menus == (uint16_t)FINDER_MENU_COUNT,
          "A ... and FINDER_MENU_COUNT agrees");

    for (mi = 0; mi < WN(WANT_BAR); mi++) {
        const want_menu_t *wm = &WANT_BAR[mi];
        const MenuInfo    *m  = &bar->menus[mi];

        CHECK(m->menuID == wm->id, "A menu id (F4-1: 512..516)");
        CHECK(m->title != NULL && strcmp(m->title, wm->title) == 0,
              "A menu title in bar order (Help rightmost)");
        CHECK(m->n_items == (uint16_t)wm->n,
              "A item count -- an inserted or dropped row is RED here");
        if (m->n_items != (uint16_t)wm->n) continue;

        for (k = 0; k < wm->n; k++) {
            const want_item_t *wi = &wm->items[k];
            const MenuItem    *it = &m->items[k];
            CHECK(it->is_divider == (uint8_t)wi->div,
                  "A divider position (item numbering COUNTS dividers)");
            if (!wi->div) {
                CHECK(it->text != NULL && strcmp(it->text, wi->text) == 0,
                      "A item text");
            }
            CHECK(it->cmdChar == wi->cmd, "A item command key (F4.2)");
            CHECK(it->mark == wi->mark, "A item mark char");
            CHECK(it->enabled == (uint8_t)wi->en,
                  "A REST-STATE enabled byte (grayed V1 decoration stays 0)");
            CHECK(it->style == 0u, "A no style bits in the V1 resource");
        }
    }

    /* The Apple slot as DATA (finder_menu.h banner: it is deliberately NOT in
     * bar->menus[], because MenuBar_hit returns -1 inside the glyph slot). */
    CHECK(ap->menuID == (int16_t)FINDER_MENU_ID_APPLE,
          "A the Apple menu carries id 0 (F4.4 'menu_id 0 == Apple-slot entry')");
    CHECK(ap->n_items == (uint16_t)WN(WANT_APPLE),
          "A the Apple menu ships ONLY About This Computer this stage (F4-3: "
          "Calculator/Note Pad are omit-if-not-installed until tdnl.14)");
    for (k = 0; k < WN(WANT_APPLE) && k < (int)ap->n_items; k++) {
        CHECK(strcmp(ap->items[k].text, WANT_APPLE[k].text) == 0,
              "A Apple item text");
        CHECK(ap->items[k].enabled == (uint8_t)WANT_APPLE[k].en,
              "A Apple item enabled byte (About This Computer grayed until R4.1)");
    }
    for (mi = 0; mi < (int)bar->n_menus; mi++)
        CHECK(bar->menus[mi].menuID != (int16_t)FINDER_MENU_ID_APPLE,
              "A the Apple menu is NOT one of the bar's titled menus");
}

/* ===========================================================================
 * B -- BAR LAYOUT: the hand-derived title columns the emu probe uses
 * ---------------------------------------------------------------------------
 * menu.h Sec 5: the first title starts at FLAIR_MENU_APPLE_W (has_apple), and
 * each title slot is text_measure + 2*FLAIR_MENU_TITLE_PAD.
 * spec/assets/chicago8x16.h is a FIXED 8px cell, so text_measure is 8*len --
 * pinned below so a move to a proportional strike is RED here first.
 *
 *   APPLE_W = 20, PAD = 7, cell = 8
 *   File    len 4 -> w 4*8 + 14 = 46 -> x  20 .. 66
 *   Edit    len 4 -> w 46           -> x  66 .. 112
 *   View    len 4 -> w 46           -> x 112 .. 158
 *   Special len 7 -> w 7*8 + 14 = 70-> x 158 .. 228
 *   Help    len 4 -> w 46           -> x 228 .. 274
 * ===========================================================================*/
typedef struct want_slot { int x, w; } want_slot_t;

static const want_slot_t WANT_SLOTS[] = {
    {  20, 46 },   /* File    */
    {  66, 46 },   /* Edit    */
    { 112, 46 },   /* View    */
    { 158, 70 },   /* Special */
    { 228, 46 }    /* Help    */
};

/* The first column PAST the last Finder title. The booted band-2 probe uses
 * this to tell the Finder bar from the Photoshop bar, whose titles ("File Edit
 * Image Layer Select View Window Help") run far past it. */
#define FINDER_BAR_INK_END   274

static void leg_layout(void)
{
    MenuBar *bar = finder_menu_bar();
    int i;

    CHECK(FLAIR_MENU_APPLE_W == 20, "B the Apple slot is 20px (menu.h Sec 5)");
    CHECK(FLAIR_MENU_TITLE_PAD == 7, "B the title side pad is 7px (menu.h Sec 5)");
    CHECK(text_measure(FONT_CHICAGO, "File") == 32,
          "B Chicago is a fixed 8px cell -- the arithmetic below assumes it");
    CHECK(text_measure(FONT_CHICAGO, "Special") == 56, "B ... for 7 glyphs too");

    for (i = 0; i < WN(WANT_SLOTS); i++) {
        CHECK(MenuBar_title_x(bar, i) == WANT_SLOTS[i].x,
              "B hand-derived title LEFT (the emu band-2 probe column)");
        CHECK(MenuBar_title_w(bar, i) == WANT_SLOTS[i].w,
              "B hand-derived title WIDTH");
        /* Hit-testing the slot's own middle finds that menu, and only it. */
        CHECK(MenuBar_hit(bar, WANT_SLOTS[i].x + WANT_SLOTS[i].w / 2) == i,
              "B a click in the slot hits THAT title");
    }
    CHECK(MenuBar_title_x(bar, WN(WANT_SLOTS) - 1) +
          MenuBar_title_w(bar, WN(WANT_SLOTS) - 1) == FINDER_BAR_INK_END,
          "B the Finder bar's title run ends at x=274 -- the discriminator the "
          "booted probe uses against the Photoshop bar");
    CHECK(MenuBar_hit(bar, FLAIR_MENU_APPLE_W - 1) < 0,
          "B the Apple GLYPH SLOT is not a titled menu (menu.h Sec 5) -- which "
          "is why the Apple menu ships as standalone data");
    CHECK(MenuBar_hit(bar, FINDER_BAR_INK_END) < 0,
          "B past the last title the bar is bare furniture");
}

/* ===========================================================================
 * C -- REFRESH: the enable truth table over FinderCtx states
 * ---------------------------------------------------------------------------
 * F4.4's predicate list, applied to the five predicate-driven rows:
 *   File #2  Open          HAS_SELECTION
 *   File #4  Close Window  WINDOW_FRONT_IS_DISKWIN
 *   File #6  Get Info      HAS_SELECTION
 *   File #7  Duplicate     HAS_SELECTION
 *   Special #2 Empty Trash TRASH_NONEMPTY
 * Every other byte must be INVARIANT across refreshes -- that is what "the
 * menu resource stays static data; only the enable bits mutate" means, and it
 * is checked explicitly below (a refresh that rewrote Put Away or Restart
 * would be a silent policy change).
 * ===========================================================================*/
typedef struct want_state {
    uint16_t sel;          /* FinderCtx.selection_count                         */
    uint8_t  trash;        /* FinderCtx.trash_nonempty                          */
    uint8_t  front;        /* FinderCtx.front_is_diskwin                        */
    int      open_en, close_en, info_en, dup_en, trash_en;   /* hand-authored   */
    const char *what;
} want_state_t;

static const want_state_t WANT_STATES[] = {
    /* sel tr fr    Open Close Info Dupl Trash */
    {  0,  0, 0,     0,   0,    0,   0,   0, "C boot: nothing selected, desktop front, Trash empty" },
    {  1,  0, 0,     1,   0,    1,   1,   0, "C one icon selected on the desktop" },
    {  0,  0, 1,     0,   1,    0,   0,   0, "C a disk window front, nothing selected" },
    {  3,  0, 1,     1,   1,    1,   1,   0, "C a disk window front WITH a selection" },
    {  0,  1, 0,     0,   0,    0,   0,   1, "C Trash non-empty is the ONLY thing that lights Empty Trash" },
    {  2,  1, 1,     1,   1,    1,   1,   1, "C everything live" }
};

/* The rows refresh must NEVER touch, with their authored bytes. */
typedef struct want_fixed { int mi, ix, en; const char *text; } want_fixed_t;

static const want_fixed_t WANT_FIXED[] = {
    { FINDER_MENU_IX_FILE,     0, 1, "New Folder" },
    { FINDER_MENU_IX_FILE,     2, 0, "Print" },
    { FINDER_MENU_IX_FILE,     8, 0, "Put Away" },
    { FINDER_MENU_IX_FILE,    10, 0, "Find" },
    { FINDER_MENU_IX_FILE,    13, 0, "Eject" },
    { FINDER_MENU_IX_EDIT,     0, 0, "Undo" },
    { FINDER_MENU_IX_EDIT,     5, 1, "Select All" },
    { FINDER_MENU_IX_EDIT,     9, 0, "Preferences" },
    { FINDER_MENU_IX_VIEW,     0, 1, "by Icons" },
    { FINDER_MENU_IX_VIEW,     4, 0, "by Kind" },
    { FINDER_MENU_IX_VIEW,    10, 1, "Clean Up" },
    { FINDER_MENU_IX_VIEW,    11, 1, "Arrange (by Name)" },
    { FINDER_MENU_IX_SPECIAL,  0, 1, "Clean Up" },
    { FINDER_MENU_IX_SPECIAL,  3, 0, "Erase Disk" },
    { FINDER_MENU_IX_SPECIAL,  5, 1, "Restart" },
    { FINDER_MENU_IX_SPECIAL,  6, 1, "Shut Down" },
    { FINDER_MENU_IX_SPECIAL,  7, 0, "Sleep" },
    { FINDER_MENU_IX_HELP,     0, 0, "About Help" },
    { FINDER_MENU_IX_HELP,     1, 0, "Show Balloons" }
};

static FinderCtx mk_ctx(const want_state_t *s)
{
    FinderCtx fx;
    memset(&fx, 0, sizeof fx);
    fx.selection_count  = s->sel;
    fx.trash_nonempty   = s->trash;
    fx.front_is_diskwin = s->front;
    return fx;
}

static int item_en(int mi, int ix)
{
    return (int)finder_menu_bar()->menus[mi].items[ix].enabled;
}

static void leg_refresh(void)
{
    int si, k;

    for (si = 0; si < WN(WANT_STATES); si++) {
        const want_state_t *s = &WANT_STATES[si];
        FinderCtx fx = mk_ctx(s);
        finder_menu_refresh_enables(&fx);

        CHECK(item_en(FINDER_MENU_IX_FILE, 1) == s->open_en, s->what);
        CHECK(item_en(FINDER_MENU_IX_FILE, 3) == s->close_en, s->what);
        CHECK(item_en(FINDER_MENU_IX_FILE, 5) == s->info_en, s->what);
        CHECK(item_en(FINDER_MENU_IX_FILE, 6) == s->dup_en, s->what);
        CHECK(item_en(FINDER_MENU_IX_SPECIAL, 1) == s->trash_en, s->what);

        for (k = 0; k < WN(WANT_FIXED); k++) {
            const want_fixed_t *f = &WANT_FIXED[k];
            CHECK(item_en(f->mi, f->ix) == f->en,
                  "C a NON-predicate row is INVARIANT across every refresh");
            CHECK(strcmp(finder_menu_bar()->menus[f->mi].items[f->ix].text,
                         f->text) == 0,
                  "C ... and refresh never moves a row");
        }
    }

    /* A NULL context is answered "nothing live" by the finder_cmd predicates,
     * which is the safe direction (Rule 2) -- never a stuck-enabled bar. */
    finder_menu_refresh_enables(NULL);
    CHECK(item_en(FINDER_MENU_IX_FILE, 1) == 0 &&
          item_en(FINDER_MENU_IX_FILE, 3) == 0 &&
          item_en(FINDER_MENU_IX_FILE, 5) == 0 &&
          item_en(FINDER_MENU_IX_FILE, 6) == 0 &&
          item_en(FINDER_MENU_IX_SPECIAL, 1) == 0,
          "C a NULL FinderCtx grays every predicate-driven row");
}

/* ===========================================================================
 * D -- MenuKey OVER THE BAR: every command key in the F4.2 resource
 * ---------------------------------------------------------------------------
 * menu.h Sec 8: "MenuKey scans every menu's items for the FIRST ENABLED,
 * non-divider item whose cmdChar matches (case-insensitive for letters)".
 * So a grayed row's command key does NOTHING -- which is exactly the property
 * that makes MenuKey over this bar STRICTLY STRONGER than the (menu,item)
 * table lookup it replaces in os/milton/kmain.c: the table lookup found the
 * row regardless and let finder_dispatch report FINDER-CMD-DISABLED; MenuKey
 * never hands the chord out at all.
 *
 * The expected result words are minted by menu.h's LOCKED MenuResult() from
 * HAND-TYPED (id, 1-based item) pairs -- the same pairs the command table's
 * rows carry, typed here independently.
 * ===========================================================================*/
typedef struct want_key {
    char     ch;
    uint32_t rest;   /* expected MenuKey result with the all-zero FinderCtx     */
    uint32_t live;   /* ... and with sel=2, trash=1, front=1                    */
    const char *what;
} want_key_t;

static void leg_menukey(void)
{
    MenuBar *bar = finder_menu_bar();
    const want_key_t WANT_KEYS[] = {
        /* always-live rows: identical in both states */
        { 'N', MenuResult(512, 1), MenuResult(512, 1), "D Cmd-N New Folder is always live" },
        { 'A', MenuResult(513, 6), MenuResult(513, 6), "D Cmd-A Select All is always live" },
        /* predicate-driven rows: dark at rest, live when the state says so */
        { 'O', 0u, MenuResult(512, 2), "D Cmd-O Open needs a selection" },
        { 'W', 0u, MenuResult(512, 4), "D Cmd-W Close Window needs a disk window front" },
        { 'I', 0u, MenuResult(512, 6), "D Cmd-I Get Info needs a selection" },
        { 'D', 0u, MenuResult(512, 7), "D Cmd-D Duplicate needs a selection" },
        /* permanently-inert V1 decoration: NEVER routes, in any state */
        { 'Y', 0u, 0u, "D Cmd-Y Put Away is grayed in V1 (F4.2) -- never routes" },
        { 'F', 0u, 0u, "D Cmd-F Find is inert V1 decoration" },
        { 'Z', 0u, 0u, "D Cmd-Z Undo is inert V1 decoration" },
        { 'X', 0u, 0u, "D Cmd-X Cut is grayed until the R4.2 scrap wiring" },
        { 'C', 0u, 0u, "D Cmd-C Copy is grayed until the R4.2 scrap wiring" },
        { 'V', 0u, 0u, "D Cmd-V Paste is grayed until the R4.2 scrap wiring" },
        /* not in the resource at all */
        { 'Q', 0u, 0u, "D an unbound chord finds nothing (the pump routes it onward)" },
        { 'B', 0u, 0u, "D ... and so does any other unbound letter" }
    };
    const want_state_t rest = {  0, 0, 0, 0,0,0,0,0, "rest" };
    const want_state_t live = {  2, 1, 1, 1,1,1,1,1, "live" };
    FinderCtx fx;
    int i;

    fx = mk_ctx(&rest);
    finder_menu_refresh_enables(&fx);
    for (i = 0; i < WN(WANT_KEYS); i++) {
        CHECK(MenuKey(bar, WANT_KEYS[i].ch) == WANT_KEYS[i].rest,
              WANT_KEYS[i].what);
        CHECK(MenuKey(bar, (char)(WANT_KEYS[i].ch | 0x20)) == WANT_KEYS[i].rest,
              "D ... case-insensitively (IM Command-key behaviour)");
    }

    fx = mk_ctx(&live);
    finder_menu_refresh_enables(&fx);
    for (i = 0; i < WN(WANT_KEYS); i++) {
        CHECK(MenuKey(bar, WANT_KEYS[i].ch) == WANT_KEYS[i].live,
              WANT_KEYS[i].what);
    }

    CHECK(MenuKey(bar, 0) == 0u, "D a zero command key never matches");

    /* Convergence (F4-4): every result word MenuKey hands out must resolve to
     * a command-table row, so the keyboard path can never produce
     * FINDER-CMD-UNKNOWN. Typed against the LIVE state, where all six route. */
    for (i = 0; i < WN(WANT_KEYS); i++) {
        uint32_t r = WANT_KEYS[i].live;
        if (r == 0u) continue;
        CHECK(finder_cmd_lookup(MenuResultID(r), MenuResultItem(r)) != NULL,
              "D every routed chord lands on a command-table row");
    }
}

/* ===========================================================================
 * E -- THE TWO TRANSCRIPTIONS AGREE (resource <-> command table)
 * ===========================================================================*/
static void leg_table_agreement(void)
{
    MenuBar *bar = finder_menu_bar();
    const want_state_t live = { 2, 1, 1, 1,1,1,1,1, "live" };
    FinderCtx fx = mk_ctx(&live);
    int mi, k;
    uint16_t r;

    finder_menu_refresh_enables(&fx);

    /* (1) Every row the resource can HAND OUT resolves to a table row. With the
     * all-live context this is every enabled, non-divider item in the bar. */
    for (mi = 0; mi < (int)bar->n_menus; mi++) {
        const MenuInfo *m = &bar->menus[mi];
        for (k = 0; k < (int)m->n_items; k++) {
            if (!MenuInfo_item_selectable(bar, mi, k)) continue;
            CHECK(finder_cmd_lookup(m->menuID, (uint16_t)(k + 1)) != NULL,
                  "E a selectable resource row has a command-table row (else "
                  "choosing it would be FINDER-CMD-UNKNOWN)");
        }
    }

    /* (2) Every table row of menus 512..516 names a REAL, non-divider row of
     * the resource. Catches a renumbered table row or a moved divider. */
    for (r = 0; r < FINDER_COMMANDS_N; r++) {
        const finder_cmd_t *c = &FINDER_COMMANDS[r];
        int found = 0;
        if (c->menu_id == (int16_t)FINDER_MENU_ID_APPLE) continue;
        for (mi = 0; mi < (int)bar->n_menus; mi++) {
            const MenuInfo *m = &bar->menus[mi];
            if (m->menuID != c->menu_id) continue;
            found = 1;
            CHECK(c->item_1based >= 1u && c->item_1based <= m->n_items,
                  "E a table row's item number is inside the resource menu");
            if (c->item_1based >= 1u && c->item_1based <= m->n_items) {
                const MenuItem *it = &m->items[c->item_1based - 1u];
                CHECK(it->is_divider == 0u,
                      "E ... and never names a divider row");
                CHECK(it->cmdChar == c->cmd_char,
                      "E ... and the command key matches the resource's");
            }
        }
        CHECK(found, "E every table menu id 512..516 exists in the resource");
    }

    /* (3) The Apple rows the table still carries for items 3/4 are the tdnl.14
     * seam, stated: the resource deliberately holds only item 1 this stage. */
    CHECK(finder_menu_apple()->n_items == 1u &&
          finder_cmd_lookup(0, 3) != NULL && finder_cmd_lookup(0, 4) != NULL,
          "E the Apple launch rows survive in the table while the resource "
          "omits their items (F4-3 omit-if-not-installed; returns with tdnl.14)");
}

/* ===========================================================================
 * F -- STRUCTURAL: no duplicate command key among SIMULTANEOUSLY-ENABLED items
 * ---------------------------------------------------------------------------
 * MenuKey returns the FIRST enabled match (menu.h Sec 8), so two enabled items
 * sharing a cmdChar means one of them is silently unreachable. Checked over
 * EVERY FinderCtx state in the truth table, because "simultaneously enabled"
 * is state-dependent.
 * ===========================================================================*/
static void leg_no_dup_cmdchar(void)
{
    MenuBar *bar = finder_menu_bar();
    int si;

    for (si = 0; si < WN(WANT_STATES); si++) {
        FinderCtx fx = mk_ctx(&WANT_STATES[si]);
        char seen[64];
        int  n_seen = 0, mi, k, j, dup = 0;

        finder_menu_refresh_enables(&fx);
        for (mi = 0; mi < (int)bar->n_menus; mi++) {
            const MenuInfo *m = &bar->menus[mi];
            for (k = 0; k < (int)m->n_items; k++) {
                char c = m->items[k].cmdChar;
                if (c == 0) continue;
                if (!MenuInfo_item_selectable(bar, mi, k)) continue;
                for (j = 0; j < n_seen; j++)
                    if (seen[j] == c) dup = 1;
                if (n_seen < (int)sizeof seen) seen[n_seen++] = c;
            }
        }
        CHECK(!dup,
              "F no two SIMULTANEOUSLY-ENABLED items share a command key "
              "(MenuKey takes the first match, so a duplicate hides a command)");
    }
}

int main(void)
{
    leg_structure();
    leg_layout();
    leg_refresh();
    leg_menukey();
    leg_table_agreement();
    leg_no_dup_cmdchar();
    return TEST_SUMMARY("test-finder-menu");
}

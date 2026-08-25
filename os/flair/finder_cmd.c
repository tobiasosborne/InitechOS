/*
 * os/flair/finder_cmd.c -- the R3 Finder COMMAND-TABLE SPINE (THE ARTIFACT).
 *
 * beads: initech-tdnl.27 (design slice tdnl.9a). See os/flair/finder_cmd.h for
 * the full banner: one table, one dispatch, both input paths converge, every
 * dispatch emits exactly one FINDER-CMD line on a caller-supplied trace sink.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.2 (menu resource --
 *        every item number below is read off it), F4.3 (Apple launch entries),
 *        F4.4 (the table struct, the dispatch contract, the predicates, the
 *        trace lines, the named mutants), F5.2 (slice tdnl.9a scope).
 *      os/flair/menu.h Sec 4 lines 171-179 (MenuResult / MenuResultID /
 *        MenuResultItem -- the (menuID<<16 | item_1based) packing this file
 *        unpacks; see fcmd_result_* below for why it is re-derived here).
 *      CLAUDE.md Law 1, Law 2, Law 3, Rule 2 (fail loud), Rule 6, Rule 11,
 *        Rule 12 (ASCII-clean).
 *
 * FREESTANDING (Law 3): includes ONLY finder_cmd.h (<stdint.h>). No libc -- the
 * decimal formatting and the string append below are hand-rolled because
 * os/flair has no formatting helper of its own (grep: no u32_to_dec / itoa /
 * sprintf anywhere under os/flair) and libc is not available in the kernel.
 *
 * MUTANT KNOBS (Rule 6; -D on the same TU, from the Makefile only -- they
 * perturb THIS IMPLEMENTATION, never the oracle's hand-authored golden):
 *   FINDER_CMD_MUT_TABLE_BYPASS      -- the key-source path calls the handler
 *                                       directly, skipping the table lookup's
 *                                       trace (the golden loses src=key lines).
 *   FINDER_CMD_MUT_SILENT_UNKNOWN    -- an unknown (menu,item) no-ops without
 *                                       the FINDER-CMD-UNKNOWN line.
 *   FINDER_CMD_MUT_PRED_STUCK_ENABLED-- predicates ignored; disabled commands
 *                                       dispatch anyway (no DISABLED line).
 * These are the design's CMD_TABLE_BYPASS / CMD_SILENT_UNKNOWN /
 * PRED_STUCK_ENABLED (F4.4 "Traceability"), namespaced to this TU.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include "finder_cmd.h"

/* ---------------------------------------------------------------------------
 * Result-word unpacking.
 *
 * os/flair/menu.h Sec 4 defines the packing as
 *     result = (uint32_t)(uint16_t)menuID << 16 | (uint32_t)item1based
 * with MenuResultID()/MenuResultItem() as the static-inline accessors. This
 * spine re-derives the two shifts locally INSTEAD of including menu.h, for two
 * reasons: (a) menu.h drags the whole Menu Manager render surface (text.h ->
 * chicago8x16.h, blitter.h, surface.h, grafport.h) into a file that draws
 * nothing; (b) it makes the host oracle a REAL differential -- test_finder_cmd.c
 * builds every result word with menu.h's MenuResult() and this file takes it
 * apart with its own arithmetic, so a divergence between the two is RED rather
 * than agreeing by construction (Law 2 / HER-02).
 * ------------------------------------------------------------------------- */
static int16_t fcmd_result_id(uint32_t r)   { return (int16_t)(uint16_t)(r >> 16); }
static uint16_t fcmd_result_item(uint32_t r){ return (uint16_t)(r & 0xFFFFu); }

/* ---------------------------------------------------------------------------
 * Libc-free line building. FCMD_LINE_CAP is generous for the longest line the
 * contract can produce; the appenders TRUNCATE rather than overrun, and the
 * buffer is always NUL-terminated (Rule 2 -- a truncated marker is visible on
 * the trace, a smashed stack is not).
 * ------------------------------------------------------------------------- */
#define FCMD_LINE_CAP 96

typedef struct fcmd_line {
    char     buf[FCMD_LINE_CAP];
    uint16_t len;
} fcmd_line_t;

static void fcmd_line_init(fcmd_line_t *l)
{
    l->len = 0;
    l->buf[0] = '\0';
}

static void fcmd_puts(fcmd_line_t *l, const char *s)
{
    if (s == 0) return;
    while (*s != '\0' && l->len + 1u < (uint16_t)FCMD_LINE_CAP)
        l->buf[l->len++] = *s++;
    l->buf[l->len] = '\0';
}

/* Unsigned decimal, no libc. Deterministic, no leading zeros, "0" for zero. */
static void fcmd_putu(fcmd_line_t *l, uint32_t v)
{
    char tmp[10];
    int  n = 0;
    if (v == 0) { fcmd_puts(l, "0"); return; }
    while (v != 0 && n < (int)sizeof tmp) { tmp[n++] = (char)('0' + (v % 10u)); v /= 10u; }
    while (n-- > 0 && l->len + 1u < (uint16_t)FCMD_LINE_CAP)
        l->buf[l->len++] = tmp[n];
    l->buf[l->len] = '\0';
}

/* Signed decimal (menu ids are int16_t; F4.4 uses 0 and 512..516, but an
 * UNKNOWN line must be able to print whatever garbage word arrived). */
static void fcmd_puti(fcmd_line_t *l, int16_t v)
{
    if (v < 0) { fcmd_puts(l, "-"); fcmd_putu(l, (uint32_t)(-(int32_t)v)); }
    else       { fcmd_putu(l, (uint32_t)v); }
}

/* Emit one built line on the caller's sink. A NULL sink means "no tracing". */
static void fcmd_emit(const FinderCtx *fx, const fcmd_line_t *l)
{
    if (fx == 0 || fx->trace == 0) return;
    fx->trace(fx->trace_user, l->buf);
}

#ifdef FINDER_CMD_MUT_TABLE_BYPASS
/* Libc-free equality, used ONLY by the TABLE_BYPASS mutant to recognize the
 * keyboard source. The correct build never needs to inspect `src`. */
static int fcmd_streq(const char *a, const char *b)
{
    if (a == 0 || b == 0) return 0;
    while (*a != '\0' && *a == *b) { a++; b++; }
    return (*a == '\0' && *b == '\0');
}
#endif

/* ===========================================================================
 * THE PREDICATES  (design F4.4)
 * ===========================================================================*/

/* HAS_SELECTION -- Open / Get Info / Duplicate / Put Away (F4.4). */
uint8_t finder_pred_has_selection(const FinderCtx *fx)
{
    return (fx != 0 && fx->selection_count > 0u) ? 1u : 0u;
}

/* TRASH_NONEMPTY -- Empty Trash is live only with something to purge (F4.2
 * Special row 2: "enabled iff Trash non-empty"). */
uint8_t finder_pred_trash_nonempty(const FinderCtx *fx)
{
    return (fx != 0 && fx->trash_nonempty != 0u) ? 1u : 0u;
}

/* WINDOW_FRONT_IS_DISKWIN -- Close Window (F4.4). The desktop itself is not a
 * window, so with no disk window in front there is nothing to close. */
uint8_t finder_pred_front_is_diskwin(const FinderCtx *fx)
{
    return (fx != 0 && fx->front_is_diskwin != 0u) ? 1u : 0u;
}

/* ===========================================================================
 * THE STUB HANDLERS  (this slice)
 * ---------------------------------------------------------------------------
 * F5.2 scopes tdnl.9a to "finder_cmd_t[], finder_dispatch, predicates, serial
 * trace" -- the bodies belong to tdnl.10 (disk windows: New Folder, Open,
 * Close Window, Clean Up), tdnl.11 (file ops: Get Info, Duplicate, Put Away,
 * Empty Trash), tdnl.12 (menu resource + Select All / view modes) and tdnl.14
 * (Apple launch). Until then EVERY command routes to one honest stub that says
 * so on the trace: a command that silently did nothing would be indisting-
 * uishable from a command that was never dispatched (Rule 2).
 * ===========================================================================*/
static void fcmd_nyi(FinderCtx *fx, const finder_cmd_t *c)
{
    fcmd_line_t l;
    fcmd_line_init(&l);
    fcmd_puts(&l, "FINDER-NYI name=");
    fcmd_puts(&l, c->serial_name);
    fcmd_emit(fx, &l);
}

/* ===========================================================================
 * THE TABLE  (design F4.2 layout -> F4.4 rows)
 * ---------------------------------------------------------------------------
 * item_1based is the IM 1-based index into the menu's items[] array COUNTING
 * DIVIDERS, read straight off the F4.2 lists:
 *
 *   Apple (menu_id 0, the has_apple slot):
 *     1 About This Computer | 2 --div-- | 3 Calculator | 4 Note Pad
 *     (the nine un-built DAs are OMITTED, not grayed -- ruling F4-3)
 *   File (512):
 *     1 New Folder N | 2 Open O | 3 Print | 4 Close Window W | 5 --div-- |
 *     6 Get Info I | 7 Duplicate D | 8 Make Alias | 9 Put Away Y |
 *     10 --div-- | 11 Find F | 12 Page Setup | 13 Sharing | 14 Eject
 *   Edit (513):
 *     1 Undo Z | 2 Cut X | 3 Copy C | 4 Paste V | 5 Clear | 6 Select All A |
 *     7 --div-- | 8 Show Clipboard | 9 --div-- | 10 Preferences
 *   View (514):
 *     1 by Icons | 2 by Small Icon | 3 by Name | 4 by Size | 5 by Kind |
 *     6 by Label | 7 by Date | 8 as Buttons | 9 as Pop-up Window |
 *     10 --div-- | 11 Clean Up | 12 Arrange (by Name)
 *   Special (515):
 *     1 Clean Up | 2 Empty Trash | 3 --div-- | 4 Erase Disk | 5 --div-- |
 *     6 Restart | 7 Shut Down | 8 Sleep
 *   Help (516):
 *     1 About Help | 2 Show Balloons     -- BOTH inert decoration in V1
 *
 * WHICH ROWS EXIST HERE: every item that carries a finder_cmd_id -- the
 * FUNCTIONAL commands plus the ones that are grayed BY PREDICATE (Open /
 * Get Info / Duplicate / Put Away with no selection, Close Window with no disk
 * window front, Empty Trash on an empty Trash) plus About This Computer, which
 * F4.2 specifies as "grayed until R4.1 (serial marker on attempt)" -- i.e. it
 * dispatches and reports, which is exactly what the stub handler does.
 *
 * The permanently-inert V1 decoration (Print, Make Alias, Find, Page Setup,
 * Sharing, Eject, Undo/Cut/Copy/Paste/Clear, Show Clipboard, Preferences, the
 * eight non-icon view modes, Erase Disk, Sleep, both Help rows) has NO command
 * id and NO row: it is menu-resource data whose MenuItem.enabled byte is 0, and
 * menu.c :: MenuSelect/MenuKey never return a dimmed or divider item
 * (MenuInfo_item_selectable, menu.c:240-258). Should one ever reach dispatch,
 * FINDER-CMD-UNKNOWN says so out loud rather than silently swallowing it.
 *
 * TWO DELIBERATE ALIASES (both from F4.2, both preserved here):
 *   - FCMD_CLEANUP appears twice: View item 11 and Special item 1 ("Clean Up
 *     (functional, same cmd id as View's)"). Same id, same serial_name -- one
 *     command reachable from two menus.
 *   - FCMD_LAUNCH_APPLE appears once per installed Apple entry. F4.4 sketches
 *     "+ per-entry target index" but its struct carries no target field;
 *     rather than widen the locked struct in this slice, the TARGET rides in
 *     serial_name (LAUNCH_APPLE_CALCULATOR / LAUNCH_APPLE_NOTEPAD), which is
 *     what the FINDER-LAUNCH trace of F4.3 needs to distinguish anyway. The
 *     tdnl.14 launch slice may promote it to a real field.
 * ===========================================================================*/
const finder_cmd_t FINDER_COMMANDS[] = {
    /* --- Apple slot (menu_id 0, F4.4) ------------------------------------ */
    { FCMD_ABOUT,           0,  1,  0,  0,                            fcmd_nyi, "ABOUT" },
    { FCMD_LAUNCH_APPLE,    0,  3,  0,  0,                            fcmd_nyi, "LAUNCH_APPLE_CALCULATOR" },
    { FCMD_LAUNCH_APPLE,    0,  4,  0,  0,                            fcmd_nyi, "LAUNCH_APPLE_NOTEPAD" },

    /* --- File (512) ------------------------------------------------------ */
    { FCMD_NEW_FOLDER,    512,  1, 'N', 0,                            fcmd_nyi, "NEW_FOLDER" },
    { FCMD_OPEN,          512,  2, 'O', finder_pred_has_selection,    fcmd_nyi, "OPEN" },
    { FCMD_CLOSE_WINDOW,  512,  4, 'W', finder_pred_front_is_diskwin, fcmd_nyi, "CLOSE_WINDOW" },
    { FCMD_GET_INFO,      512,  6, 'I', finder_pred_has_selection,    fcmd_nyi, "GET_INFO" },
    { FCMD_DUPLICATE,     512,  7, 'D', finder_pred_has_selection,    fcmd_nyi, "DUPLICATE" },
    /* Put Away's V1 rest-graying (F4.2) is a menu-resource enabled byte owned
     * by tdnl.12; the SEMANTIC predicate -- the one dispatch enforces -- is
     * "there is something selected to put away" (F1-5 stores the origins). */
    { FCMD_PUT_AWAY,      512,  9, 'Y', finder_pred_has_selection,    fcmd_nyi, "PUT_AWAY" },

    /* --- Edit (513) ------------------------------------------------------ */
    { FCMD_SELECT_ALL,    513,  6, 'A', 0,                            fcmd_nyi, "SELECT_ALL" },

    /* --- View (514) ------------------------------------------------------ */
    { FCMD_VIEW_ICONS,    514,  1,  0,  0,                            fcmd_nyi, "VIEW_ICONS" },
    { FCMD_CLEANUP,       514, 11,  0,  0,                            fcmd_nyi, "CLEANUP" },
    { FCMD_ARRANGE_BY_NAME, 514, 12, 0, 0,                            fcmd_nyi, "ARRANGE_BY_NAME" },

    /* --- Special (515) --------------------------------------------------- */
    { FCMD_CLEANUP,       515,  1,  0,  0,                            fcmd_nyi, "CLEANUP" },
    { FCMD_EMPTY_TRASH,   515,  2,  0,  finder_pred_trash_nonempty,   fcmd_nyi, "EMPTY_TRASH" },
    { FCMD_RESTART,       515,  6,  0,  0,                            fcmd_nyi, "RESTART" },
    { FCMD_SHUTDOWN,      515,  7,  0,  0,                            fcmd_nyi, "SHUTDOWN" }
};

const uint16_t FINDER_COMMANDS_N =
    (uint16_t)(sizeof FINDER_COMMANDS / sizeof FINDER_COMMANDS[0]);

/* ===========================================================================
 * LOOKUP + DISPATCH
 * ===========================================================================*/

const finder_cmd_t *finder_cmd_lookup(int16_t menu_id, uint16_t item_1based)
{
    uint16_t i;
    if (item_1based == 0u) return 0;   /* IM items are 1-based; 0 is no item  */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        if (FINDER_COMMANDS[i].menu_id == menu_id &&
            FINDER_COMMANDS[i].item_1based == item_1based)
            return &FINDER_COMMANDS[i];
    }
    return 0;
}

/* ASCII case fold, libc-free (Law 3). Only A-Z/a-z are folded; every other byte
 * compares verbatim, so a future non-letter cmd_char is unaffected. */
static char fcmd_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

const finder_cmd_t *finder_cmd_key_lookup(char ch)
{
    uint16_t i;
    char want = fcmd_upper(ch);
    if (want == '\0') return 0;       /* 0 == "no command key" (MenuItem)      */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        if (FINDER_COMMANDS[i].cmd_char != '\0' &&
            fcmd_upper(FINDER_COMMANDS[i].cmd_char) == want)
            return &FINDER_COMMANDS[i];
    }
    return 0;
}

uint32_t finder_cmd_result(const finder_cmd_t *c)
{
    if (c == 0) return 0u;
    return ((uint32_t)(uint16_t)c->menu_id << 16) | (uint32_t)c->item_1based;
}

uint8_t finder_cmd_enabled(const finder_cmd_t *c, const FinderCtx *fx)
{
    if (c == 0) return 0u;
    if (c->enabled == 0) return 1u;    /* ALWAYS (F4.4)                       */
    return c->enabled(fx) ? 1u : 0u;
}

void finder_dispatch(FinderCtx *fx, uint32_t menu_result, const char *src)
{
    const finder_cmd_t *c;
    int16_t   menu_id;
    uint16_t  item;
    uint8_t   live;
    fcmd_line_t l;

    if (fx == 0) return;               /* a NULL ctx is the caller's error    */
    if (menu_result == 0u) return;     /* IM "nothing chosen" (menu.h Sec 4)  */
    if (src == 0) src = "none";

    menu_id = fcmd_result_id(menu_result);
    item    = fcmd_result_item(menu_result);
    c       = finder_cmd_lookup(menu_id, item);

#ifdef FINDER_CMD_MUT_TABLE_BYPASS
    /* MUTANT (design CMD_TABLE_BYPASS): the keyboard path shortcuts straight
     * to the handler, so the FINDER-CMD src=key lines never appear and the
     * predicate is never consulted. Proves the golden bites on the "both input
     * paths converge on the table" property (F4-4). */
    if (c != 0 && fcmd_streq(src, "key")) {
        if (c->handler != 0) c->handler(fx, c);
        return;
    }
#endif

    if (c == 0) {
        /* Rule 2 / F4.4: loud on the trace, never a silent swallow. */
        fcmd_line_init(&l);
        fcmd_puts(&l, "FINDER-CMD-UNKNOWN menu=");
        fcmd_puti(&l, menu_id);
        fcmd_puts(&l, " item=");
        fcmd_putu(&l, (uint32_t)item);
#ifndef FINDER_CMD_MUT_SILENT_UNKNOWN
        fcmd_emit(fx, &l);
#endif
        /* MUTANT (design CMD_SILENT_UNKNOWN): the emit above is compiled out,
         * so an unrecognized (menu,item) no-ops in total silence. */
        return;
    }

    /* Exactly ONE FINDER-CMD line per dispatched command (F4.4). */
    fcmd_line_init(&l);
    fcmd_puts(&l, "FINDER-CMD id=");
    fcmd_putu(&l, (uint32_t)c->id);
    fcmd_puts(&l, " name=");
    fcmd_puts(&l, c->serial_name);
    fcmd_puts(&l, " src=");
    fcmd_puts(&l, src);
    fcmd_puts(&l, " sel=");
    fcmd_putu(&l, (uint32_t)fx->selection_count);
    fcmd_emit(fx, &l);

    live = finder_cmd_enabled(c, fx);
#ifdef FINDER_CMD_MUT_PRED_STUCK_ENABLED
    /* MUTANT (design PRED_STUCK_ENABLED): every command reads as live, so a
     * disabled command dispatches into its handler instead of reporting
     * FINDER-CMD-DISABLED. */
    live = 1u;
#endif
    if (!live) {
        fcmd_line_init(&l);
        fcmd_puts(&l, "FINDER-CMD-DISABLED name=");
        fcmd_puts(&l, c->serial_name);
        fcmd_emit(fx, &l);
        return;                        /* the handler is NOT called (F4.4)    */
    }

    /* The shell's execution hook wins when bound (bead initech-tdnl.10); the
     * table's stub handler is the fallback. Either way the FINDER-CMD line
     * above has ALREADY been emitted, so the trace records the dispatch
     * regardless of who executes it. */
    if (fx->exec != 0) {
        fx->exec(fx->shell, c->id);
        return;
    }
    if (c->handler != 0)
        c->handler(fx, c);
}

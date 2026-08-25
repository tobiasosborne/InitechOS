/*
 * os/flair/finder_cmd.h -- the R3 Finder COMMAND-TABLE SPINE (THE ARTIFACT).
 *
 * beads: initech-tdnl.27 (the GUI-remediation R3 Finder slice "tdnl.9a --
 *        command table spine": `finder_cmd_t[]`, `finder_dispatch`, predicates,
 *        serial trace). Host-testable pure logic; no rendering, no FAT, no
 *        emulator. The real handlers land in the later R3 beads (tdnl.10 disk
 *        windows, tdnl.11 file ops, tdnl.12 menu resource, tdnl.14 launch).
 *
 * WHAT THIS IS (docs/design/GUI-remediation-R3-finder-design.md F4.4, F5.2):
 *   ONE table, ONE dispatch function. The mouse path (flair_menu_track /
 *   MenuSelect) and the keyboard path (MenuKey, R1.6 Ctrl==Cmd) BOTH land in
 *   `finder_dispatch`, so there is exactly one place a Finder command can be
 *   executed and exactly one place it can be traced. F4-4: "One command table
 *   (finder_cmd_t[]), one dispatch function, both input paths converge, every
 *   dispatch emits FINDER-CMD."
 *
 * THE TRACE SINK -- WHY IT IS CALLER-SUPPLIED (Law 3 / layering):
 *   os/flair layer files do NOT call serial_puts; the serial markers of the
 *   live system are emitted from os/milton/kmain.c (grep serial_ over os/flair:
 *   no hits). Threading a serial dependency in here would make the Toolbox
 *   layer un-host-buildable and would put a kernel symbol in Layer 3/5 code.
 *   So FinderCtx carries a `trace` function pointer: the artifact side binds a
 *   serial-backed sink from kmain, the host oracle
 *   (harness/proptest/test_finder_cmd.c) binds a capture buffer. Same spine,
 *   same lines, two sinks (the ADR-0013 BC-2 "single spine" idiom).
 *
 * MENU-RESULT PACKING (os/flair/menu.h Sec 4, lines 171-179):
 *   MenuSelect/MenuKey pack the choice as (uint16_t)menuID << 16 | item_1based,
 *   the item number being 1-BASED and COUNTING DIVIDERS (menu.c :: MenuKey
 *   returns `it + 1` over the raw items[] index). `finder_dispatch` takes that
 *   word verbatim. NOTE the one corner: menu.h documents the FULL word 0 as
 *   "nothing chosen"; F4.4 assigns menu_id 0 to the Apple-slot entries. There
 *   is no collision -- an Apple item carries item_1based >= 1, so its result
 *   word is >= 1 and only the literal 0 word means "nothing chosen".
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.2 (the menu resource
 *        layout -- the item numbers in FINDER_COMMANDS come from it verbatim),
 *        F4.3 (Apple-menu launch entries), F4.4 (THIS table + dispatch +
 *        predicates + trace + the named mutants), F5.2 (slice tdnl.9a).
 *      os/flair/menu.h Sec 3 (MenuItem: text/cmdChar/enabled/is_divider),
 *        Sec 4 (MenuResult / MenuResultID / MenuResultItem).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (the oracle is truth -- the trace
 *        golden is HAND-AUTHORED in the oracle, never read back off this
 *        table), Law 3 (freestanding artifact, dual-compile), Rule 2 (fail
 *        loud -- an unknown (menu,item) SAYS SO on the trace, never silently
 *        no-ops), Rule 6 (mutation-proven), Rule 11, Rule 12 (ASCII-clean).
 *
 * ARTIFACT code: freestanding C (ADR-0002). No libc, no malloc, no printf --
 * all storage is static const table data plus a caller-owned FinderCtx.
 * Compiles BOTH under the kernel flags (gcc -m32 -march=i386 -ffreestanding
 * -nostdlib -std=c11 -Wall -Wextra -Werror) AND hosted for the property suite
 * (the window.c / process.c dual-compile pattern).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_FINDER_CMD_H
#define INITECH_OS_FLAIR_FINDER_CMD_H

#include <stdint.h>

/* ===========================================================================
 * 1. THE COMMAND IDS  (design F4.4 verbatim, with the one typo fixed)
 * ---------------------------------------------------------------------------
 * F4.4 prints the first entry as "FCMD_ABOUT_THIS Computer" -- an obvious
 * transcription slip of the menu item's display text ("About This Computer",
 * F4.2 Apple row 1) into the identifier. The id is FCMD_ABOUT.
 *
 * The values are pinned EXPLICITLY: the id is printed as a decimal on the
 * FINDER-CMD trace line, and that trace is a locked golden (F4.4
 * "Traceability"). Renumbering by insertion would silently rewrite every
 * golden, so new commands append before FCMD_COUNT with a fresh number.
 * ===========================================================================*/
typedef enum finder_cmd_id {
    FCMD_NONE            = 0,   /* not a command (sentinel / no match)         */
    FCMD_ABOUT           = 1,   /* Apple: About This Computer (F4.2)           */
    /* lifecycle */
    FCMD_NEW_FOLDER      = 2,   /* File: New Folder  Cmd-N                     */
    FCMD_OPEN            = 3,   /* File: Open        Cmd-O                     */
    FCMD_CLOSE_WINDOW    = 4,   /* File: Close Window Cmd-W                    */
    /* item ops */
    FCMD_GET_INFO        = 5,   /* File: Get Info    Cmd-I                     */
    FCMD_DUPLICATE       = 6,   /* File: Duplicate   Cmd-D                     */
    FCMD_PUT_AWAY        = 7,   /* File: Put Away    Cmd-Y                     */
    FCMD_SELECT_ALL      = 8,   /* Edit: Select All  Cmd-A                     */
    FCMD_CLEANUP         = 9,   /* View AND Special: Clean Up (one id, F4.2)   */
    FCMD_ARRANGE_BY_NAME = 10,  /* View: Arrange (by Name)                     */
    /* trash */
    FCMD_EMPTY_TRASH     = 11,  /* Special: Empty Trash                        */
    /* system */
    FCMD_RESTART         = 12,  /* Special: Restart                            */
    FCMD_SHUTDOWN        = 13,  /* Special: Shut Down                          */
    /* view */
    FCMD_VIEW_ICONS      = 14,  /* View: by Icons (the checked view mode)      */
    /* apple */
    FCMD_LAUNCH_APPLE    = 15,  /* Apple: a launchable DA entry (F4.3);        */
                                /* the TARGET is carried by serial_name        */
    FCMD_COUNT           = 16
} finder_cmd_id;

/* ===========================================================================
 * 2. FinderCtx -- the dispatch context (MINIMAL, this slice)
 * ---------------------------------------------------------------------------
 * The enablement predicates of F4.4 read exactly three pieces of Finder state,
 * so exactly three state fields exist here today. The DesktopMgr slice
 * (tdnl.9, design F1/F2) populates them (and will extend this record); the
 * spine deliberately does NOT reach into a desktop structure it cannot yet
 * name, so this file stays pure logic and host-buildable.
 *
 * `trace` is the caller-supplied sink (see the header banner): the artifact
 * binds serial, the oracle binds a capture buffer. A NULL sink means "no
 * tracing"; dispatch still routes. `trace_user` is passed back untouched.
 * ===========================================================================*/
typedef struct FinderCtx {
    /* --- state the F4.4 predicates read ---------------------------------- */
    uint16_t selection_count;   /* number of selected desktop/window icons     */
    uint8_t  trash_nonempty;    /* 1 == \TRASH holds at least one entry (F1.4) */
    uint8_t  front_is_diskwin;  /* 1 == frontmost window is a Finder disk win  */

    /* --- the trace sink (caller-supplied; see banner) --------------------- */
    void   (*trace)(void *user, const char *line);
    void    *trace_user;

    /* --- the SHELL EXECUTION HOOK (bead initech-tdnl.10) ------------------
     * The table's own handlers are the honest FINDER-NYI stubs of tdnl.9a; the
     * real behaviour lives in the Finder shell (os/flair/finder_windows.c),
     * which this header cannot name without inverting the layering (finder_cmd
     * is pure logic: one table, one dispatch, no rendering, no FAT, no windows).
     *
     * So the shell binds `exec` + its `shell` cookie at boot and finder_dispatch
     * calls THAT instead of the row's stub handler. The spine is unchanged --
     * one lookup, one predicate evaluation, one FINDER-CMD trace line, emitted
     * BEFORE the hook runs -- so the trace golden and all three tdnl.9a mutants
     * are untouched, and there is still exactly ONE place a Finder command can
     * execute. A NULL `exec` means "run the table's handler", which is what the
     * host oracle (test_finder_cmd.c) does.
     * ------------------------------------------------------------------- */
    void   (*exec)(void *shell, finder_cmd_id id);
    void    *shell;
} FinderCtx;

/* ===========================================================================
 * 3. THE TABLE ENTRY  (design F4.4 struct, verbatim field-for-field)
 * ===========================================================================*/
struct finder_cmd;

/* A predicate answers "is this command live right now?" (1 == enabled).
 * NULL == the ALWAYS predicate (F4.4). */
typedef uint8_t (*finder_cmd_pred_fn)(const FinderCtx *fx);

/* A handler executes the command. THIS SLICE ships stubs that emit exactly one
 * FINDER-NYI line; the real bodies land in tdnl.10/.11/.12/.14. */
typedef void (*finder_cmd_handler_fn)(FinderCtx *fx, const struct finder_cmd *c);

typedef struct finder_cmd {
    finder_cmd_id          id;
    int16_t                menu_id;      /* 512..516; 0 == Apple-slot (F4.4)   */
    uint16_t               item_1based;  /* IM 1-based item number (menu.h S4) */
    char                   cmd_char;     /* 0 == none (MenuItem.cmdChar)       */
    finder_cmd_pred_fn     enabled;      /* NULL == ALWAYS                     */
    finder_cmd_handler_fn  handler;
    const char            *serial_name;  /* the name printed on the trace      */
} finder_cmd_t;

/* THE table. Mouse and keyboard both land here (F4.4). */
extern const finder_cmd_t FINDER_COMMANDS[];
/* Live length of FINDER_COMMANDS[] (no sentinel row; the table is const data
 * and the count is compiled from it, so the two cannot drift). */
extern const uint16_t FINDER_COMMANDS_N;

/* ===========================================================================
 * 4. THE PREDICATES  (F4.4: HAS_SELECTION / TRASH_NONEMPTY /
 *    WINDOW_FRONT_IS_DISKWIN / ALWAYS==NULL)
 * ---------------------------------------------------------------------------
 * Exported so the tdnl.12 menu-resource slice can recompute the static
 * MenuItem.enabled bytes from the SAME predicates the dispatcher enforces --
 * one source of truth for graying, per F4.4 ("Recomputed into the static
 * MenuItem.enabled bytes before every DrawMenuBar/track").
 * ===========================================================================*/
uint8_t finder_pred_has_selection(const FinderCtx *fx);
uint8_t finder_pred_trash_nonempty(const FinderCtx *fx);
uint8_t finder_pred_front_is_diskwin(const FinderCtx *fx);

/* ===========================================================================
 * 5. LOOKUP + DISPATCH
 * ===========================================================================*/

/*
 * finder_cmd_lookup -- the (menu_id, item_1based) -> table row map.
 * Returns NULL when no row matches (the caller's cue to fail loud).
 */
const finder_cmd_t *finder_cmd_lookup(int16_t menu_id, uint16_t item_1based);

/*
 * finder_cmd_key_lookup -- the COMMAND-KEY -> table row map (bead initech-tdnl.10).
 *
 * `ch` is the Cmd-equivalent character as the MenuItem carries it (upper-case
 * in the F4.2 resource: 'N', 'O', 'W', ...); the match is case-INSENSITIVE for
 * ASCII letters, because a PC keyboard's Ctrl-chord cooks to whatever case the
 * Shift state produced and the period Mac never distinguished Cmd-n from Cmd-N.
 * The FIRST row with a matching cmd_char wins (the table has no duplicate
 * cmd_chars; a duplicate would be a table bug, and taking the first is the same
 * rule menu.c :: MenuKey applies). Returns NULL when nothing matches.
 *
 * THIS IS A LOOKUP, NOT A SECOND DISPATCH PATH. The caller turns the row into a
 * result word with finder_cmd_result() and calls finder_dispatch(..., "key"),
 * so the keyboard converges on the SAME spine as the mouse (F4-4) and the trace
 * line is the ordinary FINDER-CMD ... src=key.
 *
 * TEMPORARY, AND SAY SO: tdnl.12 lands the real Finder menu bar, at which point
 * MenuKey() over that bar replaces this helper -- MenuKey additionally honours
 * the live MenuItem.enabled bytes and the divider rule. Until the bar exists
 * there is no MenuBar to scan, and inventing a hidden one early would re-key
 * every band-2 gate. The predicate is still enforced: finder_dispatch evaluates
 * it after the lookup, exactly as on the mouse path.
 */
const finder_cmd_t *finder_cmd_key_lookup(char ch);

/*
 * finder_cmd_result -- pack a table row back into the IM result word
 * ((uint16_t)menu_id << 16 | item_1based; menu.h Sec 4). Returns 0 for NULL,
 * which finder_dispatch reads as "nothing chosen" and ignores.
 */
uint32_t finder_cmd_result(const finder_cmd_t *c);

/*
 * finder_cmd_enabled -- evaluate a row's predicate against fx (NULL == ALWAYS).
 */
uint8_t finder_cmd_enabled(const finder_cmd_t *c, const FinderCtx *fx);

/*
 * finder_dispatch -- THE single command entry point (design F4.4).
 *
 *   menu_result : the IM result word from MenuSelect/MenuKey, i.e.
 *                 (uint16_t)menuID << 16 | item_1based (os/flair/menu.h Sec 4,
 *                 MenuResult()). The literal word 0 is IM's "nothing chosen"
 *                 and returns without a trace line -- it is not a command and
 *                 a pull-down released off-menu produces it constantly.
 *   src         : "mouse" | "key" (F4.4). Printed verbatim on the trace.
 *
 * Emits, on the fx->trace sink, EXACTLY:
 *   - unknown (menu,item):
 *         FINDER-CMD-UNKNOWN menu=<menu_id> item=<item_1based>
 *     and no-ops (Rule 2 -- loud on the trace, never silent).
 *   - a known command:
 *         FINDER-CMD id=<decimal> name=<serial_name> src=<src> sel=<n>
 *     then, if its predicate says disabled:
 *         FINDER-CMD-DISABLED name=<serial_name>
 *     and the handler is NOT called; otherwise the handler runs (and this
 *     slice's stub handler emits FINDER-NYI name=<serial_name>).
 */
void finder_dispatch(FinderCtx *fx, uint32_t menu_result, const char *src);

#endif /* INITECH_OS_FLAIR_FINDER_CMD_H */

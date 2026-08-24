/* test_finder_cmd.c -- the HOST oracle for the R3 Finder command-table spine.
 *
 * beads: initech-tdnl.27 (design slice tdnl.9a, docs/design/
 *        GUI-remediation-R3-finder-design.md F4.4 / F5.2 "Host trace golden;
 *        mutants CMD_TABLE_BYPASS, CMD_SILENT_UNKNOWN" + F4-4's
 *        PRED_STUCK_ENABLED).
 *
 * WHAT IT GRADES: os/flair/finder_cmd.c -- the EXACT symbol the live Finder
 * runs (`finder_dispatch`), driven through the caller-supplied trace sink of
 * FinderCtx. The artifact binds a serial-backed sink from os/milton/kmain.c;
 * this oracle binds a capture buffer. Same spine, same lines (the ADR-0013
 * BC-2 single-spine idiom).
 *
 * HAND-AUTHORED GOLDEN (Law 2; the test_process.c discipline). The expected
 * trace below is TYPED OUT BY HAND from the F4.4 contract -- it is never read
 * back off FINDER_COMMANDS[], never rendered by a second copy of the format
 * code, never regenerated from a run. An oracle that asked the dispatcher what
 * the dispatcher emitted would agree BY CONSTRUCTION and is forbidden (Law 2 /
 * Revocation Record HER-02). Same rule for the structural goldens: the
 * per-id occurrence counts and the cmd-key map are hand-typed from F4.2, not
 * counted out of the table and compared with itself.
 *
 * A REAL DIFFERENTIAL ON THE PACKING: every menu_result fed to the dispatcher
 * is built with menu.h's MenuResult() (the LOCKED Inside Macintosh packing,
 * os/flair/menu.h Sec 4 lines 171-179), while finder_cmd.c takes the word
 * apart with its own local shifts. If the two ever disagree the script goes
 * RED instead of agreeing with itself.
 *
 * THE SCRIPT (both sources; enabled, disabled and unknown cases; the
 * predicates flipped by mutating FinderCtx mid-script -- exactly the state the
 * DesktopMgr slice will drive):
 *   1  File/New Folder, mouse       -- ALWAYS, no selection needed
 *   2  File/Get Info, mouse         -- HAS_SELECTION with sel=0  -> DISABLED
 *   3  (selection_count = 2) File/Get Info, key   -> runs; sel=2 on the line
 *   4  Special/Empty Trash, mouse   -- TRASH_NONEMPTY, empty     -> DISABLED
 *   5  (trash_nonempty = 1) Special/Empty Trash, key -> runs
 *   6  File/Close Window, key       -- FRONT_IS_DISKWIN, desktop -> DISABLED
 *   7  (front_is_diskwin = 1) File/Close Window, mouse -> runs
 *   8  View/Clean Up, mouse         -- the aliased FCMD_CLEANUP, View side
 *   9  Special/Clean Up, key        -- the SAME id from the other menu
 *   10 Apple/Calculator, mouse      -- launchable Apple entry (F4.3)
 *   11 Apple/Note Pad, key          -- the second launch target
 *   12 Edit/Select All, key         -- ALWAYS
 *   13 File item 3 (Print), mouse   -- inert V1 decoration, no id -> UNKNOWN
 *   14 Help item 1, key             -- menu 516 carries no commands -> UNKNOWN
 *   15 the literal result word 0    -- IM "nothing chosen": NO line at all
 *   16 Special/Shut Down, mouse     -- ALWAYS
 *
 * MUTATION-PROVEN (Rule 6). The mutants are -D knobs on os/flair/finder_cmd.c
 * (the IMPLEMENTATION, never this golden):
 *   FINDER_CMD_MUT_TABLE_BYPASS       -- key-source dispatch skips the table's
 *     trace: legs 3/5/9/11/12 lose their FINDER-CMD line and leg 6 loses both
 *     its FINDER-CMD and its DISABLED line (running the handler instead). RED.
 *   FINDER_CMD_MUT_SILENT_UNKNOWN     -- legs 13/14 emit nothing. RED.
 *   FINDER_CMD_MUT_PRED_STUCK_ENABLED -- legs 2/4/6 run their handler instead
 *     of reporting FINDER-CMD-DISABLED. RED.
 *
 * Ref: docs/design/GUI-remediation-R3-finder-design.md F4.2 (the menu resource
 *        the item numbers come from), F4.3, F4.4, F5.2.
 *      os/flair/finder_cmd.h (the contract), os/flair/menu.h Sec 4.
 *      harness/proptest/test_process.c (the hand-authored-log discipline this
 *        file follows). CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "menu.h"          /* the LOCKED MenuResult packing (-Ios/flair)      */
#include "finder_cmd.h"    /* THE spine under test (-Ios/flair)               */
#include "test_assert.h"   /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* ===========================================================================
 * The capture sink -- the host's stand-in for the kernel's serial sink.
 * ===========================================================================*/
enum { CAP_MAX_LINES = 64, CAP_LINE_CAP = 128 };

typedef struct capture {
    char lines[CAP_MAX_LINES][CAP_LINE_CAP];
    int  n;
    int  overflow;
} capture_t;

static void cap_sink(void *user, const char *line)
{
    capture_t *cap = (capture_t *)user;
    if (cap->n >= CAP_MAX_LINES) { cap->overflow = 1; return; }
    if (strlen(line) >= (size_t)CAP_LINE_CAP) { cap->overflow = 1; return; }
    strcpy(cap->lines[cap->n], line);
    cap->n++;
}

/* ===========================================================================
 * THE HAND-AUTHORED GOLDEN (typed from F4.4's contract; NOT generated)
 * ===========================================================================*/
static const char *const EXPECTED[] = {
    /* 1  File/New Folder, mouse */
    "FINDER-CMD id=2 name=NEW_FOLDER src=mouse sel=0",
    "FINDER-NYI name=NEW_FOLDER",
    /* 2  File/Get Info, mouse, nothing selected */
    "FINDER-CMD id=5 name=GET_INFO src=mouse sel=0",
    "FINDER-CMD-DISABLED name=GET_INFO",
    /* 3  File/Get Info, key, two icons selected */
    "FINDER-CMD id=5 name=GET_INFO src=key sel=2",
    "FINDER-NYI name=GET_INFO",
    /* 4  Special/Empty Trash, mouse, Trash empty */
    "FINDER-CMD id=11 name=EMPTY_TRASH src=mouse sel=2",
    "FINDER-CMD-DISABLED name=EMPTY_TRASH",
    /* 5  Special/Empty Trash, key, Trash now holds something */
    "FINDER-CMD id=11 name=EMPTY_TRASH src=key sel=2",
    "FINDER-NYI name=EMPTY_TRASH",
    /* 6  File/Close Window, key, desktop front (no disk window) */
    "FINDER-CMD id=4 name=CLOSE_WINDOW src=key sel=2",
    "FINDER-CMD-DISABLED name=CLOSE_WINDOW",
    /* 7  File/Close Window, mouse, a disk window is front */
    "FINDER-CMD id=4 name=CLOSE_WINDOW src=mouse sel=2",
    "FINDER-NYI name=CLOSE_WINDOW",
    /* 8  View/Clean Up, mouse */
    "FINDER-CMD id=9 name=CLEANUP src=mouse sel=2",
    "FINDER-NYI name=CLEANUP",
    /* 9  Special/Clean Up, key -- SAME command id from the other menu */
    "FINDER-CMD id=9 name=CLEANUP src=key sel=2",
    "FINDER-NYI name=CLEANUP",
    /* 10 Apple/Calculator, mouse */
    "FINDER-CMD id=15 name=LAUNCH_APPLE_CALCULATOR src=mouse sel=2",
    "FINDER-NYI name=LAUNCH_APPLE_CALCULATOR",
    /* 11 Apple/Note Pad, key */
    "FINDER-CMD id=15 name=LAUNCH_APPLE_NOTEPAD src=key sel=2",
    "FINDER-NYI name=LAUNCH_APPLE_NOTEPAD",
    /* 12 Edit/Select All, key */
    "FINDER-CMD id=8 name=SELECT_ALL src=key sel=2",
    "FINDER-NYI name=SELECT_ALL",
    /* 13 File item 3 == Print: inert V1 decoration, carries no command id */
    "FINDER-CMD-UNKNOWN menu=512 item=3",
    /* 14 Help item 1 == About Help: menu 516 carries no commands at all */
    "FINDER-CMD-UNKNOWN menu=516 item=1",
    /* 15 the literal word 0 emits NOTHING (IM "nothing chosen") */
    /* 16 Special/Shut Down, mouse */
    "FINDER-CMD id=13 name=SHUTDOWN src=mouse sel=2",
    "FINDER-NYI name=SHUTDOWN"
};
enum { EXPECTED_N = (int)(sizeof EXPECTED / sizeof EXPECTED[0]) };

/* ===========================================================================
 * Leg 1: the scripted trace vs the hand-authored golden.
 * ===========================================================================*/
static void run_script(capture_t *cap)
{
    FinderCtx fx;

    memset(cap, 0, sizeof *cap);
    memset(&fx, 0, sizeof fx);
    fx.selection_count  = 0;
    fx.trash_nonempty   = 0;
    fx.front_is_diskwin = 0;
    fx.trace            = cap_sink;
    fx.trace_user       = cap;

    /* Every result word is minted by the LOCKED packer (menu.h Sec 4). */
    finder_dispatch(&fx, MenuResult(512, 1), "mouse");   /* 1  New Folder    */
    finder_dispatch(&fx, MenuResult(512, 6), "mouse");   /* 2  Get Info (0)  */

    fx.selection_count = 2;                              /* predicate flip   */
    finder_dispatch(&fx, MenuResult(512, 6), "key");     /* 3  Get Info (2)  */

    finder_dispatch(&fx, MenuResult(515, 2), "mouse");   /* 4  Empty Trash   */
    fx.trash_nonempty = 1;                               /* predicate flip   */
    finder_dispatch(&fx, MenuResult(515, 2), "key");     /* 5  Empty Trash   */

    finder_dispatch(&fx, MenuResult(512, 4), "key");     /* 6  Close Window  */
    fx.front_is_diskwin = 1;                             /* predicate flip   */
    finder_dispatch(&fx, MenuResult(512, 4), "mouse");   /* 7  Close Window  */

    finder_dispatch(&fx, MenuResult(514, 11), "mouse");  /* 8  Clean Up/View */
    finder_dispatch(&fx, MenuResult(515,  1), "key");    /* 9  Clean Up/Spec */

    finder_dispatch(&fx, MenuResult(0, 3), "mouse");     /* 10 Calculator    */
    finder_dispatch(&fx, MenuResult(0, 4), "key");       /* 11 Note Pad      */

    finder_dispatch(&fx, MenuResult(513, 6), "key");     /* 12 Select All    */

    finder_dispatch(&fx, MenuResult(512, 3), "mouse");   /* 13 Print -> ???  */
    finder_dispatch(&fx, MenuResult(516, 1), "key");     /* 14 Help  -> ???  */

    finder_dispatch(&fx, 0u, "mouse");                   /* 15 nothing chosen*/

    finder_dispatch(&fx, MenuResult(515, 7), "mouse");   /* 16 Shut Down     */
}

static void leg_trace_golden(void)
{
    capture_t cap;
    int i;

    run_script(&cap);

    CHECK(cap.overflow == 0, "trace capture overflowed (line too long or too many)");
    CHECK(cap.n == EXPECTED_N, "trace line COUNT differs from the hand-authored golden");
    if (cap.n != EXPECTED_N)
        fprintf(stderr, "        got %d lines, want %d\n", cap.n, EXPECTED_N);

    for (i = 0; i < EXPECTED_N; i++) {
        if (i >= cap.n) {
            g_checks++; g_fails++;
            fprintf(stderr, "  FAIL %s:%d: trace line %d MISSING\n"
                            "        want: %s\n", __FILE__, __LINE__, i, EXPECTED[i]);
            continue;
        }
        g_checks++;
        if (strcmp(cap.lines[i], EXPECTED[i]) != 0) {
            g_fails++;
            fprintf(stderr, "  FAIL %s:%d: trace line %d differs\n"
                            "        got : %s\n        want: %s\n",
                    __FILE__, __LINE__, i, cap.lines[i], EXPECTED[i]);
        }
    }
    for (i = EXPECTED_N; i < cap.n; i++) {
        g_checks++; g_fails++;
        fprintf(stderr, "  FAIL %s:%d: UNEXPECTED extra trace line %d\n"
                        "        got : %s\n", __FILE__, __LINE__, i, cap.lines[i]);
    }
}

/* ===========================================================================
 * Leg 2: structural invariants of the table.
 * ===========================================================================*/

/* HAND-AUTHORED: how many rows each command id owns (F4.2). Every id in
 * 1..FCMD_COUNT-1 appears at least once; the two DELIBERATE aliases are
 * Clean Up (View 11 + Special 1) and the Apple launch entries (Calculator +
 * Note Pad). FCMD_NONE is never a row. */
static const uint16_t EXPECTED_ROWS_PER_ID[FCMD_COUNT] = {
    /* FCMD_NONE            */ 0,
    /* FCMD_ABOUT           */ 1,
    /* FCMD_NEW_FOLDER      */ 1,
    /* FCMD_OPEN            */ 1,
    /* FCMD_CLOSE_WINDOW    */ 1,
    /* FCMD_GET_INFO        */ 1,
    /* FCMD_DUPLICATE       */ 1,
    /* FCMD_PUT_AWAY        */ 1,
    /* FCMD_SELECT_ALL      */ 1,
    /* FCMD_CLEANUP         */ 2,   /* View item 11 AND Special item 1        */
    /* FCMD_ARRANGE_BY_NAME */ 1,
    /* FCMD_EMPTY_TRASH     */ 1,
    /* FCMD_RESTART         */ 1,
    /* FCMD_SHUTDOWN        */ 1,
    /* FCMD_VIEW_ICONS      */ 1,
    /* FCMD_LAUNCH_APPLE    */ 2    /* Calculator AND Note Pad (F4.2/F4.3)    */
};

/* HAND-AUTHORED: the command-key map (F4.2). Exactly these seven chars carry
 * a Cmd equivalent in V1, each on exactly one row, each on this command. */
typedef struct { char ch; finder_cmd_id id; } cmdkey_row_t;
static const cmdkey_row_t EXPECTED_CMDKEYS[] = {
    { 'N', FCMD_NEW_FOLDER },
    { 'O', FCMD_OPEN },
    { 'W', FCMD_CLOSE_WINDOW },
    { 'I', FCMD_GET_INFO },
    { 'D', FCMD_DUPLICATE },
    { 'Y', FCMD_PUT_AWAY },
    { 'A', FCMD_SELECT_ALL }
};
enum { EXPECTED_CMDKEYS_N =
       (int)(sizeof EXPECTED_CMDKEYS / sizeof EXPECTED_CMDKEYS[0]) };

static void leg_structure(void)
{
    uint16_t i, j;
    uint16_t seen[FCMD_COUNT];
    int k;
    uint16_t total_expected = 0;

    /* (a) Table size matches the hand-authored row budget. */
    for (i = 0; i < (uint16_t)FCMD_COUNT; i++)
        total_expected = (uint16_t)(total_expected + EXPECTED_ROWS_PER_ID[i]);
    CHECK(FINDER_COMMANDS_N == total_expected,
          "FINDER_COMMANDS row count differs from the hand-authored budget");

    /* (b) No duplicate (menu_id, item_1based) pair -- a duplicate would make
     *     finder_cmd_lookup order-dependent and silently shadow a command. */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        for (j = (uint16_t)(i + 1u); j < FINDER_COMMANDS_N; j++) {
            g_checks++;
            if (FINDER_COMMANDS[i].menu_id == FINDER_COMMANDS[j].menu_id &&
                FINDER_COMMANDS[i].item_1based == FINDER_COMMANDS[j].item_1based) {
                g_fails++;
                fprintf(stderr, "  FAIL %s:%d: duplicate (menu %d, item %u) at rows %u and %u\n",
                        __FILE__, __LINE__, (int)FINDER_COMMANDS[i].menu_id,
                        (unsigned)FINDER_COMMANDS[i].item_1based,
                        (unsigned)i, (unsigned)j);
            }
        }
    }

    /* (c) Per-id row counts match the hand-authored golden: every id covered,
     *     no accidental second home for a command, aliases exactly as ruled. */
    for (i = 0; i < (uint16_t)FCMD_COUNT; i++) seen[i] = 0;
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        finder_cmd_id id = FINDER_COMMANDS[i].id;
        g_checks++;
        if (id <= FCMD_NONE || id >= FCMD_COUNT) {
            g_fails++;
            fprintf(stderr, "  FAIL %s:%d: row %u carries out-of-range id %d\n",
                    __FILE__, __LINE__, (unsigned)i, (int)id);
            continue;
        }
        seen[id]++;
    }
    for (i = 0; i < (uint16_t)FCMD_COUNT; i++) {
        g_checks++;
        if (seen[i] != EXPECTED_ROWS_PER_ID[i]) {
            g_fails++;
            fprintf(stderr, "  FAIL %s:%d: id %u has %u rows, golden says %u\n",
                    __FILE__, __LINE__, (unsigned)i, (unsigned)seen[i],
                    (unsigned)EXPECTED_ROWS_PER_ID[i]);
        }
    }

    /* (d) Menu ids are the F4.1 range: 0 (Apple slot) or 512..516. */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        int16_t m = FINDER_COMMANDS[i].menu_id;
        CHECK(m == 0 || (m >= 512 && m <= 516),
              "a table row sits outside the F4.1 menu-id range (0 / 512..516)");
        CHECK(FINDER_COMMANDS[i].item_1based >= 1u,
              "a table row carries a 0 item number (IM items are 1-based)");
        CHECK(FINDER_COMMANDS[i].serial_name != NULL &&
              FINDER_COMMANDS[i].serial_name[0] != '\0',
              "a table row carries an empty serial_name (the trace would lose it)");
        CHECK(FINDER_COMMANDS[i].handler != NULL,
              "a table row carries a NULL handler (the command would vanish)");
    }

    /* (e) serial_name is ASCII-clean (Rule 12) and space-free (the trace is
     *     whitespace-delimited: a space in a name would split a field). */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        const char *s = FINDER_COMMANDS[i].serial_name;
        int ok = 1;
        for (; *s != '\0'; s++)
            if ((unsigned char)*s < 0x21u || (unsigned char)*s > 0x7Eu) ok = 0;
        CHECK(ok, "serial_name is not printable-ASCII-without-space");
    }

    /* (f) The hand-authored command-key map (F4.2). */
    for (k = 0; k < EXPECTED_CMDKEYS_N; k++) {
        int hits = 0;
        finder_cmd_id got = FCMD_NONE;
        for (i = 0; i < FINDER_COMMANDS_N; i++) {
            if (FINDER_COMMANDS[i].cmd_char == EXPECTED_CMDKEYS[k].ch) {
                hits++;
                got = FINDER_COMMANDS[i].id;
            }
        }
        g_checks++;
        if (hits != 1 || got != EXPECTED_CMDKEYS[k].id) {
            g_fails++;
            fprintf(stderr, "  FAIL %s:%d: cmd-key '%c': %d row(s), id %d; golden wants 1 row, id %d\n",
                    __FILE__, __LINE__, EXPECTED_CMDKEYS[k].ch, hits, (int)got,
                    (int)EXPECTED_CMDKEYS[k].id);
        }
    }
    /* ...and no OTHER row carries a cmd-key. */
    for (i = 0; i < FINDER_COMMANDS_N; i++) {
        char ch = FINDER_COMMANDS[i].cmd_char;
        int listed = 0;
        if (ch == 0) continue;
        for (k = 0; k < EXPECTED_CMDKEYS_N; k++)
            if (EXPECTED_CMDKEYS[k].ch == ch) listed = 1;
        CHECK(listed, "a table row carries a cmd-key the F4.2 golden does not list");
    }
}

/* ===========================================================================
 * Leg 3: lookup + predicate unit behavior (the pieces the script leans on).
 * ===========================================================================*/
static void leg_lookup_and_predicates(void)
{
    FinderCtx fx;
    const finder_cmd_t *c;

    memset(&fx, 0, sizeof fx);

    c = finder_cmd_lookup(512, 1);
    CHECK(c != NULL && c->id == FCMD_NEW_FOLDER, "lookup(512,1) is not New Folder");
    c = finder_cmd_lookup(515, 1);
    CHECK(c != NULL && c->id == FCMD_CLEANUP, "lookup(515,1) is not Clean Up");
    c = finder_cmd_lookup(514, 11);
    CHECK(c != NULL && c->id == FCMD_CLEANUP, "lookup(514,11) is not Clean Up");
    CHECK(finder_cmd_lookup(512, 3) == NULL, "lookup(512,3) (Print) should be unknown");
    CHECK(finder_cmd_lookup(516, 1) == NULL, "lookup(516,1) (Help) should be unknown");
    CHECK(finder_cmd_lookup(512, 0) == NULL, "lookup with item 0 should be unknown");
    CHECK(finder_cmd_lookup(999, 1) == NULL, "lookup of an unknown menu should be unknown");

    /* ALWAYS (NULL predicate) is live with an empty context. */
    c = finder_cmd_lookup(512, 1);
    CHECK(c != NULL && c->enabled == NULL, "New Folder should carry the ALWAYS predicate");
    CHECK(finder_cmd_enabled(c, &fx) == 1, "ALWAYS should be live on an empty context");

    /* The three real predicates against both states. */
    CHECK(finder_pred_has_selection(&fx) == 0, "HAS_SELECTION with sel=0 should be 0");
    fx.selection_count = 1;
    CHECK(finder_pred_has_selection(&fx) == 1, "HAS_SELECTION with sel=1 should be 1");

    CHECK(finder_pred_trash_nonempty(&fx) == 0, "TRASH_NONEMPTY on an empty Trash should be 0");
    fx.trash_nonempty = 1;
    CHECK(finder_pred_trash_nonempty(&fx) == 1, "TRASH_NONEMPTY on a full Trash should be 1");

    CHECK(finder_pred_front_is_diskwin(&fx) == 0, "FRONT_IS_DISKWIN on the desktop should be 0");
    fx.front_is_diskwin = 1;
    CHECK(finder_pred_front_is_diskwin(&fx) == 1, "FRONT_IS_DISKWIN with a disk window should be 1");
}

int main(void)
{
    leg_trace_golden();
    leg_structure();
    leg_lookup_and_predicates();
    return TEST_SUMMARY("test-finder-cmd");
}

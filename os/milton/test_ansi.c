/* test_ansi.c -- host unit oracle for the ANSI.SYS escape-sequence FSM
 *                (ansi.c / ansi.h).
 *
 * beads: initech-x3mh (ANSI.SYS escape-sequence interpreter -- LOGIC half).
 * Ref:   MS-DOS 3.3 Technical Reference Chapter 4 "ANSI.SYS";
 *        Ralf Brown's Interrupt List -- ANSI.SYS section;
 *        ECMA-48 (5th ed., 1991) for the CSI parameter grammar;
 *        IBM PC Technical Reference (1984) Appendix B "Display Adapter"
 *        for the CGA text-mode attribute byte encoding.
 *        CLAUDE.md Law 2 (oracle is truth), Rule 1 (RED->GREEN), Rule 6
 *        (mutation-proven), Rule 12 (ASCII).
 *
 * Compiles HOSTED by #including ansi.c directly (same TU trick as
 * test_batch.c / test_env.c / test_mz.c).  ansi.c is pure + I/O-free;
 * no kernel defines required.
 *
 * MUTATION (Rule 6) -- driven by make test-ansi-mutant:
 *   -DANSI_MUTATE_PARAM_ACCUM  : multi-digit params accumulate as last-digit
 *                                only (x1 instead of x10); the cursor-position
 *                                and cursor-up-3 tests go RED.
 *   -DANSI_MUTATE_SGR_COLOR    : CGA nibble = raw ANSI index (no lookup swap);
 *                                the bright-red SGR and multi-param SGR tests
 *                                go RED (red nibble is 1, not 4).
 *
 * When compiled with any mutant flag this binary MUST exit non-zero.
 * The clean build MUST exit 0 and print "<n> checks, 0 failures".
 */

#include <stdint.h>
#include <string.h>   /* strcmp -- libc OK in host test (Law 3) */
#include <stdio.h>

#include "ansi.h"
#include "test_assert.h"

/* Pull in the real artifact source (same TU trick as test_env.c).  No
 * KERNEL define needed; ansi.c is pure and I/O-free.  Mutation flags
 * propagate automatically from the gcc command line. */
#include "ansi.c"

TEST_HARNESS();

/* ===========================================================================
 * Action-capture harness
 *
 * Feed bytes through ansi_feed() and capture the resulting action stream
 * into a bounded array for inspection.
 * =========================================================================*/

#define MAX_ACTIONS  64

typedef struct {
    ansi_action_t acts[MAX_ACTIONS];
    int           count;
    int           overflow;   /* set if more than MAX_ACTIONS were emitted */
} action_log_t;

static void log_cb(const ansi_action_t *act, void *ctx)
{
    action_log_t *log = (action_log_t *)ctx;
    if (log->count >= MAX_ACTIONS) {
        log->overflow = 1;
        return;
    }
    log->acts[log->count++] = *act;
}

/* Reset the log to empty. */
static void log_reset(action_log_t *log)
{
    log->count    = 0;
    log->overflow = 0;
}

/* Feed a NUL-terminated string of bytes (including embedded NULs would need
 * explicit length; for NUL-terminated strings this suffices). */
static void feed_str(ansi_state_t *st, const char *s, action_log_t *log)
{
    while (*s != '\0') {
        ansi_feed(st, (uint8_t)*s, log_cb, log);
        s++;
    }
}

/* Feed a byte sequence of explicit length (for sequences containing NUL). */
static void feed_bytes(ansi_state_t *st, const uint8_t *b, int len,
                       action_log_t *log)
{
    int i;
    for (i = 0; i < len; i++) {
        ansi_feed(st, b[i], log_cb, log);
    }
}

/* ===========================================================================
 * Helper: find the nth action of a given kind in the log.
 * Returns a pointer to it or NULL if not found.
 * =========================================================================*/
static const ansi_action_t *find_action(const action_log_t *log,
                                        ansi_action_kind_t kind, int nth)
{
    int found = 0;
    int i;
    for (i = 0; i < log->count; i++) {
        if (log->acts[i].kind == kind) {
            if (found == nth) {
                return &log->acts[i];
            }
            found++;
        }
    }
    return 0;
}

/* ===========================================================================
 * test_erase_display
 *
 * ESC[2J must produce ANSI_ACT_ERASE_DISPLAY with erase_mode == 2.
 * Ref: MS-DOS 3.3 Technical Reference Ch 4 "ED": n=2 = erase entire screen.
 * =========================================================================*/
static void test_erase_display(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC [ 2 J */
    const uint8_t seq[] = { 0x1Bu, '[', '2', 'J' };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);

    CHECK(log.count == 1, "ESC[2J: exactly 1 action");
    const ansi_action_t *a = find_action(&log, ANSI_ACT_ERASE_DISPLAY, 0);
    CHECK(a != 0, "ESC[2J: ERASE_DISPLAY action present");
    if (a) {
        CHECK(a->erase_mode == 2,
              "ESC[2J: erase_mode == 2 (whole screen)");
    }

    /* ESC[J with no param -> default 0 (cursor to end). */
    log_reset(&log);
    ansi_init(&st, 25, 80);
    const uint8_t seq0[] = { 0x1Bu, '[', 'J' };
    feed_bytes(&st, seq0, (int)sizeof(seq0), &log);
    a = find_action(&log, ANSI_ACT_ERASE_DISPLAY, 0);
    CHECK(a != 0, "ESC[J (no param): ERASE_DISPLAY action present");
    if (a) {
        CHECK(a->erase_mode == 0, "ESC[J (no param): erase_mode == 0");
    }

    /* ESC[1J -> erase start to cursor. */
    log_reset(&log);
    ansi_init(&st, 25, 80);
    const uint8_t seq1[] = { 0x1Bu, '[', '1', 'J' };
    feed_bytes(&st, seq1, (int)sizeof(seq1), &log);
    a = find_action(&log, ANSI_ACT_ERASE_DISPLAY, 0);
    CHECK(a != 0, "ESC[1J: ERASE_DISPLAY action present");
    if (a) {
        CHECK(a->erase_mode == 1, "ESC[1J: erase_mode == 1");
    }
}

/* ===========================================================================
 * test_cursor_position
 *
 * ESC[10;5H must set cursor to row 9 col 4 (0-based; ANSI is 1-based).
 *
 * IMPORTANT off-by-one documentation:
 *   ANSI sequences use 1-based row and column numbering (ECMA-48 Sec 4.2).
 *   ansi.c converts to 0-based BEFORE emitting ANSI_ACT_MOVE_CURSOR so the
 *   caller (console driver) does NOT re-adjust.  ESC[10;5H -> row=9, col=4.
 *   ESC[1;1H -> row=0, col=0 (home).
 *   ESC[;H (both absent) -> defaults row=1, col=1 -> row=0, col=0.
 *
 * Mutation proof: with ANSI_MUTATE_PARAM_ACCUM the "10" is accumulated as
 * "1*1+0 = 1" so row becomes 0 (not 9) -> the CHECK(a->row == 9) goes RED.
 * =========================================================================*/
static void test_cursor_position(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC [ 1 0 ; 5 H */
    const uint8_t seq[] = { 0x1Bu, '[', '1', '0', ';', '5', 'H' };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);

    CHECK(log.count == 1, "ESC[10;5H: exactly 1 action");
    const ansi_action_t *a = find_action(&log, ANSI_ACT_MOVE_CURSOR, 0);
    CHECK(a != 0, "ESC[10;5H: MOVE_CURSOR action present");
    if (a) {
        /* ANSI 1-based -> 0-based: row 10 -> 9, col 5 -> 4.
         * RED under ANSI_MUTATE_PARAM_ACCUM (row would be 0 not 9). */
        CHECK(a->row == 9,
              "ESC[10;5H: row == 9 (0-based) "
              "(RED under ANSI_MUTATE_PARAM_ACCUM)");
        CHECK(a->col == 4, "ESC[10;5H: col == 4 (0-based)");
    }

    /* ESC[1;1H -> home (row=0, col=0). */
    log_reset(&log);
    ansi_init(&st, 25, 80);
    const uint8_t home[] = { 0x1Bu, '[', '1', ';', '1', 'H' };
    feed_bytes(&st, home, (int)sizeof(home), &log);
    a = find_action(&log, ANSI_ACT_MOVE_CURSOR, 0);
    CHECK(a != 0, "ESC[1;1H: MOVE_CURSOR action present");
    if (a) {
        CHECK(a->row == 0, "ESC[1;1H: row == 0 (home)");
        CHECK(a->col == 0, "ESC[1;1H: col == 0 (home)");
    }

    /* ESC[H (no params) -> default 1;1 -> row=0, col=0. */
    log_reset(&log);
    ansi_init(&st, 25, 80);
    const uint8_t bare[] = { 0x1Bu, '[', 'H' };
    feed_bytes(&st, bare, (int)sizeof(bare), &log);
    a = find_action(&log, ANSI_ACT_MOVE_CURSOR, 0);
    CHECK(a != 0, "ESC[H (bare): MOVE_CURSOR action present");
    if (a) {
        CHECK(a->row == 0, "ESC[H: row == 0");
        CHECK(a->col == 0, "ESC[H: col == 0");
    }

    /* ESC[<r>;<c>f (synonym for H): same test with 'f' final byte. */
    log_reset(&log);
    ansi_init(&st, 25, 80);
    const uint8_t fseq[] = { 0x1Bu, '[', '3', ';', '7', 'f' };
    feed_bytes(&st, fseq, (int)sizeof(fseq), &log);
    a = find_action(&log, ANSI_ACT_MOVE_CURSOR, 0);
    CHECK(a != 0, "ESC[3;7f: MOVE_CURSOR action present");
    if (a) {
        CHECK(a->row == 2, "ESC[3;7f: row == 2 (0-based)");
        CHECK(a->col == 6, "ESC[3;7f: col == 6 (0-based)");
    }
}

/* ===========================================================================
 * test_sgr_bright_red
 *
 * ESC[31;1m must set bright-red foreground.
 * Grounded CGA attribute calculation:
 *   SGR 31: fg colour red -> CGA nibble 4 (ANSI_TO_CGA[1]=4).
 *   SGR 1 (bold): set bit 3 (bright).
 *   Combined fg nibble = 0x4 | 0x8 = 0xC; bg = 0 (unchanged from reset).
 *   Expected attribute byte = 0x0C.
 *
 * Ref: IBM PC Technical Reference Appendix B attribute byte:
 *   bits 3:0 = fg (0xC = bright red), bits 6:4 = bg (0x0 = black).
 *
 * Mutation proof: with ANSI_MUTATE_SGR_COLOR the red nibble is written as
 * raw ANSI index 1 instead of CGA nibble 4, yielding 0x0B (bright blue) not
 * 0x0C (bright red) -> the CHECK(a->attr == 0x0C) goes RED.
 * =========================================================================*/
static void test_sgr_bright_red(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC [ 3 1 ; 1 m -- red fg, then bold */
    const uint8_t seq[] = { 0x1Bu, '[', '3', '1', ';', '1', 'm' };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);

    /* Should produce exactly one SET_ATTR action. */
    const ansi_action_t *a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[31;1m: SET_ATTR action present");
    if (a) {
        /* CGA bright red: fg=4 | bright=8 -> nibble 0xC; bg=0.
         * Attribute byte = 0x0C.
         * RED under ANSI_MUTATE_SGR_COLOR (attr would be 0x09 or 0x0B). */
        CHECK(a->attr == 0x0Cu,
              "ESC[31;1m: attr == 0x0C (bright red fg, black bg) "
              "(RED under ANSI_MUTATE_SGR_COLOR)");
    }

    /* Also verify that SGR 0 resets to default. */
    log_reset(&log);
    const uint8_t reset[] = { 0x1Bu, '[', '0', 'm' };
    feed_bytes(&st, reset, (int)sizeof(reset), &log);
    a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[0m: SET_ATTR action present");
    if (a) {
        CHECK(a->attr == ANSI_ATTR_DEFAULT,
              "ESC[0m: attr == 0x07 (default: light grey on black)");
    }
}

/* ===========================================================================
 * test_cursor_up
 *
 * ESC[A with no param defaults to 1; ESC[3A moves up 3.
 *
 * Mutation proof: with ANSI_MUTATE_PARAM_ACCUM, "3" parses as 3 (single digit,
 * unaffected) but "30A" would give 3 instead of 30.  The multi-digit test
 * in test_cursor_position covers the accumulator more directly; here we add
 * a two-digit case to be thorough.
 * =========================================================================*/
static void test_cursor_up(void)
{
    ansi_state_t st;
    action_log_t log;

    /* ESC[A (no param) -> up 1. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t up1[] = { 0x1Bu, '[', 'A' };
    feed_bytes(&st, up1, (int)sizeof(up1), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[A (no param): CURSOR_REL action present");
    if (a) {
        CHECK(a->delta_row == -1,
              "ESC[A (no param): delta_row == -1 (default=1)");
        CHECK(a->delta_col == 0, "ESC[A: delta_col == 0");
    }

    /* ESC[3A -> up 3. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t up3[] = { 0x1Bu, '[', '3', 'A' };
    feed_bytes(&st, up3, (int)sizeof(up3), &log);
    a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[3A: CURSOR_REL action present");
    if (a) {
        CHECK(a->delta_row == -3, "ESC[3A: delta_row == -3");
    }

    /* ESC[15A -- two-digit param; RED under ANSI_MUTATE_PARAM_ACCUM. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t up15[] = { 0x1Bu, '[', '1', '5', 'A' };
    feed_bytes(&st, up15, (int)sizeof(up15), &log);
    a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[15A: CURSOR_REL action present");
    if (a) {
        /* RED under ANSI_MUTATE_PARAM_ACCUM: delta would be -6 not -15. */
        CHECK(a->delta_row == -15,
              "ESC[15A: delta_row == -15 (RED under ANSI_MUTATE_PARAM_ACCUM)");
    }

    /* ESC[B, C, D (down, right, left) with default param. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t down[] = { 0x1Bu, '[', 'B' };
    feed_bytes(&st, down, (int)sizeof(down), &log);
    a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[B: CURSOR_REL present");
    if (a) { CHECK(a->delta_row == 1, "ESC[B: delta_row == 1"); }

    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t right[] = { 0x1Bu, '[', 'C' };
    feed_bytes(&st, right, (int)sizeof(right), &log);
    a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[C: CURSOR_REL present");
    if (a) { CHECK(a->delta_col == 1, "ESC[C: delta_col == 1"); }

    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t left[] = { 0x1Bu, '[', 'D' };
    feed_bytes(&st, left, (int)sizeof(left), &log);
    a = find_action(&log, ANSI_ACT_CURSOR_REL, 0);
    CHECK(a != 0, "ESC[D: CURSOR_REL present");
    if (a) { CHECK(a->delta_col == -1, "ESC[D: delta_col == -1"); }
}

/* ===========================================================================
 * test_sgr_multi
 *
 * ESC[1;33;44m must set: bold (bit 3), yellow fg (CGA 6), blue bg (CGA 1).
 * Expected attribute byte:
 *   fg: ANSI_TO_CGA[3] (yellow->6) | bright (0x8) = 0xE
 *   bg: ANSI_TO_CGA[4] (blue->1) << 4 = 0x10
 *   => attr = 0x10 | 0x0E = 0x1E
 *
 * Ref: IBM PC Technical Reference Appendix B; RBIL ANSI.SYS colour table.
 * Mutation proof: ANSI_MUTATE_SGR_COLOR gives fg nibble=3 (raw yellow) and
 * bg nibble=4 (raw blue) -> attr = (4<<4)|3|8 = 0x4B != 0x1E -> RED.
 * =========================================================================*/
static void test_sgr_multi(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC [ 1 ; 3 3 ; 4 4 m */
    const uint8_t seq[] = {
        0x1Bu, '[', '1', ';', '3', '3', ';', '4', '4', 'm'
    };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);

    const ansi_action_t *a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[1;33;44m: SET_ATTR action present");
    if (a) {
        /* bold: bit3=0x8; yellow fg: CGA 6 -> 0x6; blue bg: CGA 1 -> 0x10.
         * attr = 0x10 | 0x08 | 0x06 = 0x1E.
         * RED under ANSI_MUTATE_SGR_COLOR. */
        CHECK(a->attr == 0x1Eu,
              "ESC[1;33;44m: attr == 0x1E (bold yellow on blue) "
              "(RED under ANSI_MUTATE_SGR_COLOR)");
    }

    /* ESC[m (bare) -> reset to default 0x07. */
    log_reset(&log);
    const uint8_t bare_m[] = { 0x1Bu, '[', 'm' };
    feed_bytes(&st, bare_m, (int)sizeof(bare_m), &log);
    a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[m (bare): SET_ATTR action present");
    if (a) {
        CHECK(a->attr == ANSI_ATTR_DEFAULT,
              "ESC[m (bare): attr == 0x07 (reset)");
    }
}

/* ===========================================================================
 * test_save_restore_cursor
 *
 * ESC[s saves cursor; movement; ESC[u restores it.
 * =========================================================================*/
static void test_save_restore_cursor(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* Position cursor at (5, 10) with ESC[6;11H (1-based -> 0-based). */
    const uint8_t pos[] = { 0x1Bu, '[', '6', ';', '1', '1', 'H' };
    feed_bytes(&st, pos, (int)sizeof(pos), &log);

    /* Save cursor: ESC[s */
    log_reset(&log);
    const uint8_t save[] = { 0x1Bu, '[', 's' };
    feed_bytes(&st, save, (int)sizeof(save), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_SAVE_CURSOR, 0);
    CHECK(a != 0, "ESC[s: SAVE_CURSOR action present");
    /* Verify saved values are in the state. */
    CHECK(st.saved_row == 5, "ESC[s: saved_row == 5");
    CHECK(st.saved_col == 10, "ESC[s: saved_col == 10");

    /* Move to a different position: ESC[1;1H */
    log_reset(&log);
    const uint8_t home[] = { 0x1Bu, '[', '1', ';', '1', 'H' };
    feed_bytes(&st, home, (int)sizeof(home), &log);

    /* Restore cursor: ESC[u */
    log_reset(&log);
    const uint8_t rest[] = { 0x1Bu, '[', 'u' };
    feed_bytes(&st, rest, (int)sizeof(rest), &log);
    a = find_action(&log, ANSI_ACT_RESTORE_CURSOR, 0);
    CHECK(a != 0, "ESC[u: RESTORE_CURSOR action present");
    if (a) {
        CHECK(a->row == 5, "ESC[u: restored row == 5");
        CHECK(a->col == 10, "ESC[u: restored col == 10");
    }
    /* FSM state must reflect the restored cursor. */
    CHECK(st.row == 5, "ESC[u: st.row == 5 after restore");
    CHECK(st.col == 10, "ESC[u: st.col == 10 after restore");
}

/* ===========================================================================
 * test_plain_passthrough
 *
 * A plain string with no escapes emits exactly one PUT_CHAR per byte and
 * the cursor advances.
 * =========================================================================*/
static void test_plain_passthrough(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* Feed "HI" (two chars, no escapes). */
    feed_str(&st, "HI", &log);

    CHECK(log.count == 2, "plain 'HI': exactly 2 actions");
    CHECK(log.overflow == 0, "plain 'HI': no overflow");

    /* Both actions must be PUT_CHAR. */
    const ansi_action_t *a0 = find_action(&log, ANSI_ACT_PUT_CHAR, 0);
    const ansi_action_t *a1 = find_action(&log, ANSI_ACT_PUT_CHAR, 1);
    CHECK(a0 != 0, "plain 'HI': first action is PUT_CHAR");
    CHECK(a1 != 0, "plain 'HI': second action is PUT_CHAR");
    if (a0) { CHECK(a0->ch == 'H', "plain 'HI': first char == 'H'"); }
    if (a1) { CHECK(a1->ch == 'I', "plain 'HI': second char == 'I'"); }

    /* Both must carry the default attribute 0x07. */
    if (a0) { CHECK(a0->attr == ANSI_ATTR_DEFAULT,
                    "plain 'H': attr == 0x07 (default)"); }
    if (a1) { CHECK(a1->attr == ANSI_ATTR_DEFAULT,
                    "plain 'I': attr == 0x07 (default)"); }

    /* The FSM must still be in GROUND state after plain chars. */
    CHECK(st.fsm == ANSI_ST_GROUND,
          "plain 'HI': FSM remains in GROUND state");
}

/* ===========================================================================
 * test_malformed_sequence
 *
 * A malformed/unknown sequence must be consumed without crashing and the FSM
 * must return to GROUND.
 * Ref: RBIL ANSI.SYS -- "unknown final byte ends the sequence".
 * Rule 2: never overflow; never crash on bad input.
 * =========================================================================*/
static void test_malformed_sequence(void)
{
    ansi_state_t st;
    action_log_t log;

    /* Unknown final byte (e.g. ESC[99Z -- 'Z' is a valid final byte in
     * ECMA-48 range 0x40..0x7E but not a DOS ANSI.SYS sequence). */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t unk[] = { 0x1Bu, '[', '9', '9', 'Z' };
    feed_bytes(&st, unk, (int)sizeof(unk), &log);
    CHECK(st.fsm == ANSI_ST_GROUND,
          "malformed ESC[99Z: FSM returns to GROUND");
    /* No actions emitted for an unknown sequence. */
    CHECK(log.count == 0,
          "malformed ESC[99Z: zero actions (silently consumed)");

    /* Bare ESC followed by a non-'[' byte: absorbed, back to GROUND. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t bare_esc[] = { 0x1Bu, 'X' };
    feed_bytes(&st, bare_esc, (int)sizeof(bare_esc), &log);
    CHECK(st.fsm == ANSI_ST_GROUND,
          "ESC then non-'[': FSM returns to GROUND");
    CHECK(log.count == 0,
          "ESC then non-'[': zero actions");

    /* Very long parameter string (overflow guard, Rule 2): more than
     * ANSI_PARAM_MAX semicolons -- must not overflow, FSM stays consistent. */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    /* Build ESC [ 1;1;1;1;...;1m with 30 params (> ANSI_PARAM_MAX=16). */
    {
        int k;
        ansi_feed(&st, 0x1Bu, log_cb, &log);
        ansi_feed(&st, '[',   log_cb, &log);
        for (k = 0; k < 30; k++) {
            ansi_feed(&st, '1', log_cb, &log);
            if (k < 29) {
                ansi_feed(&st, ';', log_cb, &log);
            }
        }
        ansi_feed(&st, 'm', log_cb, &log);
    }
    /* Must have returned to GROUND and not crashed. */
    CHECK(st.fsm == ANSI_ST_GROUND,
          "oversized param list: FSM returns to GROUND");
    /* A SET_ATTR action must have been emitted (the params that fit are fine). */
    const ansi_action_t *a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "oversized param list: SET_ATTR still emitted (for params that fit)");

    /* ESC with no following '[': multiple ESC bytes in a row (re-arm). */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    ansi_feed(&st, 0x1Bu, log_cb, &log);
    ansi_feed(&st, 0x1Bu, log_cb, &log);
    ansi_feed(&st, '[',   log_cb, &log);
    ansi_feed(&st, '2',   log_cb, &log);
    ansi_feed(&st, 'J',   log_cb, &log);
    a = find_action(&log, ANSI_ACT_ERASE_DISPLAY, 0);
    CHECK(a != 0,
          "double-ESC then [2J: second ESC re-arms, ESC[2J parses correctly");
    if (a) { CHECK(a->erase_mode == 2, "double-ESC [2J: erase_mode==2"); }
}

/* ===========================================================================
 * test_erase_line
 *
 * ESC[K (default 0) -> erase to end of line.
 * ESC[2K -> erase whole line.
 * =========================================================================*/
static void test_erase_line(void)
{
    ansi_state_t st;
    action_log_t log;

    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t ek[] = { 0x1Bu, '[', 'K' };
    feed_bytes(&st, ek, (int)sizeof(ek), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_ERASE_LINE, 0);
    CHECK(a != 0, "ESC[K: ERASE_LINE action present");
    if (a) { CHECK(a->erase_mode == 0, "ESC[K: erase_mode == 0 (to end)"); }

    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t ek2[] = { 0x1Bu, '[', '2', 'K' };
    feed_bytes(&st, ek2, (int)sizeof(ek2), &log);
    a = find_action(&log, ANSI_ACT_ERASE_LINE, 0);
    CHECK(a != 0, "ESC[2K: ERASE_LINE action present");
    if (a) { CHECK(a->erase_mode == 2, "ESC[2K: erase_mode == 2 (whole line)"); }
}

/* ===========================================================================
 * test_device_status_report
 *
 * ESC[6n must emit ANSI_ACT_DEVICE_STATUS (with current cursor in the action).
 * Other 'n' codes are consumed silently.
 * =========================================================================*/
static void test_device_status_report(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* Position to row 3, col 7 first (ESC[4;8H, 1-based). */
    const uint8_t pos[] = { 0x1Bu, '[', '4', ';', '8', 'H' };
    feed_bytes(&st, pos, (int)sizeof(pos), &log);
    log_reset(&log);

    /* ESC[6n: Device Status Report. */
    const uint8_t dsr[] = { 0x1Bu, '[', '6', 'n' };
    feed_bytes(&st, dsr, (int)sizeof(dsr), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_DEVICE_STATUS, 0);
    CHECK(a != 0, "ESC[6n: DEVICE_STATUS action present");
    if (a) {
        CHECK(a->row == 3, "ESC[6n: reported row == 3 (0-based)");
        CHECK(a->col == 7, "ESC[6n: reported col == 7 (0-based)");
    }
    CHECK(st.fsm == ANSI_ST_GROUND,
          "ESC[6n: FSM returns to GROUND");
}

/* ===========================================================================
 * test_sgr_reverse_and_conceal
 *
 * ESC[7m (reverse video) swaps fg and bg nibbles.
 * ESC[8m (conceal) sets fg == bg (text invisible).
 * =========================================================================*/
static void test_sgr_reverse_and_conceal(void)
{
    ansi_state_t st;
    action_log_t log;

    /* Start from known state: SGR 37;42m (white fg=7, green bg=2). */
    ansi_init(&st, 25, 80);
    log_reset(&log);
    const uint8_t setup[] = { 0x1Bu, '[', '3', '7', ';', '4', '2', 'm' };
    feed_bytes(&st, setup, (int)sizeof(setup), &log);
    /* Expected: fg=CGA 7, bg=CGA 2 -> attr = (2<<4)|7 = 0x27. */
    CHECK(st.attr == 0x27u, "setup 37;42m: attr == 0x27");

    /* Now ESC[7m: reverse -> swap fg(7) and bg(2). */
    log_reset(&log);
    const uint8_t rev[] = { 0x1Bu, '[', '7', 'm' };
    feed_bytes(&st, rev, (int)sizeof(rev), &log);
    /* After reverse: fg=2, bg=7 -> attr = (7<<4)|2 = 0x72. */
    const ansi_action_t *a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[7m: SET_ATTR present");
    if (a) {
        CHECK(a->attr == 0x72u,
              "ESC[7m (reverse after 37;42m): attr == 0x72 (fg=2 bg=7)");
    }

    /* Now ESC[8m: conceal -> set fg=bg (fg = 7, bg = 7). */
    log_reset(&log);
    const uint8_t conceal[] = { 0x1Bu, '[', '8', 'm' };
    feed_bytes(&st, conceal, (int)sizeof(conceal), &log);
    /* fg nibble (bits 2:0) set to current bg nibble (7); bright preserved. */
    /* Before conceal: attr = 0x72; fg=2, bg=7.
     * Conceal: fg := bg = 7; attr = 0x77. */
    a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[8m: SET_ATTR present");
    if (a) {
        CHECK(a->attr == 0x77u,
              "ESC[8m (conceal after reverse): attr == 0x77 (fg=bg=7)");
    }
}

/* ===========================================================================
 * test_blink_and_bold
 *
 * SGR 1 sets bit 3 (bright/bold); SGR 5 sets bit 7 (blink).
 * =========================================================================*/
static void test_blink_and_bold(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC[5m: blink -> bit 7. Starting from 0x07. */
    const uint8_t blink[] = { 0x1Bu, '[', '5', 'm' };
    feed_bytes(&st, blink, (int)sizeof(blink), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[5m: SET_ATTR present");
    if (a) {
        CHECK(a->attr == 0x87u,
              "ESC[5m: attr == 0x87 (blink set, default fg/bg)");
    }

    /* Reset then ESC[1m: bold -> bit 3. */
    log_reset(&log);
    const uint8_t rst[] = { 0x1Bu, '[', '0', 'm' };
    feed_bytes(&st, rst, (int)sizeof(rst), &log);
    log_reset(&log);
    const uint8_t bold[] = { 0x1Bu, '[', '1', 'm' };
    feed_bytes(&st, bold, (int)sizeof(bold), &log);
    a = find_action(&log, ANSI_ACT_SET_ATTR, 0);
    CHECK(a != 0, "ESC[1m: SET_ATTR present");
    if (a) {
        CHECK(a->attr == 0x0Fu,
              "ESC[1m: attr == 0x0F (bright white on black)");
    }
}

/* ===========================================================================
 * test_interleaved_text_and_escapes
 *
 * A realistic sequence: text, then colour change, then more text.
 * Verifies the attr field on PUT_CHAR actions follows the current SGR state.
 * =========================================================================*/
static void test_interleaved_text_and_escapes(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* "A" ESC[31m "B" ESC[0m "C"
     * Expected: PUT_CHAR 'A' attr=0x07; SET_ATTR 0x04; PUT_CHAR 'B' attr=0x04;
     *           SET_ATTR 0x07; PUT_CHAR 'C' attr=0x07. */
    const uint8_t seq[] = {
        'A',
        0x1Bu, '[', '3', '1', 'm',   /* red fg: attr -> 0x04 */
        'B',
        0x1Bu, '[', '0', 'm',         /* reset: attr -> 0x07 */
        'C'
    };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);

    /* Find PUT_CHAR actions. */
    const ansi_action_t *pA = find_action(&log, ANSI_ACT_PUT_CHAR, 0);
    const ansi_action_t *pB = find_action(&log, ANSI_ACT_PUT_CHAR, 1);
    const ansi_action_t *pC = find_action(&log, ANSI_ACT_PUT_CHAR, 2);
    CHECK(pA != 0, "interleaved: PUT_CHAR 'A' present");
    CHECK(pB != 0, "interleaved: PUT_CHAR 'B' present");
    CHECK(pC != 0, "interleaved: PUT_CHAR 'C' present");
    if (pA) {
        CHECK(pA->ch == 'A', "interleaved: first PUT_CHAR is 'A'");
        CHECK(pA->attr == 0x07u,
              "interleaved: 'A' carries default attr 0x07");
    }
    if (pB) {
        CHECK(pB->ch == 'B', "interleaved: second PUT_CHAR is 'B'");
        /* After ESC[31m: ANSI red -> CGA 4; attr = 0x04.
         * Not affected by mutant since we are just checking the char. */
        CHECK(pB->attr == 0x04u,
              "interleaved: 'B' carries red attr 0x04");
    }
    if (pC) {
        CHECK(pC->ch == 'C', "interleaved: third PUT_CHAR is 'C'");
        CHECK(pC->attr == 0x07u,
              "interleaved: 'C' carries reset attr 0x07");
    }
}

/* ===========================================================================
 * test_key_remap
 *
 * ESC[...p must emit ANSI_ACT_KEY_REMAP (classified, not executed in the
 * pure module).  The FSM must return to GROUND.
 *
 * Note on DOS ANSI.SYS keyboard reassignment syntax:
 *   ESC[<key>;<string>p  reassigns a key.  Ref: MS-DOS 3.3 Technical
 *   Reference Ch 4 "Keyboard Key Reassignment".
 *   The string form uses ASCII 34 (0x22, '"') as delimiters followed by
 *   ASCII bytes.  However, bytes in the range 0x40..0x7E are ECMA-48 final
 *   bytes and would dispatch as CSI sequences; the real MS-DOS 3.3 ANSI.SYS
 *   driver handles this by treating 0x22-delimited spans specially.  Our pure
 *   FSM implements ECMA-48 strictly for the parameter byte range; the quoted-
 *   string format is deferred to the wiring bead.  We test the numeric-only
 *   form here: ESC[0;59;13p (numeric params only, no quoted string).
 * =========================================================================*/
static void test_key_remap(void)
{
    ansi_state_t st;
    action_log_t log;
    ansi_init(&st, 25, 80);
    log_reset(&log);

    /* ESC[0;59;13p -- numeric key reassignment (scancode 59 -> Enter 13).
     * All bytes are in the parameter/separator range 0x30..0x3B so they
     * accumulate cleanly; 'p' (0x70) is the final byte. */
    const uint8_t seq[] = {
        0x1Bu, '[', '0', ';', '5', '9', ';', '1', '3', 'p'
    };
    feed_bytes(&st, seq, (int)sizeof(seq), &log);
    const ansi_action_t *a = find_action(&log, ANSI_ACT_KEY_REMAP, 0);
    CHECK(a != 0, "ESC[0;59;13p: KEY_REMAP action present");
    CHECK(st.fsm == ANSI_ST_GROUND, "ESC[0;59;13p: FSM returns to GROUND");
}

/* ===========================================================================
 * main
 * =========================================================================*/

int main(void)
{
    test_erase_display();
    test_cursor_position();
    test_sgr_bright_red();
    test_cursor_up();
    test_sgr_multi();
    test_save_restore_cursor();
    test_plain_passthrough();
    test_malformed_sequence();
    test_erase_line();
    test_device_status_report();
    test_sgr_reverse_and_conceal();
    test_blink_and_bold();
    test_interleaved_text_and_escapes();
    test_key_remap();
    return TEST_SUMMARY("test_ansi");
}

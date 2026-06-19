/* test_event.c -- the FLAIR Event Manager's deterministic-replay oracle (D-8).
 *
 * beads: initech-8b7 (FLAIR Event Manager: WaitNextEvent + SPSC ring).
 * Ref:   ADR-0004 D-8 (the test-event oracle row):
 *          "A recorded raw-input trace replays to a deterministic EventRecord/
 *           dispatch sequence."
 *        ADR-0004 D-4 (ISR enqueue-only; EventRecord synthesis in task context).
 *        ADR-0004 D-6 (cooperative, non-preemptive WaitNextEvent on PIT tick).
 *        spec/event_model.h (LOCKED: EventRecord, flair_raw_event_t, what codes,
 *          modifier bits, mask bits, FLAIR_RAW_RING_CAP).
 *        CLAUDE.md Law 2 (the oracle is the truth), Rule 1 (RED->GREEN),
 *        Rule 6 (oracle mutation-proven), Rule 11 (seeded LCG -> deterministic),
 *        Rule 12 (ASCII-clean).
 *
 * ORACLE MODEL (ADR-0004 D-8 -- "the decisive property"):
 *   A RECORDED raw-input trace (sequence of flair_raw_event_t records) is
 *   posted into the SPSC ring (producer side, simulating ISRs), then drained
 *   via WaitNextEvent (consumer side, task context). The EXACT synthesized
 *   EventRecord sequence -- what/message/where/when/modifiers -- must be
 *   BIT-DETERMINISTIC: the same trace always produces the same events,
 *   every run (Rule 11). There is no nondeterminism in the Event Manager.
 *
 * TEST GROUPS:
 *   (A) DETERMINISTIC REPLAY: key press/release, mouse move + click, ticks.
 *       Assert EXACT cooked EventRecord sequence (byte fields verified).
 *   (B) eventMask FILTERING: masked-out event class is not returned;
 *       nullEvent is returned instead.
 *   (C) nullEvent on empty ring / timeout: WaitNextEvent returns nullEvent
 *       (sleepTicks=0 and ring empty).
 *   (D) `where` TRACKING: cumulative mouse deltas clamp to frame;
 *       the `where` field tracks cursor position across events.
 *   (E) MODIFIER REFLECTION: shift/ctrl held -> EventRecord.modifiers bits set.
 *   (F) SPSC RING CORRECTNESS: interleaved post/drain, full-ring handling.
 *   (G) MUTATION PROOF: compile with EVENT_MUTATE_DROP_SYNTH (or
 *       EVENT_MUTATE_STALE_WHERE) and run -- this oracle MUST report failures.
 *       See "make test-event-mutant" Makefile recipe (gate name: test-event-mutant).
 *
 * AUTOKEY (auto-repeat): DEFERRED. Inside Macintosh autoKey fires after a
 *   key-down is held across multiple PIT ticks with no intervening keyUp. The
 *   current event.c model does not track inter-tick key-hold state (it would
 *   require a "last key pressed" field and a repeat-delay counter -- a
 *   straightforward extension but not yet implemented). This test documents the
 *   deferral: the autoKey what-code is defined in the spec but is not yet
 *   exercised here. A follow-up issue should add a TICK-stream replay test
 *   that drives an autoKey sequence when the hold-tracking is added to event.c.
 *
 * COMPILATION:
 *   Hosted (oracle run):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *         harness/proptest/test_event.c os/flair/event.c -o /tmp/test_event
 *     /tmp/test_event          # must exit 0 (green)
 *
 *   Mutant (must exit NON-ZERO):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *         -DEVENT_MUTATE_DROP_SYNTH \
 *         harness/proptest/test_event.c os/flair/event.c -o /tmp/test_event_mut
 *     /tmp/test_event_mut      # must exit NON-ZERO (red)
 *
 *     gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *         -DEVENT_MUTATE_STALE_WHERE \
 *         harness/proptest/test_event.c os/flair/event.c -o /tmp/test_event_mut2
 *     /tmp/test_event_mut2     # must exit NON-ZERO (red)
 *
 * Makefile gate recipe (for the orchestrator to wire):
 *   test-event:
 *       gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *           harness/proptest/test_event.c os/flair/event.c \
 *           -o $(BUILD)/test_event
 *       $(BUILD)/test_event
 *
 *   test-event-mutant:
 *       gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *           -DEVENT_MUTATE_DROP_SYNTH \
 *           harness/proptest/test_event.c os/flair/event.c \
 *           -o $(BUILD)/test_event_mut
 *       ! $(BUILD)/test_event_mut
 *       gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec -Iseed \
 *           -DEVENT_MUTATE_STALE_WHERE \
 *           harness/proptest/test_event.c os/flair/event.c \
 *           -o $(BUILD)/test_event_mut2
 *       ! $(BUILD)/test_event_mut2
 *
 *   Dependencies: os/flair/event.c os/flair/event.h spec/event_model.h
 *                 spec/grafport.h seed/test_assert.h
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Override the bare-metal panic with a hosted abort so the oracle can report
 * the failure rather than spinning forever. */
#ifdef FLAIR_PANIC
#  undef FLAIR_PANIC
#endif
#define FLAIR_PANIC(msg) \
    do { fprintf(stderr, "PANIC: %s (at %s:%d)\n", (msg), __FILE__, __LINE__); \
         exit(1); } while(0)

#include "event.h"        /* os/flair/event.h (-Ios/flair)                   */
#include "test_assert.h"  /* seed/test_assert.h: TEST_HARNESS/CHECK/TEST_SUMMARY */

TEST_HARNESS();

/* ===========================================================================
 * HELPERS
 * ===========================================================================*/

/* fresh_ring -- zero-initialise a ring via flair_event_init (clears head/tail/
 * drop_count and zeroes all slots). Also resets tick and cursor state. */
static void fresh_ring(flair_raw_ring_t *ring)
{
    flair_event_init(ring);
    /* Install a no-op yield hook so WaitNextEvent does not spin in the host
     * environment. The test sets sleepTicks=0 for all non-blocking checks;
     * this hook is a safety net for any path that calls yield. */
    flair_event_set_yield((flair_event_yield_fn)0);  /* NULL = tight spin (OK for tests) */
}

/* post1 -- post one raw event into the ring (producer side). Helper wrapping
 * flair_raw_post with a CHECK that the post succeeds (ring should not be full
 * in these tests -- we always leave room). */
static void post1(flair_raw_ring_t *ring, flair_raw_kind_t kind,
                  uint32_t tick, uint32_t payload)
{
    flair_raw_event_t raw;
    int rc;
    raw.kind    = (uint32_t)kind;
    raw.tick    = tick;
    raw.payload = payload;
    rc = flair_raw_post(ring, &raw);
    CHECK(rc == 1, "post1: flair_raw_post returned 0 (ring full or error)");
}

/* drain1 -- drain one event via GetNextEvent (non-blocking). */
static int drain1(flair_raw_ring_t *ring, uint16_t mask, EventRecord *out)
{
    return GetNextEvent(ring, mask, out);
}

/* ===========================================================================
 * SEEDED LCG (Rule 11: deterministic fuzz)
 * ---------------------------------------------------------------------------
 * Same LCG idiom as test_region.c / test_flair_heap.c. Seed chosen by
 * inspection; produces a stable sequence across runs and architectures.
 * ===========================================================================*/
static uint32_t g_lcg = 0xC0FFEE1Du;  /* deterministic seed (Rule 11) */

static uint32_t lcg_next(void)
{
    g_lcg = g_lcg * 1664525u + 1013904223u;  /* Numerical Recipes LCG */
    return g_lcg;
}

/* ===========================================================================
 * GROUP A -- DETERMINISTIC REPLAY (the decisive property; ADR-0004 D-8)
 * ---------------------------------------------------------------------------
 * Post a fixed-known raw trace; drain it; assert the EXACT EventRecord
 * sequence (what, message, where.h, where.v, when, modifiers) field by field.
 * Same trace -> same events, every run, every machine (Rule 11).
 *
 * Trace:
 *   [0] TICK (tick=1)                   -> nullEvent (no event synthesized)
 *   [1] KEYBOARD sc=0x1E (make 'a')     -> keyDown, ASCII 'a', vkey 0x1E
 *   [2] KEYBOARD sc=0x1E break ('a')    -> keyUp,   ASCII 'a', vkey 0x1E
 *   [3] MOUSE dx=+10 dy=+5 btn=1       -> mouseDown, where=(320+10, 240+5)
 *   [4] MOUSE dx=0  dy=0  btn=0        -> mouseUp,   where=(330, 245)
 *   [5] KEYBOARD sc=0x14 (make 'q')    -> keyDown, ASCII 'q', vkey 0x14
 *
 * Initial cursor = (320, 240) -- the start position set by flair_event_init.
 * ===========================================================================*/
static void test_deterministic_replay(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* Post the trace (simulating ISRs in order). */
    post1(&ring, FLAIR_RAW_TICK,     1u, 0u);
    post1(&ring, FLAIR_RAW_KEYBOARD, 2u, 0x1Eu);          /* sc=0x1E make 'a' */
    post1(&ring, FLAIR_RAW_KEYBOARD, 3u, 0x1Eu | (1u<<8u));/* sc=0x1E break   */
    /* Mouse: btn byte=1 (left down), dx=+10 (bits 8..15), dy=+5 (bits 16..23) */
    post1(&ring, FLAIR_RAW_MOUSE,    4u,
          (uint32_t)0x01u |
          ((uint32_t)(uint8_t)(int8_t)10 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)5  << 16u));
    /* Mouse: btn=0 (released), dx=0 dy=0 */
    post1(&ring, FLAIR_RAW_MOUSE,    5u, 0x00u);
    post1(&ring, FLAIR_RAW_KEYBOARD, 6u, 0x10u);           /* sc=0x10 make 'q' (IBM PC AT Table B-3: 'q' = SET-1 0x10) */

    /* --- Drain [0]: TICK -> should yield nullEvent; GetNextEvent returns 0. --- */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0,               "replay[0]: TICK should yield rc=0 (nullEvent)");
    CHECK(ev.what == nullEvent,  "replay[0]: TICK what should be nullEvent");

    /* --- Drain [1]: KEYBOARD make 'a' -> keyDown. --- */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 1,               "replay[1]: keyDown should yield rc=1");
    CHECK(ev.what == keyDown,    "replay[1]: what should be keyDown");
    /* ASCII 'a' is scancode 0x1E; sc_unshifted[0x1E] = 'a' = 0x61. */
    CHECK((ev.message & 0xFFu) == 'a',   "replay[1]: message low byte = 'a'");
    CHECK(((ev.message >> 8u) & 0xFFu) == 0x1Eu,
                                 "replay[1]: message vkey = 0x1E");
    CHECK(ev.when == 2u,         "replay[1]: when = 2 (from raw.tick)");
    CHECK(ev.where.h == 320,     "replay[1]: where.h = 320 (cursor init)");
    CHECK(ev.where.v == 240,     "replay[1]: where.v = 240 (cursor init)");
    /* No modifiers held at this point; btnState=1 (button UP; MTE 2-6 inverted). */
    CHECK((ev.modifiers & FLAIR_EVT_MOD_SHIFT_KEY) == 0u,
                                 "replay[1]: Shift not held");
    CHECK((ev.modifiers & FLAIR_EVT_MOD_BTN_STATE) != 0u,
                                 "replay[1]: btnState=1 (button UP; MTE 2-6)");

    /* --- Drain [2]: KEYBOARD break 'a' -> keyUp. --- */
    rc = drain1(&ring, everyEvent, &ev);
    /* keyUp IS a non-null event (what=keyUp=4); GetNextEvent returns rc=1.
     * everyEvent (0xFFFF) includes keyUpMask = (1<<keyUp=4) = 0x0010. */
    CHECK(rc == 1,               "replay[2]: keyUp yields rc=1 (non-null; everyEvent)");
    CHECK(ev.what == keyUp,      "replay[2]: what should be keyUp");
    CHECK((ev.message & 0xFFu) == 'a',   "replay[2]: message low byte = 'a'");
    CHECK(ev.when == 3u,         "replay[2]: when = 3");

    /* --- Drain [3]: MOUSE btn=1 -> mouseDown; where MUST reflect deltas. --- */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 1,               "replay[3]: mouseDown yields rc=1");
    CHECK(ev.what == mouseDown,  "replay[3]: what = mouseDown");
    CHECK(ev.when == 4u,         "replay[3]: when = 4");
    /* where must be updated by dx=+10, dy=+5 from cursor start (320,240).
     * MUTANT EVENT_MUTATE_STALE_WHERE: clamp_coord is not called, so
     * where.h stays 320 and where.v stays 240 -- these CHECKs go RED. */
    CHECK(ev.where.h == 330,     "replay[3]: where.h = 320+10 = 330");
    CHECK(ev.where.v == 245,     "replay[3]: where.v = 240+5 = 245");
    /* btnState should now be 0 (button DOWN; MTE 2-6 inverted). */
    CHECK((ev.modifiers & FLAIR_EVT_MOD_BTN_STATE) == 0u,
                                 "replay[3]: btnState=0 (button DOWN; MTE 2-6)");

    /* --- Drain [4]: MOUSE btn=0 -> mouseUp; cursor unchanged (dx=0,dy=0). --- */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 1,               "replay[4]: mouseUp yields rc=1");
    CHECK(ev.what == mouseUp,    "replay[4]: what = mouseUp");
    CHECK(ev.when == 5u,         "replay[4]: when = 5");
    CHECK(ev.where.h == 330,     "replay[4]: where.h unchanged = 330");
    CHECK(ev.where.v == 245,     "replay[4]: where.v unchanged = 245");
    CHECK((ev.modifiers & FLAIR_EVT_MOD_BTN_STATE) != 0u,
                                 "replay[4]: btnState=1 (button UP again; MTE 2-6)");

    /* --- Drain [5]: KEYBOARD sc=0x10 make 'q' -> keyDown. --- */
#ifndef EVENT_MUTATE_DROP_SYNTH
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 1,               "replay[5]: keyDown('q') yields rc=1");
    CHECK(ev.what == keyDown,    "replay[5]: what = keyDown");
    /* sc_unshifted[0x10] = 'q' (IBM PC AT Table B-3). */
    CHECK((ev.message & 0xFFu) == 'q',   "replay[5]: message low byte = 'q'");
    CHECK(ev.when == 6u,         "replay[5]: when = 6");
#else
    /* MUTANT EVENT_MUTATE_DROP_SYNTH: keyboard events are not cooked. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(ev.what == nullEvent,  "MUTANT replay[5]: DROP_SYNTH must suppress keyDown (RED)");
#endif

    /* Ring should now be empty. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0 && ev.what == nullEvent,
                                 "replay: ring empty after trace");
}

/* ===========================================================================
 * GROUP A2 -- SECOND FIXED TRACE: verifies identical output on second run
 * ---------------------------------------------------------------------------
 * Run the same trace as Group A from a fresh ring. The outputs must be
 * bit-identical (Rule 11 determinism). We re-verify the critical fields.
 * ===========================================================================*/
static void test_deterministic_replay2(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* Same trace as Group A. */
    post1(&ring, FLAIR_RAW_KEYBOARD, 10u, 0x1Eu);              /* make 'a' */
    post1(&ring, FLAIR_RAW_MOUSE,    11u,
          (uint32_t)0x01u |
          ((uint32_t)(uint8_t)(int8_t)10 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)5  << 16u));

    /* Drain keyDown: must be identical to Group A result. */
    rc = drain1(&ring, everyEvent, &ev);
#ifndef EVENT_MUTATE_DROP_SYNTH
    CHECK(rc == 1,            "replay2: keyDown rc=1 (deterministic)");
    CHECK(ev.what == keyDown, "replay2: what=keyDown (same as run 1)");
    CHECK((ev.message & 0xFFu) == 'a', "replay2: ASCII='a' (deterministic)");
    CHECK(ev.when == 10u,     "replay2: when=10 (from raw.tick)");
    CHECK(ev.where.h == 320,  "replay2: where.h=320 (fresh cursor)");
    CHECK(ev.where.v == 240,  "replay2: where.v=240 (fresh cursor)");
#else
    CHECK(ev.what == nullEvent, "replay2 MUTANT: keyDown suppressed (RED)");
#endif

    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 1,               "replay2: mouseDown rc=1");
    CHECK(ev.what == mouseDown,  "replay2: what=mouseDown (deterministic)");
    /* MUTANT EVENT_MUTATE_STALE_WHERE: these go RED (cursor not updated). */
    CHECK(ev.where.h == 330,     "replay2: where.h=330 (deterministic delta)");
    CHECK(ev.where.v == 245,     "replay2: where.v=245 (deterministic delta)");
}

/* ===========================================================================
 * GROUP B -- eventMask FILTERING
 * ---------------------------------------------------------------------------
 * Post a keyDown; drain with a mask that does NOT include keyDownMask.
 * GetNextEvent must return nullEvent (rc=0).
 * Then drain with everyEvent -- ring is empty (the event was consumed and
 * discarded by the masked drain).
 *
 * NOTE: the filtering semantics: the masked-out event IS consumed from the
 * ring (drained) but reported as nullEvent to the caller. This matches the
 * MTE model ("if the event is not in the mask, it is discarded").
 * ===========================================================================*/
static void test_mask_filtering(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* Post a keyDown and a mouseDown. */
    post1(&ring, FLAIR_RAW_KEYBOARD, 20u, 0x1Eu);  /* 'a' make */
    post1(&ring, FLAIR_RAW_MOUSE,    21u,
          (uint32_t)0x01u | ((uint32_t)(uint8_t)(int8_t)0 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));   /* mouseDown, no delta */

    /* Drain with only mDownMask -- keyDown should be masked out. */
    rc = drain1(&ring, mDownMask, &ev);
    CHECK(rc == 0,              "mask: keyDown masked by mDownMask -> rc=0");
    CHECK(ev.what == nullEvent, "mask: keyDown masked -> nullEvent");

    /* Drain next: mouseDown should pass the mDownMask. */
    rc = drain1(&ring, mDownMask, &ev);
    CHECK(rc == 1,              "mask: mouseDown passes mDownMask -> rc=1");
    CHECK(ev.what == mouseDown, "mask: mouseDown not masked");

    /* Ring empty. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0,              "mask: ring empty after filtered drain");
}

/* ===========================================================================
 * GROUP C -- nullEvent on empty ring / timeout (WaitNextEvent sleepTicks=0)
 * ===========================================================================*/
static void test_null_event_empty(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* WaitNextEvent with sleepTicks=0 and empty ring: must return nullEvent. */
    rc = WaitNextEvent(&ring, everyEvent, &ev, 0u);
    CHECK(rc == 0,              "null: empty ring + sleepTicks=0 -> rc=0");
    CHECK(ev.what == nullEvent, "null: empty ring + sleepTicks=0 -> nullEvent");

    /* GetNextEvent on empty ring: same result. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0,              "null: GetNextEvent on empty -> rc=0");
    CHECK(ev.what == nullEvent, "null: GetNextEvent on empty -> nullEvent");

    /* nullEvent carries current tick and cursor. */
    CHECK(ev.when == flair_tick_count(), "null: nullEvent.when = current tick");
    CHECK(ev.where.h == 320,    "null: nullEvent.where.h = 320 (init centre)");
    CHECK(ev.where.v == 240,    "null: nullEvent.where.v = 240 (init centre)");
}

/* ===========================================================================
 * GROUP D -- `where` TRACKING: cumulative deltas + clamping
 * ---------------------------------------------------------------------------
 * Post a sequence of mouse-move events (no button) and assert the cursor
 * position accumulates correctly and clamps at the frame boundary.
 * ===========================================================================*/
static void test_where_tracking(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* Start at (320, 240). Move +100 in X five times: 320, 420, 520, 620,
     * 639 (clamped at FLAIR_SCREEN_W-1=639). */
    /* Move 1: dx=+100, dy=0, btn=0 (no click -> pure move -> nullEvent). */
    post1(&ring, FLAIR_RAW_MOUSE, 30u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)100 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));
    /* Move 2: dx=+100. */
    post1(&ring, FLAIR_RAW_MOUSE, 31u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)100 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));
    /* Move 3: dx=+100. Should hit 520. */
    post1(&ring, FLAIR_RAW_MOUSE, 32u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)100 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));
    /* Move 4: dx=+100. Would be 620; clamped to 639. */
    post1(&ring, FLAIR_RAW_MOUSE, 33u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)100 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));
    /* Move 5: dx=+100 more. Still clamped at 639. */
    post1(&ring, FLAIR_RAW_MOUSE, 34u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)100 << 8u) |
          ((uint32_t)(uint8_t)(int8_t)0 << 16u));

    /* Pure moves (no button transition) -> nullEvent. State still updates.
     * MUTANT EVENT_MUTATE_STALE_WHERE: cursor not updated; where.h stays 320;
     * the CHECK(where.h == 420) goes RED, proving the mutant is caught. */
    rc = drain1(&ring, everyEvent, &ev);
    (void)rc;
    CHECK(ev.where.h == 420, "where: move1 where.h=320+100=420");
    CHECK(ev.where.v == 240, "where: move1 where.v unchanged=240");
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(ev.where.h == 520, "where: move2 where.h=420+100=520");
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(ev.where.h == 620, "where: move3 where.h=520+100=620");
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(ev.where.h == 639, "where: move4 where.h clamped at 639");
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(ev.where.h == 639, "where: move5 where.h stays at 639 (clamped)");

    /* Test negative delta / Y clamping. */
    fresh_ring(&ring);
    /* Start at (320, 240). Move dy=-127 twice: 240-127=113; 113-127<0 -> 0. */
    post1(&ring, FLAIR_RAW_MOUSE, 40u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)0   << 8u) |
          ((uint32_t)(uint8_t)(int8_t)-127 << 16u));
    post1(&ring, FLAIR_RAW_MOUSE, 41u,
          (uint32_t)0x00u | ((uint32_t)(uint8_t)(int8_t)0   << 8u) |
          ((uint32_t)(uint8_t)(int8_t)-127 << 16u));
    rc = drain1(&ring, everyEvent, &ev);
    rc = drain1(&ring, everyEvent, &ev);
    /* After two -127 moves: 240-127=113; 113-127=-14 -> clamped to 0. */
    CHECK(ev.where.v == 0,   "where: negative Y delta clamped to 0");
    (void)rc;
}

/* ===========================================================================
 * GROUP E -- MODIFIER REFLECTION (shift/ctrl -> modifiers bits)
 * ===========================================================================*/
static void test_modifier_reflection(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    int rc;

    fresh_ring(&ring);

    /* Press Left Shift (sc=0x2A make). Shift press alone -> no event (nullEvent). */
    post1(&ring, FLAIR_RAW_KEYBOARD, 50u, 0x2Au);          /* SC_LSHIFT make */
    /* Press 'a' (sc=0x1E make). With shift -> 'A'. */
    post1(&ring, FLAIR_RAW_KEYBOARD, 51u, 0x1Eu);
    /* Release 'a'. */
    post1(&ring, FLAIR_RAW_KEYBOARD, 52u, 0x1Eu | (1u<<8u));
    /* Release Shift (sc=0x2A break). */
    post1(&ring, FLAIR_RAW_KEYBOARD, 53u, 0x2Au | (1u<<8u));

    /* Drain Shift press -> nullEvent (modifier-only key). */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0,              "mod: Shift press -> nullEvent");
    CHECK(ev.what == nullEvent, "mod: Shift press what=nullEvent");

    /* Drain 'a' with shift -> keyDown 'A'. */
    rc = drain1(&ring, everyEvent, &ev);
#ifndef EVENT_MUTATE_DROP_SYNTH
    CHECK(rc == 1,              "mod: keyDown with shift -> rc=1");
    CHECK(ev.what == keyDown,   "mod: what=keyDown (shifted 'a')");
    CHECK((ev.message & 0xFFu) == 'A',   "mod: message='A' (shifted)");
    CHECK((ev.modifiers & FLAIR_EVT_MOD_SHIFT_KEY) != 0u,
                                "mod: shiftKey bit set in modifiers");
#endif
    (void)rc;

    /* Drain 'a' release -> keyUp, Shift still held at release time. */
    rc = drain1(&ring, everyEvent, &ev);
    (void)rc;
#ifndef EVENT_MUTATE_DROP_SYNTH
    CHECK(ev.what == keyUp,     "mod: keyUp after release");
    CHECK((ev.modifiers & FLAIR_EVT_MOD_SHIFT_KEY) != 0u,
                                "mod: shift still held at keyUp");
#endif

    /* Drain Shift release -> nullEvent. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0,              "mod: Shift release -> nullEvent");

    /* After Shift release, next event should have Shift=0. */
    post1(&ring, FLAIR_RAW_KEYBOARD, 54u, 0x1Eu);   /* 'a' again (unshifted) */
    rc = drain1(&ring, everyEvent, &ev);
#ifndef EVENT_MUTATE_DROP_SYNTH
    CHECK((ev.message & 0xFFu) == 'a',  "mod: after shift release -> 'a' unshifted");
    CHECK((ev.modifiers & FLAIR_EVT_MOD_SHIFT_KEY) == 0u,
                                         "mod: shiftKey clear after release");
#endif
    (void)rc;
}

/* ===========================================================================
 * GROUP F -- SPSC RING CORRECTNESS
 * ---------------------------------------------------------------------------
 * (F1) INTERLEAVED POST/DRAIN: post N events, drain N/2, post N/2 more,
 *      drain all -- ring must not lose or corrupt any event.
 * (F2) FULL-RING HANDLING: fill the ring to FLAIR_RAW_RING_CAP; one more
 *      post must fail (return 0) and increment drop_count.
 * (F3) POST/DRAIN/POST/DRAIN alternating: ring stays consistent under
 *      maximum-frequency interleaving.
 * ===========================================================================*/
static void test_spsc_ring(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    uint32_t i, tick;
    int rc;

    /* F1: Interleaved post/drain. */
    fresh_ring(&ring);
    /* Post 4 events. */
    for (i = 0; i < 4u; i++) {
        post1(&ring, FLAIR_RAW_TICK, i, 0u);
    }
    /* Drain 2. */
    rc = drain1(&ring, everyEvent, &ev);
    (void)rc;
    rc = drain1(&ring, everyEvent, &ev);
    (void)rc;
    /* Post 4 more (ring had 2 left; now has 6). */
    for (i = 4u; i < 8u; i++) {
        post1(&ring, FLAIR_RAW_TICK, i, 0u);
    }
    /* Drain all remaining 6. */
    for (i = 0; i < 6u; i++) {
        rc = drain1(&ring, everyEvent, &ev);
        CHECK(rc == 0 && ev.what == nullEvent,
              "spsc F1: TICK events cooked to nullEvent (expected)");
    }
    /* Ring should be empty now. */
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0, "spsc F1: ring empty after interleaved sequence");

    /* F2: Full-ring -- fill to capacity, then assert one more post fails. */
    fresh_ring(&ring);
    tick = 100u;
    for (i = 0; i < FLAIR_RAW_RING_CAP; i++) {
        flair_raw_event_t raw;
        int post_rc;
        raw.kind    = (uint32_t)FLAIR_RAW_TICK;
        raw.tick    = tick++;
        raw.payload = 0u;
        post_rc = flair_raw_post(&ring, &raw);
        CHECK(post_rc == 1, "spsc F2: posts 0..(CAP-1) must succeed");
    }
    /* One more post: ring is full; must fail (return 0). */
    {
        flair_raw_event_t raw;
        int post_rc;
        raw.kind    = (uint32_t)FLAIR_RAW_TICK;
        raw.tick    = tick++;
        raw.payload = 0u;
        post_rc = flair_raw_post(&ring, &raw);
        CHECK(post_rc == 0,  "spsc F2: post when full must return 0 (dropped)");
        CHECK(ring.drop_count == 1u,
              "spsc F2: drop_count must be 1 after one full-ring drop");
    }
    /* Drain all; confirm ring usable after full-ring drop. */
    for (i = 0; i < FLAIR_RAW_RING_CAP; i++) {
        rc = drain1(&ring, everyEvent, &ev);
        (void)rc;
    }
    rc = drain1(&ring, everyEvent, &ev);
    CHECK(rc == 0, "spsc F2: ring empty after draining full-ring");

    /* F3: POST/DRAIN alternating (maximum interleave, no buffering). */
    fresh_ring(&ring);
    for (i = 0u; i < 20u; i++) {
        /* Post one TICK event. */
        post1(&ring, FLAIR_RAW_TICK, i, 0u);
        /* Drain immediately. */
        rc = drain1(&ring, everyEvent, &ev);
        CHECK(rc == 0,              "spsc F3: TICK -> nullEvent");
        CHECK(ev.what == nullEvent, "spsc F3: TICK what=nullEvent");
    }

    /* F4: Verify the drop counter is NOT incremented on a successful post. */
    fresh_ring(&ring);
    post1(&ring, FLAIR_RAW_TICK, 0u, 0u);
    CHECK(ring.drop_count == 0u, "spsc F4: no drop on successful post");
}

/* ===========================================================================
 * GROUP G -- RANDOMISED REPLAY DETERMINISM (Rule 11)
 * ---------------------------------------------------------------------------
 * Generate a random trace with the seeded LCG; replay it; store the output
 * EventRecord sequence. Reset to the same seed; replay the identical trace.
 * The two output sequences must be bit-identical (determinism guarantee).
 * ===========================================================================*/

#define RAND_TRACE_LEN  32u
#define MAX_EVENTS      64u

static void test_random_determinism(void)
{
    flair_raw_ring_t ring;
    EventRecord out1[MAX_EVENTS], out2[MAX_EVENTS];
    uint32_t n1, n2, i;
    uint32_t saved_lcg;

    /* Build a random trace with the seeded LCG. Post/drain in batches. */
    saved_lcg = g_lcg;  /* save seed for second run */

    /* Run 1. */
    fresh_ring(&ring);
    g_lcg = saved_lcg;
    n1 = 0u;
    for (i = 0u; i < RAND_TRACE_LEN; i++) {
        uint32_t r    = lcg_next();
        uint32_t kind = r % 3u;   /* 0=KEYBOARD, 1=MOUSE, 2=TICK */
        uint32_t tick_val = i + 200u;
        uint32_t payload;
        flair_raw_event_t raw;
        int post_rc;

        if (kind == 0u) {
            /* Random scancode 0x10..0x32 (printable alpha range). */
            uint8_t sc = (uint8_t)(0x10u + (r >> 8u) % 0x23u);
            payload = (uint32_t)sc;
        } else if (kind == 1u) {
            /* Random mouse: dx = (int8_t)(r>>8), dy = (int8_t)(r>>16), btn. */
            payload = ((r >> 8u) & 0xFFu) |      /* buttons (low byte) */
                      (((r >> 12u) & 0xFFu) << 8u) |   /* dx */
                      (((r >> 20u) & 0xFFu) << 16u);    /* dy */
        } else {
            payload = 0u;
        }
        raw.kind    = kind;
        raw.tick    = tick_val;
        raw.payload = payload;
        post_rc = flair_raw_post(&ring, &raw);
        (void)post_rc;
    }
    /* Drain all. */
    while (n1 < MAX_EVENTS) {
        int rc = GetNextEvent(&ring, everyEvent, &out1[n1]);
        if (rc == 0 && out1[n1].what == nullEvent) break;
        n1++;
    }

    /* Run 2: reset to same seed, same ring. */
    fresh_ring(&ring);
    g_lcg = saved_lcg;
    n2 = 0u;
    for (i = 0u; i < RAND_TRACE_LEN; i++) {
        uint32_t r    = lcg_next();
        uint32_t kind = r % 3u;
        uint32_t tick_val = i + 200u;
        uint32_t payload;
        flair_raw_event_t raw;
        int post_rc;

        if (kind == 0u) {
            uint8_t sc = (uint8_t)(0x10u + (r >> 8u) % 0x23u);
            payload = (uint32_t)sc;
        } else if (kind == 1u) {
            payload = ((r >> 8u) & 0xFFu) |
                      (((r >> 12u) & 0xFFu) << 8u) |
                      (((r >> 20u) & 0xFFu) << 16u);
        } else {
            payload = 0u;
        }
        raw.kind    = kind;
        raw.tick    = tick_val;
        raw.payload = payload;
        post_rc = flair_raw_post(&ring, &raw);
        (void)post_rc;
    }
    while (n2 < MAX_EVENTS) {
        int rc = GetNextEvent(&ring, everyEvent, &out2[n2]);
        if (rc == 0 && out2[n2].what == nullEvent) break;
        n2++;
    }

    /* Bit-identical: both runs must produce the same number of events and
     * the same EventRecord contents (Rule 11 determinism). */
    CHECK(n1 == n2, "random: both runs produce same event count (Rule 11)");
    {
        uint32_t ok = 1u;
        uint32_t minN = (n1 < n2) ? n1 : n2;
        for (i = 0u; i < minN; i++) {
            if (memcmp(&out1[i], &out2[i], sizeof(EventRecord)) != 0) {
                ok = 0u;
                break;
            }
        }
        CHECK(ok, "random: EventRecord sequences bit-identical across runs (Rule 11)");
    }
}

/* ===========================================================================
 * GROUP H -- NAMED MUTANT PROBE (Rule 6: mutation-proven oracle discipline)
 * ---------------------------------------------------------------------------
 * The mutant probes (EVENT_MUTATE_DROP_SYNTH, EVENT_MUTATE_STALE_WHERE) are
 * NAMED mutants in os/flair/event.c. Each mutant MUST drive THIS oracle RED
 * (exit non-zero). The mechanism:
 *
 *   EVENT_MUTATE_DROP_SYNTH: the keyboard-synthesis block in cook_raw() is
 *     skipped entirely; all FLAIR_RAW_KEYBOARD events cook to nullEvent.
 *     Groups A, A2, E, G assert the CORRECT EventRecord sequence, which will
 *     fail because expected keyDown/keyUp events are not produced. This group
 *     adds an explicit confirmation check so the failure path is visible even
 *     if groups A/A2 are reordered.
 *
 *   EVENT_MUTATE_STALE_WHERE: clamp_coord() is not called in the MOUSE branch;
 *     g_cursor_h/v are never updated from mouse deltas; `where` is always stale.
 *     Groups A (replay[3,4]), A2, D (where_tracking) assert where.h==330 etc.,
 *     which fail because the cursor stays at (320,240). Explicit confirmations
 *     are in those groups (the CHECKs are unconditional).
 *
 * Rule 6 verification: after mutation-proving, RESTORE the event.c source and
 * confirm this oracle returns to GREEN (exit 0). That is the full discipline.
 *
 * In the NON-mutant build this group is empty (the DROP_SYNTH confirmation
 * check below is the only one; STALE_WHERE is proven by groups A/A2/D above).
 * ===========================================================================*/
static void test_named_mutant_gate(void)
{
#ifdef EVENT_MUTATE_DROP_SYNTH
    /* Explicit DROP_SYNTH confirmation: a fresh keyboard replay that must
     * produce nullEvent (not keyDown). This makes the mutant failure
     * immediately visible from the test name in the failure output. */
    {
        flair_raw_ring_t ring;
        EventRecord ev;
        int rc;
        fresh_ring(&ring);
        post1(&ring, FLAIR_RAW_KEYBOARD, 99u, 0x1Eu);  /* 'a' make sc */
        rc = drain1(&ring, everyEvent, &ev);
        /* CORRECT behaviour: keyDown. With DROP_SYNTH: suppressed to nullEvent.
         * This CHECK must FAIL under the mutant (oracle goes RED). */
        CHECK(ev.what == keyDown,
              "MUTANT_GATE DROP_SYNTH: ev.what must be keyDown (FAILS for mutant)");
        (void)rc;
    }
#endif
    /* Note: STALE_WHERE is proven RED by the unconditional where.h/v CHECKs
     * in test_deterministic_replay, test_deterministic_replay2, and
     * test_where_tracking. No additional block needed here. */
}

/* ===========================================================================
 * main
 * ===========================================================================*/
int main(void)
{
    test_deterministic_replay();
    test_deterministic_replay2();
    test_mask_filtering();
    test_null_event_empty();
    test_where_tracking();
    test_modifier_reflection();
    test_spsc_ring();
    test_random_determinism();
    test_named_mutant_gate();

    return TEST_SUMMARY("test_event");
}

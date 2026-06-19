/*
 * os/flair/event.c -- FLAIR Event Manager: Layer 4 cooperative event core.
 *
 * beads: initech-8b7 (FLAIR Event Manager: WaitNextEvent + SPSC ring).
 * Ref:   ADR-0004 D-4 (ISR enqueue-only; EventRecord synthesis in task context).
 * Ref:   ADR-0004 D-6 (cooperative, non-preemptive WaitNextEvent on PIT tick).
 * Ref:   ADR-0004 D-8 (test-event oracle: recorded raw trace -> deterministic
 *          EventRecord sequence).
 * Ref:   spec/event_model.h (LOCKED: EventRecord, flair_raw_event_t,
 *          flair_event_what_t, modifier bits, mask bits, FLAIR_RAW_RING_CAP).
 * Ref:   spec/grafport.h (flair_point_t -- QuickDraw Point; the `where` type).
 * Ref:   Inside Macintosh: Macintosh Toolbox Essentials (1992) [MTE] Ch 2
 *          "Event Manager" -- WaitNextEvent/GetNextEvent semantics.
 * Ref:   Intel 8259A PRM / IBM PC AT Technical Reference (IRQ0 PIT tick;
 *          IRQ1 PS/2 keyboard; IRQ12 PS/2 mouse via slave PIC cascade --
 *          dual-PIC EOI: slave-EOI then master-EOI; ADR-0004 D-7 / AM-7).
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc, no malloc.
 * Compiles BOTH under kernel flags:
 *   gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall -Wextra -Werror
 *       -Ios/flair -Ispec
 * AND hosted for the test oracle (harness/proptest/test_event.c).
 * Rule 11 (reproducible / deterministic). Rule 12 (ASCII-clean). Law 3.
 *
 * ISR / TASK-CONTEXT SPLIT (ADR-0004 D-4 / C-3)
 * -----------------------------------------------
 * ISR context  -> flair_raw_post(ring, raw)  -- enqueue ONLY; never Toolbox.
 *             + flair_tick_advance()          -- increment tick counter ONLY.
 * Task context -> WaitNextEvent / GetNextEvent -- drain ring, synthesize,
 *                 filter, deliver. The two contexts are mutually exclusive
 *                 by cooperative scheduling (ADR-0004 D-6 / C-5).
 *
 * SCANCODE -> ASCII (minimal table)
 * ----------------------------------
 * The PS/2 SET-1 make-code to ASCII mapping used here covers the printable
 * ASCII set (0x20..0x7E) needed for the cooperative desktop (InitechCalc,
 * the FILE COPY dialog, SAMIR, Turbo Initech IDE). The table is clean-room,
 * derived from the IBM PC AT Technical Reference (keyboard scan codes,
 * Table B-3). Non-printable / unmapped scancodes yield ASCII 0.
 * Ref: IBM PC AT Technical Reference (1984) keyboard appendix, Table B-3.
 *
 * MOUSE DELTA -> CURSOR POSITION (ADR-0004 D-7 / OD-3)
 * ------------------------------------------------------
 * Mouse deltas are signed bytes packed in the FLAIR_RAW_MOUSE payload
 * (spec/event_model.h Sec 5, bits 8..15 = delta X, bits 16..23 = delta Y,
 * already sign-extended and Y-inverted from PS/2 convention to screen-down-
 * positive). The cursor position is clamped to [0, FLAIR_SCREEN_W-1] x
 * [0, FLAIR_SCREEN_H-1] (640x480, ADR-0004 OD-3).
 */
#include "event.h"        /* os/flair/event.h (-Ios/flair)                  */

/* ===========================================================================
 * MINIMAL PANIC STUB (freestanding; host builds shadow with assert/fprintf)
 * ---------------------------------------------------------------------------
 * On bare metal a real panic renders "PC LOAD LETTER" and halts (Rule 2;
 * PRD Appendix B). In this translation unit the macro is used only for
 * invariant violations that should never fire in a correct build. The
 * definition below is a bare-metal stub (infinite loop); the test harness
 * overrides it via the EVENT_TEST_PANIC macro before including this unit.
 * ---------------------------------------------------------------------------*/
#ifndef FLAIR_PANIC
#  define FLAIR_PANIC(msg) do { (void)(msg); for(;;){} } while(0)
#endif

/* ===========================================================================
 * GLOBAL STATE
 * ---------------------------------------------------------------------------
 * All mutable state is in this translation unit. The test harness accesses
 * it only through the public API (no extern hacks).
 * ===========================================================================*/

/* Global PIT tick counter (Rule 11: monotonic, starts at 0, advanced ONLY
 * by flair_tick_advance() which is called from the PIT ISR). */
static volatile uint32_t g_tick;

/* Cursor position in screen (global) coordinates. Updated by the pump from
 * FLAIR_RAW_MOUSE deltas; clamped to [0, FLAIR_SCREEN_W-1] x
 * [0, FLAIR_SCREEN_H-1]. Starts at the screen centre (period-authentic
 * Mac/System-7 convention -- cursor starts somewhere in the middle). */
static int16_t g_cursor_h;   /* horizontal (x) */
static int16_t g_cursor_v;   /* vertical   (y) */

/* Current modifier-key state maintained across raw keyboard events. */
static uint16_t g_modifiers;

/* The cooperative yield hook (see event.h Sec 3). */
static flair_event_yield_fn g_yield_fn;

/* PS/2 SET-1 break (key-release) prefix accumulator. The PS/2 stream sends
 * 0xF0 followed by the make code. The ISR sets bit 8 of payload when it
 * consumes the 0xF0 prefix (see spec/event_model.h Sec 5, KEYBOARD payload
 * layout: bit 8 = key-break flag). We re-check it here for synthesis. */

/* ===========================================================================
 * 1. TICK COUNTER
 * ===========================================================================*/

void flair_tick_advance(void)
{
    /* ISR-safe: single 32-bit write, no lock needed (cooperative single-CPU,
     * ADR-0004 D-6). Overflow wraps; after ~828 days at 60 Hz, acceptable
     * for a cooperative desktop session (PRD era authenticity). */
    g_tick++;
}

uint32_t flair_tick_count(void)
{
    return g_tick;
}

/* ===========================================================================
 * 2. YIELD HOOK
 * ===========================================================================*/

void flair_event_set_yield(flair_event_yield_fn fn)
{
    g_yield_fn = fn;
}

/* ===========================================================================
 * 3. INITIALISATION
 * ===========================================================================*/

void flair_event_init(flair_raw_ring_t *ring)
{
    uint32_t i;
    if (!ring) { FLAIR_PANIC("flair_event_init: NULL ring"); }
    ring->head       = 0u;
    ring->tail       = 0u;
    ring->drop_count = 0u;
    for (i = 0; i < FLAIR_RAW_RING_CAP; i++) {
        ring->slots[i].kind    = 0u;
        ring->slots[i].tick    = 0u;
        ring->slots[i].payload = 0u;
    }
    g_tick       = 0u;
    /* Start cursor in the approximate centre of the 640x480 screen. */
    g_cursor_h   = (int16_t)(FLAIR_SCREEN_W / 2);
    g_cursor_v   = (int16_t)(FLAIR_SCREEN_H / 2);
    g_modifiers  = FLAIR_EVT_MOD_BTN_STATE; /* button UP at init (MTE 2-6: 1=UP) */
    g_yield_fn   = (flair_event_yield_fn)0;
}

/* ===========================================================================
 * 4. SPSC RAW RING: PRODUCER SIDE
 * ---------------------------------------------------------------------------
 * flair_raw_post -- ISR producer: push one raw event into the ring.
 *
 * LOCK-FREE SPSC PROTOCOL (spec/event_model.h Sec 5):
 *   Producer (ISR) owns head; consumer (pump) owns tail.
 *   "Full" = (head - tail) >= FLAIR_RAW_RING_CAP  (unsigned modular arithmetic).
 *   "Empty" = (head == tail).
 *
 * X86 SINGLE-CORE ATOMICITY:
 *   On the 386+ cooperative desktop (ADR-0004 D-6 / C-5), a single-core ISR
 *   and the pump are mutually exclusive by construction (ISR suspends the pump;
 *   the pump cannot be interrupted mid-operation by the same ISR on one core).
 *   The volatile qualifiers on head/tail ensure the compiler emits actual
 *   memory reads rather than caching the value in a register across the
 *   ISR/pump boundary -- which is the only hazard on a single cooperative CPU.
 *   (A multi-core port would need explicit memory barriers; that is out of
 *   scope -- ADR-0004 D-6 cooperative, non-SMP target.)
 *
 * FULL-RING: drop + increment drop_count (Rule 2 -- defined, non-silent
 * failure; see event.h Sec 1 rationale). Returns 0 (dropped) or 1 (queued).
 * ===========================================================================*/

int flair_raw_post(flair_raw_ring_t *ring, const flair_raw_event_t *raw)
{
    uint32_t h, t;

    if (!ring || !raw) { FLAIR_PANIC("flair_raw_post: NULL arg"); }

    h = ring->head;    /* producer's unmasked write index (producer owns this) */
    t = ring->tail;    /* volatile read of consumer's unmasked tail            */

    /* Full: occupancy = (h - t) >= FLAIR_RAW_RING_CAP (unsigned arithmetic).
     * Both h and t are unmasked (see SPSC INDEX DESIGN in event.h); the
     * unsigned subtract gives occupancy correctly across uint32_t wrap as long
     * as the ring is never held for 2^32 events simultaneously (impossible here).
     * This is the standard LMAX Disruptor-style single-CPU SPSC full check. */
    if ((h - t) >= FLAIR_RAW_RING_CAP) {
        ring->drop_count++;
        return 0;   /* ring full: drop + telemetry (Rule 2, defined non-silent) */
    }

    /* Write the slot at the masked head index, then advance head (unmasked). */
    ring->slots[h & FLAIR_RAW_RING_MASK] = *raw;

    /* Volatile store: compiler must not hoist head update above slot write
     * (the sole ordering hazard on single-core cooperative x86; see header). */
    ring->head = h + 1u;

    return 1;   /* successfully enqueued */
}

/* ===========================================================================
 * 5. SCANCODE TABLE  (PS/2 SET-1, IBM PC AT Technical Reference Table B-3)
 * ---------------------------------------------------------------------------
 * 128-entry table (make codes 0x00..0x7F). Index = make code; value = ASCII.
 * 0 = unmapped / non-printable. Shift variants follow in the parallel table.
 * Extended codes (0xE0 prefix) are not stored here; the ISR handles the
 * prefix flag via a separate bit in the KEYBOARD payload (spec/event_model.h).
 * Source: IBM PC AT Technical Reference (1984) keyboard appendix, Table B-3.
 * ===========================================================================*/

/* Unshifted ASCII for PS/2 SET-1 make codes 0x00..0x7F.
 * Entries are set to the printable character or 0 for non-printable.
 * Suppress unused-variable warnings when EVENT_MUTATE_DROP_SYNTH skips
 * the synthesis block (the tables are logically part of the translation
 * unit; the mutant suppresses their use at compile time). */
#ifdef __GNUC__
__attribute__((unused))
#endif
static const uint8_t sc_unshifted[128] = {
/*00*/ 0,    0,    '1',  '2',  '3',  '4',  '5',  '6',
/*08*/ '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
/*10*/ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
/*18*/ 'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
/*20*/ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
/*28*/ '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
/*30*/ 'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
/*38*/ 0,    ' ',  0,    0,    0,    0,    0,    0,
/*40*/ 0,    0,    0,    0,    0,    0,    0,    '7',
/*48*/ '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
/*50*/ '2',  '3',  '0',  '.',  0,    0,    0,    0,
/*58*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*60*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*68*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*70*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*78*/ 0,    0,    0,    0,    0,    0,    0,    0
};

/* Shifted ASCII for the same make codes (Shift held, no Caps Lock). */
#ifdef __GNUC__
__attribute__((unused))
#endif
static const uint8_t sc_shifted[128] = {
/*00*/ 0,    0,    '!',  '@',  '#',  '$',  '%',  '^',
/*08*/ '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
/*10*/ 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
/*18*/ 'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
/*20*/ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
/*28*/ '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
/*30*/ 'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
/*38*/ 0,    ' ',  0,    0,    0,    0,    0,    0,
/*40*/ 0,    0,    0,    0,    0,    0,    0,    '7',
/*48*/ '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
/*50*/ '2',  '3',  '0',  '.',  0,    0,    0,    0,
/*58*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*60*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*68*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*70*/ 0,    0,    0,    0,    0,    0,    0,    0,
/*78*/ 0,    0,    0,    0,    0,    0,    0,    0
};

/* PS/2 SET-1 make codes for the modifier keys (IBM PC AT Table B-3). */
#define SC_LSHIFT  0x2Au
#define SC_RSHIFT  0x36u
#define SC_CTRL    0x1Du   /* left Ctrl */
#define SC_ALT     0x38u   /* left Alt (Option) */
#define SC_CAPS    0x3Au

/* ===========================================================================
 * 6. RING CONSUMER: DRAIN ONE RAW EVENT + COOK INTO EventRecord
 * ---------------------------------------------------------------------------
 * cook_raw -- drain one raw record from the ring and cook it into an
 * EventRecord. Updates g_cursor_h/v (mouse deltas) and g_modifiers
 * (keyboard modifier state). Returns the event `what` code. Returns
 * (uint16_t)nullEvent if the ring is empty.
 *
 * This function is the SYNTHESIS step of ADR-0004 D-4: it runs in task
 * context (inside WaitNextEvent / GetNextEvent). ISRs NEVER call this.
 *
 * NOTE on mutation probe injection (EVENT_MUTATE_DROP_SYNTH / STALE_WHERE):
 *   These preprocessor guards let test_event.c compile in a mutant build
 *   by defining the relevant macro. See harness/proptest/test_event.c.
 * ===========================================================================*/

/* Apply a signed 8-bit delta (packed as int8_t in a uint8_t slot) to a
 * coordinate and clamp to [0, max-1]. Returns the new clamped position. */
#ifdef __GNUC__
__attribute__((unused))
#endif
static int16_t clamp_coord(int16_t cur, int8_t delta, int16_t max)
{
    int32_t next = (int32_t)cur + (int32_t)delta;
    if (next < 0)           next = 0;
    if (next >= (int32_t)max) next = (int32_t)max - 1;
    return (int16_t)next;
}

/*
 * drain_one -- pop one raw record from the ring. Returns 1 if a record was
 * popped (written to *out), 0 if the ring was empty.
 */
static int drain_one(flair_raw_ring_t *ring, flair_raw_event_t *out)
{
    uint32_t h, t;
    h = ring->head;   /* volatile read -- see consumer note above */
    t = ring->tail;
    if (h == t) return 0;   /* empty */
    *out = ring->slots[t & FLAIR_RAW_RING_MASK];
    ring->tail = t + 1u;   /* unmasked increment; mask applied only on slot access */
    return 1;
}

/*
 * cook_raw -- synthesize an EventRecord from one raw ring record.
 *
 * On entry: raw is a raw event record freshly drained from the ring.
 * On exit:  *ev is filled with the synthesized EventRecord (what, message,
 *           when, where, modifiers). Returns the `what` code.
 *
 * MUTATION PROBES (compile-time, for test_event.c):
 *   EVENT_MUTATE_DROP_SYNTH  -- for FLAIR_RAW_KEYBOARD events, skip
 *     writing the EventRecord fields (returns nullEvent instead of keyDown/
 *     keyUp). This makes the replay oracle go RED because expected keyDown
 *     events are not produced.
 *   EVENT_MUTATE_STALE_WHERE -- do NOT update g_cursor_h/v from mouse
 *     deltas. The `where` field in mouse events remains stale. Makes the
 *     replay oracle go RED because expected cursor positions are wrong.
 */
static uint16_t cook_raw(const flair_raw_event_t *raw, EventRecord *ev)
{
    uint16_t what = (uint16_t)nullEvent;

    /* Stamp `when` from the raw record's tick (the tick at ISR fire time,
     * not the pump time -- more accurate; spec/event_model.h Sec 5). */
    ev->when      = raw->tick;
    ev->modifiers = g_modifiers;
    ev->message   = 0u;

    switch ((flair_raw_kind_t)raw->kind) {

    /* ------------------------------------------------------------------
     * KEYBOARD raw event
     * ------------------------------------------------------------------
     * Payload layout (spec/event_model.h Sec 5 KEYBOARD):
     *   bits 0..7  = raw PS/2 SET-1 make/break scancode byte.
     *   bit  8     = key-break flag (1 = key released; set by ISR after
     *                consuming the 0xF0 break prefix from the PS/2 stream).
     *   bits 9..31 = reserved/zero.
     * ------------------------------------------------------------------ */
    case FLAIR_RAW_KEYBOARD: {
        uint8_t  sc        = (uint8_t)(raw->payload & 0xFFu);
        int      is_break  = (int)((raw->payload >> 8u) & 1u);
        uint8_t  ascii     = 0u;
        uint16_t vkey      = (uint16_t)sc;

        /* Modifier-key tracking: update g_modifiers on press/release. */
        if (sc == SC_LSHIFT || sc == SC_RSHIFT) {
            if (is_break) g_modifiers &= (uint16_t)~FLAIR_EVT_MOD_SHIFT_KEY;
            else          g_modifiers |= FLAIR_EVT_MOD_SHIFT_KEY;
            /* Shift press/release itself is NOT a separate keyDown/keyUp
             * in Inside Macintosh -- it updates the modifier state that is
             * stamped onto the NEXT event. */
            ev->modifiers = g_modifiers;
            /* Return nullEvent for standalone modifier. */
            break;
        }
        if (sc == SC_CTRL) {
            if (is_break) g_modifiers &= (uint16_t)~FLAIR_EVT_MOD_CONTROL_KEY;
            else          g_modifiers |= FLAIR_EVT_MOD_CONTROL_KEY;
            ev->modifiers = g_modifiers;
            break;
        }
        if (sc == SC_ALT) {
            if (is_break) g_modifiers &= (uint16_t)~FLAIR_EVT_MOD_OPTION_KEY;
            else          g_modifiers |= FLAIR_EVT_MOD_OPTION_KEY;
            ev->modifiers = g_modifiers;
            break;
        }
        if (sc == SC_CAPS) {
            if (!is_break) {
                /* Toggle Caps Lock on key-press (period-authentic). */
                g_modifiers ^= FLAIR_EVT_MOD_ALPHA_LOCK;
            }
            ev->modifiers = g_modifiers;
            break;
        }

#ifdef EVENT_MUTATE_DROP_SYNTH
        /* MUTANT: skip cooking keyboard events -> nullEvent.
         * This MUST drive the replay oracle RED (Rule 6). */
        (void)ascii; (void)vkey; (void)is_break;
        break;
#else
        /* Cook ASCII from the scancode table. */
        if (sc < 128u) {
            int shifted = (g_modifiers & FLAIR_EVT_MOD_SHIFT_KEY) ? 1 : 0;
            /* Caps Lock: for alpha keys, invert the shifted state. */
            if ((g_modifiers & FLAIR_EVT_MOD_ALPHA_LOCK) &&
                ((sc >= 0x10u && sc <= 0x19u) ||   /* Q..P row */
                 (sc >= 0x1Eu && sc <= 0x26u) ||   /* A..L row */
                 (sc >= 0x2Cu && sc <= 0x32u))) {  /* Z..M row */
                shifted = !shifted;
            }
            ascii = shifted ? sc_shifted[sc] : sc_unshifted[sc];
        }

        /* MTE p. 2-7 message field for key events:
         *   low byte  (bits 0..7)  = ASCII char code.
         *   byte 1    (bits 8..15) = virtual key code (= PS/2 make code here).
         *   bits 16..31 = 0 (FLAIR simplification). */
        ev->message   = ((uint32_t)vkey << 8u) | (uint32_t)ascii;
        ev->modifiers = g_modifiers;

        if (is_break) {
            what = (uint16_t)keyUp;
        } else {
            what = (uint16_t)keyDown;
        }
#endif /* EVENT_MUTATE_DROP_SYNTH */
        break;
    }

    /* ------------------------------------------------------------------
     * MOUSE raw event
     * ------------------------------------------------------------------
     * Payload layout (spec/event_model.h Sec 5 MOUSE):
     *   bits  0..7  = raw button byte (bit 0 = left; bit 1 = right; ...).
     *   bits  8..15 = signed delta X as int8_t.
     *   bits 16..23 = signed delta Y as int8_t (already Y-inverted).
     *   bits 24..31 = reserved/zero.
     * ------------------------------------------------------------------ */
    case FLAIR_RAW_MOUSE: {
        uint8_t  buttons = (uint8_t)(raw->payload & 0xFFu);
        int8_t   dx      = (int8_t)((raw->payload >>  8u) & 0xFFu);
        int8_t   dy      = (int8_t)((raw->payload >> 16u) & 0xFFu);
        int      was_down = !(g_modifiers & FLAIR_EVT_MOD_BTN_STATE); /* 0=down */
        int      is_down  = (int)(buttons & 0x01u);                   /* left btn */

#ifdef EVENT_MUTATE_STALE_WHERE
        /* MUTANT: do NOT update cursor position from mouse deltas.
         * The `where` field will be stale. This MUST drive the replay RED. */
        (void)dx; (void)dy;
#else
        /* Update tracked cursor position from the delta (clamp to frame). */
        g_cursor_h = clamp_coord(g_cursor_h, dx, (int16_t)FLAIR_SCREEN_W);
        g_cursor_v = clamp_coord(g_cursor_v, dy, (int16_t)FLAIR_SCREEN_H);
#endif

        /* Update btnState modifier bit (MTE 2-6: 1 = button UP, inverted). */
        if (is_down) g_modifiers &= (uint16_t)~FLAIR_EVT_MOD_BTN_STATE;
        else         g_modifiers |= FLAIR_EVT_MOD_BTN_STATE;
        ev->modifiers = g_modifiers;

        /* Synthesize mouseDown, mouseUp, or nullEvent (no click). */
        if (!was_down && is_down) {
            /* Transition UP -> DOWN: mouseDown. */
            what = (uint16_t)mouseDown;
        } else if (was_down && !is_down) {
            /* Transition DOWN -> UP: mouseUp. */
            what = (uint16_t)mouseUp;
        } else {
            /* Pure move (no state change): no event in MTE model (mouse
             * moves do not generate events; only clicks do). */
            what = (uint16_t)nullEvent;
        }

        ev->message = 0u;   /* undefined for mouse events (MTE p. 2-6) */
        break;
    }

    /* ------------------------------------------------------------------
     * TICK raw event
     * ------------------------------------------------------------------
     * Payload = 0 (spec/event_model.h Sec 5 TICK). The pump updates the
     * local tick accumulator view but does NOT generate an EventRecord
     * for a TICK alone (it advances internal state only). nullEvent is
     * returned when the caller finds no other event pending.
     * ------------------------------------------------------------------ */
    case FLAIR_RAW_TICK:
        /* g_tick is already advanced by flair_tick_advance() in the ISR.
         * Nothing additional to do in synthesis; return nullEvent. */
        what = (uint16_t)nullEvent;
        break;

    default:
        /* Unknown raw kind -- fail loud (Rule 2). */
        FLAIR_PANIC("cook_raw: unknown flair_raw_kind_t");
        break;
    }

    /* Stamp `where` from the current tracked cursor position. */
    ev->where.h = g_cursor_h;
    ev->where.v = g_cursor_v;
    ev->what    = what;
    return what;
}

/* ===========================================================================
 * 7. GetNextEvent -- non-blocking, single drain (MTE Ch 2 p. 2-26)
 * ===========================================================================*/

int GetNextEvent(flair_raw_ring_t *ring,
                 uint16_t eventMask,
                 EventRecord *theEvent)
{
    flair_raw_event_t raw;
    uint16_t          what;

    if (!ring || !theEvent) { FLAIR_PANIC("GetNextEvent: NULL arg"); }

    /* Drain one raw event. If nothing pending, return nullEvent. */
    if (!drain_one(ring, &raw)) {
        theEvent->what      = (uint16_t)nullEvent;
        theEvent->message   = 0u;
        theEvent->when      = g_tick;
        theEvent->where.h   = g_cursor_h;
        theEvent->where.v   = g_cursor_v;
        theEvent->modifiers = g_modifiers;
        return 0;
    }

    what = cook_raw(&raw, theEvent);

    /* eventMask filter: if this event class is masked out, return nullEvent. */
    if (what != (uint16_t)nullEvent &&
        !((eventMask >> what) & 1u)) {
        /* Masked. Still update state (modifiers / cursor updated in cook_raw),
         * but report nullEvent so the app does not see the masked class. */
        theEvent->what = (uint16_t)nullEvent;
        return 0;
    }

    return (what != (uint16_t)nullEvent) ? 1 : 0;
}

/* ===========================================================================
 * 8. WaitNextEvent -- cooperative pump (MTE Ch 2 p. 2-24)
 * ===========================================================================*/

int WaitNextEvent(flair_raw_ring_t *ring,
                  uint16_t eventMask,
                  EventRecord *theEvent,
                  uint32_t sleepTicks)
{
    uint32_t deadline;
    flair_raw_event_t raw;
    uint16_t what;

    if (!ring || !theEvent) { FLAIR_PANIC("WaitNextEvent: NULL arg"); }

    deadline = g_tick + sleepTicks;

    for (;;) {
        /* Try to drain and cook one raw event. */
        if (drain_one(ring, &raw)) {
            what = cook_raw(&raw, theEvent);
            if (what != (uint16_t)nullEvent &&
                ((eventMask >> what) & 1u)) {
                /* Non-null event that passes the mask: deliver it. */
                return 1;
            }
            /* Masked or cooked to nullEvent: continue draining (multiple
             * raw events may arrive between pump calls in a burst). */
            continue;
        }

        /* Ring empty: check timeout. */
        if (sleepTicks == 0u || g_tick >= deadline) {
            theEvent->what      = (uint16_t)nullEvent;
            theEvent->message   = 0u;
            theEvent->when      = g_tick;
            theEvent->where.h   = g_cursor_h;
            theEvent->where.v   = g_cursor_v;
            theEvent->modifiers = g_modifiers;
            return 0;
        }

        /* Cooperative yield: hand the CPU back until the next IRQ.
         * On bare metal this executes HLT; in the test harness it is a
         * no-op or a tick-advance function (see event.h Sec 3). */
        if (g_yield_fn) g_yield_fn();
        /* After yield, re-check ring (next ISR may have posted an event). */
    }
    /* unreachable */
}

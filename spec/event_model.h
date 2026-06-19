/*
 * spec/event_model.h -- FLAIR Event Manager: the LOCKED contract.
 *
 * LOCKED spec-data (CLAUDE.md Rule 8; PRD Sec 6.3 -- "the Toolbox layer").
 * beads: initech-k8o5.4 (FLAIR event + window model as spec-data).
 *
 * This header locks the EventRecord, the event `what` codes, the modifier
 * flag bits, the event mask bits, and the compact raw-ring record that the
 * ISR enqueues (ADR-0004 D-4). Cooperative scheduling (ADR-0004 D-6) is
 * documented here at the architecture level.
 *
 * SOURCE CITATIONS (Law 1 -- all local or free Apple developer archive):
 *
 *   Inside Macintosh: Macintosh Toolbox Essentials (1992) [MTE; free Apple
 *     dev archive; docs/research/gui-ground-truth.md Sec 3.2]:
 *     Chapter 2 "Event Manager" -- the EventRecord definition (MTE 2-4..2-8):
 *       what, message, when, where, modifiers fields and their semantics.
 *     Table 2-1 (MTE p. 2-5): "Event Code Constants" -- the verbatim `what`
 *       values (nullEvent=0 through osEvt=15).
 *     Table 2-2 (MTE p. 2-6): "Modifier Flag Constants" -- verbatim bit masks
 *       (activeFlag, btnState, cmdKey, shiftKey, alphaLock, optionKey,
 *        controlKey).
 *     Table 2-3 (MTE p. 2-7): "Event Mask Constants" -- event mask bits
 *       (mDownMask, mUpMask, keyDownMask, keyUpMask, autoKeyMask, updateMask,
 *        diskMask, activMask, osMask, everyEvent).
 *
 *   Inside Macintosh Vol I Ch 2 (IM-I) [archive.org bitsavers_applemacIn84;
 *     docs/research/gui-ground-truth.md Sec 3.2]:
 *     "The Event Manager" (IM-I p. I-58 ff): EventRecord fields, `what` code
 *     meanings, modifier bits. Original definitions match MTE; MTE is cited
 *     as the unified, pagination-stable reference for the verbatim values.
 *
 *   ADR-0004 (FLAIR Toolbox Architecture, RATIFIED 2026-06-19):
 *     D-4 (ISR enqueue-only; EventRecord synthesis in task context):
 *       "Hardware interrupt service routines (PS/2 keyboard IRQ1, PS/2 mouse
 *        IRQ12, PIT IRQ0) do the minimum: read the device, push a compact raw
 *        record into a lock-free single-producer/single-consumer ring buffer,
 *        send the PIC EOI(s), and return. ISRs do NOT synthesize EventRecords,
 *        allocate, or call Managers."
 *       "EventRecord SYNTHESIS happens in task context inside WaitNextEvent/
 *        GetNextEvent: the pump drains the raw ring, cooks raw scancodes/
 *        mouse-deltas/ticks into EventRecords (...), stamps when (tick count)
 *        and where (cursor position), and dispatches."
 *     D-6 (cooperative, non-preemptive WaitNextEvent):
 *       "Scheduling is cooperative, non-preemptive, on the PIT tick -- a task
 *        holds the CPU until it calls back into WaitNextEvent; there is no
 *        preemption and no protected inter-process isolation."
 *       "An app that fails to yield hangs the desktop -- that is period-
 *        authentic, not a bug (do not fix with preemption)."
 *
 *   spec/grafport.h (flair_point_t = QuickDraw Point; reused for `where` --
 *     do NOT define a second Point type).
 *
 * DESIGN: EVENT PIPELINE (ADR-0004 D-4 architecture summary)
 * -----------------------------------------------------------
 *
 *   Hardware ISR   ->   raw ring   ->   WaitNextEvent   ->   EventRecord
 *   (IRQ context)       (SPSC)         (task context)        (synthesized)
 *
 *   1. ISR (keyboard IRQ1, mouse IRQ12, PIT IRQ0) wakes; reads the hardware
 *      register; pushes ONE flair_raw_event_t into the SPSC ring (a statically-
 *      allocated lock-free ring with producer head + consumer tail; ISR = sole
 *      producer, WaitNextEvent pump = sole consumer); sends PIC EOI; returns.
 *      The ISR NEVER: synthesizes an EventRecord, allocates memory, calls any
 *      Manager, or touches the Toolbox. (ADR-0004 D-4, Rationale 4.2.)
 *   2. WaitNextEvent (task context, Layer 4, ADR-0004 D-1) drains the ring:
 *      for each flair_raw_event_t it synthesizes an EventRecord (cooks
 *      scancode -> key character; accumulates mouse deltas to cursor position;
 *      stamps `when` from the PIT tick count; stamps `where` from the current
 *      cursor position; builds `message` and `modifiers`; selects event `what`).
 *      It also generates updateEvt, activateEvt, and nullEvent synthetically
 *      (no corresponding ISR input needed for these).
 *   3. The synthesized EventRecord is delivered to the application via the
 *      standard MTE Chapter 2 dispatch.
 *
 * COOPERATIVE SCHEDULING (ADR-0004 D-6):
 *   The ONLY preemption is the PIT tick (IRQ0), which increments the global
 *   tick count and returns. No task switch occurs on the tick. A task runs
 *   until it calls WaitNextEvent (or GetNextEvent), which is the yield point.
 *   An app that does not call WaitNextEvent holds the CPU indefinitely -- this
 *   is period-authentic (PRD Sec 2 non-goals, Sec 15 DECIDED). Do NOT add
 *   preemption; see CLAUDE.md hallucination-risk callout.
 *
 * DUAL-COMPILE: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). Only <stdint.h> + local headers. No host malloc;
 * no libc beyond stdint. Rule 11 (reproducible). ASCII-clean (Rule 12).
 * Changing this file is a deliberate, beads-tracked Rule 8 act.
 */
#ifndef INITECH_SPEC_EVENT_MODEL_H
#define INITECH_SPEC_EVENT_MODEL_H

#include <stdint.h>

/* flair_point_t is already defined in spec/grafport.h (the QuickDraw Point
 * record, v/h 2 x int16_t).  Include it here -- do NOT re-define Point. */
#include "grafport.h"   /* flair_point_t (QuickDraw Point), GrafPort       */

/* ===========================================================================
 * 1. EVENT `what` CODE CONSTANTS  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 2, Table 2-1 (p. 2-5): "Event Code Constants."
 *      IM-I Ch 2 p. I-59 (original definitions; same numeric values).
 *
 * The `what` field of an EventRecord holds one of these values, selecting
 * the event class. The pump synthesizes each class at the point in the
 * pipeline where it is appropriate (task context; ADR-0004 D-4).
 *
 * NOTE on osEvt (15): in Inside Macintosh, osEvt is the "operating-system
 * event" umbrella (suspend/resume events delivered via the message field).
 * InitechOS uses it for PIT-driven cooperative-yield notifications if needed;
 * the suspend/resume sub-event decoding follows MTE Ch 2 (message bit 24
 * = resumeFlag). The other values (diskEvt=7, activateEvt=8) are carried
 * verbatim even though FLAIR's current-release disk / multi-app model is
 * simplified -- the constants are the locked contract.
 * ===========================================================================*/

/* flair_event_what_t -- event class codes (MTE Table 2-1 / IM-I p. I-59). */
typedef enum flair_event_what {
    nullEvent    = 0,  /* no event pending (WaitNextEvent yields with this)   */
    mouseDown    = 1,  /* mouse button pressed    (MTE 2-5 "mouseDown")        */
    mouseUp      = 2,  /* mouse button released   (MTE 2-5 "mouseUp")          */
    keyDown      = 3,  /* key pressed             (MTE 2-5 "keyDown")          */
    keyUp        = 4,  /* key released            (MTE 2-5 "keyUp")            */
    autoKey      = 5,  /* key held for auto-repeat (MTE 2-5 "autoKey")         */
    updateEvt    = 6,  /* window needs update     (MTE 2-5 "updateEvt")        */
    diskEvt      = 7,  /* disk inserted           (MTE 2-5 "diskEvt")          */
    activateEvt  = 8,  /* window activate/deactivate (MTE 2-5 "activateEvt")   */
    /* 9..14 reserved (not defined by MTE for the Toolbox Essentials set)      */
    osEvt        = 15  /* OS/suspend/resume event (MTE 2-5 "osEvt")            */
} flair_event_what_t;

/* ===========================================================================
 * 2. MODIFIER FLAG BIT CONSTANTS  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 2, Table 2-2 (p. 2-6): "Modifier Flag Constants."
 *      IM-I Ch 2 p. I-62 (original definitions; same bit positions).
 *
 * The `modifiers` field of an EventRecord is a uint16_t bit field. Each bit
 * signals a keyboard-modifier or mouse-button state at the time the event was
 * synthesized (task context, ADR-0004 D-4 -- the pump applies the current
 * modifier state when building the EventRecord from the raw ring).
 *
 * activeFlag (0x0001): event is an activate (not a deactivate) event.
 *   Set only for activateEvt events; 0 = deactivate, 1 = activate (MTE 2-6).
 * btnState   (0x0080): mouse button state. 1 = button UP (not pressed);
 *   0 = button DOWN (pressed). CAUTION: this bit is 1 when NOT pressed --
 *   the polarity is inverted relative to intuition. (MTE 2-6 / IM-I p. I-62.)
 * cmdKey     (0x0100): Command key (Apple/Clover key) is held down (MTE 2-6).
 * shiftKey   (0x0200): Shift key is held down (MTE 2-6).
 * alphaLock  (0x0400): Caps Lock is on (MTE 2-6).
 * optionKey  (0x0800): Option (Alt) key is held down (MTE 2-6).
 * controlKey (0x1000): Control key is held down (MTE 2-6).
 * ===========================================================================*/

#define FLAIR_EVT_MOD_ACTIVE_FLAG  0x0001u  /* activateEvt: 1=activate (MTE Table 2-2) */
#define FLAIR_EVT_MOD_BTN_STATE    0x0080u  /* mouse button: 1=UP (inverted; MTE 2-6)  */
#define FLAIR_EVT_MOD_CMD_KEY      0x0100u  /* Command key held (MTE Table 2-2)        */
#define FLAIR_EVT_MOD_SHIFT_KEY    0x0200u  /* Shift key held (MTE Table 2-2)          */
#define FLAIR_EVT_MOD_ALPHA_LOCK   0x0400u  /* Caps Lock on (MTE Table 2-2)            */
#define FLAIR_EVT_MOD_OPTION_KEY   0x0800u  /* Option (Alt) key held (MTE Table 2-2)   */
#define FLAIR_EVT_MOD_CONTROL_KEY  0x1000u  /* Control key held (MTE Table 2-2)        */

/* ===========================================================================
 * 3. EVENT MASK BIT CONSTANTS  (verbatim Inside Macintosh values)
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 2, Table 2-3 (p. 2-7): "Event Mask Constants."
 *      IM-I Ch 2 (GetNextEvent mask parameter; same values).
 *
 * The `eventMask` parameter to WaitNextEvent / GetNextEvent is a uint16_t
 * whose bits select which event classes the pump may return. Each bit
 * corresponds to one event `what` code: mask bit (1 << what). The pump
 * suppresses events whose class has a 0 bit in the mask.
 *
 * Derivation: mDownMask = (1 << mouseDown) = (1 << 1) = 0x0002.
 * This formula holds for every `what` code in Section 1 above. everyEvent is
 * all bits set (MTE 2-7: "all events"); the pump typically uses everyEvent.
 * ===========================================================================*/

#define FLAIR_MASK_NULL_EVENT  0x0001u   /* (1 << nullEvent=0)    MTE Table 2-3 */
#define FLAIR_MASK_MOUSE_DOWN  0x0002u   /* (1 << mouseDown=1)    MTE Table 2-3 */
#define FLAIR_MASK_MOUSE_UP    0x0004u   /* (1 << mouseUp=2)      MTE Table 2-3 */
#define FLAIR_MASK_KEY_DOWN    0x0008u   /* (1 << keyDown=3)      MTE Table 2-3 */
#define FLAIR_MASK_KEY_UP      0x0010u   /* (1 << keyUp=4)        MTE Table 2-3 */
#define FLAIR_MASK_AUTO_KEY    0x0020u   /* (1 << autoKey=5)      MTE Table 2-3 */
#define FLAIR_MASK_UPDATE      0x0040u   /* (1 << updateEvt=6)    MTE Table 2-3 */
#define FLAIR_MASK_DISK        0x0080u   /* (1 << diskEvt=7)      MTE Table 2-3 */
#define FLAIR_MASK_ACTIVATE    0x0100u   /* (1 << activateEvt=8)  MTE Table 2-3 */
/* bits 9..14 (reserved what codes) not defined as public masks                 */
#define FLAIR_MASK_OS_EVT      0x8000u   /* (1 << osEvt=15)       MTE Table 2-3 */
#define FLAIR_MASK_EVERY_EVENT 0xFFFFu   /* all event classes      MTE Table 2-3 */

/* Aliases using the verbatim Inside Macintosh constant names (MTE Table 2-3).
 * These match the Pascal constant names in the IM source; kept alongside the
 * FLAIR_MASK_* prefixed forms for search-ability without namespace collision. */
#define mDownMask   FLAIR_MASK_MOUSE_DOWN  /* MTE "mDownMask"   */
#define mUpMask     FLAIR_MASK_MOUSE_UP    /* MTE "mUpMask"     */
#define keyDownMask FLAIR_MASK_KEY_DOWN    /* MTE "keyDownMask" */
#define keyUpMask   FLAIR_MASK_KEY_UP      /* MTE "keyUpMask"   */
#define autoKeyMask FLAIR_MASK_AUTO_KEY    /* MTE "autoKeyMask" */
#define updateMask  FLAIR_MASK_UPDATE      /* MTE "updateMask"  */
#define diskMask    FLAIR_MASK_DISK        /* MTE "diskMask"    */
#define activMask   FLAIR_MASK_ACTIVATE    /* MTE "activMask"   */
#define osMask      FLAIR_MASK_OS_EVT      /* MTE "osMask"      */
#define everyEvent  FLAIR_MASK_EVERY_EVENT /* MTE "everyEvent"  */

/* ===========================================================================
 * 4. EventRecord -- the synthesized event structure
 * ---------------------------------------------------------------------------
 * Ref: MTE Chapter 2, "The EventRecord Data Type" (p. 2-4):
 *   "Every event has an associated event record, which stores information
 *    about the event. Your application examines the fields of an event record
 *    to determine what kind of event occurred and what response to make."
 *   Fields: what, message, when, where, modifiers (verbatim IM names).
 *
 * Field semantics (MTE p. 2-4..2-8):
 *
 *   what      (uint16_t): event class code; one of flair_event_what_t above.
 *             MTE p. 2-5: "The what field contains the event code for the
 *             type of event."
 *
 *   message   (uint32_t): event-specific payload. Semantics depend on `what`:
 *             - keyDown/keyUp/autoKey: low byte = ASCII char; byte 1 =
 *               key code; bits 8-15 = virtual key code (MTE p. 2-7
 *               "message field contains the character code of the key").
 *             - mouseDown/mouseUp: undefined / 0 (MTE p. 2-6 -- the click
 *               position is in `where`, not `message`).
 *             - updateEvt/activateEvt: the WindowPtr (window pointer) of the
 *               affected window (MTE p. 2-7 "message ... contains a pointer
 *               to the window that needs updating").
 *             - diskEvt: disk-driver result code in high word; drive number
 *               in low word (MTE p. 2-8).
 *             - osEvt: sub-event code (suspend/resume) in high byte (MTE 2-8).
 *             FLAIR encodes the WindowPtr as a uint32_t (flat 32-bit address,
 *             ADR-0001; the receiver casts to WindowRecord * as needed).
 *             Ref: IM-I Ch 2 p. I-59 "The message field."
 *
 *   when      (uint32_t): event time as a PIT tick count since system startup.
 *             MTE p. 2-6: "The when field contains the time at which the event
 *             occurred, expressed in ticks." (1 tick = 1/60 second on the Mac;
 *             FLAIR uses the same 60 Hz PIT tick granularity, ADR-0004 D-6.)
 *             The pump stamps `when` from the global tick counter at synthesis
 *             time (task context; ADR-0004 D-4). NOT a wall-clock timestamp
 *             (no host time, Rule 11 reproducible).
 *
 *   where     (flair_point_t): cursor position in GLOBAL (screen) coordinates
 *             at the time of the event. MTE p. 2-6: "The where field contains
 *             the location of the mouse cursor at the time the event occurred,
 *             expressed in global coordinates."
 *             FLAIR reuses flair_point_t (= QuickDraw Point, spec/grafport.h)
 *             for the `where` field. Do NOT define a second Point typedef;
 *             flair_point_t is the single Point type in this codebase.
 *
 *   modifiers (uint16_t): modifier-key and mouse-button state bits at event
 *             time. MTE p. 2-6: "The modifiers field contains information
 *             about the state of the mouse button and modifier keys."
 *             Bit mask: see Section 2 above (FLAIR_EVT_MOD_* constants).
 * ===========================================================================*/

/*
 * EventRecord (verbatim QuickDraw/Toolbox name, MTE Ch 2 p. 2-4; IM-I Ch 2):
 *   "Every event has an associated event record."
 * FLAIR Lock: synthesized ONLY in task context inside WaitNextEvent/
 * GetNextEvent; never inside an ISR (ADR-0004 D-4, C-3).
 */
typedef struct EventRecord {
    uint16_t       what;       /* event class (flair_event_what_t; MTE 2-5)   */
    uint32_t       message;    /* event payload (semantics depend on what)     */
    uint32_t       when;       /* tick count at synthesis (MTE 2-6; 60 Hz)    */
    flair_point_t  where;      /* cursor pos in global coords (MTE 2-6)       */
    uint16_t       modifiers;  /* modifier bits (FLAIR_EVT_MOD_*; MTE 2-6)    */
} EventRecord;

/* ===========================================================================
 * 5. flair_raw_event_t -- the compact ISR-enqueued raw record
 * ---------------------------------------------------------------------------
 * Ref: ADR-0004 D-4 -- "push a compact raw record into a lock-free
 *      single-producer/single-consumer ring buffer."
 *
 * This is NOT an Inside Macintosh type. It is FLAIR's internal compact record
 * that an ISR pushes into the SPSC ring. It must be:
 *   - Small: a struct that fits in a few uint32_t slots (ring slots are
 *     sized to this record; static ring, no allocation).
 *   - ISR-safe: no pointers to heap objects; plain integer fields only.
 *   - Self-describing: carries a `kind` tag so the pump can route it.
 *
 * The pump (WaitNextEvent task context) reads flair_raw_event_t records out
 * of the ring, accumulates state (mouse position, modifier keys, tick count),
 * and synthesizes the final EventRecord. ISRs NEVER do this synthesis.
 *
 * The raw ring is an SPSC (single-producer / single-consumer) ring:
 *   - Producer (ISR): writes head; never reads tail; atomic store of head.
 *   - Consumer (pump): reads tail; never writes head; atomic load of head.
 *   - No locking required because x86 stores/loads of aligned uint32_t are
 *     atomic on the single 386+ core (cooperative single-CPU system; no SMP;
 *     ADR-0004 D-6 cooperative scheduling; no cache coherence problem).
 *   - Ring capacity is a power-of-two constant (FLAIR_RAW_RING_CAP below)
 *     so head/tail wrap by AND mask; no modulus.
 *   - An ISR that finds the ring full DROPS the raw event and increments a
 *     drop counter (fail-fast telemetry, Rule 2; not a panic -- an ISR cannot
 *     safely panic).
 * ===========================================================================*/

/* flair_raw_kind_t -- identifies the hardware source of a raw event. */
typedef enum flair_raw_kind {
    FLAIR_RAW_KEYBOARD = 0,  /* PS/2 keyboard (IRQ1): scancode byte          */
    FLAIR_RAW_MOUSE    = 1,  /* PS/2 mouse (IRQ12): delta X, delta Y, buttons */
    FLAIR_RAW_TICK     = 2   /* PIT tick (IRQ0): heartbeat / tick increment   */
} flair_raw_kind_t;

/*
 * flair_raw_event_t -- compact ISR-enqueued raw event (FLAIR-internal type).
 *
 * Total size: 12 bytes (3 x uint32_t), naturally aligned. The ring buffer
 * holds FLAIR_RAW_RING_CAP of these records; the ring itself is statically
 * allocated in the Layer 4 cooperative event core (no malloc, Rule 11).
 *
 * Field layout:
 *   kind    (uint32_t): flair_raw_kind_t tag -- routes the record in the pump.
 *   tick    (uint32_t): global PIT tick count AT THE TIME THE ISR FIRED.
 *                       The pump uses this for the `when` field in EventRecord
 *                       (not the tick at pump time, for accuracy).
 *   payload (uint32_t): kind-specific data packed into one word --
 *     KEYBOARD: low byte = raw PS/2 scancode byte (0x00..0xFF).
 *               bit 8 = key-break flag (1 = key released; set by ISR when
 *               0xF0 break prefix consumed from the PS/2 stream).
 *               bits 9..31 reserved/zero.
 *     MOUSE:    bits  0..7  = raw button byte (bit 0 = left; bit 1 = right;
 *                             bit 2 = middle; from PS/2 mouse data byte 0).
 *               bits  8..15 = signed delta X as int8_t (from PS/2 byte 1,
 *                             sign-extended from bits 0..7 + sign bit 4 of
 *                             byte 0; values -256..+255 representable).
 *               bits 16..23 = signed delta Y as int8_t (from PS/2 byte 2,
 *                             sign-extended similarly; Y axis inverted from
 *                             PS/2 convention to screen-down-positive).
 *               bits 24..31 reserved/zero.
 *     TICK:     payload = 0 (the tick itself is in the `tick` field; the
 *               TICK raw event is enqueued to let the pump update its tick
 *               accumulator and generate null events at the right cadence).
 */
typedef struct flair_raw_event {
    uint32_t  kind;     /* flair_raw_kind_t tag (uint32_t for alignment)      */
    uint32_t  tick;     /* PIT tick count when ISR fired (60 Hz counter)      */
    uint32_t  payload;  /* kind-specific packed data (see above)              */
} flair_raw_event_t;

/*
 * FLAIR_RAW_RING_CAP -- capacity of the SPSC raw-event ring.
 * Must be a power of two (wrap by AND mask = FLAIR_RAW_RING_CAP - 1).
 * 64 slots x 12 bytes = 768 bytes; ample for burst IRQ traffic between
 * WaitNextEvent calls in a cooperative desktop (ADR-0004 D-6; a task that
 * holds the CPU for longer than 64 hardware events is already broken).
 * Sized to avoid a full-ring ISR drop under worst-case key-repeat bursts
 * (autoKey at 30 events/sec * generous 2-second hold = 60 events headroom).
 * Locked (Rule 8): changing this is a deliberate, beads-tracked act.
 */
#define FLAIR_RAW_RING_CAP  64u    /* SPSC ring capacity; MUST be a power of 2 */

/* Mask for head/tail wrap (bit-AND, no modulus -- valid only while power-of-2).*/
#define FLAIR_RAW_RING_MASK (FLAIR_RAW_RING_CAP - 1u)

/* ===========================================================================
 * 6. COMPILE-TIME CONTRACT CHECKS  (the oracle bites at build time)
 * ---------------------------------------------------------------------------
 * Style follows spec/region_algebra.h (the style exemplar).
 * ===========================================================================*/

/* Event `what` codes -- verbatim IM values (MTE Table 2-1 / IM-I p. I-59). */
_Static_assert((int)nullEvent   == 0,  "nullEvent=0 (MTE Table 2-1)");
_Static_assert((int)mouseDown   == 1,  "mouseDown=1 (MTE Table 2-1)");
_Static_assert((int)mouseUp     == 2,  "mouseUp=2 (MTE Table 2-1)");
_Static_assert((int)keyDown     == 3,  "keyDown=3 (MTE Table 2-1)");
_Static_assert((int)keyUp       == 4,  "keyUp=4 (MTE Table 2-1)");
_Static_assert((int)autoKey     == 5,  "autoKey=5 (MTE Table 2-1)");
_Static_assert((int)updateEvt   == 6,  "updateEvt=6 (MTE Table 2-1)");
_Static_assert((int)diskEvt     == 7,  "diskEvt=7 (MTE Table 2-1)");
_Static_assert((int)activateEvt == 8,  "activateEvt=8 (MTE Table 2-1)");
_Static_assert((int)osEvt       == 15, "osEvt=15 (MTE Table 2-1)");

/* Modifier flag bits -- verbatim IM values (MTE Table 2-2). */
_Static_assert(FLAIR_EVT_MOD_ACTIVE_FLAG == 0x0001u,
               "activeFlag=0x0001 (MTE Table 2-2)");
_Static_assert(FLAIR_EVT_MOD_BTN_STATE   == 0x0080u,
               "btnState=0x0080 (MTE Table 2-2; 1=button UP)");
_Static_assert(FLAIR_EVT_MOD_CMD_KEY     == 0x0100u,
               "cmdKey=0x0100 (MTE Table 2-2)");
_Static_assert(FLAIR_EVT_MOD_SHIFT_KEY   == 0x0200u,
               "shiftKey=0x0200 (MTE Table 2-2)");
_Static_assert(FLAIR_EVT_MOD_ALPHA_LOCK  == 0x0400u,
               "alphaLock=0x0400 (MTE Table 2-2)");
_Static_assert(FLAIR_EVT_MOD_OPTION_KEY  == 0x0800u,
               "optionKey=0x0800 (MTE Table 2-2)");
_Static_assert(FLAIR_EVT_MOD_CONTROL_KEY == 0x1000u,
               "controlKey=0x1000 (MTE Table 2-2)");

/* No two modifier bits overlap (all are distinct powers of 2). */
_Static_assert((FLAIR_EVT_MOD_ACTIVE_FLAG & FLAIR_EVT_MOD_BTN_STATE)   == 0u,
               "activeFlag and btnState do not overlap");
_Static_assert((FLAIR_EVT_MOD_BTN_STATE   & FLAIR_EVT_MOD_CMD_KEY)     == 0u,
               "btnState and cmdKey do not overlap");
_Static_assert((FLAIR_EVT_MOD_CMD_KEY     & FLAIR_EVT_MOD_SHIFT_KEY)   == 0u,
               "cmdKey and shiftKey do not overlap");
_Static_assert((FLAIR_EVT_MOD_SHIFT_KEY   & FLAIR_EVT_MOD_ALPHA_LOCK)  == 0u,
               "shiftKey and alphaLock do not overlap");
_Static_assert((FLAIR_EVT_MOD_ALPHA_LOCK  & FLAIR_EVT_MOD_OPTION_KEY)  == 0u,
               "alphaLock and optionKey do not overlap");
_Static_assert((FLAIR_EVT_MOD_OPTION_KEY  & FLAIR_EVT_MOD_CONTROL_KEY) == 0u,
               "optionKey and controlKey do not overlap");

/* Event mask bits derived as (1 << what); verify the key ones. */
_Static_assert(mDownMask   == (1u << (unsigned)mouseDown),
               "mDownMask = 1<<mouseDown (MTE Table 2-3)");
_Static_assert(mUpMask     == (1u << (unsigned)mouseUp),
               "mUpMask = 1<<mouseUp (MTE Table 2-3)");
_Static_assert(keyDownMask == (1u << (unsigned)keyDown),
               "keyDownMask = 1<<keyDown (MTE Table 2-3)");
_Static_assert(updateMask  == (1u << (unsigned)updateEvt),
               "updateMask = 1<<updateEvt (MTE Table 2-3)");
_Static_assert(activMask   == (1u << (unsigned)activateEvt),
               "activMask = 1<<activateEvt (MTE Table 2-3)");
_Static_assert(osMask      == (1u << (unsigned)osEvt),
               "osMask = 1<<osEvt (MTE Table 2-3)");
_Static_assert(everyEvent  == 0xFFFFu,
               "everyEvent = 0xFFFF (MTE Table 2-3)");

/* EventRecord field layout (verbatim MTE Chapter 2 order). */
_Static_assert(offsetof(EventRecord, what)      == 0,
               "EventRecord.what must be first field (MTE Ch 2 field order)");
_Static_assert(offsetof(EventRecord, message)   > offsetof(EventRecord, what),
               "EventRecord.message must follow what");
_Static_assert(offsetof(EventRecord, when)      > offsetof(EventRecord, message),
               "EventRecord.when must follow message");
_Static_assert(offsetof(EventRecord, where)     > offsetof(EventRecord, when),
               "EventRecord.where must follow when");
_Static_assert(offsetof(EventRecord, modifiers) > offsetof(EventRecord, where),
               "EventRecord.modifiers must follow where");

/* EventRecord field sizes. */
_Static_assert(sizeof(((EventRecord *)0)->what)      == 2,
               "EventRecord.what is uint16_t (2 bytes)");
_Static_assert(sizeof(((EventRecord *)0)->message)   == 4,
               "EventRecord.message is uint32_t (4 bytes)");
_Static_assert(sizeof(((EventRecord *)0)->when)      == 4,
               "EventRecord.when is uint32_t (4 bytes; tick count)");
_Static_assert(sizeof(((EventRecord *)0)->where)     == 4,
               "EventRecord.where is flair_point_t (4 bytes; QuickDraw Point)");
_Static_assert(sizeof(((EventRecord *)0)->modifiers) == 2,
               "EventRecord.modifiers is uint16_t (2 bytes)");

/* flair_raw_event_t is 3 x uint32_t = 12 bytes (ISR-safe, aligned). */
_Static_assert(sizeof(flair_raw_event_t) == 12,
               "flair_raw_event_t must be 12 bytes (3 x uint32_t; ISR ring slot)");
_Static_assert(offsetof(flair_raw_event_t, kind)    == 0,
               "flair_raw_event_t.kind at offset 0");
_Static_assert(offsetof(flair_raw_event_t, tick)    == 4,
               "flair_raw_event_t.tick at offset 4");
_Static_assert(offsetof(flair_raw_event_t, payload) == 8,
               "flair_raw_event_t.payload at offset 8");

/* Ring capacity must be a power of two (mask trick is only valid then). */
_Static_assert(FLAIR_RAW_RING_CAP > 0u,
               "FLAIR_RAW_RING_CAP must be positive");
_Static_assert((FLAIR_RAW_RING_CAP & (FLAIR_RAW_RING_CAP - 1u)) == 0u,
               "FLAIR_RAW_RING_CAP must be a power of 2 (SPSC mask wrap)");
_Static_assert(FLAIR_RAW_RING_MASK == (FLAIR_RAW_RING_CAP - 1u),
               "FLAIR_RAW_RING_MASK == FLAIR_RAW_RING_CAP - 1");

#endif /* INITECH_SPEC_EVENT_MODEL_H */

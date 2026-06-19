/*
 * os/flair/event.h -- FLAIR Event Manager: Layer 4 cooperative event core.
 *
 * beads: initech-8b7 (FLAIR Event Manager: WaitNextEvent + SPSC ring).
 * Ref:   ADR-0004 D-4 (ISR enqueue-only; EventRecord synthesis in task context):
 *          "Hardware interrupt service routines ... do the minimum: read the
 *           device, push a compact raw record into a lock-free single-producer/
 *           single-consumer ring buffer, send the PIC EOI(s), and return.
 *           ISRs do NOT synthesize EventRecords, allocate, or call Managers.
 *           EventRecord SYNTHESIS happens in task context inside WaitNextEvent/
 *           GetNextEvent: the pump drains the raw ring, cooks raw scancodes/
 *           mouse-deltas/ticks into EventRecords, stamps when (tick count) and
 *           where (cursor position), and dispatches."
 * Ref:   ADR-0004 D-6 (cooperative, non-preemptive WaitNextEvent):
 *          "A task holds the CPU until it calls back into WaitNextEvent; there
 *           is no preemption and no protected inter-process isolation. An app
 *           that fails to yield hangs the desktop -- that is period-authentic,
 *           not a bug (CLAUDE.md hallucination-risk callout; do not 'fix' with
 *           preemption)."
 * Ref:   ADR-0004 D-8 (test-event oracle -- D-8 table row):
 *          "A recorded raw-input trace replays to a deterministic EventRecord/
 *           dispatch sequence."
 * Ref:   spec/event_model.h -- LOCKED EventRecord, flair_raw_event_t,
 *         flair_event_what_t, modifier constants, mask constants, FLAIR_RAW_RING_CAP.
 * Ref:   spec/grafport.h -- flair_point_t (QuickDraw Point; the `where` type).
 *          Do NOT define a second Point type (spec/event_model.h warning).
 * Ref:   Inside Macintosh: Macintosh Toolbox Essentials (1992) [MTE] Ch 2
 *          "Event Manager" -- WaitNextEvent, GetNextEvent signatures and semantics
 *          (MTE p. 2-24..2-27); verbatim names preserved.
 *
 * ARTIFACT code: freestanding (gcc -m32 -ffreestanding -nostdlib -std=c11)
 * AND hosted (cc -std=c11). <stdint.h> only. No host malloc. No libc beyond
 * stdint. Rule 11 (reproducible). Rule 12 (ASCII-clean). Law 3 (dual-compile).
 *
 * COOPERATIVE YIELD HOOK
 * ----------------------
 * On bare metal, WaitNextEvent spins or executes HLT while waiting for new
 * events. On the host (test harness), a real HLT is illegal. To make the
 * Event Manager host-testable without baking in any OS-specific idle
 * instruction, the yield behaviour is abstracted as a function pointer:
 *
 *   flair_event_yield_fn
 *
 * When WaitNextEvent finds the ring empty and the sleepTicks timeout has not
 * expired, it calls this hook once per iteration. On bare metal the kernel
 * installs a stub that executes HLT (halts until the next IRQ, cooperative
 * energy-efficient idle). In the test harness the caller installs a no-op
 * (or a function that advances the simulated tick count). The hook receives
 * no arguments and returns void; it is never NULL in normal operation (a NULL
 * hook is treated as a tight spin -- fail-loud philosophy: a NULL hook in
 * production is detectable by the per-iteration panic path).
 *
 * This is the ONLY architectural seam between the Event Manager and the
 * platform layer; it keeps ISR-context completely separate from task-context
 * (ADR-0004 D-4 C-3).
 */
#ifndef INITECH_OS_FLAIR_EVENT_H
#define INITECH_OS_FLAIR_EVENT_H

#include <stdint.h>

/* Pull in the LOCKED spec: EventRecord, flair_raw_event_t, what codes,
 * modifier bits, mask bits, FLAIR_RAW_RING_CAP / FLAIR_RAW_RING_MASK.
 * Include via "event_model.h" so -Ispec on the compile line resolves it.  */
#include "event_model.h"   /* spec/event_model.h (LOCKED; -Ispec)          */

/* ============================================================
 * 1. THE SPSC RAW-EVENT RING
 * ----------------------------------------------------------
 * Lock-free single-producer / single-consumer ring of flair_raw_event_t.
 *
 * On-metal x86 (cooperative, single-CPU, no SMP -- ADR-0004 D-6):
 *   - Aligned uint32_t load/store is atomic on the 386+ core.
 *   - The ISR (producer) writes head only; the pump (consumer) writes tail
 *     only; neither reads the other's index without a volatile-qualified
 *     load (enforced by the uint32_t volatile head/tail below).
 *   - No memory barrier beyond volatile is needed on a single-core 386:
 *     all writes complete in program order at the ISR/pump boundary.
 *     (A multi-core port would need barriers; that is out of scope for the
 *     cooperative-desktop target -- ADR-0004 D-6 non-goal statement.)
 *
 * FULL-RING POLICY (Rule 2 -- fail loud):
 *   An ISR that finds the ring full DROPS the raw event (cannot panic safely
 *   from ISR context -- see spec/event_model.h Sec 5 rationale) and
 *   increments drop_count. The host-testable flair_raw_post() function
 *   returns 0 (dropped) or 1 (enqueued); the ISR wrapper checks the return
 *   and may assert in debug builds. The drop counter is readable by the pump
 *   for telemetry. This is the documented, defined, non-silent failure mode;
 *   it is NOT silent corruption.
 * ============================================================ */

typedef struct flair_raw_ring {
    flair_raw_event_t  slots[FLAIR_RAW_RING_CAP]; /* ring storage (static)  */
    volatile uint32_t  head;     /* producer (ISR) write index (UNMASKED)     */
    volatile uint32_t  tail;     /* consumer (pump) read index (UNMASKED)     */
    uint32_t           drop_count; /* events dropped due to full ring (telemetry) */
} flair_raw_ring_t;
/*
 * SPSC INDEX DESIGN: head and tail are UNMASKED monotonically-increasing
 * counters (never wrapped to 0 by the ring logic; uint32_t overflow wraps
 * after ~4 billion events, not a concern for a cooperative desktop session).
 * Slot access always uses: slots[index & FLAIR_RAW_RING_MASK].
 *
 * Full:  (head - tail) >= FLAIR_RAW_RING_CAP   (unsigned subtraction).
 * Empty: head == tail.
 *
 * This avoids the "is next_head == tail empty or full?" ambiguity that
 * arises when indices are kept pre-masked (the classic SPSC ring pitfall).
 * Since head >= tail always holds for a well-behaved producer+consumer,
 * the unsigned subtract gives the occupancy correctly even after uint32_t
 * wrap (both head and tail wrap together at the same rate).
 */

/*
 * flair_raw_post -- ISR producer side: enqueue one raw event.
 *
 * Called from ISR context ONLY. Must be called after the hardware register
 * has been read and before the PIC EOI is sent (the caller -- the ISR
 * wrapper -- does the EOI after returning; this function is pure ring logic).
 *
 * Returns 1 if the event was enqueued, 0 if the ring was full (dropped).
 * The ISR must treat 0 as the defined drop case (Rule 2 telemetry).
 *
 * Lock-free SPSC: reads head (owner), reads tail as volatile (to see the
 * consumer's progress), computes next head, checks for full (next == tail),
 * writes slot, stores new head with a volatile write.
 */
int flair_raw_post(flair_raw_ring_t *ring, const flair_raw_event_t *raw);

/* ============================================================
 * 2. GLOBAL STATE: TICK COUNTER + CURSOR POSITION
 * ----------------------------------------------------------
 * These live in the event.c translation unit. They are written by:
 *   - flair_tick_advance(): called from the PIT IRQ0 ISR (or from the
 *     test harness tick-injector). Updates the global tick count by 1.
 *   - The pump inside WaitNextEvent: reads the tick count for `when`
 *     stamps; updates cursor position from mouse-delta raw events.
 *
 * Cursor position is tracked in screen (global) coordinates, clamped to
 * the 640x480 frame (spec: OD-3 in ADR-0004 Sec 3.0; 640x480 is the
 * M3/M4 native resolution).
 * ============================================================ */

#define FLAIR_SCREEN_W  640   /* native resolution, ADR-0004 OD-3 */
#define FLAIR_SCREEN_H  480   /* native resolution, ADR-0004 OD-3 */

/* flair_tick_advance -- advance the global tick counter by 1.
 * Called from the PIT ISR (IRQ0). Must be ISR-safe (no Toolbox calls). */
void flair_tick_advance(void);

/* flair_tick_count -- read the current tick count (monotonic since boot).
 * Task-context read; no synchronisation needed (single-CPU cooperative). */
uint32_t flair_tick_count(void);

/* ============================================================
 * 3. COOPERATIVE YIELD HOOK
 * ----------------------------------------------------------
 * See the module-level comment above for the full rationale.
 *
 * The kernel/boot layer installs a hook before the desktop runs:
 *   flair_event_set_yield(my_hlt_stub);
 *
 * The test harness installs a no-op or a tick-advance function:
 *   flair_event_set_yield(test_noop_yield);
 *
 * The hook is called once per iteration when WaitNextEvent finds no ready
 * event and the sleep timeout has not expired.
 * ============================================================ */

typedef void (*flair_event_yield_fn)(void);

/* Install the yield hook. Must be called before WaitNextEvent is first used.
 * Passing NULL disables yielding (tight spin -- acceptable for unit tests). */
void flair_event_set_yield(flair_event_yield_fn fn);

/* ============================================================
 * 4. EVENT MANAGER API  (verbatim Inside Macintosh names, MTE Ch 2)
 * ----------------------------------------------------------
 * Ref: MTE p. 2-24 "WaitNextEvent":
 *   "OSErr WaitNextEvent (EventMask eventMask, EventRecord *theEvent,
 *    UInt32 sleep, RgnHandle mouseRgn);"
 * Ref: MTE p. 2-26 "GetNextEvent":
 *   "Boolean GetNextEvent (EventMask eventMask, EventRecord *theEvent);"
 *
 * FLAIR simplification: mouseRgn is omitted (InitechOS window/app switching
 * is cooperative and handled by the desktop shell at layer 5; the mouse
 * cursor region notification is a M5+ refinement). sleep is sleepTicks
 * (uint32_t tick count to wait before returning nullEvent).
 *
 * The pump always drains from the shared global ring below; the ring is
 * passed here as a pointer so that the test harness can supply its own
 * ring without a global (dual-compile host-testable pattern, matching
 * the region engine and heap allocator conventions).
 *
 * NOTE on ISR-enqueue / task-synthesis split (ADR-0004 D-4 / C-3):
 *   - ISRs call flair_raw_post(ring, raw) -- they NEVER call WaitNextEvent
 *     or GetNextEvent or any Manager function.
 *   - WaitNextEvent / GetNextEvent are called ONLY from task context.
 *   These two sets of callers are mutually exclusive by construction
 *   (cooperative scheduling -- ADR-0004 D-6; the pump runs between events,
 *   the ISR fires between pump iterations; there is no concurrent call).
 * ============================================================ */

/*
 * WaitNextEvent -- cooperative event pump (MTE Ch 2 p. 2-24).
 *
 * Drains the raw ring in task context; synthesizes EventRecords; filters by
 * eventMask; returns the front event (returns 1 / true if a non-null event
 * is returned) or nullEvent after sleepTicks ticks (returns 0 / false).
 *
 * Cooperative yield: while the ring is empty and the sleep timeout has not
 * expired, the yield hook is called once per iteration (see Sec 3 above).
 *
 * Parameters:
 *   ring       -- the SPSC raw ring to drain (typically the global ring).
 *   eventMask  -- which event classes to return (FLAIR_MASK_* / everyEvent).
 *   theEvent   -- output: the synthesized EventRecord (always written;
 *                 nullEvent on timeout or empty ring).
 *   sleepTicks -- max ticks to wait before returning nullEvent; 0 = return
 *                 immediately even if nothing is queued.
 *
 * Returns 1 if a non-nullEvent was returned, 0 otherwise (MTE convention:
 * WaitNextEvent returns a Boolean, true iff the returned event is non-null).
 */
int WaitNextEvent(flair_raw_ring_t *ring,
                  uint16_t eventMask,
                  EventRecord *theEvent,
                  uint32_t sleepTicks);

/*
 * GetNextEvent -- non-blocking event retrieval (MTE Ch 2 p. 2-26).
 *
 * Drains one pending raw event from the ring (task context) and returns it
 * if it matches eventMask. Returns 1 (true) if an event was returned, 0
 * (false) if none was pending or the pending event was masked out. Does
 * NOT yield; does NOT wait.
 *
 * This is the "check once and return" variant used by apps that have
 * other work to do between event checks (MTE p. 2-26 "Your application
 * can call GetNextEvent to get the next event in the event queue without
 * having to wait for one to occur").
 */
int GetNextEvent(flair_raw_ring_t *ring,
                 uint16_t eventMask,
                 EventRecord *theEvent);

/* ============================================================
 * 5. FLAIR EVENT MANAGER INITIALISATION
 * ----------------------------------------------------------
 * flair_event_init -- initialise the ring, cursor state, and tick counter.
 * Must be called once before any ISR or pump call.
 * On bare metal this is called by the kernel before enabling IRQs.
 * In the test harness it is called by the test fixture.
 * ============================================================ */
void flair_event_init(flair_raw_ring_t *ring);

#endif /* INITECH_OS_FLAIR_EVENT_H */

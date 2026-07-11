/* mouse_pack.h -- pure PS/2-delta -> FLAIR_RAW_MOUSE payload packing.
 *
 * beads: initech-8f5p (independent physical-direction oracle + the producer
 *          sign fix); initech-rgt8 (the BUG this fix resolves: vertical mouse
 *          axis INVERTED because the producer packed the RAW PS/2 dy with no
 *          Y-flip, violating the locked ring-payload contract).
 *
 * Ref (Law 1):
 *   - spec/event_model.h Sec 5 (flair_raw_event_t MOUSE payload, LOCKED):
 *     "bits 16..23 = signed delta Y as int8_t ... Y axis inverted from PS/2
 *      convention to screen-down-positive." The flip MUST happen BEFORE the
 *      value enters the SPSC ring -- i.e. here, in the producer, not in
 *      cook_raw (os/flair/event.c), which only accumulates an
 *      already-flipped delta (g_cursor_v += dy).
 *   - os/milton/mouse.c: int8_t dy = (int8_t)g_pkt[2]; "signed PS/2 Y delta
 *     (+ = up)" -- the raw hardware/PS/2 convention (IBM PS/2 Hardware
 *     Interface Technical Reference / Intel 8042 aux-device byte stream):
 *     PS/2 byte 2 is +N for physical UP, -N for physical DOWN.
 *   - Screen convention (spec/grafport.h `where`, ADR-0004 OD-3): v grows
 *     DOWNWARD (top-left origin, QuickDraw global coordinates). So a PS/2
 *     "+up" delta must become a NEGATIVE screen-v delta, and vice versa.
 *
 * THE BUG (initech-rgt8): the producer call site (os/milton/kmain.c,
 *   flair_live_mouse_post) packed the raw PS/2 dy STRAIGHT into bits 16..23
 *   with no negation -- silently violating the spec/event_model.h Sec 5
 *   contract cited above. cook_raw (event.c) and test_event.c are CORRECT
 *   per spec (they assume an already-flipped input); the producer was wrong.
 *   This header is the extracted, pure (no hardware I/O), dual-compilable
 *   seam where that contract is actually enforced, so it is host-testable
 *   without linking the freestanding IRQ shell (mouse_irq_handler needs real
 *   inb()/outb() and cannot run un-privileged on a host process).
 *
 * ARTIFACT code (Law 3): freestanding-safe (stdint.h only, no I/O, no libc).
 * Dual-compile: hosted (harness/proptest/test_mouse_producer.c) AND
 * freestanding (included from os/milton/kmain.c under -m32 -ffreestanding).
 * Rule 11 (deterministic, pure function). Rule 12 (ASCII-clean).
 *
 * MUTATION PROBE (Rule 6):
 *   EVENT_MUTATE_FLIP_SIGN -- inverts the already-cooked screen-dy a SECOND
 *   time, exactly reproducing the initech-rgt8 polarity bug. Compiling
 *   harness/proptest/test_mouse_producer.c with this macro defined MUST
 *   drive the independent physical-direction oracle (initech-8f5p) RED,
 *   while leaving harness/proptest/test_event.c (which never calls this
 *   function) GREEN -- the contrast that proves the OLD oracle was blind to
 *   the sign axis (the HER-02 by-construction heresy this ticket fixes).
 */
#ifndef INITECH_MOUSE_PACK_H
#define INITECH_MOUSE_PACK_H

#include <stdint.h>

/* mouse_pack_negate_dy -- PS/2 "+up" convention -> screen "down-positive".
 *
 * Negating INT8_MIN (-128) is not representable as int8_t (-(-128) = +128
 * overflows [-128,127]); saturate to +127 (initech-rgt8 FIX note: "negate in
 * int, or saturate -128->+127"). Every other value negates exactly. */
static inline int8_t mouse_pack_negate_dy(int dy_ps2)
{
    if (dy_ps2 == -128) {
        return 127;   /* saturate; -(-128) not int8_t-representable */
    }
    return (int8_t)(-dy_ps2);
}

/* mouse_pack_raw_payload -- build the FLAIR_RAW_MOUSE payload (bits 0..23)
 * from decoded PS/2 deltas + button byte, per spec/event_model.h Sec 5.
 *
 * dx needs NO flip (PS/2 "+right" already matches screen "+right"; only the
 * Y axis is inverted between the two conventions). dy IS flipped here --
 * this is the exact fix for initech-rgt8: the ONE call site (kmain.c
 * flair_live_mouse_post) that used to pack raw dy unflipped now calls this
 * function instead. */
static inline uint32_t mouse_pack_raw_payload(int dx, int dy, uint8_t buttons)
{
    int8_t screen_dy = mouse_pack_negate_dy(dy);

#ifdef EVENT_MUTATE_FLIP_SIGN
    /* MUTANT (Rule 6): invert the cooked dy sign AGAIN -- this reproduces
     * the ORIGINAL initech-rgt8 bug bit-for-bit (double negation cancels the
     * fix above). MUST drive test_mouse_producer.c RED. screen_dy is always
     * in [-127,127] here (the -128 case saturates above), so this second
     * negation never itself overflows. */
    screen_dy = (int8_t)(-(int)screen_dy);
#endif

    return (uint32_t)(buttons & 0x07u)                       /* bits 0..7  */
         | (((uint32_t)(uint8_t)(int8_t)dx) << 8u)            /* bits 8..15 */
         | (((uint32_t)(uint8_t)screen_dy)  << 16u);          /* bits 16..23*/
}

#endif /* INITECH_MOUSE_PACK_H */

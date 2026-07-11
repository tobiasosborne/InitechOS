/* test_mouse_producer.c -- INDEPENDENT physical-direction oracle for the
 * FLAIR mouse producer (initech-8f5p; blocks initech-rgt8).
 *
 * THE HER-02 HERITAGE THIS FIXES (Law 2 / ADR-0010):
 *   Every existing mouse-Y oracle (harness/proptest/test_event.c) grades the
 *   cursor by MANUFACTURING the FLAIR_RAW_MOUSE payload directly (e.g. "dy=+5"
 *   packed straight into bits 16..23) and then asserting v == 240 + dy -- the
 *   SAME "cursor_v += dy" rule cook_raw (os/flair/event.c) uses to compute it.
 *   That is by-construction: it agrees with the artifact's own arithmetic
 *   whatever the SIGN convention happens to be, and cannot catch a producer
 *   that packs the wrong-signed dy (initech-rgt8: kmain.c's
 *   flair_live_mouse_post packed the RAW, un-flipped PS/2 dy). The only
 *   mouse mutant in that file, EVENT_MUTATE_STALE_WHERE, tests MAGNITUDE
 *   (does the cursor move at all) -- never SIGN (does it move the right way).
 *
 * THE INDEPENDENT GROUND TRUTH THIS ORACLE USES INSTEAD:
 *   1. A raw 3-byte PS/2 mouse packet, exactly as it would appear on the wire
 *      (Intel 8042 / IBM PS/2 Hardware Interface Technical Reference;
 *      os/milton/mouse.c/.h cite the same refs), encoding PHYSICAL-UP motion:
 *      byte0 = 0x08 (bit3 always-1 anchor; X/Y sign bits clear -> positive);
 *      byte1 = 0x00 (dx = 0); byte2 = 0x05 (dy = +5, PS/2 "+up" convention --
 *      os/milton/mouse.c:222 "signed PS/2 Y delta (+ = up)").
 *   2. DECODE those wire bytes exactly as the real ISR does (os/milton/
 *      mouse.c:220-222: dx=(int8_t)byte1, dy=(int8_t)byte2 -- a bare int8_t
 *      cast; NOT an "arbitrary artifact convention" but the actual, trivial,
 *      hardware-mandated two's-complement read of the wire byte. This one
 *      line is mirrored here, cited, rather than linked, because
 *      mouse_irq_handler() itself calls inb()/outb() directly (privileged
 *      8042 port I/O) and cannot run un-privileged in a host test process --
 *      "if the decode is inseparable from the IRQ shell, extract nothing"
 *      (initech-8f5p task brief). A SEPARATE known bug (docs/
 *      FLAIR-GUI-bug-hunt-2026-06-28.md #45: byte0 XS/YS sign bits never
 *      consulted) affects only |delta|>127; this oracle stays at |dy|<=5,
 *      outside that bug's blast radius, so the two defects do not entangle.
 *   3. Feed the DECODED (dx, dy, buttons) through the REAL producer packing
 *      function, mouse_pack_raw_payload() (os/milton/mouse_pack.h) -- the
 *      EXACT function os/milton/kmain.c's flair_live_mouse_post() calls at
 *      its one call site. This is the actual, shipped, host-linkable
 *      producer logic (no duplicate/parallel implementation).
 *   4. Post the resulting payload into a REAL flair_raw_ring_t via the REAL
 *      flair_raw_post() (os/flair/event.c), then drain it via the REAL
 *      GetNextEvent()/cook_raw() (same file) -- the actual consumer.
 *   5. Assert the GROUND-TRUTH physical expectation: physical UP must
 *      DECREASE screen v (spec/grafport.h / ADR-0004 OD-3: v grows downward,
 *      top-left origin -- QuickDraw global coordinates). This expectation is
 *      derived from the hardware contract + the screen-coordinate convention,
 *      NEVER from cook_raw's "+=" rule (unlike test_event.c, we do not know
 *      or care what cook_raw's internal formula is; we only assert the
 *      physically-correct DIRECTION of travel).
 *
 * MUTATION PROOF (Rule 6): EVENT_MUTATE_FLIP_SIGN (os/milton/mouse_pack.h)
 *   inverts the cooked dy a second time inside the producer, reproducing the
 *   ORIGINAL initech-rgt8 bug bit-for-bit. Compiling this file with
 *   -DEVENT_MUTATE_FLIP_SIGN MUST drive this oracle RED. Compiling
 *   test_event.c (which never includes mouse_pack.h or calls
 *   mouse_pack_raw_payload) with the SAME flag has NO EFFECT and MUST stay
 *   GREEN -- that contrast is the proof this oracle is independent and the
 *   old one was blind on the sign axis (see harness output / Makefile
 *   test-mouse-producer-mutant + test-event with the same -D, wired
 *   side-by-side for the orchestrator to diff).
 *
 * COMPILATION:
 *   Hosted (oracle run):
 *     gcc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/milton -Ispec -Iseed \
 *         harness/proptest/test_mouse_producer.c os/flair/event.c \
 *         -o /tmp/test_mouse_producer
 *     /tmp/test_mouse_producer      # must exit 0 (green)
 *
 *   Mutant (must exit NON-ZERO):
 *     gcc ... -DEVENT_MUTATE_FLIP_SIGN ... -o /tmp/test_mouse_producer_mut
 *     /tmp/test_mouse_producer_mut  # must exit NON-ZERO (red)
 *
 * Dependencies: os/flair/event.c os/flair/event.h spec/event_model.h
 *               spec/grafport.h os/milton/mouse_pack.h os/milton/mouse.c
 *               (cited, not linked) seed/test_assert.h
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Override the bare-metal panic with a hosted abort (mirrors test_event.c). */
#ifdef FLAIR_PANIC
#  undef FLAIR_PANIC
#endif
#define FLAIR_PANIC(msg) \
    do { fprintf(stderr, "PANIC: %s (at %s:%d)\n", (msg), __FILE__, __LINE__); \
         exit(1); } while(0)

#include "event.h"        /* os/flair/event.h (-Ios/flair): REAL ring + cook_raw */
#include "mouse_pack.h"    /* os/milton/mouse_pack.h (-Ios/milton): REAL producer pack */
#include "test_assert.h"  /* seed/test_assert.h: TEST_HARNESS/CHECK/TEST_SUMMARY */

TEST_HARNESS();

/* ===========================================================================
 * RAW PS/2 WIRE DECODE (mirrors os/milton/mouse.c:220-222 EXACTLY)
 * ---------------------------------------------------------------------------
 * This is NOT the artifact's arbitrary "cursor += dy" convention -- it is the
 * hardware-mandated two's-complement byte read every real PS/2 controller
 * (and mouse_irq_handler) performs. Cited, not linked, because the real
 * function reads the bytes via privileged inb() (os/milton/mouse.c) and
 * cannot run un-privileged in a host process (initech-8f5p brief: "if the
 * decode is inseparable from the IRQ shell, extract nothing").
 * ===========================================================================*/
typedef struct ps2_packet {
    uint8_t b0, b1, b2;
} ps2_packet_t;

static void ps2_decode(const ps2_packet_t *pkt, int *dx, int *dy, uint8_t *buttons)
{
    /* os/milton/mouse.c:220-222:
     *   uint8_t  b0 = g_pkt[0];
     *   int8_t   dx = (int8_t)g_pkt[1];   -- signed PS/2 X delta (+ = right)
     *   int8_t   dy = (int8_t)g_pkt[2];   -- signed PS/2 Y delta (+ = up)
     *   uint8_t  buttons = (uint8_t)(b0 & PKT_BTN_MASK);  -- PKT_BTN_MASK=0x07 */
    *dx      = (int)(int8_t)pkt->b1;
    *dy      = (int)(int8_t)pkt->b2;
    *buttons = (uint8_t)(pkt->b0 & 0x07u);
}

/* ===========================================================================
 * fresh_ring / drain1 -- same helpers as test_event.c (kept local; this file
 * is deliberately standalone so it does not depend on test_event.c's fixture
 * helpers, keeping the two oracles fully independent).
 * ===========================================================================*/
static void fresh_ring(flair_raw_ring_t *ring)
{
    flair_event_init(ring);
    flair_event_set_yield((flair_event_yield_fn)0);
}

static int drain1(flair_raw_ring_t *ring, uint16_t mask, EventRecord *out)
{
    return GetNextEvent(ring, mask, out);
}

/* ===========================================================================
 * post_producer_payload -- the FULL chain under test:
 *   raw PS/2 wire packet -> ps2_decode (hardware cast) ->
 *   mouse_pack_raw_payload (REAL producer, os/milton/mouse_pack.h) ->
 *   flair_raw_post (REAL SPSC ring producer side, os/flair/event.c).
 * ===========================================================================*/
static void post_producer_payload(flair_raw_ring_t *ring, uint32_t tick,
                                   const ps2_packet_t *pkt)
{
    int dx, dy;
    uint8_t buttons;
    flair_raw_event_t raw;
    int rc;

    ps2_decode(pkt, &dx, &dy, &buttons);

    raw.kind    = (uint32_t)FLAIR_RAW_MOUSE;
    raw.tick    = tick;
    raw.payload = mouse_pack_raw_payload(dx, dy, buttons);  /* THE REAL PRODUCER */

    rc = flair_raw_post(ring, &raw);
    CHECK(rc == 1, "post_producer_payload: flair_raw_post failed (ring full?)");
}

/* ===========================================================================
 * TEST 1 -- PHYSICAL UP -> SCREEN v DECREASES (the decisive property)
 * ---------------------------------------------------------------------------
 * Wire packet: byte0=0x08 (anchor bit3 set, sign bits clear -> X,Y positive,
 * no buttons), byte1=0x00 (dx=0), byte2=0x05 (dy=+5, PS/2 "+up").
 * Ground truth (spec/grafport.h + ADR-0004 OD-3, NOT cook_raw's rule):
 * physical UP must move the on-screen cursor UP, i.e. v must DECREASE from
 * the FLAIR_SCREEN_H/2 = 240 starting centre. |dy|=5 stays well clear of the
 * SEPARATE byte0 XS/YS-sign-bit bug (#45 in the bug-hunt doc), which only
 * bites |delta|>127.
 * ===========================================================================*/
static void test_physical_up_decreases_v(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    ps2_packet_t up_pkt = { 0x08u, 0x00u, 0x05u };  /* dy=+5 PS/2 (physical UP) */

    fresh_ring(&ring);

    post_producer_payload(&ring, 1u, &up_pkt);

    /* Pure move (no button transition) cooks to nullEvent, but the cursor
     * position (`where`) is still updated unconditionally -- same idiom as
     * test_event.c's GROUP D (where-tracking). */
    (void)drain1(&ring, everyEvent, &ev);

    CHECK(ev.where.v < (int16_t)(FLAIR_SCREEN_H / 2),
          "PHYSICAL-UP GROUND TRUTH: cursor v must DECREASE (move up) for a "
          "PS/2 +dy (physical-up) packet -- independent of cook_raw's "
          "internal += rule (initech-8f5p / initech-rgt8)");
    CHECK(ev.where.h == (int16_t)(FLAIR_SCREEN_W / 2),
          "PHYSICAL-UP: dx=0 -> horizontal cursor unchanged");
}

/* ===========================================================================
 * TEST 2 -- PHYSICAL DOWN -> SCREEN v INCREASES (the mirror-image property)
 * ---------------------------------------------------------------------------
 * Wire packet: byte0=0x28 (anchor bit3 + Y-sign bit5 set -> Y negative),
 * byte1=0x00, byte2=0xFB (=-5 two's complement, dy=-5 PS/2 = physical DOWN).
 * Ground truth: physical DOWN must move the on-screen cursor DOWN, i.e. v
 * must INCREASE.
 * ===========================================================================*/
static void test_physical_down_increases_v(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    ps2_packet_t down_pkt = { 0x28u, 0x00u, 0xFBu };  /* dy=-5 PS/2 (physical DOWN) */

    fresh_ring(&ring);

    post_producer_payload(&ring, 1u, &down_pkt);
    (void)drain1(&ring, everyEvent, &ev);

    CHECK(ev.where.v > (int16_t)(FLAIR_SCREEN_H / 2),
          "PHYSICAL-DOWN GROUND TRUTH: cursor v must INCREASE (move down) "
          "for a PS/2 -dy (physical-down) packet");
}

/* ===========================================================================
 * TEST 3 -- MAGNITUDE-EXACT: physical-up by 5 lands EXACTLY at v=235
 * ---------------------------------------------------------------------------
 * A stronger check than "just the sign": the full round trip (wire decode ->
 * real producer pack -> real ring -> real cook_raw) must move the cursor by
 * EXACTLY the physical magnitude, not just in the right direction.
 * ===========================================================================*/
static void test_physical_up_exact_magnitude(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    ps2_packet_t up_pkt = { 0x08u, 0x00u, 0x05u };

    fresh_ring(&ring);
    post_producer_payload(&ring, 1u, &up_pkt);
    (void)drain1(&ring, everyEvent, &ev);

    CHECK(ev.where.v == (int16_t)(FLAIR_SCREEN_H / 2) - 5,
          "PHYSICAL-UP: v == 240-5 == 235 (exact magnitude through the "
          "real wire-decode -> real producer -> real ring -> real cook_raw chain)");
}

/* ===========================================================================
 * TEST 4 -- REPEATED PHYSICAL-UP PACKETS MONOTONICALLY DECREASE v
 * ---------------------------------------------------------------------------
 * Three consecutive physical-up packets must move the cursor up three times
 * in a row (never flat, never reversing) -- catches a sign bug that only
 * manifests after the first packet (e.g. an accidental double-negation on
 * alternate calls) as well as a a flat no-op bug.
 * ===========================================================================*/
static void test_repeated_physical_up_monotonic(void)
{
    flair_raw_ring_t ring;
    EventRecord ev;
    ps2_packet_t up_pkt = { 0x08u, 0x00u, 0x0Au };  /* dy=+10 PS/2 (physical UP) */
    int16_t prev_v;
    int i;

    fresh_ring(&ring);
    prev_v = (int16_t)(FLAIR_SCREEN_H / 2);

    for (i = 0; i < 3; i++) {
        post_producer_payload(&ring, (uint32_t)(10 + i), &up_pkt);
        (void)drain1(&ring, everyEvent, &ev);
        CHECK(ev.where.v < prev_v,
              "REPEATED PHYSICAL-UP: each packet must strictly decrease v "
              "(monotonic upward travel)");
        prev_v = ev.where.v;
    }
}

/* ===========================================================================
 * main
 * ===========================================================================*/
int main(void)
{
    test_physical_up_decreases_v();
    test_physical_down_increases_v();
    test_physical_up_exact_magnitude();
    test_repeated_physical_up_monotonic();

    return TEST_SUMMARY("test_mouse_producer");
}

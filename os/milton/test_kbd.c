/* test_kbd.c -- host unit oracle for the PURE keyboard/PIT logic (beads
 * initech-3rs). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * Ref: IBM PC/XT scancode SET 1 table (the make codes asserted below); Intel
 *      8254 PIT datasheet (input 1.193182 MHz; divisor = round(input/hz);
 *      divisor 0 == 65536). CLAUDE.md Law 2 (oracle is truth), Rule 1
 *      (RED->GREEN), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiled HOSTED against the REAL artifact kbd.c + pit.c (the same
 * kbd_ring / kbd_translate / pit_divisor_for_hz the kernel uses). The
 * I/O-touching
 * halves (kbd_irq_handler/pit_irq_handler/kbd_init/pit_init) are NOT exercised
 * here -- the end-to-end emulator test (make test-kbd) proves those.
 *
 * Three property classes:
 *   1. RING: init/empty, enqueue+dequeue order (FIFO), full rejects, wrap.
 *   2. TRANSLATE: scancode set 1 -> ASCII incl. a SHIFTed key + Shift make/break
 *      + CapsLock; modifiers and break codes produce no char.
 *   3. PIT: divisor math at 100 Hz and the 18.2 Hz (divisor-0) sentinel.
 *
 * MUTATION (Rule 6): kbd.c built with -DKBD_MUTATE_RING_OFFBYONE (full-ring
 * never detected), -DKBD_MUTATE_SCANCODE_TABLE (mis-indexed table), or
 * -DKBD_MUT_ONE_SHIFT (release either Shift clears both), or
 * -DKBD_MUT_CAPS_REPEAT (repeated makes toggle Caps) MUST fail;
 * `make test-kbd-unit-mutant` runs those builds and asserts the oracle bites.
 */

#include <stdint.h>
#include <string.h>

#include "kbd.h"
#include "pit.h"
#include "test_assert.h"

TEST_HARNESS();
#include "../../harness/proptest/expect_abort.h"

/* Drain a ring into a C string for easy comparison. */
static void drain(kbd_ring_t *r, char *out, size_t cap)
{
    size_t n = 0;
    uint8_t c;
    while (n + 1 < cap && kbd_ring_get(r, &c)) {
        out[n++] = (char)c;
    }
    out[n] = '\0';
}

static void test_ring(void)
{
    kbd_ring_t r;
    uint8_t c;

    kbd_ring_init(&r);
    CHECK(kbd_ring_empty(&r), "fresh ring is empty");
    CHECK(kbd_ring_get(&r, &c) == 0, "get on empty ring returns 0");

    /* FIFO order. */
    CHECK(kbd_ring_put(&r, 'd') == 1, "put d ok");
    CHECK(kbd_ring_put(&r, 'i') == 1, "put i ok");
    CHECK(kbd_ring_put(&r, 'r') == 1, "put r ok");
    CHECK(!kbd_ring_empty(&r), "ring non-empty after puts");
    char got[8];
    drain(&r, got, sizeof(got));
    CHECK_STR_EQ(got, "dir", "ring dequeues in FIFO order");
    CHECK(kbd_ring_empty(&r), "ring empty after full drain");

    /* FULL rejects: capacity holds KBD_RING_CAP-1 live bytes (one slot is the
     * head/tail sentinel). The (CAP-1)th put succeeds, the CAPth is rejected. */
    kbd_ring_init(&r);
    unsigned ok = 0;
    for (unsigned i = 0; i < KBD_RING_CAP + 4u; i++) {
        if (kbd_ring_put(&r, (uint8_t)('A' + (i % 26u)))) {
            ok++;
        }
    }
    CHECK(ok == KBD_RING_CAP - 1u, "full ring accepts exactly CAP-1 bytes");

    /* WRAP: after filling+draining, indices wrap and the ring still works. */
    kbd_ring_init(&r);
    for (int rounds = 0; rounds < 4; rounds++) {
        for (int i = 0; i < (int)(KBD_RING_CAP - 1u); i++) {
            CHECK(kbd_ring_put(&r, (uint8_t)('0' + (i % 10))) == 1, "wrap put");
        }
        int n = 0;
        while (kbd_ring_get(&r, &c)) {
            CHECK(c == (uint8_t)('0' + (n % 10)), "wrap dequeue value correct");
            n++;
        }
        CHECK(n == (int)(KBD_RING_CAP - 1u), "wrap dequeued all bytes");
    }
}

static void test_translate(void)
{
    kbd_state_t st;
    memset(&st, 0xFF, sizeof(st)); /* init must clear every held-key bit */
    kbd_state_init(&st);

    /* Plain letters: scancode set 1 makes for d=0x20, i=0x17, r=0x13. */
    CHECK(kbd_translate(&st, 0x20) == 'd', "0x20 -> 'd'");
    CHECK(kbd_translate(&st, 0x17) == 'i', "0x17 -> 'i'");
    CHECK(kbd_translate(&st, 0x13) == 'r', "0x13 -> 'r'");

    /* Enter (0x1C) -> CR (0x0D), the DOS/BIOS convention so the whole CON path
     * uses one terminator and AH=0Ah needs no LF->CR bandaid (initech-62m).
     * Space (0x39). */
    CHECK(kbd_translate(&st, 0x1C) == '\r', "0x1C -> Return (CR 0x0D)");
    CHECK(kbd_translate(&st, 0x39) == ' ',  "0x39 -> Space");

    /* Break code: bit 0x80 set -> no character (release of 'd'). */
    CHECK(kbd_translate(&st, 0x20 | 0x80) == 0, "break code -> no char");

    /* A SHIFTED key (Rule 6 / the spec asks for a shifted sample): hold LShift
     * (make 0x2A -> no char), then '1' (0x02) must shift to '!'. */
    CHECK(kbd_translate(&st, 0x2A) == 0, "LShift make -> no char");
    CHECK(kbd_translate(&st, 0x02) == '!', "Shift+'1' -> '!'");
    /* A shifted LETTER: Shift+'d' -> 'D'. */
    CHECK(kbd_translate(&st, 0x20) == 'D', "Shift+'d' -> 'D'");
    /* Release Shift (break 0xAA) -> letters lowercase again. */
    CHECK(kbd_translate(&st, 0xAA) == 0, "LShift break -> no char");
    CHECK(kbd_translate(&st, 0x20) == 'd', "after Shift release -> 'd'");

    /* CapsLock latch (0x3A make): letters upper-case, digits unaffected. */
    CHECK(kbd_translate(&st, 0x3A) == 0, "CapsLock make -> no char");
    CHECK(kbd_translate(&st, 0x20) == 'D', "Caps -> 'd' upper-cases to 'D'");
    CHECK(kbd_translate(&st, 0x02) == '1', "Caps does NOT affect digit '1'");
    /* Caps + Shift on a letter -> back to lowercase. */
    CHECK(kbd_translate(&st, 0x2A) == 0, "LShift make under caps");
    CHECK(kbd_translate(&st, 0x20) == 'd', "Caps+Shift letter -> lowercase 'd'");
    CHECK(kbd_translate(&st, 0xAA) == 0, "LShift break under caps");
    /* Release before pressing CapsLock again (a new physical press). */
    CHECK(kbd_translate(&st, 0xBA) == 0, "CapsLock break -> no char");
    CHECK(kbd_translate(&st, 0x3A) == 0, "CapsLock make (toggle off)");
    CHECK(kbd_translate(&st, 0x20) == 'd', "caps off -> 'd'");

    /* An unmapped make (e.g. 0x01 = Esc maps to ESC; 0x00 -> no char). */
    CHECK(kbd_translate(&st, 0x00) == 0, "scancode 0x00 -> no char");
}

/* Ref: IBM Personal System/2 Hardware Interface Technical Reference,
 * "Keyboards (101- and 102-Key)", pp. 19, 27-30, 45-48:
 * https://www.ardent-tool.com/docs/pdf/ibm_hitrc11.pdf
 * Set 1: LShift 2A/AA, RShift 36/B6, Caps 3A/BA; held keys repeat makes.
 * Figs. 8-10 give the E0 artificial shifts for grey keys and PrintScreen.
 * Physical-key tracking mirrors os/flair/event.c (initech-tdnl.58).
 * Expected ASCII is literal and independent of the artifact's tables/state. */
typedef struct {
    uint8_t scan;
    uint8_t ascii;
} kbd_step_t;

static void check_sequence(const kbd_step_t *steps, size_t count,
                           const char *message)
{
    kbd_state_t st;
    kbd_state_init(&st);
    for (size_t i = 0; i < count; i++) {
        uint8_t got = kbd_translate(&st, steps[i].scan);
        CHECK(got == steps[i].ascii, message);
        if (got != steps[i].ascii)
            fprintf(stderr, "    step %zu scan=%02X got=%02X want=%02X\n",
                    i, steps[i].scan, got, steps[i].ascii);
    }
}

static void test_modifier_sequences(void)
{
    static const kbd_step_t left_released[] = {
        {0x2A,0}, {0x36,0}, {0x20,'D'}, {0xA0,0},
        {0xAA,0}, {0x20,'D'}, {0xA0,0}, {0x02,'!'}, {0x82,0},
        {0x36,0}, {0x20,'D'}, {0xA0,0}, /* repeated right make */
        {0xB6,0}, {0x20,'d'}, {0xA0,0}, {0x02,'1'}, {0x82,0}
    };
    static const kbd_step_t right_released[] = {
        {0x36,0}, {0x2A,0}, {0x20,'D'}, {0xA0,0},
        {0xB6,0}, {0x20,'D'}, {0xA0,0}, {0x02,'!'}, {0x82,0},
        {0x2A,0}, {0x20,'D'}, {0xA0,0}, /* repeated left make */
        {0xAA,0}, {0x20,'d'}, {0xA0,0}, {0x02,'1'}, {0x82,0}
    };
    static const kbd_step_t print_screen[] = {
        {0xE0,0}, {0x2A,0}, {0xE0,0}, {0x37,0},
        {0x20,'d'}, {0xA0,0},
        {0xE0,0}, {0xB7,0}, {0xE0,0}, {0xAA,0},
        {0x20,'d'}, {0xA0,0}
    };
    static const kbd_step_t left_grey_up[] = {
        {0x2A,0}, {0xE0,0}, {0xAA,0}, {0xE0,0}, {0x48,0},
        {0x20,'D'}, {0xA0,0}, /* fake break must not clear real Shift */
        {0xE0,0}, {0xC8,0}, {0xE0,0}, {0x2A,0},
        {0x20,'D'}, {0xA0,0}, {0xAA,0}, {0x20,'d'}, {0xA0,0}
    };
    static const kbd_step_t right_grey_up[] = {
        {0x36,0}, {0xE0,0}, {0xB6,0}, {0xE0,0}, {0x48,0},
        {0x20,'D'}, {0xA0,0},
        {0xE0,0}, {0xC8,0}, {0xE0,0}, {0x36,0},
        {0x20,'D'}, {0xA0,0}, {0xB6,0}, {0x20,'d'}, {0xA0,0}
    };
    static const kbd_step_t caps_repeat[] = {
        {0x3A,0}, {0x20,'D'}, {0xA0,0},
        {0x3A,0}, {0x20,'D'}, {0xA0,0}, {0x02,'1'}, {0x82,0},
        {0x3A,0}, {0x20,'D'}, {0xA0,0},
        {0xBA,0}, {0x20,'D'}, {0xA0,0}, /* release preserves lock */
        {0xBA,0}, {0x20,'D'}, {0xA0,0}, /* repeated break is harmless */
        {0x2A,0}, {0x20,'d'}, {0xA0,0}, {0xAA,0},
        {0x3A,0}, {0x20,'d'}, {0xA0,0}, /* next press toggles off */
        {0x3A,0}, {0x20,'d'}, {0xA0,0}, /* repeat cannot toggle on */
        {0xBA,0}, {0x3A,0}, {0x20,'D'}, {0xA0,0}
    };
#define CHECK_SEQUENCE(trace, message) \
    check_sequence(trace, sizeof(trace) / sizeof((trace)[0]), message)
    CHECK_SEQUENCE(left_released, "right Shift survives left release");
    CHECK_SEQUENCE(right_released, "left Shift survives right release");
    CHECK_SEQUENCE(print_screen, "PrintScreen fake Shifts leave case alone");
    CHECK_SEQUENCE(left_grey_up, "grey Up preserves held left Shift");
    CHECK_SEQUENCE(right_grey_up, "grey Up preserves held right Shift");
    CHECK_SEQUENCE(caps_repeat, "Caps toggles once per make-after-break");
#undef CHECK_SEQUENCE
}

static void test_pit(void)
{
    /* 100 Hz: round(1193182 / 100) = 11932 (99.9985 Hz). */
    CHECK(pit_divisor_for_hz(100u) == 11932u, "100 Hz -> divisor 11932");
    /* The classic 18.2 Hz ROM-BIOS rate uses the full 16-bit count: target 0
     * (and any target needing >65535) maps to the divisor-0 == 65536 sentinel.
     * round(1193182/18) = 66288 which is > 65535 -> the divisor-0 sentinel. */
    CHECK(pit_divisor_for_hz(0u) == 0u, "target 0 -> divisor 0 (==65536)");
    CHECK(pit_divisor_for_hz(18u) == 0u, "18 Hz needs >65535 -> divisor 0 sentinel");
    /* A frequency whose divisor fits: 1000 Hz -> round(1193.182) = 1193. */
    CHECK(pit_divisor_for_hz(1000u) == 1193u, "1000 Hz -> divisor 1193");
    /* The lowest representable: target == input -> divisor 1. */
    CHECK(pit_divisor_for_hz(PIT_INPUT_HZ) == 1u, "target==input -> divisor 1");
}

/* K12: IBM Set 1 Ctrl=1D, right Ctrl=E0 1D. DOS control bytes:
 * MS-DOS_3.3_Users_Guide_198707.pdf pp. 132, 158-159 (PDF 217, 243-244). */
static void test_control(void)
{
    kbd_state_t st;
    static const uint8_t letters[26] = {
        0x1e,0x30,0x2e,0x20,0x12,0x21,0x22,0x23,0x17,0x24,0x25,0x26,0x32,
        0x31,0x18,0x19,0x10,0x13,0x1f,0x14,0x16,0x2f,0x11,0x2d,0x15,0x2c
    };
    kbd_state_init(&st);
    CHECK(kbd_translate(&st, 0x1d) == 0, "left Ctrl make emits nothing");
    for (unsigned i = 0; i < 26; i++)
        CHECK(kbd_translate(&st, letters[i]) == i + 1, "Ctrl+A..Z -> 01..1A");
    kbd_translate(&st, 0x3a);
    kbd_translate(&st, 0x2a);
    CHECK(kbd_translate(&st, 0x2e) == 3, "Ctrl+Shift+Caps+C -> 03");
    kbd_translate(&st, 0xaa);
    kbd_translate(&st, 0xba);
    kbd_translate(&st, 0x3a);
    CHECK(kbd_translate(&st, 0xe0) == 0, "E0 prefix emits nothing");
    CHECK(kbd_translate(&st, 0x1d) == 0, "right Ctrl make emits nothing");
    kbd_translate(&st, 0x9d); /* release LEFT while RIGHT remains down */
    CHECK(kbd_translate(&st, 0x2c) == 0x1a, "right Ctrl survives left release");
    kbd_translate(&st, 0xe0);
    kbd_translate(&st, 0x9d);
    CHECK(kbd_translate(&st, 0x2e) == 'c', "right Ctrl release ends chord");
    kbd_translate(&st, 0x1d);
    kbd_translate(&st, 0xe0);
    kbd_translate(&st, 0x1d);
    kbd_translate(&st, 0xe0);
    kbd_translate(&st, 0x9d);
    CHECK(kbd_translate(&st, 0x2e) == 3, "left Ctrl survives right release");
    kbd_translate(&st, 0x9d);
    /* PrintScreen's E0 fake shift must not latch Shift. Unmapped extended
     * keys must not alias a letter or a main-block modifier. */
    kbd_translate(&st, 0xe0);
    CHECK(kbd_translate(&st, 0x2a) == 0, "E0 fake Shift ignored");
    CHECK(kbd_translate(&st, 0x20) == 'd', "E0 fake Shift leaves case alone");
    kbd_translate(&st, 0xe0);
    CHECK(kbd_translate(&st, 0x2e) == 0, "extended byte does not alias C");
    kbd_translate(&st, 0xe0);
    CHECK(kbd_translate(&st, 0x1c) == '\r', "keypad Enter -> CR");
    kbd_translate(&st, 0xe0);
    CHECK(kbd_translate(&st, 0x35) == '/', "keypad slash -> slash");
    kbd_translate(&st, 0xe0);
    kbd_state_init(&st);
    CHECK(kbd_translate(&st, 0x2e) == 'c', "init clears pending E0 and Ctrl");
}

int main(void)
{
    {
        kbd_ring_t ring; uint8_t out;
        kbd_ring_init(&ring);
        for (unsigned i = 0; i < KBD_RING_CAP + 3; i++) (void)kbd_ring_put(&ring, 'x');
        EXPECT_MARKER({ (void)kbd_ring_get(&ring, &out); (void)kbd_ring_get(&ring, &out); },
                      "KBD-RING-DROP\n", "owpg keyboard drop reported exactly once in task context");
    }
    test_ring();
    test_translate();
    test_modifier_sequences();
    test_control();
    test_pit();
    return TEST_SUMMARY("test-kbd-unit");
}

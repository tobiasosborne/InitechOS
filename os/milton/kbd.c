/* kbd.c -- PS/2 keyboard driver: ring buffer + scancode set 1 -> US ASCII +
 *          IRQ1 handler. See kbd.h for the full citation set + the reentrancy
 *          invariant.
 *
 * beads: initech-3rs.
 * Ref:   8042 PS/2 controller (read scancode at port 0x60 on IRQ1); IBM PC/XT
 *        scancode SET 1 table (make 0x01..0x58; break = make|0x80; LShift make
 *        0x2A / break 0xAA; RShift make 0x36 / break 0xB6; CapsLock make 0x3A);
 *        Intel 8259A datasheet EOI (OCW2 = 0x20 to master command port 0x20).
 *        CLAUDE.md Law 1 (cite), Rule 2 (fail safe), Rule 11 (deterministic),
 *        Rule 12 (ASCII).
 *
 * The pure halves (kbd_ring_* / kbd_translate) compile HOSTED in test_kbd.c;
 * kbd_init / kbd_irq_handler / kbd_getchar / kbd_haschar are kernel-only.
 */

#include "kbd.h"
#include "../flair/ring_report.h"
#include "../../spec/tsr_contract.h"   /* TSR_KBD_FLAGS_LINEAR (bead initech-6fdt) */
#include "io.h"

/* 8042 keyboard data port (the controller output buffer). Ref: 8042 datasheet. */
#define KBD_DATA_PORT  0x60u

/* 8259A master command port + non-specific EOI (OCW2). Ref: 8259A datasheet. */
#define PIC1_CMD       0x20u
#define PIC_EOI        0x20u

/* Scancode SET 1 control bytes (IBM PC/XT scancode set 1). */
#define SC_LSHIFT_MAKE  0x2Au
#define SC_RSHIFT_MAKE  0x36u
#define SC_LSHIFT_BREAK 0xAAu
#define SC_RSHIFT_BREAK 0xB6u
#define SC_CAPS_MAKE    0x3Au
#define SC_CAPS_BREAK   0xBAu
#define SC_BREAK_FLAG   0x80u   /* bit set in a release (break) scancode      */

/* ---- pure ring buffer ---------------------------------------------------- */

void kbd_ring_init(kbd_ring_t *r)
{
    r->head = 0u;
    r->tail = 0u;
    r->dropped = 0;
}

int kbd_ring_empty(const kbd_ring_t *r)
{
    return r->head == r->tail;
}

int kbd_ring_put(kbd_ring_t *r, uint8_t c)
{
#ifdef KBD_MUTATE_RING_OFFBYONE
    /* MUTANT (CLAUDE.md Rule 6; defined only by `make test-kbd-unit-mutant`):
     * compute the wrapped next index WITHOUT the +1, so a full ring is never
     * detected and the wrap overwrites unread data -- the ring oracle must go
     * RED. NEVER define this in a real build. */
    uint16_t next = (uint16_t)(r->head & (KBD_RING_CAP - 1u));
#else
    uint16_t next = (uint16_t)((r->head + 1u) & (KBD_RING_CAP - 1u));
#endif
    if (next == r->tail) {
        if (r->dropped == 0) r->dropped = 1; /* ISR records, task reports. */
        /* Full: drop the byte (fail safe -- a dropped keystroke is not fatal,
         * Rule 2 fail-loud applies to invariant violations, not lost input). */
        return 0;
    }
    r->buf[r->head] = c;
    r->head = next;
    return 1;
}

int kbd_ring_get(kbd_ring_t *r, uint8_t *out)
{
#ifndef KBD_MUT_SILENT_DROP
    if (r->dropped == 1) {
        r->dropped = 2;
        RING_REPORT("KBD-RING-DROP\n");
    }
#endif
    if (r->head == r->tail) {
        return 0; /* empty */
    }
    *out = r->buf[r->tail];
    r->tail = (uint16_t)((r->tail + 1u) & (KBD_RING_CAP - 1u));
    return 1;
}

/* ---- pure scancode set 1 -> US ASCII translation ------------------------- */

/* Unshifted ASCII for make codes 0x00..0x39 (US layout). 0 == no character
 * (modifier / function / unmapped). Index is the make scancode. Ref: IBM PC/XT
 * scancode set 1; only the printable rows + Enter/Space/Tab/Backspace map.
 * Enter (0x1C) decodes to CR (0x0D, '\r') -- the DOS/BIOS INT 16h convention,
 * so the whole CON path uses ONE line terminator and AH=0Ah needs no LF->CR
 * bandaid (initech-62m). */
static const char SC1_NORMAL[0x3A] = {
    /* 0x00 */ 0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',
    /* 0x08 */ '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    /* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    /* 0x18 */ 'o',  'p',  '[',  ']',  '\r', 0,    'a',  's',
    /* 0x20 */ 'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
    /* 0x28 */ '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',
    /* 0x30 */ 'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',
    /* 0x38 */ 0,    ' '
};

/* Shifted ASCII for the same make codes (US layout): the symbol row and
 * shifted punctuation. Letters are handled by upper-casing SC1_NORMAL so this
 * table need only differ for the non-letter keys; for letters we leave the
 * lowercase value and the caller upper-cases. Ref: IBM PC/XT set 1, US shift. */
static const char SC1_SHIFT[0x3A] = {
    /* 0x00 */ 0,    0x1B, '!',  '@',  '#',  '$',  '%',  '^',
    /* 0x08 */ '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
    /* 0x10 */ 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
    /* 0x18 */ 'O',  'P',  '{',  '}',  '\r', 0,    'A',  'S',
    /* 0x20 */ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    /* 0x28 */ '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    /* 0x30 */ 'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
    /* 0x38 */ 0,    ' '
};

/* Pure Set-1 decoder; the ring and IRQ publication retain native -O0 bytes.
 * Ref: kbd.h Set-1 contract; test-kernel-native-boundaries. */
#ifdef KERNEL_PURE_DECODE_OZ
#pragma GCC push_options
#pragma GCC optimize ("Oz", "no-strict-aliasing", "omit-frame-pointer")
#endif
void kbd_state_init(kbd_state_t *st)
{
    st->shift = 0u;
    st->caps  = 0u;
    st->caps_down = 0u;
    st->ctrl  = 0u;
    st->e0    = 0u;
    st->alt   = 0u;
}

uint8_t kbd_bios_flags(const kbd_state_t *st)
{
    uint8_t f = 0u;
    if (st->shift & 2u) f |= TSR_KBD_FLAG_RSHIFT;
    if (st->shift & 1u) f |= TSR_KBD_FLAG_LSHIFT;
    if (st->ctrl)       f |= TSR_KBD_FLAG_CTRL;
    if (st->alt)        f |= TSR_KBD_FLAG_ALT;
    if (st->caps)       f |= TSR_KBD_FLAG_CAPS;
    return f;
}

uint8_t kbd_translate(kbd_state_t *st, uint8_t scancode)
{
    /* IBM translated Set 1: right Ctrl is E0 1D/E0 9D, left Ctrl 1D/9D.
     * Consume E0 BEFORE the break flag (E0 itself has bit 7 set). */
    if (scancode == 0xE0u) {
#ifdef KBD_MUTATE_E0
        st->e0 = 0u; /* Rule 6: right Ctrl aliases left; fake Shift leaks. */
#else
        st->e0 = 1u;
#endif
        return 0u;
    }
    uint8_t extended = st->e0;
    st->e0 = 0u;
    if ((scancode & 0x7Fu) == 0x1Du) {
        uint8_t side = extended ? 2u : 1u;
        if (scancode & SC_BREAK_FLAG) {
#ifdef KBD_MUTATE_CTRL_RELEASE_ALL
            st->ctrl = 0u; /* Rule 6: releasing either Ctrl releases both. */
#else
            st->ctrl &= (uint8_t)~side;
#endif
        }
        else st->ctrl |= side;
        return 0u;
    }
    /* Alt: left 38/B8, right E0 38/E0 B8 (the same Set-1 pages as Ctrl). It
     * produces no character; it is tracked for the BIOS flag byte a resident
     * polls (SideKick's Ctrl+Alt chord; bead initech-6fdt). */
    if ((scancode & 0x7Fu) == 0x38u) {
        uint8_t side = extended ? 2u : 1u;
#ifdef KBD_MUT_NO_ALT
        side = 0u;   /* Rule 6: Alt never reaches the flag byte. */
#endif
        if (scancode & SC_BREAK_FLAG) st->alt &= (uint8_t)~side;
        else st->alt |= side;
        return 0u;
    }
    if (extended) {
        /* Extended keypad Enter/slash are characters; navigation, right Alt,
         * PrintScreen (including its fake Shifts) are not ASCII aliases. */
        if (scancode == 0x1Cu) return '\r';
        if (scancode == 0x35u) return '/';
        return 0u;
    }
    /* Ref: IBM PS/2 Hardware Interface Technical Reference, "Keyboards
     * (101- and 102-Key)", pp. 19, 27-30, 45-48. Track physical keys as in
     * os/flair/event.c (initech-tdnl.58): Shift = left OR right; typematic
     * makes are not new Caps presses. E0 fake Shifts were consumed above. */
    if (scancode == SC_LSHIFT_MAKE || scancode == SC_RSHIFT_MAKE) {
        st->shift |= scancode == SC_LSHIFT_MAKE ? 1u : 2u;
        return 0u;
    }
    if (scancode == SC_LSHIFT_BREAK || scancode == SC_RSHIFT_BREAK) {
#ifdef KBD_MUT_ONE_SHIFT
        /* Rule 6: restore the single-flag release; either break clears both. */
        st->shift = 0u;
#else
        uint8_t side = scancode == SC_LSHIFT_BREAK ? 1u : 2u;
        st->shift &= (uint8_t)~side;
#endif
        return 0u;
    }
    if (scancode == SC_CAPS_MAKE) {
#ifndef KBD_MUT_CAPS_REPEAT
        if (!st->caps_down)
#endif
            st->caps ^= 1u;
        st->caps_down = 1u;
        return 0u;
    }
    if (scancode == SC_CAPS_BREAK) {
        st->caps_down = 0u;
        return 0u;
    }

    /* Any other break code (bit 0x80) yields no character. */
    if (scancode & SC_BREAK_FLAG) {
        return 0u;
    }

    /* Make code outside the mapped block -> no character. */
    if (scancode >= 0x3Au) {
        return 0u;
    }

#ifdef KBD_MUTATE_SCANCODE_TABLE
    /* MUTANT (CLAUDE.md Rule 6; defined only by `make test-kbd-unit-mutant`):
     * swap the low bit of the index so e.g. 0x20 ('d') decodes as 0x21 ('f') --
     * stays in [0,0x3A) (scancode < 0x3A here) so no OOB, but the values are
     * wrong; the translation oracle must go RED. NEVER define in a real build. */
    char base = st->shift ? SC1_SHIFT[scancode ^ 1u] : SC1_NORMAL[scancode ^ 1u];
#else
    char base = st->shift ? SC1_SHIFT[scancode] : SC1_NORMAL[scancode];
#endif
    if (base == 0) {
        return 0u;
    }

#ifndef KBD_MUTATE_NO_CTRL
    /* DOS control letters are case-independent: Ctrl-C=03h, Ctrl-Z=1Ah.
     * Ref: MS-DOS_3.3_Users_Guide_198707.pdf pp. 132, 158-159.
     * Use the cooked byte only; the FLAIR hook still receives raw scancodes. */
    if (st->ctrl) {
        if (base >= 'a' && base <= 'z') return (uint8_t)(base - 'a' + 1);
        if (base >= 'A' && base <= 'Z') return (uint8_t)(base - 'A' + 1);
    }
#endif

    /* CapsLock affects LETTERS only. With Shift OFF, caps upper-cases a-z; with
     * Shift ON over caps, a letter goes back to lowercase (the usual DOS/Mac
     * behavior). Non-letters are unaffected by caps. */
    if (st->caps) {
        if (base >= 'a' && base <= 'z') {
            base = (char)(base - 'a' + 'A');   /* caps, no shift: lower -> UPPER */
        } else if (base >= 'A' && base <= 'Z') {
            base = (char)(base - 'A' + 'a');   /* caps + shift: UPPER -> lower   */
        }
    }
    return (uint8_t)base;
}

#ifdef KERNEL_PURE_DECODE_OZ
#pragma GCC pop_options
#endif

/* Ref: IBM DOS 3.30 Technical Reference pp.6-11--6-12: CON returns
 * extended keys as 00,scan. Shared by IRQ1 and the host ring oracle. */
void kbd_scancode_put(kbd_ring_t *ring, kbd_state_t *state, uint8_t sc)
{
    int special = !(sc & SC_BREAK_FLAG) &&
        ((!state->e0 && sc >= 0x3Bu && sc <= 0x3Fu) ||
         (sc == 0x52u || sc == 0x53u || sc == 0x4Bu));
    uint8_t ascii = kbd_translate(state, sc);
    if (special) {
#ifdef KBD_MUT_DROP_EXTENDED
        return; /* Rule 6: restore the pre-template lost-function-key path. */
#endif
        uint16_t next = (uint16_t)((ring->head + 1u) & (KBD_RING_CAP - 1u));
        uint16_t end = (uint16_t)((next + 1u) & (KBD_RING_CAP - 1u));
        if (next != ring->tail && end != ring->tail) {
            ring->buf[ring->head] = 0;
            ring->buf[next] = sc;
            ring->head = end;
        } else if (ring->dropped == 0) {
            ring->dropped = 1; /* Task context reports the lost pair once. */
        }
    } else if (ascii != 0u) {
        (void)kbd_ring_put(ring, ascii);
    }
}
void kbd_input_init(kbd_input_t *input)
{
    kbd_ring_init(&input->cooked);
    kbd_state_init(&input->state);
    input->owner = KBD_OWNER_DOS;
}

void kbd_input_claim(kbd_input_t *input, kbd_owner_t owner)
{
    if (input->owner != owner) {
        kbd_ring_init(&input->cooked);
        input->owner = owner;
    }
}

void kbd_input_put(kbd_input_t *input, uint8_t sc)
{
#ifndef KBD_MUT_DUAL_RING
    if (input->owner == KBD_OWNER_GUI) {
        (void)kbd_translate(&input->state, sc);
        return;
    }
#endif
    kbd_scancode_put(&input->cooked, &input->state, sc);
}

/* ---- kernel-only: state + IRQ1 handler + consumer ------------------------ */

/* The single keyboard ring + modifier state. File scope (BSS) so the IRQ1
 * handler and the event-loop consumer share exactly one instance and nothing
 * else (the reentrancy invariant in kbd.h). */
static kbd_input_t g_input;

void kbd_console_replay(kbd_state_t *state, uint8_t scancode)
{
    kbd_scancode_put(&g_input.cooked, state, scancode);
}

/* ISR-only observer, installed before sti and detached at exit. While DOS
 * owns input it records physical releases for GUI resumption; those raw
 * events are drained without GUI dispatch. The owner controls cooked input. */
static void (*g_scancode_hook)(uint8_t) = 0;

void kbd_set_scancode_hook(void (*fn)(uint8_t))
{
    g_scancode_hook = fn;
}

void kbd_set_input_owner(kbd_owner_t owner)
{
    unsigned long flags;
    __asm__ __volatile__("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    kbd_input_claim(&g_input, owner);
    __asm__ __volatile__("push %0; popf" : : "r"(flags) : "memory", "cc");
}

void kbd_init(void)
{
    kbd_input_init(&g_input);

    /* Drain any stale byte the BIOS/firmware left in the 8042 output buffer so
     * the first real IRQ1 is not preceded by a ghost scancode (8042 datasheet:
     * a pending byte blocks further IRQ1s until read). Bounded loop (Rule 2 --
     * never spin forever): at most a few bytes can be buffered. */
    for (int i = 0; i < 16; i++) {
        (void)inb(KBD_DATA_PORT);
    }
}

void kbd_irq_handler(void)
{
    /* Read the scancode FIRST (8042: reading port 0x60 clears the output-buffer
     * full state and re-arms IRQ1). */
    uint8_t sc = inb(KBD_DATA_PORT);

    /* EDLIN template keys (MS-DOS 3.3 User's Guide pp.164-169).
     * AH=07h reads DOS's two-byte 00,scan convention. Capture E0 state
     * before kbd_translate consumes it. Publish both bytes together so a
     * nearly-full ring cannot leave a dangling extended-key prefix.
     * IBM DOS 3.30 Technical Reference pp.6-11--6-12 (PDF pp.134-135).
     */
    kbd_input_put(&g_input, sc);
    /* Publish the BIOS keyboard flag byte BEFORE returning into a chained
     * resident hook (spec/tsr_contract.h Sec 2/4; bead initech-6fdt). */
    {
        /* 0x417 is the BIOS data area flag byte -- a real, fixed, mapped
         * address in the flat model (spec/tsr_contract.h Sec 4), not a null
         * dereference. Launder the pointer through an empty asm so GCC's
         * -Warray-bounds (whole-TU -Oz) cannot treat the low constant as an
         * object at address zero. Same store, same bytes; no flag relaxed. */
        volatile uint8_t *kbd_flags = (volatile uint8_t *)(uintptr_t)TSR_KBD_FLAGS_LINEAR;
        __asm__ __volatile__("" : "+r"(kbd_flags));
        *kbd_flags = kbd_bios_flags(&g_input.state);
    }

    /* ISR-enqueue-only raw observer (ADR-0004 D-4). GUI keys have not been
     * published to DOS; while DOS owns input the GUI drains these raw bytes
     * only for physical state synchronization before it resumes. */
    if (g_scancode_hook) {
        g_scancode_hook(sc);
    }

    /* End-of-interrupt to the master 8259A (OCW2 non-specific EOI). Ref: 8259A
     * datasheet. Without this the PIC will not deliver further IRQ1s. */
    outb(PIC1_CMD, PIC_EOI);
}

int kbd_haschar(void)
{
    int has;
    /* Brief critical section: stop IRQ1 from advancing head mid-read. The
     * cooperative model has a single consumer, so this only guards the index
     * read against a torn producer update. */
    __asm__ __volatile__("cli");
    has = !kbd_ring_empty(&g_input.cooked);
    __asm__ __volatile__("sti");
    return has;
}

int kbd_getchar(void)
{
    uint8_t c;
    int got;
    __asm__ __volatile__("cli");
    got = kbd_ring_get(&g_input.cooked, &c);
    __asm__ __volatile__("sti");
    return got ? (int)c : -1;
}

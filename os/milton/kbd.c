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
#define SC_BREAK_FLAG   0x80u   /* bit set in a release (break) scancode      */

/* ---- pure ring buffer ---------------------------------------------------- */

void kbd_ring_init(kbd_ring_t *r)
{
    r->head = 0u;
    r->tail = 0u;
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
 * scancode set 1; only the printable rows + Enter/Space/Tab/Backspace map. */
static const char SC1_NORMAL[0x3A] = {
    /* 0x00 */ 0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',
    /* 0x08 */ '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
    /* 0x10 */ 'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
    /* 0x18 */ 'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',
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
    /* 0x18 */ 'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',
    /* 0x20 */ 'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
    /* 0x28 */ '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',
    /* 0x30 */ 'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',
    /* 0x38 */ 0,    ' '
};

void kbd_state_init(kbd_state_t *st)
{
    st->shift = 0u;
    st->caps  = 0u;
}

uint8_t kbd_translate(kbd_state_t *st, uint8_t scancode)
{
    /* Modifier make/break first (these never produce a character). */
    if (scancode == SC_LSHIFT_MAKE || scancode == SC_RSHIFT_MAKE) {
        st->shift = 1u;
        return 0u;
    }
    if (scancode == SC_LSHIFT_BREAK || scancode == SC_RSHIFT_BREAK) {
        st->shift = 0u;
        return 0u;
    }
    if (scancode == SC_CAPS_MAKE) {
        st->caps ^= 1u;   /* CapsLock latches on each make (we ignore its break) */
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

/* ---- kernel-only: state + IRQ1 handler + consumer ------------------------ */

/* The single keyboard ring + modifier state. File scope (BSS) so the IRQ1
 * handler and the event-loop consumer share exactly one instance and nothing
 * else (the reentrancy invariant in kbd.h). */
static kbd_ring_t  g_kbd;
static kbd_state_t g_kbd_state;

void kbd_init(void)
{
    kbd_ring_init(&g_kbd);
    kbd_state_init(&g_kbd_state);

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

    uint8_t ascii = kbd_translate(&g_kbd_state, sc);
    if (ascii != 0u) {
        (void)kbd_ring_put(&g_kbd, ascii);   /* full -> dropped (fail safe) */
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
    has = !kbd_ring_empty(&g_kbd);
    __asm__ __volatile__("sti");
    return has;
}

int kbd_getchar(void)
{
    uint8_t c;
    int got;
    __asm__ __volatile__("cli");
    got = kbd_ring_get(&g_kbd, &c);
    __asm__ __volatile__("sti");
    return got ? (int)c : -1;
}

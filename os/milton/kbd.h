/* kbd.h -- PS/2 keyboard (IRQ1) driver: scancode set 1 -> US ASCII + ring.
 *
 * beads: initech-3rs ("PS/2 keyboard (IRQ1) + PIT (IRQ0) tick").
 * Ref:   IBM PS/2 8042 keyboard controller -- the keyboard data byte is read
 *        from the 8042 output buffer at I/O port 0x60 on IRQ1 (Intel/IBM
 *        8042 controller datasheet; "PS/2 Hardware" / OSDev "8042 PS/2
 *        Controller", "PS/2 Keyboard"). Scancode SET 1 (the default IBM PC/XT
 *        translated set the 8042 presents): a make code is 0x01..0x58 for the
 *        main block; bit 0x80 set marks a BREAK (key release); 0x2A/0x36 are
 *        LShift/RShift make, 0xAA/0xB6 their breaks; 0x3A is CapsLock make.
 *        (IBM PC/XT scancode set 1 table; OSDev "PS/2 Keyboard" Scan Code Set 1.)
 *        8259A PIC: end-of-interrupt is OCW2 = 0x20 to the master command port
 *        0x20 (Intel 8259A datasheet, EOI). CLAUDE.md Law 1 (cite source),
 *        Rule 2 (fail loud / fail safe), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * SPLIT for the host oracle (CLAUDE.md Rule 6 / Law 2): the PURE logic -- the
 * scancode->ASCII translator (kbd_state_t + kbd_translate) and the ring buffer
 * (kbd_ring_t + kbd_ring_*) -- has NO I/O and compiles HOSTED in test_kbd.c.
 * Only kbd_init() and the IRQ1 handler (kbd_irq_handler) touch ports 0x60/0x20,
 * so they are kernel-only. test-kbd-unit exercises the pure half; the end-to-end
 * test-kbd proves the whole path on the emulator.
 *
 * REENTRANCY INVARIANT (beads initech-xk2): the IRQ1 handler touches ONLY this
 * file's g_kbd ring + g_kbd_state. It NEVER calls int21_dispatch and NEVER
 * touches the INT 21h dispatcher globals (g_dta/g_find/g_sft/g_cur_psp). INT 21h
 * uses a TRAP gate (0x8F, IF stays set), so once IRQ1 is unmasked an IRQ CAN
 * interrupt an in-flight INT 21h call -- that is safe ONLY because there is zero
 * shared mutable state between them. Do NOT add a syscall call here.
 */
#ifndef INITECH_KBD_H
#define INITECH_KBD_H

#include <stdint.h>

/* Ring buffer capacity (bytes of decoded ASCII held before overflow drops the
 * oldest-unread input). Power of two so the wrap is a mask. 32 is ample for the
 * cooperative event loop, which drains every WaitNextEvent tick. */
#define KBD_RING_CAP 32u
/* The ring index wrap uses `& (KBD_RING_CAP - 1u)` (kbd.c kbd_ring_put/get),
 * which is a correct modulo ONLY when the capacity is a power of two. Enforce
 * it at compile time so a future resize that breaks the invariant fails the
 * build rather than silently corrupting the wrap at runtime (Rule 2). */
_Static_assert((KBD_RING_CAP & (KBD_RING_CAP - 1u)) == 0u,
               "KBD_RING_CAP must be a power of two for the index-mask wrap");

/* A single-producer (IRQ1) / single-consumer (event loop) byte ring. The
 * producer only advances head; the consumer only advances tail; head==tail is
 * empty and (head+1)%CAP==tail is full. uint16_t indices are wide enough that
 * the mask (CAP-1) is the only wrap arithmetic. */
typedef struct {
    uint8_t  buf[KBD_RING_CAP];
    uint16_t head;   /* next slot the producer writes (mod CAP)  */
    uint16_t tail;   /* next slot the consumer reads  (mod CAP)  */
} kbd_ring_t;

/* Reset a ring to empty. */
void kbd_ring_init(kbd_ring_t *r);

/* Enqueue one byte. Returns 1 on success, 0 if the ring was FULL (the byte is
 * dropped -- fail safe, NOT fail loud: a dropped keystroke must never panic the
 * desktop). */
int kbd_ring_put(kbd_ring_t *r, uint8_t c);

/* Dequeue one byte into *out. Returns 1 on success, 0 if the ring was EMPTY. */
int kbd_ring_get(kbd_ring_t *r, uint8_t *out);

/* Non-destructive emptiness test (1 == empty). */
int kbd_ring_empty(const kbd_ring_t *r);

/* Modifier state carried across scancodes (Shift held, CapsLock latched). Pure
 * data; the translator both reads and updates it. */
typedef struct {
    uint8_t shift;  /* 1 while either Shift make is held (cleared on break)   */
    uint8_t caps;   /* 1 while CapsLock is latched (toggled on each 0x3A make) */
} kbd_state_t;

/* Reset modifier state (no keys held, caps off). */
void kbd_state_init(kbd_state_t *st);

/* Translate ONE scancode-set-1 byte, updating *st for modifier make/break.
 * Returns the decoded ASCII byte (>0) for a printable key make, or 0 when the
 * scancode produced no character (a modifier, a break code, or an unmapped
 * key). CapsLock affects letters only (A-Z); Shift affects letters AND the
 * symbol row, per a US layout. Ref: scancode set 1 table (above). */
uint8_t kbd_translate(kbd_state_t *st, uint8_t scancode);

/* ---- kernel-only (freestanding; touch I/O ports) ------------------------- */

/* Initialize the keyboard ring + modifier state. Does NOT unmask the PIC line
 * or enable interrupts (kmain.c does that, after the boot demos). Drains any
 * stale byte sitting in the 8042 output buffer so the first real IRQ1 is clean. */
void kbd_init(void);

/* The IRQ1 (vector 0x29) handler body, called by the asm stub in isr.asm.
 * Reads the scancode from port 0x60, translates it, enqueues any ASCII into the
 * ring, then sends EOI (0x20 -> port 0x20). Kernel-only (port I/O). */
void kbd_irq_handler(void);

/* Non-blocking consumer API for the event loop / the echo self-test.
 * kbd_getchar returns the next ASCII byte, or -1 if the ring is empty.
 * kbd_haschar returns 1 if a byte is waiting. Both run with IRQs briefly
 * masked (cli/sti) around the ring access so a concurrent IRQ1 producer cannot
 * tear the index update -- belt-and-suspenders for the single-reader case. */
int kbd_getchar(void);
int kbd_haschar(void);

#endif /* INITECH_KBD_H */

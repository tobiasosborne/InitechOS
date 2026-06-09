/* irq.c -- in-IRQ depth counter + the INT 21h reentrancy fail-loud panic.
 *
 * beads: initech-xk2. See irq.h for the full citation set + the invariant.
 * Ref:   Intel SDM Vol 3A Sec 6.12.1.2 (TRAP gate leaves IF set), Sec 6.15
 *        (#BP vector 3); CLAUDE.md Law 1, Law 2, Rule 2 (fail loud), Rule 11,
 *        Rule 12. The serial-marker + register-dump panic mirrors panic.c
 *        (isr_dispatch_c) -- we reach it by RAISING a CPU exception (int3) so the
 *        SAME fail-loud path (dump to COM1 + PC LOAD LETTER + cli;hlt) fires; we
 *        do NOT duplicate the dump here.
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. ALSO
 * compiles HOSTED (test_int21.c links it): g_irq_depth is never incremented in
 * the host build (no IRQs), so irq_depth() == 0 and dos_reentry_panic is never
 * reached there -- the int3 below is unreachable host-side.
 */

#include "irq.h"
#include "io.h"

/* COM1 16550 UART (same protocol as panic.c / kmain.c serial_putc). Kept local
 * so the reentry marker reaches the wire even if the rest of the kernel is wedged
 * mid-syscall. */
#define COM1_THR 0x3F8u
#define COM1_LSR 0x3FDu
#define LSR_THRE 0x20u

volatile uint32_t g_irq_depth = 0u;

void irq_enter(void)
{
    g_irq_depth++;
}

void irq_leave(void)
{
    /* Defensive (Rule 2): never underflow below 0. A balanced enter/leave pair
     * per IRQ keeps this at the nesting depth; an unbalanced leave would be a
     * stub bug, so clamp rather than wrap to 0xFFFFFFFF (which would then read as
     * "in IRQ" forever and wedge every later syscall in the guard). */
    if (g_irq_depth > 0u) {
        g_irq_depth--;
    }
}

uint32_t irq_depth(void)
{
    return g_irq_depth;
}

int kbd_in_irq(void)
{
    return g_irq_depth != 0u ? 1 : 0;
}

static void rserial_putc(char c)
{
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
        /* spin until THR empty */
    }
    outb(COM1_THR, (uint8_t)c);
}

static void rserial_puts(const char *s)
{
    while (*s) {
        rserial_putc(*s++);
    }
}

void dos_reentry_panic(void)
{
    /* Grep-able marker FIRST (the oracle keys on this exact line), so the cause
     * is on the wire before the generic register dump. Then raise #BP (vector 3)
     * to route through the REAL panic path (isr_dispatch_c, panic.c): it prints
     * "PANIC vec=03 ...", the register dump, "PC LOAD LETTER", and cli;hlt --
     * never returning into the corrupted syscall. */
    rserial_puts("INT21-REENTRY-PANIC: int 0x21 entered from IRQ context (an ISR called DOS)\n");

    for (;;) {
        __asm__ __volatile__("int $3");   /* -> isr3 -> isr_dispatch_c -> halt */
    }
}

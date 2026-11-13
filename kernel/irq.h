/* irq.h -- in-IRQ depth counter + the INT 21h reentrancy guard hook.
 *
 * beads: initech-xk2 ("INT 21h reentrancy hardening when IRQs are unmasked").
 * Ref:   CLAUDE.md Law 1 (cite source), Law 2 (oracle is truth), Rule 2 (fail
 *        LOUD on an invariant violation), Rule 11 (deterministic), Rule 12
 *        (ASCII). Intel SDM Vol 3A Sec 6.12.1.2 (a TRAP gate, 0x8F, leaves IF
 *        unchanged, so an unmasked IRQ CAN be delivered inside a software-int
 *        handler reached through one) -- this is exactly why INT 21h, installed
 *        as a 0x8F trap gate (idt_install_trap, vector 0x21), can be interrupted
 *        by IRQ0(PIT)/IRQ1(keyboard) mid-syscall. DOS 3.3 internals: the real
 *        DOS InDOS flag (a byte the kernel increments at INT 21h entry and
 *        decrements at exit) is the period-authentic primitive a TSR/driver
 *        polls before issuing its OWN INT 21h call -- it defers while InDOS != 0
 *        because DOS INT 21h is NOT reentrant. We mirror both halves here: a
 *        kernel-maintained IN-IRQ depth (the guard that fails loud if an ISR ever
 *        calls DOS) and, in int21.c, the InDOS depth (the documented future hook
 *        a deferring TSR/driver checks via dos_in_dos()).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. This
 * TU ALSO compiles HOSTED for the int21 unit oracle (test_int21.c links irq.c):
 * there g_irq_depth is never incremented (no IRQs), so irq_depth() == 0 and the
 * reentrancy guard never fires -- the host oracle exercises the dispatcher with
 * the guard present but quiescent, exactly as a real syscall from task context.
 *
 * THE INVARIANT (kbd.h / pit.c / isr.asm xk2 comments): the IRQ handlers
 * (pit_irq_handler, kbd_irq_handler) touch ONLY their own tick/ring state and
 * NEVER call int21_dispatch. The guard mechanically ENFORCES that: if an ISR
 * ever (directly or via a future driver) issues `int 0x21`, irq_depth() != 0 at
 * dispatch entry and we fail loud rather than silently corrupt the in-flight
 * syscall's frame or shared globals (g_dta / the FINDFIRST search state /
 * g_cur_psp / the FAT cluster scratch). EXEC's SYNCHRONOUS nesting (a child
 * program's INT 21h calls inside do_exec) is NOT an IRQ reentry: those run with
 * irq_depth() == 0, so the guard does not false-fire on them.
 */
#ifndef INITECH_IRQ_H
#define INITECH_IRQ_H

#include <stdint.h>

/* The IN-IRQ depth: 0 in task context, > 0 while a hardware IRQ handler is on
 * the stack. The asm IRQ stubs (irq0_entry/irq1_entry, isr.asm) call irq_enter
 * on entry and irq_leave before iretd. volatile: the asm side writes it and the
 * INT 21h dispatcher reads it; the compiler must not cache either side. */
extern volatile uint32_t g_irq_depth;

/* Increment / decrement g_irq_depth. Called from the asm IRQ stubs around the C
 * handler call (the IRQ gates clear IF, so these are not themselves reentrant).
 * Plain counter ops -- no I/O -- so irq.c stays host-compilable. */
void irq_enter(void);
void irq_leave(void);

/* The current in-IRQ depth (0 == task context). The INT 21h dispatch entry reads
 * this; a non-zero value at dispatch entry is the forbidden case (an ISR called
 * DOS) and triggers dos_reentry_panic. */
uint32_t irq_depth(void);

/* Convenience predicate: 1 while executing inside a hardware IRQ handler. A
 * future ISR/driver MUST NOT issue INT 21h while this is true. */
int kbd_in_irq(void);

/* FAIL LOUD (Rule 2): an INT 21h dispatch was entered from IRQ context -- an
 * invariant violation that can only happen if an ISR called DOS (forbidden). The
 * kernel impl emits a grep-able serial marker ("INT21-REENTRY-PANIC") + the
 * vector dump and halts (it routes through the real panic path via a deliberate
 * exception, so the serial register dump + PC LOAD LETTER screen fire exactly as
 * a CPU fault would; panic.c). Never returns. In the host oracle this is never
 * reached (irq_depth() is always 0), so a __builtin_trap fallback is sufficient
 * there. */
void dos_reentry_panic(void);

#endif /* INITECH_IRQ_H */

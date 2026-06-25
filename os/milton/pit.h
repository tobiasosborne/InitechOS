/* pit.h -- Intel 8254 Programmable Interval Timer (channel 0, IRQ0) tick.
 *
 * beads: initech-3rs ("PS/2 keyboard (IRQ1) + PIT (IRQ0) tick").
 * Ref:   Intel 8254 PIT datasheet -- channel 0 is wired to IRQ0; the input
 *        clock is the PC standard 1.193182 MHz (PIT_INPUT_HZ). The output
 *        frequency is INPUT / divisor, where the 16-bit divisor is written
 *        LSB-then-MSB to the channel-0 data port 0x40 after a mode byte to the
 *        command port 0x43. Mode byte 0x36 = channel 0, access lo/hi byte,
 *        mode 3 (square wave), binary count. (8254 datasheet; OSDev "Programmable
 *        Interval Timer".) The classic ROM-BIOS rate uses divisor 0 (== 65536)
 *        -> 18.2065 Hz; a divisor of 11932 -> ~100.0 Hz. 8259A EOI = 0x20 to the
 *        master command port 0x20. CLAUDE.md Law 1 (cite), Rule 2 (fail loud),
 *        Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * SPLIT for the host oracle: pit_divisor_for_hz() is PURE divisor math (no I/O)
 * and compiles HOSTED in test_kbd.c; pit_init()/the IRQ0 handler touch ports.
 */
#ifndef INITECH_PIT_H
#define INITECH_PIT_H

#include <stdint.h>

/* PC PIT input clock: 1.193182 MHz (the canonical 14.31818 MHz / 12). The
 * datasheet value the divisor math is computed against. */
#define PIT_INPUT_HZ 1193182u

/* The tick rate the kernel programs (initech-3rs: "~100 Hz or the classic
 * 18.2 Hz"). We pick 100 Hz: a round 10 ms tick for the cooperative
 * WaitNextEvent loop. divisor = round(1193182 / 100) = 11932 -> 99.9985 Hz. */
#define PIT_TICK_HZ 100u

/* Compute the 16-bit reload divisor for a target frequency, PURE (host-test).
 * Rounds to nearest. A target of 0 or one that would need a divisor > 65535 is
 * clamped to divisor 0, which the 8254 treats as 65536 (the slowest rate,
 * ~18.2 Hz) -- the documented out-of-range behavior, not a silent wrong value.
 * Returns the value to write LSB-then-MSB to port 0x40. Ref: 8254 datasheet. */
uint16_t pit_divisor_for_hz(uint32_t target_hz);

/* ---- kernel-only (freestanding; touch I/O ports) ------------------------- */

/* Program channel 0 to PIT_TICK_HZ (mode 3, lo/hi) and reset the tick counter.
 * Does NOT unmask IRQ0 or enable interrupts (kmain.c does that). */
void pit_init(void);

/* The IRQ0 (vector 0x28) handler body, called by the asm stub in isr.asm:
 * increments the tick counter, sends EOI. Kernel-only (port I/O). */
void pit_irq_handler(void);

/* Optional per-tick hook (ADR-0006 E-D3a / FO-4 -- the FLAIR live event loop).
 * The IRQ0 handler invokes this AFTER incrementing g_ticks and BEFORE the PIC
 * EOI (pit.c:93), iff a non-NULL function has been installed. It DEFAULTS to
 * NULL: every kernel that does not call pit_set_tick_hook() observes a
 * pit_irq_handler that is byte-for-byte the legacy behaviour (increment + EOI),
 * so pit.o is shared across all kernels with ZERO new undefined symbol and ZERO
 * behavioural change (CLAUDE.md Rule 11 byte-stability; Rule 2 fail-loud is
 * preserved -- a NULL hook is simply not called, never dereferenced).
 *
 * The FLAIR-live kernel installs flair_tick_advance() here so the cooperative
 * WaitNextEvent time base advances from the real PIT interrupt. The hook is
 * ISR context: it must do the ADR-0004 D-4 minimum (advance a counter only --
 * no Toolbox call, no allocation, no port I/O beyond what flair_tick_advance
 * itself does, which is a single volatile increment). Ref: ADR-0006 E-D3(a),
 * FO-4; os/flair/event.c:98 (flair_tick_advance). */
void pit_set_tick_hook(void (*fn)(void));

/* Monotonic tick count since pit_init (incremented by the IRQ0 handler).
 * volatile read of the shared counter. */
uint32_t pit_ticks(void);

#endif /* INITECH_PIT_H */

/* mouse.h -- PS/2 aux mouse (IRQ12) driver: the FLAIR live-loop pointer source.
 *
 * beads: initech-5l5z FO-6 (ADR-0006 E-D3(b) / FO-6 -- the mouse IRQ12 lane).
 * Ref (Law 1):
 *   - ADR-0006 E-D3(b)/BC-3: "a NEW os/milton/mouse.c plus an irq12_entry in
 *     isr.asm ... reads the 3-byte PS/2 packet, posts ONE FLAIR_RAW_MOUSE event,
 *     and issues a DUAL-PIC EOI: slave 0xA0 THEN master 0x20." ISR-enqueue-only
 *     per ADR-0004 D-4 (read device, post raw, EOI, return; no Manager call,
 *     no allocation in interrupt context).
 *   - Intel 8042 keyboard-controller datasheet + the IBM PS/2 Hardware
 *     Interface (Technical Reference): the auxiliary-device (mouse) channel,
 *     the status register OBF/IBF bits (0x64 bit0/bit1), controller commands
 *     0xA8 (enable aux) / 0x20 (read config) / 0x60 (write config) /
 *     0xD4 (write-to-aux prefix), and the PS/2 mouse command 0xF4 (enable data
 *     reporting, ACK 0xFA). [osdev.org "PS/2 Mouse" / "8042 PS/2 Controller".]
 *   - Intel 8259A datasheet: IRQ12 is the SLAVE 8259A IR4 (cascaded on the
 *     master IRQ2); a cascaded handler issues EOI to the slave THEN the master.
 *   - spec/event_model.h Sec 5: flair_raw_event_t MOUSE payload layout
 *     (buttons in bits 0..7, signed dX in 8..15, signed dY in 16..23).
 *
 * ARTIFACT code (CLAUDE.md Law 3): freestanding kernel C, io.h (inb/outb) only,
 * no libc. Rule 11 (deterministic: every 8042 spin is bounded by a COUNT, never
 * wall-clock). Rule 12 (ASCII-clean). Cooperative, not preemptive (ADR-0004 D-6).
 */
#ifndef INITECH_MOUSE_H
#define INITECH_MOUSE_H

#include <stdint.h>

/* mouse_init -- bring up the 8042 auxiliary (PS/2 mouse) channel.
 *
 * Sequence (ADR-0006 FO-6; Intel 8042 + PS/2 mouse refs above):
 *   1. drain any stale 8042 output byte (bounded; 8042 OBF blocks fresh data);
 *   2. enable the aux device           (cmd 0xA8 -> 0x64);
 *   3. read the controller config byte (cmd 0x20 -> 0x64, byte <- 0x60);
 *   4. SET bit1 (enable IRQ12) and CLEAR bit5 (enable the aux clock);
 *   5. write the config byte back      (cmd 0x60 -> 0x64, byte -> 0x60);
 *   6. enable data reporting on the mouse: 0xD4 prefix (-> 0x64) then 0xF4
 *      (-> 0x60); read the 0xFA ACK.
 *
 * Every wait on the 8042 status register (0x64) is a BOUNDED spin (Rule 2 +
 * Rule 11 -- fail-soft, never hang): a timed-out wait is skipped, never looped
 * forever. Call with IRQ12 still masked (before pic_unmask_irq12) and BEFORE
 * sti, so the 0xF4 ACK + any startup byte are drained by polling here, not
 * delivered as a surprise IRQ12 before the handler/ring is ready. */
void mouse_init(void);

/* mouse_irq_handler -- the C body called from irq12_entry (isr.asm).
 *
 * Reads ONE byte from port 0x60 (re-arming the next IRQ12), accumulates the
 * 3-byte PS/2 packet (resyncing on byte0's always-1 bit3), and on a COMPLETE
 * packet calls the installed event hook (if non-NULL) with (dx, dy, buttons).
 * THEN it issues the DUAL-PIC EOI: SLAVE (0xA0) first, THEN master (0x20) --
 * the load-bearing minefield correctness point (ADR-0006 BC-3). Kernel-only
 * (port I/O). */
void mouse_irq_handler(void);

/* mouse_set_event_hook -- install the completed-packet callback (NULL default).
 *
 * Mirrors pit_set_tick_hook / kbd_set_scancode_hook (FO-4/FO-5). DEFAULT NULL:
 * the handler does the 8042 read + packet assembly + dual-PIC EOI but posts
 * nothing. Only the FLAIR-live kernel installs a non-NULL hook (which
 * flair_raw_posts a FLAIR_RAW_MOUSE event). The hook runs in ISR context and
 * must do the ADR-0004 D-4 minimum (post to the lock-free ring and return; no
 * Toolbox, no allocation, no port I/O). dx/dy are the signed PS/2 deltas;
 * buttons is the low 3 bits of packet byte 0 (bit0 left, bit1 right, bit2
 * middle). Written once from task context BEFORE sti; read in ISR context --
 * safe on the single-core cooperative system (no concurrent writer). */
void mouse_set_event_hook(void (*fn)(int dx, int dy, uint8_t buttons));

#endif /* INITECH_MOUSE_H */

/* pic.h -- 8259A Programmable Interrupt Controller remap + mask.
 *
 * beads: initech-a5a (interrupt foundation). PIC base choice: initech-1f9.
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 4 (ICW sequence,
 *        ports, mask-all) + Sec 5.1 / Sec 8 Risk 3 (the 0x28/0x30 remap that
 *        keeps vector 0x21 free for `int 0x21`); Intel 8259A datasheet (ICW1-4
 *        initialization protocol, external reference). CLAUDE.md Law 1, Rule 2.
 *
 * ARTIFACT code: freestanding, uses io.h (inb/outb) only.
 */
#ifndef INITECH_PIC_H
#define INITECH_PIC_H

#include <stdint.h>

/* Post-remap vector bases. master IRQ0-7 -> 0x28-0x2F, slave IRQ8-15 ->
 * 0x30-0x37. CRITICAL (beads initech-1f9): NOT the conventional 0x20/0x28 --
 * we shift up so vector 0x21 stays free for the DOS-idiomatic `int 0x21`
 * syscall gate (the NEXT task). With master_base=0x20, IRQ1 (keyboard) would
 * land on 0x21 and collide with INT 21h (ground-truth Sec 8 Risk 3). */
#define PIC_MASTER_BASE  0x28u
#define PIC_SLAVE_BASE   0x30u

/* Remap both 8259As to PIC_MASTER_BASE/PIC_SLAVE_BASE and MASK ALL 15 IRQs
 * (OCW1 = 0xFF to both data ports). The kernel keeps every hardware IRQ masked
 * this milestone; specific unmasks (PIT IRQ0, keyboard IRQ1) happen in the
 * shell phase. EOI (outb(cmd,0x20)) is documented for that later work. */
void pic_remap_and_mask(void);

/* Unmask ONLY IRQ0 (PIT) and IRQ1 (keyboard) on the master 8259A: read the
 * current master IMR (OCW1, port 0x21), clear bits 0 and 1, write it back.
 * Every other line stays at whatever pic_remap_and_mask left it (all masked),
 * and the slave PIC is untouched (still fully masked). After this the caller
 * may `sti`. Ref: Intel 8259A datasheet (OCW1 interrupt mask register; a 1 bit
 * MASKS the line). beads: initech-3rs. */
void pic_unmask_irq0_irq1(void);

/* Unmask the PS/2 mouse IRQ12 (ADR-0006 E-D3(d) / FO-6; beads initech-5l5z).
 * IRQ12 is the SLAVE 8259A's IR4, cascaded behind the master IRQ2: a slave IRQ
 * reaches the CPU ONLY when BOTH the master IMR bit 2 (cascade) AND the slave
 * IMR bit 4 (IRQ12) are clear. This clears BOTH (read-modify-write each IMR,
 * leaving the PIT/keyboard unmasks intact). Call AFTER the IRQ12 IDT gate is
 * installed and mouse_init() has brought up the 8042 aux channel. Ref: Intel
 * 8259A datasheet (OCW1 IMR; cascade mode). */
void pic_unmask_irq12(void);

#endif /* INITECH_PIC_H */

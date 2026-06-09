/* pic.c -- 8259A remap + mask-all. See pic.h for the full citation set.
 *
 * beads: initech-a5a; base choice initech-1f9.
 * Ref: docs/research/internals-int21h-ground-truth.md Sec 4.2 (ICW sequence),
 *      Sec 4.3 (mask all); Intel 8259A datasheet (ICW1-4 protocol, external).
 */

#include "pic.h"
#include "io.h"

/* 8259A port addresses (ground-truth Sec 4.2; standard ISA). */
#define PIC1_CMD   0x20u   /* master command port  */
#define PIC1_DATA  0x21u   /* master data port     */
#define PIC2_CMD   0xA0u   /* slave  command port  */
#define PIC2_DATA  0xA1u   /* slave  data port     */

/* ICW values (Intel 8259A datasheet). */
#define ICW1_INIT  0x11u   /* 0x10 = begin init, 0x01 = ICW4 present; edge-trig */
#define ICW3_MASTER 0x04u  /* slave is cascaded on master IRQ2 (bit 2)          */
#define ICW3_SLAVE  0x02u  /* slave cascade identity = 2                        */
#define ICW4_8086   0x01u  /* 8086/88 mode, normal (manual) EOI                 */

#define PIC_MASK_ALL 0xFFu /* OCW1: all 8 lines masked                          */

/* Short I/O delay between ICW writes -- a dummy outb to the unused diagnostic
 * port 0x80. Good hygiene on real-ish hardware where the 8259A needs a moment
 * to latch each ICW (ground-truth Sec 4.2). */
static inline void io_wait(void)
{
    outb(0x80u, 0u);
}

void pic_remap_and_mask(void)
{
    /* ICW1: start the init sequence on both PICs (cascade mode). */
    outb(PIC1_CMD, ICW1_INIT);  io_wait();
    outb(PIC2_CMD, ICW1_INIT);  io_wait();

    /* ICW2: vector base offsets. master->0x28, slave->0x30 (initech-1f9). */
    outb(PIC1_DATA, PIC_MASTER_BASE);  io_wait();
    outb(PIC2_DATA, PIC_SLAVE_BASE);   io_wait();

    /* ICW3: wire master<->slave cascade on IRQ2. */
    outb(PIC1_DATA, ICW3_MASTER);  io_wait();
    outb(PIC2_DATA, ICW3_SLAVE);   io_wait();

    /* ICW4: 8086/88 mode, manual EOI. */
    outb(PIC1_DATA, ICW4_8086);  io_wait();
    outb(PIC2_DATA, ICW4_8086);  io_wait();

    /* OCW1: mask ALL IRQs. The shell phase unmasks PIT (IRQ0)/keyboard (IRQ1)
     * later; EOI for those handlers will be outb(<cmd port>, 0x20). */
    outb(PIC1_DATA, PIC_MASK_ALL);
    outb(PIC2_DATA, PIC_MASK_ALL);
}

void pic_unmask_irq0_irq1(void)
{
    /* OCW1 is the Interrupt Mask Register: a 1 bit MASKS, a 0 bit ENABLES the
     * line (8259A datasheet). Read-modify-write so we clear ONLY bit 0 (IRQ0,
     * PIT) and bit 1 (IRQ1, keyboard) and leave every other master line, and
     * the entire slave, masked exactly as pic_remap_and_mask left them. */
    uint8_t mask = inb(PIC1_DATA);
    mask &= (uint8_t)~((1u << 0) | (1u << 1));  /* enable IRQ0 + IRQ1 only */
    outb(PIC1_DATA, mask);
}

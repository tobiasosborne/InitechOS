/* pit.c -- Intel 8254 PIT channel 0 tick. See pit.h for citations.
 *
 * beads: initech-3rs.
 * Ref:   Intel 8254 datasheet (channel 0 -> IRQ0; cmd port 0x43, ch0 data port
 *        0x40; mode byte 0x36 = ch0/lohi/mode3/binary; divisor written LSB then
 *        MSB; input clock 1.193182 MHz; divisor 0 == 65536); Intel 8259A EOI
 *        (OCW2 0x20 -> master cmd port 0x20). CLAUDE.md Law 1, Rule 2, Rule 11,
 *        Rule 12.
 *
 * pit_divisor_for_hz is PURE (host-tested); pit_init/pit_irq_handler/pit_ticks
 * are kernel-only.
 */

#include "pit.h"
#include "io.h"

/* 8254 ports (8254 datasheet; standard ISA). */
#define PIT_CH0_DATA  0x40u   /* channel 0 counter data port      */
#define PIT_CMD       0x43u   /* mode/command register            */

/* Mode byte: channel 0 (bits 7:6 = 00), access lo/hi (bits 5:4 = 11), mode 3
 * square wave (bits 3:1 = 011), binary (bit 0 = 0) -> 0x36. Ref: 8254 Table. */
#define PIT_MODE_CH0_LOHI_M3  0x36u

/* 8259A master command port + non-specific EOI. Ref: 8259A datasheet. */
#define PIC1_CMD  0x20u
#define PIC_EOI   0x20u

uint16_t pit_divisor_for_hz(uint32_t target_hz)
{
    if (target_hz == 0u) {
        return 0u;   /* 8254: divisor 0 == 65536 (slowest, ~18.2 Hz) */
    }
    /* Round to nearest: (input + hz/2) / hz. PIT_INPUT_HZ fits in 32 bits and
     * target_hz >= 1, so no overflow for the +hz/2 nudge. */
    uint32_t div = (PIT_INPUT_HZ + target_hz / 2u) / target_hz;
    if (div >= 65536u) {
        return 0u;   /* too large for the 16-bit counter -> 65536 sentinel */
    }
    if (div == 0u) {
        div = 1u;    /* defensive: never write a 0 divisor for a fast target */
    }
    return (uint16_t)div;
}

/* The monotonic tick counter. volatile: the IRQ0 handler writes it, the
 * cooperative loop reads it; the compiler must not cache either side. */
static volatile uint32_t g_ticks;

void pit_init(void)
{
    g_ticks = 0u;

    uint16_t div = pit_divisor_for_hz(PIT_TICK_HZ);

    /* Mode byte, then divisor LSB then MSB (lo/hi access mode). Ref: 8254. */
    outb(PIT_CMD, PIT_MODE_CH0_LOHI_M3);
    outb(PIT_CH0_DATA, (uint8_t)(div & 0xFFu));
    outb(PIT_CH0_DATA, (uint8_t)((div >> 8) & 0xFFu));
}

void pit_irq_handler(void)
{
    g_ticks++;

    /* End-of-interrupt to the master 8259A (OCW2 non-specific EOI). */
    outb(PIC1_CMD, PIC_EOI);
}

uint32_t pit_ticks(void)
{
    return g_ticks;
}

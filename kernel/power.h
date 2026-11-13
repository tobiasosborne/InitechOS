/* power.h -- the kernel RESET and HALT primitives (Special > Restart and
 *            Special > Shut Down).
 *
 * beads: initech-tdnl.78 ("Restart and Shut Down: kernel reset/halt primitive +
 *        the period shut-down screen"; the rows tdnl.37 drew disabled in
 *        4ff8cab because neither primitive existed).
 *
 * Ref:   IBM PC/AT Technical Reference, 8042 keyboard controller: the status
 *        register is read at I/O port 0x64 (bit 1 = INPUT buffer full -- the
 *        controller has not yet taken the last byte written to 0x60/0x64);
 *        controller COMMANDS are written to port 0x64; commands 0xF0..0xFF
 *        PULSE the controller's output-port bits 0..3 low, and output-port
 *        bit 0 is wired to the CPU RESET line, so 0xFE (pulse bit 0 only)
 *        resets the system -- the period AT-class reset path. NOT backed by a local
 *        copy (cited the way spec/hardware.json cites the Intel SDM): no PC/AT
 *        Technical Reference or 8042 datasheet is in the tree or the manuals
 *        corpus. os/milton/kbd.c / kbd.h already drive this controller (port
 *        0x60, the same chip) on the same citation footing.
 *        Intel SDM Vol 2 (CLI / HLT): with IF clear, HLT stops the processor
 *        until NMI or RESET -- the halt is permanent. NOT backed by a local
 *        copy either.
 *        CLAUDE.md Law 1 (cite), Rule 2 (fail loud: a reset that does not
 *        happen is a PANIC with context, never a silent hang), Rule 11, Rule 12.
 *
 * WHY A HEADER (static inline), not a new object: the only kernel caller is
 * os/milton/kmain.c's Finder power path (os/flair may not include os/milton --
 * kmain.c is the seam that services every Finder command outcome, the Empty
 * Trash pattern). Inline keeps every other kernel image byte-identical and lets
 * the Rule-6 mutant ride the existing kmain mutant rules (-D on kmain.o only).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * MUTANT KNOB (Rule 6; -D on kmain.o only, test-flair-restart-mutant):
 *   POWER_MUT_NO_PULSE -- the reset command byte is 0xFF ("pulse NO output-
 *                         port bit") instead of 0xFE: the controller accepts a
 *                         command, nothing resets, the bounded spin expires and
 *                         the caller's PANIC fires. NEVER in a real build.
 */
#ifndef INITECH_POWER_H
#define INITECH_POWER_H

#include <stdint.h>
#include "io.h"

#define KBC_STATUS_PORT      0x64u  /* 8042 status register (read)            */
#define KBC_COMMAND_PORT     0x64u  /* 8042 command register (write)          */
#define KBC_STATUS_IBF       0x02u  /* bit 1: input buffer full               */
#ifndef POWER_MUT_NO_PULSE
#define KBC_CMD_PULSE_RESET  0xFEu  /* pulse output-port bit 0 = CPU RESET    */
#else
#define KBC_CMD_PULSE_RESET  0xFFu  /* MUTANT: pulse no bit -- nothing resets */
#endif
/* Bounded spins (Rule 2): each is one port read, about 1 us on an ISA-bus AT,
 * so 2^20 reads is about a second -- far above the controller's command
 * latency, small enough that a dead controller is reported, not waited on. */
#define KBC_SPIN_MAX         0x100000u

/* kernel_halt -- interrupts off, HLT forever. The machine stops here until the
 * operator switches it off (or presses the reset button). Never returns. */
static inline void kernel_halt(void) __attribute__((noreturn));
static inline void kernel_halt(void)
{
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

/* kernel_reset -- the 8042 RESET pulse. Interrupts go off first (nothing may
 * run between the decision to reset and the reset). Waits, bounded, for the
 * controller's input buffer to drain, writes 0xFE to the command port, then
 * spins, bounded, for the reset to land.
 *
 * Returns ONLY ON FAILURE, with what the caller needs for its PANIC line: the
 * last status byte read, plus 0x100 when the input buffer never drained (the
 * command was never written). A caller must treat a return as fatal (Rule 2):
 * kmain.c prints "PANIC kernel_reset: ..." and calls kernel_halt(). */
static inline uint32_t kernel_reset(void)
{
    uint32_t spins;
    uint8_t  st = 0u;

    __asm__ __volatile__("cli");
    for (spins = 0u; spins < KBC_SPIN_MAX; spins++) {
        st = inb((uint16_t)KBC_STATUS_PORT);
        if ((st & KBC_STATUS_IBF) == 0u) break;
    }
    if (spins >= KBC_SPIN_MAX)
        return 0x100u | (uint32_t)st;          /* the command was never sent */

    outb((uint16_t)KBC_COMMAND_PORT, (uint8_t)KBC_CMD_PULSE_RESET);

    for (spins = 0u; spins < KBC_SPIN_MAX; spins++)
        st = inb((uint16_t)KBC_STATUS_PORT);   /* the reset lands in here     */
    return (uint32_t)st;
}

#endif /* INITECH_POWER_H */

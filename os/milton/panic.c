/* panic.c -- fail-loud CPU-exception dispatcher (the IDT common-stub C side).
 *
 * beads: initech-a5a ("interrupt foundation").
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 3.3 (handler =
 *        dump to serial + console panic line + halt; NEVER return into the
 *        faulting instruction), Sec 8 Risk 1 (a wrong handler triple-faults --
 *        we halt cleanly instead). CLAUDE.md Law 2 (oracle is truth), Rule 2
 *        (fail fast/loud -- a panic with context beats a silently-wrong state),
 *        Rule 11 (deterministic; no timestamps), Rule 12 (ASCII only).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only.
 *
 * initech-s25: PRD Appendix B + Sec 5; CLAUDE Rule 2. Paint the physical
 * display directly from the boot handoff, even before CON is bound, and
 * independently of any FLAIR/DOS console viewport. VGA 8x16 at 2x is the
 * CRT transposition of the lane's two-line printer panel (no panic in the
 * Appendix A frame). Registers stay serial-only, behind PANIC_DEBUG=1.
 */

#include <stdint.h>
#include "idt.h"
#include "io.h"
#include "boot_info.h"
#include "panic_screen.h"
#include "pic.h"   /* PIC_SLAVE_BASE for the spurious-IRQ EOI discipline (bcg.7) */

/* 8259A master command port + non-specific EOI (OCW2). Ref: 8259A datasheet
 * (the same constants kbd.c/pit.c use to ack their IRQs). bcg.7. */
#define PIC_MASTER_CMD       0x20u
#define PIC_EOI_NONSPECIFIC  0x20u

/* COM1 16550 UART -- same protocol as kmain.c's serial_putc (poll LSR THRE,
 * write THR). Duplicated locally so panic has NO dependency on kmain internals
 * and works even if the rest of the kernel is wedged. */
#define COM1_THR 0x3F8u
#define COM1_LSR 0x3FDu
#define LSR_THRE 0x20u
/* Bounded spin: a stuck/absent UART (THRE never asserts) must NEVER hang the
 * panic path itself (Rule 2 -- the fail-loud path cannot be the thing that
 * wedges). Far above the worst-case poll count for a working 16550 at 115200
 * baud, small enough to bail in a fraction of a second on dead hardware. */
#define PSERIAL_SPIN_MAX 100000u

/* Release builds keep the PANIC marker but omit the private register dump.
 * The gate substitutes a -DPANIC_DEBUG=1 object; text/format is unchanged.
 * Ref: PRD Appendix B; initech-s25. */
#ifndef PANIC_DEBUG
#define PANIC_DEBUG 0
#endif

/* The CPU-exception triage hook (bead initech-tdnl.14; reconciliation Part C
 * item 7). NULL in every kernel except the FLAIRTENANTS ones, which install the
 * disk-tenant host's triage: a fault whose EIP lies inside the resident disk
 * tenant's image while tenant code runs is routed to FlairProcess_kill (the
 * hook never returns for it). For EVERY other fault the hook returns and the
 * halt below runs exactly as before -- fail-loud is preserved for all
 * non-tenant faults. */
static void (*g_fault_hook)(uint32_t vector, uint32_t eip) = 0;

void panic_set_fault_hook(void (*hook)(uint32_t vector, uint32_t eip))
{
    g_fault_hook = hook;
}

static void pserial_putc(char c)
{
    uint32_t spins = 0u;
    while ((inb(COM1_LSR) & LSR_THRE) == 0) {
        if (++spins >= PSERIAL_SPIN_MAX) {
            return;             /* UART not draining -> drop the byte, never hang */
        }
    }
    outb(COM1_THR, (uint8_t)c);
}

static void pserial_puts(const char *s)
{
    while (*s) {
        pserial_putc(*s++);
    }
}

/* Emit `v` as a fixed-width, zero-padded hex of `digits` nibbles (MSB first).
 * Deterministic, ASCII, no libc. */
static void pserial_hex(uint32_t v, int digits)
{
    static const char H[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--) {
        pserial_putc(H[(v >> (i * 4)) & 0xFu]);
    }
}

#if PANIC_DEBUG
/* Mnemonic table for the first 32 vectors (Intel SDM Vol 3A Table 6-1). */
static const char *exc_name(uint32_t vec)
{
    switch (vec) {
        case 0:  return "#DE divide error";
        case 1:  return "#DB debug";
        case 2:  return "NMI";
        case 3:  return "#BP breakpoint";
        case 4:  return "#OF overflow";
        case 5:  return "#BR bound range";
        case 6:  return "#UD invalid opcode";
        case 7:  return "#NM device n/a";
        case 8:  return "#DF double fault";
        case 10: return "#TS invalid TSS";
        case 11: return "#NP segment not present";
        case 12: return "#SS stack-segment fault";
        case 13: return "#GP general protection";
        case 14: return "#PF page fault";
        case 16: return "#MF x87 fp error";
        case 17: return "#AC alignment check";
        case 18: return "#MC machine check";
        case 19: return "#XM simd fp";
        case 20: return "#VE virtualization";
        case 21: return "#CP control protection";
        default: return "reserved/spurious";
    }
}
#endif

static void panic_draw(uint32_t vector)
{
    const boot_info_t *b=(const boot_info_t *)(uintptr_t)BOOT_INFO_ADDR;
    if (b->lfb_bpp==8u) {
        /* Reset our two indices even if FLAIR replaced the entire palette.
         * Ref: kmain.c vga_set_dac; boot_info.h lfb_dac_bits (6/8).
         * Black needs no quantisation; gray=48 at 6 bits, 192 at 8 bits. */
        uint8_t gray=b->lfb_dac_bits==8u ? 192u : 48u;
        outb(0x3C8,0); outb(0x3C9,0); outb(0x3C9,0); outb(0x3C9,0);
        outb(0x3C8,15); outb(0x3C9,gray); outb(0x3C9,gray); outb(0x3C9,gray);
    }
    panic_screen_paint((volatile uint8_t *)(uintptr_t)b->lfb_addr,
                       b->lfb_pitch,b->lfb_bpp,b->lfb_width,b->lfb_height,
                       (const uint8_t *)(uintptr_t)b->font_addr,vector);
}

/* The common-stub C entry. For ANY exception this is terminal: dump + halt. */
void KERNEL_ASM_ABI isr_dispatch_c(int_frame_t *frame)
{
    /* Spurious / unhandled vector (bcg.6): a stray software int or an
     * unexpected hardware vector arrives via isr_spurious with the sentinel
     * vector 0xFF -- NOT a CPU exception (those are 0..31 with their real
     * vector). The fail-loud-but-NON-fatal contract is to emit a grep-able
     * diagnostic and RETURN cleanly (isr_common's iret tail resumes the
     * interruptee), so one stray vector does not wedge the desktop forever.
     * The permanent halt below is reserved for genuine CPU exceptions. */
    if (frame->vector > 31u) {
        /* 8259A spurious-IRQ EOI discipline (bcg.7): a SLAVE spurious (IRQ15,
         * PIC_SLAVE_BASE+7) requires a MASTER-only EOI -- the master latched the
         * cascade (IRQ2), so without it the cascade stays in-service and blocks
         * every later slave IRQ. A MASTER spurious (IRQ7, PIC_MASTER_BASE+7)
         * gets NO EOI. Any other unhandled vector (sentinel 0xFF) gets none
         * either. Ref: Intel 8259A datasheet (spurious interrupt). */
        if (frame->vector == (uint32_t)(PIC_SLAVE_BASE + 7u)) {
            outb(PIC_MASTER_CMD, PIC_EOI_NONSPECIFIC);   /* master EOI only */
        }
        pserial_puts("SPURIOUS vec=");
        pserial_hex(frame->vector, 2);
        pserial_puts(" eip=");
        pserial_hex(frame->eip, 8);
        pserial_puts(" -- resuming\n");
        return;                 /* clean iret via isr_common; do NOT halt */
    }

    /* Tenant crash triage (tdnl.14): returns unless the fault is a disk
     * tenant's own, in which case it unwinds and never comes back here. */
    if (g_fault_hook != 0) {
        g_fault_hook(frame->vector, frame->eip);
    }

    panic_draw(frame->vector);

    /* Grep-able one-liner first (the oracle keys on "PANIC vec=NN err=MM"). */
    pserial_puts("PANIC vec=");
    pserial_hex(frame->vector, 2);
    pserial_puts(" err=");
    pserial_hex(frame->err_code, 8);
    pserial_putc('\n');

#if PANIC_DEBUG
    /* Human-readable register dump (Rule 2 -- a panic with context). */
    pserial_puts("  "); pserial_puts(exc_name(frame->vector)); pserial_putc('\n');
    pserial_puts("  eip="); pserial_hex(frame->eip, 8);
    pserial_puts(" cs=");   pserial_hex(frame->cs, 4);
    pserial_puts(" eflags=");pserial_hex(frame->eflags, 8); pserial_putc('\n');
    pserial_puts("  eax="); pserial_hex(frame->eax, 8);
    pserial_puts(" ebx="); pserial_hex(frame->ebx, 8);
    pserial_puts(" ecx="); pserial_hex(frame->ecx, 8);
    pserial_puts(" edx="); pserial_hex(frame->edx, 8); pserial_putc('\n');
    pserial_puts("  esi="); pserial_hex(frame->esi, 8);
    pserial_puts(" edi="); pserial_hex(frame->edi, 8);
    pserial_puts(" ebp="); pserial_hex(frame->ebp, 8); pserial_putc('\n');
#endif

    pserial_puts("HALTED\n");

    /* Terminal: never return into the faulting instruction (would loop). IF is
     * already 0 (interrupt gate); cli is belt-and-suspenders (Rule 2). */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

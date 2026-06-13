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
 * The full in-universe "PC LOAD LETTER" panic screen is beads initech-s25; this
 * milestone needs only a serial register dump (grep-able marker "PANIC vec=NN
 * err=MM") plus one console line, then a permanent cli;hlt loop.
 */

#include <stdint.h>
#include "idt.h"
#include "io.h"
#include "console.h"
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

/* Optional live console for the one-line panic banner. kmain.c sets this once
 * the console is up (panic_set_console); NULL before that -> serial-only. */
static console_t *g_panic_con = 0;

void panic_set_console(void *con)
{
    g_panic_con = (console_t *)con;
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

/* The common-stub C entry. For ANY exception this is terminal: dump + halt. */
void isr_dispatch_c(int_frame_t *frame)
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

    /* Grep-able one-liner first (the oracle keys on "PANIC vec=NN err=MM"). */
    pserial_puts("PANIC vec=");
    pserial_hex(frame->vector, 2);
    pserial_puts(" err=");
    pserial_hex(frame->err_code, 8);
    pserial_putc('\n');

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

    /* One visible console line (the full PC LOAD LETTER screen is initech-s25).
     * The hourglass-canon panic text plus the vector in hex. */
    if (g_panic_con) {
        console_puts(g_panic_con, "\nPC LOAD LETTER  (exception 0x");
        char hx[3];
        static const char H[] = "0123456789ABCDEF";
        hx[0] = H[(frame->vector >> 4) & 0xFu];
        hx[1] = H[frame->vector & 0xFu];
        hx[2] = 0;
        console_puts(g_panic_con, hx);
        console_puts(g_panic_con, ")\n");
    }

    pserial_puts("HALTED\n");

    /* Terminal: never return into the faulting instruction (would loop). IF is
     * already 0 (interrupt gate); cli is belt-and-suspenders (Rule 2). */
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

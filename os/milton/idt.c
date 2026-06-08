/* idt.c -- IDT table + gate encode + lidt. See idt.h for the full citations.
 *
 * beads: initech-a5a.
 * Ref: docs/research/internals-int21h-ground-truth.md Sec 2 (gate encode, lidt
 *      pseudo-descriptor); Intel SDM Vol 3A Sec 6.10-6.12 (IDT/gates/delivery),
 *      Vol 2A LIDT. CLAUDE.md Law 1, Rule 2 (fail loud), Rule 11 (deterministic),
 *      Rule 12 (ASCII). The gate-encode math is shared with the host oracle
 *      (test_idt.c) -- keep it freestanding-clean.
 */

#include "idt.h"

/* The 256-entry IDT and its LIDT pseudo-descriptor. .bss => zero-initialized;
 * no fixed physical address needed (flat binary, link addr == phys addr). */
static idt_gate_t g_idt[IDT_NUM_ENTRIES];
static idt_ptr_t  g_idt_ptr;

/* The 32 CPU-exception entry stubs (isr.asm). Declared as opaque code symbols;
 * we only ever take their addresses for the gate offset. */
extern void isr0(void);   extern void isr1(void);   extern void isr2(void);
extern void isr3(void);   extern void isr4(void);   extern void isr5(void);
extern void isr6(void);   extern void isr7(void);   extern void isr8(void);
extern void isr9(void);   extern void isr10(void);  extern void isr11(void);
extern void isr12(void);  extern void isr13(void);  extern void isr14(void);
extern void isr15(void);  extern void isr16(void);  extern void isr17(void);
extern void isr18(void);  extern void isr19(void);  extern void isr20(void);
extern void isr21(void);  extern void isr22(void);  extern void isr23(void);
extern void isr24(void);  extern void isr25(void);  extern void isr26(void);
extern void isr27(void);  extern void isr28(void);  extern void isr29(void);
extern void isr30(void);  extern void isr31(void);

/* The catch-all "spurious / unhandled vector" stub (isr.asm). Installed on all
 * vectors not given a dedicated handler so an unexpected vector fails loud via
 * the same panic path rather than triple-faulting (Rule 2). */
extern void isr_spurious(void);

void idt_set_gate(uint8_t vec, void *handler, uint16_t sel, uint8_t type_attr)
{
    uint32_t off = (uint32_t)(uintptr_t)handler;

    g_idt[vec].offset_lo = (uint16_t)(off & 0xFFFFu);
    g_idt[vec].selector  = sel;
    g_idt[vec].zero      = 0u;
    g_idt[vec].type_attr = type_attr;
#ifdef IDT_MUTATE_OFFSET_HI
    /* MUTANT (CLAUDE.md Rule 6 mutation-proof; defined only by the host oracle's
     * `make test-idt-mutant` run): wrong shift -- the encode oracle must go RED.
     * NEVER define this in a real build. */
    g_idt[vec].offset_hi = (uint16_t)((off >> 8) & 0xFFFFu);
#else
    g_idt[vec].offset_hi = (uint16_t)((off >> 16) & 0xFFFFu);
#endif
}

/* Read back a gate (a copy) -- a host-oracle hook so test_idt.c can assert the
 * EXACT bytes the REAL idt_set_gate wrote. Trivial + side-effect-free; harmless
 * in the kernel (never called there). */
idt_gate_t idt_get_gate(uint8_t vec)
{
    return g_idt[vec];
}

void idt_install_trap(uint8_t vec, void *handler)
{
    /* DPL=0 trap gate on the kernel code selector (initech-1f9 seam for the
     * INT 21h dispatcher at vector 0x21). */
    idt_set_gate(vec, handler, IDT_KERNEL_CS, IDT_GATE_TRAP32);
}

/* Table of the 32 exception stub entrypoints, indexed by vector. */
static void (*const g_exc_stubs[32])(void) = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
};

void idt_init(void)
{
    /* Point every vector at the spurious stub first (fail loud on the
     * unexpected), then override 0-31 with the real exception stubs. */
    for (int v = 0; v < IDT_NUM_ENTRIES; v++) {
        idt_set_gate((uint8_t)v, (void *)isr_spurious,
                     IDT_KERNEL_CS, IDT_GATE_INT32);
    }

    /* CPU exceptions 0-31: 32-bit INTERRUPT gates (0x8E -> IF cleared on
     * entry, no nesting during exception dispatch). Ref: ground-truth Sec 2.1. */
    for (int v = 0; v < 32; v++) {
        idt_set_gate((uint8_t)v, (void *)g_exc_stubs[v],
                     IDT_KERNEL_CS, IDT_GATE_INT32);
    }

    /* Vector 0x21 stays on the spurious stub here; the NEXT task installs the
     * INT 21h trap gate via idt_install_trap (initech-1f9). */

    g_idt_ptr.limit = (uint16_t)(sizeof(g_idt) - 1u);   /* 0x7FF */
    g_idt_ptr.base  = (uint32_t)(uintptr_t)&g_idt[0];

    __asm__ __volatile__("lidt %0" : : "m"(g_idt_ptr));
}

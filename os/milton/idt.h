/* idt.h -- InitechDOS Interrupt Descriptor Table (32-bit protected/flat).
 *
 * beads: initech-a5a ("Interrupt foundation: IDT + PIC remap/mask + CPU
 *        exception handlers + panic stub").
 * Ref:   docs/research/internals-int21h-ground-truth.md Sec 2 (IDT mechanics:
 *        8-byte gate descriptor, type_attr 0x8E interrupt / 0x8F trap, lidt
 *        pseudo-descriptor limit=0x7FF), Sec 3 (exception error-code list +
 *        uniform-frame dummy-0 pattern), Sec 8 Risk 2 (regs struct field order
 *        MUST match the asm pushad order); Intel SDM Vol 3A Sec 6.10-6.12 (IDT,
 *        gate descriptors, interrupt/exception delivery); Intel SDM Vol 2A
 *        PUSHAD (push order EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI) + LIDT/IRET.
 *        CLAUDE.md Law 1 (cite source), Rule 2 (fail loud), Rule 12 (ASCII).
 *
 * ARTIFACT code: freestanding (-ffreestanding -nostdlib), <stdint.h> only. The
 * encode/layout logic is ALSO compiled HOSTED by the factory oracle
 * (os/milton/test_idt.c, reuses seed/test_assert.h) -- so this header stays
 * hosted-clean (stdint only, no host headers).
 *
 * SEAM (the NEXT task, INT 21h, initech-1f9): vector 0x21 is reserved. The next
 * task installs the int 0x21 dispatcher as a TRAP gate (0x8F) via
 * idt_install_trap(0x21, int21_entry). idt_init() leaves 0x21 pointing at the
 * spurious stub until then. The int21_frame_t reuses THIS file's int_frame_t
 * field discipline (pushad order locked by spec/int21h_calling_convention.json).
 */
#ifndef INITECH_IDT_H
#define INITECH_IDT_H

#include <stdint.h>

/* Kernel code selector (stage2 GDT: gdt_code - gdt_start == 0x08; the far jump
 * at stage2.asm sets CS=CODE_SEL). All kernel handlers run on this selector. */
#define IDT_KERNEL_CS  0x08u

/* type_attr (byte at bits 47:40 of the 8-byte gate descriptor). P=1, DPL=0.
 *   0x8E = 32-bit INTERRUPT gate (clears IF on entry) -- used for exceptions.
 *   0x8F = 32-bit TRAP gate (leaves IF unchanged)     -- reserved for int 0x21.
 * Ref: ground-truth Sec 2.1; Intel SDM Vol 3A Sec 6.11. */
#define IDT_GATE_INT32   0x8Eu
#define IDT_GATE_TRAP32  0x8Fu

#define IDT_NUM_ENTRIES  256

/* An 8-byte IDT gate descriptor (Intel SDM Vol 3A Fig 6-2). The handler offset
 * is SPLIT across two fields (offset_lo = bits 15:0, offset_hi = bits 31:16) --
 * the classic encode bug (ground-truth Sec 8 Risk 2) lives here; the host oracle
 * pins the exact byte layout. `zero` is the always-0 byte between selector and
 * type_attr. Packed so sizeof == 8 (asserted below). */
typedef struct {
    uint16_t offset_lo;   /* handler address bits 15:0  */
    uint16_t selector;    /* code segment selector (IDT_KERNEL_CS)            */
    uint8_t  zero;        /* reserved, must be 0                              */
    uint8_t  type_attr;   /* P | DPL | 0 | type (0x8E interrupt / 0x8F trap)  */
    uint16_t offset_hi;   /* handler address bits 31:16 */
} __attribute__((packed)) idt_gate_t;

_Static_assert(sizeof(idt_gate_t) == 8, "idt_gate_t must be exactly 8 bytes");

/* The 6-byte LIDT pseudo-descriptor: limit (size-1) then 32-bit base.
 * limit = IDT_NUM_ENTRIES*8 - 1 = 0x7FF (ground-truth Sec 2.3). */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

_Static_assert(sizeof(idt_ptr_t) == 6, "idt_ptr_t must be exactly 6 bytes");

/* The uniform register frame the common asm stub (isr.asm) builds on the stack
 * and hands (by pointer) to isr_dispatch_c. Field order is LOAD-BEARING: it
 * MUST mirror, top-of-stack (lowest address) first, exactly what the stub
 * pushes. The stub does, in order:
 *   push <dummy 0 OR cpu error code>   (highest of our pushes)
 *   push <vector number>
 *   push gs / fs / es / ds
 *   pushad   -> EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI (EDI lands lowest)
 * so reading from ESP upward (lowest address = first struct field) gives:
 *   edi,esi,ebp,esp_dummy,ebx,edx,ecx,eax, ds,es,fs,gs, vector, err_code,
 *   then the CPU-pushed eip,cs,eflags (no SS:ESP -- ring0->ring0, no CPL change).
 * Ref: ground-truth Sec 3.2 + Sec 8 Risk 2; Intel SDM Vol 2A PUSHAD order;
 * Vol 3A Sec 6.12.1 (interrupt stack frame). The host oracle (test_idt.c)
 * asserts every offsetof here against the documented byte positions. */
typedef struct {
    /* pushad pushes EAX first ... EDI last; EDI is therefore at the LOWEST
     * address (top of stack) and is the FIRST field. esp_dummy is the ESP
     * value pushad captured -- not meaningful as a stack pointer here. */
    uint32_t edi;        /* +0  */
    uint32_t esi;        /* +4  */
    uint32_t ebp;        /* +8  */
    uint32_t esp_dummy;  /* +12 (pushad's saved ESP; ignore)                 */
    uint32_t ebx;        /* +16 */
    uint32_t edx;        /* +20 */
    uint32_t ecx;        /* +24 */
    uint32_t eax;        /* +28 */
    /* Segment selectors pushed by the stub (32-bit slots; push of a 16-bit
     * seg reg in 32-bit mode writes a zero-extended dword). */
    uint32_t ds;         /* +32 */
    uint32_t es;         /* +36 */
    uint32_t fs;         /* +40 */
    uint32_t gs;         /* +44 */
    /* Pushed by the per-vector stub. */
    uint32_t vector;     /* +48 */
    uint32_t err_code;   /* +52 (real CPU error code, or dummy 0)            */
    /* Pushed by the CPU on interrupt delivery (no CPL change in ring0-only). */
    uint32_t eip;        /* +56 */
    uint32_t cs;         /* +60 */
    uint32_t eflags;     /* +64 */
} __attribute__((packed)) int_frame_t;

_Static_assert(sizeof(int_frame_t) == 68, "int_frame_t must be 17 dwords (68 bytes)");

/* Set IDT entry `vec` to dispatch through `handler` on selector `sel` with the
 * given type_attr (0x8E interrupt / 0x8F trap). The handler offset is split
 * lo/hi here -- the host oracle pins this. */
void idt_set_gate(uint8_t vec, void *handler, uint16_t sel, uint8_t type_attr);

/* Read back a gate descriptor (a copy). Host-oracle hook (test_idt.c) so the
 * EXACT bytes idt_set_gate wrote can be asserted; harmless in the kernel. */
idt_gate_t idt_get_gate(uint8_t vec);

/* Convenience seam for the NEXT task (initech-1f9): install a 32-bit TRAP gate
 * (0x8F, DPL=0, kernel CS) at `vec`. The INT 21h dispatcher installs at 0x21
 * via this. */
void idt_install_trap(uint8_t vec, void *handler);

/* Install a 32-bit INTERRUPT gate (0x8E, DPL=0, kernel CS) at `vec` -- IF is
 * CLEARED on entry, so the handler runs with hardware interrupts disabled (no
 * IRQ nesting). Used for the hardware IRQ stubs (PIT IRQ0 -> vector 0x28,
 * keyboard IRQ1 -> vector 0x29 after the PIC remap; beads initech-3rs). */
void idt_install_irq(uint8_t vec, void *handler);

/* Zero the table, install the 32 CPU-exception stubs (isr0..isr31) as 0x8E
 * interrupt gates, point all other vectors at the spurious-interrupt stub, then
 * lidt. Does NOT enable interrupts (the kernel keeps IF=0 this milestone). */
void idt_init(void);

/* The C dispatcher the asm common stub calls. For a CPU exception it FAILS LOUD
 * (serial register dump + one console panic line) then halts forever -- it must
 * NEVER return into a faulting instruction (would loop). Defined in panic.c. */
void isr_dispatch_c(int_frame_t *frame);

/* Bind a live console for the one-line panic banner (panic.c). NULL (the
 * default) -> serial-only. kmain.c calls this once the console is up. Typed as
 * void* so idt.h need not include console.h; panic.c casts to console_t*. */
void panic_set_console(void *con);

#endif /* INITECH_IDT_H */

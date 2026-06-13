/* test_idt.c -- host unit oracle for the IDT gate encode + interrupt-frame
 * layout (beads initech-a5a). Factory test: libc OK, reuses seed/test_assert.h.
 *
 * Ref: docs/research/internals-int21h-ground-truth.md Sec 2.1 (8-byte gate
 *      descriptor: offset SPLIT lo[15:0]/hi[31:16], selector, zero, type_attr),
 *      Sec 8 Risk 2 (the int_frame_t field order is load-bearing and MUST match
 *      the asm pushad order). CLAUDE.md Law 2 (the oracle is truth), Rule 1
 *      (RED->GREEN), Rule 6 (mutation-prove), Rule 12 (ASCII).
 *
 * Compiled HOSTED against the REAL artifact idt.c (the same idt_set_gate the
 * kernel uses). Two property classes:
 *   1. ENCODE: a known (vector, handler addr, selector, type) lands as the EXACT
 *      8 bytes the Intel layout demands -- catches the offset-split bug.
 *   2. FRAME OFFSETS: every int_frame_t field sits at the offset the documented
 *      pushad order requires -- catches a transposed field.
 *
 * MUTATION (Rule 6): build idt.c with -DIDT_MUTATE_OFFSET_HI to make the
 * offset-hi shift wrong (>>8 instead of >>16) and confirm the encode test goes
 * RED. The macro is consumed by the REAL idt_set_gate in idt.c; `make
 * test-idt-mutant` runs that build and asserts the oracle bites.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "idt.h"
#include "test_assert.h"

TEST_HARNESS();

/* idt.c's g_exc_stubs takes the addresses of these 32 stubs + isr_spurious at
 * load time, so they must exist at link. The host test never dispatches; dummy
 * bodies suffice. (idt_init / lidt is likewise never invoked here.) */
#define STUB(n) void isr##n(void) {}
STUB(0)  STUB(1)  STUB(2)  STUB(3)  STUB(4)  STUB(5)  STUB(6)  STUB(7)
STUB(8)  STUB(9)  STUB(10) STUB(11) STUB(12) STUB(13) STUB(14) STUB(15)
STUB(16) STUB(17) STUB(18) STUB(19) STUB(20) STUB(21) STUB(22) STUB(23)
STUB(24) STUB(25) STUB(26) STUB(27) STUB(28) STUB(29) STUB(30) STUB(31)
void isr_spurious(void) {}
void isr_irq7_spurious(void) {}    /* 8259A master spurious stub (bcg.7) */
void isr_irq15_spurious(void) {}   /* 8259A slave  spurious stub (bcg.7) */

/* Hand-compute the 8 descriptor bytes for a sample handler (the independent
 * expected value -- NOT derived from idt_set_gate's code). */
static void expected_gate_bytes(uint32_t handler, uint16_t sel, uint8_t type,
                                uint8_t out[8])
{
    uint16_t lo = (uint16_t)(handler & 0xFFFFu);
    uint16_t hi = (uint16_t)((handler >> 16) & 0xFFFFu);
    out[0] = (uint8_t)(lo & 0xFFu);
    out[1] = (uint8_t)((lo >> 8) & 0xFFu);
    out[2] = (uint8_t)(sel & 0xFFu);
    out[3] = (uint8_t)((sel >> 8) & 0xFFu);
    out[4] = 0u;            /* zero byte */
    out[5] = type;
    out[6] = (uint8_t)(hi & 0xFFu);
    out[7] = (uint8_t)((hi >> 8) & 0xFFu);
}

int main(void)
{
    /* --- Property 1: descriptor size + encode (the offset-split contract). --- */
    CHECK(sizeof(idt_gate_t) == 8, "idt_gate_t must be 8 bytes");

    const uint32_t H = 0x00123456u;   /* sample handler address */
    const uint16_t SEL = IDT_KERNEL_CS;
    const uint8_t  TY  = IDT_GATE_INT32;

    uint8_t want[8];
    expected_gate_bytes(H, SEL, TY, want);
    /* Spot-check the hand-computed bytes for 0x00123456 / sel 0x08 / 0x8E:
     *   lo = 0x3456 -> bytes 0x56,0x34 ; sel 0x08 -> 0x08,0x00 ; zero 0x00 ;
     *   type 0x8E ; hi = 0x0012 -> 0x12,0x00. */
    CHECK(want[0] == 0x56u && want[1] == 0x34u, "offset_lo split wrong");
    CHECK(want[2] == 0x08u && want[3] == 0x00u, "selector bytes wrong");
    CHECK(want[4] == 0x00u, "zero byte must be 0");
    CHECK(want[5] == 0x8Eu, "type_attr wrong");
    CHECK(want[6] == 0x12u && want[7] == 0x00u, "offset_hi split wrong");

    /* Exercise the REAL artifact encode: install via idt_set_gate, read back
     * via idt_get_gate, compare the raw bytes to the hand-computed expectation.
     * This is what the -DIDT_MUTATE_OFFSET_HI build breaks (offset-split bug). */
    idt_set_gate(0x21u, (void *)(uintptr_t)H, SEL, TY);
    idt_gate_t g = idt_get_gate(0x21u);

    uint8_t got[8];
    memcpy(got, &g, 8);
    CHECK(memcmp(got, want, 8) == 0,
          "idt_set_gate bytes != hand-computed expected (offset-split bug?)");

    /* A second handler with a different high word -- pins offset_hi independently
     * (a >>8 mutant corrupts hi while lo happens to still match for some addrs). */
    const uint32_t H2 = 0xDEADBEEFu;
    uint8_t want2[8];
    expected_gate_bytes(H2, SEL, IDT_GATE_TRAP32, want2);
    idt_set_gate(0x80u, (void *)(uintptr_t)H2, SEL, IDT_GATE_TRAP32);
    idt_gate_t g2 = idt_get_gate(0x80u);
    CHECK(memcmp(&g2, want2, 8) == 0,
          "idt_set_gate bytes for 0xDEADBEEF/trap != expected");

    /* --- Property 2: int_frame_t field offsets match the pushad order. ------
     * The asm common stub builds, low-address-first:
     *   pushad: edi,esi,ebp,esp,ebx,edx,ecx,eax  (EDI lowest)
     *   then ds,es,fs,gs  (pushed gs,fs,es,ds; ds just above eax)
     *   then vector, err_code  (pushed by the per-vector stub)
     *   then eip,cs,eflags  (CPU). A transposed field here => RED. */
    CHECK(offsetof(int_frame_t, edi)       ==  0, "edi offset");
    CHECK(offsetof(int_frame_t, esi)       ==  4, "esi offset");
    CHECK(offsetof(int_frame_t, ebp)       ==  8, "ebp offset");
    CHECK(offsetof(int_frame_t, esp_dummy) == 12, "esp_dummy offset");
    CHECK(offsetof(int_frame_t, ebx)       == 16, "ebx offset");
    CHECK(offsetof(int_frame_t, edx)       == 20, "edx offset");
    CHECK(offsetof(int_frame_t, ecx)       == 24, "ecx offset");
    CHECK(offsetof(int_frame_t, eax)       == 28, "eax offset");
    CHECK(offsetof(int_frame_t, ds)        == 32, "ds offset");
    CHECK(offsetof(int_frame_t, es)        == 36, "es offset");
    CHECK(offsetof(int_frame_t, fs)        == 40, "fs offset");
    CHECK(offsetof(int_frame_t, gs)        == 44, "gs offset");
    CHECK(offsetof(int_frame_t, vector)    == 48, "vector offset");
    CHECK(offsetof(int_frame_t, err_code)  == 52, "err_code offset");
    CHECK(offsetof(int_frame_t, eip)       == 56, "eip offset");
    CHECK(offsetof(int_frame_t, cs)        == 60, "cs offset");
    CHECK(offsetof(int_frame_t, eflags)    == 64, "eflags offset");
    CHECK(sizeof(int_frame_t) == 68, "int_frame_t total size");

    /* --- LIDT pseudo-descriptor sanity (limit = 256*8 - 1 = 0x7FF). --- */
    CHECK(sizeof(idt_ptr_t) == 6, "idt_ptr_t must be 6 bytes");
    CHECK((IDT_NUM_ENTRIES * 8 - 1) == 0x7FF, "IDT limit constant");

    return TEST_SUMMARY("test_idt");
}

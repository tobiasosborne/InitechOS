/* test_tsr.c -- host oracle for the InitechDOS TSR kernel mechanics
 * (bead initech-6fdt sub-deliverable 2; spec/tsr_contract.h).
 *
 * Drives the REAL int21.c + mcb.c translation units HOSTED (the same TUs the
 * kernel links) through int21_dispatch / int27_dispatch:
 *   T1  AH=31h from a top-level program whose PSP sits BELOW the disjoint heap
 *       arena (the loader's depth-0 layout): the resident range is
 *       [PSP, PSP + DX*16); int21_tsr_top() reports its end, AH=4Dh AH=3.
 *   T2  TR p. 6-105: the KEEP does not close the program's files and does not
 *       free its other blocks (a heap block becomes resident, and the resident
 *       top covers it).
 *   T3  the next load goes ABOVE the resident: the arena re-bound above the
 *       residents hands EXEC a PSP at or above int21_tsr_top().
 *   T4  31h twice = two residents: the second (an MCB-hosted program above the
 *       first) shrinks its own block resident; count 2, top grows, the first
 *       resident's bytes are untouched.
 *   T5  INT 27h (TR p. 6-26): EDX bytes -> paragraphs, exit code 0, type 3.
 *   T6  DX below the PSP floor keeps the PSP (TSR_KEEP_MIN_PARAS).
 *   T7  AH=34h returns the InDOS byte's flat address: 0 outside DOS, 1 inside a
 *       console read, 0 while an EXEC child runs user code, restored after.
 *   T8  AH=25h gate type: IRQ vectors 0x28..0x37 get 0x8E (DEC-04a.1), every
 *       software vector 0x8F.
 * Mutants (Rule 6, make test-tsr-unit-mutant): TSR_MUT_NO_KEEP (31h acts as
 * 4Ch), INT21_MUT_INDOS_EXEC (EXEC keeps the parent's InDOS), INT21_MUT_
 * SETVECT_TRAP (every vector a trap gate). Each must go RED.
 * CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, Rule 12. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include "int21.h"
#include "mcb.h"
#include "psp.h"
#include "sft.h"
#include "tsr_contract.h"
#include "test_low_mapping.h"

static int g_checks = 0, g_fails = 0;
#define CHECK(cond, msg)                                                    \
    do {                                                                    \
        g_checks++;                                                         \
        if (!(cond)) {                                                      \
            g_fails++;                                                      \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, msg); \
        }                                                                   \
    } while (0)

static int     g_exit_fired;
static uint8_t g_exit_code;
static void rec_exit(uint8_t code) { g_exit_fired = 1; g_exit_code = code; }

static int_frame_t frame(uint8_t ah, uint8_t al, uint32_t ebx, uint32_t edx)
{
    int_frame_t f;
    memset(&f, 0, sizeof f);
    f.eflags = 0x00000203u;             /* CF preset: success must clear it */
    f.eax = ((uint32_t)ah << 8) | al;
    f.ebx = ebx;
    f.edx = edx;
    return f;
}

static int_frame_t call(uint8_t ah, uint8_t al, uint32_t ebx, uint32_t edx)
{
    int_frame_t f = frame(ah, al, ebx, edx);
    int21_dispatch(&f);
    return f;
}

static void bind_psp(psp_t *p)
{
    psp_params_t pp;
    memset(&pp, 0, sizeof pp);
    pp.alloc_end_linear = (uint32_t)(uintptr_t)p + 0x80000u;
    (void)psp_build(p, &pp);
    int21_set_psp((struct psp *)p);
}

/* The window: PSP at W, image W+0x100.., the disjoint arena [W+0x4000, W+WIN). */
#define WIN        0x80000u
#define ARENA_OFF  0x4000u
static uint8_t *W;
static uint32_t Wl;

static uint16_t block_owner_at(uint32_t linear_data)
{
    /* the MCB header sits one paragraph below a block's data */
    return ((const mcb_t *)(uintptr_t)(linear_data - 16u))->owner;
}

/* ---- T7 helpers: observe the InDOS byte from inside nested calls --------- */
static volatile const uint8_t *g_indos_byte;
static int g_seen_in_conin = -1, g_seen_in_child = -1;
static int conin_peek(void) { g_seen_in_conin = *g_indos_byte; return 'k'; }
static int conin_poll(void) { return -1; }
static uint16_t exec_peek(const char *n, uint16_t d, const char *t, uint32_t tl,
                          uint32_t env, uint8_t *rc)
{
    (void)n; (void)d; (void)t; (void)tl; (void)env;
    g_seen_in_child = *g_indos_byte;
    *rc = 0;
    return 0;
}

int main(void)
{
    W = (uint8_t *)alloc_low(WIN);
    psp_t *kpsp = (psp_t *)alloc_low(sizeof(psp_t));
    char  *name = (char *)alloc_low(16);
    if (!W || !kpsp || !name) {
        fprintf(stderr, "test_tsr: low mapping unavailable\n");
        return 2;
    }
    Wl = (uint32_t)(uintptr_t)W;
    memset(W, 0, WIN);
    sft_init();
    int21_set_exit(rec_exit);

    /* ---- T1: top-level KEEP, PSP below the disjoint arena ------------------ */
    psp_t *tsr1 = (psp_t *)W;
    bind_psp(tsr1);
    CHECK(int21_mcb_bind_program(Wl + ARENA_OFF, Wl + WIN) == 1, "T1: arena bound");
    memset(W + 0x100, 0xA5, 0x300);     /* the resident's code bytes */

    /* T2 setup: an open FILE handle and a heap block the program owns. */
    uint8_t slot = sft_alloc();
    CHECK(slot != 0xFFu, "T2: a free SFT slot");
    g_sft[slot].kind = SFT_KIND_FILE;
    g_sft[slot].ref_count = 1u;
    tsr1->jft[5] = slot;
    int_frame_t a = call(0x48u, 0u, 0x40u, 0u);        /* ALLOC 0x40 paragraphs */
    CHECK((a.eflags & 1u) == 0u, "T2: heap ALLOC ok");
    /* The first block of a fresh arena: data at base + 1 paragraph. AX carries
     * only the low 16 bits of the host segment, so check those and use the
     * linear address directly. */
    uint32_t heap_lin = Wl + ARENA_OFF + 16u;
    uint32_t heap_end = heap_lin + 0x400u;
    CHECK((a.eax & 0xFFFFu) == ((heap_lin >> 4) & 0xFFFFu), "T2: heap is block 1");

    g_exit_fired = 0;
    (void)call(0x31u, 9u, 0u, 0x40u);                  /* KEEP DX=40h, AL=9 */
    CHECK(g_exit_fired && g_exit_code == 9u, "T1: KEEP terminated with AL");
    CHECK(int21_tsr_count() == 1u, "T1: one resident");
    CHECK(int21_tsr_top() >= Wl + 0x400u, "T1: resident covers PSP + DX*16");
    int_frame_t rc = call(0x4Du, 0u, 0u, 0u);
    CHECK((rc.eax & 0xFFFFu) == 0x0309u, "T1: 4Dh AH=3 (KEEP) AL=9");
    CHECK(tsr1->jft[5] == slot && g_sft[slot].kind == SFT_KIND_FILE &&
          g_sft[slot].ref_count == 1u, "T2: KEEP left the file open (TR 6-105)");
    CHECK(block_owner_at(heap_lin) == MCB_OWNER_SYSTEM,
          "T2: the heap block was not freed; it is resident");
    CHECK(int21_tsr_top() == heap_end, "T2: resident top covers the kept heap");

    /* ---- T3: the next load is placed ABOVE the resident -------------------- */
    int21_set_psp((struct psp *)kpsp);
    CHECK(int21_mcb_bind_above_residents(Wl + WIN) == 1, "T3: arena above residents");
    uint32_t cpsp = 0, cend = 0;
    CHECK(int21_mcb_exec_alloc(0x200u, &cpsp, &cend) == 0u, "T3: EXEC block");
    CHECK(cpsp >= int21_tsr_top(), "T3: next load's PSP is above the resident");
    CHECK(W[0x100] == 0xA5 && W[0x3FF] == 0xA5, "T3: resident bytes intact");

    /* ---- T4: 31h twice = two residents ------------------------------------ */
    psp_t *tsr2 = (psp_t *)(uintptr_t)cpsp;
    bind_psp(tsr2);
    (void)call(0x31u, 0u, 0u, 0x20u);
    CHECK(int21_tsr_count() == 2u, "T4: two residents");
    CHECK(int21_tsr_top() == cpsp + 0x200u, "T4: top = second PSP + DX*16");
    CHECK(block_owner_at(cpsp) == MCB_OWNER_SYSTEM, "T4: second block resident");
    int21_mcb_exec_free(cpsp);          /* the loader's post-child reclaim */
    CHECK(block_owner_at(cpsp) == MCB_OWNER_SYSTEM, "T4: reclaim spares it");
    CHECK(W[0x100] == 0xA5, "T4: first resident untouched");

    /* ---- T5: INT 27h, EDX = resident bytes from the PSP -------------------- */
    int21_set_psp((struct psp *)kpsp);
    CHECK(int21_mcb_bind_above_residents(Wl + WIN) == 1, "T5: rebind");
    CHECK(int21_mcb_exec_alloc(0x100u, &cpsp, &cend) == 0u, "T5: EXEC block");
    bind_psp((psp_t *)(uintptr_t)cpsp);
    g_exit_fired = 0;
    int_frame_t f27 = frame(0u, 0x55u, 0u, 0x123u);
    int27_dispatch(&f27);
    CHECK(g_exit_fired && g_exit_code == 0u, "T5: INT 27h exit code 0");
    CHECK(int21_tsr_count() == 3u && int21_tsr_top() == cpsp + 0x130u,
          "T5: INT 27h kept (0x123+15)/16 paragraphs");
    rc = call(0x4Du, 0u, 0u, 0u);
    CHECK(((rc.eax >> 8) & 0xFFu) == 3u, "T5: INT 27h reports type 3");

    /* ---- T6: DX below the PSP floor keeps the PSP --------------------------- */
    int21_set_psp((struct psp *)kpsp);
    CHECK(int21_mcb_bind_above_residents(Wl + WIN) == 1, "T6: rebind");
    CHECK(int21_mcb_exec_alloc(0x100u, &cpsp, &cend) == 0u, "T6: EXEC block");
    bind_psp((psp_t *)(uintptr_t)cpsp);
    (void)call(0x31u, 0u, 0u, 0x2u);
    CHECK(int21_tsr_top() == cpsp + TSR_KEEP_MIN_PARAS * 16u,
          "T6: DX=2 keeps the 16-paragraph PSP");

    /* ---- T7: AH=34h, the InDOS byte ------------------------------------------ */
    int21_set_psp((struct psp *)kpsp);
    int_frame_t q = call(0x34u, 0u, 0u, 0u);
    CHECK((q.eflags & 1u) == 0u, "T7: 34h CF clear");
    g_indos_byte = (volatile const uint8_t *)int21_indos_ptr();
    CHECK(q.ebx == (uint32_t)(uintptr_t)g_indos_byte, "T7: EBX = InDOS address");
    CHECK(*g_indos_byte == 0u, "T7: InDOS 0 outside DOS");
    int21_set_conin(conin_peek, conin_poll);
    (void)call(0x08u, 0u, 0u, 0u);
    CHECK(g_seen_in_conin == 1, "T7: InDOS 1 inside a console read");
    int21_set_exec_backend(exec_peek);
    strcpy(name, "CHILD.COM");
    int_frame_t ex = call(0x4Bu, 0u, 0u, (uint32_t)(uintptr_t)name);
    CHECK((ex.eflags & 1u) == 0u, "T7: EXEC ran");
    CHECK(g_seen_in_child == 0, "T7: InDOS 0 while the child runs user code");
    CHECK(*g_indos_byte == 0u, "T7: InDOS 0 after EXEC returns");

    /* ---- T8: AH=25h gate type ------------------------------------------------ */
    CHECK(int21_vector_gate(0x29u) == 0x8Eu, "T8: IRQ1 vector -> interrupt gate");
    CHECK(int21_vector_gate(0x28u) == 0x8Eu && int21_vector_gate(0x37u) == 0x8Eu,
          "T8: whole IRQ band 0x28..0x37 -> interrupt gates");
    CHECK(int21_vector_gate(0x27u) == 0x8Fu && int21_vector_gate(0x38u) == 0x8Fu &&
          int21_vector_gate(0x21u) == 0x8Fu, "T8: software vectors -> trap gates");

    printf("test_tsr: %d checks, %d failures\n", g_checks, g_fails);
    return g_fails ? 1 : 0;
}

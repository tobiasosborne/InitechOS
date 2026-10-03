/* fpu.h -- x87 FPU bring-up: the PURE decision logic (beads initech-zj6w).
 *
 * THE ARTIFACT (CLAUDE.md Law 3): freestanding, <stdint.h> only, header-only
 * static inlines. Shared VERBATIM by the kernel (sysinit.c sysinit_fpu_init,
 * which owns the CR0 / x87 instructions) and the host oracle
 * (os/milton/test_fpu.c), so the probe verdict and the CR0 arithmetic the boot
 * path uses are exactly what the host gate grades (the flair_heap_ram_ok
 * pattern, spec/memory_map.h).
 *
 * THE CONTRACT (operator ruling 2026-10-03, beads initech-zj6w / initech-yjlo;
 * PRD Sec 5; ADR-0001 DEC-02/DEC-04; spec/hardware.json "fpu"):
 *   - FPU present (486DX on-die, or a 387/487): CR0.EM=0, CR0.MP=1, CR0.NE=1,
 *     CR0.TS=0, FNINIT, FLDCW X87_BOOT_CONTROL_WORD (spec/fpu_contract.h).
 *     Single-task cooperative scheduling -> no lazy context switch, TS stays 0.
 *     NE=1 -> an unmasked x87 fault is delivered as #MF (vector 16), which the
 *     IDT routes to the fail-loud panic (panic.c).
 *   - FPU absent (386SX/486SX class; "386 desirable, non-blocking", ADR-0001):
 *     the OS must still boot. CR0.EM=1, CR0.MP=0, TS=0 -> the FIRST x87
 *     instruction traps #NM (vector 7) and panics there, loudly. No boot
 *     panic, no software emulator (SAMIR is soft-float, ADR-0009 DEC-01).
 *
 * Intel references (Law 1). NO local copy of the Intel SDM exists under docs/
 * or spec/ (checked 2026-10-03); the citations below are recorded so a
 * reviewer can verify them against the SDM, and are NOT backed locally:
 *   - SDM Vol 3A Sec 2.5 "Control Registers": CR0.MP bit 1, EM bit 2, TS bit 3,
 *     ET bit 4, NE bit 5 (NE is 486+; reserved on the 386).
 *   - SDM Vol 3A Sec 9.2 "x87 FPU Initialization" incl. the recommended EM/MP
 *     settings table (FPU present: EM=0 MP=1; no FPU: EM=1 MP=0) and the
 *     FNINIT/FNSTSW/FNSTCW presence test.
 *   - SDM Vol 3A Table 9-1 (x87 state after RESET: CW 0040H, TW 5555H).
 *   - SDM Vol 1 Sec 8.1.5 (control word 037FH after FINIT).
 *   - The probe constants (status low byte == 0; control word AND 103FH ==
 *     003FH; memory pre-loaded with a 5A5AH sentinel because an absent
 *     coprocessor stores nothing) are the classic Intel detection sequence
 *     (Intel AP-485 "Processor Identification" FPU check). Also not local.
 *
 * Rule 2 (fail loud), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_MILTON_FPU_H
#define INITECH_MILTON_FPU_H

#include <stdint.h>

/* CR0 bits (SDM Vol 3A Sec 2.5). */
#define CR0_MP  0x00000002u   /* monitor coprocessor: WAIT honours TS */
#define CR0_EM  0x00000004u   /* emulation: x87 instructions raise #NM */
#define CR0_TS  0x00000008u   /* task switched: lazy-save trap (unused here) */
#define CR0_NE  0x00000020u   /* numeric error: native #MF, not FERR#/IRQ13 */

/* Sentinel pre-loaded into the probe's store targets: an absent coprocessor
 * writes nothing, so the sentinel survives and the verdict is "absent". */
#define FPU_PROBE_SENTINEL  0x5A5Au

/* Probe verdict from the words FNSTSW / FNSTCW stored right after FNINIT.
 * Present iff the status word's low byte is 0 (FNINIT cleared every exception
 * flag and the stack fault / ES bits) AND the control word, masked to the bits
 * FNINIT defines (exception masks 0-5 + bit 12), reads 003FH. */
static inline int fpu_probe_present(uint16_t sw, uint16_t cw)
{
#ifdef FPU_MUTATE_PROBE_SW_ONLY
    /* MUTANT (host oracle only, make test-fpu-probe-mutant): trust the status
     * word alone -- a bus that reads back zeros would pass as an FPU. */
    (void)cw;
    return (sw & 0x00FFu) == 0u;
#else
    return ((sw & 0x00FFu) == 0u) && ((cw & 0x103Fu) == 0x003Fu);
#endif
}

/* CR0 for an FPU-present machine (also the state the probe runs in): EM=0 so
 * the probe instructions reach the coprocessor, TS=0, MP=1, NE=1. Every other
 * CR0 bit (PE, ET, ...) is preserved. */
static inline uint32_t fpu_cr0_present(uint32_t cr0)
{
    return (cr0 & ~(CR0_EM | CR0_TS)) | CR0_MP | CR0_NE;
}

/* CR0 for an FPU-absent machine: EM=1 (the first x87 instruction traps #NM),
 * MP=0, TS=0. NE is left set (no effect without an FPU). */
static inline uint32_t fpu_cr0_absent(uint32_t cr0)
{
#ifdef FPU_MUTATE_ABSENT_KEEPS_EM_CLEAR
    /* MUTANT (host oracle only): leave EM clear -- x87 ops on a machine with
     * no coprocessor would then silently do nothing instead of trapping. */
    return (cr0 & ~(CR0_MP | CR0_TS | CR0_EM)) | CR0_NE;
#else
    return (cr0 & ~(CR0_MP | CR0_TS)) | CR0_EM | CR0_NE;
#endif
}

#endif /* INITECH_MILTON_FPU_H */

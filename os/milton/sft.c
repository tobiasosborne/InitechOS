/* sft.c -- InitechDOS System File Table (SFT) + Job File Table (JFT) layer.
 *
 * beads: initech-509.3. The per-process JFT (psp_t.jft[20], offset 0x18) indexes
 *        a system-wide SFT (g_sft[20]); handles 0-4 are predefined; DUP (45h) /
 *        DUP2 (46h) duplicate/redirect handles for the shell's I/O redirection.
 *
 * Ref:   ADR-0003 Sec 5.6 (DEC-06: 20-entry JFT -> system SFT sized by FILES=;
 *        handles 0-4 predefined), Appendix D.2 (FILES=20);
 *        docs/research/fs-mount-sft-ground-truth.md Sec 3 (the model, the entry
 *        struct, the DUP/DUP2 algorithms); spec/dos_structs.h (psp_t.jft);
 *        DOS 3.3 Programmer's Reference Manual AH=45h/46h. CLAUDE.md Law 1
 *        (cite), Law 2 (oracle), Law 3 (artifact = C), Rule 2 (fail loud),
 *        Rule 11 (deterministic), Rule 12 (ASCII).
 *
 * This TU compiles BOTH freestanding (-ffreestanding -nostdlib, the kernel) and
 * HOSTED (the factory oracle test_sft.c). It performs NO I/O -- pure table
 * manipulation over g_sft[] and the caller-supplied psp->jft[] -- so it is a
 * fully host-unit-testable module, exactly like psp.c.
 *
 * SINGLE-PROCESS NOTE (milestone scope): ref_count tracks JFT references within
 * the current process. SFT teardown / re-init on process EXIT (so a second
 * program starts with a clean device table) is deferred to the multi-process
 * milestone; sft_init() is called once at SYSINIT. The cooperative single-
 * program model (CLAUDE.md "Cooperative, not preemptive") makes this correct
 * for the current release.
 */

#include "sft.h"

/* ------------------------------------------------------------------------ *
 * Fail-loud (CLAUDE.md Rule 2), mirroring psp.c. A corrupt JFT entry, or a
 * release of a slot that is already free, is an INVARIANT VIOLATION -- never a
 * recoverable condition. Hosted: abort() (the oracle observes a non-zero exit).
 * Freestanding: __builtin_trap() raises #UD, routed by the IDT to the panic
 * path (PC LOAD LETTER + serial register dump). sft.c stays dependency-free (it
 * does not pull in panic.c/console.c) so it remains purely host-testable.
 * ------------------------------------------------------------------------ */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__
#include <stdlib.h> /* abort */
#define SFT_FAIL_LOUD() abort()
#else
#define SFT_FAIL_LOUD() __builtin_trap()
#endif

/* The kernel-global System File Table. BSS-zeroed -> all slots SFT_KIND_FREE
 * until sft_init() runs. */
sft_entry_t g_sft[SFT_MAX_ENTRIES];

/* Deterministic local zero-fill (sft.c is freestanding; no <string.h> memset
 * assumed). */
static void sft_zero(uint8_t *p, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        p[i] = 0;
    }
}

void sft_init(void)
{
    /* Start from a fully-FREE table so a re-init cannot leave a stale file
     * slot live. Every byte zero == SFT_KIND_FREE, ref_count 0, no file. */
    sft_zero((uint8_t *)g_sft, (uint32_t)sizeof(g_sft));

    /* Slot 0 -- stdin: CON device, read. One JFT reference (jft[0]). */
    g_sft[SFT_SLOT_CON_IN].kind      = SFT_KIND_DEVICE;
    g_sft[SFT_SLOT_CON_IN].dev_id    = SFT_DEV_CON;
    g_sft[SFT_SLOT_CON_IN].open_mode = SFT_MODE_READ;
    g_sft[SFT_SLOT_CON_IN].ref_count = 1u;

    /* Slot 1 -- stdout AND stderr: CON device, write. TWO JFT references
     * (jft[1] stdout, jft[2] stderr both point here -- the DOS convention that
     * stderr aliases stdout's SFT slot). */
    g_sft[SFT_SLOT_CON_OUT].kind      = SFT_KIND_DEVICE;
    g_sft[SFT_SLOT_CON_OUT].dev_id    = SFT_DEV_CON;
    g_sft[SFT_SLOT_CON_OUT].open_mode = SFT_MODE_WRITE;
    g_sft[SFT_SLOT_CON_OUT].ref_count = 2u;

    /* Slot 2 -- AUX (COM1): device, read/write. One JFT reference (jft[3]).
     * Vestigial this milestone (no COM driver) but predefined IN FULL per
     * DEC-06 "handles 0 through 4 ... auxiliary". */
    g_sft[SFT_SLOT_AUX].kind      = SFT_KIND_DEVICE;
    g_sft[SFT_SLOT_AUX].dev_id    = SFT_DEV_AUX;
    g_sft[SFT_SLOT_AUX].open_mode = SFT_MODE_RDWR;
    g_sft[SFT_SLOT_AUX].ref_count = 1u;

    /* Slot 3 -- PRN (LPT1): device, write. One JFT reference (jft[4]).
     * Vestigial this milestone (no LPT driver) but predefined IN FULL per
     * DEC-06 "... printer". */
    g_sft[SFT_SLOT_PRN].kind      = SFT_KIND_DEVICE;
    g_sft[SFT_SLOT_PRN].dev_id    = SFT_DEV_PRN;
    g_sft[SFT_SLOT_PRN].open_mode = SFT_MODE_WRITE;
    g_sft[SFT_SLOT_PRN].ref_count = 1u;

    /* Slots 4..19 remain SFT_KIND_FREE (available for file opens). */
}

sft_entry_t *sft_from_handle(const psp_t *psp, uint8_t handle)
{
    if (psp == 0 || handle >= JFT_MAX_ENTRIES) {
        return 0; /* out of range / no process -> caller returns invalid handle */
    }

    uint8_t idx = psp->jft[handle];
    if (idx == JFT_CLOSED) {
        return 0; /* legitimately closed handle -- not an error to probe */
    }

    /* A JFT entry that is neither 0xFF nor an in-range index of a LIVE slot is
     * corruption (Rule 2: fail loud, never return a plausible-but-wrong slot). */
    if (idx >= SFT_MAX_ENTRIES) {
        SFT_FAIL_LOUD();
        return 0; /* not reached */
    }
    if (g_sft[idx].kind == SFT_KIND_FREE) {
        SFT_FAIL_LOUD();
        return 0; /* not reached: a JFT must never reference a freed SFT slot */
    }

    return &g_sft[idx];
}

uint8_t jft_alloc(const psp_t *psp)
{
    if (psp == 0) {
        return JFT_CLOSED;
    }
    for (uint8_t i = 0; i < (uint8_t)JFT_MAX_ENTRIES; i++) {
        if (psp->jft[i] == JFT_CLOSED) {
            return i;
        }
    }
    return JFT_CLOSED; /* full */
}

uint8_t sft_alloc(void)
{
    /* Files never reuse the predefined device slots 0..3. */
    for (uint8_t i = (uint8_t)SFT_FIRST_FILE; i < (uint8_t)SFT_MAX_ENTRIES; i++) {
        if (g_sft[i].kind == SFT_KIND_FREE) {
            return i;
        }
    }
    return (uint8_t)SFT_MAX_ENTRIES; /* full */
}

/* Drop one JFT reference to SFT slot `idx`; free the slot (back to FREE) when
 * the last reference goes. Uniform for devices and files -- if a program
 * redirects both stdout and stderr away from CON the CON-write slot legitimately
 * has zero references and is freed (process load re-establishes the table).
 * Fails loud on a release of an already-free slot (Rule 2).
 *
 * __attribute__((unused)): the SFT_MUTATE_DUP2_NO_RELEASE mutant (Rule 6) elides
 * the sole call site; the attribute lets that mutant still COMPILE so the oracle
 * fails on the ASSERTION (released-slot ref drops), not on -Werror=unused. */
__attribute__((unused))
static void sft_release(uint8_t idx)
{
    if (idx >= SFT_MAX_ENTRIES || g_sft[idx].kind == SFT_KIND_FREE) {
        SFT_FAIL_LOUD();
        return; /* not reached */
    }
    if (g_sft[idx].ref_count > 0u) {
        g_sft[idx].ref_count--;
    }
    if (g_sft[idx].ref_count == 0u) {
        sft_zero((uint8_t *)&g_sft[idx], (uint32_t)sizeof(g_sft[idx]));
        /* kind is now SFT_KIND_FREE (0). */
    }
}

uint16_t sft_dup(psp_t *psp, uint8_t src, uint8_t *out_handle)
{
    sft_entry_t *e = sft_from_handle(psp, src);
    if (e == 0) {
        return SFT_ERR_INVALID_HANDLE;
    }

    uint8_t j = jft_alloc(psp);
    if (j == JFT_CLOSED) {
        return SFT_ERR_TOO_MANY_OPEN;
    }

    uint8_t sft_idx = psp->jft[src];
    psp->jft[j] = sft_idx;            /* the new handle aliases the same entry */
#ifndef SFT_MUTATE_DUP_NO_REFCOUNT
    /* Rule-6 mutant target: WITHOUT this increment, a later release/close
     * over-frees the shared slot. The oracle asserts the bump -> the mutant
     * (which omits it) goes RED. */
    g_sft[sft_idx].ref_count++;
#endif

    if (out_handle != 0) {
        *out_handle = j;
    }
    return SFT_OK;
}

uint16_t sft_dup2(psp_t *psp, uint8_t src, uint8_t dst)
{
    sft_entry_t *e = sft_from_handle(psp, src);
    if (e == 0) {
        return SFT_ERR_INVALID_HANDLE;
    }
    if (dst >= (uint8_t)JFT_MAX_ENTRIES) {
        return SFT_ERR_INVALID_HANDLE;
    }
    if (src == dst) {
        return SFT_OK; /* DOS: DUP2 onto itself is a success no-op */
    }

    /* If the target handle is already open, release its current SFT slot first
     * (the redirect overwrites it). */
    if (psp->jft[dst] != JFT_CLOSED) {
#ifndef SFT_MUTATE_DUP2_NO_RELEASE
        /* Rule-6 mutant target: WITHOUT this release the old target slot leaks
         * a reference (stays live forever). The oracle asserts the old slot's
         * ref_count drops -> the mutant (which omits it) goes RED. */
        sft_release(psp->jft[dst]);
#endif
    }

    uint8_t sft_idx = psp->jft[src];
    psp->jft[dst] = sft_idx;
    g_sft[sft_idx].ref_count++;
    return SFT_OK;
}

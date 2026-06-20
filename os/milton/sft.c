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

/* Runtime FILES= cap (beads initech-509.2). Default == SFT_MAX_ENTRIES so a
 * kernel/oracle that never runs SYSINIT keeps the full 16-file-slot table (no
 * existing gate regresses); SYSINIT lowers it from CONFIG.SYS via
 * sft_set_files_limit. NOT a BSS zero: a non-trivial initializer so a build that
 * forgets SYSINIT still has the permissive default rather than a 0 (which would
 * make every OPEN fail). */
uint8_t g_files_limit = (uint8_t)SFT_MAX_ENTRIES;

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
    /* Files never reuse the predefined device slots 0..3, AND never occupy a slot
     * at or above the FILES= cap (g_files_limit; beads initech-509.2). The cap is
     * clamped to [SFT_FIRST_FILE, SFT_MAX_ENTRIES] by sft_set_files_limit, so this
     * loop bound is always within the physical table. A request that finds no free
     * slot below the cap returns SFT_MAX_ENTRIES -- int21 do_open/do_creat map that
     * onto INT21_ERR_TOO_MANY_OPEN (DOS code 4), which is precisely "FILES=
     * exhausted". */
    uint8_t cap = g_files_limit;
    if (cap > (uint8_t)SFT_MAX_ENTRIES) {
        cap = (uint8_t)SFT_MAX_ENTRIES; /* defensive: never index past the table */
    }
    for (uint8_t i = (uint8_t)SFT_FIRST_FILE; i < cap; i++) {
        if (g_sft[i].kind == SFT_KIND_FREE) {
            return i;
        }
    }
    return (uint8_t)SFT_MAX_ENTRIES; /* full / FILES= cap reached */
}

uint8_t sft_set_files_limit(uint8_t files)
{
    /* Clamp to [SFT_FIRST_FILE, SFT_MAX_ENTRIES]: the device slots 0..3 are always
     * present; the cap never exceeds the physical table. A FILES= of 0 (or below
     * SFT_FIRST_FILE) leaves zero FILE slots but keeps the devices coherent. */
    if (files > (uint8_t)SFT_MAX_ENTRIES) {
        files = (uint8_t)SFT_MAX_ENTRIES;
    }
    if (files < (uint8_t)SFT_FIRST_FILE) {
        files = (uint8_t)SFT_FIRST_FILE;
    }
    g_files_limit = files;
    return g_files_limit;
}

/* Drop one JFT reference to SFT slot `idx`; free the slot (back to FREE) when
 * the last reference goes. Uniform for devices and files -- if a program
 * redirects both stdout and stderr away from CON the CON-write slot legitimately
 * has zero references and is freed (process load re-establishes the table).
 * Fails loud on a release of an already-free slot (Rule 2).
 *
 * Called by sft_dup2 (DUP2 redirect) AND sft_close_process (process EXIT
 * teardown, beads initech-6hk). The latter is an UNCONDITIONAL call site, so
 * sft_release is always reachable even when the SFT_MUTATE_DUP2_NO_RELEASE
 * mutant (Rule 6) elides the DUP2 call -- no __attribute__((unused)) needed. */
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

/* Release ALL of an exiting process's open FILE handles (beads initech-6hk; epic
 * initech-6qy). "Real DOS closes all a process's handles on terminate": without
 * this, a child that OPENs files and exits (4Ch / 00h / INT 20h) WITHOUT closing
 * them leaks SFT slots, and an EXEC chain (shell -> program -> ...) exhausts the
 * 16 file slots (SFT_FIRST_FILE..SFT_MAX_ENTRIES) so a later OPEN fails.
 *
 * Walk the process's JFT; for every entry that is OPEN (!= JFT_CLOSED) AND points
 * at a FILE-kind SFT slot, drop its SFT reference via sft_release (freeing the
 * slot at zero) and mark the JFT entry JFT_CLOSED.
 *
 * The predefined DEVICE slots 0..3 (CON/AUX/PRN) are RESIDENT -- established once
 * by sft_init at SYSINIT, shared across every process -- and are skipped here
 * (kind != SFT_KIND_FILE): releasing them would zero a device slot and leave the
 * next program with no stdin/stdout. We gate on kind == SFT_KIND_FILE (slots are
 * >= SFT_FIRST_FILE by construction; sft_alloc never hands out 0..3), so a
 * device-backed handle -- including one DUP2'd onto a device slot -- is left
 * untouched.
 *
 * Idempotent: a second call finds every FILE handle already JFT_CLOSED -> no-op.
 * Fails LOUD on a corrupt JFT entry (neither the 0xFF sentinel nor an in-range
 * index of a live slot), consistent with sft_from_handle (Rule 2). psp == NULL
 * is a no-op. Ref: DOS 3.3 process terminate; beads initech-6hk; sft.h. */
void sft_close_process(psp_t *psp)
{
    if (psp == 0) {
        return; /* no process -> nothing to reclaim */
    }

    for (uint8_t h = 0; h < (uint8_t)JFT_MAX_ENTRIES; h++) {
        uint8_t idx = psp->jft[h];
        if (idx == JFT_CLOSED) {
            continue; /* closed/unused handle */
        }

        /* A JFT entry that is neither 0xFF nor an in-range index of a LIVE slot
         * is corruption -- fail loud, never zero a plausible-but-wrong slot
         * (Rule 2; mirrors sft_from_handle). */
        if (idx >= SFT_MAX_ENTRIES || g_sft[idx].kind == SFT_KIND_FREE) {
            SFT_FAIL_LOUD();
            return; /* not reached */
        }

        /* Reclaim the process's OWN handles: FILE slots, AND OPEN-by-name DEVICE
         * slots (beads initech-6zd9 -- an AH=3Dh OPEN of "NUL"/"CON"/etc. binds a
         * SFT_KIND_DEVICE slot at index >= SFT_FIRST_FILE, exactly like a file, and
         * leaks identically if not released on exit). The RESIDENT device slots
         * 0..3 (CON/AUX/PRN, index < SFT_FIRST_FILE) are shared/live across every
         * program and are NEVER touched -- releasing them would leave the next
         * program with no stdin/stdout. sft_alloc only ever hands out slots
         * >= SFT_FIRST_FILE, so the index test cleanly separates the two: a
         * process-owned slot (FILE or OPEN-by-name DEVICE) is at/above the floor;
         * a resident device slot is below it. */
        if (idx < (uint8_t)SFT_FIRST_FILE) {
            continue;   /* resident CON/AUX/PRN device slot -- never reclaimed */
        }
        if (g_sft[idx].kind != SFT_KIND_FILE &&
            g_sft[idx].kind != SFT_KIND_DEVICE) {
            continue;   /* defensive: only file/device kinds are reclaimable */
        }

#ifndef SFT_MUTATE_NO_CLOSE_PROCESS
        /* Rule-6 mutant target (make test-exit-handles-mutant): WITHOUT this
         * release the exiting process's FILE slots leak -- an EXEC chain of a
         * leaky child exhausts the 16 file slots and a later OPEN fails. The
         * in-emulator oracle goes RED. NEVER define in a real build. */
        sft_release(idx);
        psp->jft[h] = JFT_CLOSED;
#endif
    }
}

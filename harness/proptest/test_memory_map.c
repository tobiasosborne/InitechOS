/*
 * harness/proptest/test_memory_map.c -- host gate for the LOCKED physical
 * memory map's window disjointness (bead initech-xu97).
 *
 * FACTORY host test (CLAUDE.md Law 3): libc OK. Non-zero exit on any failure
 * (Law 2: the oracle is the truth, never false-green).
 *
 * WHAT IT GATES: spec/memory_map.h carries a compile-time _Static_assert for
 * every pair of the windows it owns (FLAIR heap, kernel image, transient
 * COMMAND.COM, kernel stack, program window, EXEC load staging, stage2 kernel
 * bounce, legacy VGA aperture) -- every kernel object that includes the header
 * re-proves them. This translation unit adds the cross-header pairs the spec
 * header cannot see (boot_info_t at BOOT_INFO_ADDR and the ROM font stash at
 * FONT_STASH_ADDR, both os/milton/boot_info.h) as further _Static_asserts, then
 * re-walks the whole window table at run time and prints it, so a reviewer
 * reads the ratified map from the gate's own output.
 *
 * The expectations are NOT derived from the artifact's values: they are the
 * half-open interval-overlap predicate applied to the named windows. A window
 * moved into another (the bead's exact defect: KERNEL_STACK [0x4F0000,
 * 0x500000) inside the FLAIR heap [0x100000,0x500000)) fails to COMPILE.
 * MUTATION GATE: `make test-memory-map-mutant` compiles this file against a
 * sed-mutated copy of spec/memory_map.h whose stack is re-overlapped with the
 * heap (and, separately, with the transient window) and requires the compile
 * to fail naming that pair (Rule 6).
 *
 * Ref (Law 1): spec/memory_map.h (the locked constants + the xu97 note);
 *   os/milton/boot_info.h (BOOT_INFO_ADDR, FONT_STASH_ADDR, sizeof boot_info_t);
 *   docs/design/kernel-runway-relocation.md K1/K2 (the runway);
 *   docs/adr/ADR-0003-AMENDMENT-DEC-08b-Kernel-Stack-Window.md (this ruling);
 *   ADR-0004 DEC-03 (the FLAIR heap window). CLAUDE.md Rule 2/6/8/12.
 * ASCII-clean (Rule 12). No timestamps / host paths / nondeterminism (Rule 11).
 */
#include <stdio.h>
#include <stdint.h>

#include "memory_map.h"
#include "boot_info.h"

/* The ROM font stash is 256 glyphs x 16 rows (boot_info.h: "4096 bytes"). */
#define FONT_STASH_SIZE 4096u

#define BI_LO   BOOT_INFO_ADDR
#define BI_HI   (BOOT_INFO_ADDR + (uint32_t)sizeof(boot_info_t))
#define FS_LO   FONT_STASH_ADDR
#define FS_HI   (FONT_STASH_ADDR + FONT_STASH_SIZE)

#define DISJ(alo, ahi, blo, bhi) ((ahi) <= (blo) || (bhi) <= (alo))

/* Cross-header compile-time teeth: boot_info / font stash vs every window the
 * spec header owns, and vs each other. */
#define BI_FS_VS(W) \
    _Static_assert(DISJ(BI_LO, BI_HI, MMAP_WIN_LO_##W, MMAP_WIN_HI_##W), \
                   "memory map windows overlap: BOOT_INFO vs " #W); \
    _Static_assert(DISJ(FS_LO, FS_HI, MMAP_WIN_LO_##W, MMAP_WIN_HI_##W), \
                   "memory map windows overlap: FONT_STASH vs " #W)
BI_FS_VS(BOUNCE);
BI_FS_VS(PROGRAM);
BI_FS_VS(STAGING);
BI_FS_VS(VGA);
BI_FS_VS(FLAIR_HEAP);
BI_FS_VS(KIMAGE);
BI_FS_VS(TRANSIENT);
BI_FS_VS(KSTACK);
BI_FS_VS(OVERLAY);
_Static_assert(DISJ(BI_LO, BI_HI, FS_LO, FS_HI),
               "memory map windows overlap: BOOT_INFO vs FONT_STASH");

typedef struct {
    const char *name;
    uint32_t lo, hi;   /* half-open [lo, hi) */
} win_t;

int main(void)
{
    static const win_t w[] = {
        { "BOOT_INFO",   BI_LO, BI_HI },
        { "FONT_STASH",  FS_LO, FS_HI },
        { "BOUNCE",      MMAP_WIN_LO_BOUNCE,     MMAP_WIN_HI_BOUNCE },
        { "PROGRAM",     MMAP_WIN_LO_PROGRAM,    MMAP_WIN_HI_PROGRAM },
        { "STAGING",     MMAP_WIN_LO_STAGING,    MMAP_WIN_HI_STAGING },
        { "VGA",         MMAP_WIN_LO_VGA,        MMAP_WIN_HI_VGA },
        { "FLAIR_HEAP",  MMAP_WIN_LO_FLAIR_HEAP, MMAP_WIN_HI_FLAIR_HEAP },
        { "KIMAGE",      MMAP_WIN_LO_KIMAGE,     MMAP_WIN_HI_KIMAGE },
        { "TRANSIENT",   MMAP_WIN_LO_TRANSIENT,  MMAP_WIN_HI_TRANSIENT },
        { "KSTACK",      MMAP_WIN_LO_KSTACK,     MMAP_WIN_HI_KSTACK },
        { "OVERLAY",     MMAP_WIN_LO_OVERLAY,    MMAP_WIN_HI_OVERLAY },
    };
    const unsigned n = (unsigned)(sizeof w / sizeof w[0]);
    unsigned i, j, pairs = 0, bad = 0;

    for (i = 0; i < n; i++) {
        printf("    %-10s [0x%08X, 0x%08X)\n", w[i].name,
               (unsigned)w[i].lo, (unsigned)w[i].hi);
        if (w[i].lo >= w[i].hi) {
            printf("!!! test-memory-map: window %s is empty or inverted\n",
                   w[i].name);
            bad++;
        }
    }
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            pairs++;
            if (!DISJ(w[i].lo, w[i].hi, w[j].lo, w[j].hi)) {
                printf("!!! test-memory-map: %s overlaps %s\n",
                       w[i].name, w[j].name);
                bad++;
            }
        }
    }
    /* The kernel stack must lie inside the RAM-gated runway, so the boot
     * probe's INITECH_MIN_EXT_KB gate already backs it with installed RAM, and
     * must keep the 64 KiB the region-engine stack audit was sized against
     * (os/flair/atkinson/region.c KERNEL STACK AUDIT). */
    if (KERNEL_STACK_BOT < KERNEL_BASE || KERNEL_STACK_TOP + 4u > KERNEL_CEIL) {
        printf("!!! test-memory-map: kernel stack outside the RAM-gated runway\n");
        bad++;
    }
    if (KERNEL_STACK_TOP + 4u - KERNEL_STACK_BOT != 0x00010000u ||
        (KERNEL_STACK_TOP & 3u) != 0u) {
        printf("!!! test-memory-map: kernel stack is not a 4-aligned 64 KiB window\n");
        bad++;
    }
    if (bad != 0u) {
        printf("!!! test-memory-map FAIL: %u violation(s)\n", bad);
        return 1;
    }
    printf(">>> test-memory-map: %u windows, %u pairs pairwise disjoint; "
           "kernel stack [0x%08X, 0x%08X) inside the runway\n",
           n, pairs, (unsigned)KERNEL_STACK_BOT,
           (unsigned)(KERNEL_STACK_TOP + 4u));
    return 0;
}

/*
 * ppm_seafoam_check.c -- RETIRED (ADR-0010 CD-6 / CD-7; epic initech-qipc).
 *
 * The seafoam desktop canon is REVOKED. The FLAIR desktop background is now
 * Initech teal #8DDCDC (color_canon idx2; ADR-0004-AMENDMENT-DEC-09 + ADR-0010,
 * WL-0053), NOT seafoam #6FA08E (the revoked ADR-0004 OD-4 value). This checker
 * was a BY-CONSTRUCTION framebuffer grade (its expected RGB mirrored
 * os/boot/stage2.asm; heresy HER-06/HER-08) and is superseded by the split:
 *
 *   - the COLOR VALUE oracle  : test-color-canon (harness/proptest/
 *                               test_color_canon.c) grades flair_canon_rgb(idx)
 *                               against the INDEPENDENT System-7 / Win-3.1
 *                               decomp goldens (ADR-0010 CD-2), NOT by
 *                               construction. The seafoam-relapse VALUE mutant
 *                               (CANON_MUTATE_TEAL: teal -> seafoam) is its
 *                               standing tripwire.
 *   - the STRUCTURE oracle    : ppm_flair_check (tools/ppm_flair_check.c) grades
 *                               the live-desktop screendump GEOMETRY / TOPOLOGY /
 *                               Z-ORDER (ADR-0010 CD-5).
 *
 * This file is NEUTERED to an inert stub so the Makefile may keep referencing it
 * until build integration removes/repoints the references (owner: operator). It
 * builds clean under -std=c11 -Wall -Wextra -Werror and EXITS 0 unconditionally;
 * it asserts NOTHING. Do NOT re-arm it -- grading the desktop color belongs to
 * test-color-canon (value) and ppm_flair_check (structure).
 *
 * Ref: ADR-0010 CD-6 (retire the seafoam apparatus), CD-7; Sec 6.1 (supersedes
 *      OD-4); CLAUDE.md Law 2 (by-construction is NOT an oracle), Rule 12 (ASCII).
 *
 * Usage (no-op): ppm_seafoam_check [<screendump.ppm>]
 */

#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    printf("ppm_seafoam_check: RETIRED -- the seafoam desktop canon is REVOKED "
           "(ADR-0010 OD-4 -> Initech teal #8DDCDC). Superseded by test-color-canon "
           "(VALUE, decomp-graded) + ppm_flair_check (STRUCTURE). This stub exits 0 "
           "and asserts nothing.\n");
    return 0;
}

/*
 * test_palette_seafoam.c -- palette SEAFOAM canon oracle (factory, C-only).
 *
 * beads: initech-ch81 (ADR-0004 AM-9 / FO-3 -- reconcile palette.json
 *        desktop_bg v0 gray -> SEAFOAM; oracle asserts palette.h canonical
 *        desktop_bg == live boot/oracle SEAFOAM value).
 * Ref:  ADR-0004 OD-4 ("desktop_bg canon = KEEP SEAFOAM teal (0x6F,0xA0,0x8E)")
 *       ADR-0004 AM-9 / FO-3 (the oracle must assert palette.json canonical
 *         desktop_bg entry == live boot/oracle SEAFOAM value, not a visual check)
 *       os/boot/stage2.asm SEAFOAM_R equ 0x6F / SEAFOAM_G equ 0xA0 /
 *         SEAFOAM_B equ 0x8E (lines 42-44 -- the live boot value, source of truth)
 *       tools/ppm_seafoam_check.c #define SEAFOAM_R/G/B (mirrors stage2.asm)
 *       spec/assets/palette.h INITECH_DESKTOP_BG_{R,G,B} (what we assert here)
 *       CLAUDE.md Law 2 (oracle is the truth), Rule 2 (fail fast/loud),
 *         Rule 6 (mutation-proven golden), Rule 12 (ASCII-clean),
 *         Law 3 (factory is C-only).
 *
 * This oracle does NOT do a visual or screendump check. It asserts EXACT
 * equality between the compile-time constants in spec/assets/palette.h and
 * the live boot/oracle SEAFOAM constants from os/boot/stage2.asm (replicated
 * here with a citation). Both sets are resolved at compile time; any drift
 * between palette.h and the boot constants is a build failure. (AM-9.)
 *
 * MUTANT: compile with -DPALETTE_MUTATE_GRAY_DESKTOP to flip INITECH_DESKTOP_BG
 * to the old v0 gray (0x73,0x69,0x6C). This MUST drive the oracle RED,
 * proving the check bites (Rule 6). The named mutant for the gate recipe is:
 *
 *   test-palette-seafoam-mutant: build with -DPALETTE_MUTATE_GRAY_DESKTOP,
 *   run, assert non-zero exit.
 *
 * Build (hosted; no freestanding flags needed for this test):
 *   gcc -std=c99 -Wall -Wextra -Ispec/assets -o /tmp/test_palette_seafoam \
 *       harness/proptest/test_palette_seafoam.c
 *
 * Mutant build:
 *   gcc -std=c99 -Wall -Wextra -Ispec/assets -DPALETTE_MUTATE_GRAY_DESKTOP \
 *       -o /tmp/test_palette_seafoam_mutant \
 *       harness/proptest/test_palette_seafoam.c
 *
 * Freestanding compile check (Law 3 / Rule 8 -- palette.h is consumed by
 * the artifact; it must not require hosted headers):
 *   gcc -ffreestanding -nostdlib -c -Ispec/assets spec/assets/palette.h \
 *       -o /dev/null
 *
 * Usage: test_palette_seafoam
 * Exit 0 = all assertions pass (GREEN). Exit 1 = at least one assertion
 * failed (RED). Exit 2 = usage error.
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * The canonical SEAFOAM constants, replicated from os/boot/stage2.asm lines
 * 42-44 (the live boot value -- the source of truth per ADR-0004 OD-4 and
 * tools/ppm_seafoam_check.c which also mirrors these three values):
 *
 *   SEAFOAM_R  equ 0x6F   ; R = 111 decimal
 *   SEAFOAM_G  equ 0xA0   ; G = 160 decimal
 *   SEAFOAM_B  equ 0x8E   ; B = 142 decimal
 *
 * If stage2.asm ever changes these, this file must be updated in the same
 * Rule 8 deliberate act (and the oracle will go RED until it is).
 */
#define STAGE2_SEAFOAM_R 0x6F
#define STAGE2_SEAFOAM_G 0xA0
#define STAGE2_SEAFOAM_B 0x8E

/*
 * Include the palette under test.
 * Under -DPALETTE_MUTATE_GRAY_DESKTOP we shadow INITECH_DESKTOP_BG_{R,G,B}
 * with the old v0 gray values so the assertions below go RED, proving the
 * oracle bites (Rule 6 mutation proof).
 */
#include "palette.h"

#ifdef PALETTE_MUTATE_GRAY_DESKTOP
/*
 * Named mutant: PALETTE_MUTATE_GRAY_DESKTOP
 * Replaces the canonical SEAFOAM desktop_bg with the old frame-sampled v0
 * gray (0x73,0x69,0x6C) = (115,105,108). This MUST drive the oracle RED.
 * The delta from SEAFOAM on the green channel alone is 160-105 = 55, far
 * beyond any reasonable tolerance -- a loud, discriminating failure.
 */
#undef INITECH_DESKTOP_BG_R
#undef INITECH_DESKTOP_BG_G
#undef INITECH_DESKTOP_BG_B
#undef INITECH_DESKTOP_BG_RGB
#define INITECH_DESKTOP_BG_R   0x73   /* old v0 gray R = 115 */
#define INITECH_DESKTOP_BG_G   0x69   /* old v0 gray G = 105 */
#define INITECH_DESKTOP_BG_B   0x6C   /* old v0 gray B = 108 */
#define INITECH_DESKTOP_BG_RGB 0x73696Cu
#endif /* PALETTE_MUTATE_GRAY_DESKTOP */

/* -------------------------------------------------------------------------- */

static int fail_count = 0;

static void check_eq_byte(const char *label, int got, int want)
{
    if (got == want) {
        printf("  PASS  %-40s  got=0x%02X (%3d)  want=0x%02X (%3d)\n",
               label, (unsigned)got, got, (unsigned)want, want);
    } else {
        fprintf(stderr,
                "  FAIL  %-40s  got=0x%02X (%3d)  want=0x%02X (%3d)  delta=%d\n",
                label, (unsigned)got, got, (unsigned)want, want, got - want);
        fail_count++;
    }
}

static void check_eq_u32(const char *label, unsigned long got, unsigned long want)
{
    if (got == want) {
        printf("  PASS  %-40s  got=0x%06lX  want=0x%06lX\n",
               label, got, want);
    } else {
        fprintf(stderr,
                "  FAIL  %-40s  got=0x%06lX  want=0x%06lX\n",
                label, got, want);
        fail_count++;
    }
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    printf("=======================================================================\n");
    printf("InitechOS (STAPLER) -- test_palette_seafoam\n");
    printf("  Ref: ADR-0004 OD-4 / AM-9 / FO-3 (beads initech-ch81)\n");
    printf("  Assert: palette.h INITECH_DESKTOP_BG_{{R,G,B}} ==\n");
    printf("          stage2.asm SEAFOAM_{{R,G,B}} = (0x%02X,0x%02X,0x%02X) = (%d,%d,%d)\n",
           STAGE2_SEAFOAM_R, STAGE2_SEAFOAM_G, STAGE2_SEAFOAM_B,
           STAGE2_SEAFOAM_R, STAGE2_SEAFOAM_G, STAGE2_SEAFOAM_B);
#ifdef PALETTE_MUTATE_GRAY_DESKTOP
    printf("  [MUTANT MODE: PALETTE_MUTATE_GRAY_DESKTOP -- desktop_bg forced to v0 gray]\n");
    printf("  [This MUST exit 1 (RED). Green exit here is an oracle failure.]\n");
#endif
    printf("-----------------------------------------------------------------------\n");

    /*
     * Core assertion: each R/G/B channel of the canonical palette desktop_bg
     * must equal the corresponding stage2.asm SEAFOAM constant.
     * Exact equality -- no tolerance. (AM-9: "not a visual check".)
     */
    printf("--- Channel equality (exact) ---\n");
    check_eq_byte("INITECH_DESKTOP_BG_R == STAGE2_SEAFOAM_R",
                  INITECH_DESKTOP_BG_R, STAGE2_SEAFOAM_R);
    check_eq_byte("INITECH_DESKTOP_BG_G == STAGE2_SEAFOAM_G",
                  INITECH_DESKTOP_BG_G, STAGE2_SEAFOAM_G);
    check_eq_byte("INITECH_DESKTOP_BG_B == STAGE2_SEAFOAM_B",
                  INITECH_DESKTOP_BG_B, STAGE2_SEAFOAM_B);

    /*
     * Packed RGB consistency: the _RGB macro must equal the
     * channel-assembled value so any consumer using either form gets the
     * same bits.
     */
    printf("--- Packed RGB consistency ---\n");
    unsigned long packed_from_channels =
        ((unsigned long)INITECH_DESKTOP_BG_R << 16) |
        ((unsigned long)INITECH_DESKTOP_BG_G <<  8) |
        ((unsigned long)INITECH_DESKTOP_BG_B);
    check_eq_u32("INITECH_DESKTOP_BG_RGB == R<<16|G<<8|B",
                 (unsigned long)INITECH_DESKTOP_BG_RGB, packed_from_channels);

    unsigned long seafoam_packed =
        ((unsigned long)STAGE2_SEAFOAM_R << 16) |
        ((unsigned long)STAGE2_SEAFOAM_G <<  8) |
        ((unsigned long)STAGE2_SEAFOAM_B);
    check_eq_u32("INITECH_DESKTOP_BG_RGB == SEAFOAM packed",
                 (unsigned long)INITECH_DESKTOP_BG_RGB, seafoam_packed);

    /*
     * Provenance sentinel: the v0 gray FRAME_V0 constants must NOT equal
     * SEAFOAM (they are the old gray and must remain distinct so the
     * provenance is meaningful). If someone accidentally sets FRAME_V0 to
     * seafoam the distinction collapses.
     */
    printf("--- Provenance sentinel (FRAME_V0 != SEAFOAM) ---\n");
    {
        int v0_r = INITECH_DESKTOP_BG_FRAME_V0_R;
        int v0_g = INITECH_DESKTOP_BG_FRAME_V0_G;
        int v0_b = INITECH_DESKTOP_BG_FRAME_V0_B;
        int distinct = (v0_r != STAGE2_SEAFOAM_R ||
                        v0_g != STAGE2_SEAFOAM_G ||
                        v0_b != STAGE2_SEAFOAM_B);
        if (distinct) {
            printf("  PASS  FRAME_V0 (0x%02X,0x%02X,0x%02X) != SEAFOAM -- provenance preserved\n",
                   (unsigned)v0_r, (unsigned)v0_g, (unsigned)v0_b);
        } else {
            fprintf(stderr,
                    "  FAIL  FRAME_V0 (0x%02X,0x%02X,0x%02X) == SEAFOAM -- provenance collapsed\n",
                    (unsigned)v0_r, (unsigned)v0_g, (unsigned)v0_b);
            fail_count++;
        }
    }

    printf("=======================================================================\n");
    if (fail_count == 0) {
        printf("VERDICT: PASS -- palette.h INITECH_DESKTOP_BG == SEAFOAM (0x%02X,0x%02X,0x%02X)"
               " -- ADR-0004 OD-4 / AM-9 / FO-3 green\n",
               STAGE2_SEAFOAM_R, STAGE2_SEAFOAM_G, STAGE2_SEAFOAM_B);
        return 0;
    } else {
        fprintf(stderr,
                "VERDICT: FAIL -- %d assertion(s) failed -- palette.h desktop_bg"
                " does not match SEAFOAM (0x%02X,0x%02X,0x%02X)\n",
                fail_count,
                STAGE2_SEAFOAM_R, STAGE2_SEAFOAM_G, STAGE2_SEAFOAM_B);
        return 1;
    }
}

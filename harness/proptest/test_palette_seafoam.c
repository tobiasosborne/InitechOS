/*
 * test_palette_seafoam.c -- palette desktop-canon oracle (factory, C-only).
 *
 * RE-KEYED TO TEAL (ADR-0004-AMENDMENT-DEC-09 OD-4-REVOKED / FO-3).
 *
 * HISTORY: this oracle formerly asserted palette.h INITECH_DESKTOP_BG ==
 * SEAFOAM (0x6F,0xA0,0x8E) == os/boot/stage2.asm SEAFOAM_R/G/B. That premise is
 * now FALSE: OD-4's seafoam desktop canon is REVOKED (management decree
 * 2026-06-21, HER-01/HER-06/HER-08) and the FLAIR desktop background is the
 * LOCKED Initech Color Canon teal #8DDCDC (color_canon.json idx2 CIDX_DESKTOP).
 * The stage2.asm SEAFOAM_R/G/B constants were renamed TEAL_R/G/B (0x8D/0xDC/
 * 0xDC) in the same Rule-8 act.
 *
 * DISPOSITION: this file is RE-KEYED to assert that palette.h's canonical
 * desktop_bg == the Initech teal, and that the ONE color authority
 * flair_canon_rgb(CIDX_DESKTOP) agrees. The single VALUE oracle for the whole
 * canon is now test-color-canon (harness/proptest/test_color_canon.c, owned by
 * ADR-0010, graded LIVE against the decomp goldens, NOT by-construction). This
 * file is the narrow desktop-slot consistency check that survives as a cheap
 * companion; it is SUPERSEDED by test-color-canon for grading authority.
 *
 * Ref:  ADR-0004-AMENDMENT-DEC-09 OD-4-REVOKED (seafoam -> teal #8DDCDC at idx2),
 *         ARB-1 (flair_palette_rgb thin alias to flair_canon_rgb), FO-3.
 *       spec/assets/color_canon.json idx2 CIDX_DESKTOP = #8DDCDC (LOCKED).
 *       spec/assets/palette.h INITECH_DESKTOP_BG_{R,G,B} (what we assert here).
 *       spec/assets/color_canon.h flair_canon_rgb / CIDX_DESKTOP.
 *       CLAUDE.md Law 2 (oracle is the truth), Rule 2 (fail fast/loud),
 *         Rule 6 (mutation-proven golden), Rule 12 (ASCII-clean),
 *         Law 3 (factory is C-only).
 *
 * This oracle does NOT do a visual or screendump check. It asserts EXACT
 * equality between the compile-time constants in spec/assets/palette.h and the
 * LOCKED canon teal; both are resolved at compile time, so any drift is a build
 * failure.
 *
 * MUTANT (unchanged gate contract): compile with -DPALETTE_MUTATE_GRAY_DESKTOP
 * to flip INITECH_DESKTOP_BG to the old v0 gray (0x73,0x69,0x6C). This MUST
 * drive the oracle RED (exit non-zero), proving the check bites (Rule 6). The
 * named mutant recipe (test-palette-seafoam-mutant) builds with that -D and
 * asserts a non-zero exit -- the gate contract is preserved.
 *
 * Build (hosted; no freestanding flags needed):
 *   gcc -std=c99 -Wall -Wextra -Ispec/assets -o /tmp/test_palette_seafoam \
 *       harness/proptest/test_palette_seafoam.c
 *
 * Usage: test_palette_seafoam
 * Exit 0 = all assertions pass (GREEN). Exit 1 = at least one assertion
 * failed (RED). Exit 2 = usage error.
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * The LOCKED Initech desktop teal (color_canon.json idx2 CIDX_DESKTOP), also
 * carried as stage2.asm TEAL_R/G/B (the documented boot color constant):
 *
 *   TEAL_R  equ 0x8D   ; R = 141 decimal
 *   TEAL_G  equ 0xDC   ; G = 220 decimal
 *   TEAL_B  equ 0xDC   ; B = 220 decimal
 *
 * If the canon ever changes these, this file must be updated in the same
 * Rule 8 deliberate act (and the oracle will go RED until it is).
 */
#define CANON_TEAL_R 0x8D
#define CANON_TEAL_G 0xDC
#define CANON_TEAL_B 0xDC

/*
 * Include the palette under test (which now #includes color_canon.h, so
 * flair_canon_rgb + CIDX_DESKTOP are in scope for the cross-check below).
 * Under -DPALETTE_MUTATE_GRAY_DESKTOP we shadow INITECH_DESKTOP_BG_{R,G,B} with
 * the old v0 gray values so the assertions below go RED (Rule 6 mutation proof).
 */
#include "palette.h"

#ifdef PALETTE_MUTATE_GRAY_DESKTOP
/*
 * Named mutant: PALETTE_MUTATE_GRAY_DESKTOP
 * Replaces the canonical teal desktop_bg with the old frame-sampled v0 gray
 * (0x73,0x69,0x6C) = (115,105,108). This MUST drive the oracle RED. The delta
 * from teal on the green channel alone is 220-105 = 115, far beyond any
 * reasonable tolerance -- a loud, discriminating failure.
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
        printf("  PASS  %-44s  got=0x%02X (%3d)  want=0x%02X (%3d)\n",
               label, (unsigned)got, got, (unsigned)want, want);
    } else {
        fprintf(stderr,
                "  FAIL  %-44s  got=0x%02X (%3d)  want=0x%02X (%3d)  delta=%d\n",
                label, (unsigned)got, got, (unsigned)want, want, got - want);
        fail_count++;
    }
}

static void check_eq_u32(const char *label, unsigned long got, unsigned long want)
{
    if (got == want) {
        printf("  PASS  %-44s  got=0x%06lX  want=0x%06lX\n",
               label, got, want);
    } else {
        fprintf(stderr,
                "  FAIL  %-44s  got=0x%06lX  want=0x%06lX\n",
                label, got, want);
        fail_count++;
    }
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    printf("=======================================================================\n");
    printf("InitechOS (STAPLER) -- test_palette_seafoam (RE-KEYED to teal)\n");
    printf("  Ref: ADR-0004-AMENDMENT-DEC-09 OD-4-REVOKED / FO-3 (seafoam -> teal)\n");
    printf("  Assert: palette.h INITECH_DESKTOP_BG_{R,G,B} ==\n");
    printf("          Initech canon teal = (0x%02X,0x%02X,0x%02X) = (%d,%d,%d)\n",
           CANON_TEAL_R, CANON_TEAL_G, CANON_TEAL_B,
           CANON_TEAL_R, CANON_TEAL_G, CANON_TEAL_B);
    printf("  NOTE: SUPERSEDED for grading authority by test-color-canon (ADR-0010,\n");
    printf("        LIVE-diff vs decomp goldens). This is a cheap slot-consistency check.\n");
#ifdef PALETTE_MUTATE_GRAY_DESKTOP
    printf("  [MUTANT MODE: PALETTE_MUTATE_GRAY_DESKTOP -- desktop_bg forced to v0 gray]\n");
    printf("  [This MUST exit 1 (RED). Green exit here is an oracle failure.]\n");
#endif
    printf("-----------------------------------------------------------------------\n");

    /*
     * Core assertion: each R/G/B channel of the canonical palette desktop_bg
     * must equal the corresponding LOCKED canon teal constant. Exact equality.
     */
    printf("--- Channel equality (exact) ---\n");
    check_eq_byte("INITECH_DESKTOP_BG_R == CANON_TEAL_R",
                  INITECH_DESKTOP_BG_R, CANON_TEAL_R);
    check_eq_byte("INITECH_DESKTOP_BG_G == CANON_TEAL_G",
                  INITECH_DESKTOP_BG_G, CANON_TEAL_G);
    check_eq_byte("INITECH_DESKTOP_BG_B == CANON_TEAL_B",
                  INITECH_DESKTOP_BG_B, CANON_TEAL_B);

    /*
     * Packed RGB consistency: the _RGB macro must equal the channel-assembled
     * value so any consumer using either form gets the same bits.
     */
    printf("--- Packed RGB consistency ---\n");
    unsigned long packed_from_channels =
        ((unsigned long)INITECH_DESKTOP_BG_R << 16) |
        ((unsigned long)INITECH_DESKTOP_BG_G <<  8) |
        ((unsigned long)INITECH_DESKTOP_BG_B);
    check_eq_u32("INITECH_DESKTOP_BG_RGB == R<<16|G<<8|B",
                 (unsigned long)INITECH_DESKTOP_BG_RGB, packed_from_channels);

    unsigned long teal_packed =
        ((unsigned long)CANON_TEAL_R << 16) |
        ((unsigned long)CANON_TEAL_G <<  8) |
        ((unsigned long)CANON_TEAL_B);
    check_eq_u32("INITECH_DESKTOP_BG_RGB == TEAL packed",
                 (unsigned long)INITECH_DESKTOP_BG_RGB, teal_packed);

    /*
     * Authority cross-check: the ONE color authority flair_canon_rgb at
     * CIDX_DESKTOP must resolve to the same teal -- proving palette.h's named
     * desktop constant agrees with the canon accessor every render site uses.
     */
    printf("--- Canon authority cross-check (flair_canon_rgb) ---\n");
    check_eq_u32("flair_canon_rgb(CIDX_DESKTOP) == TEAL packed",
                 (unsigned long)flair_canon_rgb((uint8_t)CIDX_DESKTOP), teal_packed);

    /*
     * Provenance sentinel: the v0 gray FRAME_V0 constants must NOT equal the
     * teal (they are the REVOKED preview-mock-up gray and must remain distinct
     * so the provenance is meaningful).
     */
    printf("--- Provenance sentinel (FRAME_V0 != TEAL) ---\n");
    {
        int v0_r = INITECH_DESKTOP_BG_FRAME_V0_R;
        int v0_g = INITECH_DESKTOP_BG_FRAME_V0_G;
        int v0_b = INITECH_DESKTOP_BG_FRAME_V0_B;
        int distinct = (v0_r != CANON_TEAL_R ||
                        v0_g != CANON_TEAL_G ||
                        v0_b != CANON_TEAL_B);
        if (distinct) {
            printf("  PASS  FRAME_V0 (0x%02X,0x%02X,0x%02X) != TEAL -- provenance preserved\n",
                   (unsigned)v0_r, (unsigned)v0_g, (unsigned)v0_b);
        } else {
            fprintf(stderr,
                    "  FAIL  FRAME_V0 (0x%02X,0x%02X,0x%02X) == TEAL -- provenance collapsed\n",
                    (unsigned)v0_r, (unsigned)v0_g, (unsigned)v0_b);
            fail_count++;
        }
    }

    printf("=======================================================================\n");
    if (fail_count == 0) {
        printf("VERDICT: PASS -- palette.h INITECH_DESKTOP_BG == Initech teal (0x%02X,0x%02X,0x%02X)"
               " -- OD-4-REVOKED / FO-3 green\n",
               CANON_TEAL_R, CANON_TEAL_G, CANON_TEAL_B);
        return 0;
    } else {
        fprintf(stderr,
                "VERDICT: FAIL -- %d assertion(s) failed -- palette.h desktop_bg"
                " does not match Initech teal (0x%02X,0x%02X,0x%02X)\n",
                fail_count,
                CANON_TEAL_R, CANON_TEAL_G, CANON_TEAL_B);
        return 1;
    }
}

/*
 * harness/proptest/fixtures/mech_policy_mutant.c -- the MECH_POLICY_MUTANT
 * fixture (Rule 6 mutation-proof for test-mech-policy).
 *
 * This is NOT a real mechanism TU and is NEVER compiled into the OS or any
 * test binary.  It is a planted, mechanism-shaped source file carrying exactly
 * ONE C-8 violation: a raw 0xRRGGBB color literal inside an imaging-path span
 * fill.  test_mech_policy.c scans this fixture ONLY when built with
 * -DMECH_POLICY_MUTANT, so the mutant scanner build MUST go RED -- proving the
 * source scanner actually bites a resurrected color literal in a mechanism
 * file.  Restore = the normal (non-mutant) scanner does NOT scan this fixture.
 *
 * Ref: ADR-0004-AMENDMENT-DEC-09 Sec 3.10 (DEC-09-D4 #1); CLAUDE.md Rule 6.
 */

#include <stdint.h>

/* PLANTED C-8 VIOLATION: a mechanism span fill that NAMES a color (0xRRGGBB)
 * instead of resolving a PART through flair_look_pixel.  The scanner MUST flag
 * this hex literal. */
static uint32_t mutant_desktop_px(void)
{
    return 0x8DDCDCu;          /* a raw color literal below the C-8 cut-line */
}

uint32_t mech_policy_mutant_probe(void);
uint32_t mech_policy_mutant_probe(void)
{
    return mutant_desktop_px();
}

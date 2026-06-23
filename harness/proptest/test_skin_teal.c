/*
 * test_skin_teal.c -- the AUTHORED-grade skin oracle (test-skin-teal).
 *
 * beads: initech-m6qx (re-flair STEP 6); epic initech-qipc step 6.
 * Ref:   ADR-0006 Sec 4.6 (test-skin-teal -- the INJECTED-VALUE oracle for the
 *        AUTHORED teal datum, two independent proofs, neither by-construction).
 *        ADR-0004-AMENDMENT-DEC-09 Sec 3.4 ARB-5 (bevel teal) + D-9b (win31 peer
 *        accents). CLAUDE.md Law 2 (oracle is truth), Rule 6 (mutation-proven).
 *
 * WHAT IT GRADES. spec/flair_skins.h is the registry VIEW; this oracle asserts
 * the registry's teal/bevel/desktop slots (base row) and the win31 peer accents
 * (peer row) equal the LOCKED canon authored datum -- the values stated
 * literally HERE from ADR-0004-AMENDMENT-DEC-09 Sec 3.9 / color_canon.json:
 *     idx2 teal          #8DDCDC
 *     bevel_light        #8DDCDC
 *     bevel_shadow       #4E9BA3
 *     win31 caption navy #000080
 *     win31 btnface      #C0C0C0
 *     win31 btnshadow    #808080
 *
 * WHY IT IS NOT BY-CONSTRUCTION (Law 2). The EXPECTED values are LITERAL
 * constants stated in this file (the locked canon datum, transcribed from the
 * ADR/JSON), NOT recomputed from flair_canon_rgb. The VALUE-UNDER-TEST is the
 * skin REGISTRY slot (flair_skin_registry[].<slot>.rgb). The registry pulls its
 * RGB by inclusion from color_canon.h; this oracle reads the SKIN record and
 * compares it to the independently-stated authored constant. A slot drifting
 * from the canon (a hand-edit to flair_skins.h, or a canon regen that the
 * registry did not track) is caught. The teal datum is AUTHORED (no upstream
 * decomp golden exists; P4 honesty) -- never claimed decomp-sourced.
 *
 * MUTATION PROOF (Rule 6): built with -DSKIN_TEAL_MUTANT, the VALUE-UNDER-TEST
 * (the skin slot the oracle reads) is perturbed by one count, simulating a slot
 * that drifted from the canon -> the locked-constant assert MUST go RED. Restore
 * by dropping the -D. (We mutate a LOCAL copy of the read value, never the
 * locked header.)
 *
 * DUAL-COMPILE / HOSTED RUN (mirrors test_color_canon.c):
 *   gcc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets \
 *       harness/proptest/test_skin_teal.c -o build/test_skin_teal \
 *       && build/test_skin_teal
 *   Mutant:  add -DSKIN_TEAL_MUTANT  (MUST exit non-zero / RED).
 *
 * ASCII-clean (Rule 12). Deterministic / no timestamps (Rule 11).
 */
#include "flair_skins.h"   /* the VALUE-UNDER-TEST: the registry VIEW. */

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * The independently-stated LOCKED authored datum (Law 2: the EXPECTED side is a
 * literal transcribed from ADR-0004-AMENDMENT-DEC-09 Sec 3.9 / color_canon.json,
 * NOT recomputed from flair_canon_rgb). These are the expected RGBs the skin
 * slots must equal. They are stated ONCE, here, as the oracle's ground truth.
 * ------------------------------------------------------------------------- */
#define EXPECT_TEAL          0x8DDCDCu  /* idx2 desktop + bevel_light            */
#define EXPECT_TEAL_SHADOW   0x4E9BA3u  /* bevel_shadow                          */
#define EXPECT_WIN31_NAVY    0x000080u  /* win31 caption navy (== CIDX_ACCENT)   */
#define EXPECT_WIN31_BTNFACE 0xC0C0C0u  /* win31 btnface     (== CIDX_CONTROL)   */
#define EXPECT_WIN31_BTNSHAD 0x808080u  /* win31 btnshadow   (gray-ramp 0x80)    */

/* read_slot(rgb) is the VALUE-UNDER-TEST hook. The mutant perturbs it by one
 * count so the locked-constant assert below bites (Rule 6). */
static uint32_t read_slot(uint32_t rgb)
{
#ifdef SKIN_TEAL_MUTANT
    return rgb ^ 0x000001u;   /* simulate a slot that drifted from the canon. */
#else
    return rgb;
#endif
}

static int check(const char *name, uint32_t got, uint32_t want, int *fails)
{
    if (got != want) {
        (*fails)++;
        printf("FAIL %-22s got #%06X want #%06X\n",
               name, (unsigned)got, (unsigned)want);
        return 0;
    }
    return 1;
}

int main(void)
{
    int fails = 0, checks = 0;

    const flair_skin_t *base = flair_skin_resolve(ERA_SYS7_0_1, HERITAGE_QUICKDRAW);
    const flair_skin_t *peer = flair_skin_resolve(ERA_WIN31, HERITAGE_GDI);

    /* --- AUTHORED teal datum on the System-7 base row (ARB-5). --- */
    checks++; check("base.desktop",      read_slot(base->desktop.rgb),      EXPECT_TEAL,        &fails);
    checks++; check("base.bevel_light",  read_slot(base->bevel_light.rgb),  EXPECT_TEAL,        &fails);
    checks++; check("base.bevel_shadow", read_slot(base->bevel_shadow.rgb), EXPECT_TEAL_SHADOW, &fails);

    /* --- Win-3.1 peer accents on the GDI peer row (D-9b). --- */
    checks++; check("peer.caption_navy", read_slot(peer->caption_navy.rgb), EXPECT_WIN31_NAVY,    &fails);
    checks++; check("peer.btnface",      read_slot(peer->btnface.rgb),      EXPECT_WIN31_BTNFACE, &fails);
    checks++; check("peer.btnshadow",    read_slot(peer->btnshadow.rgb),    EXPECT_WIN31_BTNSHAD, &fails);

    /* The slot's stored idx must be the canon index it claims (the VIEW carries
     * idx + RGB; a mismatched idx would make the slot un-gradeable). */
    checks++; if (base->desktop.idx != CIDX_DESKTOP) { fails++; printf("FAIL base.desktop.idx != CIDX_DESKTOP\n"); }
    checks++; if (peer->caption_navy.idx != CIDX_ACCENT) { fails++; printf("FAIL peer.caption_navy.idx != CIDX_ACCENT\n"); }
    checks++; if (peer->btnface.idx != CIDX_CONTROL) { fails++; printf("FAIL peer.btnface.idx != CIDX_CONTROL\n"); }

    printf("test_skin_teal: %d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}

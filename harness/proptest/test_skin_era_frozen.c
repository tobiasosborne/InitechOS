/*
 * test_skin_era_frozen.c -- the ACCRETION digest oracle (test-skin-era-frozen).
 *
 * beads: initech-m6qx (re-flair STEP 6); epic initech-qipc step 6.
 * Ref:   ADR-0006 Sec 4.6 (test-skin-era-frozen -- the accretion guardrail: a
 *        frozen-row digest-of-fields over the LOCKED ERA_SYS7_0_1 + ERA_WIN31
 *        registry rows; ERA_SYS8_PLATINUM must stay zero-rows). ADR-0004-
 *        AMENDMENT-DEC-09 Sec 3.7 D-9 P5 (accretion = APPEND, never MUTATE a
 *        locked base row). CLAUDE.md Law 2, Rule 6 (mutation-proven), Rule 11.
 *
 * WHAT IT GRADES. It computes a deterministic digest-of-fields (FNV-1a-32) over
 * the LOCKED base rows of flair_skin_registry[] -- ERA_SYS7_0_1/QUICKDRAW and
 * ERA_WIN31/GDI, every color slot (idx + rgb) and every scalar field -- and
 * asserts it equals the COMMITTED expected digest below. Mutating ANY base-row
 * field changes the digest -> RED. This mechanically enforces the additive-only
 * accretion rule: a later ERA_SYS8_PLATINUM commit may APPEND rows (which this
 * digest, computed over ONLY the two base rows, ignores) but must NOT mutate a
 * locked base-row field. It also asserts ERA_SYS8_PLATINUM still has ZERO rows
 * (the registry holds exactly the two base rows today).
 *
 * WHY IT IS NOT BY-CONSTRUCTION (Law 2). The EXPECTED side is a frozen constant
 * (SKIN_FROZEN_DIGEST) committed once. The VALUE-UNDER-TEST is the live
 * registry's bytes. The oracle does not recompute "expected" from the same
 * source on every run; it compares the live digest to a pinned number. A drift
 * in any locked field diverges from the pin and bites.
 *
 * THE DIGEST IS DETERMINISTIC (Rule 11). FNV-1a-32 over a fixed, ordered byte
 * stream of the row fields (each multi-byte field emitted little-end-first); no
 * struct padding is hashed (fields are fed explicitly), no timestamps, no host
 * paths. The stream order is the field order of feed_row() below and is part of
 * the contract; changing the field set or order is a deliberate re-pin.
 *
 * MUTATION PROOF (Rule 6): built with -DSKIN_FROZEN_MUTANT, ONE base-row field
 * (the System-7 desktop slot rgb) is perturbed by one count before hashing ->
 * the live digest diverges from SKIN_FROZEN_DIGEST -> RED. Restore by dropping
 * the -D. (We mutate a LOCAL copy fed to the hash, never the locked header.)
 *
 * COMPUTING / RE-PINNING THE DIGEST. The expected digest is the value this same
 * program prints on its "live_digest=0x........" line in a NON-mutant build. To
 * (re-)pin after a DELIBERATE, ADR-sanctioned base-row change: build without
 * -DSKIN_FROZEN_MUTANT, read the printed live_digest, and set SKIN_FROZEN_DIGEST
 * to it (a deliberate Rule-8 re-pin with a beads issue + worklog note). An
 * accidental edit is exactly what the un-changed pin catches.
 *
 * DUAL-COMPILE / HOSTED RUN (mirrors test_color_canon.c):
 *   gcc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets \
 *       harness/proptest/test_skin_era_frozen.c -o build/test_skin_era_frozen \
 *       && build/test_skin_era_frozen
 *   Mutant:  add -DSKIN_FROZEN_MUTANT  (MUST exit non-zero / RED).
 *
 * ASCII-clean (Rule 12). Deterministic / no timestamps (Rule 11).
 */
#include "flair_skins.h"   /* the VALUE-UNDER-TEST: the registry VIEW. */

#include <stdio.h>

/* ---------------------------------------------------------------------------
 * THE COMMITTED EXPECTED DIGEST (Rule 8 locked-data). This is the FNV-1a-32 of
 * the two LOCKED base rows under the field order of feed_row() below. It is
 * re-pinned ONLY by a deliberate ADR-sanctioned base-row change (a beads issue
 * + worklog note); an accidental base-row edit diverges from it and goes RED.
 * ------------------------------------------------------------------------- */
#define SKIN_FROZEN_DIGEST 0xDEF099ACu

/* FNV-1a-32 (deterministic, no libc, ASCII; Rule 11). */
#define FNV1A32_OFFSET 0x811C9DC5u
#define FNV1A32_PRIME  0x01000193u

static uint32_t fnv_byte(uint32_t h, uint8_t b)
{
    h ^= (uint32_t)b;
    h *= FNV1A32_PRIME;
    return h;
}

/* Feed a uint32 little-end-first (fixed order; part of the digest contract). */
static uint32_t fnv_u32(uint32_t h, uint32_t v)
{
    h = fnv_byte(h, (uint8_t)(v & 0xFFu));
    h = fnv_byte(h, (uint8_t)((v >> 8) & 0xFFu));
    h = fnv_byte(h, (uint8_t)((v >> 16) & 0xFFu));
    h = fnv_byte(h, (uint8_t)((v >> 24) & 0xFFu));
    return h;
}

static uint32_t fnv_slot(uint32_t h, const flair_skin_slot_t *s)
{
    h = fnv_u32(h, (uint32_t)s->idx);
    h = fnv_u32(h, s->rgb);
    return h;
}

/* feed_row() -- the FIXED field-order byte stream of one locked row. The order
 * here IS the digest contract (Rule 11); it matches flair_skin_t's field order. */
static uint32_t feed_row(uint32_t h, const flair_skin_t *r)
{
    h = fnv_u32(h, (uint32_t)r->era);
    h = fnv_u32(h, (uint32_t)r->heritage);
    h = fnv_slot(h, &r->desktop);
    h = fnv_slot(h, &r->caption_navy);
    h = fnv_slot(h, &r->caption_ink);
    h = fnv_slot(h, &r->menubar);
    h = fnv_slot(h, &r->btnface);
    h = fnv_slot(h, &r->btnshadow);
    h = fnv_slot(h, &r->btnhilight);
    h = fnv_slot(h, &r->frame_ink);
    h = fnv_slot(h, &r->content);
    h = fnv_slot(h, &r->bevel_light);
    h = fnv_slot(h, &r->bevel_shadow);
    h = fnv_u32(h, (uint32_t)r->pinstripe_pattern);
    h = fnv_u32(h, (uint32_t)r->bevel_width);
    return h;
}

int main(void)
{
    int fails = 0, checks = 0;

    /* The two LOCKED base rows are exactly the registry today (zero Platinum
     * rows). Assert that invariant first -- a stray row would shift the digest
     * AND violate "ERA_SYS8_PLATINUM has zero rows". */
    checks++;
    if (FLAIR_SKIN_REGISTRY_COUNT != 2u) {
        fails++;
        printf("FAIL registry row count = %u (expected 2: SYS7 base + WIN31 peer; "
               "ERA_SYS8_PLATINUM must have ZERO rows)\n",
               (unsigned)FLAIR_SKIN_REGISTRY_COUNT);
    }
    /* No row may carry the RESERVED ERA_SYS8_PLATINUM era today. */
    {
        uint16_t i;
        for (i = 0; i < FLAIR_SKIN_REGISTRY_COUNT; i++) {
            checks++;
            if ((int)flair_skin_registry[i].era == ERA_SYS8_PLATINUM) {
                fails++;
                printf("FAIL row %u carries ERA_SYS8_PLATINUM (reserved -- must be ZERO rows)\n",
                       (unsigned)i);
            }
        }
    }

    /* Digest the two LOCKED base rows by (era, heritage), found via the resolver
     * so the digest is independent of physical row order. */
    {
        flair_skin_t base = *flair_skin_resolve(ERA_SYS7_0_1, HERITAGE_QUICKDRAW);
        flair_skin_t peer = *flair_skin_resolve(ERA_WIN31, HERITAGE_GDI);
        uint32_t h = FNV1A32_OFFSET;

#ifdef SKIN_FROZEN_MUTANT
        /* Perturb ONE base-row field by one count (a LOCAL copy) so the digest
         * diverges from the pin and the assert bites (Rule 6). */
        base.desktop.rgb ^= 0x000001u;
#endif

        h = feed_row(h, &base);
        h = feed_row(h, &peer);

        printf("live_digest=0x%08X  expected=0x%08X\n",
               (unsigned)h, (unsigned)SKIN_FROZEN_DIGEST);

        checks++;
        if (h != SKIN_FROZEN_DIGEST) {
            fails++;
            printf("FAIL frozen-row digest drift (a locked base-row field "
                   "changed -- accretion is APPEND-ONLY, never MUTATE; D-9 P5)\n");
        }
    }

    printf("test_skin_era_frozen: %d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}

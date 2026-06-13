/* test_mcb.c -- host property suite for the MCB memory arena (initech-509.6).
 *
 * The oracle for the AH=48h/49h/4Ah allocator (mcb.c). It covers, by scenario
 * and by randomized fuzz: the chain invariants (M/Z signatures, blocks tile the
 * arena exactly, one terminal 'Z' last), first-fit placement, split-on-alloc,
 * coalesce-on-free, setblock grow/shrink, the failure path (insufficient +
 * largest-free report), the bad-block / owner guards, and -- the real test --
 * DATA INTEGRITY: every live allocation's bytes survive unrelated alloc/free/
 * resize traffic (no allocation or header write ever stomps another block).
 *
 * Mutation-proven via make test-mcb-mutant (MCB_MUTATE_NO_COALESCE,
 * MCB_MUTATE_NO_OWNER_CHECK). CLAUDE.md Law 2, Rule 6, Rule 11 (seeded PRNG ->
 * deterministic), Rule 12 (ASCII).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcb.h"
#include "test_assert.h"

TEST_HARNESS();

/* Header at paragraph `para` (the test knows the same layout mcb.c uses). */
static mcb_t *H(mcb_arena_t *a, uint32_t para)
{
    return (mcb_t *)(a->base + (size_t)para * 16u);
}

static void arena_make(mcb_arena_t *a, uint32_t paras)
{
    void *buf = malloc((size_t)paras * 16u);
    /* Poison so a missing header write would be visible. */
    memset(buf, 0xCC, (size_t)paras * 16u);
    mcb_init(a, buf, paras);
}

/* Count the blocks and the free/used data totals by walking the chain. */
static void survey(mcb_arena_t *a, int *nblocks, uint32_t *free_data,
                   uint32_t *used_data, int *nterm)
{
    uint32_t para = 0, fd = 0, ud = 0;
    int nb = 0, nt = 0;
    for (;;) {
        mcb_t *h = H(a, para);
        nb++;
        if (h->owner == MCB_OWNER_FREE) fd += h->size_paras;
        else                            ud += h->size_paras;
        if (h->signature == MCB_SIG_TERMINAL) { nt++; break; }
        para += 1u + h->size_paras;
    }
    *nblocks = nb; *free_data = fd; *used_data = ud; *nterm = nt;
}

/* ---- a deterministic LCG so the fuzz is reproducible (Rule 11) ----------- */
static uint32_t g_seed = 0x1234567u;
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}

/* A live allocation the fuzz tracks: data position + a unique data signature. */
struct live { int present; uint32_t data_para; uint32_t paras; uint16_t owner; uint8_t tag; };

static void fill(mcb_arena_t *a, const struct live *L)
{
    uint8_t *d = a->base + (size_t)L->data_para * 16u;
    uint32_t n = L->paras * 16u;
    for (uint32_t j = 0; j < n; j++) d[j] = (uint8_t)(L->tag + (uint8_t)j);
}
static int intact_data(mcb_arena_t *a, const struct live *L)
{
    const uint8_t *d = a->base + (size_t)L->data_para * 16u;
    uint32_t n = L->paras * 16u;
    for (uint32_t j = 0; j < n; j++)
        if (d[j] != (uint8_t)(L->tag + (uint8_t)j)) return 0;
    return 1;
}

int main(void)
{
    /* ===== Scenario 1: init lays one terminal free block. ================= */
    {
        mcb_arena_t a; arena_make(&a, 16u);
        CHECK(mcb_chain_intact(&a), "init: chain intact");
        CHECK(H(&a, 0)->signature == MCB_SIG_TERMINAL, "init: lone block is 'Z'");
        CHECK(H(&a, 0)->owner == MCB_OWNER_FREE, "init: lone block is free");
        CHECK(H(&a, 0)->size_paras == 15u, "init: data size = total-1 (header)");
        free(a.base);
    }

    /* ===== Scenario 2: alloc splits; size is exactly `want`. ============== */
    {
        mcb_arena_t a; arena_make(&a, 16u);
        uint32_t dp = 0, lg = 0;
        uint16_t rc = mcb_alloc(&a, 0x0001u, 4u, &dp, &lg);
        CHECK(rc == MCB_OK, "alloc: succeeds when room");
        CHECK(dp == 1u, "alloc: data paragraph follows the header");
        CHECK(H(&a, 0)->owner == 0x0001u, "alloc: owner tagged");
        CHECK(H(&a, 0)->size_paras == 4u, "alloc: block size is exactly want");
        CHECK(H(&a, 0)->signature == MCB_SIG_NONTERMINAL, "alloc: now 'M' (precedes remainder)");
        CHECK(H(&a, 5)->signature == MCB_SIG_TERMINAL, "alloc: remainder is the new 'Z'");
        CHECK(H(&a, 5)->owner == MCB_OWNER_FREE, "alloc: remainder is free");
        CHECK(H(&a, 5)->size_paras == 10u, "alloc: remainder = 15-4-1");
        CHECK(mcb_chain_intact(&a), "alloc: chain intact after split");
        free(a.base);
    }

    /* ===== Scenario 3: first-fit reuses the earliest hole. ================ */
    {
        mcb_arena_t a; arena_make(&a, 64u);
        uint32_t A = 0, B = 0, C = 0;
        mcb_alloc(&a, 1u, 8u, &A, 0);     /* block A @ data 1  */
        mcb_alloc(&a, 2u, 8u, &B, 0);     /* block B @ data 10 */
        mcb_alloc(&a, 3u, 8u, &C, 0);     /* block C @ data 19 */
        CHECK(mcb_free(&a, A, 1u) == MCB_OK, "first-fit: free A");
        uint32_t D = 0;
        CHECK(mcb_alloc(&a, 4u, 4u, &D, 0) == MCB_OK, "first-fit: alloc fits earlier hole");
        CHECK(D == A, "first-fit: new small alloc reuses A's hole, not the tail");
        CHECK(mcb_chain_intact(&a), "first-fit: chain intact");
        free(a.base);
    }

    /* ===== Scenario 4: coalesce -- free everything -> one 'Z' of full size. */
    {
        mcb_arena_t a; arena_make(&a, 64u);
        uint32_t h[5]; uint16_t own[5];
        for (int i = 0; i < 5; i++) { own[i] = (uint16_t)(i + 1); mcb_alloc(&a, own[i], 5u, &h[i], 0); }
        /* free out of order to exercise forward + multi-run coalescing */
        mcb_free(&a, h[1], own[1]);
        mcb_free(&a, h[3], own[3]);
        mcb_free(&a, h[0], own[0]);
        mcb_free(&a, h[2], own[2]);
        mcb_free(&a, h[4], own[4]);
        int nb, nt; uint32_t fd, ud;
        survey(&a, &nb, &fd, &ud, &nt);
        CHECK(nb == 1, "coalesce: free-all leaves a single block");
        CHECK(H(&a, 0)->signature == MCB_SIG_TERMINAL, "coalesce: it is the terminal 'Z'");
        CHECK(H(&a, 0)->owner == MCB_OWNER_FREE, "coalesce: it is free");
        CHECK(H(&a, 0)->size_paras == 63u, "coalesce: it spans the whole arena again");
        free(a.base);
    }

    /* ===== Scenario 5: failure reports the largest free run (AH=48h BX). == */
    {
        mcb_arena_t a; arena_make(&a, 32u);
        uint32_t x = 0;
        mcb_alloc(&a, 1u, 10u, &x, 0);          /* leaves a free tail of 32-1-10-1 = 20 */
        uint32_t dp = 0, lg = 0;
        uint16_t rc = mcb_alloc(&a, 2u, 100u, &dp, &lg);
        CHECK(rc == MCB_ERR_INSUFFICIENT, "fail: too-big request is insufficient");
        CHECK(lg == 20u, "fail: largest free run reported (DOS AH=48h BX)");
        free(a.base);
    }

    /* ===== Scenario 6: free guards -- bad addr / double free / wrong owner. */
    {
        mcb_arena_t a; arena_make(&a, 32u);
        uint32_t A = 0; mcb_alloc(&a, 7u, 6u, &A, 0);
        CHECK(mcb_free(&a, A + 2u, 7u) == MCB_ERR_BAD_BLOCK, "guard: mid-block free rejected");
        CHECK(mcb_free(&a, A, 99u) == MCB_ERR_BAD_BLOCK, "guard: wrong-owner free rejected");
        CHECK(mcb_free(&a, A, 7u) == MCB_OK, "guard: correct free accepted");
        CHECK(mcb_free(&a, A, 7u) == MCB_ERR_BAD_BLOCK, "guard: double free rejected");
        CHECK(mcb_chain_intact(&a), "guard: chain intact after rejected frees");
        free(a.base);
    }

    /* ===== Scenario 7: setblock shrink frees the tail. =================== */
    {
        mcb_arena_t a; arena_make(&a, 32u);
        uint32_t A = 0; mcb_alloc(&a, 3u, 12u, &A, 0);
        CHECK(mcb_setblock(&a, A, 4u, 3u, 0) == MCB_OK, "setblock: shrink ok");
        CHECK(H(&a, A - 1u)->size_paras == 4u, "setblock: block shrunk to 4");
        CHECK(mcb_chain_intact(&a), "setblock: chain intact after shrink");
        int nb, nt; uint32_t fd, ud; survey(&a, &nb, &fd, &ud, &nt);
        CHECK(ud == 4u, "setblock: only 4 paras still used");
        free(a.base);
    }

    /* ===== Scenario 8: setblock grows into a free neighbour, else fails. == */
    {
        mcb_arena_t a; arena_make(&a, 32u);
        uint32_t A = 0; mcb_alloc(&a, 1u, 4u, &A, 0);   /* A then a free tail */
        CHECK(mcb_setblock(&a, A, 10u, 1u, 0) == MCB_OK, "setblock: grow into free tail");
        CHECK(H(&a, A - 1u)->size_paras == 10u, "setblock: grew to 10");
        CHECK(mcb_chain_intact(&a), "setblock: chain intact after grow");

        /* Now wall A in with an allocated neighbour and prove grow fails. */
        mcb_arena_t b; arena_make(&b, 32u);
        uint32_t P = 0, Q = 0; mcb_alloc(&b, 1u, 5u, &P, 0); mcb_alloc(&b, 2u, 5u, &Q, 0);
        uint32_t lg = 0;
        CHECK(mcb_setblock(&b, P, 9u, 1u, &lg) == MCB_ERR_INSUFFICIENT,
              "setblock: grow blocked by an allocated neighbour");
        CHECK(lg == 5u, "setblock: largest reachable = current size when walled in");
        free(a.base); free(b.base);
    }

    /* ===== Scenario 9: randomized fuzz with data-integrity verification. == */
    {
        enum { POOL = 48, ITERS = 20000, APARAS = 600 };
        mcb_arena_t a; arena_make(&a, (uint32_t)APARAS);
        struct live live[POOL];
        memset(live, 0, sizeof(live));
        uint8_t next_tag = 1;
        int corrupt = 0, stomp = 0;

        for (int it = 0; it < ITERS && !corrupt && !stomp; it++) {
            int slot = (int)(lcg() % POOL);
            struct live *L = &live[slot];
            uint32_t op = lcg() % 3u;

            if (!L->present) {
                /* free slot -> attempt an allocation of 1..20 data paras */
                uint32_t want = 1u + (lcg() % 20u);
                uint32_t dp = 0;
                uint16_t owner = (uint16_t)(slot + 1);   /* distinct, nonzero */
                if (mcb_alloc(&a, owner, want, &dp, 0) == MCB_OK) {
                    L->present = 1; L->data_para = dp; L->paras = want;
                    L->owner = owner; L->tag = next_tag++;
                    if (next_tag == 0) next_tag = 1;
                    fill(&a, L);
                }
            } else if (op == 0u) {
                /* free it (with the correct owner) */
                if (mcb_free(&a, L->data_para, L->owner) != MCB_OK) corrupt = 1;
                L->present = 0;
            } else {
                /* setblock to a new random size; on success re-stamp + re-track */
                uint32_t want = 1u + (lcg() % 24u);
                uint32_t lg = 0;
                uint16_t rc = mcb_setblock(&a, L->data_para, want, L->owner, &lg);
                if (rc == MCB_OK) { L->paras = want; fill(&a, L); }
                /* INSUFFICIENT is a legal outcome (no free neighbour); ignore */
            }

            if (!mcb_chain_intact(&a)) { corrupt = 1; break; }

            /* Every so often, verify EVERY live allocation's bytes survived. */
            if ((it & 0x3F) == 0) {
                for (int s = 0; s < POOL; s++)
                    if (live[s].present && !intact_data(&a, &live[s])) { stomp = 1; break; }
            }
        }
        CHECK(!corrupt, "fuzz: the MCB chain stays intact across 20k random ops");
        CHECK(!stomp, "fuzz: no allocation's data is ever stomped by other traffic");

        /* Final data check across all survivors. */
        int all_ok = 1;
        for (int s = 0; s < POOL; s++)
            if (live[s].present && !intact_data(&a, &live[s])) all_ok = 0;
        CHECK(all_ok, "fuzz: all survivors' data intact at the end");

        /* Free everything -> the arena must return to a single full 'Z' block. */
        for (int s = 0; s < POOL; s++)
            if (live[s].present) mcb_free(&a, live[s].data_para, live[s].owner);
        int nb, nt; uint32_t fd, ud; survey(&a, &nb, &fd, &ud, &nt);
        CHECK(nb == 1 && nt == 1, "fuzz: free-all coalesces to one terminal block");
        CHECK(H(&a, 0)->size_paras == (uint32_t)(APARAS - 1),
              "fuzz: the lone block spans the whole arena again");
        free(a.base);
    }

    return TEST_SUMMARY("test_mcb");
}

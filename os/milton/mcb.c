/* mcb.c -- InitechDOS memory arena: the MCB chain behind AH=48h/49h/4Ah.
 *
 * beads: initech-509.6 (DEC-03). Pure, host-testable allocator over a caller-
 * provided region, addressed in arena-relative paragraphs. See mcb.h for the
 * model + the DOS mapping. Fail loud on a corrupt chain or a bad block (Rule 2);
 * the property suite (test_mcb.c) is the oracle (Law 2).
 *
 * Ref: ADR-0003 DEC-03 + Appendix B.3; DOS 3.3 PRM AH=48h/49h/4Ah. ASCII (R12).
 */
#include "mcb.h"

#define MCB_PARA 16u   /* bytes per paragraph */

/* Forward declaration: mcb_set_arena_owner / mcb_free_owner (defined below the
 * header helpers) coalesce on completion; the coalescer is defined further down. */
static void mcb_coalesce(mcb_arena_t *a);

static mcb_t *mcb_hdr(const mcb_arena_t *a, uint32_t para)
{
    return (mcb_t *)(a->base + (uintptr_t)para * MCB_PARA);
}

static int sig_ok(uint8_t s)
{
    return s == MCB_SIG_NONTERMINAL || s == MCB_SIG_TERMINAL;
}

static void mcb_zero_reserved(mcb_t *h)
{
    for (int i = 0; i < 11; i++) {
        h->reserved[i] = 0u;
    }
}

void mcb_init(mcb_arena_t *a, void *base, uint32_t total_paras)
{
    a->base        = (uint8_t *)base;
    a->total_paras = total_paras;

    /* One terminal ('Z') free block spanning the whole arena: header + data. */
    mcb_t *h = mcb_hdr(a, 0u);
    h->signature  = MCB_SIG_TERMINAL;
    h->owner      = MCB_OWNER_FREE;
    h->size_paras = (uint16_t)(total_paras - 1u);
    mcb_zero_reserved(h);
}

int mcb_set_arena_owner(mcb_arena_t *a, uint16_t owner)
{
    if (a->total_paras == 0u) {
        return 0;
    }
    mcb_t *h = mcb_hdr(a, 0u);
    /* Only a pristine single terminal block (one 'Z' spanning the whole arena)
     * may be wholesale-claimed: signature 'Z' AND span == total. This is exactly
     * the post-mcb_init state. Refuse on any other shape so a fragmented or
     * mid-chain arena can never be silently re-owned. */
    if (h->signature != MCB_SIG_TERMINAL) {
        return 0;
    }
    if (1u + (uint32_t)h->size_paras != a->total_paras) {
        return 0;
    }
    h->owner = owner;
    return 1;
}

uint32_t mcb_free_owner(mcb_arena_t *a, uint16_t owner)
{
    if (owner == MCB_OWNER_FREE) {
        return 0u;                                      /* nonsensical request   */
    }
    if (!mcb_chain_intact(a)) {
        return 0u;                                      /* corrupt -> no-op       */
    }
    uint32_t freed = 0u;
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = mcb_hdr(a, para);
        uint8_t term = (h->signature == MCB_SIG_TERMINAL);
        if (h->owner == owner) {
            h->owner = MCB_OWNER_FREE;
            freed++;
        }
        if (term) {
            break;
        }
        para += 1u + h->size_paras;
    }
    if (freed != 0u) {
        mcb_coalesce(a);
    }
    return freed;
}

int mcb_chain_intact(const mcb_arena_t *a)
{
    if (a->total_paras == 0u) {
        return 0;
    }
    uint32_t para = 0u;
    for (;;) {
        if (para >= a->total_paras) {
            return 0;                                  /* walked off the end     */
        }
        const mcb_t *h = mcb_hdr(a, para);
        if (!sig_ok(h->signature)) {
            return 0;                                  /* corrupt signature      */
        }
        uint32_t span = 1u + h->size_paras;            /* header + data          */
        if (para + span > a->total_paras) {
            return 0;                                  /* block overruns arena   */
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return (para + span == a->total_paras);     /* 'Z' must end the arena*/
        }
        para += span;
    }
}

/* Merge every run of adjacent FREE blocks into one. Idempotent. Assumes the
 * chain is intact (callers check first). */
static void mcb_coalesce(mcb_arena_t *a)
{
#ifdef MCB_MUTATE_NO_COALESCE
    /* MUTANT (Rule 6; make test-mcb-mutant only): never merge free neighbours,
     * so "free everything -> one terminal block" leaves a fragmented chain and
     * the round-trip oracle goes RED. NEVER define in a real build. */
    (void)a;
    return;
#else
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = mcb_hdr(a, para);
        if (h->signature == MCB_SIG_TERMINAL) {
            return;                                     /* last block            */
        }
        uint32_t npara = para + 1u + h->size_paras;
        mcb_t *n = mcb_hdr(a, npara);
        if (h->owner == MCB_OWNER_FREE && n->owner == MCB_OWNER_FREE) {
            /* Absorb n into h: h grows by n's whole span; h inherits n's
             * terminal-ness (becomes 'Z' iff n was the last block). Do NOT
             * advance -- h may now abut a further free block. */
            h->size_paras = (uint16_t)(h->size_paras + 1u + n->size_paras);
            h->signature  = n->signature;
        } else {
            para = npara;
        }
    }
#endif
}

/* Confirm header_para is a real block boundary by walking from the start. */
static int mcb_find_header(const mcb_arena_t *a, uint32_t header_para)
{
    uint32_t para = 0u;
    for (;;) {
        const mcb_t *h = mcb_hdr(a, para);
        if (para == header_para) {
            return 1;
        }
        if (h->signature == MCB_SIG_TERMINAL) {
            return 0;                                   /* passed the last block */
        }
        uint32_t np = para + 1u + h->size_paras;
        if (np <= para || np >= a->total_paras) {
            return 0;                                   /* corrupt / overshoot   */
        }
        para = np;
    }
}

/* Carve a FREE remainder of (avail - want - 1) data paras out of an allocated
 * block `h` at header `hpara` whose current data size is `avail` (> want). The
 * remainder block inherits h's terminal-ness; h becomes non-terminal of size
 * `want`. (When avail == want+1 the remainder is a valid 0-data free header.) */
static void mcb_split(mcb_arena_t *a, uint32_t hpara, mcb_t *h,
                      uint32_t avail, uint32_t want)
{
    uint32_t rpara = hpara + 1u + want;
    mcb_t *r = mcb_hdr(a, rpara);
    r->signature  = h->signature;                       /* R inherits terminal   */
    r->owner      = MCB_OWNER_FREE;
    r->size_paras = (uint16_t)(avail - want - 1u);
    mcb_zero_reserved(r);

    h->signature  = MCB_SIG_NONTERMINAL;                /* h now precedes R      */
    h->size_paras = (uint16_t)want;
}

uint16_t mcb_alloc(mcb_arena_t *a, uint16_t owner, uint32_t want,
                   uint32_t *out_data_para, uint32_t *out_largest)
{
    if (!mcb_chain_intact(a)) {
        return MCB_ERR_DESTROYED;
    }
    mcb_coalesce(a);                                    /* fair first-fit        */

    uint32_t largest = 0u;
    uint32_t para = 0u;
    for (;;) {
        mcb_t *h = mcb_hdr(a, para);
        uint8_t term = (h->signature == MCB_SIG_TERMINAL);
        if (h->owner == MCB_OWNER_FREE) {
            uint32_t sz = h->size_paras;
            if (sz > largest) {
                largest = sz;
            }
            if (sz >= want) {
                if (sz > want) {
                    mcb_split(a, para, h, sz, want);    /* leaves remainder free */
                }
                h->owner = owner;
                if (out_data_para) {
                    *out_data_para = para + 1u;
                }
                return MCB_OK;
            }
        }
        if (term) {
            break;
        }
        para += 1u + h->size_paras;
    }
    if (out_largest) {
        *out_largest = largest;
    }
    return MCB_ERR_INSUFFICIENT;
}

uint16_t mcb_free(mcb_arena_t *a, uint32_t data_para, uint16_t owner)
{
    if (!mcb_chain_intact(a)) {
        return MCB_ERR_DESTROYED;
    }
    if (data_para == 0u) {
        return MCB_ERR_BAD_BLOCK;                       /* no header at -1       */
    }
    uint32_t hpara = data_para - 1u;
    if (!mcb_find_header(a, hpara)) {
        return MCB_ERR_BAD_BLOCK;                        /* not a block boundary */
    }
    mcb_t *h = mcb_hdr(a, hpara);
    if (h->owner == MCB_OWNER_FREE) {
        return MCB_ERR_BAD_BLOCK;                        /* already free         */
    }
#ifndef MCB_MUTATE_NO_OWNER_CHECK
    if (owner != MCB_OWNER_ANY && h->owner != owner) {
        return MCB_ERR_BAD_BLOCK;                        /* not the caller's     */
    }
#else
    /* MUTANT (Rule 6; make test-mcb-mutant only): free WITHOUT the owner check,
     * so a cross-owner free is wrongly accepted and the owner-guard oracle goes
     * RED. NEVER define in a real build. */
    (void)owner;
#endif
    h->owner = MCB_OWNER_FREE;
    mcb_coalesce(a);
    return MCB_OK;
}

uint16_t mcb_setblock(mcb_arena_t *a, uint32_t data_para, uint32_t want,
                      uint16_t owner, uint32_t *out_largest)
{
    if (!mcb_chain_intact(a)) {
        return MCB_ERR_DESTROYED;
    }
    if (data_para == 0u) {
        return MCB_ERR_BAD_BLOCK;
    }
    uint32_t hpara = data_para - 1u;
    if (!mcb_find_header(a, hpara)) {
        return MCB_ERR_BAD_BLOCK;
    }
    mcb_t *h = mcb_hdr(a, hpara);
    if (h->owner == MCB_OWNER_FREE) {
        return MCB_ERR_BAD_BLOCK;                        /* not an allocated block*/
    }
    if (owner != MCB_OWNER_ANY && h->owner != owner) {
        return MCB_ERR_BAD_BLOCK;
    }

    uint32_t cur = h->size_paras;
    if (want == cur) {
        return MCB_OK;
    }

    if (want < cur) {
        mcb_split(a, hpara, h, cur, want);              /* free the shrunk tail  */
        mcb_coalesce(a);
        return MCB_OK;
    }

    /* Grow: only by absorbing the immediately following FREE block. */
    if (h->signature != MCB_SIG_TERMINAL) {
        uint32_t npara = hpara + 1u + cur;
        mcb_t *n = mcb_hdr(a, npara);
        if (n->owner == MCB_OWNER_FREE) {
            uint32_t combined = cur + 1u + n->size_paras;   /* + n's header      */
            if (combined >= want) {
                h->size_paras = (uint16_t)combined;     /* absorb n ...          */
                h->signature  = n->signature;           /* ... inherit terminal  */
                if (combined > want) {
                    mcb_split(a, hpara, h, combined, want);
                }
                mcb_coalesce(a);
                return MCB_OK;
            }
            if (out_largest) {
                *out_largest = combined;
            }
            return MCB_ERR_INSUFFICIENT;
        }
    }
    if (out_largest) {
        *out_largest = cur;                             /* cannot grow at all    */
    }
    return MCB_ERR_INSUFFICIENT;
}

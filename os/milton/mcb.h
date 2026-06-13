/* mcb.h -- InitechDOS memory arena (the MCB chain behind AH=48h/49h/4Ah).
 *
 * beads: initech-509.6 (DEC-03 memory arena, retained on a vestigial basis but
 *        implemented IN FULL per the design stance). The arena is a chain of
 *        16-byte Memory Control Blocks ('M' non-terminal / 'Z' terminal; owner;
 *        size in paragraphs) exactly as ADR-0003 Appendix B.3 / DEC-03 specify.
 *        AH=48h ALLOC / 49h FREE / 4Ah SETBLOCK operate on it. It is the heap a
 *        loaded program -- ultimately Turbo Initech (New/GetMem) and InitechBase
 *        -- allocates from.
 *
 * This is the PURE allocator: it walks a caller-provided region and is addressed
 * in arena-relative PARAGRAPHS, NOT DOS segments -- so it is fully host-testable
 * on any buffer and is decoupled from the 32-bit flat segment convention. The
 * int21.c AH=48/49/4A handlers convert a returned data-paragraph index to a DOS
 * segment ((arena_base_linear >> 4) + data_para) at the syscall boundary.
 *
 * Block layout (paragraph p): a 1-paragraph mcb_t header at p, then size_paras
 * data paragraphs at p+1.. The next header is at p + 1 + size_paras. The last
 * block carries signature 'Z'; all others 'M'. owner==0 marks a FREE block.
 *
 * Ref: ADR-0003 DEC-03 + Appendix B.3 (the 16-byte MCB); spec/dos_structs.h
 *      (mcb_t, _Static_assert 16 bytes); DOS 3.3 PRM AH=48h/49h/4Ah.
 * CLAUDE.md Law 2 (the property suite is the oracle), Rule 2 (fail loud on a
 * corrupt chain / bad block), Rule 11 (deterministic), Rule 12 (ASCII).
 */
#ifndef INITECH_OS_MCB_H
#define INITECH_OS_MCB_H

#include <stdint.h>
#include "dos_structs.h"   /* mcb_t (16-byte arena header; ADR-0003 B.3) */

/* DOS AH=48/49/4A result codes (the subset this arena raises). */
#define MCB_OK               0x0000u
#define MCB_ERR_DESTROYED    0x0007u   /* MCB chain corrupted (bad signature)   */
#define MCB_ERR_INSUFFICIENT 0x0008u   /* not enough free memory to satisfy     */
#define MCB_ERR_BAD_BLOCK    0x0009u   /* invalid block address (free/setblock) */

#define MCB_OWNER_FREE 0x0000u         /* owner == 0 marks a free block          */
#define MCB_OWNER_ANY  0xFFFFu         /* free/setblock: skip the owner check    */

/* A region handed to the arena: `base` must be paragraph-aligned and span
 * `total_paras` 16-byte paragraphs (header paragraphs included). */
typedef struct mcb_arena {
    uint8_t *base;
    uint32_t total_paras;
} mcb_arena_t;

/* Lay a single terminal ('Z') FREE block over the whole arena. total_paras must
 * be >= 1 (room for the header). The lone block's data size is total_paras-1. */
void mcb_init(mcb_arena_t *a, void *base, uint32_t total_paras);

/* AH=48h ALLOC: first-fit `want` DATA paragraphs to `owner` (must be != 0). On
 * success returns MCB_OK and writes the DATA paragraph index to *out_data_para
 * (the header is at out_data_para-1). On failure returns MCB_ERR_INSUFFICIENT
 * and writes the largest free DATA run to *out_largest (DOS AH=48h failure BX).
 * MCB_ERR_DESTROYED if the chain is corrupt. out_* may be NULL. */
uint16_t mcb_alloc(mcb_arena_t *a, uint16_t owner, uint32_t want,
                   uint32_t *out_data_para, uint32_t *out_largest);

/* AH=49h FREE the block whose DATA starts at data_para. `owner` must match the
 * block's owner (or be MCB_OWNER_ANY). Returns MCB_ERR_BAD_BLOCK if data_para is
 * not a live allocated block boundary or the owner mismatches; MCB_ERR_DESTROYED
 * on corruption. Adjacent free blocks are coalesced. */
uint16_t mcb_free(mcb_arena_t *a, uint32_t data_para, uint16_t owner);

/* AH=4Ah SETBLOCK: resize the block at data_para to `want` DATA paragraphs.
 * Shrinking always succeeds (the freed tail becomes a free block). Growing
 * succeeds only by absorbing a following FREE block; otherwise returns
 * MCB_ERR_INSUFFICIENT with *out_largest = the largest size this block could
 * reach. MCB_ERR_BAD_BLOCK / _DESTROYED as for free. out_largest may be NULL. */
uint16_t mcb_setblock(mcb_arena_t *a, uint32_t data_para, uint32_t want,
                      uint16_t owner, uint32_t *out_largest);

/* Walk the chain and assert the structural invariants: every header signature is
 * 'M' or 'Z'; no block runs past the arena; exactly one 'Z' and it is last; the
 * blocks tile the arena exactly (sum of 1+size == total_paras). Returns 1 if the
 * chain is intact, 0 if corrupt. For the oracle + fail-loud entry guards. */
int mcb_chain_intact(const mcb_arena_t *a);

#endif /* INITECH_OS_MCB_H */

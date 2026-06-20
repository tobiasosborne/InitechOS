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

/* The RESIDENT (system-owned) marker for a TSR's kept block (INT 21h AH=31h KEEP;
 * beads initech-bo40). Real DOS marks the DOS system MCB with owner id 8 (the
 * "owned by DOS" arena -- DOS 3.3 internals / RBIL INT 21/AH=31h; the MCB owner
 * field). We reuse that canonical id so a KEEP'd block (a) is NEVER reclaimed by
 * mcb_free_owner(terminating_psp) -- its owner is no longer the terminating PSP --
 * and (b) is NEVER handed out by mcb_alloc (which reuses ONLY MCB_OWNER_FREE
 * blocks; any non-free owner, including SYSTEM, is skipped). It is distinct from a
 * live PSP owner (a flat-mode PSP segment is PROGRAM_BASE>>4, far above 8) so a
 * resident block can never be confused with a process's own block. */
#define MCB_OWNER_SYSTEM 0x0008u       /* DOS system-owned (a resident TSR block) */

/* A region handed to the arena: `base` must be paragraph-aligned and span
 * `total_paras` 16-byte paragraphs (header paragraphs included). */
typedef struct mcb_arena {
    uint8_t *base;
    uint32_t total_paras;
} mcb_arena_t;

/* Lay a single terminal ('Z') FREE block over the whole arena. total_paras must
 * be >= 1 (room for the header). The lone block's data size is total_paras-1. */
void mcb_init(mcb_arena_t *a, void *base, uint32_t total_paras);

/* Assign the lone block's owner. Valid ONLY when the arena is a single terminal
 * block (one 'Z' header spanning the whole arena, as just after mcb_init): the
 * authentic "a freshly-loaded program owns its whole window" assignment. Returns
 * 1 on success; 0 (no-op) if the arena is not a single terminal block (so it
 * cannot mis-claim a fragmented chain). owner==MCB_OWNER_FREE marks it free. */
int mcb_set_arena_owner(mcb_arena_t *a, uint16_t owner);

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

/* Free EVERY block owned by `owner` (the authentic "DOS frees a terminating
 * process's memory on exit"). Coalesces afterward. owner must be != FREE.
 * Returns the number of blocks freed (0 if the owner held none). Used by INT 21h
 * AH=4Ch/00h/INT 20h terminate to reclaim a process's arena, mirroring the JFT
 * handle close. Returns 0 on a corrupt chain (no-op; the caller is mid-teardown
 * and a panic on the way out is worse than leaving the chain). */
uint32_t mcb_free_owner(mcb_arena_t *a, uint16_t owner);

/* INT 21h AH=31h KEEP (Terminate and Stay Resident; beads initech-bo40). Make the
 * block currently owned by `psp_owner` RESIDENT at a size of `keep_data_paras`
 * DATA paragraphs: SHRINK it to keep_data_paras (the freed tail becomes a free
 * block, exactly as mcb_setblock's shrink), then RE-OWN it to MCB_OWNER_SYSTEM so
 * the subsequent terminate-time mcb_free_owner(psp_owner) SKIPS it and a later
 * mcb_alloc never hands the kept region out. `psp_owner` must be a live owner (not
 * MCB_OWNER_FREE / MCB_OWNER_SYSTEM); the resized block is the FIRST block that
 * psp_owner owns (the authentic single-big-block a freshly-loaded program owns --
 * the PSP+image window the loader hands it via mcb_set_arena_owner).
 *
 * Semantics (DOS 3.3 AH=31h; RBIL INT 21/AH=31h; MS-DOS 3.3 Tech Ref):
 *   - keep_data_paras > the block's current data size -> CLAMPED to the current
 *     size (DOS keeps at most the program's own block; it never grows on KEEP).
 *   - keep_data_paras == current size -> the block is re-owned resident, no split.
 *   - keep_data_paras < current size -> shrink (free the tail) then re-own.
 * Returns MCB_OK on success; MCB_ERR_BAD_BLOCK if psp_owner owns no block (the
 * "KEEP on a process owning no arena block" invariant -- the do_keep caller fails
 * loud), or if psp_owner is FREE/SYSTEM; MCB_ERR_DESTROYED on a corrupt chain.
 * Coalesces the freed tail afterward (Rule 2: leave the chain in normal form). */
uint16_t mcb_keep_resident(mcb_arena_t *a, uint16_t psp_owner,
                           uint32_t keep_data_paras);

/* Walk the chain and assert the structural invariants: every header signature is
 * 'M' or 'Z'; no block runs past the arena; exactly one 'Z' and it is last; the
 * blocks tile the arena exactly (sum of 1+size == total_paras). Returns 1 if the
 * chain is intact, 0 if corrupt. For the oracle + fail-loud entry guards. */
int mcb_chain_intact(const mcb_arena_t *a);

#endif /* INITECH_OS_MCB_H */

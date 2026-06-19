/*
 * os/flair/heap.h -- the FLAIR Toolbox heap allocator (the artifact).
 *
 * beads: initech-k8o5.5 (DEC-03 allocator deliverable; the spec-lock + boot
 *        probe landed first, commit 033e2fd).
 * Ref:   ADR-0004 Sec 8.1 DEC-03 *Allocator* (verbatim intent): "A single flat
 *        arena owned by FLAIR over the fixed [FLAIR_HEAP_BASE,
 *        FLAIR_HEAP_BASE+FLAIR_HEAP_SIZE) window: bump-pointer for allocation,
 *        typed free-list per allocation class (region rows[]/x_pool backing,
 *        indexed-8 offscreen bitmaps, WindowRecord/MenuInfo/ControlRecord/
 *        DialogRecord handles, NFNT strikes) for frees -- matching PRD Sec 5
 *        'bump + free-list allocator'. NO per-row / per-handle malloc (Rule 11
 *        deterministic layout; freestanding, no libc). Fail-loud (Rule 2) on
 *        exhaustion -- never silent truncation."
 * Ref:   spec/memory_map.h -- the LOCKED FLAIR_HEAP_BASE (0x00100000) /
 *        FLAIR_HEAP_SIZE (0x00400000) / FLAIR_HEAP_MIN window. The kernel binds
 *        this allocator to that window by passing the constants to
 *        flair_heap_init; this allocator NEVER hardcodes 0x100000 (it takes a
 *        caller-supplied base+size, exactly as the ATKINSON region engine takes
 *        caller-supplied rows[]/x_pool storage -- which is what makes it
 *        host-testable).
 * Ref:   PRD Sec 5 ("bump + free-list allocator").
 *        CLAUDE.md Law 1 (ground truth before code), Law 2 (the oracle is the
 *        truth -- harness/proptest/test_flair_heap.c), Law 3 (freestanding
 *        artifact; dual-compile for the host oracle), Rule 2 (fail loud: NULL on
 *        exhaustion, never silent truncation/wrap; bounds-checked), Rule 11
 *        (deterministic layout: the same call sequence yields the identical
 *        layout every run -- the self-host fixpoint K2==K3 depends on it),
 *        Rule 12 (ASCII-clean source).
 *
 * ARTIFACT code: freestanding, <stdint.h> only, no libc, no malloc. Compiles
 * BOTH under kernel flags (gcc -m32 -ffreestanding -nostdlib -std=c11 -Wall
 * -Wextra -Werror) AND hosted for the property suite (the dual-compile pattern
 * region.c/surface.c follow; CLAUDE.md Law 3).
 *
 * THE MODEL (DEC-03)
 * ------------------
 * One FLAIR-owned flat arena over a caller-supplied [base, base+size) window.
 *
 *   - BUMP-POINTER for fresh allocations: a monotonic cursor advances from
 *     `base`, 16-byte aligned (see FLAIR_HEAP_ALIGN). Every block also carries a
 *     small in-band header (flair_blk_t) so a freed block can be re-pushed onto
 *     its class free-list and reused by a later same-class request.
 *
 *   - TYPED FREE-LISTS, ONE PER CLASS (flair_class_t): flair_free pushes a block
 *     onto its class's singly-linked free-list (LIFO). flair_alloc of class C
 *     first tries to REUSE the head of class C's free-list IF the request fits
 *     the freed block's capacity; otherwise it BUMPS a fresh block. A freed
 *     block is NEVER handed to a different class (CLASS ISOLATION) -- the typed
 *     free-list is the per-class reuse pool, exactly as DEC-03 specifies.
 *
 *   - FAIL LOUD ON EXHAUSTION (Rule 2): when neither a fitting same-class
 *     free-list block exists NOR a fresh bump fits inside the window, flair_alloc
 *     returns NULL. It NEVER truncates, wraps, or scribbles past base+size. The
 *     in-kernel caller treats NULL as a panic-worthy condition (PC LOAD LETTER);
 *     the host oracle asserts NULL.
 *
 * DETERMINISM (Rule 11): base is caller-supplied and the bump cursor + the LIFO
 * free-list policy are pure functions of the call sequence -- no timestamps, no
 * nondeterminism, no address-derived hashing. Two fresh heaps init'd over two
 * buffers and driven by the SAME call sequence produce the SAME *relative*
 * layout (identical offsets-from-base for every allocation). The kernel binds
 * the fixed spec window, so its absolute layout is identical every boot.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_HEAP_H
#define INITECH_OS_FLAIR_HEAP_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * Allocation alignment.
 *
 * 16 bytes (one x86 "paragraph"). Chosen over 8 because the FLAIR allocation
 * classes include indexed-8 offscreen bitmaps and region row pools whose
 * scanline math is friendliest on a 16-byte boundary, and 16 is the natural
 * paragraph granularity already used across the conventional memory map
 * (spec/memory_map.h, MCB_PARA). Both the block header and every returned
 * payload pointer are 16-aligned. (DEC-03 left the alignment open: "choose a
 * sane alignment, e.g. 8 or 16 bytes; document it" -- 16 is the choice.)
 * -------------------------------------------------------------------------- */
#define FLAIR_HEAP_ALIGN 16u

/* --------------------------------------------------------------------------
 * Allocation classes (DEC-03: "typed free-list per allocation class").
 *
 * Each FLAIR allocation belongs to exactly one class. The class selects which
 * typed free-list a freed block returns to and, by CLASS ISOLATION, which
 * requests may reuse it. The set mirrors DEC-03's enumeration of what FLAIR
 * allocates from this arena:
 *
 *   FLAIR_CLASS_REGION  -- ATKINSON region storage (rows[] + x_pool backing).
 *   FLAIR_CLASS_BITMAP  -- indexed-8 offscreen bitmaps (back buffers,
 *                          save-unders; OD-2).
 *   FLAIR_CLASS_HANDLE  -- Manager records: WindowRecord / MenuInfo /
 *                          ControlRecord / DialogRecord handles.
 *   FLAIR_CLASS_STRIKE  -- NFNT font strikes.
 *   FLAIR_CLASS_GENERAL -- everything else FLAIR needs from the arena.
 *
 * FLAIR_CLASS_COUNT is the free-list array length; it is NOT a usable class.
 * -------------------------------------------------------------------------- */
typedef enum {
    FLAIR_CLASS_REGION = 0,
    FLAIR_CLASS_BITMAP = 1,
    FLAIR_CLASS_HANDLE = 2,
    FLAIR_CLASS_STRIKE = 3,
    FLAIR_CLASS_GENERAL = 4,
    FLAIR_CLASS_COUNT = 5
} flair_class_t;

/* --------------------------------------------------------------------------
 * flair_blk_t -- the in-band block header that precedes every payload.
 *
 * Laid down by the bump path; reused intact when a block is recycled. `cap` is
 * the aligned payload capacity in bytes (the bump path may round a request up to
 * FLAIR_HEAP_ALIGN, so a recycled block can satisfy any request <= cap). `klass`
 * records the owning class so flair_free can route the block to the right
 * free-list WITHOUT trusting the caller's class argument blindly (Rule 2: a
 * cross-class free is rejected). `next` links the per-class free-list (LIFO);
 * it is meaningful only while the block sits on a free-list.
 *
 * The header is itself padded to FLAIR_HEAP_ALIGN so the payload that follows it
 * is 16-aligned (see FLAIR_HEAP_HDR_BYTES in heap.c).
 * -------------------------------------------------------------------------- */
typedef struct flair_blk {
    uint32_t          cap;     /* aligned payload capacity in bytes            */
    uint32_t          klass;   /* owning flair_class_t (set at bump time)      */
    struct flair_blk *next;    /* next on the class free-list (LIFO); else 0   */
} flair_blk_t;

/* --------------------------------------------------------------------------
 * flair_heap_t -- one FLAIR-owned flat arena. Caller supplies the storage
 * window; this struct holds only the bookkeeping (no embedded buffer), so the
 * kernel binds the fixed spec window and the host oracle binds a malloc'd or
 * static buffer with the SAME code (the dual-compile / caller-supplied-storage
 * pattern, region.h).
 * -------------------------------------------------------------------------- */
typedef struct {
    uint8_t     *base;    /* window base (caller-supplied)                     */
    uint32_t     size;    /* window size in bytes (caller-supplied)           */
    uint32_t     used;    /* bump cursor: bytes consumed from base (aligned)  */
    flair_blk_t *freelist[FLAIR_CLASS_COUNT]; /* per-class LIFO free-lists     */
    /* Stats (deterministic; for the oracle + future diagnostics, never used in
     * an allocation decision -- so they cannot perturb the layout, Rule 11). */
    uint32_t     n_bump;  /* count of fresh bump allocations                  */
    uint32_t     n_reuse; /* count of free-list reuse allocations             */
    uint32_t     n_free;  /* count of frees                                   */
} flair_heap_t;

/* --------------------------------------------------------------------------
 * flair_heap_init -- bind a heap to a caller-supplied [base, size) window.
 *
 * Resets the bump cursor to 0 and clears every free-list. `base` MUST be
 * 16-aligned and `size` SHOULD be a multiple of FLAIR_HEAP_ALIGN; the allocator
 * never writes outside [base, base+size). The kernel calls this with
 * (FLAIR_HEAP_BASE, FLAIR_HEAP_SIZE) from spec/memory_map.h; the host oracle
 * calls it with its own backing buffer. No allocation, no libc.
 * -------------------------------------------------------------------------- */
void flair_heap_init(flair_heap_t *heap, void *base, uint32_t size);

/* --------------------------------------------------------------------------
 * flair_alloc -- allocate `size` payload bytes for allocation class `klass`.
 *
 * Policy (DEC-03, deterministic -- Rule 11):
 *   1. REUSE: if class `klass`'s free-list head has cap >= the aligned request,
 *      pop it and return its payload (CLASS ISOLATION -- only this class's list
 *      is consulted).
 *   2. BUMP: else advance the bump cursor by the aligned (header + payload) span
 *      if it fits inside [base, base+size); return the fresh payload.
 *   3. FAIL LOUD: else return NULL (Rule 2 -- never truncate/wrap/overrun).
 *
 * Returns a 16-aligned payload pointer, or NULL on exhaustion or a bad argument
 * (size==0, klass out of range). The returned bytes are NOT zeroed (the caller
 * owns initialization, as with a freestanding bump arena).
 * -------------------------------------------------------------------------- */
void *flair_alloc(flair_heap_t *heap, flair_class_t klass, uint32_t size);

/* --------------------------------------------------------------------------
 * flair_free -- return a block previously returned by flair_alloc to its class
 * free-list.
 *
 * `klass` MUST match the class the block was allocated with; `ptr` MUST be a
 * payload pointer this heap returned. flair_free pushes the block onto class
 * `klass`'s LIFO free-list for later same-class reuse. Fail loud (Rule 2): a
 * NULL ptr, a ptr outside the window, a misaligned ptr, or a class mismatch is
 * REJECTED (no-op) rather than corrupting a free-list. Returns 1 on success,
 * 0 on a rejected free.
 *
 * Freeing does NOT roll the bump cursor back (a bump arena does not compact);
 * reuse comes via the free-list. Double-free is the caller's contract; this
 * allocator does not (and need not, for the cooperative single-threaded FLAIR
 * model) walk the list to detect it.
 * -------------------------------------------------------------------------- */
int flair_free(flair_heap_t *heap, flair_class_t klass, void *ptr);

/* --------------------------------------------------------------------------
 * flair_heap_avail -- bytes still reachable by a FRESH bump (the tail), for
 * diagnostics / the oracle. This is NOT the total free memory (free-listed
 * blocks are reusable only within their class and are not counted here). Pure,
 * side-effect-free.
 * -------------------------------------------------------------------------- */
uint32_t flair_heap_avail(const flair_heap_t *heap);

#endif /* INITECH_OS_FLAIR_HEAP_H */

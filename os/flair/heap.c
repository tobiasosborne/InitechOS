/*
 * os/flair/heap.c -- the FLAIR Toolbox heap allocator (the artifact).
 *
 * beads: initech-k8o5.5 (DEC-03 allocator deliverable). See heap.h for the full
 *        model, the class set, and the Law/ADR/PRD citations.
 * Ref:   ADR-0004 Sec 8.1 DEC-03 *Allocator*; spec/memory_map.h (the LOCKED
 *        FLAIR_HEAP_BASE/SIZE window the kernel binds via flair_heap_init);
 *        PRD Sec 5 ("bump + free-list allocator").
 *        CLAUDE.md Law 2 (the oracle -- harness/proptest/test_flair_heap.c -- is
 *        the truth), Rule 2 (fail loud: NULL on exhaustion, bounds-checked,
 *        never silent truncation/wrap), Rule 11 (deterministic layout), Rule 12
 *        (ASCII).
 *
 * Freestanding: <stdint.h> only, no libc, no malloc. Dual-compiles under kernel
 * flags AND hosted (Law 3). The property suite is the oracle (Law 2); two named
 * mutants (FLAIR_HEAP_MUTATE_NO_BOUNDS, FLAIR_HEAP_MUTATE_NO_REUSE) prove it
 * BITES (Rule 6) -- NEVER define either in a real build.
 */
#include "heap.h"

/* Header bytes that precede every payload, rounded UP to FLAIR_HEAP_ALIGN so the
 * payload pointer (header end) is itself 16-aligned. The block header is small
 * (two u32 + one pointer); rounding makes the layout alignment-exact regardless
 * of pointer width (4 bytes freestanding, 8 hosted) -- which keeps the host
 * oracle and the kernel layout in the SAME alignment class (Rule 11). */
#define FLAIR_HEAP_HDR_BYTES \
    ((uint32_t)((sizeof(flair_blk_t) + (FLAIR_HEAP_ALIGN - 1u)) \
                & ~(uint32_t)(FLAIR_HEAP_ALIGN - 1u)))

/* Round `n` up to the next multiple of FLAIR_HEAP_ALIGN. Pure, overflow-safe for
 * any n that already passed the request-sanity check below. */
static uint32_t flair_align_up(uint32_t n)
{
    return (n + (FLAIR_HEAP_ALIGN - 1u)) & ~(uint32_t)(FLAIR_HEAP_ALIGN - 1u);
}

static int flair_class_ok(flair_class_t klass)
{
    return (klass >= FLAIR_CLASS_REGION) && (klass < FLAIR_CLASS_COUNT);
}

void flair_heap_init(flair_heap_t *heap, void *base, uint32_t size)
{
    heap->base = (uint8_t *)base;
    heap->size = size;
    heap->used = 0u;
    for (int c = 0; c < FLAIR_CLASS_COUNT; c++) {
        heap->freelist[c] = (flair_blk_t *)0;
    }
    heap->n_bump  = 0u;
    heap->n_reuse = 0u;
    heap->n_free  = 0u;
}

/* The header that precedes a payload pointer this heap returned. */
static flair_blk_t *flair_hdr_of(uint8_t *payload)
{
    return (flair_blk_t *)(payload - FLAIR_HEAP_HDR_BYTES);
}

/* Is `payload` a plausible block this heap returned? It must lie inside the
 * window, leave room for its header below it, and be 16-aligned. This is the
 * bounds + sanity guard for flair_free (Rule 2). */
static int flair_payload_in_window(const flair_heap_t *heap, const uint8_t *payload)
{
    uintptr_t b   = (uintptr_t)heap->base;
    uintptr_t end = b + (uintptr_t)heap->used;   /* one past the last bumped byte */
    uintptr_t p   = (uintptr_t)payload;

    if (p < b + (uintptr_t)FLAIR_HEAP_HDR_BYTES) {
        return 0;                                 /* no room for a header below   */
    }
    if (p >= end) {
        return 0;                                 /* beyond what was ever bumped  */
    }
    if ((p & (uintptr_t)(FLAIR_HEAP_ALIGN - 1u)) != 0u) {
        return 0;                                 /* not a 16-aligned payload     */
    }
    return 1;
}

void *flair_alloc(flair_heap_t *heap, flair_class_t klass, uint32_t size)
{
    if (size == 0u || !flair_class_ok(klass)) {
        return (void *)0;                         /* bad request -> fail loud     */
    }

    uint32_t need = flair_align_up(size);         /* aligned payload bytes        */

    /* ---- 1. REUSE: pop a fitting block off THIS class's free-list (LIFO).
     *      CLASS ISOLATION -- only freelist[klass] is consulted, so a freed
     *      REGION block can never satisfy a BITMAP request. -------------------*/
#ifndef FLAIR_HEAP_MUTATE_NO_REUSE
    {
        flair_blk_t *blk = heap->freelist[klass];
        flair_blk_t *prev = (flair_blk_t *)0;
        while (blk != (flair_blk_t *)0) {
            if (blk->cap >= need) {
                /* unlink blk */
                if (prev == (flair_blk_t *)0) {
                    heap->freelist[klass] = blk->next;
                } else {
                    prev->next = blk->next;
                }
                blk->next = (flair_blk_t *)0;
                heap->n_reuse++;
                return (void *)((uint8_t *)blk + FLAIR_HEAP_HDR_BYTES);
            }
            prev = blk;
            blk = blk->next;
        }
    }
#else
    /* MUTANT (Rule 6; make test-flair-heap-mutant only): NEVER reuse a freed
     * block -- always bump. The free-list-reuse oracle then goes RED because a
     * free-then-same-size-alloc returns a FRESH bumped pointer (the bump cursor
     * advances) instead of the recycled block. NEVER define in a real build. */
#endif

    /* ---- 2. BUMP a fresh (header + payload) span if it fits the window. ----*/
    {
        uint32_t span = FLAIR_HEAP_HDR_BYTES + need;
        /* Overflow-safe fit test: compute remaining tail, compare without
         * adding into a possibly-wrapping sum (Rule 2 -- no wrap). */
#ifndef FLAIR_HEAP_MUTATE_NO_BOUNDS
        uint32_t avail = (heap->size > heap->used)
                       ? (heap->size - heap->used) : 0u;
        if (span > avail) {
            return (void *)0;                     /* EXHAUSTED -> fail loud       */
        }
#else
        /* MUTANT (Rule 6; make test-flair-heap-mutant only): DROP the exhaustion
         * bounds check, so an over-window bump "succeeds" and hands out a pointer
         * past base+size. The exhaustion oracle then goes RED (it expects NULL
         * and instead gets a non-NULL out-of-window pointer). NEVER define in a
         * real build. */
#endif
        {
            flair_blk_t *blk = (flair_blk_t *)(heap->base + heap->used);
            blk->cap   = need;
            blk->klass = (uint32_t)klass;
            blk->next  = (flair_blk_t *)0;
            heap->used += span;
            heap->n_bump++;
            return (void *)((uint8_t *)blk + FLAIR_HEAP_HDR_BYTES);
        }
    }
}

int flair_free(flair_heap_t *heap, flair_class_t klass, void *ptr)
{
    if (ptr == (void *)0 || !flair_class_ok(klass)) {
        return 0;                                 /* bad free -> reject (Rule 2)  */
    }
    if (!flair_payload_in_window(heap, (const uint8_t *)ptr)) {
        return 0;                                 /* not a block we handed out    */
    }

    flair_blk_t *blk = flair_hdr_of((uint8_t *)ptr);

    /* CLASS-MATCH guard (Rule 2): the block records its own class at bump time;
     * a free with the wrong class is rejected so a cross-class free can never
     * poison the wrong free-list (which would later hand a REGION block to a
     * BITMAP request -- a class-isolation violation). */
    if (blk->klass != (uint32_t)klass) {
        return 0;
    }

    /* Push onto the class free-list (LIFO). */
    blk->next = heap->freelist[klass];
    heap->freelist[klass] = blk;
    heap->n_free++;
    return 1;
}

uint32_t flair_heap_avail(const flair_heap_t *heap)
{
    return (heap->size > heap->used) ? (heap->size - heap->used) : 0u;
}

/*
 * os/flair/heap.c -- the FLAIR Toolbox heap allocator (the artifact).
 *
 *  (arena lifetime allocator deliverable). See heap.h for the full
 *        model, the class set, and the Law/ADR/PRD citations.
 * Ref:   clipped Toolbox drawing arena lifetime *Allocator*; spec/memory_map.h (the LOCKED
 *        FLAIR_HEAP_BASE/SIZE window the kernel binds via flair_heap_init);
 *        PRD Sec 5 ("bump + free-list allocator").
 *        independent checks (the oracle -- harness/proptest/test_flair_heap.c -- is
 *        the truth), explicit error handling (fail loud: NULL on exhaustion, bounds-checked,
 *        never silent truncation/wrap), reproducibility (deterministic layout), ASCII source
 *        (ASCII).
 *
 * Freestanding: <stdint.h> only, no libc, no malloc. Dual-compiles under kernel
 * flags AND hosted. The property suite is the oracle; two named
 * mutants (no bounds patch, no reuse patch) prove it
 * BITES  -- NEVER define either in a real build.
 */
#include "heap.h"

/* Header bytes that precede every payload, rounded UP to FLAIR_HEAP_ALIGN so the
 * payload pointer (header end) is itself 16-aligned. The block header is small
 * (two u32 + one pointer); rounding makes the layout alignment-exact regardless
 * of pointer width (4 bytes freestanding, 8 hosted) -- which keeps the host
 * oracle and the kernel layout in the SAME alignment class. */
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
 * bounds + sanity guard for flair_free. */
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
    /* both the alignment addition AND the header span must fit.
     * Ref: heap.h arena lifetime bounds contract; reject before any arena mutation. */
    if (size > UINT32_MAX - (FLAIR_HEAP_ALIGN - 1u)) return (void *)0;

    uint32_t need = flair_align_up(size);         /* aligned payload bytes        */
    if (need > UINT32_MAX - FLAIR_HEAP_HDR_BYTES) return (void *)0;

    /* ---- 1. REUSE: pop a fitting block off THIS class's free-list (LIFO).
     *      CLASS ISOLATION -- only freelist[klass] is consulted, so a freed
     *      REGION block can never satisfy a BITMAP request. -------------------*/
    {
        flair_blk_t *blk = heap->freelist[klass];
        /* arena lifetime specifies head-only LIFO, not first-fit search. */
        if (blk != (flair_blk_t *)0 && blk->cap >= need) {
            heap->freelist[klass] = blk->next;
            blk->next = (flair_blk_t *)0;
            heap->n_reuse++;
            return (void *)((uint8_t *)blk + FLAIR_HEAP_HDR_BYTES);
        }
    }

    /* ---- 2. BUMP a fresh (header + payload) span if it fits the window. ----*/
    {
        uint32_t span = FLAIR_HEAP_HDR_BYTES + need;
        /* Overflow-safe fit test: compute remaining tail, compare without
         * adding into a possibly-wrapping sum (explicit error handling -- no wrap). */
        uint32_t avail = (heap->size > heap->used)
                       ? (heap->size - heap->used) : 0u;
        if (span > avail) {
            return (void *)0;                     /* EXHAUSTED -> fail loud       */
        }
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
        return 0;                                 /* bad free -> reject   */
    }
    if (!flair_payload_in_window(heap, (const uint8_t *)ptr)) {
        return 0;                                 /* not a block we handed out    */
    }

    flair_blk_t *blk = flair_hdr_of((uint8_t *)ptr);

    /* CLASS-MATCH guard: the block records its own class at bump time;
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

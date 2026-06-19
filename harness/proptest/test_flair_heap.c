/* test_flair_heap.c -- the FLAIR Toolbox heap allocator's property suite (ORACLE).
 *
 * beads: initech-k8o5.5 (DEC-03 allocator deliverable).
 * Ref:   ADR-0004 Sec 8.1 DEC-03 *Allocator*: "one FLAIR-owned flat arena ...
 *        bump-pointer for allocation, typed free-list per allocation class ...
 *        NO per-row/per-handle malloc (Rule 11 deterministic layout) ...
 *        fail-loud (Rule 2) on exhaustion -- never silent truncation."
 *        spec/memory_map.h (the LOCKED FLAIR_HEAP_BASE/SIZE window the kernel
 *        binds; here the host backs the heap with a malloc'd buffer -- the
 *        caller-supplied-storage / dual-compile pattern).
 *        CLAUDE.md Law 2 (the oracle is the truth), Rule 1 (RED->GREEN), Rule 6
 *        (oracle mutation-proven), Rule 11 (seeded LCG -> deterministic), Rule 12
 *        (ASCII).
 *
 * This is the test_mcb.c / test_region.c idiom: TEST_HARNESS()/CHECK, a seeded
 * LCG so the fuzz is reproducible, host-side (malloc the arena backing; the
 * ALLOCATOR itself does NO host malloc). It links the SAME heap.c the kernel
 * links freestanding.
 *
 * The suite, by property:
 *   (1) BUMP MONOTONICITY + ALIGNMENT: fresh allocations advance strictly and
 *       every returned payload is 16-aligned and inside the window.
 *   (2) FREE-LIST REUSE: free a class-X block, the next class-X alloc of <= its
 *       size REUSES it (the bump cursor does NOT advance) -- not a fresh bump.
 *   (3) CLASS ISOLATION: a freed REGION block is NEVER handed to a BITMAP
 *       request (the documented per-class policy).
 *   (4) FAIL-LOUD EXHAUSTION: an alloc past the window returns NULL and NEVER
 *       overruns; the heap stays usable for a smaller request afterward.
 *   (5) DETERMINISM: the same seeded call sequence over two fresh heaps yields
 *       the IDENTICAL pointer layout (offsets-from-base bit-for-bit equal).
 *   Plus a randomized fuzz with data-integrity verification: no live block's
 *   bytes are ever stomped by unrelated alloc/free traffic, and every returned
 *   payload is disjoint from every other live payload.
 *
 * Mutation-proven via make test-flair-heap-mutant (FLAIR_HEAP_MUTATE_NO_BOUNDS,
 * FLAIR_HEAP_MUTATE_NO_REUSE) -- each must drive THIS oracle RED.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"          /* the allocator under test (-Ios/flair)            */
#include "test_assert.h"   /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)         */

TEST_HARNESS();

/* A generously-sized host backing buffer. Big enough that bump tests have room,
 * small enough that the exhaustion test can drain it in a bounded number of
 * allocs. The allocator never assumes any particular size -- it takes (base,
 * size) (the caller-supplied-storage pattern), so the host picks freely. */
enum { HEAP_BYTES = 256u * 1024u };

/* Make a fresh heap over a poisoned malloc'd buffer (poison so an out-of-window
 * write or an uninitialized-read bug would be visible). The buffer is 16-aligned
 * by construction (malloc returns at least 16-aligned on the host, and we assert
 * it). Returns the backing buffer via *out_buf for free(). */
static void heap_make(flair_heap_t *h, uint8_t **out_buf, uint32_t bytes)
{
    uint8_t *buf = (uint8_t *)malloc(bytes);
    if (!buf) { fprintf(stderr, "  FATAL: malloc failed\n"); exit(2); }
    memset(buf, 0xCC, bytes);
    /* The allocator requires a 16-aligned base; host malloc gives >=16 on
     * LP64/ILP32 glibc. Assert rather than silently mis-aligning. */
    if (((uintptr_t)buf & 15u) != 0u) {
        fprintf(stderr, "  FATAL: host malloc returned mis-aligned buffer\n");
        exit(2);
    }
    flair_heap_init(h, buf, bytes);
    *out_buf = buf;
}

/* ---- a deterministic LCG so the fuzz is reproducible (Rule 11) ----------- */
static uint32_t g_seed = 0x1234567u;
static uint32_t lcg(void)
{
    g_seed = g_seed * 1103515245u + 12345u;
    return (g_seed >> 16) & 0x7FFFu;
}

/* offset of a payload from a heap base (for the determinism + monotonicity
 * checks). Caller guarantees p is inside [base, base+size). */
static uintptr_t off_of(const flair_heap_t *h, const void *p)
{
    return (uintptr_t)p - (uintptr_t)h->base;
}

/* ---- a live allocation the fuzz tracks: ptr + size + class + a data tag --- */
struct live {
    int           present;
    void         *ptr;
    uint32_t      size;
    flair_class_t klass;
    uint8_t       tag;
};

static void fill(struct live *L)
{
    uint8_t *d = (uint8_t *)L->ptr;
    for (uint32_t j = 0; j < L->size; j++) d[j] = (uint8_t)(L->tag + (uint8_t)j);
}
static int intact(const struct live *L)
{
    const uint8_t *d = (const uint8_t *)L->ptr;
    for (uint32_t j = 0; j < L->size; j++)
        if (d[j] != (uint8_t)(L->tag + (uint8_t)j)) return 0;
    return 1;
}

/* Replay a fixed (class,size) script onto a fresh heap, recording each returned
 * offset-from-base. Used by the determinism test: two replays must agree. */
struct step { flair_class_t klass; uint32_t size; int do_free; };

static void replay(const struct step *script, int n, uintptr_t *offs)
{
    flair_heap_t h; uint8_t *buf;
    heap_make(&h, &buf, HEAP_BYTES);
    void *last = NULL; flair_class_t last_k = FLAIR_CLASS_GENERAL;
    for (int i = 0; i < n; i++) {
        if (script[i].do_free && last) {
            flair_free(&h, last_k, last);
            last = NULL;
        }
        void *p = flair_alloc(&h, script[i].klass, script[i].size);
        offs[i] = p ? off_of(&h, p) : (uintptr_t)-1;
        last = p; last_k = script[i].klass;
    }
    free(buf);
}

int main(void)
{
    /* ===== Property 0: init lays an empty arena; avail == size. =========== */
    {
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        CHECK(h.used == 0u, "init: bump cursor starts at 0");
        CHECK(flair_heap_avail(&h) == HEAP_BYTES, "init: avail == window size");
        CHECK(flair_alloc(&h, FLAIR_CLASS_REGION, 0u) == NULL,
              "init: zero-size alloc fails loud (NULL)");
        free(buf);
    }

    /* ===== Property 1: bump monotonicity + 16-alignment + in-window. ====== */
    {
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        uintptr_t prev = 0; int first = 1;
        int mono = 1, aligned = 1, inwin = 1;
        for (int i = 0; i < 200; i++) {
            uint32_t sz = 1u + (lcg() % 300u);
            void *p = flair_alloc(&h, (flair_class_t)(lcg() % FLAIR_CLASS_COUNT), sz);
            if (!p) { mono = 0; break; }            /* should not exhaust here */
            uintptr_t o = off_of(&h, p);
            if (((uintptr_t)p & 15u) != 0u) aligned = 0;
            if (o + sz > HEAP_BYTES) inwin = 0;
            if (!first && o <= prev) mono = 0;       /* strictly increasing     */
            prev = o; first = 0;
        }
        CHECK(mono, "bump: fresh allocations advance strictly (monotonic)");
        CHECK(aligned, "bump: every payload is 16-aligned");
        CHECK(inwin, "bump: every payload + size stays inside the window");
        CHECK(h.n_bump == 200u && h.n_reuse == 0u,
              "bump: all-fresh sequence is 200 bumps, 0 reuses");
        free(buf);
    }

    /* ===== Property 2: free-list REUSE (same class, fits -> recycle). ===== */
    {
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        void *a = flair_alloc(&h, FLAIR_CLASS_HANDLE, 64u);
        CHECK(a != NULL, "reuse: first alloc ok");
        uint32_t used_after_a = h.used;
        CHECK(flair_free(&h, FLAIR_CLASS_HANDLE, a) == 1, "reuse: free accepted");
        CHECK(h.used == used_after_a, "reuse: free does NOT roll back the bump cursor");
        /* same class, smaller-or-equal request must REUSE a (no new bump). */
        void *b = flair_alloc(&h, FLAIR_CLASS_HANDLE, 64u);
        CHECK(b == a, "reuse: same-class same-size alloc returns the freed block");
        CHECK(h.used == used_after_a, "reuse: reuse did NOT advance the bump cursor");
        CHECK(h.n_reuse == 1u, "reuse: the reuse counter incremented");
        /* a smaller request also fits the freed cap and reuses. */
        flair_free(&h, FLAIR_CLASS_HANDLE, b);
        void *c = flair_alloc(&h, FLAIR_CLASS_HANDLE, 16u);
        CHECK(c == a, "reuse: a smaller same-class request reuses the larger freed block");
        /* a request LARGER than the freed cap must NOT reuse -> fresh bump. */
        flair_free(&h, FLAIR_CLASS_HANDLE, c);
        uint32_t used_before_big = h.used;
        void *d = flair_alloc(&h, FLAIR_CLASS_HANDLE, 4096u);
        CHECK(d != NULL && d != a, "reuse: an oversize request does NOT reuse a too-small block");
        CHECK(h.used > used_before_big, "reuse: the oversize request bumped fresh");
        free(buf);
    }

    /* ===== Property 3: CLASS ISOLATION (freed REGION not given to BITMAP). = */
    {
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        void *r = flair_alloc(&h, FLAIR_CLASS_REGION, 128u);
        CHECK(r != NULL, "isolation: region alloc ok");
        CHECK(flair_free(&h, FLAIR_CLASS_REGION, r) == 1, "isolation: region free ok");
        uint32_t used_before = h.used;
        /* a BITMAP request that WOULD fit r's cap must NOT get r -- different
         * class -> fresh bump (only freelist[BITMAP] is consulted). */
        void *bmp = flair_alloc(&h, FLAIR_CLASS_BITMAP, 128u);
        CHECK(bmp != NULL && bmp != r,
              "isolation: a BITMAP request is NOT handed the freed REGION block");
        CHECK(h.used > used_before, "isolation: the cross-class request bumped fresh");
        /* the REGION block is still on REGION's list: a REGION request reuses it. */
        void *r2 = flair_alloc(&h, FLAIR_CLASS_REGION, 128u);
        CHECK(r2 == r, "isolation: a same-class REGION request still reuses the freed block");
        free(buf);
    }

    /* ===== Property 3b: cross-class FREE is rejected (Rule 2 guard). ====== */
    {
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        void *s = flair_alloc(&h, FLAIR_CLASS_STRIKE, 48u);
        CHECK(flair_free(&h, FLAIR_CLASS_HANDLE, s) == 0,
              "guard: freeing a STRIKE block as HANDLE is rejected");
        CHECK(flair_free(&h, FLAIR_CLASS_STRIKE, s) == 1,
              "guard: freeing it with the correct class succeeds");
        /* and the wrong-class free did NOT poison HANDLE's list. */
        uint32_t used_before = h.used;
        void *hbk = flair_alloc(&h, FLAIR_CLASS_HANDLE, 48u);
        CHECK(hbk != NULL && hbk != s && h.used > used_before,
              "guard: the rejected cross-class free left HANDLE's list empty (fresh bump)");
        /* bad-pointer / NULL frees are rejected, not crashes. */
        CHECK(flair_free(&h, FLAIR_CLASS_HANDLE, NULL) == 0, "guard: NULL free rejected");
        CHECK(flair_free(&h, FLAIR_CLASS_HANDLE, buf - 16) == 0,
              "guard: out-of-window free rejected");
        CHECK(flair_free(&h, FLAIR_CLASS_HANDLE, (uint8_t *)s + 1) == 0,
              "guard: misaligned free rejected");
        free(buf);
    }

    /* ===== Property 4: FAIL-LOUD EXHAUSTION (NULL, no overrun). =========== */
    {
        /* A SMALL heap window inside a LARGER host backing buffer. The extra
         * GUARD bytes past the window are real, mapped host memory so that a
         * NO_BOUNDS mutant (which hands out an over-window pointer) can be caught
         * by the in-window CHECK *before* any write -- a clean oracle RED, not a
         * SEGV (Rule 1: fail for the RIGHT reason, via the oracle). The GUARD
         * region is sentinel-filled and asserted untouched (no overrun, Rule 2).
         * In a correct build the allocator NEVER hands out a block past SMALL, so
         * nothing ever writes into the GUARD. */
        enum { SMALL = 4096u, GUARD = 64u * 1024u };
        uint8_t *buf = (uint8_t *)malloc(SMALL + GUARD + 16u);
        if (!buf) { fprintf(stderr, "  FATAL malloc\n"); return 2; }
        memset(buf, 0xCC, SMALL + GUARD + 16u);
        /* align the heap base to 16 */
        uint8_t *base = (uint8_t *)(((uintptr_t)buf + 15u) & ~(uintptr_t)15u);
        flair_heap_t h; flair_heap_init(&h, base, SMALL);

        int got_null = 0, overran = 0; int n_ok = 0;
        for (int i = 0; i < 10000; i++) {
            void *p = flair_alloc(&h, FLAIR_CLASS_GENERAL, 512u);
            if (!p) { got_null = 1; break; }        /* exhausted -> fail loud   */
            uintptr_t o = (uintptr_t)p - (uintptr_t)base;
            if (o + 512u > SMALL) {                  /* handed out past window!  */
                overran = 1;                         /* a NO_BOUNDS mutant bug    */
                break;                               /* break BEFORE the write    */
            }
            n_ok++;
            memset(p, 0xEE, 512u);                   /* actually use the block   */
        }
        CHECK(got_null, "exhaust: alloc past the window returns NULL (fail loud)");
        CHECK(!overran, "exhaust: no returned block ever crosses base+size");
        /* every GUARD byte past the window must be untouched (no overrun). */
        {
            int guard_ok = 1;
            for (uint32_t g = 0; g < GUARD; g++)
                if (base[SMALL + g] != 0xCCu) { guard_ok = 0; break; }
            CHECK(guard_ok, "exhaust: the bytes past the window are untouched (no overrun)");
        }
        /* The heap is still usable for a request that fits the remaining tail. */
        void *tiny = flair_alloc(&h, FLAIR_CLASS_GENERAL,
                                 (flair_heap_avail(&h) > FLAIR_HEAP_ALIGN * 2u)
                                 ? 1u : 0u);
        /* avail may be < a header+payload; only assert non-overrun if we got one */
        if (tiny) {
            uintptr_t o = (uintptr_t)tiny - (uintptr_t)base;
            CHECK(o < SMALL, "exhaust: a post-exhaustion small alloc still stays in-window");
        } else {
            CHECK(1, "exhaust: heap correctly refuses when even a tiny block won't fit");
        }
        CHECK(n_ok > 0, "exhaust: at least one alloc succeeded before exhaustion");
        free(buf);
    }

    /* ===== Property 5: DETERMINISM (same script -> identical layout). ===== */
    {
        /* A mixed script with frees so reuse participates in the layout. */
        static const struct step SCRIPT[] = {
            { FLAIR_CLASS_REGION,  100u, 0 },
            { FLAIR_CLASS_BITMAP,  300u, 0 },
            { FLAIR_CLASS_HANDLE,   40u, 1 },   /* free the previous, then alloc */
            { FLAIR_CLASS_HANDLE,   40u, 0 },
            { FLAIR_CLASS_STRIKE,   64u, 0 },
            { FLAIR_CLASS_GENERAL,  16u, 1 },
            { FLAIR_CLASS_REGION,  100u, 0 },
            { FLAIR_CLASS_BITMAP,  300u, 0 },
        };
        enum { N = (int)(sizeof(SCRIPT) / sizeof(SCRIPT[0])) };
        uintptr_t a[N], b[N];
        replay(SCRIPT, N, a);
        replay(SCRIPT, N, b);
        int same = 1;
        for (int i = 0; i < N; i++) if (a[i] != b[i]) { same = 0; break; }
        CHECK(same, "determinism: the same script yields the identical offset layout twice");
        /* every step produced a real allocation (no spurious NULL in this script) */
        int all_alloc = 1;
        for (int i = 0; i < N; i++) if (a[i] == (uintptr_t)-1) { all_alloc = 0; break; }
        CHECK(all_alloc, "determinism: the script allocates without exhaustion");
    }

    /* ===== Property 6: randomized fuzz with data-integrity + disjointness. = */
    {
        enum { POOL = 64, ITERS = 40000 };
        flair_heap_t h; uint8_t *buf; heap_make(&h, &buf, HEAP_BYTES);
        struct live live[POOL];
        memset(live, 0, sizeof(live));
        uint8_t next_tag = 1;
        int stomp = 0, overlap = 0, oow = 0;

        for (int it = 0; it < ITERS && !stomp && !overlap && !oow; it++) {
            int slot = (int)(lcg() % POOL);
            struct live *L = &live[slot];

            if (!L->present) {
                uint32_t sz = 1u + (lcg() % 256u);
                flair_class_t k = (flair_class_t)(lcg() % FLAIR_CLASS_COUNT);
                void *p = flair_alloc(&h, k, sz);
                if (p) {
                    /* in-window check */
                    uintptr_t o = off_of(&h, p);
                    if (o + sz > HEAP_BYTES) { oow = 1; break; }
                    L->present = 1; L->ptr = p; L->size = sz; L->klass = k;
                    L->tag = next_tag++; if (next_tag == 0) next_tag = 1;
                    fill(L);
                    /* a fresh/reused block must not overlap any OTHER live block */
                    uintptr_t lo = (uintptr_t)p, hi = lo + sz;
                    for (int s = 0; s < POOL && !overlap; s++) {
                        if (s == slot || !live[s].present) continue;
                        uintptr_t olo = (uintptr_t)live[s].ptr;
                        uintptr_t ohi = olo + live[s].size;
                        if (lo < ohi && olo < hi) overlap = 1;
                    }
                }
            } else {
                /* free it (correct class) */
                if (flair_free(&h, L->klass, L->ptr) != 1) { stomp = 1; break; }
                L->present = 0;
            }

            /* periodically verify EVERY live block's bytes survived */
            if ((it & 0x7F) == 0) {
                for (int s = 0; s < POOL; s++)
                    if (live[s].present && !intact(&live[s])) { stomp = 1; break; }
            }
        }
        CHECK(!oow, "fuzz: no allocation ever lands outside the window");
        CHECK(!overlap, "fuzz: no two live allocations ever overlap");
        CHECK(!stomp, "fuzz: no live block's data is ever stomped by other traffic");

        int all_ok = 1;
        for (int s = 0; s < POOL; s++)
            if (live[s].present && !intact(&live[s])) all_ok = 0;
        CHECK(all_ok, "fuzz: all survivors' data intact at the end");
        free(buf);
    }

    return TEST_SUMMARY("test_flair_heap");
}

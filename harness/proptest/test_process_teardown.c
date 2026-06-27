/* test_process_teardown.c -- the FLAIR App Contract O-3 teardown / leak oracle.
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013 Sec 7 O-3), Wave 3a.
 *        The HOST oracle for the ARENA memory model: FlairProcess_launch carves a
 *        HANDLE FlairApp + a GENERAL child block from the master FLAIR heap and
 *        FlairProcess_terminate one-shot-frees them (ADR-0013 Sec 3.2/3.4/3.6).
 *
 * THE REFRAMED LEAK INVARIANT (folded fix; ADR-0013 Sec 3.6 + Sec 7 O-3).
 *   The FLAIR heap is a BUMP allocator with per-class LIFO free-lists: flair_free
 *   pushes the freed block onto its class free-list and does NOT roll the bump
 *   cursor back, and flair_heap_avail reports size-used = the bump TAIL only, not
 *   free-listed blocks (heap.h:179-203, heap.c:176-178). So the naive "avail
 *   returns to the pre-launch value after teardown" invariant is WRONG for this
 *   heap and would RED a correct impl. The TRUE invariant, asserted here:
 *
 *     - cycle 1 (free-lists empty): launch BUMPS a HANDLE span + a GENERAL span,
 *       teardown frees them to the class lists but does NOT roll the cursor back,
 *       so avail DROPS by EXACTLY (HANDLE span + GENERAL span);
 *     - cycles k >= 2: launch REUSES the free-listed HANDLE + GENERAL blocks (same
 *       sizeof(FlairApp) + same budget => the freed block's cap fits), so the bump
 *       cursor never advances again and avail stays STABLE.
 *
 *   A LEAK (terminate forgetting flair_free(app->block)) leaves the GENERAL
 *   free-list empty, so every cycle BUMPS a fresh GENERAL span and avail DRIFTS
 *   MONOTONICALLY DOWN -- which the artifact mutant TEARDOWN_MUT_LEAK_BLOCK (a
 *   #ifdef in os/flair/process.c FlairProcess_terminate) makes happen, turning the
 *   "avail stable across cycles" check RED (Rule 6).
 *
 * INDEPENDENT-BY-RECOMPUTATION (Law 2). The expected cycle-1 drop is recomputed
 *   HERE from the documented heap span formula (header rounded to FLAIR_HEAP_ALIGN
 *   + payload aligned up), NOT read back from heap internals; the stability across
 *   cycles is a structural invariant independent of the artifact's counters. An
 *   oracle that asked the heap what the heap did would agree BY CONSTRUCTION and is
 *   forbidden (Law 2; HER-02).
 *
 * THE WINDOW PATH (faithful clean teardown). open() builds ONE real document
 *   window FROM THE CHILD ARENA (the test_interact/test_process region-backing
 *   idiom: a caller-supplied rows[]/x_pool per region) and binds w->refCon =
 *   (int32_t)(uintptr_t)self; terminate finds it by that refCon and DisposeWindow's
 *   it -- so the real teardown path (close + DisposeWindow + one-shot free) runs
 *   every cycle. The child-arena allocations come out of the GENERAL block and so
 *   do NOT perturb the MASTER avail accounting (they are inside the block already
 *   counted by the GENERAL span). SCOPE LIMIT (bead initech-ubd0): CLEAN teardown
 *   only -- no corrupted-arena death path (ADR Sec 3.4 vs 3.6 tension, deferred).
 *
 * Ref: ADR-0013 Sec 3.2 (launch a-g), Sec 3.4 (clean teardown), Sec 3.6 (child
 *      sub-arena + the heap-contract note), Sec 7 O-3; heap.h (flair_heap_t,
 *      flair_alloc/free/init, flair_heap_avail, FLAIR_HEAP_ALIGN, flair_blk_t);
 *      window.h (NewWindow/DisposeWindow). Mirrors harness/proptest/test_interact.c
 *      + test_process.c scaffolding. CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager (NewWindow/DisposeWindow/...)*/
#include "heap.h"             /* the FLAIR heap under the arena model (-Ios/flair)*/
#include "event_model.h"      /* EventRecord (-Ispec)                            */
#include "process.h"          /* THE App Contract under test (-Ios/flair)        */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* ===========================================================================
 * Independent heap-span recomputation (NOT read from heap internals -- Law 2).
 * Mirrors heap.c's FLAIR_HEAP_HDR_BYTES + flair_align_up, the documented model
 * (heap.h: header rounded to FLAIR_HEAP_ALIGN, payload aligned up).
 * ===========================================================================*/
static uint32_t align_up16(uint32_t n)
{
    return (n + (FLAIR_HEAP_ALIGN - 1u)) & ~(uint32_t)(FLAIR_HEAP_ALIGN - 1u);
}
static uint32_t hdr_bytes(void)
{
    return align_up16((uint32_t)sizeof(flair_blk_t));
}
static uint32_t alloc_span(uint32_t payload)   /* header + aligned payload */
{
    return hdr_bytes() + align_up16(payload);
}

/* ===========================================================================
 * The WindowMgr scaffolding (full-cap regions; test-owned static storage, NOT
 * from the master heap -- so it never perturbs the avail accounting under test).
 * ===========================================================================*/
enum { FRAME_W = 96, FRAME_H = 64 };

typedef struct rgn_store {
    region_t   r;
    rgn_row_t  rows[RGN_ROWS_CAP];
    int16_t    pool[RGN_X_POOL_CAP];
} rgn_store_t;

static void store_attach(rgn_store_t *s)
{
    memset(s, 0, sizeof *s);
    s->r.rows       = s->rows;
    s->r.cap_rows   = RGN_ROWS_CAP;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = RGN_X_POOL_CAP;
    region_set_empty(&s->r);
}

typedef struct mgr_store {
    WindowMgr   wm;
    rgn_store_t desk, sa, sb, sc;
} mgr_store_t;

static void mgr_attach(mgr_store_t *m, rgn_rect_t frame)
{
    store_attach(&m->desk); store_attach(&m->sa);
    store_attach(&m->sb);   store_attach(&m->sc);
    WindowMgr_init(&m->wm, frame, &m->desk.r, &m->sa.r, &m->sb.r, &m->sc.r);
}

/* ===========================================================================
 * The tenant's window bundle, allocated FROM THE CHILD ARENA in open(). Small
 * region caps (a single rectangular document window needs only a couple of rows
 * / x-points) keep the child budget modest; the WindowMgr's own scratch + the
 * desktop-update region above carry full caps for the damage algebra.
 * ===========================================================================*/
enum { TW_ROWS = 32, TW_POOL = 128 };

typedef struct tw_rgn {
    region_t  r;
    rgn_row_t rows[TW_ROWS];
    int16_t   pool[TW_POOL];
} tw_rgn_t;

typedef struct tw_win {
    WindowRecord rec;
    tw_rgn_t     struc, cont, upd;
} tw_win_t;

static void tw_rgn_attach(tw_rgn_t *s)
{
    s->r.rows       = s->rows;
    s->r.cap_rows   = TW_ROWS;
    s->r.x_pool     = s->pool;
    s->r.x_pool_cap = TW_POOL;
    region_set_empty(&s->r);
}

/* The required (no-op) event entry-point. */
static void tenant_event(FlairApp *self, const EventRecord *ev)
{
    (void)self; (void)ev;
}

/* open(): build ONE document window from the child arena + bind its refCon. */
static int tenant_open(FlairApp *self, const FlairLaunchParams *lp)
{
    tw_win_t *w = (tw_win_t *)flair_alloc(&self->arena, FLAIR_CLASS_HANDLE,
                                          (uint32_t)sizeof(tw_win_t));
    if (w == NULL) return 1;                 /* child budget too small -> fail */

    /* zero just the WindowRecord (regions are re-attached below). */
    memset(&w->rec, 0, sizeof w->rec);
    tw_rgn_attach(&w->struc);
    tw_rgn_attach(&w->cont);
    tw_rgn_attach(&w->upd);
    w->rec.strucRgn   = &w->struc.r;
    w->rec.contRgn    = &w->cont.r;
    w->rec.updateRgn  = &w->upd.r;
    w->rec.nextWindow = NULL;

    rgn_rect_t b = lp->bounds;
    rgn_rect_t c = { (int16_t)(b.top + 3), (int16_t)(b.left + 1),
                     (int16_t)(b.bottom - 1), (int16_t)(b.right - 1) };
    NewWindow(lp->wm, &w->rec, b, c, documentKind, documentProc, 1); /* lp->wm: Sec 3.2 */
    w->rec.refCon = (int32_t)(uintptr_t)self;   /* the binding rule (Sec 3.1) */
    self->windows = &w->rec;
    return 0;
}

static const FlairAppProcs g_tenant_procs = {
    tenant_open,    /* open  */
    tenant_event,   /* event (REQUIRED) */
    NULL,           /* idle  */
    NULL            /* close */
};

/* ===========================================================================
 * MAIN -- the O-3 teardown / leak oracle.
 * ===========================================================================*/
enum { NCYCLES = 4 };                 /* >= 3 cycles (ADR-0013 Sec 7 O-3)        */
enum { BUDGET  = 4096 };              /* per-tenant child-arena byte budget       */
enum { MASTER_BYTES = 64 * 1024 };    /* master heap window (holds N mutant bumps)*/

int main(void)
{
    static uint8_t master_buf[MASTER_BYTES];
    static mgr_store_t M;
    flair_heap_t master;
    FlairProcessList plist;

    rgn_rect_t FRAME = { 0, 0, FRAME_H, FRAME_W };
    rgn_rect_t bounds = { 10, 10, 46, 70 };   /* the tenant's document window      */

    mgr_attach(&M, FRAME);
    flair_heap_init(&master, master_buf, (uint32_t)MASTER_BYTES);
    FlairProcessList_init(&plist);

    /* the child budget must hold one window bundle + its block header. */
    CHECK(alloc_span((uint32_t)sizeof(tw_win_t)) <= (uint32_t)BUDGET,
          "setup: BUDGET holds one window bundle (alloc_span(sizeof tw_win_t) <= BUDGET)");

    /* the INDEPENDENT expected cycle-1 drop (header + aligned payload, per side). */
    uint32_t hspan = alloc_span((uint32_t)sizeof(FlairApp));
    uint32_t gspan = alloc_span((uint32_t)BUDGET);
    uint32_t expect_drop = hspan + gspan;

    uint32_t avail_pre = flair_heap_avail(&master);
    uint32_t avail_after[NCYCLES + 1];

    for (int k = 1; k <= NCYCLES; k++) {
        FlairApp *app = FlairProcess_launch(&plist, &M.wm, NULL /* surface */, &master,
                                            &g_tenant_procs, "T", bounds,
                                            (uint32_t)BUDGET);
        CHECK(app != NULL, "cycle: arena launch succeeds");
        CHECK(plist.head == app, "cycle: launched app is the foreground head");
        CHECK(M.wm.front != NULL && M.wm.front->refCon == (int32_t)(uintptr_t)app,
              "cycle: open() installed the tenant's window (refCon-bound) at front");

        FlairProcess_terminate(&plist, &M.wm, &master, app);
        CHECK(plist.head == NULL, "cycle: after terminate the process list is empty");
        CHECK(M.wm.front == NULL, "cycle: after terminate the window is disposed");

        avail_after[k] = flair_heap_avail(&master);
    }

    /* --- cycle 1: avail DROPS by EXACTLY (HANDLE span + GENERAL span). --- */
    CHECK(avail_after[1] < avail_pre,
          "O-3: cycle 1 drops avail (the bump cursor advanced; frees do not roll it back)");
    CHECK(avail_pre - avail_after[1] == expect_drop,
          "O-3: the cycle-1 drop == EXACTLY hdr+align(sizeof FlairApp) + hdr+align(budget)");
    CHECK(avail_pre - avail_after[1] >= (uint32_t)BUDGET,
          "O-3: the drop is at least the child budget (sanity)");

    /* --- cycles k >= 2: avail is STABLE (the freed HANDLE+GENERAL blocks are
     *     reused from the class free-lists; the bump cursor never advances again).
     *     The LEAK mutant bumps a fresh GENERAL span every cycle, so this is the
     *     check that goes RED under TEARDOWN_MUT_LEAK_BLOCK. --- */
    int stable = 1;
    for (int k = 2; k <= NCYCLES; k++)
        if (avail_after[k] != avail_after[1]) stable = 0;
    CHECK(stable,
          "O-3: avail is STABLE for cycles >= 2 (free-list reuse; no leak, no drift)");

    /* an explicit monotone-non-increasing spelling of the same property: a leak
     * makes avail STRICTLY decrease cycle-over-cycle; a correct impl holds it. */
    CHECK(avail_after[NCYCLES] == avail_after[1],
          "O-3: avail after the last cycle == avail after cycle 1 (no monotonic drift)");

    return TEST_SUMMARY("test-process-teardown");
}

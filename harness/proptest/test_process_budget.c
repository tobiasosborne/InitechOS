/* test_process_budget.c -- the FLAIR App Contract O-4 fail-loud launch-budget oracle.
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013 Sec 7 O-4 / BC-5),
 *        Wave 3a. The HOST oracle for the FAIL-LOUD launch budget of the arena
 *        memory model: a launch that cannot carve its partition -- because the
 *        master heap is exhausted, OR because the tenant's open() reports failure
 *        -- must REFUSE: return NULL, install NO window/menubar, leave the process
 *        list + the WindowMgr UNCHANGED, and reclaim cleanly (ADR-0013 Sec 3.2,
 *        Sec 3.6 "partition sizing is a fail-loud budget; a launch that cannot
 *        carve its block refuses and reports; it does not overcommit").
 *
 * THE TWO LEGS (ADR-0013 Sec 7 O-4):
 *   (a) OVER-BUDGET: a budget exceeding the master heap remaining. The HANDLE
 *       carve may succeed but the GENERAL (block) carve returns NULL -> launch
 *       reclaims the handle, installs nothing, returns NULL.
 *   (b) OPEN-FAIL: both carves succeed but the tenant's open() returns != 0 ->
 *       launch reclaims block + handle, installs nothing, returns NULL.
 *
 * THE "avail returns to before" CHECK + WHY WE PRIME (the bump+free-list heap).
 *   flair_free does NOT roll the bump cursor back; it pushes the freed block onto
 *   its class free-list (heap.h:179-203). So on a PRISTINE heap a failed launch's
 *   reclaim would leave avail LOWER (the handle was bumped, then freed-not-rolled-
 *   back). To assert the strong "a failed launch is a perfect no-op on avail", we
 *   first PRIME the free-lists with one successful launch+terminate cycle: the
 *   HANDLE + GENERAL free-lists then each hold a reusable block, so each fail leg
 *   below REUSES those blocks (no fresh bump) and frees them straight back ->
 *   avail returns EXACTLY to its pre-attempt value. This is the true contract, not
 *   a "returns to pristine" fiction (Sec 3.6 heap-contract note; mirrors O-3).
 *
 * MUTATION-PROVEN (Rule 6). The artifact mutant BUDGET_MUT_OVERCOMMIT (a #ifdef in
 *   os/flair/process.c FlairProcess_launch) OVERCOMMITS: on an alloc-NULL or an
 *   open()-failure it partially installs the app (links it at the list head /
 *   returns non-NULL) instead of failing loud + reclaiming. Against this oracle's
 *   "launch == NULL + list/WindowMgr unchanged" checks, that goes RED on BOTH
 *   legs. INDEPENDENT golden: every expected value (NULL return, unchanged list
 *   head, unchanged wm->front, restored avail) is computed HERE, never read back
 *   from the launcher (Law 2; HER-02).
 *
 * Ref: ADR-0013 Sec 3.2 (launch a-g, the budget fail path), Sec 3.6 (fail-loud
 *      budget), Sec 6 BC-5, Sec 7 O-4; heap.h (flair_alloc NULL on exhaustion;
 *      flair_heap_avail). Mirrors harness/proptest/test_process_teardown.c +
 *      test_interact.c scaffolding. CLAUDE.md Law 2, Rule 1, Rule 2 (fail loud),
 *      Rule 6, Rule 11, Rule 12.
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
 * WindowMgr scaffolding (full-cap regions; test-owned static, not from master).
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

static WindowMgr *g_wm = NULL;

/* tenant window bundle (child-arena allocated; small caps -- see test_process_teardown). */
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

static void tenant_event(FlairApp *self, const EventRecord *ev)
{
    (void)self; (void)ev;
}

/* good open(): builds a document window from the child arena (success path). */
static int good_open(FlairApp *self, const FlairLaunchParams *lp)
{
    tw_win_t *w = (tw_win_t *)flair_alloc(&self->arena, FLAIR_CLASS_HANDLE,
                                          (uint32_t)sizeof(tw_win_t));
    if (w == NULL) return 1;

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
    NewWindow(g_wm, &w->rec, b, c, documentKind, documentProc, 1);
    w->rec.refCon = (int32_t)(uintptr_t)self;
    self->windows = &w->rec;
    return 0;
}

/* bad open(): reports launch failure and builds NOTHING (the open-fail leg). */
static int bad_open(FlairApp *self, const FlairLaunchParams *lp)
{
    (void)self; (void)lp;
    return 1;                                /* != 0 -> launch fail (Sec 3.1)     */
}

static const FlairAppProcs g_good_procs = { good_open, tenant_event, NULL, NULL };
static const FlairAppProcs g_bad_procs  = { bad_open,  tenant_event, NULL, NULL };

/* ===========================================================================
 * MAIN -- the O-4 fail-loud launch-budget oracle.
 * ===========================================================================*/
enum { BUDGET  = 4096 };
enum { MASTER_BYTES = 64 * 1024 };

int main(void)
{
    static uint8_t master_buf[MASTER_BYTES];
    static mgr_store_t M;
    flair_heap_t master;
    FlairProcessList plist;

    rgn_rect_t FRAME  = { 0, 0, FRAME_H, FRAME_W };
    rgn_rect_t bounds = { 10, 10, 46, 70 };

    mgr_attach(&M, FRAME);
    g_wm = &M.wm;
    flair_heap_init(&master, master_buf, (uint32_t)MASTER_BYTES);
    FlairProcessList_init(&plist);

    /* --- PRIME: one successful launch+terminate so the HANDLE + GENERAL free-lists
     *     each hold a reusable block (see the file header on why this makes the
     *     "avail returns to before" assertion the TRUE contract). --- */
    FlairApp *app0 = FlairProcess_launch(&plist, &M.wm, &master, &g_good_procs,
                                         "P", bounds, (uint32_t)BUDGET);
    CHECK(app0 != NULL, "prime: a within-budget launch succeeds");
    CHECK(plist.head == app0 && M.wm.front != NULL,
          "prime: the primed tenant is installed (head + front window)");
    FlairProcess_terminate(&plist, &M.wm, &master, app0);
    CHECK(plist.head == NULL && M.wm.front == NULL,
          "prime: teardown clears the list + disposes the window (free-lists now primed)");

    /* ===== leg (a): OVER-BUDGET launch (budget > master remaining) ===== */
    uint32_t  avail_before_a = flair_heap_avail(&master);
    FlairApp *head_before_a  = plist.head;     /* NULL */
    WindowPtr front_before_a = M.wm.front;     /* NULL */
    uint32_t  over_budget    = (uint32_t)MASTER_BYTES;   /* > remaining, always   */

    CHECK(over_budget > avail_before_a,
          "leg(a): the requested budget genuinely exceeds the master remaining (meaningful)");

    FlairApp *app_a = FlairProcess_launch(&plist, &M.wm, &master, &g_good_procs,
                                          "OB", bounds, over_budget);
    CHECK(app_a == NULL,
          "leg(a): an over-budget launch FAILS LOUD -- returns NULL (BC-5)");
    CHECK(plist.head == head_before_a,
          "leg(a): the process list is UNCHANGED (no partial install)");
    CHECK(M.wm.front == front_before_a,
          "leg(a): the WindowMgr z-order is UNCHANGED -- NO window installed");
    CHECK(flair_heap_avail(&master) == avail_before_a,
          "leg(a): the failed launch reclaims cleanly (avail returns to before)");

    /* ===== leg (b): OPEN-FAIL launch (carves succeed, open() returns != 0) ===== */
    uint32_t  avail_before_b = flair_heap_avail(&master);
    FlairApp *head_before_b  = plist.head;     /* NULL */
    WindowPtr front_before_b = M.wm.front;     /* NULL */

    FlairApp *app_b = FlairProcess_launch(&plist, &M.wm, &master, &g_bad_procs,
                                          "OF", bounds, (uint32_t)BUDGET);
    CHECK(app_b == NULL,
          "leg(b): an open()-failure launch FAILS LOUD -- returns NULL (Sec 3.1)");
    CHECK(plist.head == head_before_b,
          "leg(b): the process list is UNCHANGED (no partial install)");
    CHECK(M.wm.front == front_before_b,
          "leg(b): the WindowMgr z-order is UNCHANGED -- NO window installed");
    CHECK(flair_heap_avail(&master) == avail_before_b,
          "leg(b): the open-fail launch reclaims block + handle (avail returns to before)");

    /* ===== sanity: after the two failed launches, a within-budget launch still
     *       succeeds + reuses the primed blocks (the heap is not poisoned). ===== */
    uint32_t avail_before_c = flair_heap_avail(&master);
    FlairApp *app_c = FlairProcess_launch(&plist, &M.wm, &master, &g_good_procs,
                                          "OK", bounds, (uint32_t)BUDGET);
    CHECK(app_c != NULL && plist.head == app_c && M.wm.front != NULL,
          "post: a within-budget launch after the failures still installs cleanly");
    FlairProcess_terminate(&plist, &M.wm, &master, app_c);
    CHECK(flair_heap_avail(&master) == avail_before_c,
          "post: that launch+terminate reused the free-listed blocks (avail stable)");

    return TEST_SUMMARY("test-process-budget");
}

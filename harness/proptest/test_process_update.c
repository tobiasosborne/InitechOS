/* test_process_update.c -- the FLAIR App Contract O-5 updateEvt-routing oracle (HOST).
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013 Sec 7 / Sec 3.3), the
 *        Wave-4 updateEvt spine. The HOST oracle for `flair_route_updates`
 *        (os/flair/process.c) -- the helper the live kmain pump calls each iteration
 *        to route each damaged window's pending repaint to its OWNING tenant as an
 *        updateEvt, then clear the damage (WindowMgr_validate). It is the SAME
 *        symbol the live pump runs (BC-2 single spine), so a green O-5 certifies the
 *        live update routing too.
 *
 * INDEPENDENT-BY-RECOMPUTATION (Law 2; the test_process / test_interact idiom). The
 *   expected delivery sequence + post-route damage state are a HAND-AUTHORED golden,
 *   NEVER read back out of the router. Each stub tenant's `event` handler APPENDS
 *   (app-id, ev.what, ev.message=window-identity) to a global log on updateEvt; the
 *   oracle diffs that log + the windows' updateRgn emptiness against the by-hand
 *   expected values. An oracle that asked the router what the router did would agree
 *   BY CONSTRUCTION and is forbidden (Law 2; HER-02).
 *
 * THE SCENE (four DISJOINT windows so each has a full visible region -- no occlusion
 *   to confound which window carries damage):
 *     - app A (foreground) owns TWO windows: WA0 (group front) + WA1;
 *     - app B (background) owns ONE window:  WB0;
 *     - WU is UNOWNED (refCon 0 -- the shell-furniture analogue) to prove
 *       flair_route_updates TOLERATES an ownerless damaged window (no panic), unlike
 *       content-click dispatch (owner_of_window) which fails loud on an unowned hit.
 *   z-order front-to-back, by construction: [WA0, WB0, WA1, WU].
 *   INVALIDATED subset: WA0, WB0, WU. NOT invalidated: WA1 (the non-invalidated
 *   owned window -- proves it gets nothing).
 *
 * THE EXPECTED GOLDEN (hand-authored; ADR-0013 Sec 3.3 "updateEvt -> each damaged
 *   window's owning app, background apps included"):
 *     flair_route_updates walks the z-order front-to-back and for each VISIBLE
 *     window with NON-EMPTY damage routes ONE updateEvt to its owner, then validates:
 *       WA0 (owner A, dirty)  -> A.event(updateEvt, msg=WA0); validate WA0
 *       WB0 (owner B, dirty)  -> B.event(updateEvt, msg=WB0); validate WB0
 *       WA1 (owner A, CLEAN)  -> skipped (no damage)            -> no delivery
 *       WU  (UNOWNED, dirty)  -> skipped (no owner, NO PANIC)   -> no delivery, NOT validated
 *     => log == [ (A,updateEvt,WA0), (B,updateEvt,WB0) ] exactly; WA0/WB0 updateRgn
 *        now EMPTY (validate ran); WA1 stayed empty; WU STILL non-empty (tolerated +
 *        skipped -- the shell repaints its own furniture).
 *
 * MUTATION-PROVEN (Rule 6; self-mutation -- the test_interact / test_process
 *   convention; the mutants perturb the INDEPENDENT EXPECTED side, NOT the router):
 *
 *     UPDATE_MUT_WRONG_OWNER  -- the EXPECTED owner of WA0's updateEvt is the WRONG
 *       app (B instead of A). The correct router demuxes WA0 to its refCon owner A,
 *       so the leg-0 routing assertion goes RED. Proves the update reaches the
 *       refCon-demuxed OWNER, not some other app.
 *
 *     UPDATE_MUT_SKIP_VALIDATE -- the EXPECTED post-route state leaves a routed
 *       window's updateRgn NON-EMPTY (i.e. validate was "skipped"). The correct
 *       router calls WindowMgr_validate after delivery, so the routed-window-now-
 *       empty assertion goes RED. Proves the damage is actually cleared (no infinite
 *       updateEvt storm).
 *
 * Ref: ADR-0013 Sec 3.1 (tenant ABI + binding rule / refCon demux), Sec 3.3
 *      (updateEvt routing in task context); spec/window_record.h Sec 4
 *      (WindowRecord.updateRgn / .visible / .refCon); spec/event_model.h Sec 1
 *      (updateEvt=6), :228-235 (message = the affected WindowPtr, MTE Ch 2),
 *      :265-271 (EventRecord); os/flair/window.h (NewWindow / WindowMgr_invalidate /
 *      WindowMgr_validate); spec/region_algebra.h (region_is_empty). Mirrors
 *      harness/proptest/test_process.c (stub-app + delivery-log idiom) and
 *      test_process_activate.c (recover_owner + the self-mutation convention).
 *      CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract + region_is_empty (-Ispec) */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager (NewWindow/invalidate/validate)*/
#include "event_model.h"      /* EventRecord, updateEvt (-Ispec)                 */
#include "process.h"          /* THE App Contract under test (-Ios/flair)        */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* The tenant ids (the per-app datum the log records). */
enum { A_ID = 1, B_ID = 2 };

/* Scene frame: wide enough for FOUR disjoint windows side by side. */
enum { GW = 160, GH = 48 };

/* ===========================================================================
 * Region storage bundles (the test_process / test_interact rgn_store arena: the
 * engine never mallocs; the CALLER supplies rows[]/x_pool). One per region.
 * ===========================================================================*/
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

/* A window bundle: the record + its three regions' arenas, all attached. */
typedef struct win_store {
    WindowRecord rec;
    rgn_store_t  struc, cont, upd;
} win_store_t;

static void win_attach(win_store_t *w)
{
    memset(&w->rec, 0, sizeof w->rec);
    store_attach(&w->struc);
    store_attach(&w->cont);
    store_attach(&w->upd);
    w->rec.strucRgn   = &w->struc.r;
    w->rec.contRgn    = &w->cont.r;
    w->rec.updateRgn  = &w->upd.r;
    w->rec.nextWindow = NULL;
}

/* The manager + its three scratch regions + desktop update, all arena-backed. */
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
 * INDEPENDENT rect membership (the test_process helper; do NOT call window.c).
 * Half-open [top,bottom) x [left,right) -- the LOCKED region span convention.
 * ===========================================================================*/
static int rect_is_empty(rgn_rect_t r)
{
    return (r.right <= r.left) || (r.bottom <= r.top);
}
static int rect_contains(rgn_rect_t r, int16_t h, int16_t v)
{
    if (rect_is_empty(r)) return 0;
    return (v >= r.top && v < r.bottom && h >= r.left && h < r.right);
}

/* normalized emptiness (0/1) of a window's updateRgn (robust against any
 * non-canonical truthy return; region_is_empty returns 1 iff the empty set). */
static int upd_empty(const WindowPtr w)
{
    return region_is_empty(w->updateRgn) ? 1 : 0;
}

/* ===========================================================================
 * The known tenants (host-safe app-id recovery: self is a REAL pointer the
 * router passes, never the truncated refCon; see process.h host note) + the
 * INDEPENDENT refCon recovery (the contract's demux key, recomputed HERE, Law 2).
 * ===========================================================================*/
static FlairApp *g_appA = NULL;
static FlairApp *g_appB = NULL;

static int app_id_of(const FlairApp *a)
{
    if (a == g_appA) return A_ID;
    if (a == g_appB) return B_ID;
    return -1;
}

/* owns -- the binding-rule demux key (Sec 3.1), reimplemented HERE: a window is
 * `app`'s iff (int32_t)(uintptr_t)app == w->refCon (truncated-equals-truncated on
 * host-64, exact on flat-32). */
static int owns(const FlairApp *app, const WindowPtr w)
{
    return w->refCon == (int32_t)(uintptr_t)app;
}

/* recover_owner -- the INDEPENDENT refCon->owner recovery; NULL if unowned. */
static FlairApp *recover_owner(const WindowPtr w)
{
    if (owns(g_appA, w)) return g_appA;
    if (owns(g_appB, w)) return g_appB;
    return NULL;
}

/* ===========================================================================
 * The delivery log -- the SOLE delivery observable. Each tenant `event` appends
 * one entry; the oracle diffs it against the hand-authored golden (Law 2).
 * ===========================================================================*/
typedef struct log_entry {
    int      app_id;     /* which tenant's event handler fired (A_ID / B_ID)     */
    uint16_t what;       /* ev.what (updateEvt)                                  */
    uint32_t message;    /* ev.message (the affected WindowPtr, truncated)       */
} log_entry_t;

enum { LOG_CAP = 16 };
static log_entry_t g_log[LOG_CAP];
static int         g_log_n = 0;

static void log_append(int app_id, const EventRecord *ev)
{
    if (g_log_n >= LOG_CAP) return;
    g_log[g_log_n].app_id  = app_id;
    g_log[g_log_n].what    = ev->what;
    g_log[g_log_n].message = ev->message;
    g_log_n++;
}

static void stub_event(FlairApp *self, const EventRecord *ev)
{
    log_append(app_id_of(self), ev);
}

static int stub_open(FlairApp *self, const FlairLaunchParams *lp)
{
    (void)self; (void)lp;
    return 0;
}

static const FlairAppProcs g_stub_procs = {
    stub_open,    /* open  */
    stub_event,   /* event (REQUIRED) */
    NULL,         /* idle  */
    NULL          /* close */
};

/* one log entry matches an (app, updateEvt, affected-window) tuple? */
static int is_update_of(const log_entry_t *g, int app_id, const WindowPtr w)
{
    return g->app_id == app_id &&
           g->what == (uint16_t)updateEvt &&
           g->message == (uint32_t)(uintptr_t)w;
}

/* ===========================================================================
 * MAIN -- the O-5 updateEvt-routing oracle.
 * ===========================================================================*/
int main(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };   /* (top,left,bottom,right) */
    static win_store_t WA0, WA1, WB0, WU;
    static mgr_store_t M;
    static FlairApp    appA, appB;
    static FlairProcessList plist;

    mgr_attach(&M, FRAME);

    /* Geometry (top,left,bottom,right): four DISJOINT windows side by side so each
     * has a FULL visible region (invalidation cannot be clipped away by occlusion). */
    rgn_rect_t A0s = { 8,   6, 30,  34 }, A0c = { 11,   7, 29,  33 };  /* app A    */
    rgn_rect_t B0s = { 8,  42, 30,  70 }, B0c = { 11,  43, 29,  69 };  /* app B    */
    rgn_rect_t A1s = { 8,  78, 30, 106 }, A1c = { 11,  79, 29, 105 };  /* app A    */
    rgn_rect_t WUs = { 8, 114, 30, 142 }, WUc = { 11, 115, 29, 141 };  /* UNOWNED  */

    /* Create back-to-front so the z-order front-to-back is [WA0, WB0, WA1, WU]:
     * WU first (back), then WA1, then WB0, then WA0 (front). NewWindow pushes front. */
    win_attach(&WU);
    NewWindow(&M.wm, &WU.rec,  WUs, WUc, documentKind, documentProc, 1);  /* WU back  */
    win_attach(&WA1);
    NewWindow(&M.wm, &WA1.rec, A1s, A1c, documentKind, documentProc, 1);  /* WA1      */
    win_attach(&WB0);
    NewWindow(&M.wm, &WB0.rec, B0s, B0c, documentKind, documentProc, 1);  /* WB0      */
    win_attach(&WA0);
    NewWindow(&M.wm, &WA0.rec, A0s, A0c, documentKind, documentProc, 1);  /* WA0 front*/

    /* Construct the two tenants. appA's group front is WA0; appB's is WB0. Bind every
     * OWNED window's refCon to its app (Sec 3.1 demux key). _register binds only the
     * group-front window, so WA1's refCon is bound explicitly here. WU stays refCon 0
     * (unowned -- the shell-furniture analogue route_updates must tolerate). */
    memset(&appA, 0, sizeof appA);
    memset(&appB, 0, sizeof appB);
    appA.name = "A"; appA.procs = &g_stub_procs; appA.windows = &WA0.rec; appA.refCon = A_ID;
    appB.name = "B"; appB.procs = &g_stub_procs; appB.windows = &WB0.rec; appB.refCon = B_ID;
    g_appA = &appA; g_appB = &appB;

    FlairProcessList_init(&plist);
    /* Register B then A so A ends as the foreground head; _register binds each app's
     * group-front window's refCon = (int32_t)(uintptr_t)app. */
    FlairProcess_register(&plist, &appB);
    FlairProcess_register(&plist, &appA);
    /* Bind the SECOND A window (WA1); _register only touched the group-front (WA0). */
    WA1.rec.refCon = (int32_t)(uintptr_t)&appA;
    /* WU.rec.refCon is left 0 (unowned). */

    /* ===== scene meaningfulness (INDEPENDENT of the router) ===== */
    /* rect_contains(rect, h, v) -- h horizontal, v vertical (the test_process order). */
    CHECK(rect_contains(A0c, 20, 20) && rect_contains(B0c, 56, 20) &&
          rect_contains(A1c, 92, 20) && rect_contains(WUc, 128, 20),
          "scene: each probe point lies in its window's content (the windows are real)");
    CHECK(plist.head == &appA,
          "scene: A is the foreground head, B is background (register ordering)");
    CHECK(recover_owner(&WA0.rec) == &appA && recover_owner(&WA1.rec) == &appA &&
          recover_owner(&WB0.rec) == &appB,
          "scene: every OWNED window's refCon round-trips to its owner (binding rule)");
    CHECK(recover_owner(&WU.rec) == NULL,
          "scene: the UNOWNED window WU (refCon 0) is owned by NO resident app (route must tolerate it)");

    /* ===== invalidate the chosen subset: WA0, WB0, WU -- NOT WA1 ===== */
    WindowMgr_invalidate(&M.wm, &WA0.rec, A0c);
    WindowMgr_invalidate(&M.wm, &WB0.rec, B0c);
    WindowMgr_invalidate(&M.wm, &WU.rec,  WUc);

    /* the invalidation genuinely took (non-empty damage on the chosen subset; the
     * non-invalidated owned window is still clean) -- so the route is meaningful. */
    CHECK(upd_empty(&WA0.rec) == 0 && upd_empty(&WB0.rec) == 0 && upd_empty(&WU.rec) == 0,
          "scene: invalidation made WA0, WB0, WU carry NON-EMPTY damage (meaningful)");
    CHECK(upd_empty(&WA1.rec) == 1,
          "scene: WA1 was NOT invalidated -- its updateRgn is empty");

    /* ===== the spine under test ===== */
    flair_route_updates(&plist, &M.wm);

    /* INDEPENDENT expected golden (hand-authored; never read from the router). */
    int exp_app_wa0     = A_ID;   /* WA0 is owned by app A (binding rule)            */
    int exp_routed_empty = 1;     /* a routed window's damage is cleared (validate)  */
#ifdef UPDATE_MUT_WRONG_OWNER
    exp_app_wa0 = B_ID;           /* self-mutation: EXPECT WA0 routed to the WRONG app */
#endif
#ifdef UPDATE_MUT_SKIP_VALIDATE
    exp_routed_empty = 0;         /* self-mutation: EXPECT validate skipped (still dirty)*/
#endif

    /* --- the delivery log: exactly WA0->A then WB0->B, in z-order --- */
    CHECK(g_log_n == 2,
          "route: EXACTLY two deliveries (WA0->A, WB0->B); WA1 (clean) + WU (unowned) produce none");
    CHECK(g_log_n >= 1 && is_update_of(&g_log[0], exp_app_wa0, &WA0.rec),
          "route: WA0's updateEvt is delivered to its refCon OWNER A (with message=WA0)");
    CHECK(g_log_n >= 2 && is_update_of(&g_log[1], B_ID, &WB0.rec),
          "route: WB0's updateEvt is delivered to its refCon OWNER B (background app included)");

    /* the non-invalidated owned window (WA1) and the unowned window (WU) never
     * appear in the log -- no delivery for clean or ownerless windows. */
    {
        int wa1_or_wu_seen = 0;
        for (int i = 0; i < g_log_n; i++)
            if (g_log[i].message == (uint32_t)(uintptr_t)&WA1.rec ||
                g_log[i].message == (uint32_t)(uintptr_t)&WU.rec)
                wa1_or_wu_seen = 1;
        CHECK(!wa1_or_wu_seen,
              "route: the non-invalidated owned window (WA1) and the UNOWNED window (WU) got NO delivery");
    }

    /* --- post-route damage state: routed windows validated; unowned tolerated --- */
    CHECK(upd_empty(&WA0.rec) == exp_routed_empty,
          "route: WA0's updateRgn is now EMPTY (WindowMgr_validate ran after delivery)");
    CHECK(upd_empty(&WB0.rec) == 1,
          "route: WB0's updateRgn is now EMPTY (validate ran)");
    CHECK(upd_empty(&WA1.rec) == 1,
          "route: WA1 (never invalidated) stayed EMPTY");
    CHECK(upd_empty(&WU.rec) == 0,
          "route: the UNOWNED window WU was TOLERATED (no panic) and SKIPPED -- its updateRgn is "
          "untouched (the shell repaints its own furniture; unlike content-click fail-loud)");

    return TEST_SUMMARY("test-process-update");
}

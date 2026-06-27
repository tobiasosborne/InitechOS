/* test_process_activate.c -- the FLAIR App Contract O-2 activation / z-order
 * oracle (HOST).
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013 Sec 7 O-2), Wave 3b.
 *        The HOST oracle for APP-GROUP RAISE on app switch: when the dispatcher
 *        switches the foreground to a background app, it raises that app's WHOLE
 *        window group to the front CONTIGUOUS run of the WindowMgr z-order,
 *        PRESERVING the group's internal relative z-order, with the group's front
 *        window hilited + active (ADR-0013 Sec 3.5: "switching ... raises that
 *        app's WHOLE window group to the front run", app-grouped z-order). It runs
 *        the EXACT Layer-5 symbol the live kmain pump runs (`flair_app_dispatch`,
 *        BC-2), so a green O-2 certifies the live group-raise too.
 *
 * INDEPENDENT-BY-RECOMPUTATION (Law 2; the test_interact / test_process idiom).
 *   The expected post-switch z-order, hilite assignment, and refCon owner map are
 *   NEVER read back out of the dispatcher. The expected z-order is maintained HERE
 *   as a separate front-to-back WindowPtr model and advanced by an INDEPENDENT
 *   group-raise (a STABLE PARTITION -- a different algorithm than the artifact's
 *   back-to-front SelectWindow loop), then diffed element-by-element against the
 *   artifact's actual `wm->front` chain. The expected hilite is the switched-to
 *   app's front window (known by construction). An oracle that asked the
 *   dispatcher what the dispatcher did would agree BY CONSTRUCTION and is
 *   forbidden (Law 2; HER-02).
 *
 * THE SCENE (meaningful -- the group is NOT already contiguous):
 *   App A (foreground) owns TWO windows A0 (front) + A1 (behind); app B
 *   (background) owns ONE window B0. They are INTERLEAVED in the z-order,
 *   front-to-back: [A0, B0, A1] -- so A0 and A1 are NOT yet a contiguous run (B0
 *   sits between them). The three windows are DISJOINT in x so a content click is
 *   unambiguously its owner's regardless of z-order. Each window's refCon is bound
 *   to its owning app (the demux key, Sec 3.1).
 *
 * THE SCRIPTED TRACE + INDEPENDENT GOLDEN (ADR-0013 Sec 7 O-2):
 *   (1) mouseDown inContent over background B0 -> switch to B. Assert B's whole
 *       group (just B0) is the front run [B0, A0, A1]; hilited==1 on exactly
 *       wm->front (B0), 0 on all others; the previously-foreground app A's front
 *       window received the deactivate (activateEvt, active-flag=0).
 *   (2) mouseDown inContent over A0 -> switch BACK to A. Assert A's WHOLE group is
 *       now the front CONTIGUOUS run in ORIGINAL relative order [A0, A1, B0] (A0
 *       still ahead of A1); hilited==1 on exactly wm->front (A0), 0 elsewhere;
 *       every window's refCon round-trips to its correct owner via the recovery
 *       rule; B's front window received the deactivate.
 *
 * MUTATION-PROVEN (Rule 6; self-mutation -- the test_interact / test_process
 *   convention). The mutants perturb the INDEPENDENT EXPECTED computation (NOT the
 *   dispatcher), each so it disagrees with the CORRECT dispatcher on a DISTINCT
 *   invariant:
 *
 *     ACTIVATE_MUT_NO_HILITE -- the EXPECTED hilite is kept on the OLD front
 *       window instead of moving to the new foreground's front. The CORRECT
 *       dispatcher (via SelectWindow -> reaffirm_active) moves the hilite to the
 *       new front, so the hilite-on-front assertion goes RED. Proves the
 *       hilite-follows-the-front check is live (it would catch a SelectWindow that
 *       skipped the hilite update).
 *
 *     ACTIVATE_MUT_RAISE_FRONT_ONLY -- the EXPECTED group-raise pulls ONLY the
 *       app's front window to the head, leaving the rest of its group behind. The
 *       CORRECT dispatcher raises the WHOLE group, so the whole-group-contiguous-
 *       run assertion goes RED on the switch back to the two-window app A (its
 *       expected order becomes [A0, B0, A1] while the artifact yields [A0, A1,
 *       B0]). Proves the group-raise (not just front-raise) is checked.
 *
 * Ref: ADR-0013 Sec 3.5 (app-grouped z-order, whole-group raise on switch), Sec
 *      3.1 (the refCon demux key), Sec 3.3 (the activate pair), Sec 7 O-2;
 *      spec/window_record.h Sec 4 (WindowRecord.hilited / .nextWindow / .refCon),
 *      spec/event_model.h Sec 1 (what-codes), :155 (active-flag), :265-271
 *      (EventRecord); os/flair/window.h (SelectWindow / the wm->front z-order).
 *      Mirrors harness/proptest/test_process.c (stub-app + delivery-log idiom) and
 *      test_interact.c (independent-by-recomputation + the self-mutation
 *      convention). CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager (NewWindow/SelectWindow/...) */
#include "event_model.h"      /* EventRecord, what-codes, active-flag (-Ispec)   */
#include "process.h"          /* THE App Contract under test (-Ios/flair)        */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* The tenant ids (the per-app datum the log records). */
enum { A_ID = 1, B_ID = 2 };

/* Scene frame: wide enough for THREE disjoint windows side by side. */
enum { GW = 120, GH = 48 };

/* Three windows in the scene; the independent z-order model is bounded by this. */
enum { NWIN = 3, ORDER_CAP = 8 };

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

/* ===========================================================================
 * The known tenants (for host-safe app-id recovery in the delivery log -- self is
 * a REAL pointer the dispatcher passes, never the truncated refCon; see process.h
 * host note) and the INDEPENDENT refCon recovery (the contract's demux key,
 * recomputed HERE, Law 2).
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

/* recover_owner -- the INDEPENDENT refCon->owner recovery (the contract rule):
 * the known app whose demux key equals w->refCon. NULL if unowned. */
static FlairApp *recover_owner(const WindowPtr w)
{
    if (owns(g_appA, w)) return g_appA;
    if (owns(g_appB, w)) return g_appB;
    return NULL;
}

/* ===========================================================================
 * The delivery log -- the activate/deactivate observable. Each tenant `event`
 * appends one entry; the oracle diffs it against the hand-authored golden (Law 2).
 * The `message` field carries the affected WindowPtr (truncated, as the
 * dispatcher stamps it) so a deactivate can be tied to the OLD foreground's
 * front window.
 * ===========================================================================*/
typedef struct log_entry {
    int      app_id;     /* which tenant's event handler fired (A_ID / B_ID)     */
    uint16_t what;       /* ev.what (mouseDown / activateEvt)                    */
    uint16_t modifiers;  /* ev.modifiers (carries FLAIR_EVT_MOD_ACTIVE_FLAG)     */
    uint32_t message;    /* ev.message (the affected WindowPtr, truncated)       */
} log_entry_t;

enum { LOG_CAP = 16 };
static log_entry_t g_log[LOG_CAP];
static int         g_log_n = 0;

static void log_append(int app_id, const EventRecord *ev)
{
    if (g_log_n >= LOG_CAP) return;
    g_log[g_log_n].app_id    = app_id;
    g_log[g_log_n].what      = ev->what;
    g_log[g_log_n].modifiers = ev->modifiers;
    g_log[g_log_n].message   = ev->message;
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

/* an activate-log entry matches (app, active-flag, affected window)? */
static int is_deactivate_of(const log_entry_t *g, int app_id, const WindowPtr w)
{
    return g->app_id == app_id &&
           g->what == (uint16_t)activateEvt &&
           (g->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) == 0 &&
           g->message == (uint32_t)(uintptr_t)w;
}
static int is_activate_of(const log_entry_t *g, int app_id, const WindowPtr w)
{
    return g->app_id == app_id &&
           g->what == (uint16_t)activateEvt &&
           (g->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) != 0 &&
           g->message == (uint32_t)(uintptr_t)w;
}

/* ===========================================================================
 * The INDEPENDENT z-order model + group-raise (Law 2). `order[]` is the expected
 * front-to-back chain; expected_raise_group advances it by a STABLE PARTITION --
 * a DIFFERENT algorithm than the artifact's back-to-front SelectWindow loop, so
 * agreement is a real differential, not by-construction.
 *
 * MUTANT ACTIVATE_MUT_RAISE_FRONT_ONLY (Rule 6): raise ONLY the app's front
 * window (app->windows), leaving the rest of the group behind -> the expected
 * z-order disagrees with the CORRECT whole-group raise on the two-window app, so
 * the whole-group-contiguous-run assertion goes RED.
 * ===========================================================================*/
static void expected_raise_group(WindowPtr *order, int n, FlairApp *app)
{
    WindowPtr tmp[ORDER_CAP];
    int t = 0;
#ifdef ACTIVATE_MUT_RAISE_FRONT_ONLY
    /* front-only: pull just the group's front window to the head, leave the rest. */
    tmp[t++] = app->windows;
    for (int i = 0; i < n; i++)
        if (order[i] != app->windows) tmp[t++] = order[i];
#else
    /* whole group: the owned windows (in their relative order) first, then the
     * rest (in their relative order) -- a stable partition raising the group. */
    for (int i = 0; i < n; i++)
        if (owns(app, order[i]))  tmp[t++] = order[i];
    for (int i = 0; i < n; i++)
        if (!owns(app, order[i])) tmp[t++] = order[i];
#endif
    for (int i = 0; i < n; i++) order[i] = tmp[i];
}

/* compare the artifact's actual wm->front chain against the expected order. */
static int zorder_matches(const WindowMgr *wm, WindowPtr *exp, int n)
{
    int i = 0;
    for (WindowPtr w = wm->front; w != NULL; w = w->nextWindow) {
        if (i >= n) return 0;          /* artifact chain is LONGER than expected   */
        if (w != exp[i]) return 0;     /* order disagrees                          */
        i++;
    }
    return i == n;                     /* and EXACTLY n windows                     */
}

/* Cook one EventRecord (Sec 7 O-2 scripted trace). */
static EventRecord mk_event(uint16_t what, int16_t v, int16_t h)
{
    EventRecord ev;
    memset(&ev, 0, sizeof ev);
    ev.what    = what;
    ev.where.v = v;
    ev.where.h = h;
    return ev;
}

/* ===========================================================================
 * MAIN -- the O-2 activation / z-order oracle.
 * ===========================================================================*/
int main(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };   /* (top,left,bottom,right) */
    static win_store_t WA0, WA1, WB0;
    static mgr_store_t M;
    static FlairApp    appA, appB;
    static FlairProcessList plist;

    mgr_attach(&M, FRAME);

    /* Geometry (top,left,bottom,right): three DISJOINT windows side by side so a
     * content click is unambiguously its owner's regardless of z-order. */
    rgn_rect_t A0s = { 8,   6, 30,  34 }, A0c = { 11,   7, 29,  33 };  /* app A */
    rgn_rect_t B0s = { 8,  42, 30,  70 }, B0c = { 11,  43, 29,  69 };  /* app B */
    rgn_rect_t A1s = { 8,  78, 30, 106 }, A1c = { 11,  79, 29, 105 };  /* app A */

    /* Create back-to-front so the z-order is INTERLEAVED [A0, B0, A1]: A1 first
     * (back), then B0, then A0 (front). NewWindow pushes at the z-order front. */
    win_attach(&WA1);
    NewWindow(&M.wm, &WA1.rec, A1s, A1c, documentKind, documentProc, 1);  /* A1 back */
    win_attach(&WB0);
    NewWindow(&M.wm, &WB0.rec, B0s, B0c, documentKind, documentProc, 1);  /* B0 mid  */
    win_attach(&WA0);
    NewWindow(&M.wm, &WA0.rec, A0s, A0c, documentKind, documentProc, 1);  /* A0 front*/

    /* Construct the two tenants. appA's group front window is A0 (its window list
     * head); appB's is B0. Bind every window's refCon to its owning app (Sec 3.1
     * demux key) -- _register binds only the group's front window, so A1's refCon
     * is bound explicitly here (the caller-storage path owns the windows). */
    memset(&appA, 0, sizeof appA);
    memset(&appB, 0, sizeof appB);
    appA.name = "A"; appA.procs = &g_stub_procs; appA.windows = &WA0.rec; appA.refCon = A_ID;
    appB.name = "B"; appB.procs = &g_stub_procs; appB.windows = &WB0.rec; appB.refCon = B_ID;
    g_appA = &appA; g_appB = &appB;

    FlairProcessList_init(&plist);
    /* Register B then A so A ends as the foreground head; _register binds each
     * app's group-front window's refCon = (int32_t)(uintptr_t)app. */
    FlairProcess_register(&plist, &appB);
    FlairProcess_register(&plist, &appA);
    /* Bind the SECOND A window (A1); _register only touched the group-front (A0). */
    WA1.rec.refCon = (int32_t)(uintptr_t)&appA;

    /* The INDEPENDENT expected z-order model, front-to-back, by construction. */
    WindowPtr exp_order[NWIN] = { &WA0.rec, &WB0.rec, &WA1.rec };

    /* ===== scene meaningfulness (independent of the dispatcher) ===== */
    CHECK(rect_contains(A0c, 20, 20),
          "scene: the A0-content probe (v20,h20) genuinely lies in A0's content");
    CHECK(rect_contains(B0c, 56, 20),
          "scene: the B0-content probe (v20,h56) genuinely lies in B0's content");
    CHECK(!rect_contains(A0c, 56, 20) && !rect_contains(B0c, 20, 20) &&
          !rect_contains(A1c, 20, 20),
          "scene: the windows are disjoint at the probe points (z-order cannot confound)");
    CHECK(plist.head == &appA,
          "scene: A is the foreground head, B is background (register ordering)");
    CHECK(zorder_matches(&M.wm, exp_order, NWIN),
          "scene: the initial z-order is the INTERLEAVED [A0, B0, A1]");
    CHECK(exp_order[1] == &WB0.rec,
          "scene: B0 sits BETWEEN A0 and A1 -> app A's group is NOT yet contiguous (meaningful)");
    CHECK(recover_owner(&WA0.rec) == &appA && recover_owner(&WA1.rec) == &appA &&
          recover_owner(&WB0.rec) == &appB,
          "scene: every window's refCon round-trips to its correct owner (binding rule)");

    /* ===== switch 1: mouseDown inContent over BACKGROUND B0 -> switch to B ===== */
    WindowPtr old_front_1 = M.wm.front;                  /* (A0; for the mutant)    */
    (void)old_front_1;                                   /* used only under NO_HILITE*/
    EventRecord ev1 = mk_event(mouseDown, /*v*/20, /*h*/56);
    flair_app_dispatch(&plist, &M.wm, &ev1);

    /* INDEPENDENT expected state after the switch to B. */
    expected_raise_group(exp_order, NWIN, &appB);        /* -> [B0, A0, A1]         */
    WindowPtr exp_hilite_1 = appB.windows;               /* the new front (B0)      */
#ifdef ACTIVATE_MUT_NO_HILITE
    exp_hilite_1 = old_front_1;                           /* mutant: hilite stays put*/
#endif

    CHECK(plist.head == &appB,
          "switch1: after the background click, B is promoted to the foreground head");
    CHECK(zorder_matches(&M.wm, exp_order, NWIN),
          "switch1: B's whole group is the front run -> z-order [B0, A0, A1]");
    {
        int hbad = 0;
        const WindowPtr scene[NWIN] = { &WA0.rec, &WA1.rec, &WB0.rec };
        for (int i = 0; i < NWIN; i++) {
            int want = (scene[i] == exp_hilite_1) ? 1 : 0;
            if (scene[i]->hilited != want) hbad = 1;
        }
        CHECK(!hbad,
              "switch1: hilited==1 on EXACTLY wm->front (B0), 0 on all others");
    }
    CHECK(M.wm.front->hilited == 1,
          "switch1: the new front window (wm->front) is hilited");
    /* the previously-foreground app A's front window (A0) received the deactivate. */
    CHECK(g_log_n >= 1 && is_deactivate_of(&g_log[0], A_ID, &WA0.rec),
          "switch1: A's front window (A0) received the deactivate (activateEvt, active=0)");
    CHECK(g_log_n >= 2 && is_activate_of(&g_log[1], B_ID, &WB0.rec),
          "switch1: B's front window (B0) received the activate (activateEvt, active=1)");

    /* ===== switch 2: mouseDown inContent over A0 -> switch BACK to A ===== */
    WindowPtr old_front_2 = M.wm.front;                  /* (B0; for the mutant)    */
    (void)old_front_2;                                   /* used only under NO_HILITE*/
    int log_before_2 = g_log_n;
    EventRecord ev2 = mk_event(mouseDown, /*v*/20, /*h*/20);
    flair_app_dispatch(&plist, &M.wm, &ev2);

    /* INDEPENDENT expected state after the switch back to A. */
    expected_raise_group(exp_order, NWIN, &appA);        /* -> [A0, A1, B0]         */
    WindowPtr exp_hilite_2 = appA.windows;               /* the new front (A0)      */
#ifdef ACTIVATE_MUT_NO_HILITE
    exp_hilite_2 = old_front_2;                           /* mutant: hilite stays put*/
#endif

    CHECK(plist.head == &appA,
          "switch2: after clicking A, A is promoted back to the foreground head");
    CHECK(zorder_matches(&M.wm, exp_order, NWIN),
          "switch2: A's WHOLE group is the front CONTIGUOUS run in ORIGINAL order [A0, A1, B0]");
    /* spell out the contiguity + internal order explicitly (the headline O-2 claim). */
    CHECK(M.wm.front == &WA0.rec && M.wm.front->nextWindow == &WA1.rec,
          "switch2: A0 then A1 are the front contiguous pair, A0 ahead of A1 (internal order kept)");
    CHECK(owns(&appA, M.wm.front) && owns(&appA, M.wm.front->nextWindow) &&
          !owns(&appA, M.wm.front->nextWindow->nextWindow),
          "switch2: the front |group A|=2 windows are A-owned; the 3rd (B0) is not");
    {
        int hbad = 0;
        const WindowPtr scene[NWIN] = { &WA0.rec, &WA1.rec, &WB0.rec };
        for (int i = 0; i < NWIN; i++) {
            int want = (scene[i] == exp_hilite_2) ? 1 : 0;
            if (scene[i]->hilited != want) hbad = 1;
        }
        CHECK(!hbad,
              "switch2: hilited==1 on EXACTLY wm->front (A0), 0 on all others");
    }
    /* refCon round-trips to the right tenant for EVERY window (post-switch). */
    CHECK(recover_owner(&WA0.rec) == &appA && recover_owner(&WA1.rec) == &appA &&
          recover_owner(&WB0.rec) == &appB,
          "switch2: every window's refCon still round-trips to its correct owner");
    /* the previously-foreground app B's front window (B0) received the deactivate. */
    CHECK(g_log_n >= log_before_2 + 2 &&
          is_deactivate_of(&g_log[log_before_2], B_ID, &WB0.rec),
          "switch2: B's front window (B0) received the deactivate (activateEvt, active=0)");
    CHECK(is_activate_of(&g_log[log_before_2 + 1], A_ID, &WA0.rec),
          "switch2: A's front window (A0) received the activate (activateEvt, active=1)");

    return TEST_SUMMARY("test-process-activate");
}

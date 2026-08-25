/* test_process.c -- the FLAIR App Contract O-1 host routing/dispatch oracle.
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013 Sec 7 O-1).
 *        The HOST oracle for the Layer-5 dispatcher `flair_app_dispatch`
 *        (os/flair/process.c) -- the refCon demux + click-to-activate + the
 *        activateEvt deactivate/activate pair (ADR-0013 Sec 3.3). It runs the
 *        EXACT symbol the live kmain pump runs (BC-2 single spine), so a green
 *        O-1 certifies the live routing too.
 *
 * ORACLE-FIRST (Rule 1, Law 2). Wave 1 ships a STUB dispatcher (process.c does
 * NO routing/activation), so THIS oracle is RED for the RIGHT reason: the
 * delivery log stays empty and every routing/activation assertion fails. Wave 2
 * fills in flair_app_dispatch and this oracle turns GREEN. (The mutants below
 * prove the checks are live differentials once that real dispatcher exists.)
 *
 * INDEPENDENT-BY-RECOMPUTATION (Law 2; the test_interact idiom). The expected
 * delivery sequence is a HAND-AUTHORED golden -- never read back out of the
 * dispatcher. Each stub tenant's `event` handler APPENDS (app-id, what,
 * where/active-flag) to a global log; the oracle diffs that log against the
 * by-hand expected sequence. An oracle that asked the dispatcher what the
 * dispatcher did would agree BY CONSTRUCTION and is forbidden (Law 2; HER-02).
 *
 * THE SCENE: two single-window tenants, A (foreground/front) and B (background/
 * behind), DISJOINT in x so a click in B's content is unambiguously B's even
 * though B is behind (no overlap that would let front A win the hit-test).
 *
 * THE SCRIPTED TRACE + HAND-AUTHORED GOLDEN (ADR-0013 Sec 7 O-1):
 *   (a) mouseDown inContent over A (already fg) -> A.event(mouseDown); B silent.
 *   (b) keyDown (cursor over B) -> the FOREGROUND app A only; never the window
 *       under the cursor (System 7: the active window IS keyboard focus).
 *   (c) mouseDown inContent over background B -> click-to-activate: EXACTLY one
 *       deactivate (A.event activateEvt, active-flag=0) THEN one activate
 *       (B.event activateEvt, active-flag=1), in that order, THEN B.event(
 *       mouseDown). The full ordered log is asserted.
 *   (d) mouseDown inDrag over foreground B -> shell-owned chrome no-op: no
 *       delivered records, no promotion, no z-order change.
 *   (e) mouseDown inDrag over background A -> title-click activation: EXACTLY
 *       one deactivate to B THEN one activate to A, promotion + raise, and NO
 *       delivered mouseDown (a title drag is never a content click).
 *
 * MUTATION-PROVEN (Rule 6; self-mutation -- the test_interact convention). The
 * mutants perturb the INDEPENDENT EXPECTED golden (NOT the dispatcher), each so
 * that against the CORRECT future dispatcher it disagrees on a DISTINCT leg:
 *
 *   PROC_MUT_IGNORE_REFCON    -- leg (c)'s click is EXPECTED at the foreground A
 *     (refCon ignored) instead of the clicked owner B. The correct dispatcher
 *     routes to B, so leg (c) ROUTING goes RED. Proves the click reaches the
 *     refCon-demuxed owner, not blindly the foreground.
 *
 *   PROC_MUT_SKIP_ACTIVATE_PAIR -- leg (c)'s golden OMITS the deactivate/activate
 *     pair (expects only the click). The correct dispatcher emits the pair, so
 *     the leg (c) length + pair assertions go RED. Proves the switch synthesizes
 *     exactly one deactivate THEN one activate, in order.
 *
 *   PROC_MUT_KEY_TO_UNDER_CURSOR -- leg (b)'s keyDown is EXPECTED at the window
 *     under the cursor (B) instead of the foreground (A). The correct dispatcher
 *     delivers to A, so leg (b) goes RED. Proves keyboard focus follows the
 *     foreground app, not the cursor.
 *
 * Ref: ADR-0013 Sec 3.1 (tenant ABI + binding rule), Sec 3.3 (event routing &
 *      activation), Sec 7 O-1; spec/window_record.h Sec 1 (part-codes), :400
 *      (refCon); spec/event_model.h Sec 1 (what-codes), :155 (active-flag),
 *      :265-271 (EventRecord). os/flair/window.h (FindWindow/NewWindow/...).
 *      Mirrors harness/proptest/test_interact.c (three-leg host oracle, the
 *      rgn_store/win_store/mgr_store arena scaffolding, the self-mutation
 *      convention). CLAUDE.md Law 2, Rule 1, Rule 6, Rule 11, Rule 12.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "region_algebra.h"   /* the LOCKED region contract (-Ispec)             */
#include "region.h"           /* engine constructors (-Ios/flair/atkinson)       */
#include "window_record.h"    /* WindowRecord, part-codes (-Ispec)               */
#include "window.h"           /* the Window Manager (NewWindow/FindWindow/...)   */
#include "event_model.h"      /* EventRecord, what-codes, active-flag (-Ispec)   */
#include "process.h"          /* THE App Contract under test (-Ios/flair)        */
#include "test_assert.h"      /* TEST_HARNESS/CHECK/TEST_SUMMARY (-Iseed)        */

TEST_HARNESS();

/* The two tenant ids (the per-app datum the log records). */
enum { A_ID = 1, B_ID = 2 };

/* Scene frame: height GH, width GW (rgn_rect bottom/right). Wide enough for two
 * disjoint windows side by side. */
enum { GW = 96, GH = 48 };

/* ===========================================================================
 * Region storage bundles (the test_interact/test_window rgn_store arena: the
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
 * INDEPENDENT rect membership (the test_interact helper; do NOT call window.c).
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
 * The delivery log -- the SOLE observable. Each tenant `event` appends one
 * entry; the oracle diffs the log against the hand-authored golden (Law 2).
 * ===========================================================================*/
typedef struct log_entry {
    int      app_id;     /* which tenant's event handler fired (A_ID / B_ID)     */
    uint16_t what;       /* ev.what (mouseDown / keyDown / activateEvt)          */
    uint16_t modifiers;  /* ev.modifiers (carries FLAIR_EVT_MOD_ACTIVE_FLAG)     */
    int16_t  v, h;       /* ev.where (checked for mouse/key; n/a for activate)   */
} log_entry_t;

enum { LOG_CAP = 16 };
static log_entry_t g_log[LOG_CAP];
static int         g_log_n = 0;

/* The known tenants, for host-safe app-id recovery (self is a REAL pointer the
 * dispatcher passes -- never the truncated refCon; see process.h host note). */
static FlairApp *g_appA = NULL;
static FlairApp *g_appB = NULL;

static int app_id_of(const FlairApp *a)
{
    if (a == g_appA) return A_ID;
    if (a == g_appB) return B_ID;
    return -1;
}

static void log_append(int app_id, const EventRecord *ev)
{
    if (g_log_n >= LOG_CAP) return;
    g_log[g_log_n].app_id    = app_id;
    g_log[g_log_n].what      = ev->what;
    g_log[g_log_n].modifiers = ev->modifiers;
    g_log[g_log_n].v         = ev->where.v;
    g_log[g_log_n].h         = ev->where.h;
    g_log_n++;
}

/* The SHARED stub tenant `event` entry-point: record the delivery. */
static void stub_event(FlairApp *self, const EventRecord *ev)
{
    log_append(app_id_of(self), ev);
}

/* A no-op open() (the Wave-1 oracle builds the windows itself; Sec 3.2). */
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

/* Compare one log entry to an expected (app, what, active, where) tuple. For
 * activateEvt (active>=0) the active-flag is checked and `where` ignored; for
 * mouse/key (active<0) `where` is checked and the active-flag ignored. */
static int entry_match(const log_entry_t *g, int app_id, uint16_t what,
                       int active, int16_t v, int16_t h)
{
    if (g->app_id != app_id) return 0;
    if (g->what   != what)   return 0;
    if (active >= 0) {
        int got = (g->modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) ? 1 : 0;
        return got == active;
    }
    return (g->v == v) && (g->h == h);
}

/* Cook one EventRecord (Sec 7 O-1 scripted trace). */
static EventRecord mk_event(uint16_t what, int16_t v, int16_t h, uint16_t mods)
{
    EventRecord ev;
    memset(&ev, 0, sizeof ev);
    ev.what      = what;
    ev.where.v   = v;
    ev.where.h   = h;
    ev.modifiers = mods;
    return ev;
}

/* ===========================================================================
 * leg (f) -- THE RAISE (bug initech-tpzf): the CLICKED window comes forward.
 * ---------------------------------------------------------------------------
 * Inside Macintosh, Macintosh Toolbox Essentials, Event Manager: a mouseDown
 * anywhere in a window that is not the active one activates it first
 * (SelectWindow) and only then is the click acted on. Legs (a)-(e) all use
 * SINGLE-window tenants, where "raise the owner's group" and "raise the
 * clicked window" happen to coincide -- so they could never see the gap.
 *
 * THE GAP, MEASURED ON THE REAL GUEST and recorded in
 * spec/flair_disk_windows_traces.mk lines 141-149: two Finder disk windows
 * belong to the SAME tenant, so a click on the background one had no tenant to
 * switch, `owner == old_fg`, and NOTHING raised it -- the click dispatched but
 * the window stayed buried. Same shape one level up: a cross-tenant click on a
 * multi-window tenant raises the GROUP but leaves the clicked member inside it.
 *
 * THE SCENE (its own manager and windows, so legs (a)-(e) stay byte-stable):
 * tenant P owns TWO windows P1 (front) and P2 (behind), tenant Q owns one.
 * All three DISJOINT in x, so the hit-test is unambiguous.
 *
 * SELF-MUTATION (Rule 6; the convention this file already uses -- the mutant
 * perturbs the GOLDEN, not the dispatcher):
 *   PROC_MUT_NO_SAMETENANT_RAISE -- the golden EXPECTS the pre-fix behaviour
 *     (the clicked window stays buried; the group head never moves). The fixed
 *     dispatcher raises it, so f1/f2/f3 go RED. Restore the bug in
 *     os/flair/process.c and the ordinary (non-mutant) golden goes RED instead
 *     -- the same discriminating assertion, from either side.
 *
 * WL-0076 ACTIVATION CONTRACT: a same-tenant raise is NOT an app switch, so it
 * must emit NO activateEvt pair (both windows belong to the same tenant, which
 * never stopped being foreground). Asserted explicitly below; the cross-tenant
 * leg still emits exactly one deactivate THEN one activate, unchanged.
 * ===========================================================================*/
enum { P_ID = 3, Q_ID = 4 };
enum { GW2 = 160, GH2 = 64 };

static FlairApp *g_appP = NULL;
static FlairApp *g_appQ = NULL;

static void pq_event(FlairApp *self, const EventRecord *ev)
{
    log_append(self == g_appP ? P_ID : (self == g_appQ ? Q_ID : -1), ev);
}

static const FlairAppProcs g_pq_procs = { stub_open, pq_event, NULL, NULL };

static void leg_same_tenant_raise(void)
{
    rgn_rect_t FRAME2 = { 0, 0, GH2, GW2 };
    static win_store_t WP1, WP2, WQ;
    static mgr_store_t M2;
    static FlairApp    appP, appQ;
    static FlairProcessList plist2;

    /* Hand-authored geometry (top,left,bottom,right), pairwise disjoint in x. */
    rgn_rect_t P1s = { 8,   6, 30,  44 }, P1c = { 11,   7, 29,  43 };
    rgn_rect_t P2s = { 8,  56, 30,  94 }, P2c = { 11,  57, 29,  93 };
    rgn_rect_t Qs  = { 8, 106, 30, 144 }, Qc  = { 11, 107, 29, 143 };
    int before;

    /* The pre-fix expectation, switched on by the self-mutation. */
#ifdef PROC_MUT_NO_SAMETENANT_RAISE
    const int raises = 0;
#else
    const int raises = 1;
#endif

    mgr_attach(&M2, FRAME2);

    /* z-order back-to-front: Q created first (deepest), then P2, then P1. */
    win_attach(&WQ);
    NewWindow(&M2.wm, &WQ.rec,  Qs,  Qc, documentKind, documentProc, 1);
    win_attach(&WP2);
    NewWindow(&M2.wm, &WP2.rec, P2s, P2c, documentKind, documentProc, 1);
    win_attach(&WP1);
    NewWindow(&M2.wm, &WP1.rec, P1s, P1c, documentKind, documentProc, 1);

    memset(&appP, 0, sizeof appP);
    memset(&appQ, 0, sizeof appQ);
    appP.name = "P"; appP.procs = &g_pq_procs; appP.windows = &WP1.rec;
    appQ.name = "Q"; appQ.procs = &g_pq_procs; appQ.windows = &WQ.rec;
    g_appP = &appP; g_appQ = &appQ;

    FlairProcessList_init(&plist2);
    FlairProcess_register(&plist2, &appQ);
    FlairProcess_register(&plist2, &appP);   /* P registered last => foreground */

    /* FlairProcess_register binds only the app's HEAD window's refCon; P's
     * SECOND window has to be bound the same way (the Sec 3.1 demux key), which
     * is exactly what the Finder does for every disk window it opens
     * (os/flair/finder_windows.c: "w->rec.refCon = (int32_t)(uintptr_t)sh->app"). */
    WP2.rec.refCon = (int32_t)(uintptr_t)&appP;

    /* Scene meaningfulness, by INDEPENDENT rect arithmetic (not FindWindow). */
    CHECK(rect_contains(P2c, 70, 20) && !rect_contains(P1s, 70, 20) &&
          !rect_contains(Qs, 70, 20),
          "leg(f) scene: the P2-content probe (v20,h70) is unambiguously P2's");
    CHECK(rect_contains(P1s, 20, 9) && !rect_contains(P1c, 20, 9),
          "leg(f) scene: the P1 title probe lies in structure but outside content");
    CHECK(plist2.head == &appP && M2.wm.front == &WP1.rec,
          "leg(f) scene: P is foreground and P1 -- not P2 -- is the front window");

    /* --- f1: SAME tenant, content click on the buried window P2 ----------- */
    before = g_log_n;
    {
        EventRecord ev = mk_event(mouseDown, /*v*/20, /*h*/70, 0);
        flair_app_dispatch(&plist2, &M2.wm, &ev);
    }
    CHECK(g_log_n == before + 1,
          "leg(f1): a same-tenant content click delivers EXACTLY the mouseDown");
    CHECK(g_log_n >= before + 1 &&
          entry_match(&g_log[before], P_ID, mouseDown, -1, 20, 70),
          "leg(f1): ... to its owner P");
    {
        int activates = 0;
        for (int i = before; i < g_log_n; i++)
            if (g_log[i].what == activateEvt) activates++;
        CHECK(activates == 0,
              "leg(f1): a same-tenant raise is NOT an app switch -- ZERO "
              "activateEvt records (the WL-0076 activation contract)");
    }
    CHECK(M2.wm.front == (raises ? &WP2.rec : &WP1.rec),
          "leg(f1): the CLICKED window comes to the front of the z-order "
          "even though its tenant was already foreground (bug initech-tpzf)");
    CHECK(appP.windows == (raises ? &WP2.rec : &WP1.rec),
          "leg(f1): ... and the tenant's group head follows it (no stale head)");
    CHECK(plist2.head == &appP,
          "leg(f1): the foreground tenant is unchanged -- P was already it");

    /* --- f2: SAME tenant, TITLE click on the now-buried window P1 --------- */
    before = g_log_n;
    {
        EventRecord ev = mk_event(mouseDown, /*v*/9, /*h*/20, 0);
        flair_app_dispatch(&plist2, &M2.wm, &ev);
    }
    CHECK(g_log_n == before,
          "leg(f2): a title mouseDown is NEVER delivered to the tenant (O-1 leg(e))");
    CHECK(M2.wm.front == (raises ? &WP1.rec : &WP2.rec),
          "leg(f2): a TITLE click raises the clicked window too -- the exact "
          "gesture spec/flair_disk_windows_traces.mk measured as broken");

    /* --- f3: CROSS tenant, click a window that is not its group's front --- */
    /* First make Q the foreground by clicking its content (the ordinary path). */
    before = g_log_n;
    {
        EventRecord ev = mk_event(mouseDown, /*v*/20, /*h*/120, 0);
        flair_app_dispatch(&plist2, &M2.wm, &ev);
    }
    CHECK(plist2.head == &appQ && M2.wm.front == &WQ.rec,
          "leg(f3) setup: Q is foreground with its single window at the front");

    /* P's group is now behind Q, with P1 ahead of P2 inside it (f2 raised P1).
     * Clicking P2 must switch to P *and* leave P2 -- the clicked member, not the
     * group's own front P1 -- frontmost. */
    before = g_log_n;
    {
        EventRecord ev = mk_event(mouseDown, /*v*/20, /*h*/70, 0);
        flair_app_dispatch(&plist2, &M2.wm, &ev);
    }
    CHECK(g_log_n == before + 3,
          "leg(f3): a cross-tenant click still emits the pair THEN the click");
    CHECK(g_log_n >= before + 2 &&
          entry_match(&g_log[before], Q_ID, activateEvt, 0, 0, 0) &&
          entry_match(&g_log[before + 1], P_ID, activateEvt, 1, 0, 0),
          "leg(f3): EXACTLY one deactivate to Q THEN one activate to P "
          "(the WL-0076 contract is untouched by the raise)");
    CHECK(g_log_n >= before + 3 &&
          entry_match(&g_log[before + 2], P_ID, mouseDown, -1, 20, 70),
          "leg(f3): THEN the mouseDown reaches P");
    CHECK(plist2.head == &appP,
          "leg(f3): P is promoted to the foreground");
    CHECK(M2.wm.front == (raises ? &WP2.rec : &WP1.rec),
          "leg(f3): the clicked member -- not the group's old front -- ends "
          "frontmost after a whole-group raise");
}

/* ===========================================================================
 * MAIN -- the O-1 routing/dispatch oracle.
 * ===========================================================================*/
int main(void)
{
    rgn_rect_t FRAME = { 0, 0, GH, GW };   /* (top,left,bottom,right) */
    static win_store_t WA, WB;
    static mgr_store_t M;
    static FlairApp    appA, appB;
    static FlairProcessList plist;

    mgr_attach(&M, FRAME);

    /* Geometry (top,left,bottom,right -- the LOCKED rgn_rect order): A on the
     * left, B on the right, DISJOINT in x so the front-most rule cannot confound
     * the routing assertions (B's content is reachable though B is behind). */
    rgn_rect_t As = { 8,  6, 30, 34 }, Ac = { 11,  7, 29, 33 };
    rgn_rect_t Bs = { 8, 50, 30, 78 }, Bc = { 11, 51, 29, 77 };

    /* B created FIRST (back), A created SECOND (front == foreground). */
    win_attach(&WB);
    NewWindow(&M.wm, &WB.rec, Bs, Bc, documentKind, documentProc, 1);   /* B back  */
    win_attach(&WA);
    NewWindow(&M.wm, &WA.rec, As, Ac, documentKind, documentProc, 1);   /* A front */

    /* Construct the two tenants (procs + name + the single owned window + the
     * per-app id datum). */
    memset(&appA, 0, sizeof appA);
    memset(&appB, 0, sizeof appB);
    appA.name = "A"; appA.procs = &g_stub_procs; appA.windows = &WA.rec; appA.refCon = A_ID;
    appB.name = "B"; appB.procs = &g_stub_procs; appB.windows = &WB.rec; appB.refCon = B_ID;
    g_appA = &appA; g_appB = &appB;

    FlairProcessList_init(&plist);
    /* Register B then A so A ends as the foreground head; register binds each
     * owned window's refCon = (int32_t)(uintptr_t)app (the demux key, Sec 3.1).
     * O-1 uses the caller-storage registrar (the two appA/appB are static here);
     * the arena-allocating production launch (FlairProcess_launch) is graded by
     * the O-3 teardown/leak + O-4 budget oracles (test_process_teardown/budget). */
    FlairProcess_register(&plist, &appB);
    FlairProcess_register(&plist, &appA);

    /* --- scene meaningfulness (INDEPENDENT rect arithmetic, NOT FindWindow) --- */
    CHECK(rect_contains(Ac, 20, 20),
          "scene: the A-content probe (v20,h20) genuinely lies in A's content");
    CHECK(rect_contains(Bc, 64, 20),
          "scene: the B-content probe (v20,h64) genuinely lies in B's content");
    CHECK(!rect_contains(As, 64, 20) && !rect_contains(Bs, 20, 20),
          "scene: A and B are disjoint at the probe points (z-order cannot confound)");
    CHECK(rect_contains(As, 20, 9) && !rect_contains(Ac, 20, 9) &&
          rect_contains(Bs, 64, 9) && !rect_contains(Bc, 64, 9),
          "scene: the A/B title probes lie in structure but outside content");
    CHECK(plist.head == &appA,
          "scene: A is the foreground head, B is background (launch ordering)");

    /* ===== leg (a): mouseDown inContent over A (already foreground) ===== */
    EventRecord ev_a = mk_event(mouseDown, /*v*/20, /*h*/20, 0);
    flair_app_dispatch(&plist, &M.wm, &ev_a);
    CHECK(g_log_n == 1,
          "leg(a): mouseDown over foreground A delivers EXACTLY one event (B stays silent)");
    CHECK(g_log_n >= 1 && entry_match(&g_log[0], A_ID, mouseDown, -1, 20, 20),
          "leg(a): mouseDown inContent over A -> A.event(mouseDown) at (v20,h20)");

    /* ===== leg (b): keyDown -> the FOREGROUND app only ===== */
    /* The cursor sits over BACKGROUND B; keyDown must STILL go to foreground A. */
    EventRecord ev_b = mk_event(keyDown, /*v*/20, /*h*/64, 0);
    flair_app_dispatch(&plist, &M.wm, &ev_b);
    int kb_app = A_ID;
#ifdef PROC_MUT_KEY_TO_UNDER_CURSOR
    kb_app = B_ID;   /* self-mutation: EXPECT the key at the window under the cursor */
#endif
    CHECK(g_log_n == 2,
          "leg(b): keyDown delivers to EXACTLY one app");
    CHECK(g_log_n >= 2 && entry_match(&g_log[1], kb_app, keyDown, -1, 20, 64),
          "leg(b): keyDown routes to the FOREGROUND app A, not the window under the cursor");

    /* ===== leg (c): mouseDown inContent over BACKGROUND B -> click-to-activate ===== */
    EventRecord ev_c = mk_event(mouseDown, /*v*/20, /*h*/64, 0);
    flair_app_dispatch(&plist, &M.wm, &ev_c);
    int click_app = B_ID;
#ifdef PROC_MUT_IGNORE_REFCON
    click_app = A_ID;   /* self-mutation: EXPECT the click at fg A (refCon ignored) */
#endif

#ifdef PROC_MUT_SKIP_ACTIVATE_PAIR
    /* self-mutation: EXPECT no deactivate/activate pair -- only the click. */
    CHECK(g_log_n == 3,
          "leg(c): background click delivers ONE event (skip-activate-pair golden)");
    CHECK(g_log_n >= 3 && entry_match(&g_log[2], click_app, mouseDown, -1, 20, 64),
          "leg(c): mouseDown delivered to the clicked owner (skip-activate-pair golden)");
#else
    CHECK(g_log_n == 5,
          "leg(c): click-to-activate emits the deactivate+activate pair THEN the click (3 new entries)");
    CHECK(g_log_n >= 3 && entry_match(&g_log[2], A_ID, activateEvt, 0, 0, 0),
          "leg(c): FIRST a deactivate (activateEvt, active-flag=0) to old-foreground A");
    CHECK(g_log_n >= 4 && entry_match(&g_log[3], B_ID, activateEvt, 1, 0, 0),
          "leg(c): THEN an activate (activateEvt, active-flag=1) to new-foreground B");
    CHECK(g_log_n >= 5 && entry_match(&g_log[4], click_app, mouseDown, -1, 20, 64),
          "leg(c): THEN the mouseDown is delivered to the clicked owner B");
    /* ordering capstone: EXACTLY one deactivate THEN one activate (Rule 11). */
    CHECK(g_log_n >= 4 &&
          g_log[2].what == activateEvt &&
          (g_log[2].modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) == 0 &&
          g_log[3].what == activateEvt &&
          (g_log[3].modifiers & FLAIR_EVT_MOD_ACTIVE_FLAG) != 0,
          "leg(c): the pair is EXACTLY one deactivate THEN one activate, in deterministic order");
#endif

    /* foreground switched after the background click (independent end-state). */
    CHECK(plist.head == &appB,
          "leg(c): after the background click, B is promoted to the foreground head");

    /* ===== leg (d): mouseDown inDrag over FOREGROUND B -> pure no-op ===== */
    {
        int before = g_log_n;
        EventRecord ev_d = mk_event(mouseDown, /*v*/9, /*h*/64, 0);
        flair_app_dispatch(&plist, &M.wm, &ev_d);
        CHECK(g_log_n == before,
              "leg(d): inDrag on foreground B delivers no tenant records");
        CHECK(plist.head == &appB && M.wm.front == &WB.rec,
              "leg(d): foreground-title inDrag changes neither app head nor z-order");
    }

    /* ===== leg (e): mouseDown inDrag over BACKGROUND A -> switch, no click ===== */
    {
        int before = g_log_n;
        int mouse_deliveries = 0;
        EventRecord ev_e = mk_event(mouseDown, /*v*/9, /*h*/20, 0);
        flair_app_dispatch(&plist, &M.wm, &ev_e);

        CHECK(g_log_n == before + 2,
              "leg(e): background-title switch emits exactly two records (activate pair only)");
        CHECK(g_log_n >= before + 1 &&
              entry_match(&g_log[before], B_ID, activateEvt, 0, 0, 0),
              "leg(e): FIRST a deactivate to old-foreground B");
        CHECK(g_log_n >= before + 2 &&
              entry_match(&g_log[before + 1], A_ID, activateEvt, 1, 0, 0),
              "leg(e): THEN an activate to new-foreground A");
        for (int i = before; i < g_log_n; i++)
            if (g_log[i].what == mouseDown) mouse_deliveries++;
        CHECK(mouse_deliveries == 0,
              "leg(e): title activation delivers ZERO content mouseDown records");
        CHECK(plist.head == &appA && M.wm.front == &WA.rec,
              "leg(e): background-title switch promotes A and raises its window once");
    }

    leg_same_tenant_raise();

    return TEST_SUMMARY("test-process");
}

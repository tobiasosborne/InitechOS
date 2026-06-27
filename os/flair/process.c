/*
 * os/flair/process.c -- the FLAIR App Contract: tenant lifecycle + the Layer-5
 * dispatcher (THE ARTIFACT). WAVE-1 (ORACLE-FIRST) increment.
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013, RATIFIED).
 *
 * STATUS (Law 2, oracle-first):
 *   - FlairProcess_register is the thin caller-storage registrar (no arena, no
 *     open()) the O-1 host oracle uses to wire the two stub tenants over
 *     caller-built windows (stamp magic, bind the demux refCon, link/demote the
 *     foreground).
 *   - FlairProcess_launch / FlairProcess_terminate are the ARENA memory model
 *     (Wave 3a; ADR-0013 Sec 3.2/3.4/3.6): launch carves a HANDLE FlairApp + a
 *     GENERAL child block from the master heap (fail-loud budget, O-4), lays a
 *     child arena, calls procs->open(), SelectWindows + links the foreground;
 *     terminate runs close() + DisposeWindow(owned) + the one-shot child-block
 *     free + foreground promotion. CLEAN teardown only -- the corrupt-arena death
 *     path is deferred (bead initech-ubd0). Graded by O-3 (teardown/leak) +
 *     O-4 (launch budget).
 *   - flair_app_dispatch (Wave 2) routes + synthesizes the activateEvt pair;
 *     graded GREEN by O-1 (harness/proptest/test_process.c).
 *
 * Freestanding (Law 3): <stdint.h> via process.h; no libc, no malloc. Compiles
 * both under the kernel flags and hosted for the property suite.
 *
 * Ref: ADR-0013 Sec 2 (single spine), Sec 3.1 (binding rule + magic tag, BC-3),
 *      Sec 3.2 (launch), Sec 3.3 (event routing & activation), Sec 3.4 (teardown),
 *      Sec 7 O-1 (the host oracle this stub must fail), Sec 8 (staging).
 *      CLAUDE.md Law 2, Law 3, Rule 1 (Red->Green), Rule 2 (fail loud), Rule 12.
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#include "process.h"

/* ---------------------------------------------------------------------------
 * Fail-loud (dual fail-loud, mirroring os/flair/window.c WIN_PANIC: panic
 * in-kernel / abort hosted). Self-contained so process.c dual-compiles with
 * only the locked headers (Rule 2 -- a panic with context beats a silently
 * mis-routed event). Ref: window.c:54-60 (the same idiom).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define PROC_PANIC(msg)  abort()
#else
/* Freestanding: a deliberate, loud, deterministic hang (Rule 2 / Rule 11). */
#  define PROC_PANIC(msg)  do { for (;;) { } } while (0)
#endif

/* --------------------------------------------------------------------------
 * FlairProcessList_init -- empty resident-app list.
 * -------------------------------------------------------------------------- */
void FlairProcessList_init(FlairProcessList *list)
{
    if (list == NULL) return;   /* a NULL list is the caller's contract error */
    list->head = NULL;
}

/* proc_zero -- libc-free byte zero (freestanding; no memset dependency, matching
 * heap.c's no-libc discipline, Law 3). */
static void proc_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) b[i] = 0u;
}

/* --------------------------------------------------------------------------
 * FlairProcess_register -- caller-storage registration (the O-1 host-oracle path).
 * Stamp magic + bind the owned window's demux refCon + link at the head as the
 * new foreground, demoting the previous foreground to background. It carves NO
 * arena and calls NO procs->open(): the caller owns the FlairApp storage and has
 * already built the window. The arena-allocating production launch is
 * FlairProcess_launch (Sec 3.2). Ref: ADR-0013 Sec 3.1 (binding rule + magic).
 * -------------------------------------------------------------------------- */
FlairApp *FlairProcess_register(FlairProcessList *list, FlairApp *app)
{
    if (list == NULL || app == NULL) return NULL;

    /* BC-3: the refCon-tag sentinel. Every later refCon->FlairApp* cast checks it. */
    app->magic = FLAIR_APP_MAGIC;

    /* The binding rule (Sec 3.1): bind the owned window's refCon to this app so
     * FindWindow -> WindowPtr -> refCon -> FlairApp* demuxes to the right tenant.
     * (On a flat-32 kernel the cast round-trips; the 64-bit-host truncation hazard
     * is documented in process.h.) */
    if (app->windows != NULL)
        app->windows->refCon = (int32_t)(uintptr_t)app;

    /* Demote the old foreground; promote this app as the new foreground head. */
    if (list->head != NULL)
        list->head->state = (uint8_t)FLAIR_APP_BG;
    app->state   = (uint8_t)FLAIR_APP_FG;
    app->nextApp = list->head;
    list->head   = app;
    return app;
}

/* --------------------------------------------------------------------------
 * FlairProcess_launch -- the ARENA-ALLOCATING Toolbox launch (ADR-0013 Sec 3.2).
 *
 * Carve a tenant out of the master FLAIR heap and bring it up as foreground.
 * Ref: ADR-0013 Sec 3.2 (steps a-g), Sec 3.6 (child sub-arena), Sec 7 O-4 /
 *      BC-5 (fail-loud launch budget -- never overcommit, never partial install).
 * -------------------------------------------------------------------------- */
FlairApp *FlairProcess_launch(FlairProcessList *list, WindowMgr *wm,
                             flair_heap_t *master, const FlairAppProcs *procs,
                             const char *name, rgn_rect_t bounds,
                             uint32_t budget)
{
    if (list == NULL || wm == NULL || master == NULL || procs == NULL)
        PROC_PANIC("launch: NULL argument (contract error)");

    /* (a) the FlairApp handle from the master heap (HANDLE class). */
    FlairApp *app = (FlairApp *)flair_alloc(master, FLAIR_CLASS_HANDLE,
                                            (uint32_t)sizeof(FlairApp));
    /* (b) the child partition block of the app's budget (GENERAL class). */
    void *block = (budget > 0u)
                ? flair_alloc(master, FLAIR_CLASS_GENERAL, budget)
                : (void *)0;

    /* (O-4 / BC-5) Fail-loud budget: if EITHER carve failed, reclaim whatever we
     * got, install NOTHING, leave `list`/`wm` unchanged, and return NULL. Do NOT
     * overcommit, do NOT partially install a window/menubar (Rule 2). */
    if (app == NULL || block == NULL) {
#ifdef BUDGET_MUT_OVERCOMMIT
        /* MUTANT BUDGET_MUT_OVERCOMMIT (Rule 6; budget-mutant build only): instead
         * of fail-loud reclaim, OVERCOMMIT -- partially install the (incompletely
         * carved) app anyway. O-4 expects NULL + no install, so this goes RED.
         * NEVER define in a real build. */
        if (app != NULL) {
            app->block   = block;          /* may be NULL */
            app->magic   = FLAIR_APP_MAGIC;
            app->nextApp = list->head;
            list->head   = app;            /* partial install */
        }
        return app;                        /* non-NULL on the handle-ok path */
#else
        if (block != NULL) flair_free(master, FLAIR_CLASS_GENERAL, block);
        if (app   != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  app);
        return NULL;
#endif
    }

    /* (c) zero + init the handle; lay a CHILD heap over the partition block. */
    proc_zero(app, (uint32_t)sizeof(FlairApp));
    flair_heap_init(&app->arena, block, budget);
    app->magic   = FLAIR_APP_MAGIC;        /* BC-3 refCon-tag sentinel            */
    app->name    = name;
    app->procs   = procs;
    app->block   = block;
    app->menubar = (struct MenuBar *)0;
    app->windows = (WindowPtr)0;
    app->refCon  = 0;
    app->state   = (uint8_t)FLAIR_APP_FG;  /* provisional; affirmed on link (f)   */
    app->nextApp = (FlairApp *)0;

    /* (d) build the app's world from its child arena. open() does NewWindow into
     * app->arena and sets each w->refCon = (int32_t)(uintptr_t)app (Sec 3.1). */
    {
        FlairLaunchParams lp;
        lp.bounds = bounds;
        lp.budget = budget;
        if (procs->open != NULL && procs->open(app, &lp) != 0) {
#ifdef BUDGET_MUT_OVERCOMMIT
            /* MUTANT (Rule 6): open() failed but install anyway (partial install).
             * O-4 expects NULL + reclaim, so this goes RED. NEVER in a real build. */
            app->nextApp = list->head;
            list->head   = app;
            return app;
#else
            /* (O-4 open-fail) reclaim block + handle, install nothing, return NULL.
             * The contract (Sec 3.1) is open()!=0 == launch fail. CLEAN failures
             * (the O-4 stub) thread no window before failing; the corrupt/partial
             * open() path -- like corrupt-arena death -- is deferred (initech-ubd0). */
            flair_free(master, FLAIR_CLASS_GENERAL, app->block);
            flair_free(master, FLAIR_CLASS_HANDLE,  app);
            return NULL;
#endif
        }
    }

    /* (e) raise the app's front window to the z-order front + activate it. */
    if (app->windows != NULL)
        SelectWindow(wm, app->windows);

    /* (f) link at the head as the new foreground; demote the old head to BG. */
    if (list->head != NULL)
        list->head->state = (uint8_t)FLAIR_APP_BG;
    app->state   = (uint8_t)FLAIR_APP_FG;
    app->nextApp = list->head;
    list->head   = app;

    /* (g) return the launched tenant. */
    return app;
}

/* --------------------------------------------------------------------------
 * FlairProcess_terminate -- CLEAN teardown (ADR-0013 Sec 3.4).
 *
 * For a tenant brought up by FlairProcess_launch (app->block carved from
 * `master`). The one-shot child-block free is the clean-death property (Sec 3.6).
 *
 * SCOPE LIMIT (orchestrator ruling, bead initech-ubd0): CLEAN teardown only. The
 * "app-death survives a CORRUPTED child arena" path (ADR Sec 3.4 vs Sec 3.6
 * tension -- the WindowRecords live IN the child arena, so step (2)'s window
 * removal reads that arena) is DEFERRED pending a committee ruling and is NOT
 * implemented here.
 * -------------------------------------------------------------------------- */
void FlairProcess_terminate(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app)
{
    if (list == NULL || wm == NULL || master == NULL || app == NULL) return;

    /* BC-3: a teardown of a non-tenant (scribbled/foreign handle) fails loud
     * rather than freeing arbitrary memory (Rule 2). */
    if (app->magic != FLAIR_APP_MAGIC)
        PROC_PANIC("terminate: app->magic mismatch (BC-3)");

    app->state = (uint8_t)FLAIR_APP_DYING;

    /* (1) optional close hook (flush docs); the shell tears down regardless. */
    if (app->procs != NULL && app->procs->close != NULL)
        app->procs->close(app);

    /* (2) DisposeWindow every window in the wm z-order OWNED by app -- matched by
     * refCon == (int32_t)(uintptr_t)app (the SAME demux key open() set, Sec 3.1).
     * DisposeWindow mutates the z-order, so re-scan from wm->front after each
     * removal. (CLEAN teardown reads the child arena where the WindowRecords live;
     * the corrupt-arena death path is deferred -- bead initech-ubd0.) */
    for (;;) {
        WindowPtr owned = (WindowPtr)0;
        for (WindowPtr w = wm->front; w != (WindowPtr)0; w = w->nextWindow) {
            if (w->refCon == (int32_t)(uintptr_t)app) { owned = w; break; }
        }
        if (owned == (WindowPtr)0) break;
        DisposeWindow(wm, owned);
    }

    /* Capture the successor + foreground status while `app` is still fully valid,
     * then unlink -- so the one-shot frees below are the LAST touch of `app`
     * (no use-after-free; Rule 2/3). */
    int       was_fg = (list->head == app);
    FlairApp *succ   = (FlairApp *)0;
    {
        FlairApp **pp = &list->head;
        while (*pp != (FlairApp *)0 && *pp != app)
            pp = &(*pp)->nextApp;
        if (*pp == app) { succ = app->nextApp; *pp = app->nextApp; }
    }

    /* (5) if app was the foreground head, promote the next app: raise its front
     * window (the deactivate/activate pair is the dispatcher's job on the live
     * click path; on teardown we re-front + re-state the successor). */
    if (was_fg && succ != (FlairApp *)0) {
        if (succ->windows != (WindowPtr)0)
            SelectWindow(wm, succ->windows);
        succ->state = (uint8_t)FLAIR_APP_FG;
    }

    /* (3) ONE-SHOT free: the child partition block (GENERAL) first ... */
#ifndef TEARDOWN_MUT_LEAK_BLOCK
    flair_free(master, FLAIR_CLASS_GENERAL, app->block);
#else
    /* MUTANT TEARDOWN_MUT_LEAK_BLOCK (Rule 6; teardown-mutant build only): SKIP the
     * child-block free -> the GENERAL block leaks -> flair_heap_avail drifts
     * monotonically DOWN per launch/terminate cycle (the bump cursor is never
     * rolled back), so O-3's "avail stable across cycles" check goes RED. NEVER
     * define in a real build. */
#endif
    /* ... then the handle (HANDLE) LAST. Do not touch `app` after this. */
    flair_free(master, FLAIR_CLASS_HANDLE, app);
}

/* --------------------------------------------------------------------------
 * owner_of_window -- WIDTH-PORTABLE refCon demux (BC-3 + the Wave-2 host ruling).
 *
 * Recover the FlairApp that owns WindowPtr `w`. Do NOT cast w->refCon back to a
 * pointer: on the 64-bit HOST (test-process; no -m32 multilib) the launch-time
 * `w->refCon = (int32_t)(uintptr_t)app` TRUNCATES the pointer, so the cast back
 * is LOSSY (process.h HOST-SAFETY NOTE). Instead MATCH: scan the process list and
 * return the app whose own truncated identity equals the stored refCon, i.e.
 *     (int32_t)(uintptr_t)app == w->refCon
 * applying the SAME truncation to both sides -- exact on the flat-32 target,
 * truncated-equals-truncated on host-64 -- so it is width-portable. `magic`
 * (FLAIR_APP_MAGIC) is the fail-loud integrity tag (BC-3): a tenant that scribbled
 * its own refCon, or an unowned/foreign window, never mis-routes silently.
 *
 * A content click on an unowned window is a bug -> fail loud (Rule 2).
 * Ref: ADR-0013 Sec 3.1 (the binding rule / demux key, w->refCon =
 *      (int32_t)(uintptr_t)self), the Wave-2 orchestrator owner-recovery ruling.
 * -------------------------------------------------------------------------- */
static FlairApp *owner_of_window(FlairProcessList *list, WindowPtr w)
{
    if (w == NULL) PROC_PANIC("dispatch: NULL hit window for inContent");
    for (FlairApp *a = list->head; a != NULL; a = a->nextApp) {
        if (a->magic == FLAIR_APP_MAGIC &&
            (int32_t)(uintptr_t)a == w->refCon)
            return a;
    }
    PROC_PANIC("dispatch: inContent on a window owned by no resident app");
    return NULL;   /* unreachable (PROC_PANIC does not return) */
}

/* deliver one cooked EventRecord to a tenant's REQUIRED `event` entry-point. */
static void deliver(FlairApp *app, const EventRecord *ev)
{
    if (app == NULL || app->procs == NULL || app->procs->event == NULL)
        PROC_PANIC("dispatch: tenant has no event entry-point (REQUIRED, Sec 3.1)");
    app->procs->event(app, ev);
}

/* Synthesize one activateEvt cooked record (ADR-0013 Sec 3.3): what=activateEvt,
 * the active-flag set/cleared per `active`, where/when copied from the driving
 * event for determinism (Rule 11). The Window Manager's affected window goes in
 * `message` (the WindowPtr, MTE Ch 2). */
static EventRecord mk_activate(const EventRecord *src, WindowPtr w, int active)
{
    EventRecord ev = *src;
    ev.what    = (uint16_t)activateEvt;
    ev.message = (uint32_t)(uintptr_t)w;
    if (active)
        ev.modifiers = (uint16_t)(ev.modifiers | FLAIR_EVT_MOD_ACTIVE_FLAG);
    else
        ev.modifiers = (uint16_t)(ev.modifiers & (uint16_t)~FLAIR_EVT_MOD_ACTIVE_FLAG);
    return ev;
}

/* Promote `app` to the head (foreground) of the process list, demoting the
 * previous head. Relinks the nextApp chain only (the WindowMgr z-order is raised
 * separately via SelectWindow). Sets the lifecycle states (Sec 3.5). */
static void promote_to_front(FlairProcessList *list, FlairApp *app,
                             FlairApp *old_fg)
{
    /* unlink `app` from its current position. */
    FlairApp **pp = &list->head;
    while (*pp != NULL && *pp != app)
        pp = &(*pp)->nextApp;
    if (*pp == app)
        *pp = app->nextApp;
    /* push at the head as the new foreground. */
    app->nextApp = list->head;
    list->head   = app;
    app->state   = (uint8_t)FLAIR_APP_FG;
    if (old_fg != NULL)
        old_fg->state = (uint8_t)FLAIR_APP_BG;
}

/* --------------------------------------------------------------------------
 * flair_app_dispatch -- the single Layer-5 dispatcher (ADR-0013 Sec 3.3, BC-2).
 *
 * Demuxes ONE cooked EventRecord to the owning tenant and performs activation,
 * called IDENTICALLY by the live kmain pump and the host O-1 oracle (single
 * spine, E-D2). The O-1 oracle (harness/proptest/test_process.c) is the truth
 * (Law 2).
 *
 *   mouseDown: FindWindow -> part-code + WindowPtr.
 *     - inContent on a window whose owner is NOT the foreground -> CLICK-TO-
 *       ACTIVATE FIRST, in this EXACT order (the O-1 leg(c) ordering assertion):
 *         (1) SelectWindow raises the owner's window group to the z-order front;
 *         (2) a DEACTIVATE activateEvt (active-flag CLEARED) to the OLD foreground;
 *         (3) an ACTIVATE activateEvt (active-flag SET) to the new owner;
 *         (4) promote the owner to the process-list head (old fg -> background);
 *       THEN deliver the original mouseDown to the owner. The deactivate/activate
 *       pair is EXACTLY two records, deactivate THEN activate (Rule 11).
 *     - inContent on the already-foreground owner -> just deliver the mouseDown
 *       (no spurious activate pair; the other app stays silent -- leg(a)).
 *     - inDrag / inGoAway / inGrow are SHELL-OWNED chrome verbs (the title bar
 *       belongs to the Window Manager, the DefWindowProc analogue), and inDesk /
 *       the menu-bar band are shell/Menu-Manager surfaces: this dispatcher does
 *       NOTHING for them here. kmain (Wave 4) drives DragWindow/DisposeWindow/
 *       grow + MenuSelect. The O-1 oracle only drives inContent + keyDown.
 *   keyDown/autoKey: delivered to the FOREGROUND app (list->head) ONLY -- in
 *     System 7 the active (front) window IS keyboard focus, never the window
 *     under the cursor (leg(b)).
 *   other what-codes (updateEvt/activateEvt/nullEvent/...): no-op here; Wave 3/4
 *     add updateEvt routing to each damaged window's owning app.
 * -------------------------------------------------------------------------- */
void flair_app_dispatch(FlairProcessList *list, WindowMgr *wm,
                        const EventRecord *ev)
{
    if (list == NULL || wm == NULL || ev == NULL)
        PROC_PANIC("dispatch: NULL argument");

    switch (ev->what) {
    case mouseDown: {
        WindowPtr w = NULL;
        flair_part_code_t part = FindWindow(wm, ev->where, &w);

        if (part != inContent || w == NULL) {
            /* inDrag/inGoAway/inGrow are shell-owned chrome verbs; inDesk and the
             * menu-bar band are shell/Menu surfaces. No app routing here. */
            return;
        }

        FlairApp *owner  = owner_of_window(list, w);
        FlairApp *old_fg = list->head;

        if (owner != old_fg) {
            /* CLICK-TO-ACTIVATE (Sec 3.3 / Sec 3.5), in deterministic order. */
            SelectWindow(wm, owner->windows);          /* (1) raise z-order      */
            EventRecord deact = mk_activate(ev, old_fg ? old_fg->windows : NULL, 0);
            deliver(old_fg, &deact);                   /* (2) deactivate old fg  */
            EventRecord act = mk_activate(ev, owner->windows, 1);
            deliver(owner, &act);                      /* (3) activate new fg    */
            promote_to_front(list, owner, old_fg);     /* (4) relink process list*/
        }
        /* THEN the original mouseDown to the (now-foreground) owner. */
        deliver(owner, ev);
        return;
    }

    case keyDown:
    case autoKey:
        /* Keyboard focus follows the FOREGROUND app, not the cursor (leg(b)). */
        if (list->head != NULL)
            deliver(list->head, ev);
        return;

    default:
        /* updateEvt/activateEvt/nullEvent/... -- no-op here (Wave 3/4). */
        return;
    }
}

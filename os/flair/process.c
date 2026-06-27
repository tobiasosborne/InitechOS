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
 * raise_group -- APP-GROUP RAISE on app switch (ADR-0013 Sec 3.5).
 *
 * Raise `app`'s ENTIRE window group to the FRONT contiguous run of the WindowMgr
 * z-order, PRESERVING the group's internal relative z-order, leaving the group's
 * front window (app->windows) frontmost + hilited. "Switching ... raises that
 * app's WHOLE window group to the front run" (ADR-0013 Sec 3.5: app-grouped
 * z-order); the foreground app's windows form the front contiguous run.
 *
 * METHOD (allocation-free, cooperative). Enumerate the app's windows in the
 * z-order by the SAME demux key the binding rule sets --
 * (int32_t)(uintptr_t)app == w->refCon (Sec 3.1; the rule owner_of_window and
 * terminate's DisposeWindow loop already use). Then pull the BACKMOST owned
 * window to the head via SelectWindow exactly `count` times: each pull takes the
 * current-deepest un-pulled member and pushes it ahead of the previously-pulled
 * ones, so the group ends clustered at the front in its ORIGINAL relative order
 * (pull back-to-front => reassemble front-to-front). No temp array, no malloc
 * (Rule 2 / Law 3); a bounded back-to-front scan per member, and the group is
 * small.
 *
 * SINGLE-WINDOW INVARIANCE (keeps O-1 / launch / single-window switches
 * byte-identical): count==1 runs exactly one SelectWindow of the sole owned
 * window, which for a single-window app IS owner->windows -- identical to the
 * pre-group `SelectWindow(wm, owner->windows)`. SelectWindow itself reaffirms the
 * hilite (window.c reaffirm_active: frontmost visible window hilited, rest 0).
 *
 * Ref: ADR-0013 Sec 3.5 (app-grouped z-order, whole-group raise on switch),
 *      Sec 3.1 (the refCon demux key); window.h SelectWindow (raise one window
 *      to front + reaffirm activation). CLAUDE.md Law 2, Rule 2, Rule 11.
 * -------------------------------------------------------------------------- */
static void raise_group(WindowMgr *wm, FlairApp *app)
{
    if (wm == NULL || app == NULL) return;
    int32_t key = (int32_t)(uintptr_t)app;   /* the demux key (Sec 3.1)         */

    /* count the app's windows currently in the z-order. */
    int count = 0;
    for (WindowPtr w = wm->front; w != (WindowPtr)0; w = w->nextWindow)
        if (w->refCon == key) count++;

    /* pull the BACKMOST owned window to the front `count` times: each pull lands
     * the current-deepest un-pulled member just ahead of the prior pulls, so the
     * group reassembles at the front in its original relative order. */
    for (int i = 0; i < count; i++) {
        WindowPtr back = (WindowPtr)0;
        for (WindowPtr w = wm->front; w != (WindowPtr)0; w = w->nextWindow)
            if (w->refCon == key) back = w;   /* last match in z-order == backmost */
        if (back == (WindowPtr)0) break;      /* defensive; `count` already guards */
        SelectWindow(wm, back);
    }
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
                             const bitmap_t *surface,
                             flair_heap_t *master, const FlairAppProcs *procs,
                             const char *name, rgn_rect_t bounds,
                             uint32_t records_budget, uint32_t budget)
{
    if (list == NULL || wm == NULL || master == NULL || procs == NULL)
        PROC_PANIC("launch: NULL argument (contract error)");
    /* `surface` MAY be NULL: the stub host oracles launch non-drawing tenants. */

    /* SPLIT-ARENA carve (ADR-0013 Amendment AC-2, bead initech-ubd0): THREE blocks
     * IN ORDER -- (a) the FlairApp handle, (b) the RECORDS block (HANDLE; holds the
     * WindowRecord(s) + region pools), (c) the DATA block (GENERAL; per-instance
     * state). Keeping the WindowRecord + regions OFF the DATA arena is what lets app
     * DEATH survive a scribbled DATA arena (BC-6). */
    /* (a) the FlairApp handle from the master heap (HANDLE class). */
    FlairApp *app = (FlairApp *)flair_alloc(master, FLAIR_CLASS_HANDLE,
                                            (uint32_t)sizeof(FlairApp));
    /* (b) the RECORDS block (HANDLE class) -- WindowRecord(s) + region pools (AC-2). */
    void *records_block = (records_budget > 0u)
                ? flair_alloc(master, FLAIR_CLASS_HANDLE, records_budget)
                : (void *)0;
    /* (c) the DATA block of the app's budget (GENERAL class) -- per-instance state. */
    void *block = (budget > 0u)
                ? flair_alloc(master, FLAIR_CLASS_GENERAL, budget)
                : (void *)0;

    /* (O-4 / BC-5) Fail-loud budget: if ANY carve failed, reclaim whatever we got
     * IN REVERSE carve order, install NOTHING, leave `list`/`wm` unchanged, and
     * return NULL. Do NOT overcommit, do NOT partially install (Rule 2). */
    if (app == NULL || records_block == NULL || block == NULL) {
#ifdef BUDGET_MUT_OVERCOMMIT
        /* MUTANT BUDGET_MUT_OVERCOMMIT (Rule 6; budget-mutant build only): instead
         * of fail-loud reclaim, OVERCOMMIT -- partially install the (incompletely
         * carved) app anyway. O-4 expects NULL + no install, so this goes RED.
         * NEVER define in a real build. */
        if (app != NULL) {
            app->block         = block;          /* may be NULL */
            app->records_block = records_block;  /* may be NULL */
            app->magic         = FLAIR_APP_MAGIC;
            app->nextApp       = list->head;
            list->head         = app;            /* partial install */
        }
        return app;                        /* non-NULL on the handle-ok path */
#else
        /* REVERSE of the (a)->(b)->(c) carve order: data, then records, then handle.
         * This LIFO discipline keeps the small FlairApp handle the HANDLE free-list
         * head (records_budget > sizeof(FlairApp)) -- see FlairProcess_terminate. */
        if (block         != NULL) flair_free(master, FLAIR_CLASS_GENERAL, block);
        if (records_block != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  records_block);
        if (app           != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  app);
        return NULL;
#endif
    }

    /* (c') zero + init the handle; lay the TWO child heaps over their blocks. */
    proc_zero(app, (uint32_t)sizeof(FlairApp));
    flair_heap_init(&app->records_arena, records_block, records_budget); /* AC-2 */
    flair_heap_init(&app->arena, block, budget);
    app->magic         = FLAIR_APP_MAGIC;  /* BC-3 refCon-tag sentinel            */
    app->name          = name;
    app->procs         = procs;
    app->block         = block;
    app->records_block = records_block;    /* AC-2 RECORDS block (freed at teardown)*/
    app->menubar       = (struct MenuBar *)0;
    app->windows       = (WindowPtr)0;
    app->refCon        = 0;
    app->state         = (uint8_t)FLAIR_APP_FG;  /* provisional; affirmed on link (f)*/
    app->nextApp       = (FlairApp *)0;

    /* (d) build the app's world from its child arena. open() does NewWindow into
     * app->arena and sets each w->refCon = (int32_t)(uintptr_t)app (Sec 3.1). */
    {
        FlairLaunchParams lp;
        lp.bounds   = bounds;
        lp.budget   = budget;
        lp.wm       = wm;   /* thread the shell WindowMgr so open() can NewWindow(lp->wm,
                             * ...) instead of reaching a file-global (bead initech-fka6). */
        lp.surface  = surface;   /* the offscreen open() draws content into (fka6). */
        if (procs->open != NULL && procs->open(app, &lp) != 0) {
#ifdef BUDGET_MUT_OVERCOMMIT
            /* MUTANT (Rule 6): open() failed but install anyway (partial install).
             * O-4 expects NULL + reclaim, so this goes RED. NEVER in a real build. */
            app->nextApp = list->head;
            list->head   = app;
            return app;
#else
            /* (O-4 open-fail) reclaim data + records + handle (REVERSE carve order,
             * LIFO), install nothing, return NULL. The contract (Sec 3.1) is
             * open()!=0 == launch fail. CLEAN failures (the O-4 stub) thread no
             * window before failing. */
            flair_free(master, FLAIR_CLASS_GENERAL, app->block);
            flair_free(master, FLAIR_CLASS_HANDLE,  app->records_block);
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
 * teardown_common -- the SHARED teardown body for FlairProcess_terminate (clean)
 * and FlairProcess_kill (death). Performs {DisposeWindow-loop + unlink +
 * promote-next + the three one-shot frees}. It does NOT call procs->close(): the
 * caller decides (terminate calls close() first; kill -- a crashed app -- never
 * runs the dead app's code). ADR-0013 Sec 3.4 + Amendment AC-2 (bead initech-ubd0).
 *
 * SPLIT-ARENA (AC-2): the WindowRecord(s) + region pools live in app->records_arena
 * (a SEPARATE HANDLE block), so the DisposeWindow loop reads ONLY records-resident
 * structural fields (refCon/nextWindow/strucRgn) -- intact even when the DATA arena
 * (app->block) was scribbled by a crash. The DATA block is freed WITHOUT reading its
 * payload (flair_free touches only the in-band master-heap header, which precedes
 * the payload and the scribble), so death survives a corrupted DATA arena (BC-6).
 *
 * FREE ORDER (LIFO -- load-bearing for the O-3 avail-stable invariant): the DATA
 * block (GENERAL) first, then the RECORDS block (HANDLE), then the FlairApp handle
 * (HANDLE) LAST. The two HANDLE frees push records_block then the small handle, so
 * the HANDLE free-list head ends up the SMALL FlairApp handle. Because
 * records_budget > sizeof(FlairApp), cycle-2's first (small) HANDLE alloc then
 * reuses the small handle and the (large) records alloc reuses records_block -- no
 * fresh bump, avail stable. The REVERSE order would hand the large records block to
 * the small request and force a fresh bump for the large request every cycle ->
 * avail drift (the very bug O-3 + TEARDOWN_MUT_LEAK_RECORDS guard).
 * -------------------------------------------------------------------------- */
static void teardown_common(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app)
{
    /* (2) DisposeWindow every window in the wm z-order OWNED by app -- matched by
     * refCon == (int32_t)(uintptr_t)app (the SAME demux key open() set, Sec 3.1).
     * DisposeWindow mutates the z-order, so re-scan from wm->front after each
     * removal. WindowRecords are records_arena-resident (AC-2): a scribbled DATA
     * arena does NOT corrupt them. */
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

    /* (5) if app was the foreground head, promote the next app: raise its WHOLE
     * window group to the front run (Sec 3.5; for a single-window successor this
     * is one SelectWindow of its front window, identical to before). The
     * deactivate/activate pair is the dispatcher's job on the live click path; on
     * teardown we re-front + re-state the successor. */
    if (was_fg && succ != (FlairApp *)0) {
        raise_group(wm, succ);
        succ->state = (uint8_t)FLAIR_APP_FG;
    }

    /* (3) ONE-SHOT frees, LIFO: the DATA block (GENERAL) first ... */
#ifndef TEARDOWN_MUT_LEAK_BLOCK
    flair_free(master, FLAIR_CLASS_GENERAL, app->block);
#else
    /* MUTANT TEARDOWN_MUT_LEAK_BLOCK (Rule 6; teardown-mutant build only): SKIP the
     * DATA-block free -> the GENERAL block leaks -> flair_heap_avail drifts
     * monotonically DOWN per launch/terminate cycle (the bump cursor is never
     * rolled back), so O-3's "avail stable across cycles" check goes RED. NEVER
     * define in a real build. */
#endif
    /* ... then the RECORDS block (HANDLE) ... */
#ifndef TEARDOWN_MUT_LEAK_RECORDS
    flair_free(master, FLAIR_CLASS_HANDLE, app->records_block);
#else
    /* MUTANT TEARDOWN_MUT_LEAK_RECORDS (Rule 6; AC-2; records-mutant build only):
     * SKIP the RECORDS-block free -> the HANDLE records block leaks -> every cycle
     * BUMPS a fresh records block (sizeof FlairApp != records_budget, so the small
     * handle on the free-list never satisfies the large records request) -> avail
     * drifts DOWN -> O-3's "avail stable across cycles" check goes RED. NEVER define
     * in a real build. */
#endif
    /* ... then the FlairApp handle (HANDLE) LAST. Do not touch `app` after this. */
    flair_free(master, FLAIR_CLASS_HANDLE, app);
}

/* --------------------------------------------------------------------------
 * FlairProcess_terminate -- CLEAN teardown (ADR-0013 Sec 3.4 + Amendment AC-2).
 *
 * For a tenant brought up by FlairProcess_launch (app->block + app->records_block
 * carved from `master`). Runs procs->close() (flush docs) THEN the shared teardown
 * helper. The split-arena one-shot frees are the clean-death property (Sec 3.6).
 * -------------------------------------------------------------------------- */
void FlairProcess_terminate(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app)
{
    if (list == NULL || wm == NULL || master == NULL || app == NULL) return;

    /* BC-3: a teardown of a non-tenant (scribbled/foreign handle) fails loud
     * rather than freeing arbitrary memory (Rule 2). The handle lives in its OWN
     * HANDLE block, NOT the DATA arena, so a scribbled DATA arena leaves it intact. */
    if (app->magic != FLAIR_APP_MAGIC)
        PROC_PANIC("terminate: app->magic mismatch (BC-3)");

    app->state = (uint8_t)FLAIR_APP_DYING;

    /* (1) optional close hook (flush docs); the shell tears down regardless. */
    if (app->procs != NULL && app->procs->close != NULL)
        app->procs->close(app);

    teardown_common(list, wm, master, app);
}

/* --------------------------------------------------------------------------
 * FlairProcess_kill -- the DEATH path (ADR-0013 Sec 3.4 application-death; AC-2 /
 * BC-6). FlairProcess_terminate MINUS procs->close(): a crashed app's code is
 * NEVER run again. The shared helper removes the windows from the z-order reading
 * only records_arena-resident fields, promotes the successor, and one-shot-frees
 * the data + records + handle WITHOUT trusting the (possibly scribbled) DATA
 * payload. This is the path the death-survival oracle scribbles app->block before
 * calling -- it must still empty the z-order with no panic (BC-6).
 * -------------------------------------------------------------------------- */
void FlairProcess_kill(FlairProcessList *list, WindowMgr *wm,
                       flair_heap_t *master, FlairApp *app)
{
    if (list == NULL || wm == NULL || master == NULL || app == NULL) return;

    /* BC-3: the magic tag lives in the handle (its OWN HANDLE block), intact even
     * when the DATA arena was scribbled by the crash -- so the integrity check is
     * meaningful here, not itself a read of the dead heap (Rule 2). */
    if (app->magic != FLAIR_APP_MAGIC)
        PROC_PANIC("kill: app->magic mismatch (BC-3)");

    app->state = (uint8_t)FLAIR_APP_DYING;

    /* NO procs->close(): never run a dead app's code (the death-vs-clean-quit
     * distinction; ADR-0013 Sec 3.4). */
    teardown_common(list, wm, master, app);
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
#ifdef FLAIR_LIVE_MUTATE_IGNORE_REFCON
    /* MUTANT FLAIR_LIVE_MUTATE_IGNORE_REFCON (Rule 6; the O-5 tenants emu-mutant
     * image ONLY): IGNORE the refCon binding rule and always return the FOREGROUND
     * app, never the clicked background tenant. A click on HELLO's visible sliver
     * then "belongs to" the foreground NOTES, so flair_app_dispatch sees
     * owner == old_fg, takes NO switch (no raise / no activate / no promote), and
     * HELLO is never raised or repainted -> the booted O-5 gate's TIER-A overlap
     * probe stays NOTES_FILL (RED). NEVER define in a real build. */
    (void)w;
    return list->head;
#else
    for (FlairApp *a = list->head; a != NULL; a = a->nextApp) {
        if (a->magic == FLAIR_APP_MAGIC &&
            (int32_t)(uintptr_t)a == w->refCon)
            return a;
    }
    PROC_PANIC("dispatch: inContent on a window owned by no resident app");
    return NULL;   /* unreachable (PROC_PANIC does not return) */
#endif
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
 *         (1) raise_group raises the owner's WHOLE window group to the z-order
 *             front contiguous run, preserving internal order (Sec 3.5);
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
            raise_group(wm, owner);                    /* (1) raise WHOLE group  */
#ifndef FLAIR_LIVE_MUTATE_SKIP_ACTIVATE
            EventRecord deact = mk_activate(ev, old_fg ? old_fg->windows : NULL, 0);
            deliver(old_fg, &deact);                   /* (2) deactivate old fg  */
            EventRecord act = mk_activate(ev, owner->windows, 1);
            deliver(owner, &act);                      /* (3) activate new fg    */
#else
            /* MUTANT FLAIR_LIVE_MUTATE_SKIP_ACTIVATE (Rule 6; the O-5 tenants
             * emu-mutant image ONLY): SKIP the deactivate/activate pair. The group
             * still raises (1) and is still promoted (4), so the booted O-5 gate's
             * TIER-A overlap repaint + MENU-BAND swap stay GREEN -- but the raised
             * tenant never receives activateEvt active=1, so it never paints its
             * content accent -> the gate's TIER-B accent probe stays FILL (RED).
             * Isolates the activation leg. NEVER define in a real build. */
            (void)mk_activate;   /* keep referenced (else -Werror=unused-function) */
#endif
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
        /* updateEvt/activateEvt/nullEvent/... -- no-op here. updateEvt routing is
         * the SEPARATE spine helper flair_route_updates (below), driven off the
         * window-damage state rather than a single cooked EventRecord. */
        return;
    }
}

/* --------------------------------------------------------------------------
 * owner_of_window_tolerant -- the refCon demux WITHOUT the fail-loud panic.
 *
 * The SAME match rule owner_of_window uses (magic-tag + truncated-identity, Sec
 * 3.1, width-portable) but returns NULL instead of panicking when no resident app
 * owns `w`. flair_route_updates uses this because the shell's own frame/desktop
 * furniture windows are UNOWNED and legitimately carry damage; routing must tolerate
 * them. A content click (owner_of_window) instead panics on an unowned hit, because
 * a click delivered to no app is a routing bug -- but damage on an ownerless window
 * is not. Ref: ADR-0013 Sec 3.1 (binding rule), Sec 3.3 (updateEvt routing).
 * -------------------------------------------------------------------------- */
static FlairApp *owner_of_window_tolerant(FlairProcessList *list, WindowPtr w)
{
    if (w == NULL) return NULL;
    for (FlairApp *a = list->head; a != NULL; a = a->nextApp) {
        if (a->magic == FLAIR_APP_MAGIC &&
            (int32_t)(uintptr_t)a == w->refCon)
            return a;
    }
    return NULL;   /* shell furniture / foreign window: no resident owner */
}

/* --------------------------------------------------------------------------
 * flair_route_updates -- route pending window damage to owning tenants (Sec 3.3).
 *
 * One forward pass over the z-order. For each VISIBLE window with a NON-EMPTY
 * updateRgn: synthesize one updateEvt, recover the owner (tolerant), deliver, then
 * validate (clear the damage). Unowned damaged windows are skipped (no delivery,
 * no validate -- the shell repaints its own furniture). WindowMgr_validate only
 * sets updateRgn empty (it does NOT mutate the z-order list), so a single forward
 * nextWindow walk is safe -- unlike terminate's DisposeWindow loop, which re-scans.
 *
 * Ref: ADR-0013 Sec 3.3 (updateEvt -> each damaged window's owning app, background
 *      apps included), Sec 3.1 (refCon demux); window.h (WindowMgr_validate,
 *      the updateRgn damage model); spec/event_model.h (updateEvt, the WindowPtr
 *      in `message`, MTE Ch 2). CLAUDE.md Law 2, Rule 2, Rule 11.
 * -------------------------------------------------------------------------- */
void flair_route_updates(FlairProcessList *list, WindowMgr *wm)
{
    if (list == NULL || wm == NULL)
        PROC_PANIC("route_updates: NULL argument");

    for (WindowPtr w = wm->front; w != (WindowPtr)0; w = w->nextWindow) {
        if (!w->visible) continue;                 /* hidden windows owe no update  */
        if (w->updateRgn == (region_t *)0) continue;
        if (region_is_empty(w->updateRgn)) continue;   /* no damage outstanding     */

        /* Recover the owning tenant by the binding rule -- TOLERANT: an unowned
         * (shell furniture) window is skipped here, NOT panicked (the distinction
         * from owner_of_window's content-click fail-loud). */
        FlairApp *owner = owner_of_window_tolerant(list, w);
        if (owner == (FlairApp *)0) continue;      /* shell furniture -> shell paints*/

        /* Synthesize ONE updateEvt for this window and deliver to its owner. The
         * window identity goes in message ((uint32_t)(uintptr_t)w, MTE Ch 2 -- exact
         * on flat-32, truncated-but-stable on host-64); where/when best-effort 0
         * (deterministic, Rule 11). */
        if (owner->procs != (const FlairAppProcs *)0 &&
            owner->procs->event != (void (*)(FlairApp *, const EventRecord *))0) {
            EventRecord ev;
            proc_zero(&ev, (uint32_t)sizeof ev);
            ev.what    = (uint16_t)updateEvt;
            ev.message = (uint32_t)(uintptr_t)w;
            owner->procs->event(owner, &ev);
        }

        /* Clear the damage (the pump's EndUpdate). Only for OWNED windows -- the
         * unowned skip above never reaches here, so shell furniture keeps its
         * updateRgn for the shell to service. */
        WindowMgr_validate(w);
    }
}

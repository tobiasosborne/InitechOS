/*
 * os/flair/process.c -- the FLAIR App Contract: tenant lifecycle + the Layer-5
 * dispatcher (THE ARTIFACT). WAVE-1 (ORACLE-FIRST) increment.
 *
 * Tenant lifecycle and cooperative dispatch share one process list.
 * ERA AXIS: Mac OS 8 Platinum is the FLAIR BASE; cited System 7
 * activation/focus behavior is retained Toolbox heritage, not a base claim.
 *
 * STATUS (independent checks, oracle-first):
 *   - FlairProcess_register is the thin caller-storage registrar (no arena, no
 *     open) the O-1 host oracle uses to wire the two stub tenants over
 *     caller-built windows (stamp magic, bind the demux refCon, link/demote the
 *     foreground).
 *   - FlairProcess_launch / _terminate / _kill implement the AC-2 split-arena
 *     memory model: HANDLE app + HANDLE records + GENERAL data, strict LIFO
 *     teardown, clean-close vs crash distinction, and successor promotion.
 *     Graded by O-3 (teardown/leak/death survival) + O-4 (launch budget).
 *   - flair_app_dispatch (Wave 2) routes + synthesizes the activateEvt pair;
 *     graded GREEN by O-1 (harness/proptest/test_process.c).
 *
 * Freestanding: <stdint.h> via process.h; no libc, no malloc. Compiles
 * both under the kernel flags and hosted for the property suite.
 *
 * Ref: tenant lifecycle (single spine), Sec 3.1 (binding rule + magic tag, BC-3),
 *      Sec 3.2 (launch), Sec 3.3 (event routing & activation), Sec 3.4 (teardown),
 *      Sec 7 O-1 (the host oracle this stub must fail), Sec 8 (staging).
 *      independent checks, language boundary,  (Red->Green), fail loudly, ASCII source.
 *
 * ASCII-clean. No timestamps / no nondeterminism.
 */
#include "process.h"

/* ---------------------------------------------------------------------------
 * Fail-loud (dual fail-loud, mirroring os/flair/window.c WIN_PANIC: panic
 * in-kernel / abort hosted). Self-contained so process.c dual-compiles with
 * only the locked headers (explicit error handling -- a panic with context beats a silently
 * mis-routed event). Ref: window.c:54-60 (the same idiom).
 * ------------------------------------------------------------------------- */
#if defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1 && !defined(__KERNEL_FREESTANDING__)
#  include <stdlib.h>   /* abort -- hosted only */
#  define PROC_PANIC(msg)  abort()
#else
/* Freestanding: a deliberate, loud, deterministic hang (explicit error handling / reproducibility). */
#  define PROC_PANIC(msg)  do { (void)(msg); __builtin_trap(); } while (0)
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
 * heap.c's no-libc discipline, language boundary). */
static void proc_zero(void *p, uint32_t n)
{
    uint8_t *b = (uint8_t *)p;
    for (uint32_t i = 0; i < n; i++) b[i] = 0u;
}

/* --------------------------------------------------------------------------
 * raise_group -- APP-GROUP RAISE on app switch (tenant lifecycle).
 *
 * Raise `app`'s ENTIRE window group to the FRONT contiguous run of the WindowMgr
 * z-order, PRESERVING the group's internal relative z-order, leaving the group's
 * front window (app->windows) frontmost + hilited. "Switching... raises that
 * app's WHOLE window group to the front run" (tenant lifecycle: app-grouped
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
 * (explicit error handling / language boundary); a bounded back-to-front scan per member, and the group is
 * small.
 *
 * SINGLE-WINDOW INVARIANCE (keeps O-1 / launch / single-window switches
 * byte-identical): count==1 runs exactly one SelectWindow of the sole owned
 * window, which for a single-window app IS owner->windows -- identical to the
 * pre-group `SelectWindow(wm, owner->windows)`. SelectWindow itself reaffirms the
 * hilite (window.c reaffirm_active: frontmost visible window hilited, rest 0).
 *
 * Ref: tenant lifecycle (app-grouped z-order, whole-group raise on switch),
 *      Sec 3.1 (the refCon demux key); window.h SelectWindow (raise one window
 *      to front + reaffirm activation). independent checks, explicit error handling, reproducibility.
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
 * arena and calls NO procs->open: the caller owns the FlairApp storage and has
 * already built the window. The arena-allocating production launch is
 * FlairProcess_launch (Sec 3.2). Ref: tenant lifecycle (binding rule + magic).
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
 * FlairProcess_launch -- the ARENA-ALLOCATING Toolbox launch (tenant lifecycle).
 *
 * Carve a tenant out of the master FLAIR heap and bring it up as foreground.
 * Ref: tenant lifecycle (steps a-g), Sec 3.6 (child sub-arena), Sec 7 O-4 /
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

    /* SPLIT-ARENA carve (tenant lifecycle Amendment AC-2, ): THREE blocks
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
     * return NULL. Do NOT overcommit, do NOT partially install. */
    if (app == NULL || records_block == NULL || block == NULL) {
        /* REVERSE of the (a)->(b)->(c) carve order: data, then records, then handle.
         * This LIFO discipline keeps the small FlairApp handle the HANDLE free-list
         * head (records_budget > sizeof(FlairApp)) -- see FlairProcess_terminate. */
        if (block         != NULL) flair_free(master, FLAIR_CLASS_GENERAL, block);
        if (records_block != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  records_block);
        if (app           != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  app);
        return NULL;
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

    /* (d) build the app's world from its child arena. open does NewWindow into
     * app->arena and sets each w->refCon = (int32_t)(uintptr_t)app (Sec 3.1). */
    {
        FlairLaunchParams lp;
        lp.bounds   = bounds;
        lp.budget   = budget;
        lp.wm       = wm;   /* thread the shell WindowMgr so open can NewWindow(lp->wm,
                             *...) instead of reaching a file-global. */
        lp.surface  = surface;   /* the offscreen open draws content into (fka6). */
        if (procs->open != NULL && procs->open(app, &lp) != 0) {
            /* (O-4 open-fail) reclaim data + records + handle (REVERSE carve order,
             * LIFO), install nothing, return NULL. The contract (Sec 3.1) is
             * open!=0 == launch fail. CLEAN failures (the O-4 stub) thread no
             * window before failing. */
            flair_free(master, FLAIR_CLASS_GENERAL, app->block);
            flair_free(master, FLAIR_CLASS_HANDLE,  app->records_block);
            flair_free(master, FLAIR_CLASS_HANDLE,  app);
            return NULL;
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
 * FlairProcess_admit -- WINDOWLESS registration of a tenant whose code is
 * already running (the disk-launched tenant,; tenant lifecycle
 * Amendment AC-3 draft disk tenant lifecycle / disk tenant lifecycle in docs/design/GUI-remediation-
 * tbxgate.h).
 *
 * The carve is FlairProcess_launch's steps (a)-(c) verbatim -- handle, RECORDS
 * (HANDLE class), DATA (GENERAL class), all-or-nothing with the reverse-order
 * reclaim (BC-5) -- and the stamp is the same. What differs is exactly what
 * disk tenant lifecycle rules:
 *   - NO open: the tenant's entry already ran (procs->open may be NULL);
 *   - NO SelectWindow and NO foreground affirmation: the tenant owns no window
 *     yet, so it is linked directly BEHIND the current head as FLAIR_APP_BG
 *     (the head, and every band-2 gate keyed on it, is undisturbed). An empty
 *     list is the one exception: the sole app is the head (FG) by definition.
 * Foreground affirmation happens later, through FlairProcess_activate -- THE
 * one switch sequence -- when the tenant's first window exists.
 * Returns the admitted app, or NULL (nothing installed) on a bad argument or a
 * failed carve. Teardown is FlairProcess_terminate / _kill, unchanged.
 * -------------------------------------------------------------------------- */
FlairApp *FlairProcess_admit(FlairProcessList *list, flair_heap_t *master,
                             const FlairAppProcs *procs, const char *name,
                             uint32_t records_budget, uint32_t budget)
{
    FlairApp *app;
    void *records_block;
    void *block;

    if (list == NULL || master == NULL || procs == NULL ||
        procs->event == NULL || records_budget == 0u || budget == 0u)
        return NULL;

    app           = (FlairApp *)flair_alloc(master, FLAIR_CLASS_HANDLE,
                                            (uint32_t)sizeof(FlairApp));
    records_block = flair_alloc(master, FLAIR_CLASS_HANDLE, records_budget);
    block         = flair_alloc(master, FLAIR_CLASS_GENERAL, budget);
    if (app == NULL || records_block == NULL || block == NULL) {
        if (block         != NULL) flair_free(master, FLAIR_CLASS_GENERAL, block);
        if (records_block != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  records_block);
        if (app           != NULL) flair_free(master, FLAIR_CLASS_HANDLE,  app);
        return NULL;
    }

    proc_zero(app, (uint32_t)sizeof(FlairApp));
    flair_heap_init(&app->records_arena, records_block, records_budget);
    flair_heap_init(&app->arena, block, budget);
    app->magic         = FLAIR_APP_MAGIC;
    app->name          = name;
    app->procs         = procs;
    app->block         = block;
    app->records_block = records_block;

    if (list->head == NULL) {
        app->state   = (uint8_t)FLAIR_APP_FG;
        app->nextApp = NULL;
        list->head   = app;
    } else {
        app->state            = (uint8_t)FLAIR_APP_BG;
        app->nextApp          = list->head->nextApp;
        list->head->nextApp   = app;
    }
    return app;
}

/* --------------------------------------------------------------------------
 * teardown_common -- the SHARED teardown body for FlairProcess_terminate (clean)
 * and FlairProcess_kill (death). Performs {DisposeWindow-loop + unlink +
 * promote-next + the three one-shot frees}. It does NOT call procs->close: the
 * caller decides (terminate calls close first; kill -- a crashed app -- never
 * runs the dead app's code). tenant lifecycle + Amendment AC-2.
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
 * avail drift (the very bug O-3 + leak records patch guard).
 * -------------------------------------------------------------------------- */
static void teardown_common(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app)
{
    /* (2) DisposeWindow every window in the wm z-order OWNED by app -- matched by
     * refCon == (int32_t)(uintptr_t)app (the SAME demux key open set, Sec 3.1).
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
     * (no use-after-free; explicit error handling/3). */
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

        /* tenant lifecycle step 5: promotion includes the successor's
         * activateEvt, not only list/z-order state. The dying tenant is never
         * re-entered; synthesize the one surviving half of the activation pair
         * with deterministic zero where/when. This restores tenant-side focus
         * state (caret/accent) before the close dispatch repaints/presents. */
        if (succ->procs != (const FlairAppProcs *)0 &&
            succ->procs->event !=
                (void (*)(FlairApp *, const EventRecord *))0) {
            EventRecord act;
            proc_zero(&act, (uint32_t)sizeof act);
            act.what = (uint16_t)activateEvt;
            act.message = (uint32_t)(uintptr_t)succ->windows;
            act.modifiers = FLAIR_EVT_MOD_ACTIVE_FLAG;
            succ->procs->event(succ, &act);
        }
    }
    /* Active chrome agrees with the foreground after the teardown:
     * DisposeWindow re-hilited the frontmost survivor whoever owns it; when the
     * head owns no window, that survivor is a background app's and is drawn
     * inactive. A no-op when raise_group(succ) put succ's window in front. */
    (void)FlairProcess_sync_active(list, wm);

    /* (3) ONE-SHOT frees, LIFO: the DATA block (GENERAL) first... */
    flair_free(master, FLAIR_CLASS_GENERAL, app->block);
    /*... then the RECORDS block (HANDLE)... */
    flair_free(master, FLAIR_CLASS_HANDLE, app->records_block);
    /*... then the FlairApp handle (HANDLE) LAST. Do not touch `app` after this. */
    app->magic = 0; /* freed handles must fail BC-3 on repeat teardown. */
    flair_free(master, FLAIR_CLASS_HANDLE, app);
}

/* --------------------------------------------------------------------------
 * FlairProcess_terminate -- CLEAN teardown (tenant lifecycle + Amendment AC-2).
 *
 * For a tenant brought up by FlairProcess_launch (app->block + app->records_block
 * carved from `master`). Runs procs->close (flush docs) THEN the shared teardown
 * helper. The split-arena one-shot frees are the clean-death property (Sec 3.6).
 * -------------------------------------------------------------------------- */
/* One ownership check serves clean quit AND death, before any callback/free.
 * Ref: tenant lifecycle BC-3/AC-2; k0n8 and . Keep both entries identical here. */
static int teardown_begin(FlairProcessList *list, WindowMgr *wm,
                          flair_heap_t *master, FlairApp *app)
{
    if (list == NULL || wm == NULL || master == NULL || app == NULL) return 0;

    /* BC-3: a teardown of a non-tenant (scribbled/foreign handle) fails loud
     * rather than freeing arbitrary memory. The handle lives in its OWN
     * HANDLE block, NOT the DATA arena, so a scribbled DATA arena leaves it intact. */
    if (app->magic != FLAIR_APP_MAGIC)
        PROC_PANIC("teardown: app->magic mismatch (BC-3)");
    if (app->block == NULL || app->records_block == NULL)
        PROC_PANIC("teardown: caller-owned registered app (k0n8)");

    app->state = (uint8_t)FLAIR_APP_DYING;
    return 1;
}

void FlairProcess_terminate(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app)
{
    if (!teardown_begin(list, wm, master, app)) return;

    /* (1) optional close hook (flush docs); the shell tears down regardless. */
    if (app->procs != NULL && app->procs->close != NULL)
        app->procs->close(app);

    teardown_common(list, wm, master, app);
}

/* --------------------------------------------------------------------------
 * FlairProcess_kill -- the DEATH path (tenant lifecycle application-death; AC-2 /
 * BC-6). FlairProcess_terminate MINUS procs->close: a crashed app's code is
 * NEVER run again. The shared helper removes the windows from the z-order reading
 * only records_arena-resident fields, promotes the successor, and one-shot-frees
 * the data + records + handle WITHOUT trusting the (possibly scribbled) DATA
 * payload. This is the path the death-survival oracle scribbles app->block before
 * calling -- it must still empty the z-order with no panic (BC-6).
 * -------------------------------------------------------------------------- */
void FlairProcess_kill(FlairProcessList *list, WindowMgr *wm,
                       flair_heap_t *master, FlairApp *app)
{
    if (!teardown_begin(list, wm, master, app)) return;

    /* NO procs->close: never run a dead app's code (the death-vs-clean-quit
     * distinction; tenant lifecycle). */
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
 * A content/title hit on an unowned window is a bug -> fail loud.
 * Ref: tenant lifecycle (the binding rule / demux key, w->refCon =
 *      (int32_t)(uintptr_t)self), the Wave-2 orchestrator owner-recovery ruling.
 * -------------------------------------------------------------------------- */
static FlairApp *owner_of_window(FlairProcessList *list, WindowPtr w)
{
    if (w == NULL) PROC_PANIC("dispatch: NULL hit tenant window");
    for (FlairApp *a = list->head; a != NULL; a = a->nextApp) {
        if (a->magic == FLAIR_APP_MAGIC &&
            (int32_t)(uintptr_t)a == w->refCon)
            return a;
    }
    PROC_PANIC("dispatch: hit window owned by no resident app");
    return NULL;   /* unreachable (PROC_PANIC does not return) */
}

/* deliver one cooked EventRecord to a tenant's REQUIRED `event` entry-point. */
static void deliver(FlairApp *app, const EventRecord *ev)
{
    if (app == NULL || app->procs == NULL || app->procs->event == NULL)
        PROC_PANIC("dispatch: tenant has no event entry-point (REQUIRED, Sec 3.1)");
    app->procs->event(app, ev);
}

/* Synthesize one activateEvt cooked record (tenant lifecycle): what=activateEvt,
 * the active-flag set/cleared per `active`, where/when copied from the driving
 * event for determinism. The Window Manager's affected window goes in
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

/* The ONE foreground-switch sequence for content and title activation
 * (tenant lifecycle/3.5; DQ5 ). Keeping all four ordered
 * steps here prevents the shell-owned inDrag arm from growing a second,
 * subtly-different activation path. The caller decides whether the driving
 * mouseDown is content (deliver after this returns) or title chrome (return
 * without delivery). */
static void switch_foreground(FlairProcessList *list, WindowMgr *wm,
                              const EventRecord *ev, FlairApp *owner,
                              FlairApp *old_fg)
{
    raise_group(wm, owner);                         /* (1) raise WHOLE group  */
    EventRecord deact = mk_activate(ev, old_fg ? old_fg->windows : NULL, 0);
    deliver(old_fg, &deact);                        /* (2) deactivate old fg  */
    EventRecord act = mk_activate(ev, owner->windows, 1);
    deliver(owner, &act);                           /* (3) activate new fg    */
    promote_to_front(list, owner, old_fg);          /* (4) relink process list*/
    /* (5) active chrome agrees with the NEW foreground: when `owner`
     * owns no window (the Finder brought forward by a desktop click), the old
     * foreground's front window stays in front but is drawn inactive. A no-op
     * whenever (1) raised a window of `owner`. */
    (void)FlairProcess_sync_active(list, wm);
}

/* --------------------------------------------------------------------------
 * FlairProcess_activate -- the foreground switch for a NON-mouseDown cause.
 *
 *. See process.h for the contract and the rationale: the
 * always-resident Finder promotes itself when it opens a disk window from a
 * DESKTOP double-click, which flair_app_dispatch never sees (inDesk returns
 * before the owner demux). One sequence, two entry points -- never two
 * sequences. Ref: tenant lifecycle/3.5; design F2-1 (the resident shell tenant).
 * -------------------------------------------------------------------------- */
int FlairProcess_activate(FlairProcessList *list, WindowMgr *wm,
                          const EventRecord *ev, FlairApp *app)
{
    FlairApp *old_fg;

    if (list == NULL || wm == NULL || ev == NULL || app == NULL) return 0;
    old_fg = list->head;
    if (app == old_fg) return 0;          /* already the foreground: no-op     */
    switch_foreground(list, wm, ev, app, old_fg);
    return 1;
}

static FlairApp *owner_of_window_tolerant(FlairProcessList *list, WindowPtr w);

/* --------------------------------------------------------------------------
 * FlairProcess_sync_active -- see process.h. Only the
 * foreground app's front window is drawn active; a background app's window that
 * reaffirm_active hilited (because it is frontmost after a close, or because the
 * new foreground app owns no window) is deactivated here. Invisible records are
 * skipped (the hidden canon frame windows stay in the z-order); unowned shell
 * furniture keeps the Window Manager's hilite (tolerant owner match -> NULL).
 * -------------------------------------------------------------------------- */
int FlairProcess_sync_active(FlairProcessList *list, WindowMgr *wm)
{
    WindowPtr w;
    FlairApp *owner;

    if (list == NULL || wm == NULL) return 0;
    for (w = wm->front; w != NULL; w = w->nextWindow)
        if (w->visible) break;
    if (w == NULL || !w->hilited) return 0;
    owner = owner_of_window_tolerant(list, w);
    if (owner == NULL || owner == list->head) return 0;
    w->hilited = 0;
    /* The 1->0 deactivation repaint, seeded exactly as window.c reaffirm_active
     * seeds it (the strucRgn bbox, SECTed to the visible region inside). */
    WindowMgr_invalidate(wm, w, region_get_bbox(w->strucRgn));
    return 1;
}

/* --------------------------------------------------------------------------
 * flair_app_dispatch -- the single Layer-5 dispatcher (tenant lifecycle, BC-2).
 *
 * Demuxes ONE cooked EventRecord to the owning tenant and performs activation,
 * called IDENTICALLY by the live kmain pump and the host O-1 oracle (single
 * spine, E-D2). The O-1 oracle (harness/proptest/test_process.c) is the truth
 *.
 *
 *   mouseDown: FindWindow -> part-code + WindowPtr.
 *     - inContent on a window whose owner is NOT the foreground -> CLICK-TO-
 *       ACTIVATE FIRST, in this EXACT order (the O-1 leg(c) ordering assertion):
 *         (1) raise_group raises the owner's WHOLE window group to the z-order
 *             front contiguous run, preserving internal order (Sec 3.5);
 *         (2) a DEACTIVATE activateEvt (active-flag CLEARED) to the OLD foreground;
 *         (3) an ACTIVATE activateEvt (active-flag SET) to the new owner;
 *         (4) promote the owner to the process-list head (old fg -> background);
 *       The activating content click is consumed. The deactivate/activate
 *       pair is EXACTLY two records, deactivate THEN activate.
 *     - inContent on the already-foreground owner -> just deliver the mouseDown
 *       (no spurious activate pair; the other app stays silent -- leg(a)).
 *     - inDrag remains a SHELL-OWNED physical chrome verb, but a non-foreground
 *       resident owner first takes the EXACT four-step switch above. The title
 *       mouseDown is NEVER delivered to the tenant (O-1 leg(e)); kmain observes
 *       the changed list head, completes repaint/menubar policy, then DragWindow.
 *       An already-foreground owner's inDrag is a pure no-op here (leg(d)).
 *     - inGoAway / inGrow / inZoomIn / inZoomOut / inCollapse remain
 *       shell-owned with no dispatcher action, and
 *       inDesk / the menu-bar band are shell/Menu-Manager surfaces.
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

        if ((part != inContent && part != inDrag) || w == NULL) {
            /* Window widgets are shell-owned chrome verbs; inDesk and the
             * menu-bar band are shell/Menu surfaces. No app routing here. */
            return;
        }

        FlairApp *owner  = owner_of_window(list, w);
        FlairApp *old_fg = list->head;
        int was_active = w == wm->front && owner == old_fg;

        if (owner != old_fg) {
            /* DQ5 mutant restores only the pre-fix background-title no-op. The
             * content-click switch remains fully live in the same build. */
            switch_foreground(list, wm, ev, owner, old_fg);
        }

        /* --- THE CLICKED WINDOW ITSELF COMES FORWARD (bug ) -----
         * Inside Macintosh, Macintosh Toolbox Essentials, Event Manager: a
         * mouseDown in ANY part of a window that is not the active (front) one
         * makes it active first -- SelectWindow(theWindow) -- and only then is
         * the click acted on. Title bar AND content, period behaviour.
         *
         * switch_foreground above covers only the CROSS-TENANT half, and even
         * there it raises the owner's GROUP (raise_group, Sec 3.5) preserving
         * the group's internal order, which lands the group's OLD front window
         * frontmost -- not necessarily the one that was clicked. Two gaps fell
         * out of that, and this is the ONE line that closes both:
         *   (1) SAME TENANT, non-front window: `owner == old_fg`, so nothing
         *       ran at all. Measured on the real guest and recorded in
         *       spec/flair_disk_windows_traces.mk lines 141-149: a title click
         *       on a background Finder disk window dispatched (FLAIR-DRAG
         *       fired) but the window never came forward, and the next
         *       double-click still went to the window on top. That trace file
         *       calls it "a REAL gap in R3.3's raise story... it belongs to
         *       the implementation / " -- this is that fix.
         *   (2) CROSS TENANT, non-group-front window: the group arrives at the
         *       front run but the clicked member is still buried inside it.
         *
         * WHY HERE AND NOT IN kmain: tenant lifecycle BC-2 -- flair_app_dispatch is the
         * SOLE Layer-5 routing spine and kmain re-implements no routing logic.
         * Which window is active after a click is routing/activation policy,
         * not a chrome verb; putting it in kmain's inDrag arm would give the
         * content arm a second, subtly-different copy (the very duplication
         * switch_foreground's banner exists to prevent) and would leave the
         * host O-1 oracle unable to see it at all.
         *
         * Already-front is a no-op by the guard, so every single-window tenant
         * scene -- every existing band-2 / appswitch / drag gate -- executes
         * byte-identically: after switch_foreground a single-window owner's
         * window IS wm->front.
         *
         * SelectWindow reaffirms the activation hilite and seeds the raised
         * window's repaint (window.c reaffirm_active 0->1, ),
         * so the newly-front window repaints through the ordinary damage cycle.
         * app->windows is the group head (process.h Sec "windows"), and the
         * clicked window is now that head, so it is re-pointed in the same
         * breath -- a stale head after a raise is the 8fhu class of bug. */
        if (w != wm->front) {
            SelectWindow(wm, w);
            owner->windows = w;
        }

        /* Title chrome is shell-owned: activation may have happened above, but
         * the mouseDown NEVER crosses into tenant content. kmain still performs
         * the physical DragWindow after its post-switch paint/present phase. */
        if (part == inDrag) return;

        /* A STANDARD SCROLL BAR is a control in the content (window.h Sec 4b,
         * ): FindWindow says inContent, so activation ran
         * above exactly as for any content click, but the click itself belongs
         * to the window's scroll-bar tracker (kmain), never to the owner's
         * content handler -- a tenant must not see a scroll-bar click as a
         * click in its document. */
        if (WindowScrollBand(w, ev->where) >= 0) return;

        /*: SelectWindow consumes a document's activating click.
         * Ref: MTE Event Manager; the following click reaches its content. */
        if (!was_active) return;

        /* The window was already active: deliver this content click. */
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
 * is not. Ref: tenant lifecycle (binding rule), Sec 3.3 (updateEvt routing).
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
 * FlairProcess_close_window -- owner-identity close disposition.
 *
 * tenant lifecycle is ratified: Quit/inGoAway on a tenant calls
 * FlairProcess_terminate, never a bare hide/unlink. disk tenant lifecycle and Finder F2-3
 * refine the already-required subset without adding the pending INT 0x81/disk
 * machinery: refCon demux says whether the hit WindowRecord belongs to a
 * resident tenant. Owned -> clean terminate; unowned -> shell-furniture hide.
 * -------------------------------------------------------------------------- */
int FlairProcess_close_window(FlairProcessList *list, WindowMgr *wm,
                              flair_heap_t *master, WindowPtr w,
                              const char **terminated_name)
{
    FlairApp *owner;

    if (list == NULL || wm == NULL || master == NULL || w == NULL)
        PROC_PANIC("close_window: NULL argument");
    if (terminated_name != NULL) *terminated_name = (const char *)0;

    owner = owner_of_window_tolerant(list, w);
    if (owner == (FlairApp *)0) {
        HideWindow(wm, w);                    /* shell furniture only */
        return 0;
    }

    /* Capture the static/tenant-owned name pointer before terminate frees the
     * FlairApp handle. The caller uses it only for the serial disposition line. */
    if (terminated_name != NULL) *terminated_name = owner->name;
    FlairProcess_terminate(list, wm, master, owner);
    return 1;
}

/* --------------------------------------------------------------------------
 * flair_route_updates -- route pending window damage to owning tenants (Sec 3.3)
 * and VALIDATE every damaged window it walks: the pump's CONTENT phase.
 *
 * One forward pass over the z-order. For each VISIBLE window with a NON-EMPTY
 * updateRgn: intersect the seed with the window's CURRENT visible region
 * (BeginUpdate fidelity; ), recover the owner (tolerant),
 * synthesize + deliver one updateEvt if the owner can receive it, then validate
 * (clear the damage) UNCONDITIONALLY.
 *
 * VALIDATION CONTRACT (, absorbing; epic
 *  DQ1/DQ2, ratified 2026-07-31): under the pump order
 * (WM-op -> desktop_paint_damage(chrome) -> HERE -> present), by the time this
 * walk runs the compositor has already painted every damaged window's WDEF
 * chrome. For an UNOWNED (shell furniture) window and for an owned-but-eventless
 * app, that chrome IS the full paint -- so the damage is serviced and is
 * validated here rather than left pending forever (the old unowned skip made
 * every later minimal repaint re-walk the same stale damage; the old
 * validate-only-after-delivery idea, dissolves for the same
 * reason: validation records "serviced by this pump cycle", not "delivered").
 * WindowMgr_validate only sets updateRgn empty (it does NOT mutate the z-order
 * list), so a single forward nextWindow walk is safe -- unlike terminate's
 * DisposeWindow loop, which re-scans.
 *
 * Ref: tenant lifecycle (updateEvt -> each damaged window's owning app, background
 *      apps included), Sec 3.1 (refCon demux); window.h (WindowMgr_validate,
 *      the updateRgn damage model); os/flair/desktop.h (desktop_paint_damage /
 *      desktop_validate_all -- the chrome phase + the no-tenant analogue);
 *      spec/event_model.h (updateEvt, the WindowPtr in `message`, MTE Ch 2).
 *      independent checks, explicit error handling, reproducibility.
 * -------------------------------------------------------------------------- */
void flair_route_updates(FlairProcessList *list, WindowMgr *wm)
{
    if (list == NULL || wm == NULL)
        PROC_PANIC("route_updates: NULL argument");

    for (WindowPtr w = wm->front; w != (WindowPtr)0; w = w->nextWindow) {
        if (!w->visible) continue;                 /* hidden windows owe no update  */
        if (w->updateRgn == (region_t *)0) continue;
        if (region_is_empty(w->updateRgn)) continue;   /* no damage outstanding     */

        /* A pending seed can outlive a z-order change. BeginUpdate installs
         * updateRgn INTERSECT the CURRENT visRgn as the paint clip; without this
         * delivery-time re-clip a background tenant can paint pixels a newly
         * foreground window now owns. The Window Manager
         * verb also asserts that the post-clip set is a visRgn subset. */
        WindowMgr_clip_update_to_visible(wm, w);
        if (region_is_empty(w->updateRgn)) continue;

        /* Recover the owning tenant by the binding rule -- TOLERANT: an unowned
         * (shell furniture) window gets no delivery, NOT a panic (the distinction
         * from owner_of_window's content-click fail-loud). */
        FlairApp *owner = owner_of_window_tolerant(list, w);

        /* Synthesize ONE updateEvt for this window and deliver to its owner. The
         * window identity goes in message ((uint32_t)(uintptr_t)w, MTE Ch 2 -- exact
         * on flat-32, truncated-but-stable on host-64); where/when best-effort 0
         * (deterministic, reproducibility). */
        if (owner != (FlairApp *)0 &&
            owner->procs != (const FlairAppProcs *)0 &&
            owner->procs->event != (void (*)(FlairApp *, const EventRecord *))0) {
            EventRecord ev;
            proc_zero(&ev, (uint32_t)sizeof ev);
            ev.what    = (uint16_t)updateEvt;
            ev.message = (uint32_t)(uintptr_t)w;
            owner->procs->event(owner, &ev);
        }

        /* Clear the damage (the pump's EndUpdate) -- for EVERY walked window,
         * owned or not (DQ1: the compositor's chrome pass already serviced the
         * unowned/eventless ones; see the header contract above). */
        WindowMgr_validate(w);
    }
}

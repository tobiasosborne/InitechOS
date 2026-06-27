/*
 * os/flair/process.c -- the FLAIR App Contract: tenant lifecycle + the Layer-5
 * dispatcher (THE ARTIFACT). WAVE-1 (ORACLE-FIRST) increment.
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013, RATIFIED).
 *
 * STATUS (Law 2, oracle-first):
 *   - FlairProcessList_init / FlairProcess_launch / FlairProcess_terminate are
 *     MINIMAL-REAL: enough for the O-1 host oracle to construct + register the
 *     two stub tenants (stamp magic, bind the demux refCon, link the process
 *     list, demote/promote foreground). They do NOT yet carve the child arena
 *     or call procs->open()/close() (Wave 2; ADR-0013 Sec 3.2/3.4).
 *   - flair_app_dispatch is a STUB -- ADR-0013 Wave 2 implements this. It does
 *     NO routing and NO activation, so the O-1 oracle
 *     (harness/proptest/test_process.c) goes RED for the RIGHT reason: the
 *     routing/activation it asserts is absent. Implementing it before the oracle
 *     is green would invert the Red->Green loop (Rule 1) and defeat BC-2.
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

/* --------------------------------------------------------------------------
 * FlairProcess_launch -- WAVE-1 minimal launch (ADR-0013 Sec 8 first cut).
 * Stamp magic + bind the owned window's demux refCon + link at the head as the
 * new foreground, demoting the previous foreground to background. The full
 * flair_alloc child-arena -> procs->open() path is Wave 2 (Sec 3.2).
 * -------------------------------------------------------------------------- */
FlairApp *FlairProcess_launch(FlairProcessList *list, FlairApp *app)
{
    if (list == NULL || app == NULL) return NULL;   /* BC-5 fail-loud placeholder */

    /* BC-3: the refCon-tag sentinel. Every later refCon->FlairApp* cast checks it. */
    app->magic = FLAIR_APP_MAGIC;

    /* The binding rule (Sec 3.1): bind the owned window's refCon to this app so
     * FindWindow -> WindowPtr -> refCon -> FlairApp* demuxes to the right tenant.
     * Wave 2's procs->open() does this for the whole window group; the Wave-1
     * single-owned-window case is bound here. (On a flat-32 kernel the cast
     * round-trips; the 64-bit-host truncation hazard is documented in process.h.) */
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
 * FlairProcess_terminate -- WAVE-1 minimal teardown: mark DYING + unlink.
 * Wave 2 (Sec 3.4) adds procs->close(), DisposeWindow on the group, the one-shot
 * flair_free(app->block), and foreground promotion + activate pair.
 * -------------------------------------------------------------------------- */
void FlairProcess_terminate(FlairProcessList *list, FlairApp *app)
{
    if (list == NULL || app == NULL) return;

    app->state = (uint8_t)FLAIR_APP_DYING;

    /* unlink from the process list (does NOT read the possibly-corrupt arena;
     * the FlairApp records are caller/parent-owned -- Sec 3.4 BC-6). */
    FlairApp **pp = &list->head;
    while (*pp != NULL && *pp != app)
        pp = &(*pp)->nextApp;
    if (*pp == app)
        *pp = app->nextApp;
    app->nextApp = NULL;
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

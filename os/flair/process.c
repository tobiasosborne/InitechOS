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
 * flair_app_dispatch -- STUB -- ADR-0013 Wave 2 implements this.
 *
 * Wave 1 (oracle-first) does NOTHING: no refCon demux, no click-to-activate, no
 * keyDown-to-foreground, no activateEvt synthesis. The O-1 host oracle MUST go
 * RED against this body (Law 2 / Rule 1) -- proving the routing and activation
 * checks are LIVE before any real dispatcher exists.
 *
 * Wave 2 will (ADR-0013 Sec 3.3):
 *   - mouseDown: FindWindow -> owner via refCon (magic-checked, BC-3); keep
 *     inDrag/inGoAway/inGrow + the menu-bar band as shell verbs; inContent on a
 *     background owner -> click-to-activate (SelectWindow + the deactivate/
 *     activate activateEvt pair) THEN deliver the mouseDown to owner->procs->event.
 *   - keyDown/autoKey: deliver to the FOREGROUND app (list->head) only.
 *   - updateEvt: route to each damaged window's owning app (background included).
 *   - activateEvt: synthesize EXACTLY two records in deterministic order on a
 *     foreground change (deactivate old, then activate new).
 * -------------------------------------------------------------------------- */
void flair_app_dispatch(FlairProcessList *list, WindowMgr *wm,
                        const EventRecord *ev)
{
    /* STUB -- ADR-0013 Wave 2 implements this. Intentionally no-op so test-process
     * (O-1) is RED for the right reason. */
    (void)list;
    (void)wm;
    (void)ev;
}

/*
 * os/flair/process.h -- the FLAIR App Contract: the tenant ABI + the Layer-5
 * event dispatcher (THE ARTIFACT).
 *
 * beads: the FLAIR App Contract epic initech-4e35 (ADR-0013, RATIFIED 2026-06-27).
 *        This is the ORACLE-FIRST (Wave 1) deliverable: the locked C contract
 *        surface + the host-buildable dispatcher entry point. The routing /
 *        activation BODY is a STUB here (process.c); Wave 2 fills it in, gated
 *        GREEN by the O-1 host oracle (harness/proptest/test_process.c) first.
 *
 * WHAT THIS IS (ADR-0013 Sec 2, Sec 3.1):
 *   A FLAIR tenant is a RECORD OF ENTRY-POINTS PLUS AN ARENA -- not a process,
 *   not a thread, not a loaded program (ADR-0001: flat-32, one address space, no
 *   v8086, no isolation). It is the Apple Process Manager / MultiFinder MODEL
 *   (a process list, app-grouped z-order, one foreground app, activate/deactivate
 *   on switch, clean application-death) realized with minimal-cooperative
 *   mechanics: a `FlairApp` carries a const vtable of C entry-points the kernel
 *   links, a per-tenant child heap carved from the FLAIR arena, and the window(s)
 *   it owns -- bound to each of its windows through the WindowRecord.refCon slot.
 *
 * THE BINDING RULE / DEMUX KEY (ADR-0013 Sec 3.1):
 *   Every WindowRecord a tenant creates carries
 *       w->refCon = (int32_t)(uintptr_t)self
 *   (spec/window_record.h:400; the 4-byte assert at :468 guarantees a flat-32
 *   pointer fits). FindWindow -> WindowPtr -> refCon -> FlairApp* is the O(1)
 *   demux with no new field in the locked WindowRecord. Every routed cast asserts
 *   `app->magic == FLAIR_APP_MAGIC` before use (BC-3 / Rule 2 fail-loud).
 *
 *   HOST-SAFETY NOTE (Wave-2 grading hazard, surfaced by the O-1 oracle): on a
 *   flat-32 kernel a pointer fits in int32_t and the cast round-trips. On a
 *   64-bit HOST (the test-process build; no -m32 multilib available here) the
 *   `(int32_t)` cast TRUNCATES a 64-bit pointer, so recovering FlairApp* by
 *   casting refCon back is LOSSY. The Wave-2 host-buildable dispatcher (BC-2)
 *   must recover the owning tenant host-safely (e.g. match the WindowPtr against
 *   each app's window group in the process list) and keep refCon + magic as the
 *   fail-loud tag, OR test-process must build -m32. This header sets refCon per
 *   the contract regardless (the value is the kernel's demux key).
 *
 * SINGLE SPINE (ADR-0006 E-D2 / ADR-0013 Sec 2, BC-1/BC-2):
 *   The refCon demux and the activateEvt synthesis live HERE, in the Layer-5
 *   `flair_app_dispatch`, called IDENTICALLY by the live kmain pump and by the
 *   host O-1 oracle. kmain stays a thin source/sink adapter -- it cooks one
 *   EventRecord off the ring and hands it to flair_app_dispatch; it carries NO
 *   routing/activation logic of its own.
 *
 * FREESTANDING (Law 3): this header includes ONLY <stdint.h> + the locked spec
 * headers + window.h / heap.h; no host libc. It compiles BOTH under the kernel
 * flags and hosted for the property suite (the window.c / heap.c dual-compile
 * pattern). MenuBar is forward-declared (the Menu Manager surface is heavy and
 * the contract only needs a pointer slot).
 *
 * Ref: ADR-0013 Sec 2 (decision), Sec 3.1 (tenant ABI), Sec 3.3 (event routing
 *      & activation), Sec 3.4 (yield + clean-exit), Sec 3.5 (co-residency),
 *      Sec 3.6 (memory model), Sec 7 O-1 (the host oracle), Sec 8 (staging).
 *      spec/window_record.h Sec 1 (part-codes), :387-400 (nextWindow + refCon).
 *      spec/event_model.h Sec 1 (what-codes), :155 (FLAIR_EVT_MOD_ACTIVE_FLAG),
 *      :265-271 (EventRecord). os/flair/window.h (WindowMgr, FindWindow, ...).
 *      os/flair/heap.h (flair_heap_t, FLAIR_CLASS_GENERAL, the child sub-arena).
 *      CLAUDE.md Law 1 (ground truth), Law 2 (the oracle is truth), Law 3
 *      (freestanding artifact), Rule 2 (fail loud), Rule 11 (deterministic),
 *      Rule 12 (ASCII-clean).
 *
 * ASCII-clean (Rule 12). No timestamps / no nondeterminism (Rule 11).
 */
#ifndef INITECH_OS_FLAIR_PROCESS_H
#define INITECH_OS_FLAIR_PROCESS_H

#include <stdint.h>

#include "window.h"        /* WindowMgr, WindowPtr, FindWindow/SelectWindow/... */
#include "heap.h"          /* flair_heap_t, flair_class_t (the child sub-arena) */
#include "event_model.h"   /* EventRecord, flair_event_what_t, modifier bits    */
#include "surface.h"       /* bitmap_t -- the offscreen a tenant draws into      */

/* MenuBar is defined by os/flair/menu.h (the whole Menu Manager surface). The
 * contract only needs a pointer slot ("swapped in when foreground"), so we
 * forward-declare the tag to keep this header freestanding/kernel-buildable
 * without pulling the Menu Manager in (ADR-0013 Sec 3.1). */
struct MenuBar;

typedef struct FlairApp FlairApp;

/* ===========================================================================
 * 1. THE TENANT ABI  (ADR-0013 Sec 3.1 -- locked C surface, verbatim)
 * ===========================================================================*/

/* FLAIR_APP_MAGIC -- the refCon-tag sentinel (Rule 2 / BC-3). Stamped into
 * FlairApp.magic at launch; every refCon->FlairApp* cast asserts it before use,
 * so a tenant that scribbles its own refCon fails LOUD instead of mis-routing
 * silently. ASCII 'F','L','A','P' (0x46 0x4C 0x41 0x50). */
#define FLAIR_APP_MAGIC  0x464C4150u   /* 'FLAP' */

/* Tenant lifecycle state (ADR-0013 Sec 3.1 FlairApp.state). Exactly one app is
 * FLAIR_APP_FG at any instant (Sec 3.3 ruling). */
enum {
    FLAIR_APP_FG    = 0,   /* foreground (front contiguous window run; has menubar) */
    FLAIR_APP_BG    = 1,   /* background (still receives updateEvt; Sec 3.3)        */
    FLAIR_APP_DYING = 2    /* terminating; teardown runs between pump iterations    */
};

/* FlairLaunchParams -- the open() build parameters (ADR-0013 Sec 3.2). */
typedef struct FlairLaunchParams {
    rgn_rect_t bounds;   /* initial structure bounds the app's open() builds into */
    uint32_t   budget;   /* child-arena byte budget; over-budget = fail-loud (BC-5)*/
    WindowMgr  *wm;      /* the shell Window Manager the tenant's open() builds its
                          * window(s) into -- NewWindow(lp->wm, ...). Threaded in by
                          * FlairProcess_launch (the kernel passes the live shell wm);
                          * the demux refCon still binds open()'s windows to `self`.
                          * Ref: ADR-0013 amendment, bead initech-fka6 (FlairLaunch-
                          * Params.wm -- the locked-contract edit, Rule 8). */
    const bitmap_t *surface; /* the OFFSCREEN the tenant's open()/event() draws its
                          * content into (global coords; blitter_fill_rect_clipped on
                          * lp->surface clipped to the window's content region). The
                          * compositor composites the live z-order from this surface;
                          * a tenant that does NOT draw (the O-1/O-3/O-4 stub tenants)
                          * is launched with surface==NULL and ignores it. Additive,
                          * ADR-0013 amendment bead initech-fka6 (Wave-4 Step 1). */
} FlairLaunchParams;

/* FlairAppProcs -- the const vtable of tenant entry-points (the A5 jump-table
 * analogue). `event` is the one REQUIRED entry; `idle`/`close` may be NULL;
 * `open` builds the world. (ADR-0013 Sec 3.1, verbatim contract.) */
typedef struct FlairAppProcs {
    /* open: build windows/menus from self->arena; set every window's
       refCon = (int32_t)(uintptr_t)self. Return 0 on success, !=0 = launch fail
       (the shell reclaims the partition and reports, Rule 2). */
    int  (*open) (FlairApp *self, const FlairLaunchParams *lp);
    /* event: handle exactly ONE cooked EventRecord aimed at this tenant
       (mouseDown inContent / keyDown / updateEvt / activateEvt). REQUIRED.
       Returns promptly; returning IS the cooperative yield. */
    void (*event)(FlairApp *self, const EventRecord *ev);
    /* idle: optional nullEvent slice (caret blink, cursor shaping). May be NULL. */
    void (*idle) (FlairApp *self);
    /* close: optional teardown hook (flush docs); the shell disposes windows
       and resets the arena regardless. May be NULL. */
    void (*close)(FlairApp *self);
} FlairAppProcs;

/* FlairApp -- one tenant instance (ADR-0013 Sec 3.1, verbatim layout). */
struct FlairApp {
    uint32_t             magic;    /* FLAIR_APP_MAGIC -- refCon-tag check (Rule 2) */
    const char          *name;
    const FlairAppProcs *procs;    /* the code surface (the A5 jump-table analogue) */
    flair_heap_t         arena;    /* per-tenant CHILD heap over a carved block     */
    void                *block;    /* the parent block backing `arena` (for free)   */
    struct MenuBar      *menubar;  /* this app's menu set; swapped in when foreground */
    WindowPtr            windows;  /* head of THIS app's window group (z-order run)  */
    int32_t              refCon;   /* per-app datum                                 */
    void                *userData; /* per-INSTANCE tenant state (FULL-WIDTH pointer, */
                                   /* NOT refCon/int32): a tenant's open() stashes a */
                                   /* heap-allocated private struct here and recovers */
                                   /* it in event(self,ev) as self->userData. Zeroed */
                                   /* by FlairProcess_launch; additive, ADR-0013      */
                                   /* amendment bead initech-fka6 (Wave-4 Step 1).    */
    uint8_t              state;    /* FLAIR_APP_FG / FLAIR_APP_BG / FLAIR_APP_DYING */
    FlairApp            *nextApp;  /* the resident-app (process) list               */
};

/* ===========================================================================
 * 2. THE RESIDENT-APP (PROCESS MANAGER) LIST  (ADR-0013 Sec 3.5)
 * ---------------------------------------------------------------------------
 * The process list IS the FlairApp.nextApp chain; its head is the foreground
 * app, and the chain runs back through the app-grouped z-order. The single
 * WindowMgr z-order holds ALL apps' windows; this list groups them by tenant.
 * ===========================================================================*/
typedef struct FlairProcessList {
    FlairApp *head;   /* the foreground app (front window run), or NULL when empty */
} FlairProcessList;

/* Initialize an empty process list. */
void FlairProcessList_init(FlairProcessList *list);

/* ===========================================================================
 * 3. LIFECYCLE + THE LAYER-5 DISPATCHER  (ADR-0013 Sec 3.2/3.3/3.4, BC-2)
 * ===========================================================================*/

/* FlairProcess_register -- caller-storage registration (the O-1 host-oracle path).
 *
 * Registers a caller-constructed FlairApp (its `procs`, `name`, and owned
 * `windows` already set, its storage owned by the caller -- NOT carved from a
 * heap) as the new FOREGROUND: it stamps FLAIR_APP_MAGIC, binds the owned
 * window's refCon = (int32_t)(uintptr_t)app (the demux key, Sec 3.1), demotes the
 * previous foreground to FLAIR_APP_BG, links the app at the list head as
 * FLAIR_APP_FG, and returns it (NULL on a NULL argument). It does NOT carve a
 * child arena and does NOT call procs->open(): it is the thin no-allocation
 * registrar the dispatch oracle (test_process.c O-1) uses to wire stub tenants
 * over caller-built windows. The arena-allocating production launch is
 * FlairProcess_launch below; an app installed by _register has app->block == NULL
 * and must NOT be torn down with FlairProcess_terminate (which one-shot-frees the
 * child block). */
FlairApp *FlairProcess_register(FlairProcessList *list, FlairApp *app);

/* FlairProcess_launch -- the ARENA-ALLOCATING Toolbox launch (ADR-0013 Sec 3.2).
 *
 * Carves a tenant out of the master FLAIR heap and brings it up as the new
 * foreground (the MultiFinder "partition" is a convention, not hardware -- Sec
 * 3.6). Steps (Sec 3.2 a-g):
 *   (a) flair_alloc(master, FLAIR_CLASS_HANDLE, sizeof(FlairApp)) -- the handle;
 *   (b) flair_alloc(master, FLAIR_CLASS_GENERAL, budget)          -- the child block;
 *   (O-4 / BC-5) if EITHER alloc fails: reclaim whatever was carved, install
 *       NOTHING, leave `list`/`wm` unchanged, and return NULL (fail-loud budget --
 *       never overcommit, never partially install; Rule 2);
 *   (c) flair_heap_init(&app->arena, block, budget) -- a CHILD heap over the block;
 *       stamp magic, name, procs, block, state;
 *   (d) procs->open(app, &lp) -- builds the window(s) from app->arena and sets each
 *       w->refCon = (int32_t)(uintptr_t)app; if open() returns != 0 the launch
 *       FAILS: reclaim block + handle, install nothing, return NULL (O-4 open-fail);
 *   (e) SelectWindow the app's front window (raise z-order + activate);
 *   (f) link at the list head as the new foreground, demoting the old head to BG;
 *   (g) return the launched tenant.
 *
 * `surface` is the offscreen the launched tenant's open() draws its content into
 * (threaded into lp->surface before procs->open; the kernel passes the live FLAIR
 * compositor surface, the host stub oracles pass NULL -- their open() does not draw).
 * Additive, ADR-0013 amendment bead initech-fka6.
 *
 * Teardown is FlairProcess_terminate (the one-shot child-block free, Sec 3.4). */
FlairApp *FlairProcess_launch(FlairProcessList *list, WindowMgr *wm,
                             const bitmap_t *surface,
                             flair_heap_t *master, const FlairAppProcs *procs,
                             const char *name, rgn_rect_t bounds,
                             uint32_t budget);

/* FlairProcess_terminate -- CLEAN teardown (ADR-0013 Sec 3.4), for tenants brought
 * up by FlairProcess_launch (app->block carved from `master`).
 *
 *   (1) procs->close(app) if non-NULL (flush docs);
 *   (2) DisposeWindow every window in the wm z-order OWNED by app (refCon match +
 *       magic, the same demux rule the dispatcher uses), accruing exposure damage;
 *   (3) ONE-SHOT free: flair_free(master, FLAIR_CLASS_GENERAL, app->block) then
 *       flair_free(master, FLAIR_CLASS_HANDLE, app) LAST -- do not touch app after;
 *   (4) unlink app from the process list;
 *   (5) if app was the foreground head, promote the next app (SelectWindow its
 *       front window + foreground state).
 *
 * SCOPE LIMIT (orchestrator ruling, bead initech-ubd0): this implements CLEAN
 * teardown only. The "app-death survives a CORRUPTED child arena" path (ADR Sec
 * 3.4 vs Sec 3.6 tension -- WindowRecords live in the child arena, so removing
 * them reads that arena) is DEFERRED pending a committee ruling and is not
 * implemented here. */
void FlairProcess_terminate(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app);

/* flair_app_dispatch -- THE single Layer-5 dispatcher (ADR-0013 Sec 3.3, BC-2).
 *
 * Demuxes ONE cooked EventRecord to the owning tenant and performs activation,
 * called IDENTICALLY by the live kmain pump and the host O-1 oracle:
 *   - mouseDown: FindWindow -> part-code + WindowPtr. inDrag/inGoAway/inGrow stay
 *     shell-owned chrome verbs; the menu-bar band -> foreground app's MenuSelect.
 *     inContent on a BACKGROUND owner triggers click-to-activate (SelectWindow +
 *     the activateEvt deactivate/activate pair) THEN delivers the mouseDown.
 *   - keyDown/autoKey: the FOREGROUND app only (active window == focus).
 *   - updateEvt: each damaged window's owning app (background apps included).
 *   - activateEvt: synthesized on a foreground change A->B as EXACTLY two records
 *     in deterministic order -- deactivate (activeFlag=0) to A, then activate
 *     (activeFlag=1) to B (ADR-0013 Sec 3.3; Rule 11).
 *
 * WAVE 1: process.c ships a STUB body (no routing/activation) so the O-1 oracle
 * goes RED for the right reason before Wave 2 lands the real logic (Law 2). */
void flair_app_dispatch(FlairProcessList *list, WindowMgr *wm,
                        const EventRecord *ev);

/* flair_route_updates -- the updateEvt SPINE: route pending window DAMAGE to each
 * damaged window's owning tenant (ADR-0013 Sec 3.3 -- "updateEvt: each damaged
 * window's owning app, background apps included").
 *
 * Walks the WindowMgr z-order (wm->front .. nextWindow) and, for every VISIBLE
 * window whose updateRgn is NON-EMPTY, synthesizes ONE updateEvt EventRecord
 * (what=updateEvt; message = the affected window's identity, (uint32_t)(uintptr_t)w
 * per MTE Ch 2 -- exact on flat-32; where/when best-effort/zero, Rule 11),
 * recovers the owning FlairApp by the SAME refCon-match rule the dispatcher uses
 * (magic-tag + truncated-identity match, width-portable), delivers it to that
 * tenant's `event` entry-point, and then WindowMgr_validate(w) clears the
 * updateRgn (the pump's EndUpdate).
 *
 * TOLERANCE (the distinction from content-click dispatch): a damaged window with
 * NO owning resident app -- the shell's own frame/desktop furniture, which is
 * unowned -- is SKIPPED without panicking and WITHOUT validating (the shell drives
 * its own furniture repaint). owner_of_window (content dispatch) instead PANICS on
 * an unowned hit, because a content click on an ownerless window IS a bug; an
 * unowned window merely carrying damage is NOT. Called by the live kmain pump each
 * iteration after the input event is dispatched; graded by the host O-5 oracle
 * (harness/proptest/test_process_update.c). */
void flair_route_updates(FlairProcessList *list, WindowMgr *wm);

#endif /* INITECH_OS_FLAIR_PROCESS_H */

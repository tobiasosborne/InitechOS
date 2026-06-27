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

/* FLAIR_TENANT_RECORDS_DEFAULT -- the default RECORDS-arena budget (bytes) carved
 * as a FLAIR_CLASS_HANDLE block from the master FLAIR heap and laid over with
 * `records_arena` (ADR-0013 Amendment AC-2, the initech-ubd0 split-arena
 * resolution). The RECORDS arena holds a tenant's WindowRecord(s) + ALL its
 * region_t pools (strucRgn/contRgn/updateRgn + the clip scratch); the shell reads
 * ONLY these during teardown, so app DEATH survives a scribbled DATA arena (BC-6).
 *
 * ARITHMETIC (the requirement this must cover; document, do not under-size):
 *     sizeof(WindowRecord)                         ~  256 B  (host; less on the
 *                                                    32-bit kernel -- 4-byte ptrs)
 *   + 4 * region bundle (ref_tenant ten_rgn_t      ~ 4 * 816 B = 3264 B host)
 *        = region_t + rgn_row_t[32] + int16_t[128])
 *   + 5 * flair_blk_t headers (16 B each, one per  ~   80 B
 *        flair_alloc: rec + 4 region bundles)
 *   ----------------------------------------------------------
 *     actual requirement                           ~ 3600 B  (host worst case)
 *   rounded UP to 16 KiB for slack (future resource blobs / extra WindowRecords /
 *   16-byte alignment). 16384 is comfortably > sizeof(FlairApp) (~232 B host) --
 *   a HARD requirement of the LIFO free-order coupling in FlairProcess_terminate /
 *   _kill: the small FlairApp HANDLE handle must be the FLAIR_CLASS_HANDLE
 *   free-list head after teardown so cycle-2's first (small) HANDLE alloc reuses
 *   it instead of the large records block (records_budget > sizeof(FlairApp);
 *   see process.c). Per-tenant overrides may pass any records_budget; the demo
 *   passes FLAIR_TENANT_RECORDS_DEFAULT (spec/flair_tenants_demo.h sizing note). */
#define FLAIR_TENANT_RECORDS_DEFAULT  (16u * 1024u)

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
    flair_heap_t         arena;    /* per-tenant DATA child heap (GENERAL block):    */
                                   /* per-instance private state, TERec, userData,   */
                                   /* future resource blobs (ADR-0013 AC-2 DATA side)*/
    void                *block;    /* the parent GENERAL block backing `arena` (free)*/
    flair_heap_t         records_arena; /* per-tenant RECORDS child heap (HANDLE     */
                                   /* block): the WindowRecord(s) + ALL region_t      */
                                   /* pools (strucRgn/contRgn/updateRgn + clip        */
                                   /* scratch). The shell reads ONLY these during     */
                                   /* teardown, so app DEATH survives a scribbled     */
                                   /* DATA arena (ADR-0013 Amendment AC-2; BC-6).     */
    void                *records_block; /* the parent HANDLE block backing            */
                                   /* records_arena (for free). AC-2.                 */
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
 * 3.6). SPLIT-ARENA carve (ADR-0013 Amendment AC-2, the initech-ubd0 resolution):
 * a tenant owns TWO child blocks -- a RECORDS block (HANDLE; WindowRecord(s) +
 * region pools) and a DATA block (GENERAL; per-instance state). Steps:
 *   (a) flair_alloc(master, FLAIR_CLASS_HANDLE, sizeof(FlairApp))   -- the handle;
 *   (b) flair_alloc(master, FLAIR_CLASS_HANDLE, records_budget)     -- RECORDS block,
 *       then flair_heap_init(&app->records_arena, records_block, records_budget);
 *   (c) flair_alloc(master, FLAIR_CLASS_GENERAL, budget)            -- DATA block,
 *       then flair_heap_init(&app->arena, block, budget);
 *   (O-4 / BC-5) if ANY alloc fails: reclaim whatever was carved IN REVERSE carve
 *       order, install NOTHING, leave `list`/`wm` unchanged, return NULL (fail-loud
 *       budget -- never overcommit, never partially install; Rule 2);
 *       stamp magic, name, procs, block, records_block, state;
 *   (d) procs->open(app, &lp) -- builds the window(s): the WindowRecord + region
 *       pools from app->records_arena (AC-2), per-instance state from app->arena,
 *       and sets each w->refCon = (int32_t)(uintptr_t)app; if open() returns != 0
 *       the launch FAILS: reclaim data + records + handle, install nothing, NULL;
 *   (e) SelectWindow the app's front window (raise z-order + activate);
 *   (f) link at the list head as the new foreground, demoting the old head to BG;
 *   (g) return the launched tenant.
 *
 * `records_budget` sizes the RECORDS arena (WindowRecord + region pools; the demo
 * passes FLAIR_TENANT_RECORDS_DEFAULT); `budget` sizes the DATA arena. `surface`
 * is the offscreen the launched tenant's open() draws its content into (threaded
 * into lp->surface before procs->open; the kernel passes the live FLAIR compositor
 * surface, the host stub oracles pass NULL -- their open() does not draw). Additive,
 * ADR-0013 amendment bead initech-fka6; split-arena AC-2 (bead initech-ubd0).
 *
 * Teardown is FlairProcess_terminate (clean) / FlairProcess_kill (death); both
 * one-shot free the data block + records block + handle (Sec 3.4 / AC-2). */
FlairApp *FlairProcess_launch(FlairProcessList *list, WindowMgr *wm,
                             const bitmap_t *surface,
                             flair_heap_t *master, const FlairAppProcs *procs,
                             const char *name, rgn_rect_t bounds,
                             uint32_t records_budget, uint32_t budget);

/* FlairProcess_terminate -- CLEAN teardown (ADR-0013 Sec 3.4), for tenants brought
 * up by FlairProcess_launch (app->block / app->records_block carved from `master`).
 *
 *   (1) procs->close(app) if non-NULL (flush docs);
 *   (2) DisposeWindow every window in the wm z-order OWNED by app (refCon match +
 *       magic, the same demux rule the dispatcher uses), accruing exposure damage;
 *       window structural state is read ONLY from records_arena-resident fields;
 *   (3) ONE-SHOT free in LIFO order: DATA block (FLAIR_CLASS_GENERAL, app->block)
 *       FIRST, then records block (FLAIR_CLASS_HANDLE, app->records_block), then the
 *       FlairApp handle (FLAIR_CLASS_HANDLE, app) LAST -- do not touch app after;
 *   (4) unlink app from the process list;
 *   (5) if app was the foreground head, promote the next app (SelectWindow its
 *       front window + foreground state).
 *
 * SPLIT-ARENA (ADR-0013 Amendment AC-2, bead initech-ubd0): the WindowRecord(s) +
 * region pools live in app->records_arena (a SEPARATE HANDLE block), NOT in the
 * DATA arena, so step (2) reads only intact records and the data block is freed
 * WITHOUT reading its (possibly corrupt) payload -- which is exactly what makes
 * FlairProcess_kill (below) survive a scribbled DATA arena (BC-6 now satisfied). */
void FlairProcess_terminate(FlairProcessList *list, WindowMgr *wm,
                            flair_heap_t *master, FlairApp *app);

/* FlairProcess_kill -- the DEATH path (ADR-0013 Sec 3.4 application-death; AC-2 /
 * BC-6). Identical to FlairProcess_terminate EXCEPT it NEVER calls procs->close():
 * a crashed app's code must not run again. The shared teardown helper removes the
 * app's windows from the z-order (reading ONLY records_arena-resident structural
 * fields -- intact even if the DATA arena was scribbled), promotes the successor,
 * and one-shot-frees the data block + records block + handle WITHOUT reading the
 * data payload. This is the corrupt-arena death path that ADR-0013 Sec 3.4 claims
 * and the initech-ubd0 split-arena ruling (AC-2) finally makes TRUE: app death
 * survives a corrupted DATA arena because no dead-heap byte is ever trusted. */
void FlairProcess_kill(FlairProcessList *list, WindowMgr *wm,
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

<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0013 -- The FLAIR App Contract: the tenant model for co-resident applications

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record (ADR)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0013 |
| Title | ADR-0013: The FLAIR App Contract (tenant model, launch, event routing, co-residency, memory) |
| Version | 1.0 (Ratified) |
| Status | **RATIFIED (ADR-by-committee, chair-synthesized + adversarially verified; operator-ratified 2026-06-27; all load-bearing claims repo-verified against HEAD)** |
| Classification | Internal Use Only |
| Information Sensitivity | Tier 2 (Non-Public, Non-Regulated) |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Review Board (App-Contract Committee: Mac-heritage / Win16 / minimal-cooperative seats), Chair-synthesized |
| Effective Date | 2026-06-27 |
| Next Scheduled Review | Upon the reference-tenant deliverable (Sec 8 minimal first cut) |
| Supersedes | (none -- supersedes the stale "ADR-0010 App Contract" reference in docs/plans/FLAIR-implementation-plan.md and epic initech-4e35) |
| Superseded By | (none) |
| Related Documents | ADR-0001 (386+, 32-bit flat, no v8086, no isolation -- decision of record; no standalone file, repo convention); ADR-0004 (FLAIR Toolbox: Layer-5 app launch D-1:138, cooperative non-preemptive D-6:180, ISR-enqueue-only D-4:170); ADR-0006 (Live Event Loop: E-D2 single-spine; two-tier behavioural grading); ADR-0009 (SAMIR/Milton: DEC-03 flat .COM, DEC-06 PAL console -- the text-tenant precedent); ADR-0010 (FLAIR Grading and Goldens -- grade vs decomp goldens, never preview.webp); ADR-0012 (North-Star Expansion -- the app-platform bar, PRD Sec 1.4); PRD Sec 2 (non-goals), Sec 6.x, Sec 8 |
| Related Issues | the FLAIR App Contract epic initech-4e35 (re-pointed from "ADR-0010" to this ADR) + the reference-tenant / pump-generalization / activateEvt-synthesis / SAMIR-text-tenant-launch children; the new Phase-4.5 "Platform Services" epic (ADR-0012 D-2b) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |
| Distribution | OEA; Platform Engineering; Presentation Layer Section (FLAIR); QA; Change Advisory Board; Records Management (Archive Annex B) |

### Revision History

| Rev | Date | Author | Description of Change | Reviewed By |
|---|---|---|---|---|
| 0.1 | 2026-06-27 | App-Contract Committee (3 seats), Chair-synthesized | Initial draft (ADR-by-committee `wf_42352963-a57`). Mac Process-Manager model as spine, minimal-cooperative mechanics grafted, Win16 dispatch-shape only. All file:line citations verified against HEAD; three drift corrections folded (refCon field at window_record.h:400; desktop_paint_all at desktop.c:149 via shell_render shell.h:239; SHELL_MAX_WINDOWS==2 at shell.h:104; osEvt synthesis not promised by event_model.h:71). | (adversarial verify) |
| 1.0 | 2026-06-27 | Chair + Adversarial Reviewer | Ratified. Adversarial verifier returned "ratifiable-with-fixes"; the four fixes are folded into the normative text: (1) the refCon demux/activation extracted into a HOST-BUILDABLE Layer-5 dispatcher (`flair_app_dispatch`) called by both kmain and the oracle, so E-D2 single-spine holds and oracle #1 is real (Sec 2, Sec 3.3, BC-2); (2) the teardown/leak oracle reframed to "avail STABLE across cycles >=2 / leak drifts down" to match the bump+free-list heap contract (Sec 7 O-3); (3) a fail-loud launch-budget oracle and a char-tenant suspend/rebuild oracle added (Sec 7 O-4/O-7); (4) citation fixes (FLAIR_CLASS_GENERAL enumerator at heap.h ~108, not heap.h:177; ADR-0001 dangling-path note). | T. Osborne (Operator) |
| 1.1 | 2026-06-27 | App-Contract Committee (Architecture Review Board) | **Amendment AC-2 -- the SPLIT-ARENA resolution of initech-ubd0** (full text in `ADR-0013-AMENDMENT-AC-2-Split-Arena.md`; summary appended as "Amendment AC-2" below). Resolves the Sec 3.4-vs-3.6 tension AC-1 deferred: Sec 3.4 claims app DEATH survives a corrupted child arena, but Sec 3.6 + the code put each tenant's WindowRecord + region pools IN the child arena, so the DisposeWindow loop reads the dead heap -- the claim was FALSE (BC-6 unsatisfied). Ruling **option (b) split-arena**: a tenant now owns TWO child blocks -- a RECORDS block (`FLAIR_CLASS_HANDLE`; WindowRecord(s) + all region pools, read during teardown) and a DATA block (`FLAIR_CLASS_GENERAL`; per-instance state). New `FlairProcess_kill` (= terminate minus `close()`) is the death path. **BC-6 now SATISFIED** -- oracle-proven (O-3 death-survival sub-test + UBD0_MUT_RECORDS_IN_DATA_ARENA / TEARDOWN_MUT_LEAK_RECORDS mutants). | (ARB; adversarial-verified) |

### Approval & Sign-Off Matrix

| Role | Name | Disposition | Date |
|---|---|---|---|
| Author / Drafter -- Platform / Kernel seat | (ARB, App-Contract Committee) | Ratify -- confirmed `flair_heap_init(base,size)` child sub-arena is real (heap.h:160); flagged `SHELL_MAX_WINDOWS==2` (shell.h:104) as the first thing to raise | 2026-06-27 |
| Author / Drafter -- FLAIR/Toolbox (Mac-heritage) seat | (ARB, App-Contract Committee) | Ratify -- Process-Manager semantics are the right fidelity spine; inverted-WaitNextEvent dispatch is the honest first increment under E-D2 | 2026-06-27 |
| Author / Drafter -- minimal-cooperative seat | (ARB, App-Contract Committee) | Ratify-with-graft -- tenant-is-an-event-handler-not-a-thread, refCon magic tag, child-arena one-shot teardown, shell-owned chrome adopted wholesale | 2026-06-27 |
| ARB Reviewer -- Technical Correctness | M. Bolton (Senior Engineer, Platform) | Approved | 2026-06-27 |
| ARB Reviewer -- Period Authenticity | S. Nagheenanajar (Engineering, Heritage Conformance) | Approved (Win16 per-task queues rejected; one SPSC ring is canon) | 2026-06-27 |
| ARB Reviewer -- Governance & Compliance | T. Smykowski (QA / Change Advisory) | Approved | 2026-06-27 |
| Adversarial Consistency Reviewer | (ARB, App-Contract Committee) | "ratifiable-with-fixes" -- four fixes folded (see Rev 1.0) | 2026-06-27 |
| Operator Ratification | T. Osborne (Operator) | **Granted (2026-06-27)** | 2026-06-27 |
| Records Management | M. Waddams (Archive Annex B) | Filed | 2026-06-27 |

---

## 1. Context and Problem

FLAIR today boots a **single hardcoded scene**: a fixed two-window desktop wired by `shell_build_scene` (`os/flair/shell.h:207`), painted by `shell_render` (`os/flair/shell.h:239`) via `desktop_paint_all` (`os/flair/desktop.c:149`), driven by the live cooperative `WaitNextEvent` pump in `flair_desktop_run` / `kmain` (`os/milton/kmain.c:806`, pump at `:1716`). The pump cooks the raw ring into one `EventRecord`, calls `FindWindow` (`os/flair/window.h:230`), and dispatches **only chrome verbs**: `inDrag -> flair_live_do_drag` (`kmain.c:1092`), `inGoAway -> flair_live_do_close` (`kmain.c:1158`), and the menu-bar band `-> flair_live_do_menu` (`kmain.c:1219`). There is **no notion of an application**: no `inContent` routing, no keyboard routing, no `activateEvt`, no way to add a window that *owns behaviour*, no way to start, switch between, or tear down apps.

PRD Sec 6 and ADR-0012 (PRD Sec 1.4, the app-platform bar) require **multiple co-resident applications** -- InitechCalc, FileManager, InitechPaint, FILE COPY now; Initech 123 and InitechWord next; the resident Turbo Initech later. The *Office Space* frame is itself the requirement: two overlapping System-7 document windows plus the modal FILE COPY box -- that **is** co-residency, on screen, in the spec. The App Contract is the missing spine: what an application *is*, how the shell starts it, how the one event pump reaches it, how several share the screen, and how one dies cleanly.

The constraints are hard and non-negotiable:

- **ADR-0001:** flat-32, single address space, **no v8086, no memory isolation, no protection.** A "process" cannot be an address space.
- **PRD Sec 2 / ADR-0004 D-6 (`:180`):** scheduling is **cooperative, non-preemptive** on the PIT tick. No preemption, ever.
- **ADR-0006 E-D2:** there is **exactly one event spine.** `kmain` is a thin source/sink adapter over the single `WaitNextEvent` pump (`event.c:535`); a second event loop or a second pixel path is forbidden.
- **The locked spec is the contract:** `EventRecord` what-codes and the `activateEvt` active-flag are `_Static_assert`-pinned (`spec/event_model.h:368-421`); `WindowRecord.refCon` is a 4-byte `LongInt` with its own assert (`spec/window_record.h:400,468`). The header records that the pump synthesizes **`updateEvt`, `activateEvt`, and `nullEvent`** -- and only those (`spec/event_model.h:71`).
- **Law 2:** every part of the contract needs an *independent* mechanical oracle.
- **The SAMIR precedent:** character-mode tenants already ship, run full-screen through the single-program loader (`loader_run_plan`, `os/milton/loader.c:609`) and the PAL console (`os/samir/pal/pal_milton.c:5`, the sole `int 0x21` site).

The problem: deliver MultiFinder-class co-residency **without** preemption and **without** isolation, composing with the one pump, the one z-order, and the one heap that already exist.

## 2. Decision

A **FLAIR tenant** is a *record of entry-points plus an arena* -- **not a process, not a thread, not a loaded program.** It is the Apple Process Manager / MultiFinder *model* (a process list, app-grouped z-order, one foreground app, activate/deactivate on switch, clean application-death) realized with the minimal-cooperative seat's discipline: in a single flat address space a tenant is a `FlairApp` struct (a const vtable of C entry-points the kernel links + a per-tenant child heap carved from the FLAIR arena + the window(s) it owns), bound to each of its windows through the existing `WindowRecord.refCon` slot. There is **one** pump; the shell **inverts `WaitNextEvent`** and demuxes each cooked `EventRecord` to the owning tenant by `refCon` -- a tenant **never** calls `WaitNextEvent` itself, so *returning from its event entry-point is its cooperative yield*. Native windowed apps (current C bundle, and 123/Word) are launched by a Toolbox-level call and co-reside on the desktop; character-mode apps (SAMIR, Turbo Initech) keep the **separate, unchanged** single-program loader and run full-screen to completion. No second event path, no second pixel path, no preemption, no isolation.

**Single-spine realization (folded fix, ADR-0006 E-D2).** The refCon demux and the `activateEvt` synthesis are NOT kmain-local logic. They are implemented as a **host-buildable FLAIR Layer-5 dispatcher** -- a new `flair_app_dispatch(EventRecord*, FlairProcessList*)` (and `FlairProcess_*` lifecycle) in a new `os/flair/process.{c,h}` -- called **identically** by the live `kmain` pump and by the host oracle `test-process`. `kmain` stays a thin source/sink adapter: it cooks the event off the ring and hands it to `flair_app_dispatch`; it carries **no** routing/activation logic of its own. This keeps the behaviour dimension single-spine (no kmain-only path the host suite cannot mutation-prove) and makes Sec 7 O-1 a real, host-runnable oracle over the exact symbols the live binary runs.

## 3. The Contract (normative)

### 3.1 The tenant ABI

A tenant is a `FlairApp` instance carrying a const vtable of entry-points (native FLAIR apps are C, ADR-0002), plus the data it owns: its child arena, its window group, its menu set. The locked C surface:

```c
typedef struct FlairApp FlairApp;

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

struct FlairApp {
    uint32_t             magic;    /* FLAIR_APP_MAGIC -- refCon-tag check (Rule 2) */
    const char          *name;
    const FlairAppProcs *procs;    /* the code surface (the A5 jump-table analogue) */
    flair_heap_t         arena;    /* per-tenant CHILD heap over a carved block     */
    void                *block;    /* the parent block backing `arena` (for free)   */
    MenuBar             *menubar;  /* this app's menu set; swapped in when foreground */
    WindowPtr            windows;  /* head of THIS app's window group (z-order run)  */
    int32_t              refCon;   /* per-app datum                                 */
    uint8_t              state;    /* FLAIR_APP_FG / FLAIR_APP_BG / FLAIR_APP_DYING */
    FlairApp            *nextApp;  /* the resident-app (process) list               */
};
```

Binding rule (the demux key): **every** `WindowRecord` a tenant creates carries `w->refCon = (int32_t)(uintptr_t)self` (`spec/window_record.h:400`; the 4-byte assert at `:468` guarantees a flat-32 pointer fits). `FindWindow -> WindowPtr -> refCon -> FlairApp*` is O(1) with no new field in the locked `WindowRecord`. **Every routed cast asserts `app->magic == FLAIR_APP_MAGIC`** before use (Rule 2: a tenant that scribbles its own refCon fails loud, it does not mis-route silently). `event` is the one **required** entry; `idle`/`close` may be NULL; `open` builds the world. Native apps expose one registration symbol, e.g. `extern const FlairAppProcs InitechCalc_procs;` -- **no new executable format** is introduced for native FLAIR apps.

### 3.2 Launch

Two launch classes mapped onto the two mechanisms that already exist; they are **deliberately separate**.

1. **Native FLAIR Toolbox app (C, in the kernel image).** Launch is a Toolbox-level call (Layer 5 owns app launch, ADR-0004 D-1:138), **not** the program loader. `FlairProcess_launch(&InitechCalc_procs, bounds)` (new, additive): (a) `flair_alloc` a partition block of the app's budget from the master FLAIR heap as `FLAIR_CLASS_GENERAL` (the enumerator at `heap.h` ~108; passed to `flair_alloc` at `heap.h:176-177`); (b) zero a `FlairApp`, set `magic`, and `flair_heap_init(&app->arena, block, size)` -- a **child heap** over that block (`heap.h:160` takes a caller (base,size) window precisely for this); (c) call `procs->open()`, which does `NewWindow` (`window.h:139`, inserts at z-order front + activates) and sets each `w->refCon = app`; (d) `SelectWindow` the app's front window (`window.h:156`); (e) install the app's `menubar`; (f) link at the head of the resident-app list as the new foreground; (g) `WindowMgr_invalidate` to seed the first `updateEvt`. No MZ/PSP/INT-21h is involved. The Finder analogue is the desktop shell.

2. **Character-mode DOS tenant (SAMIR, Turbo Initech, any `.COM`/InitechMZ `.EXE`).** Launched through the **unchanged** single-program loader: `load_program_from_fat` (`loader.c:432`) -> `loader_run_plan` (`loader.c:609`), with the single-in-flight guard `g_load_active` rejecting nested loads as `LOADER_ERR_BUSY` (`loader.c:504-510`). These take the **full console** and run to completion (the SAMIR precedent, ADR-0009). They are **not** co-resident FLAIR tenants: launching one from the desktop **suspends** the cooperative pump -- the desktop is saved-under and torn down, the DOS box runs, the desktop is rebuilt and repainted on `loader_exit_hook` return (`loader.c:528`). Period-authentic, like a DOS session under a GUI. The native-launch path **never re-enters the loader**, and the loader path never threads the z-order.

### 3.3 Event routing and activation

The single ISR raw ring -> `WaitNextEvent` pump is **unchanged** (`event.c:168` producer, `:324` `cook_raw`, `:535` pump). The shell hands each cooked `EventRecord` to the Layer-5 dispatcher `flair_app_dispatch` (Sec 2; new `os/flair/process.c`), which generalizes the current hardcoded `FindWindow` dispatch (`kmain.c:1716`+) into refCon-based demux:

- **`mouseDown`:** `FindWindow(wm, ev.where, &w)` yields part-code + `WindowPtr`. `inDrag`/`inGoAway`/`inGrow` stay **shell-owned chrome verbs** (`DragWindow`/`DisposeWindow`/grow) -- the title bar belongs to the Window Manager, not the app (period-authentic; the DefWindowProc analogue). The menu-bar band (`y < FLAIR_MENUBAR_H`, for which `FindWindow` returns `inDesk` -- `window.c` has no `inMenuBar` branch, so the band test *is* the detection) goes to the **foreground** app's `menubar -> MenuSelect`. For `inContent`: if `(FlairApp*)w->refCon` is not the foreground app, perform **click-to-activate first** (raise the owner's whole window group via `SelectWindow`, swap menubars, fire the activate pair below), **then** deliver the `mouseDown` to `owner->procs->event`.
- **`keyDown`/`autoKey`:** delivered to the **foreground** app only. In System 7 the active (front, hilited) window *is* keyboard focus; there is no separate focus state.
- **`updateEvt` (`=6`):** synthesized in **task context** (never in an ISR; ADR-0004 D-4). After any damage op, the shell walks visible windows and routes an `updateEvt` to each damaged window's **owning** app -- **including background apps** (background windows still repaint). The handler repaints clipped to `updateRgn`, then `WindowMgr_validate`.
- **`activateEvt` (`=8`, active-flag `FLAIR_EVT_MOD_ACTIVE_FLAG` at `event_model.h:155`):** the **required** activation mechanism. On any foreground change A->B the dispatcher synthesizes **exactly two** cooked records in deterministic order (Rule 11): a **deactivate** (`activeFlag=0`) to A's front window, then an **activate** (`activeFlag=1`) to B's front window. This synthesis does not exist yet and is net-new work -- but it is **header-promised and assert-pinned** (`event_model.h:71,376,418`), so it is the authentic, locked contract.

**Suspend/resume ruling (reconciling the Mac seat with the locked spec).** Seat A wanted MultiFinder `osEvt` suspend/resume. The locked event model promises synthesis of **only** `updateEvt`/`activateEvt`/`nullEvent` (`event_model.h:71`); `osEvt` (`=15`) exists as a code but its synthesis is **not** promised. Ruling: **`activateEvt` (deactivate-old, activate-new) is the REQUIRED switch mechanism now.** `osEvt` suspend/resume is **DEFERRED** to the future true-background-processing phase (an additive amendment that extends `cook_raw` synthesis); until then, "background" means "not foreground, still receives `updateEvt`," which is sufficient for co-residency and stays inside the locked spec. Exactly one foreground app at any instant.

### 3.4 The cooperative yield and clean-exit contract

**Yield is structural and inverted.** A tenant does **not** run a loop; its `event`/`idle` handler does the work for one event and **returns to the shell pump.** Returning *is* the yield. A tenant **never** calls `WaitNextEvent` (single spine, E-D2) and **never** runs a nested modal `WaitNextEvent` loop -- modal loops (the existing re-enter-WNE-until-mouseUp idiom in `flair_live_do_drag`/`flair_live_do_menu`) stay **shell-owned helpers**. The HLT idle hook stays where it is: `flair_event_set_yield` (`event.c:115`, installed at boot) is owned by the pump, not tenants; idle energy behaviour is unchanged.

**Non-yielding tenant = authentic hang.** If a handler loops forever the entire desktop hangs (all co-resident apps). This is **period-authentic cooperative behaviour and is explicitly NOT to be fixed with preemption or a shipping watchdog** (ADR-0004 D-6; CLAUDE.md hallucination callout -- a watchdog needs preemption, a non-goal). The **test-only** bounded tick budget (`FLAIR_LIVE_TICK_BUDGET` in the gate build) may *detect* non-termination for the oracle; **no watchdog ships.**

**Clean exit / teardown** happens **between pump iterations, never mid-handler.** An app requests quit (Quit item / `inGoAway` -> `FlairProcess_terminate(self)`, sets `state=DYING`). The shell then: (1) calls `procs->close()` if non-NULL; (2) `DisposeWindow` on each window in the app's group (`window.h:148`), which accrues the exposure damage of everything they covered into the windows behind + the desktop (the same `DiffRgn` path `flair_live_do_close` already uses, `kmain.c:1158`); (3) frees the app's **whole child arena in one shot** -- `flair_free(parent, FLAIR_CLASS_GENERAL, app->block)` (`heap.h:195`); no per-block walk; (4) unlinks it from the process list; (5) if it was foreground, promotes the next app (its front window via `SelectWindow` + the activate pair + menubar swap); (6) `desktop_paint_damage` repaints only the exposed area + present.

**Application death (crash, not clean quit)** routes through steps (2) and (5) **independently of the dead app's possibly-corrupt arena**: `WindowRecord`s are caller-owned records the WindowMgr threads, so removing them from the z-order does not read the dead heap. The corrupt block is leaked-then-reclaimed at teardown, never trusted.

### 3.5 Multi-app co-residency

The **resident-app list** (the Process Manager process list) is the `FlairApp.nextApp` chain; its head is the foreground app. The **single** WindowMgr z-order (`nextWindow`, `window.h:88`) holds **all** apps' windows, **grouped by application** MultiFinder-style: the foreground app's windows form the front contiguous run, each background app's group below it in app order. Co-residency is achieved **without preemption or isolation** purely by: (a) **one** compositor paints every visible window of every app back-to-front -- `shell_render` (`shell.h:239`) -> `desktop_paint_all` (`desktop.c:149`) -- no second pixel path (ADR-0006 D-2); (b) **one** pump cooks events and the dispatcher demuxes by `refCon`. All apps' state lives simultaneously in the single flat address space; **only one app's code runs at any instant** -- the one whose handler the pump is currently inside -- which is exactly cooperative multitasking.

**Switching:** a click in a background app's window calls `SelectWindow`, raises that app's **whole** window group to the front run, moves the foreground pointer, fires the deactivate/activate `activateEvt` pair, and swaps the foreground app's `MenuBar` into the menu-bar band. The active app's front window shows the hilited title bar; background windows show deactivated chrome.

**Required mechanical change (Law 1 honesty):** `SHELL_MAX_WINDOWS` is hardcoded to **2** (`shell.h:104`) and the window store is a fixed array (`shell.h:121,149`). The first cut **must** raise this bound and generalize the store from a hardcoded scene to a tenant-installed registry; the back-to-front painter's pass and `DiffRgn` minimal-repaint already handle arbitrary overlap.

### 3.6 The memory model

Flat-32, single address space, **no MMU partitions, no protection** (ADR-0001) -- so the MultiFinder "partition" is a **convention**, not hardware. The master FLAIR heap over `[FLAIR_HEAP_BASE, +FLAIR_HEAP_SIZE)` (`kmain.c:834`) is the **parent** allocator. Each tenant gets a **child sub-arena**: `flair_alloc` one `FLAIR_CLASS_GENERAL` block of the app's budget, then `flair_heap_init` a child `flair_heap_t` over that block (`heap.h:160`). The app allocates its windows/regions/menus/globals **only** from its own child arena. **Teardown is a single `flair_free` of the parent block** -- the one-shot clean-death property, and the reason app-death survives a corrupt child heap. Partition sizing is a **fail-loud budget** (Rule 2): a launch that cannot carve its block **refuses and reports**; it does not overcommit. There is **no inter-app protection**: a buggy tenant can scribble another's arena -- authentic and an explicit non-goal (ADR-0001 / PRD Sec 2); that is precisely why Sec 3.4 structures death-cleanup to be independent of the dead heap. The shell-owned modal dialog / FILE COPY overlay stays a shell layer drawn last (`shell_render` order, `shell.c:17`), **not** a tenant.

**Heap-contract note (folded fix).** The FLAIR heap is a **bump allocator with per-class LIFO free-lists**: `flair_free` pushes the freed block onto its class free-list and does **NOT** roll the bump cursor back (`heap.h:179-195`), and `flair_heap_avail` reports `size - used` = the bump tail only, not free-listed blocks (`heap.h:197-203`, `heap.c:176-178`). Consequence for teardown: after the first launch+free, `avail` drops and does **not** return to its pre-launch value; a subsequent same-budget launch **reuses** the free-listed block, so `avail` stays **stable** across cycles. The teardown oracle (Sec 7 O-3) is written to that true invariant, not to a false "returns to pre-launch" expectation.

## 4. The native-vs-text-host fork ruling (Initech 123 / InitechWord)

**RULING: Initech 123 and InitechWord are NATIVE FLAIR Toolbox tenants** (C, ADR-0002) -- co-resident windowed apps under this contract -- **NOT** character-mode tenants, and **NOT** run inside any "windowed text-console host." The fork is decided by *what the artifact is in the frame*, and both arms already have a precedent in the tree, so **no third mechanism is created.**

Rationale:

1. **Law 4 (look like the frame).** The *Office Space* still shows 123 and Word as System-7 **document windows** with Mac chrome, under the two-stacked-menu-bar Photoshop chimera, with live pull-down menus. A full-screen TUI cannot reproduce `documentProc` chrome + live `MenuSelect`; fidelity is the product.
2. **Co-residency requires it.** Only native FLAIR apps participate in the WindowMgr z-order and the one compositor (`shell_render`/`desktop_paint_all`). A full-screen text tenant takes the **entire** console and is mutually exclusive with the desktop -- it cannot co-reside.
3. **Architectural law.** Running a character-mode program inside a FLAIR window would demand a **second pixel path AND a second event path** -- a direct ADR-0006 D-2 / E-D2 violation. 123 and Word draw through the GrafPort like every other tenant.

**SAMIR / InitechBase and Turbo Initech are the counter-precedent and stay CHARACTER-MODE** text tenants: a full-screen dot-prompt REPL / blue IDE doing all I/O through the cooked-line PAL (`conin_line`/`conout`, `os/samir/include/samir/pal.h:141,146`; sole `int 0x21` site `pal_milton.c:5`), launched via the single-program loader run-to-completion (ADR-0009 DEC-03/DEC-06). That is exactly how a dBASE dot prompt and a Turbo Pascal IDE each owned the whole screen -- authentic, not a limitation. The "DOS box in a window" is **explicitly rejected**: it needs the barred second pixel/event path, and a v8086 DOS box is barred by ADR-0001.

**Honest scope caveat:** 123/Word are not implemented yet; today's bundled C apps are InitechCalc/FileManager/InitechPaint/FILE COPY. They join as native `FlairApp` tenants when built. A *future* FLAIR windowed front-end over the SAMIR engine (ADR-0009 S8.3, gated later) would be a **new GrafPort front-end over the engine** -- not the `.COM` in a text box -- and is out of scope here. **Crisp rule: in the frame as a window -> native windowed tenant (`FlairApp` + refCon + pump routing); a character-mode REPL/IDE -> text tenant via the existing single-program loader. Two ABIs, two existing precedents, zero new loader.**

## 5. Alternatives Considered

**Seat B -- Win16 message-loop (RegisterClass/WndProc/GetMessage).** Rejected as the spine. FLAIR's GUI heritage is verbatim Inside-Macintosh -- `EventRecord`, `WindowRecord`, `FindWindow`, `WaitNextEvent` are everywhere in the existing code -- so grafting Win16 vocabulary on top produces an **incoherent chimera** that fails the period-authenticity bar (Law 4). Worse, the Win16 model assumes per-task message queues and independent `GetMessage` stacks, but FLAIR has **one** global SPSC raw ring, **one** cursor, **one** modifier state (`event.c`); true per-task queues are impossible without forking the event core, which E-D2 forbids. **Grafted, not adopted:** the per-window dispatch *shape* (one pump fanning out to many windows' handlers in one task) is exactly our inverted-`WaitNextEvent` demux -- we keep that idea and drop the Win16 naming and the per-task-queue assumption.

**Seat C -- minimal pragmatic cooperative.** Not rejected so much as **grafted wholesale**, but not adopted as the *sole* spine. Its thesis -- co-residency is ~90% already built in `window.c`; only label windows with owners and route the four event classes -- is correct and is the backbone of Sec 3.1-3.6 (tenant-is-an-event-handler-not-a-thread; refCon binding + magic tag; chrome stays shell-owned; teardown = `DisposeWindow` + arena free; no nested `WaitNextEvent`). It was **not** sufficient alone because it omits the explicit **process table** and **app-grouped z-order** needed for more than one *background* app, for raising a whole window group on switch, and for clean multi-app **death** -- the Process-Manager structure from Seat A. The synthesis is Seat A's *model* expressed with Seat C's *mechanics*.

## 6. Consequences

**Becomes possible:** multiple co-resident native apps on one desktop; click-to-activate app switching with correct chrome and menubar swap; clean app launch and teardown; application-death recovery that survives a corrupt tenant heap; a stable target ABI for InitechCalc/FileManager/InitechPaint/FILE COPY now and 123/Word/Turbo-Initech later.

**New obligations created -- the Phase-4.5 platform services now have a home** (they become shared Toolbox services tenants call):
- **Resource Manager** -- tenant assets (`WDEF`/menu/icon/string resources) loaded into the tenant's child arena.
- **Scrap (Clipboard)** -- a shell-owned cross-tenant Scrap; copy/paste between co-resident apps (the payoff of co-residency).
- **TextEdit** -- shared editable-text service for `event`-driven content (123 cells, Word body).
- **Standard File** -- shell-owned Open/Save dialog (a shell modal helper, like FILE COPY, **not** a tenant).
- **Print Manager** -- shared print path; the `570-` trailing-minus and 116% pie canon live in app content, not the service.

**Migration of SAMIR to the first tenant:** SAMIR is migrated as the first **contract-governed text-tenant**. Its launch from the desktop becomes the ratified suspend-desktop -> save-under -> `loader_run_plan` -> rebuild-desktop-on-return path (Sec 3.2 class 2). The **save-under + desktop-rebuild-on-return path does not exist yet** and is the concrete first text-tenant deliverable (graded by Sec 7 O-7).

**Binding constraints (BC).**
- **BC-1 (single spine, pixel + behaviour).** One pump, one compositor; no second event loop or pixel path (ADR-0006 D-2/E-D2).
- **BC-2 (host-buildable dispatcher).** The refCon demux + `activateEvt` synthesis live in the host-buildable Layer-5 `flair_app_dispatch` (`os/flair/process.c`), called identically by `kmain` and `test-process`; `kmain` carries no routing logic of its own. Routing must not be trusted until O-1 (host) is green and mutation-proven.
- **BC-3 (refCon magic tag).** Every refCon->`FlairApp*` cast asserts `magic == FLAIR_APP_MAGIC` (Rule 2 fail-loud).
- **BC-4 (cooperative, no watchdog ships).** Yield is return-from-handler; a non-yielding tenant hangs the desktop; only a test-only tick budget detects it (ADR-0004 D-6).
- **BC-5 (fail-loud launch budget).** An over-budget launch refuses + reports + reclaims with no partial install (graded by O-4).
- **BC-6 (death survives a corrupt arena).** Teardown removes windows from the z-order and one-shot-frees the child block without reading the dead arena.

**Costs accepted:** a non-yielding tenant hangs everything (authentic; no watchdog ships); no memory isolation (a bug in one tenant can corrupt another -- mitigated by the refCon magic tag and death-cleanup independence, not by protection); child sub-arenas must fit a finite master heap, so launch is a fail-loud budget; `SHELL_MAX_WINDOWS==2` and the hardcoded scene must be generalized; `activateEvt` synthesis is net-new and must be host-graded **before** routing is trusted; "co-resident apps" today means **in-image, statically-linked** native tenants (`extern const FlairAppProcs InitechCalc_procs`), **not** disk-loaded native app files (per-file native loading stays deferred, ADR-0003 DEC-08) -- a weaker but explicitly-labeled form of multi-app that still delivers multiple genuinely independent co-resident tenants.

## 7. Oracle Plan (Law 2 -- every gate independent and mutation-proven; never by-construction)

- **O-1 -- Host routing/dispatch property oracle** (new `test-process`, sibling to `test-interact`; runs `flair_app_dispatch` directly per BC-2): launch >=2 stub `FlairApp`s whose `event` appends `(what, where, window-id)` to a log; replay a scripted `EventRecord` trace; assert the log against a **hand-authored** expected sequence (the independent golden, **not** computed from the dispatcher). Asserts: each `mouseDown`/`keyDown` reaches the **correct** owner (recomputed from refCon + z-order, independently of the router); a switch emits **exactly one** deactivate+activate `activateEvt` pair, in order. **Mutants:** "always route to foreground / ignore refCon" -> RED; "skip the activate/deactivate synthesis" -> RED; "deliver keyDown to window-under-cursor not active" -> RED.
- **O-2 -- Host activation / z-order oracle** (extend `test_window`): after each `SelectWindow`, assert `hilited==1` on exactly `wm->front`, the front app's whole group is the front contiguous run, and `refCon` round-trips to the right tenant. **Mutant:** `SelectWindow` that omits the hilite update -> RED.
- **O-3 -- Host teardown / leak oracle (reframed to the true heap contract).** Launch then exit a tenant for N>=3 cycles. Assert: cycle-1 `flair_heap_avail` drops by ~the app budget; cycles N>=2 keep `avail` **STABLE** (the freed `FLAIR_CLASS_GENERAL` block is reused from the class free-list; `flair_free` does not roll back the bump cursor, `heap.h:179-203`). A **leak** (forgetting `flair_free(app->block)`) makes `avail` **drift monotonically down** per cycle -> RED. App-death with a deliberately corrupted child block still removes the windows (caller-owned records) and one-shot-frees the parent block. **Mutants:** "free the windows but leak the child block" -> avail drifts down -> RED; "read the corrupt child heap during death" -> panic/RED. *(NOTE: the naive "avail returns to pre-launch value" invariant is WRONG for this bump+free-list heap and would red a correct impl -- do not use it.)*
- **O-4 -- Host fail-loud launch-budget oracle.** Call `FlairProcess_launch` with a budget exceeding the master heap remaining; assert it returns launch-fail, installs **no** window/menubar (no partial state), reclaims cleanly, and reports (Rule 2 / BC-5). **Mutant:** "overcommit / leave a partial install on budget fail" -> RED.
- **O-5 -- Emu behavioural app-switch gate** (extend `test-flair-drag`, ADR-0006 E-D5, two-tier): boot a multi-app desktop; inject a click on background app B while A is foreground; **screendump (Law 4)** asserts B's group is now front, B's menubar replaced A's in the bar, A's front window deactivated (hilite gone); **Tier-A damage law** asserts no over-repaint outside `(old front UNION new front)`. Independent **value** golden = a WDEF/chrome geometry scan graded against the `system7-decomp`/`win31-decomp` rendered goldens, **never `preview.webp`** (ADR-0010). Extend the markers to `FLAIR-DISPATCH app=<name>`. **Mutants:** "menubar does not swap on activate" -> bar-band assertion RED; "drop updateEvt routing" -> exposed area stays stale -> RED.
- **O-6 -- Cross-emulator (Rule 5):** the switch + activate path runs on **QEMU and Bochs**; a disagreement is a stop-condition, not a QEMU-pin. The character-mode text-tenant path keeps its own ADR-0009 boot->USE->LIST gate unchanged.
- **O-7 -- Emu char-tenant suspend/rebuild gate (the first text-tenant deliverable).** From the live desktop, launch SAMIR (Sec 3.2 class 2); assert (serial) the pump suspends + a `FLAIR-SUSPEND` marker; SAMIR runs its ADR-0009 boot->USE->LIST; on `loader_exit_hook` return assert (screendump) the rebuilt desktop is **bit-identical** to the pre-suspend dump and a `FLAIR-RESUME` marker fires. **Mutant:** "do not rebuild/repaint the desktop on return" -> post-return screendump != pre-suspend dump -> RED.
- **O-8 -- Non-yielding watchdog (test-only):** a mutant tenant whose `event` loops must trip the bounded-tick timeout marker -- proving the *detector*, while confirming no preemptive kill ships.

## 8. Staging

**Minimal first cut (the ratifiable increment):**
- Implement the host-buildable Layer-5 dispatcher `os/flair/process.{c,h}`: `FlairApp`/`FlairAppProcs`, refCon binding + magic, `flair_app_dispatch`, `FlairProcess_launch`/`_terminate`, the child sub-arena (`flair_alloc` block -> `flair_heap_init` child -> one-shot free).
- Add `activateEvt` (deactivate/activate) synthesis in task context inside the dispatcher -- gated by O-1/O-2 **before** routing is trusted (BC-2).
- Make `kmain` a thin adapter: cook the event off the ring, call `flair_app_dispatch`; keep `inDrag`/`inGoAway`/menu-band as shell verbs; remove kmain-local content/key routing.
- Raise `SHELL_MAX_WINDOWS` (`shell.h:104`) and convert the hardcoded scene/window-store into a tenant registry.
- Ship a **reference 'hello window' native tenant + a SECOND co-resident native tenant**, and the host oracles (O-1..O-4) + the emu app-switch gate (O-5), each mutation-proven.
- **Migrate SAMIR** as the first text-tenant: the suspend-desktop -> `loader_run_plan` -> save-under -> rebuild-on-return launch path (O-7).

**Deferred (additive amendments, each its own issue):** `osEvt` suspend/resume for true background processing (Sec 3.3 ruling); the Phase-4.5 platform services (Resource Manager, Scrap, TextEdit, Standard File, Print Manager -- ADR-0012 D-2b); per-file native app loading (ADR-0003 DEC-08 still defers it -- native apps ship in-image for now); a future windowed SAMIR GrafPort front-end (ADR-0009 S8.3); any multi-program loader (the single-program guard `g_load_active` stands, so multi-app co-residency is scoped to **in-image native tenants** -- a hard boundary, not an oversight).

---

## 9. Related Decisions

| Document | Relationship |
|---|---|
| **ADR-0012 (North-Star Expansion -- the app-platform bar)** | Driver. This ADR is the keystone deliverable of ADR-0012 D-2(a); the platform bar (PRD Sec 1.4) is unreachable without it. |
| **ADR-0004 (FLAIR Toolbox Architecture)** | Parent. Layer-5 owns app launch (D-1:138); cooperative non-preemptive (D-6:180); ISR-enqueue-only task-context synthesis (D-4:170). |
| **ADR-0006 (Live Event Loop and Behavioural Grading)** | The single spine (E-D2) this contract routes over; BC-1/BC-2 keep the behaviour dimension single-path. O-5 extends `test-flair-drag`. |
| **ADR-0009 (SAMIR/Milton Integration)** | The character-mode text-tenant precedent (DEC-03 flat .COM, DEC-06 PAL console); SAMIR is migrated as the first text-tenant (O-7). |
| **ADR-0010 (FLAIR Grading and Goldens)** | The value-golden discipline O-5 obeys: grade vs the decomp goldens, never `preview.webp`, never by-construction. |
| **ADR-0001 (386+, 32-bit flat)** | The flat-32 / no-v8086 / no-isolation constraints that make the tenant a convention-not-a-process. |
| **ADR-0003 + DEC-08 (InitechDOS loader)** | The single-program loader the class-2 text path reuses unchanged; per-file native loading stays deferred (in-image tenants for now). |
| **docs/plans/FLAIR-implementation-plan.md** | Phase 4 (this ADR) + the new Phase 4.5 platform services; the "ADR-0010 App Contract" references are corrected to ADR-0013. |
| **PRD Sec 2, Sec 6.x, Sec 8** | The cooperative-not-preemptive non-goal; the desktop/apps spec; the every-subsystem-has-an-oracle mandate this Sec 7 satisfies. |

---

## Amendment AC-2 -- the split-arena resolution of initech-ubd0 (BC-6 satisfied)

*Normative. Amends Sec 3.2, 3.4, 3.6, Sec 7 O-3, and BC-6. Full enterprise-doc
version: `ADR-0013-AMENDMENT-AC-2-Split-Arena.md`. Resolution of bead initech-ubd0,
the death-survival tension AC-1 (`ADR-0013-AMENDMENT-AC-1-App-Contract-Integration.md`
Sec 6) explicitly deferred.*

**The defect.** Sec 3.4 asserts application **death** "routes through steps (2) and
(5) **independently of the dead app's possibly-corrupt arena**" because "`WindowRecord`s
are caller-owned records the WindowMgr threads, so removing them from the z-order does
not read the dead heap." But Sec 3.6 and the implementation allocate each tenant's
`WindowRecord` **and** its region pools **from the tenant child arena** (`ref_tenant.c
open_common`: `rec`,`rs`,`rc`,`ru`,`rk` carved from `&self->arena`). So
`FlairProcess_terminate`'s `DisposeWindow` loop dereferences `w->nextWindow` /
`w->refCon` / `w->strucRgn` -- all of which live in the very arena a crash corrupts.
The Sec 3.4 claim was therefore **false today** and **BC-6 was unsatisfied** (the
corrupt-arena death path was scoped out of the first cut, AC-1 Sec 6 / initech-ubd0).

**The ruling -- option (b), SPLIT ARENA.** A tenant now owns **TWO** child blocks
carved from the master FLAIR heap at launch, instead of one:

1. **(Sec 3.6 amended) Two child blocks.** A **RECORDS block** (`FLAIR_CLASS_HANDLE`),
   over which a new `records_arena` is `flair_heap_init`'d, holds the tenant's
   `WindowRecord`(s) + **all** its `region_t` pools (`strucRgn`/`contRgn`/`updateRgn`
   + the clip scratch). A **DATA block** (`FLAIR_CLASS_GENERAL`), over which the
   existing `arena` is init'd, holds everything else (per-instance private state,
   `TERec`, `userData`, future resource blobs). The shell reads **only** the RECORDS
   arena during teardown; the DATA arena is freed without reading its payload. The
   field names `block`/`arena` keep their meaning as the DATA side (unchanged);
   siblings `records_block`/`records_arena` are added (`os/flair/process.h`).

2. **(Sec 3.2 amended) Launch carves THREE blocks, fail-loud.** `FlairProcess_launch`
   gains a `records_budget` parameter (immediately before `budget`) and carves, in
   order: (a) the `FlairApp` handle (`HANDLE`), (b) the RECORDS block (`HANDLE`,
   `records_budget`) then `flair_heap_init(&app->records_arena, ...)`, (c) the DATA
   block (`GENERAL`, `budget`) then `flair_heap_init(&app->arena, ...)`. On **any**
   carve NULL it reclaims whatever was carved **in reverse**, installs nothing, leaves
   `list`/`wm` unchanged, and returns NULL (BC-5 preserved; no partial install).
   `open()` carves the `WindowRecord` + regions from `records_arena`, per-instance
   state from `arena`. A new constant `FLAIR_TENANT_RECORDS_DEFAULT` (16 KiB; >
   `sizeof(FlairApp)`) sizes the demo records arena.

3. **(Sec 3.4 amended) Two teardown entries over one helper.** A shared helper does
   {`DisposeWindow` loop + unlink + promote-next + the three one-shot frees}, reading
   window structural state **only** from `records_arena`-resident fields and freeing
   the DATA block **without** reading its payload. `FlairProcess_terminate` (clean
   quit) calls `procs->close()` then the helper; the **new** `FlairProcess_kill`
   (DEATH/crash) calls the helper **only** -- a dead app's code is never run again.
   **Free order is LIFO and load-bearing:** DATA block (`GENERAL`) first, then the
   RECORDS block (`HANDLE`), then the `FlairApp` handle (`HANDLE`) **last** -- so the
   small handle is the `HANDLE` free-list head and cycle-2's first (small) `HANDLE`
   alloc reuses it rather than the large records block (requires `records_budget >
   sizeof(FlairApp)`), keeping the O-3 avail-stable invariant.

4. **(Sec 7 O-3 amended) The death-survival sub-test is now REQUIRED.** O-3's cycle-1
   drop is now THREE spans (handle + records + data). The deferred corrupt-arena
   sub-test is added and mandatory: launch a tenant, `memset(app->block, 0xFF, budget)`
   to scribble the DATA arena, call `FlairProcess_kill`, and assert the process list +
   z-order go empty with **no panic** and `avail` stable across N>=3 cycles -- a
   NON-by-construction proof (Law 2). Mutants: `UBD0_MUT_RECORDS_IN_DATA_ARENA`
   (records carved from the DATA arena -> the scribble corrupts them -> kill reads a
   corrupted z-order -> RED) and `TEARDOWN_MUT_LEAK_RECORDS` (skip the records-block
   free -> `avail` drifts down -> RED); the prior `TEARDOWN_MUT_LEAK_BLOCK` still
   bites. O-4 gains an over-records-budget fail-loud leg.

5. **(BC-6 amended) BC-6 is now SATISFIED.** "Death survives a corrupt arena" is no
   longer aspirational: teardown removes windows from the z-order and one-shot-frees
   the child blocks **without reading the (DATA) dead heap**, because the records the
   loop reads live in a SEPARATE, uncorrupted block. Verified mechanically, not by
   reviewer prose (Law 2).

This is the initech-ubd0 resolution (architecture-committee ruling, option (b)).

---

*End of ADR-0013. RATIFIED by ADR-by-committee + adversarial verification, operator-ratified 2026-06-27; Amendment AC-2 (split-arena, initech-ubd0) appended 2026-06-27. Spine: Apple Process Manager model (Seat A); mechanics: minimal-cooperative (Seat C, grafted); Win16 dispatch-shape only (Seat B, noted). All file:line citations verified against HEAD. Controlled Document; verify revision before use.*

<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0013 Amendment AC-1 -- App Contract Integration (FlairLaunchParams.wm/surface, FlairApp.userData, flair_route_updates) + the live-desktop wiring

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** ADR Amendment (binding; amends ADR-0013 in place)
**Programme:** STAPLER (InitechOS)
**Status:** **RATIFIED** (Wave-4 integration committee `wf_8a917ec8-514` + adversarial verification; orchestrator-graded on QEMU+Bochs; 2026-06-27)
**Amends:** ADR-0013 (The FLAIR App Contract)
**Related Issues:** initech-fka6 (this amendment), initech-id1z (Wave-4 first cut), initech-ubd0 (deferred death-survival tension)

---

## 1. Why this amendment

ADR-0013 ratified the tenant model but left three small contract points and several
live-desktop integration decisions unspecified. Implementing the first cut (the
host-buildable dispatcher booting two co-resident tenants on the 386) surfaced them.
This amendment records the locked-contract additions (Rule 8) and the integration
rulings, all oracle-backed and graded green.

## 2. Locked-contract additions (Rule 8)

- **AC-1.1 `FlairLaunchParams.wm` (`WindowMgr *`).** A tenant's `open(self, lp)` needs
  the Window Manager to `NewWindow`. `FlairProcess_launch` now sets `lp.wm` before
  calling `procs->open`. (`os/flair/process.h`.)
- **AC-1.2 `FlairLaunchParams.surface` (`const bitmap_t *`).** A tenant repaints its
  content into the shared offscreen on `updateEvt`/`activateEvt`. `FlairProcess_launch`
  gains a `surface` parameter (placed after `wm`) and sets `lp.surface`. Host oracles
  that do not draw pass `NULL`.
- **AC-1.3 `FlairApp.userData` (`void *`).** Per-instance tenant state, reachable in
  `event(self, ev)` as `self->userData`. A FULL-WIDTH pointer (not the int32 `refCon`,
  which truncates on the 64-bit host) -- the classic Toolbox per-instance hook.

These are additive; the host oracle surface stayed green at 283 across the change.

## 3. New spine helper

- **AC-1.4 `flair_route_updates(FlairProcessList *, WindowMgr *)`.** Task-context
  `updateEvt` routing: walks the z-order, and for each visible window with a non-empty
  `updateRgn` delivers an `updateEvt` (clipped to that region) to its owning tenant
  (refCon match) then `WindowMgr_validate`s it. **It TOLERATES unowned windows** (shell
  frame/furniture) -- no panic -- unlike the content-click dispatch path, which fails
  loud on an unowned content click (Rule 2). Host-graded by `test-process-update`
  (+ WRONG_OWNER / SKIP_VALIDATE mutants).

## 4. Live-desktop integration rulings (Wave-4 committee + verifier fixes)

- **AC-1.5 Sub-flag, not a rewrite.** The App Contract enters the booted desktop behind
  a NEW sub-flag `-DFLAIR_LIVE_TENANTS` on `BOOT_FLAIR_LIVE`, in a 4th demo image
  `build/flair_tenants.img`. The canonical Office Space frame (FLAIRSHELL/FLAIRLIVE) and
  its green gates (`test-flair-desktop/drag/menu`) stay BYTE-IDENTICAL (Law 4, Rule 11).
- **AC-1.6 shell.c untouched (frame protection).** The committee's "relax the
  `shell_build_scene` n_windows<1 guard" was REJECTED because it changes `shell.o` bytes
  (breaking byte-identity of the frame images). Instead the tenant arm reuses
  `shell_build_scene` UNCHANGED (`show_modal=0`) and `HideWindow`s the two frame doc
  windows -- a clean bars+desktop scene with no chrome divergence and no unowned-content
  panic (hidden windows are skipped by `FindWindow`).
- **AC-1.7 kmain is a thin adapter (ADR-0006 E-D2).** `flair_app_dispatch` is the SOLE
  routing call for every cooked event; the existing chrome verbs
  (`inDrag`/`inGoAway`/menu-band -> `flair_live_do_*`) stay verbatim. On a foreground
  switch the arm invalidates the new-foreground content, then
  `desktop_paint_damage` + `flair_route_updates` + `DrawMenuBar(head->menubar)` +
  present, emitting `FLAIR-DISPATCH app=<name>`.
- **AC-1.8 Activation observable = tenant CONTENT accent, NOT chrome hilite.** The
  renderer has no active/inactive title-bar distinction (`flair_draw_document_window`
  takes no `hilited` arg; `w->hilited` is never read by the compositor). Grading a
  title-bar hilite would force editing the SHARED chrome path and move the frozen frame
  goldens. So the active tenant renders a content ACCENT block (its `event(activateEvt)`
  path); the O-5 gate grades that. (Adversarial-verifier fix.)
- **AC-1.9 O-5 demo geometry: NOTES partially occludes HELLO.** So clicking HELLO's
  visible sliver raises HELLO and EXPOSES the overlap, which the tenant must repaint via
  `updateEvt` -- making the drop-updateEvt mutant bite (avoids the "passes because windows
  don't overlap" trap). (Adversarial-verifier fix.)
- **AC-1.10 Tenant menu pull-down deferred.** `bar_sys` remains the live pull-down source;
  the O-5 gate grades only the rendered menu-bar BAND swap (the foreground tenant's bar
  drawn into the top band). Functional tenant pull-down is a follow-up.

## 5. Oracle / acceptance (Law 2/4)

- Host: O-1 routing, O-2 activation/z-order + group-raise, O-3 teardown/leak, O-4
  launch-budget, `test-process-update` -- all green + mutation-proven (283 host gates).
- Emu: `test-flair-appswitch` (QEMU, two-tier independent grader, canon-only) PASS +
  4 mutants RED (IGNORE_REFCON/DROP_UPDATE = TIER-A, SKIP_ACTIVATE = TIER-B,
  NO_MENUBAR_SWAP = MENU-BAND); `test-flair-appswitch-bochs` boot-leg PASS;
  `test-flair-desktop` (frame) PASS. NO_GROUP_RAISE mutant omitted (single-window
  no-op; host-covered by `test-process-activate-mutant` RAISE_FRONT_ONLY).
- `_kernel_end = 0x38860 < 0x40000`; default images byte-identical (sha256/cmp proven).

## 6. Deferred (beads)

- **initech-ubd0** -- ADR-0013 Sec 3.4-vs-3.6 tension: app-DEATH survival vs
  WindowRecords-in-child-arena. This amendment scopes the first cut to CLEAN teardown
  only; corrupt-arena death survival needs a committee ruling + a further amendment.
- Tenant menu pull-down (AC-1.10); `osEvt` suspend/resume; per-file native app loading
  (ADR-0003 DEC-08); the SAMIR character-mode text-tenant (initech-06jd, Wave 5);
  86Box leg (initech-q0gy); frame-windows-as-tenants end-state.

---

*End of ADR-0013 Amendment AC-1. RATIFIED 2026-06-27. Controlled Document.*

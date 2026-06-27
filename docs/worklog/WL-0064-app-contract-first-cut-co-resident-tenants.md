<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0064 -- The FLAIR App Contract first cut: co-resident tenants LIVE on the 386 (4e35)

**Type:** orchestrated multi-lane session (orchestrator owns grading/integration/commits;
delegated coding lanes; one committee for the kernel-integration fork).
**Date:** 2026-06-27.
**Beads:** epic `initech-4e35` (App Contract, ADR-0013) -- children `vi5k`+`xp5t` CLOSED
(dispatcher + host oracles), `id1z` delivered (live wiring + O-5), `06jd` (SAMIR
text-tenant) OPEN. Amendments: `fka6` (ADR-0013 AC-1). Tension filed: `ubd0`.
Commits `e91d00b..2451dfd` (Wave 1..Step 6).

## Context

Operator directive: orchestrate the next most consequential FLAIR work -- delegate each
coding step, orchestrator grades/integrates/commits, raise beads on issues, convene the
committee for serious decisions, keep going. The ratified north star now includes the
app-platform bar (ADR-0012); ADR-0013 ratified the App Contract design but `os/apps/` was
empty and nothing booted a tenant. This session built the first cut.

## What changed (RED->GREEN per wave, each graded + committed)

- **Wave 1 (e91d00b):** oracle-first -- `os/flair/process.{c,h}` FlairApp/FlairAppProcs ABI
  + `test_process.c` O-1 routing oracle, stub dispatcher RED for the right reason.
- **Wave 2 (4164255):** `flair_app_dispatch` GREEN -- refCon demux + click-to-activate
  (deactivate/activate pair, deterministic order) + foreground promotion. Owner recovery
  is WIDTH-PORTABLE (match `(int32_t)(uintptr_t)app == w->refCon` + magic tag) -- the
  resolution to the host-64/no-m32 truncation hazard. 275 host.
- **Wave 3a (4ecd62d):** arena lifecycle -- `FlairProcess_launch` (child sub-arena,
  fail-loud budget) + `FlairProcess_terminate` (clean teardown, one-shot free). O-3
  teardown/leak (reframed to the bump+free-list heap: avail STABLE >=2, leak drifts down)
  + O-4 launch-budget. 279 host. (terminate unlinks/promotes BEFORE the free -- the ADR's
  literal order would use-after-free.)
- **Wave 3b (e1ff3e7):** app-group raise (whole group to the front run) + O-2
  activation/z-order oracle. 281 host. The host App-Contract arc (O-1..O-4) complete +
  mutation-proven.
- **Wave 4 Step 0 (a320846):** contract foundation -- `FlairLaunchParams.wm` +
  `flair_route_updates` (updateEvt routing; tolerates unowned shell windows) +
  `test-process-update`. 283 host. (ADR-0013 amendment AC-1.)
- **Wave 4 Steps 1-2 (eb2063c):** `os/apps/ref_tenant.c` HELLO+NOTES tenants (content
  draw via the `flair_look` C-8 seam; ABI: `FlairLaunchParams.surface` + `FlairApp.userData`)
  + the O-5 grader `tools/ppm_flair_appswitch_check.c` + locked trace, mutation-proven
  against hand-made PPM pairs. Shared demo contract `spec/flair_tenants_demo.h`.
- **Wave 4 Step 4 (caf3d65):** THE BOOT -- the `-DFLAIR_LIVE_TENANTS` kmain arm launches
  HELLO+NOTES co-resident; QEMU screendump (Law-4 eyeball) shows the two stacked menu bars
  + teal desktop + HELLO (white) partially occluded by NOTES (gray, foreground). shell.c
  byte-identical (reuse + HideWindow), default images byte-identical, frame gate PASS.
  `_kernel_end=0x38860<0x40000`.
- **Wave 4 Steps 5-6 (2451dfd):** `test-flair-appswitch` -- VERIFIED on metal that clicking
  HELLO's sliver RAISES+ACTIVATES it (TIER-A overlap NOTES->HELLO + updateEvt repaint,
  TIER-B content accent, MENU-BAND swap), independent canon grader PASS. 4 mutants RED;
  Bochs boot-leg PASS; wired into TEST_EMU_GATES.

## The committee (serious fork)

Convened (`wf_8a917ec8-514`, 3 seats + chair + adversarial verify) on the Wave-4
integration strategy: how to wire tenant dispatch into the booted desktop WITHOUT
regressing the Office Space frame (Law 4). Ratified: sub-flag + separate image; shell.c
untouched. The adversarial verifier caught two real fixes folded before any coding: (1)
the renderer has no chrome hilite -> the activation observable is the TENANT CONTENT
ACCENT (else grading it would touch the shared chrome path and move the frozen frame
goldens); (2) HELLO must be PARTIALLY occluded so the drop-updateEvt mutant bites (the
no-overlap trap). The orchestrator further overrode the committee's shell.c guard-relax
(it moves shell.o bytes) in favour of the byte-safe HideWindow fallback.

## Frictions / lessons

- Host is 64-bit with NO -m32 multilib -> `(int32_t)(uintptr_t)ptr` truncates. Fixed by
  matching truncated-both-sides (refCon demux works on host-64 AND flat-32) + a full-width
  `void *userData` for per-instance state. Never cast refCon back to a pointer.
- The FLAIR heap is bump + per-class free-list; `flair_free` does NOT roll back the bump
  cursor. The teardown/leak oracle must assert "avail STABLE across cycles >=2", NOT
  "returns to pre-launch" (which would red a correct impl).
- ADR-0013 Sec 3.4 ("death survives a corrupt arena") conflicts with Sec 3.6
  (WindowRecords live in the child arena). Filed `ubd0`; first cut does CLEAN teardown only.
- Concurrent `make` in `build/` races -- parallel coding lanes verified via direct `gcc`
  to /tmp; the orchestrator ran the authoritative `make` serially.
- Law-4 grading by an agent: build image -> QEMU screendump PPM -> pnmtopng -> Read the
  PNG. The booted desktop image is the operator-grade eyeball, in-loop.

## Acceptance

`make test-unit` ALL GREEN 283 host (was 273). `test-flair-appswitch` PASS + 4 mutants RED;
`test-flair-desktop` (frame) PASS; `test-flair-appswitch-bochs` boot-leg PASS. Full
`make test` re-run as the authoritative integration grade. Default boot + frame images
byte-identical; `_kernel_end<0x40000` all images; reproducible; ASCII-clean.

## Pointers / next

- `os/flair/process.{c,h}` (dispatcher/lifecycle/route_updates), `os/apps/ref_tenant.{c,h}`,
  `spec/flair_tenants_demo.h`, `tools/ppm_flair_appswitch_check.c`,
  `os/milton/kmain.c` (FLAIR_LIVE_TENANTS arm), `docs/adr/ADR-0013-AMENDMENT-AC-1-*.md`.
- See it: `make run-flair-tenants` (interactive) or `build/qemu_harness --disk
  build/flair_tenants.img --screendump --screendump-after FLAIR-TENANTS-READY ...`.
- **NEXT (Wave 5, `initech-06jd`):** migrate SAMIR as the first CHARACTER-MODE text-tenant
  (suspend-desktop -> save-under -> loader_run_plan -> rebuild-on-return), O-7 gate.
  Then resolve `ubd0` (death-survival) and the Phase-4.5 platform services (`initech-49ez`)
  before the canonical app suite.

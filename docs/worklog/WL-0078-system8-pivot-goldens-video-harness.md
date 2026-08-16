<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0078 -- The System 8 pivot opens: Platinum goldens minted, the video harness lands, two DQ rulings ship

**Date:** 2026-08-15/16
**Certificate:** `make clean && make test` = **ALL GREEN 321 host + 85 emu** (closing run; legs G/H joined test-flair-solid, no gate-count change)
**Commits:** `6a5e817..f23ee36` (initech-os, pushed) + `60c7a36` (system7-decomp, local -- no remote configured)

## Context

The operator ruled (2026-08-15, after extensive research) that the reference
frame's Mac chrome is **Mac OS 8 (Platinum), not System 7**, and directed: get
System 8 goldens, orchestrate the pivot, validate all GUI work with actual
tested videos, use codex (gpt-5.6-sol xhigh) extensively, work mostly serial.

## What changed

### Epic initech-t1i0 (System 8 pivot) -- filed; 4 of 6 children CLOSED this session

1. **`kd60` CLOSED -- the Platinum golden-mint environment exists.** Mac OS
   8.1 ISO (archive.org, exact 451,694,592 bytes) installed under Basilisk II
   on the EXISTING Quadra 650 ROM/config from the System-7 mint era; the
   codex lane was structurally blocked (its sandbox forbids Unix sockets --
   no Xvfb/QMP), so an opus agent drove the installer by
   screenshot-inspect-click. `goldens/work/boot81.img` boots to the Platinum
   Finder at 640x480@256 (in-emulator Monitors&Sound proof). Hard-won traps
   (Setup-Assistant hang; SIGKILL-poisons-HFS-invisibly) in bd memory
   `basilisk-ii-mac-os-8-1-mint-traps`.
2. **`slwi` CLOSED -- 8 Platinum chrome captures + 329 extracted resources.**
   Active/inactive window PAIR, dropped menu, Appearance panes, real modal
   alert w/ default ring, windowshade. HONEST finding: **no wctb -4096, no
   thme -- Platinum's grays are hard-coded in the Appearance Extension's
   CDEF/WDEF 68k code**, so the captures are the color ground truth. Full
   named Platinum CDEF taxonomy extracted; 19 accent clut ramps.
3. **`4lmg` CLOSED -- specs/sys8/ authored (sister repo, ~1430 lines).**
   Title bar 22px (Sys7: 19), 12 stripe rows FFFFFF/969696, widget order
   close|zoom|collapse(RIGHTMOST) w/ diagonal-gradient bevels, 3 scrollbar
   states. **THE GAMMA PROOF:** all 160 sampled colors map bijectively onto
   the ROM clut's plain 0x11 ladder under Mac HiRes Std Gamma -- every spec
   table carries sampled AND nominal columns (a reimplementation must NOT
   bake gamma into its CLUT). Accent = clut 208 "Lavender" (independently
   cross-checked: the scroll thumb IS its entries 1-5). Orchestrator
   spot-verified by independent PIL scan.
4. **`s97h` DRAFT authored (commit 12f265a) -- ADR-0004-AMENDMENT-DEC-10.**
   SYS8_PLATINUM becomes the base era; teal identity survives; the accretion
   digest is NOT re-baselined (append-only proof preserved -- the agent's
   correction to the brief); `flair_skin_resolve` has ZERO os/ callers, so
   the flip lands via canon re-key (both claims re-verified first-person).
   **STATUS: awaiting operator ratification** (OQ-2 teal-vs-lavender accent
   is now a sharper question given the clut-208 finding).
5. Remaining: `3knt` (populate rows + re-key oracles; blocked on s97h) ->
   `i3si` (chrome.c Platinum rendering; l9cd dependency now satisfied).

### initech-l9cd CLOSED (+v1.1) -- the GUI-interaction VIDEO harness

`qemu_harness --record` dumps one PPM per injected event; `make record-flair
SCRIPT=<locked trace>` encodes GIF+MP4 (ffmpeg bitexact, metadata stripped);
`make record-flair-repro` PROVEN byte-identical across two full
boot-capture-encode cycles. **Two observer-effect traps caught by serial
markers, not by looking at the pretty frames** (bd memory
`record-flair-observer-effect`): per-frame overhead pushed injection past (a)
the pump's 250-tick live budget and (b) the 150-tick drag-track guard --
clips LOOKED complete but the trace's back half never dispatched. Fix: both
bounds now -D-overridable, a widened RECORD image variant
(`flair_tenants_mut_record.img`); the default kernel proven byte-identical by
sha256. Deferred halves (auto-on-red ring, icount hardening, flairlive-trace
recording) -> `zwo8`.

### Solidity Wave B: two DQ rulings shipped, both codex-implemented + video-validated

- **`r8r7` CLOSED (DQ6 drag clamp, commit 14e6fef).** Pump-policy clamp in
  flair_live_do_drag (window.c byte-untouched): title band reachable below
  menu band 2 + IM 4px margin. Solid **leg G** (locked FLAIR_SOLID_CLAMP_SPEC,
  serial tooth `(260,120)->(-185,40)`) GREEN clean / RED on `no_drag_clamp`.
  Clip `solid_clamp.gif`: title band visibly pinned at y=40.
- **`haaq` CLOSED (DQ5 raise-on-title-click, commit f23ee36).**
  flair_app_dispatch gains a REAL inDrag arm: the O-5 switch (promotion +
  exactly one deactivate/activate pair) **without mouseDown delivery**. Solid
  **leg H** + `no_raise_on_title` mutant; test_process encodes the new
  contract (pre-change dispatcher RED on exactly the 4 new checks);
  test-flair-appswitch regression green. **THE GRADING CATCH (Rule 4):**
  codex's first draft synthesized a content-click through the dispatcher --
  which DELIVERS the click, so every background-title drag would have toggled
  the tenant's visible content marker. Root cause: the orchestrator's own
  brief over-constrained ("process.c untouched"). Corrected via
  `codex exec resume` with the constraint lifted; the rework is the
  single-spine fix.

## Frictions

- codex `danger-full-access` was denied by the permission classifier;
  `workspace-write` blocks Unix sockets entirely (no X, no QMP, no emulator).
  Division of labor that works: codex for pure code+host-oracle lanes
  (it delivered twice, honestly reporting what it could not run), Claude
  agents for anything driving emulators; orchestrator runs all emu proofs.
- `make -n test` "fails": recursive `$(MAKE)` sub-gates execute under -n and
  produce phantom REDs -- run the real gate, don't trust -n.
- system7-decomp has NO git remote; its sys8 specs commit (60c7a36) is
  local-only.

## Acceptance

Closing certificate ALL GREEN 321 host + 85 emu (clean build). Every closed
bead carries its proof in the close reason; every new gate mutation-proven;
all GUI changes have regenerable video clips + eyeballed frames.

## Pointers

- **POST-SCRIPT (same session): DEC-10 IS RATIFIED** (operator 2026-08-16,
  "lavender->teal is canon"; commit 6de3320; s97h CLOSED, all 9 OQs
  dispositioned in ADR Sec 6). The epic is fully unblocked: `3knt` -> `i3si`
  land together in one certificate (re-keyed oracles are RED until chrome.c
  renders Platinum). The cold-start brief lives in the HANDOFF's NEXT AGENT
  block. Era-independent Wave B lanes (b3hl DQ7, t1rv/7tjp DQ8) remain good
  codex fodder meanwhile.
- Ground truth: ../system7-decomp/specs/sys8/ + goldens/captures/s8_*.png
  (gitignored, regenerable via the mint; boot81.img + pristine backup exist).

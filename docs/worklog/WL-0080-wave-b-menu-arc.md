<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0080 — The Wave B Menu Arc: Band 2 Lives, the Cancel Restores, the Wart Dies

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Worklog Shard (session record)
**Session:** 2026-08-17/18 (orchestrated; codex exec gpt-5.6-sol xhigh lanes for
all code+host-oracle slices, orchestrator graded EVERY diff first-person and ran
ALL emulator/video proofs)

## Context

WL-0079 closed the Platinum landing with the era-independent Wave B menu arc as
the queued work (HANDOFF: `7tjp` -> `t1rv` -> `b3hl`, plus `j0vt` behind the
restore). The operator directive for this session: orchestrate, mostly serial,
codex lanes for the rottweiler coding work, and validate every GUI-visible
change with actual recorded video. Baseline re-verified first-person before any
dispatch: `make clean && make test` ALL GREEN 324 host + 85 emu (Rule 4).

Also this session (operator, direct): **the SAMIR graphical frontend is ruled
dBASE-Mac-SHAPED** — SAMIR BROWSE/forms in FLAIR windows riding the existing
Toolbox; the "enterprise office divider" ugliness delivered as content through
the already-ratified chimera surface (GDI-facade BTNFACE panels, win31
accents); a dBASE-for-Windows chrome family is REJECTED (post-period 1994;
win95ism guardrails). Recorded on bead `initech-e7qc`; staged behind the
`initech-586` text-mode gaps + `81ft` control chrome.

## What changed

Three serial slices, each: brief -> codex lane -> first-person diff grade ->
orchestrator-run emulator proofs -> recorded clip -> commit.

1. **`initech-7tjp` (commit `1483770`) — NOTES owns a distinct bar.** The
   two-Apple stacked-bars Law-4 violation is dead: NOTES's menubar is a
   SimpleText-flavored File/Edit/Notes fixture (spec'd in
   `flair_tenants_demo.h`, menuIDs 384-386, no Apple slot) instead of aliasing
   `bar_sys`. Oracle-first: new strictly-stronger POST-DISTINCT leg in
   `ppm_flair_appswitch_check` (post-switch band-1 vs band-2 intra-scene
   differential) + `KMAIN_MUT_NOTES_BAR_SYS` as the 5th appswitch mutant — the
   mutant IS the pre-change behavior, so its RED (0 differing px on exactly
   POST-DISTINCT) doubles as the RED-first proof. Bochs leg green.

2. **`initech-t1rv` (commit `45a8860`) — band 2 is ALIVE (DQ8).** The tenants
   pump gains the band-2 hit test `[SHELL_MENUBAR2_TOP, +H)` dispatching
   `ten_plist.head->menubar`; `flair_live_do_menu_at(y_top)` renders the panel
   through the make_offset_view idiom (menu geometry stays y=0-local, event
   coords translated per tick; the FO-7/8 single-bar pump untouched via a
   y_top=0 wrapper). LOCKED `FLAIR_SOLID_MENU2_SPEC` trace + solid leg E
   (independent-canon pixel probes below y=40) + the serial ROUTING tooth
   (`FLAIR-MENU-DROP menu=256` — post-7tjp the menuID ranges 128/256/384 make
   the serial marker a routing proof). TWO mutants, one per failure axis:
   `MENU2_DEAD` (the original y<20-only test; pixel-RED + no marker = the
   RED-first proof) and `MENU2_BAR_SYS` (panel drops but menu=128; the gate
   REQUIRES pixels green for this mutant = right-reason isolation).

3. **`initech-b3hl` + `initech-j0vt` (this commit) — the restore contract.**
   The deliberate demo-era wart (cancel leaves the panel; kmain comment said
   the oracle needed it) is gone, and so is every `shell_render` call in the
   menu path (the j0vt chrome-only tenant-content wipe). New
   `WindowMgr_invalidate_desktop` partitions compositor-layer damage among
   frontmost windows + bare desktop (`distribute_exposure` with no departing
   window); track-end (cancel AND selection) and mid-track cross-title erases
   both run the DQ2 spine (invalidate panel footprint -> paint_damage ->
   content phase -> present), with the band-2 overlay slice redrawn by
   DrawMenuBar when a band-1 panel covered it (menu-verb repaint, never
   whole-scene). New `FLAIR-MENU-XDROP menu=<id>` marker on cross-title
   redraw, emitted AFTER present. `MenuInfo_panel_rect` is locked to the
   drawer by a new host tooth: test_menu pixel-scans the complete painted
   extent below the bar and requires equality with the helper rect on all four
   edges (the restore footprint cannot drift). Solid leg D: TWO deterministic
   boots (no-input PRE, cancel-trace POST incl. a cross-title move), WHOLE
   FRAME byte-equality TOL=0 — simultaneously bites panel persistence and the
   shell_render content wipe. Mutants `MENU_NO_RESTORE` (the original wart =
   RED-first proof; Edit panel frame persists) and `MENU_RESTORE_SHELLRENDER`
   (content wiped to WDEF face) both RED on their exact axes.

**The oracles were de-warted (Law 2 — the artifact no longer bends for the
oracle):** the harness (`harness/emu/qemu.c`) now captures held-state markers
(`FLAIR-MENU-DROP`/`XDROP`) BETWEEN injected input events at their exact event
boundary, and a missing marker now means NO dump (strictly louder: the old
best-effort late dump could grade an unrelated frame as if it were the marker
state). test-flair-menu re-aimed to the DROP frame (held, no premature
hilite), crossdrag to the XDROP frame (the actual switch instant), solid leg E
to its DROP marker; selection/hilite proofs live in the serial teeth + host
test_menu.

## Traps caught (first-person grading + certificate)

- **The stricter no-dump-on-missing-marker rule broke two mutant/gate
  assumptions**, both fixed honestly rather than reverted:
  (1) `test-flair-appswitch-mutant`'s `ignore_refcon` kills the switch so
  `FLAIR-DISPATCH` never fires — under the new rule there is no dump at all.
  The mutant loop now distinguishes: missing dump + missing marker = mutant
  correctly RED; missing dump + marker present = harness fault (gate FAIL).
  (2) **`test-samir-boot` went RED in the first closing certificate**: its
  screendump run passes `--screendump-after SHELL-READY`, a marker already on
  serial BEFORE key injection starts, and relied (documented in its own
  comment) on the dump landing after all keys. The between-events capture
  dumped after key 1 — a pre-LIST frame. Fix: between-events capture is
  HELD-STATE-MARKERS-ONLY (`CaptureCtx.midtrack`); every other marker keeps
  end-of-injection semantics + the loud missing-marker rule. samir-boot,
  menu, and solid D/E re-proven green after the narrowing.
- `record-flair-repro` flaked ONCE on solid_switch (same-invocation captures
  differed, immediate re-run passed) — logged as live evidence on `zwo8`
  (icount hardening no longer hypothetical; also noted: the repro FAIL path
  overwrites the failing captures — evidence-preservation gap).
- Law-4 eyeball false alarm resolved by zooming: the band-2 File panel's
  "both rows black" look at 1x is the black top frame + black text; the zoom
  shows gray body + exactly one inverted hilite. The Photoshop menus' New/Open
  items are the standing placeholder fixture (kmain.c:971), not a regression.

## Acceptance

- Closing certificate: `make clean && make test` ALL GREEN — 324 host +
  emulator vector incl. the 3 Bochs legs (see the commit for the exact count;
  the solid gate is now 7 legs A/B/C/D/E/G/H with 8 mutants, appswitch 6 legs
  with 5 mutants).
- VIDEO (operator directive): `solid_menu2` + `solid_menucancel` join
  RECORD_SCRIPTS; clips recorded + frame-eyeballed (band-2 drop anatomy zoomed;
  cancel clip's final frame proven byte-identical to its boot frame). Fresh
  `solid_switch`/`appswitch` clips show the distinct File/Edit/Notes band-2
  after switch. All clips in `build/clips/`.

## Postscript (same sitting, 2026-08-18): `initech-chd4` — Route 1 lands

The DEC-10 OQ-7 funded follow-up landed as a fourth serial slice (commit
`d1ac218`): the era axis is DATA at the policy seam. `flair_look` gained a
pure-data PART->skin-slot offset map + `flair_look_pixel_for_skin`; chrome.c
threads `const flair_skin_t*` beside the GrafPort (documented macro wrappers,
geometry call sites untouched); desktop/dialog source the row via
`flair_look_default_skin()` — the registry's FIRST os/ callers, closing the
DEC-10 Sec 5.3 Law-2 smell. Look-neutral proven (host render byte-identical,
zero re-keys, exact-profile emu gates, clip eyeball); `FLAIR_MUT_SKIN_WRONG_ERA`
(SYS7 row into the live pointer) is the 15th fidelity mutant, RED on exactly
the five slot-fed legs. `spec/flair_skins.h` touched only to name the struct
tag (Rule 8 deliberate act; digest unchanged). Certificate: `make clean &&
make test` ALL GREEN 324 host + 85 emu.

Operational note (recorded as bd memory
`codex-exec-cross-session-kill-trap-2026-08`): cross-session forensics with the
rk-campaign-D orchestrator confirmed to the second that a fresh `codex exec`
dispatch SIGKILLs OTHER sessions' running codex children (~20-30 s after
attach; the newcomer is spared). Codex windows are now SERIALIZED between
sessions via SendMessage coordination; rk will ship a host-wide lockfile as
the durable fix — gate dispatches on it once it exists.

## Pointers

- Beads closed: `7tjp`, `t1rv`, `b3hl`, `j0vt`. Filed/updated: `e7qc` (dBASE-Mac
  ruling), `zwo8` (repro flake evidence).
- Next: `chd4` (Route 1 era-as-data, P2), `sjvq` (Platinum menu chrome — needs
  the operator's Law-4 call on re-skinning the chimera bar), `81ft` (Platinum
  control chrome), `zwo8`, or the North-Star B8 seed lane (`ogxv`).

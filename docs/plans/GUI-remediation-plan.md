<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# GUI Remediation Plan — "Sellable in the Initech Era"

**Date:** 2026-08-23 · **Status:** ACTIVE (operator directive, this date)
**Baseline:** commit `48de686`, certificate ALL GREEN 336 host + 90 emu.
**Companion research:** `docs/research/period-gui-suite.md` (the sellability
bar, minted 2026-08-23 from period sources).

## 0. Mandate (operator, verbatim intent)

> "The GUI is very disappointing. Lots of visible bugs still. Not upgraded to
> System 8. Basically nothing like a useful GUI OS — a buggy pair of windows
> with menus that don't work. Bring this GUI into a form that could have
> actually been sold for real in the Initech era. Half of the bugs can only
> be seen in motion/video. It should all be System 8 pixel-perfect windows.
> For bonus marks the apps are to be written in TPS Pascal."

Binding consequences:

1. **The bar is a 1996 store demo**, not the reference frame alone. A buyer
   boots the machine and expects: a pointer, desktop icons, a Trash, windows
   with titles, menus whose every item does something, and a bundled app
   suite. PRD §6.5's frame-only app list is NECESSARY but NOT SUFFICIENT.
2. **Fidelity target = Mac OS 8 Platinum, pixel-perfect**, graded against
   `../system7-decomp/specs/sys8/` + captures (DEC-10; sampled domain; teal
   accent substitution per OQ-2). Sys7-era leftovers in menus/controls are
   remediation targets, not heritage.
3. **Motion is a first-class oracle surface.** Every interaction slice ships
   a `record-flair` script + repro gate; the operator's clip eyeball is the
   Law-4 judge. Endpoint screendumps alone are insufficient evidence.
4. **Apps in TPS Pascal** is the bonus lane (Phase R5): a Toolbox trap
   interface + Pascal bindings, per ADR-0007's world (Pascal = the language
   of user programs).

## 1. Ground truth (first-person drive, 2026-08-23)

Fresh `make image` boot of `build/flair_tenants.img`, driven via
`harness/emu/drive_flair.py` + all 8 locked `record-flair` clips re-recorded
and frame-eyeballed. Findings, worst first:

| # | Finding | Evidence | Bead |
|---|---------|----------|------|
| G1 | **No mouse pointer exists.** No cursor is ever drawn; `grep cursor os/flair/` has zero render hits. A GUI without a visible pointer fails the 5-minute store demo instantly. | all frames | NEW `R0.1` |
| G2 | **No window ever shows a title.** Title bars are blank on both tenants, both states. | s01 + all clips | `vfd8` |
| G3 | **Menus are dead decoration.** Band-1 File drops a 2-item placeholder (`New/Open`) with no separators, no Cmd-equivalents, no dispatch (only About does anything). Band-2 items are the kmain.c:971 placeholder fixture. No menu item anywhere executes a command. | s06/s12 | NEW `R3.*` |
| G4 | **No desktop.** No volume icon, no Trash, no desktop pattern (flat teal), no icons at all; flagship boot logs `FLAIR-FAT-MOUNT-FAIL` (no slave data disk attached) so the desktop has no volume to show. | s01 serial | NEW `R3.*` |
| G5 | **No real apps.** HELLO/NOTES are in-kernel demo tenants painting colored squares. Nothing is launchable; the PRD §6.5 suite (InitechCalc grid, File Manager, InitechPaint, FILE COPY) does not exist as interactive apps on this desktop. | s01-s15 | NEW `R4.*` |
| G6 | Zoom box misroutes to drag; grow is dead (1×1 hit zone); real zoom/grow unimplemented. | s09/s10 | `tbef` `ci4o` `cjfr` |
| G7 | Close leaves the tenant process list stale (closed foreground app keeps keys); modal FILE COPY does not block input. | code-level | `8fhu` `zn61` |
| G8 | Chrome defects: drop shadow clipped away, separator notch doubled, zoom glyph wrong, bevel collapse at 8bpp, scrollbar column erased by content, top content row outside contRgn. | s01 zoom-ins | `9d0e` `6yzb` `jxsf` `l0ra` `hber` `javs` `l0mh` |
| G9 | Platinum is only skin-deep: the title band re-key landed (WL-0079) but menu bar/panels and all controls are pre-Platinum; the desktop runs 32bpp path, not the period 640×480×8 indexed mode. | s01 vs sys8 specs | `sjvq` `81ft` `zqy3` |
| G10 | Serial identity: every window op reports `win -1`. | all serials | `883x` |
| G11 | updateRgn-within-visible invariant unstated (background tenant can stomp foreground). | code-level | `wlzp` |

The WL-0076/WL-0080 solidity arc DID land its repaint contract (content
survives drag/close/switch; band-2 dispatches; menu cancel restores) — those
gates hold. What remains is everything a real OS is MADE of.

## 2. The sellability bar

`docs/research/period-gui-suite.md` enumerates the shipped functionality of
Mac OS 8 / System 7.5 (primary) and Win 3.1 (accent), with sources, and
ranks the 15 features whose absence most reads "not a real OS". The plan
below tracks that ranking. Headline floor (in rank order): visible pointer;
icons on a desktop + Trash; window titles; menus that pull down AND execute
with Cmd-equivalents; draggable/closable/zoomable/growable windows with live
scrollbars; double-click opens things; a file browser; Get Info; standard
Open/Save dialogs; an About box; a small app suite (text editor, calculator,
note pad); control panels (a credible minimum set); copy-progress dialog;
menu-bar clock; consistent alert/dialog conventions.

## 3. Phases

Ordering rule: each phase's oracle set must be green (host + emu + clip
repro) before the next phase's lanes dispatch; chrome-only phases may
overlap app phases when they touch disjoint files (ONE main-tree writer at a
time, always).

### R0 — The pointer and the floor (P0)

- **R0.1 CursorMgr**: 16×16 1-bit arrow + mask (sys8 pointer spec,
  hand-authored strike per the glyph rule), hotspot, save-under
  draw/erase on every pump mouse event, hidden during full repaints
  (ShieldCursor analogue at the present seam). Oracle: injected-position
  pixel probe (independent golden from the authored strike, test-clut
  pattern) + a motion clip where the pointer crosses both windows; mutants:
  NO_ERASE (trail) and HOTSPOT_OFF.
- **R0.2 Window titles**: Chicago strike title text, Platinum
  centered layout, active/inactive variants per
  `sys8/window-chrome.md`; wired into the WDEF for every window incl.
  dialogs. Golden-diff leg + TITLE_BLANK mutant. (`vfd8`)
- **R0.3 Chrome tail**: `9d0e` shadow band, `6yzb` notch, zoom cluster
  `l0ra`+`hber`+`jxsf`, scrollbar column `javs`+`l0mh`. All graded against
  sys8 goldens, pixel-exact.
- **R0.4** `883x` win-id serial identity (unblocks every later oracle).

### R1 — Interaction correctness (P0/P1)

- **R1.1** Hit-zone unification (`tbef` `ci4o` `cjfr`): FindWindow zones ==
  drawn chrome geometry, single source in chrome_metrics.
- **R1.2** Real zoom: user-state/standard-state toggle, region-correct
  exposure. Real grow: outline drag (period gray XOR outline), min/max,
  contRgn+scrollbar recompute. Real collapse (windowshade) — Platinum's
  third widget must DO something (sys8: collapse box).
- **R1.3** Period drag look: title-bar drag shows the gray outline and
  moves on release (authentic + cheaper than live drag); clamp retained.
- **R1.4** Close semantics (`8fhu`), true modality (`zn61`), update-clip
  fidelity (`wlzp`).
- **R1.5** Live scrollbars: Control Manager scrollbars attached to real
  windows — arrows/paging/thumb-drag scroll actual content (List Manager
  view as first client).
- **R1.6** Keyboard: Cmd-equivalents (PC Ctrl = Cmd) dispatched through the
  menu command table; menu flash-on-select.

### R2 — Platinum pixel-perfect surface (P1)

- **R2.1** `sjvq` menu bar + panels per `sys8/menus.md` (bevels, highlight,
  separators, Cmd-glyph column, panel shadow).
- **R2.2** `81ft` controls per `sys8/controls.md` (push button + default
  ring, checkbox, radio, popup, alert frame, progress).
- **R2.3** Platinum scrollbars per `sys8/scrollbars.md` (3 states).
- **R2.4** `zqy3` 640×480×8 VESA 0x101 indexed mode + DAC palette as the
  flagship present path (period-authentic).
- **R2.5** Whole-desktop golden: one locked composite screendump diffed
  pixel-exact per scene (the test-clut pattern, independent goldens from
  sys8 captures re-rendered), plus per-window crops. SSIM stays a guide.

### R3 — The Finder shell (P1) — codename STAPLER-DESK

- **R3.1** Flagship image gains a FAT12 data volume (mount OK at boot);
  desktop DB (icon positions) lives on it.
- **R3.2** DesktopMgr: desktop pattern (Platinum default, canon-graded),
  volume icon, Trash icon, icon grid, selection (click/shift/rubber-band),
  icon drag w/ outline, double-click opens.
- **R3.3** Disk windows: icon view over MILTON FAT enumeration, folder
  navigation, New Folder (Cmd-N), Clean Up, list view (by Name) later.
- **R3.4** File ops: drag-move within volume, drag-to-Trash + Empty Trash
  (confirm alert), Duplicate (Cmd-D), Get Info (Cmd-I), rename-in-place.
  Differential oracle vs mtools on the same image (the FAT rail exists).
- **R3.5** The real Finder menu bar: Apple (About This Computer + DAs),
  File/Edit/View/Special/Help with the canonical items + Cmd-equivalents,
  every item either functional or period-correctly disabled (grayed).
  Command dispatch table = data, not switch-in-kmain.
- **R3.6** Menu-bar clock (RTC via the INT-21 clock seam, right end).
- **R3.7** App launch: double-click an app icon → MILTON EXEC of a flat
  binary that registers as a FLAIR tenant (App Contract ADR-0013 over the
  disk-load path). DESIGN GATE D1: current tenants are compiled-in; the
  disk-launch tenant path is the load-bearing new mechanism of R3.

### R4 — The bundled suite (P1/P2)

Per the research floor; each app = FLAIR tenant using Platform Services
(Resource Mgr, Scrap, TextEdit/List, Standard File — all landed per 49ez):

- **R4.1** About This Computer (memory bars, version string).
- **R4.2** InitechText (SimpleText-alike: TextEdit + Standard File + Scrap;
  wire `ww9c` live clipboard as part of this).
- **R4.3** Calculator (period 4-function layout, keyboard + click parity).
- **R4.4** Note Pad DA + Scrapbook (Scrap-backed).
- **R4.5** Control panels (minimum credible set): Date & Time, Desktop
  Patterns, Mouse (tracking/double-click), Sound (beep), Memory (display).
- **R4.6** The frame apps land as real tenants: InitechCalc (ledger grid +
  116% pie), InitechPaint (Photoshop-exact bar carrier), FILE COPY dialog
  performing a REAL file copy at the michael_bolton.conf rate; File Manager
  = R3 disk window ("Drive A Files").
- **R4.7** SAMIR dBASE-Mac frontend (`e7qc`, ruled 2026-08-17) — staged
  after R4.2 proves the TextEdit/List host pattern.

### R5 — Bonus: TPS Pascal apps (P2, bonus mandate)

- **R5.1** DESIGN GATE D2 (ox-alpha seat + ADR-0007 amendment): the Toolbox
  trap interface — INT-based dispatcher (A-trap analogue) or fixed jump
  table exported at a locked address; WindowPtr/EventRecord as opaque
  handles/records over the TPS subset (records/var-params suffice; no
  pointer extension unless the design gate proves it unavoidable).
- **R5.2** `FLAIR.PAS` binding file (TPS include, subset-clean) + trap RTL
  stubs in the TPS runtime (start_dos lineage).
- **R5.3** One shipped app authored in TPS Pascal (Calculator or Note Pad),
  compiled by the seed AND by TPS on-OS (the resident-compile rail), ships
  on the flagship volume, launches by double-click. Three-way behavioral
  agreement where computable.

### R6 — Motion/video gating (cross-cutting, starts at R0)

- Every slice that changes an interaction ships/extends a `record-flair`
  script; `record-flair-repro` byte-identical is the gate; clips filed for
  the operator eyeball each wave. `zwo8` hardening folded in here.
- New scripts as functionality lands: cursor_cross, zoom_toggle, grow,
  collapse, scroll_drag, icon_dragdrop, trash_empty, app_launch, calc_use,
  text_edit, copy_progress.

## 4. Oracles & method (unchanged Laws, applied)

- Chrome/pixels: golden-diff vs independent sys8-derived goldens
  (sampled domain, teal substitution), never by-construction (HER-02).
- Behavior: serial-marker traces + locked injection scripts + host unit
  suites; FAT file ops differentially vs mtools; every golden
  mutation-proven (Rule 6).
- Motion: record-flair repro gates + operator clip eyeball (Law 4).
- Tri-emulator on phase closes (QEMU dev loop; Bochs in `make test`;
  86Box still `x0i`).

## 5. Working mode

- **codex exec gpt-5.6-sol xhigh**: all code+host-oracle lanes (sandbox
  blocks emulators). Serial slices, ONE main-tree writer; orchestrator
  grades every diff first-person and runs ALL emu legs + records clips.
- **ox-alpha (openrouter via pi, 1M ctx, one session at a time)**: the
  large-context seats — whole-FLAIR audits, design gates D1/D2, plan
  reviews. Writes reports/briefs only, never the main tree.
- **opus/sonnet subagents sparingly** (research, emulator-driving); no
  Fable subagents.
- Beads: epic + per-phase children; existing open beads absorbed as deps,
  not duplicated.

## 6. Milestone alignment

R0-R2 harden M4/M5 ground; R3-R4 BECOME the real M5 (Desktop apps + full
reference frame — the frame arrangement falls out of R4.6); R5 rides M7/M8
(TPS). The M5 oracle upgrades from "reproduce the frame" to "reproduce the
frame ON a desktop that passes the sellability floor."

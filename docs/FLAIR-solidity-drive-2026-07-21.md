<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# FLAIR Live-Desktop Solidity Drive — Evidence, Forensics, and Campaign Plan

**Date:** 2026-07-21 · **Epic:** `initech-av7s` (label `flair-solidity`)
**Baseline:** commit `bfac0d3`, certificate ALL GREEN 320 host + 83 emu.
**Mandate (operator):** the windowing system must be solid and dependable;
"a dozen little visual quirks and the sum total is a feeling of real mess."

## 1. Method

15 deterministic QEMU boots of `build/flair_tenants.img`, mouse/keyboard
injected via `qemu_harness` QMP after `FLAIR-LIVE-READY`, one marker-gated
screendump per scenario, every frame inspected first-person at 1x and 4-6x
zoom. Driver: `harness/emu/drive_flair.py` (dev aid, NOT an oracle; scenarios
are deterministic and reproducible — frames were not committed).
Operational gotcha: `--out` must be a SHORT path (the QMP unix socket
`sun_path` is 108-char capped; a deep scratch dir fails with exit=1, empty
log).

## 2. The three-painter synthesis (why it feels like a mess)

Three painters render the same windows with NO shared invariant; every
interaction leaves wreckage, so the desktop degrades monotonically while all
83 emu gates stay green:

- **P1 boot:** `shell_render` chrome (incl. unconditional scroll frame +
  WHITE content fill, `chrome.c:586,607-778`) then tenant content OVER it
  (erasing the scrollbar column — `javs`, `l0mh`).
- **P2 compositor:** `desktop_paint_damage` paints WDEF chrome ONLY, clipped
  to visible∩updateRgn with current `hilited`, then `WindowMgr_validate` for
  EVERY window unconditionally (`desktop.c:269-273`) — tenant-owed content
  damage destroyed before any updateEvt. `flair_route_updates` (capable,
  routes to background tenants too, `process.c:626-660`) is invoked ONLY in
  the foreground-switch block (`kmain.c:2406-2413`), never on inDrag/inGoAway
  (`kmain.c:1208,1236`), and even there AFTER the damage was cleared,
  re-seeding only head's CONTENT bbox.
- **P3 activation dispatch:** the tenant redraws content (fill+accents), but
  `reaffirm_active` seeds chrome repaint only on 1→0 (`window.c:243-246`);
  0→1 deliberately seeds NOTHING (`window.c:226-235`); `SelectWindow` seeds
  no raise-exposure (`window.c:428-442`).

The natural experiment that seals it (s15): dragging newly-activated NOTES
gives it full correct ACTIVE chrome (drag damage repaints chrome with
hilited=1) while its content goes blank WHITE even as the foreground app.

## 3. Scenario evidence (drive battery)

| # | Trace | Finding | Bead |
|---|---|---|---|
| s01 | boot | no titles; no shadow; zoom-box bevel reads desktop-teal + black inner ring; thick black separator notch right end both windows; no scroll frame (content erased it) | vfd8, 9d0e, l0ra/hber/jxsf, 6yzb, javs |
| s02 | switch→NOTES | band 2 = identical duplicate of band 1 incl. Apple; NOTES title stays flat-inactive though active; stale HELLO right-edge line crosses NOTES's exposed title band; HELLO grows a PARTIAL scrollbar | 7tjp, rqz5 |
| s03 | drag HELLO | window moves, vacated teal OK; full scroll frame appears that boot lacked (white-on-white hides content loss) | javs, gofc |
| s04 | drag NOTES (bg) | ghost drag: no raise/activate; content gray→WHITE at new position | haaq, gofc |
| s05 | close HELLO | exposed overlap = WHITE hole; NOTES title bar HALF active chrome (exposed strip) HALF flat | gofc, rqz5 |
| s06/s07 | band-2 press (both states) | NO menu drop ever — the ACTIVE APP's bar is dead decoration | t1rv |
| s08 | switch round-trip | HELLO active w/ flat inactive title; NOTES content wiped white | rqz5, gofc |
| s09 | zoom click | dispatches as `FLAIR-DRAG win -1 (60,60)->(60,60)`; press-and-move on zoom box DRAGS the window | tbef, ci4o |
| s10 | grow drag | nothing (1×1 hit zone) | cjfr |
| s11 | drag to (-135,-5) | no constraint; title bar under menu bars = window mouse-unrecoverable; another white hole | r8r7 |
| s12 | menu cancel | dropped panel + stale "About" highlight NEVER erased (deliberate demo wart, `kmain.c:1276-1285`) | b3hl |
| s13 | close+keys | harness injects keys before mouse (fixed order) — 8fhu confirmed at code level instead | 8fhu |
| s14 | double switch | NOTES center accent gone — content partially stale even on the activation path | gofc |
| s15 | switch→drag NOTES | active chrome appears via drag damage; content wiped white though foreground | gofc, rqz5 |

Serial-oracle integrity: every FLAIR-DRAG/FLAIR-CLOSE prints `win -1` in the
tenants scene (`kmain.c:1141-1150` scans only the hidden shell `wins[]`) — bead
`883x`.

## 4. Canon rulings pulled (Law 1)

- `../system7-decomp/specs/chrome/window-frame.md`: 1px frame; the title bar
  is its OWN FrameRect whose bottom edge coincides with the body top (single
  1px line — the notch is over-draw); documentProc carries a hard-edged drop
  shadow right+bottom outside the frame (struct = content grown 1px + shadow
  band, CalcDoc).
- `scrollbar.md`: inactive bar = frame + arrow boxes + empty #F3F3F3 track,
  #969696 separators; active = black separators + thumb + dithered track.
- FLAIR canon: windows DO carry chrome scroll frames (the frame still); the
  tenant content rect must EXCLUDE the 16px scrollbar column — the boot look
  is the wrong one.
- Chrome forensic: the drop shadow IS drawn (`chrome.c:854-865`) but writes
  1px outside strucRgn so `clip_in` discards every pixel — root of `9d0e`.
- Zoom-box "teal bleed": BEVEL_LIGHT == #8DDCDC is the RATIFIED lavender→teal
  canon; the visual wreck comes from BEVEL_SHADOW collapsing to black at 8bpp
  (`l0ra`) + the corner-fill bug (`hber`).

## 5. Oracle-first state (already built, RED-proven)

- `tools/ppm_flair_solid_check.c` — legs A (close-expose content), B (drag
  preserves content), C (activation chrome full-width + no stale band).
  Proven RED against the captured frames: A/B sample #FFFFFF where canon
  #C0C0C0 belongs; C reports 0 pinstripe rows + an 18-row stale black run at
  x=359, while its HELLO-flat sub-check passes (not blanket-failing).
- `spec/flair_solid_traces.mk` — LOCKED leg traces (Rule 11).
- Recipe draft (legs, markers, mutant names): see WL-0075 §Acceptance;
  mutants: `FLAIR_LIVE_MUTATE_NO_ROUTE_ON_CHROME` (A+B),
  `FLAIR_MUTATE_NO_ACTIVATE_INVAL` (C). NOT yet wired into `make test`
  (gates land WITH the fixes, green).

## 6. The design committee (STOPPED mid-run — resume next session)

A three-seat committee (2 design seats + adversarial critic) was launched on
DQ1-DQ10 and stopped before any seat completed (operator wrap-up). Resume:

    Workflow({scriptPath: "<session>/workflows/scripts/flair-solidity-committee-wf_cef190ac-1e7.js",
              resumeFromRunId: "wf_cef190ac-1e7"})

(or re-launch from the DQ list below — the script is a plain harness around
these questions; nothing else is in it).

**DQ1** validation ownership (proposal: `desktop_paint_damage` stops
validating; validation moves to the caller — `flair_route_updates` validates
all incl. unowned; non-tenant pumps call an explicit `desktop_validate_all`).
**DQ2** pump ordering: WM-op → paint_damage(chrome) → route(content,
validate) → present, on EVERY damaging dispatch; one composite helper.
**DQ3** activation/raise seeding: `reaffirm_active` seeds strucRgn bbox on
BOTH transitions; kmain's content-bbox seed becomes redundant. Blast radius:
`test_drag.c`/`test_process*`/`test_window` host contracts (update =
strengthen), O-7 suspend bit-identical invariant (deterministic both
captures — expected safe), `ppm_flair_appswitch_check` does NOT probe title
bands (verified — its header even records the old no-chrome-distinction
assumption as a NOTE, now stale).
**DQ4** close semantics for `8fhu` (hide+unlink+promote vs terminate vs park).
**DQ5** raise-on-title-click (`haaq`): title click on non-head window = same
switch path as content click, then drag.
**DQ6** drag clamp placement + bounds source (`r8r7`).
**DQ7** menu-cancel restore + re-aim `test-flair-menu` to dump while button
held (`b3hl`).
**DQ8** band-2 dispatch to `ten_plist.head->menubar` at `SHELL_MENUBAR2_TOP`
(`t1rv`), NOTES gets a distinct bar (`7tjp`, operator Law-4 eyeball).
**DQ9** gate shape: one `test-flair-solid` with legs + per-leg mutants.
**DQ10** non-goals this arc: real grow-resize, real zoom toggle, preemption,
per-tenant offscreens.

## 7. Implementation waves (planned)

- **Wave A (first-person, main tree):** the repaint contract — `gofc` +
  `rqz5` + `0zxp` narrow case + wire legs A/B/C + mutants. Key mechanical
  facts: `ten_plist` is a kmain local (`kmain.c:1996`); `flair_live_ctx_t`
  (`kmain.c:806-820`) needs a `FlairProcessList *plist` member (NULL in
  non-tenant scenes) so `do_drag`/`do_close` can route-or-validate.
- **Wave B (worktree lanes, disjoint):** B1 chrome.c (`6yzb` notch, `9d0e`
  shadow band, zoom cluster `l0ra`/`hber`/`jxsf`); B2 ref_tenant.c (`javs`,
  `l0mh`, `vfd8` titles); B3 kmain interaction (`t1rv`+`7tjp`+`b3hl`+`3pl4`;
  `r8r7` clamp; `883x` win-id; `tbef`+`ci4o` variant/hit zones; `8fhu`
  close). B3 overlaps kmain.c — single lane or sequenced after Wave A.
- **Wave C:** gate assembly + full aggregate + Bochs legs + WL shard + push.

## 8. Beads

Epic `initech-av7s` (17 children): new this session `gofc` `rqz5` `r8r7`
`b3hl` `883x` `6yzb` `tbef` + epic; annotated with roots: `0zxp` `t1rv`
`7tjp`(P3→P2) `8fhu` `9d0e` `vfd8`; adopted: `javs` `l0mh` `ci4o` `8kx3`.
Also relevant, not in epic: `haaq` `cjfr` `zn61` `ekde` `3pl4` `ek9g` `zw9c`
`iqhh` `36pm` `hq7l` `srd1` `lwh4` `v513` `l9xt` `359n` `ml5c` `eppk`.

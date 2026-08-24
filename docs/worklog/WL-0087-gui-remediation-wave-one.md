<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0087 — The GUI Remediation Opens: R0+R1+R2 Land, the Kernel Moves High

**Date:** 2026-08-23/24 (one orchestrated overnight session) · **Epic:** `initech-tdnl`
**Commits:** `b8f06d8..74ae642` (15) · **Certificates:** every slice under a full
green vector; final `make clean && make test` class certificate **341 host + 99 emu**
(from 336+90 at session open). All pushed.

## Context

Operator directive (2026-08-23, verbatim intent): the GUI is a buggy pair of
windows with dead menus — nothing like a sellable OS. Update the plan from
ground truth, research the period functionality suite, and run a remediation
programme to a form "that could have actually been sold in the Initech era."
Riders: half the bugs only show in motion (video-gate everything); the bar is
pixel-perfect System 8; bonus marks for apps written in TPS Pascal. Heavy
delegation authorized: codex exec gpt-5.6-sol xhigh + the openrouter
stealth/ox-alpha 1M-context seat; opus/sonnet sparingly; no Fable subagents.

## Ground truth (first-person drive, session open)

Fresh boot + all 8 locked clips re-recorded and frame-eyeballed: NO mouse
pointer exists anywhere (zero cursor code in FLAIR); no window ever shows a
title; menus drop a 2-item placeholder with no dispatch; no desktop
(FLAIR-FAT-MOUNT-FAIL every boot — no data disk attached); no launchable
apps; zoom misroutes to drag, grow dead (1x1 zone); Platinum only on the
title band. Catalogued as G1-G11 in the plan.

## What changed

**The plan + research + design corpus (committed):**
- `docs/plans/GUI-remediation-plan.md` — phases R0-R6; epic `initech-tdnl`
  (24 new children + 20 absorbed beads; now 30+ children).
- `docs/research/period-gui-suite.md` — the sourced sellability bar (sonnet
  web-research seat): Mac OS 8 menus item-by-item, the DA/control-panel
  suite, the ranked 15-item "not a real OS" floor.
- Four ox-alpha 1M-seat design docs, orchestrator-reviewed: D1+D2+D3
  (INT-0x81 Toolbox Gate ruling; zero-subset-growth TPS Pascal binding;
  63-row Platinum pixel-gap audit), the R3 Finder shell (the Finder is an
  always-resident compiled-in App Contract tenant; 3-seam icon underlay;
  DESKTOP.DB + \TRASH staging; data-driven command table; 9-slice plan),
  the ADR reconciliation (TWO ratified-text contradictions caught pre-code:
  the park/resume WNE loop violates ADR-0013 Sec 3.4 → push-callback; the
  launch watchdog violates BC-4 → descoped), and the kernel runway
  relocation (K1-K4).

**Landed code slices (each: codex lane → orchestrator first-person grade →
emu legs + clips → full vector → commit):**
1. **R0.1 THE POINTER EXISTS** — CursorMgr on the final LFB (save-under,
   nestable shield at the one present seam, first-activity latch preserving
   every boot golden). Rule-8 amendment `tdnl.25`: the arrow strike v2
   (white dilation) adopted INTO locked cursors.h. The PARK CONVENTION born
   (leg H read the arrow's outline in its stripe probes).
2. **R0.2/R0.4 WINDOWS HAVE NAMES** — Platinum title text (active knockout /
   inactive dim, sampled values), SetWTitle as the one D2-3 seam; real
   win-ids in every serial marker.
3. **R0.3 the chrome tail x7** — shadow visible, notch single, zoom glyph
   right, bevel/corner fixed, scrollbar column excluded from content; D3
   VERIFY rows discharged.
4. **R1.1-R1.3 THE WIDGETS DO THINGS** — unified hit zones, real zoom
   (user/standard toggle), real grow (outline + min clamp), real collapse
   (windowshade), period release-commit outline drag; 3 new emu gates.
5. **R2.1 THE MENUS ARE PLATINUM** — bar 3-D profile + rounded corners,
   teal pulled-title hilite, panel bevels + drop shadow, cmd-key column,
   etched separators, disabled ink; 9 menu mutants.
6. **R2.3 Platinum scrollbars** — 5-value wells, raised arrow tiles, black
   active separators, 15px teal accent thumb + disabled state (Control Mgr);
   window-gutter thumbs honestly deferred to the R1.5 scroll model.
7. **R2.2 Platinum controls** — checkbox tiles + shaded X, button bevels +
   corner smoothing, THE DEFAULT RING wired to defaultItem, dialog inset
   faces. Rule-8 STOP honored on the alert pinks (nominal-only in the CLUT;
   sampled rows proposed, follow-up `81ft.4`).
8. **R1.4 CLOSE TERMINATES, MODALS BLOCK, UPDATES CLIP** — owner-identity
   close disposition (anti-8fhu, per ratified ADR-0013 3.4), true modality
   at the dispatch seam (+ the movable modal drags itself), BeginUpdate-
   fidelity visRgn clip (stomp case proven RED-first). GATE ARCHAEOLOGY:
   both FO-9 flair_live gates had zn61's bug baked into their locked traces
   (dragging windows behind/across the modal) — re-keyed to the modal-drag
   proof + the reverse survival oracle; menu gates relocated to a new
   no-modal image (period ruling: menus are blocked while a modal is up).
9. **R3.1 THE DESKTOP HAS A DISK** — flair_data.img primary slave on every
   tenants boot; DESKTOP.DB + \TRASH created/persistent/regenerate-loud,
   mtools-differential; the fat12 -15 error collision fixed. **THE BOCHS
   CATCH:** the new code PANIC vec=06'd under Bochs (cpu model=pentium)
   while QEMU passed — the freestanding artifact had compiled at HOST
   default arch (cmov everywhere) since forever, silently violating
   ADR-0001's 386+ target. `-march=i386` pinned (objdump-verified clean);
   lesson + the stale-objects-on-CFLAGS-change trap in bd memory
   `march-i386-pin-2026-08`.
10. **tdnl.29 THE KERNEL MOVES HIGH** — 0x500000 residence via a single
    post-PE rep movsd from the vacated conventional bounce; program window
    [0x40000,0x80000) byte-identical (mutation-proven floor guards); KHI/KAT
    placement legs green under QEMU AND Bochs; ~0.9MiB runway. Closes
    5dr8/yrzo (the shell kernel had already been saved by delinking the
    dead FLAIR objects it never referenced — nm-verified, ~60KiB back).

## Frictions / lessons

- ox-alpha via pi: tool-use mode returns empty against this endpoint — run
  it TOOL-LESS with the corpus inlined in the prompt (@file); ~50% of long
  runs time out (retry once; then fall back to codex).
- Lane blind spot: codex lanes twice missed cross-cutting gate impact (the
  modal-scene FO-9 gates; the R3.1 Bochs arch trap). The full first-person
  vector after EVERY lane is what catches these — keep it non-negotiable.
- test_keep is a rare ASLR-draw flake (`initech-ahp2` w/ forensics): a lone
  in-vector RED there gets a standalone re-run before blaming a diff.
- Codex resume (`codex exec resume --last`) works well for orchestrator-
  directed rework within a lane's context (used twice for the gate re-keys).

## Open / next

- **Running at close of this shard:** the R3.2 DesktopMgr lane (icons,
  Trash, selection, marquee, drag, command-table spine, the Finder tenant).
- Next per the Finder plan order: tdnl.10 disk windows → tdnl.12 the real
  Finder bar (the deliberate band-2-at-rest re-key + operator Law-4 clip) →
  tdnl.26 move primitive → tdnl.11 file ops → tdnl.13 clock → tdnl.14 app
  launch (per the reconciled AC-3 DEC blocks) → R4 apps → R5 TPS Pascal.
- Operator rulings pending (flagged, non-blocking): rainbow vs mono Apple,
  Charcoal vs Chicago, the Finder-bar-at-rest clip sign-off, alert-pink
  canon accretion (`81ft.4`), ADR ratification of the AC-3/DEC-09/10 blocks
  + the relocation DEC (drafts all committed under docs/design/).
- Follow-ups filed: `xu97` (high kernel stack), `tdnl.30` (TRASH hidden
  attr), `ahp2` (test_keep flake), `tdnl.26/27` in the R3 order.

## Acceptance

Every slice: RED-first or golden-diff oracles + named Rule-6 mutants proven
RED; emu legs + clips run first-person by the orchestrator; full vector
green before every commit; three structural finds (dead-weight linking, the
non-386 codegen, the modal-poisoned gates) fixed at root, not papered.

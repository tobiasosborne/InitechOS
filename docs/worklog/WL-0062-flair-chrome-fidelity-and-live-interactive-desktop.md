<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0062 -- FLAIR window-chrome FIDELITY arc + the LIVE INTERACTIVE desktop (5l5z)

**Type:** orchestrated multi-lane session (one orchestrator, delegated coding
subagents, independent grading, committee for the strategic fork).
**Date:** 2026-06-25 -> 2026-06-26.
**Beads:** epics `initech-qipc` (re-flair) + `initech-hmll` (chrome fidelity, CLOSED)
+ `initech-5l5z` (live event loop, core delivered). 13 commits
`93e8cd4..7dcc4e1`, all pushed to origin/command-com-default.

## Context

Operator directive: orchestrate the next most consequential FLAIR work --
delegate each coding step to a subagent, orchestrator owns grading / integration
/ plan-following / beads, convene the committee for serious decisions, keep
working. Entry state: the static FLAIR chimera desktop rendered live (re30.3) but
(a) the windows "did not look like System 7" and only 2 of N chrome-fidelity
elements were graded, and (b) the desktop was inert -- `kmain` rendered once and
`cli;hlt`'d (HER-14: a WaitNextEvent spine ratified but dead on metal).

## The strategic fork (committee)

Two arcs competed: window-chrome FIDELITY (`hmll`, P1, oracle infra built) vs the
live EVENT LOOP (`5l5z`, P2, the P0 epic finale, ADR-0006 ratified but ZERO code
landed). A 2-opus committee (Seat A product/Law-4, Seat B risk/sequencing)
deliberated independently and CONVERGED: do the contained chrome elements first
(proven non-by-construction oracle, certain), ratify the 5l5z design forks by
convergence, then build the event loop. The forks were ratified WITHOUT a separate
round (both seats independently chose the same resolutions): pit/kbd/mouse =
FUNCTION-POINTER HOOKS (byte-stable across the 27 kernels, not weak symbols / not
per-config objs); `-DBOOT_FLAIR_LIVE` = a THIRD demo kernel (default boot stays
static, Rule 11); IRQ ordering tick -> kbd -> mouse-LAST (a mouse bug cannot
masquerade as a loop bug), mouse Bochs-gated (the minefield).

## What changed

**Chrome FIDELITY arc (`hmll`, CLOSED) -- all graded vs the INDEPENDENT
`../system7-decomp` goldens via `test-chrome-fidelity` (NOT by-construction):**
- `54nw` drop shadow (1px L @ offset (1,1)) + title-bar-only groove. The drawer is
  correct (host oracle green); the LIVE shadow is clipped by `strucRgn` (the
  compositor clips chrome to the window bounds; `frame = bbox(strucRgn)`) -> filed
  `initech-9d0e`. (Commits `93e8cd4` + the `384bd34` fix-forward.)
- `ts3t` close/zoom box 11x11 + close +9 / zoom -20 offsets + 3-D double bevel +
  zoom nested-square glyph. Orchestrator corrected the bevel highlight
  PIN_LIGHT -> BEVEL_LIGHT (canon teal #8DDCDC, idx2 -- the WL-0053 lavender->teal).
- `jh7m` scrollbar black-outer/gray-inner separators + #969696 arrow glyphs + solid
  #F3F3F3 track (was BTNFACE).
- `92li` title-bar bevel rows + exactly-15-row interior recomposition (bevel-hi
  BEVEL_LIGHT + 15 phase-locked stripe + bevel-lo BEVEL_SHADOW + shared frame);
  content_top top+20 -> top+19. Forced `KERNEL_SECTORS` 320 -> 352 (cumulative
  chrome growth crossed the image window; Rule-5 boot-geometry, both Bochs legs).

**Live INTERACTIVE desktop (`5l5z`) -- the booted 386 is now interactive:**
- FO-4 (`2483d07`) PIT tick function-pointer hook + the `-DBOOT_FLAIR_LIVE` kernel.
- FO-5 (`cf48800`) kbd IRQ1 raw-scancode hook (+ FO-9 HOST oracle test-interact,
  21 checks, independent-by-recomputation of FindWindow/DragWindow/visible-region).
- FO-6 (`3835b02`) mouse IRQ12 -- THE minefield: dual-PIC EOI (slave 0xA0 THEN
  master 0x20), cascade+IRQ12 unmask, 8042 aux init, weak-extern `irq12_entry`
  (26 non-flair kernels inert/byte-stable). No-wedge proven (>=2 distinct
  FLAIR-MOUSE; the master-only-EOI mutant wedges).
- FO-7/8 (`b64c986`) the WaitNextEvent cooperative pump replacing render-once-HLT:
  cursor tracking + FindWindow dispatch -> inDrag DragWindow + D-5
  desktop_paint_damage + present (FLAIR-DRAG); inGoAway close. The EMU oracle
  test-flair-drag grades the shifted chrome + vacated-teal vs the independent canon;
  the drag-noop mutant catches HER-14.
- FO-8b (`7dcc4e1`) live pull-down menus: inMenuBar -> drop panel -> track ->
  MenuSelect (FLAIR-MENU sel=0x00800002); test-flair-menu + menu-noop mutant.

## Why

Law 4 is the product: a person who used early-90s Mac+DOS software says "yes,
that's it" to the LIVE, draggable arrangement with WORKING MENUS. The chrome arc
makes the windows look right; the event loop makes them WORK. Together they convert
FLAIR from "renders the frame" to "IS the frame, live." Every element graded
against an INDEPENDENT golden/recomputation (Law 2), never by construction.

## Frictions / lessons (the orchestrator grading caught every one)

- **NEVER trust the subagent report; always `make clean && make test` + Bochs.**
  Caught: `54nw` host-green but EMU-RED (a premature ppm assertion -- the live shadow
  is strucRgn-clipped; root-caused, not patched); `ts3t` non-canon bevel COLOR (the
  mechanism-only fidelity oracle can't see policy); the cumulative kernel-size
  crossing `KERNEL_SECTORS`; the FO-5 keyboard TIMING RACE (a 10-tick/~100ms loop
  window vs the ~100-300ms `--keys-after` inject latency -> widened to a 150-tick
  deterministic window); a missing `TEST_EMU_GATES` wiring in a subagent patch; and
  the PRE-EXISTING ASLR-flaky `test-arena-disjoint` (mmap(NULL,MAP_32BIT) -- filed,
  unrelated to any lane, re-run green).
- **Subagents must NOT commit/push** (one did, leaving an EMU-RED commit on origin
  that needed a fix-forward). The orchestrator owns grading + commit-per-step.
- **The function-pointer-hook pattern** keeps a shared object (pit.o/kbd.o/isr.o)
  byte-stable across all 27 kernels while only the flair-live kernel wires it.
- **Async-inject gates need a generous deterministic window**, bounded by tick
  COUNT (not wall-clock), so the harness round-trip lands inside it (Rule 11).
- **Bochs constraint:** the flair_live kernel ENOMODEs 640x480 and halts at the
  fail-loud guard BEFORE the pump, so the interactive lanes are graded on QEMU
  (multi-event no-wedge + triple_fault detection) and test-flair-live-bochs is the
  Bochs boot-accuracy check; a strict-PIC IRQ12 probe is filed (`initech-04ae`).

## Acceptance

- `make clean && make test` = **ALL GREEN 273 host + 53 emu** (entry: 271 + 43).
- Both Bochs legs PASS; `test-kernel-repro` green every step (the 26 non-flair
  kernels byte-identical -- all live changes isolated to the flair_live kernel).
- `_kernel_end(flair_live) = 0x37174 < 0x40000`.
- Live screendumps independently probed: the teal title bevel + 15-row pinstripe
  composition; a window dragged (300,120)->(340,150) with the vacated area teal;
  the File menu dropped + Quit selected.
- 13 commits pushed; `git status` up to date with origin.

## Pointers

- Chrome: `os/flair/chrome.c`, `spec/chrome_fidelity_golden.h`,
  `harness/proptest/test_chrome_fidelity.c`; goldens `../system7-decomp/specs/chrome/*`.
- Event loop: `os/milton/{pit,kbd,mouse,kmain}.c`, `os/boot`-adjacent `os/milton/isr.asm`,
  `os/flair/{event,window,desktop,menu}.c`; oracles `harness/proptest/test_interact.c`,
  `tools/ppm_flair_drag_check.c`, `tools/ppm_flair_menu_check.c`.
- Governing: ADR-0006 (live event loop), ADR-0004 D-3/D-5/D-6, the re-flair epic
  `bd show initech-qipc`.
- Filed follow-ons: `9d0e` (live drop-shadow), `04ae` (Bochs mouse probe), the menu
  close-restore + CombineRgn (rmsr), the mouse-ACK drain, the arena ASLR determinism.

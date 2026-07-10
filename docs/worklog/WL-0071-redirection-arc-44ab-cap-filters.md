<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0071 — Redirection arc complete + 44ab cap decision + FILE COPY fidelity + filters

**Session:** 2026-07-10/11 (continuation of WL-0070's orchestrated session:
subagent lanes graded and re-verified first-person on main before every
acceptance — Rule 4; committee process for the locked-spec change)
**Commits:** `79f9b2a` (7s1z) → `e1ceb0e` (rl4v) → `78ad8a4` (bsy.7) →
`c6a58b9` (tf3c) → `aaea2c7` (zvo6+a90f) → `e639195` (bsy.8) →
`4835184` (44ab) → `fb5449c` (m0dc)
**Certificate:** closing `make clean && make test` — see the final line of
this shard (recorded post-run).

## What changed

- **The MILTON redirection arc is COMPLETE** (epic `bsy` big step): `bsy.7`
  `<` input redirect (two-pass parser, restore-handle-0-on-every-path,
  DOS 3.3 file-not-found semantics, GOBBLE.COM consumer), `bsy.8` `|` as
  the authentic temp-file pipe (PIPEn.$$$, no exit-code short-circuit,
  temps deleted on every path, 8-stage bound), umbrella `hsct` closed.
  `>` `>>` `<` `|` all work in COMMAND.COM, every leg mutation-proven at
  host AND booted-QEMU level.
- **`m0dc` filters:** SORT (/R, case-insensitive collation), FIND
  (/V /C /N /I), MORE (AH=44h IOCTL device fork: CON pager / file
  passthrough — the authentic behavior and what makes it emu-testable).
  Six emu gates + three mutant legs.
- **`44ab` — deliberate Rule-8 locked-spec change, committee-ratified:**
  `RGN_ROW_X_MAX` 256→640 (= the ADR-0004 OD-3 domain bound; the per-band
  merge output is provably domain-limited because xmerge collapses
  coincident edges — now written into Sec-5 as the two-bounds passage),
  `RGN_X_POOL_CAP` 1024→2048, ADR-0005 formally amended. Process: 3-lens
  advisor panel (unanimous) → implementation tripwire #1 (stack audit:
  18 KiB working set vs 64 KiB shared kernel stack) → adversarial
  verification of the statics resolution (caught: kstart.asm never zeroes
  .bss — a naive guard flag would have prevented the desktop from ever
  rendering) → tripwire #2 (the statics busted kernel_shell's
  `_kernel_end < PROGRAM_BASE` guard by 8,340 B) → final dual-mode
  design: hosted keeps static slots, freestanding binds two FLAIR-heap
  blocks (`region_engine_bind_ws`, boot-time fail-loud OOM, unbound-
  pointer fail-loud). New oracle legs: 640px-wide pixel-exact
  homomorphism, at-cap round-trips, guard-leak and nested-acquire
  probes. test-region now 65 checks; all 4 mutants bite at the new cap;
  QEMU + all three Bochs boot legs green (86Box remains `x0i`).
- **Law 4:** `zvo6`+`a90f` — FILE COPY is now the reference's moveable
  titled modal (movableDBoxProc = the missing MTE Table 4-1 row, drawn by
  the SAME title-band code as document windows) with the canon 68%
  progress fill (cited 65–70 range midpoint, static-assert-locked).
  `rl4v` — cross-menu drag works (per-point MenuBar re-hit per IM
  MenuSelect); sibling live-redraw bug filed.
- **Factory/onboarding:** `7s1z` — `make image` / `make run` /
  `make run-bochs` are real (flagship flair_tenants desktop;
  `-S` dropped from default run with rationale). `tf3c` — seed has
  repeatable mutant gates; negative div/mod proven a coverage gap, not a
  bug. `0k2d` — all 6 xbase prog-diff goldens now mutation-proven via
  engine perturbations (previously only exact.out, and its mutant
  perturbed the grader, not the engine).

## Frictions / lessons

- The stale-worktree trap (`ck22`, WL-0070) recurred in 3 more lanes;
  fresh HEAD worktrees minted manually (`git worktree add ... HEAD`)
  avoided it for the rest of the session. Addendum recorded: nested
  worktrees also break `../sister-repo` relative paths.
- Committee tripwires earn their keep: two escalations on `44ab` were
  both real (kernel stack; kernel image budget) and both would have been
  silent-ish failures if landed naively. The `_kernel_end` margin is now
  ~8.6 KiB on the largest kernel — the structural runway problem is
  filed (`PROGRAM_BASE` relocation decision looming).
- Integration re-measures: the lane measured a 10,084 B margin; main
  (carrying bsy.8's growth) measured 8,644 B. Numbers from a lane's
  worktree are evidence about the LANE's tree, not about main.

## Beads

Closed this session (WL-0070+WL-0071 arc): `6vr1 mtqw ljck gymo c921
xi7x msol bsy.9 0k2d 7s1z rl4v bsy.7 tf3c zvo6 a90f bsy.8 hsct 44ab
m0dc` + audit epic Phase 1. Filed: `g0bm ck22 mpi7 mtqw` (closed same
session), prog-diff corpus gap, kmain live-redraw menu bug, kernel
runway. Epic `croo`: Phase 1 complete, Phase 2 substantially complete
(`xi7x 0k2d tf3c` done; `quke in2g` remain), Phase 3 GUI/MILTON arcs
well underway.

## Acceptance

Closing gate: `make clean && make test` — **ALL GREEN, 306 host + 78 emu
gates** (from 296+61 at the WL-0069 audit — +10 host, +17 emu across
WL-0070/71; the count reconciles gate-by-gate: Phase-1 exit 303+68, then
seed mutants +2 host, prog-diff mutants +1 host, bsy.7 +2 emu, bsy.8
+2 emu, filters +6 emu. Every added gate is a real, mutation-proven
oracle).

## Pointers

- Next per plan: `in2g` (promote Bochs boot leg into `make test`),
  `quke` (softfp/ansi oracle audit), `h5vg` (.ndx B-tree ceiling),
  `hv7u`/`jh7m` (chrome/scrollbar fidelity), kmain live-redraw menu bug,
  `x0i` (86Box driver, P1), kernel runway bead when next triggered.
- The committee record for the 44ab change lives in the bead's DESIGN +
  NOTES fields (3 rounds, all riders, all verification evidence).

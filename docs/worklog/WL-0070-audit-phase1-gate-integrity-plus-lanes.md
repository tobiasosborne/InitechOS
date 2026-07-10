<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0070 — Audit Phase 1 complete (gate integrity) + parallel lanes (xi7x, msol, bsy.9)

**Session:** 2026-07-10 (orchestrated: 6 subagent lanes, all output graded and
re-verified first-person on main before acceptance — Rule 4)
**Commits:** `c1f7e36` (bsy.9) → `d362882` (gymo) → `103bb68` (xi7x) →
`09342fa` (msol) → `e4bdfa1` (Makefile gate integrity + wiring)
**Certificate:** `make clean && make test` **ALL GREEN — 303 host + 68 emu**
(was 296+61 at WL-0069). Counts reconcile exactly: +5 host ljck orphans,
+2 host stdfile, +5 emu ljck/mtqw, +2 emu bsy9-redir.

## Context

WL-0069 ratified the 4-phase improvement plan (epic `initech-croo`).
Operator directive: orchestrate the next most consequential work,
delegating coding to subagents. Phase 1 ("restore gate integrity, do
first") was the target; independent-file lanes from Phases 2/3 and the
docs track ran in parallel worktrees.

## What changed

- **`initech-6vr1` (P1, closed):** 8 `KERNEL_*_OBJ` rules (the audited 7 +
  `KERNEL_WINDOW_MUT_OVERLAY_OBJ` found by full-file sweep) used
  later-defined variables in prereq lists → parse-time EMPTY → stale
  incremental kernels. Literal paths now; misleading ~7100 comment
  rewritten. Proven: touch → rebuild → idempotent.
- **`initech-ljck` (P1, closed) + `initech-mtqw` (P1, filed+closed):** all
  10 orphaned mutation-proven gates folded into the aggregate lists. Two
  of them (`test-mcb-emu-mutant`, `test-int21-irqstorm-mutant`) had NEVER
  been runnable — their mutant kernels' object lists were missing
  devices.o (+mcb.o), a Law-2 finding on top of a Law-2 finding. Fixed to
  match non-mutant siblings; both bite RED now.
- **`initech-gymo` (P1, closed):** the finished stdfile WIP landed —
  `test-stdfile` 29/29 + 3 mutants wired per the test-list idiom.
  Follow-up filed: `initech-mpi7` (doOpen swallows navigate errors).
- **`initech-c921` (P3, closed):** stale worktree pruned (its 4 orphan
  files byte-identical to tracked copies); `.gitignore` agent-runtime
  block committed.
- **`initech-xi7x` (P2, closed, Phase 2):** region generators now cover
  negative + near-INT16 bands via a window-translated oracle, high-n
  stress, bidirectional shrinker. Proven with a sign-dependent
  `region_intersects` mutant the old domain provably missed.
- **`initech-msol` (P3, closed):** 6 stale-prose sites rewritten (each
  fact re-verified first).
- **`initech-bsy.9` (P2, closed, Phase 3 arc opener):** child PSP inherits
  the parent JFT on EXEC (all 20 handles), `sft_inherit` refcount symmetry
  with `sft_close_process`, RED-first host oracles + `test-bsy9-redir`
  emu gate (GREETINGS exactly twice through TYPE). External `>` redirect
  works; unblocks `bsy.7` (`<`) → `bsy.8` (`|`) → `hsct`.

## Frictions / lessons

- **Stale worktree trap (`initech-ck22`, P2 + `bd remember`):** agent
  worktrees spawned from `5bc5104` (WL-0066, Jun 20), 26 commits behind
  HEAD. Every worktree lane's diff was integrated via `git apply -3` and
  ALL its oracles re-run first-person on main before acceptance; msol had
  a real conflict (a9iq paragraph), xi7x's verification was invalid until
  re-proven against the newer engine. Check `git log -1` in any lane
  worktree before trusting it.
- `git apply` is atomic: the bsy.9 patch had to be split
  (`--exclude=Makefile`, then a staged-index 3-way for the Makefile part).
- New beads this session: `g0bm` (.PHONY prereq pattern, P3), `mtqw`
  (closed), `ck22` (trap), `mpi7` (stdfile fail-loud).

## Acceptance

Full clean gate ALL GREEN 303 host + 68 emu (log:
session scratchpad `phase1_exit_gate.log`). Every closed bead's close
reason records its specific oracle evidence.

## Pointers

- Next per plan: Phase 2 — `0k2d` (in flight this session), `tf3c`,
  `quke`, `in2g`; Phase 1 leftover `7s1z` (real `make image`/`run`);
  Phase 3 GUI tail + `bsy.7`.
- Epic `initech-croo` reopened after an over-eager close; Phase-1
  certificate lives in its notes.

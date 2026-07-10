<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->

# WL-0069 — The Whole-Repository Audit + Improvement Plan

**Date:** 2026-07-09 / 2026-07-10
**Operator directive:** fan out subagents to understand the entire project;
completely review it; create an improvement plan; update beads if stale or
missing.

## Context

First full-programme audit since the FLAIR re-ratification (WL-0054). The
repo stood at WL-0068 (SAMIR P1 cluster cleared, 296 host + 61 emu green
claimed) with three untracked `stdfile` files on `command-com-default`.

## What was done

1. **Verified the certificate first-person:** `make clean && make test` =
   ALL GREEN, 296 host + 61 emu (2026-07-09). The WL-0068 claim is honest.
2. **12-lane parallel survey** (boot/kernel, milton-dos, flair, atkinson,
   samir, seed/tps, harness-oracles, spec/fixtures, worklog/handoff, beads,
   build-repro; docs lane failed structured-output and was covered
   first-person) + an opus completeness critic over the composed result
   (workflow `wf_61baffe8-1d5`, ~1.4M subagent tokens, 482 tool uses).
3. **First-person re-verification of every load-bearing claim** (Rule 4):
   the Makefile prereq bug via `make -rpn`; the orphaned-gate list via
   make-expanded `TEST_UNIT_GATES`/`TEST_EMU_GATES` (357 words); the
   `stdfile` WIP compiled + run (29/29 green, 3/3 mutants bite, freestanding
   typecheck OK); bead statuses via live `bd show` after the dolt lock
   cleared.
4. **Authored the plan:** `docs/plans/AUDIT-2026-07-09-improvement-plan.md`
   (4 phases + docs track), tracked by epic `initech-croo`, all items
   labeled `audit-2026-07`.
5. **Beads reconciliation:** closed `pipa`/`ax9.2`/`5l5z` (verifiably
   shipped); reverted `bea`/`40oq`/`hdlb`/`hsct`/`jh7m`/`re30.4` from
   in_progress to open with evidence notes; claimed `gymo` (WIP verified
   green); widened `6vr1` to all seven affected objects (P2→P1); filed 15
   new issues (`ljck`, `7s1z`, `c921`, `xi7x`, `tf3c`, `qvtj`, `0k2d`,
   `h5vg`, `jk8t`, `msol`, `lk29`, `quke`, `2h6j`, `in2g`, epic `croo`);
   registered the redirection-arc dep chain `bsy.7←bsy.9`, `bsy.8←bsy.7`.

## Headline findings (full detail in the plan doc)

- **P1 build-integrity bug (`6vr1` widened):** seven `KERNEL_*_OBJ` rules
  reference later-defined variables in their PREREQUISITE lists, which GNU
  Make expands at parse time — `build/region.o` does not depend on
  `region.c`; incremental builds relink stale kernels. Oracles unaffected
  (they compile sources directly), which is exactly why it survived.
- **9 real mutation-proven gates orphaned from `make test` (`ljck`)**,
  including the only regression guard for the WL-0068 `jf8p` fix.
- **`gymo` stdfile WIP is finished and orphaned** — green with biting
  mutants; needs Makefile wiring + commit only.
- **Law-2 soft spots:** the real-DBASE.EXE differential (PRD's stated
  primary SAMIR oracle) is loud-skipped everywhere; 6 of 7 prog-diff
  goldens have no proven mutant; `.ndx` B-tree is 2-level only (`h5vg`).
- **Rule 5 in practice:** default gate is QEMU-only; `run-bochs` is a
  no-op; no 86Box driver exists (`x0i`, `in2g`).
- **Region property suite never samples negative coords (`xi7x`)** though
  every top-left drag produces them live.
- **Governance:** ADR-0001/0002 cited as ratified but absent (`lk29`);
  `os/tps/` empty and the M7/M8 gap unsized (`qvtj`).

## Frictions

- The beads dolt lock (single-writer) blocked all lane agents' `bd`
  verification mid-audit; statuses were re-checked first-person afterward.
- **A persisted `cd` into a stale `.claude/worktrees/` checkout silently
  substituted a WL-0032-era Makefile during verification** (176+27 gates)
  — caught by count reconciliation, re-verified from the root, recorded as
  `bd memories audit-trap-2026-07-09`, and the prune is now `c921`.
- One survey lane (docs-adr-prd, opus) exhausted its structured-output
  retries; its scope was covered first-person instead.
- Lane severity calibration drifted (same artifact rated P0/P1/P2 by three
  lanes); the critic pass corrected this — keep the critic stage in future
  audits.

## Acceptance

- `make clean && make test` ALL GREEN 296 host + 61 emu (pre-existing
  state, re-verified; no artifact code changed this session).
- Tracker consistent with the tree for every audited id; plan doc + this
  shard + HANDOFF block committed.

## Pointers

- Plan: `docs/plans/AUDIT-2026-07-09-improvement-plan.md`
- Epic: `bd show initech-croo`; items: `bd list --label audit-2026-07`
- Lane transcripts: session workflow `wf_61baffe8-1d5` (journal.jsonl)
- Start here next session: **Phase 1** — `6vr1` → `ljck` → `gymo` →
  `7s1z` → `c921`, then the authoritative clean gate.

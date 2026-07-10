<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Programme Improvement Plan — 2026-07-09 Whole-Repository Audit

**Issuing Body:** Initech Systems Corporation — Platform Engineering, Office of Programme Assurance
**Document Class:** Improvement Plan (living document; supersede in place)
**Session Record:** `docs/worklog/WL-0069-whole-repo-audit-improvement-plan.md`
**Tracking Epic:** `initech-croo` (label `audit-2026-07` on all plan items)
**Ratified:** 2026-07-10 (operator directive: "completely review this project ... create a plan to improve it")

---

## 1. Method and evidentiary basis

Twelve parallel survey lanes (boot/kernel, MILTON DOS, FLAIR Toolbox,
ATKINSON, SAMIR, seed/TPS, harness/oracles, spec/fixtures, worklogs/HANDOFF,
beads, build/repro, plus a failed docs lane covered first-person) plus an
adversarial completeness critic, composed by the orchestrator. **Every
load-bearing claim below was re-verified first-person** against the repo
root (Rule 4), including a full `make clean && make test` — **ALL GREEN,
296 host + 61 emu gates (2026-07-09)** — confirming the WL-0068 certificate.

Audit trap recorded for posterity (`bd memories audit-trap-2026-07-09`): a
persisted shell `cd` into a stale `.claude/worktrees/` checkout silently
substituted a WL-0032-era Makefile (176+27 gates) during verification. All
findings were re-established from the repo root afterward.

## 2. Verdict

The Programme's discipline is sound: the failures found are almost all
*meta*-failures — oracles that exist but never run, prerequisites that
parse empty, documentation that lags the tree — rather than incorrect
shipped code. This is what Laws 1–4 predict would slip through, and it is
what this plan corrects. The least-started axis is the North Star itself:
`os/tps/` is empty, the seed speaks integer-only Pascal with no control
flow, and `test-compiler`/`selfhost`/`ddc` are honest stubs.

## 3. The Plan

### Phase 1 — Restore gate integrity (one session; small diffs; do first)

| Step | Bead | Finding (verified) |
|---|---|---|
| Fix the seven `KERNEL_*_OBJ` prerequisite rules | `initech-6vr1` (P1) | `make -rpn`: `build/region.o` omits `region.c`/`region.h`; `build/heap.o` has NO source prereqs; `build/chrome.o` omits `chrome.c/.h`. Prereq lists reference variables defined ~2,000 lines later; GNU Make expands prereqs at parse time. Incremental builds relink STALE kernels. |
| Fold the 9 orphaned gates into `make test` | `initech-ljck` (P1, after 6vr1) | `test-fat-write`, `test-fat12-subdir`, `test-samir-query`, `test-flair-dc4v(+mutant)`, `test-dbf-read-openrw-hdrlen-mutant` (the ONLY `jf8p` regression guard), `test-exit-handles-mutant`, `test-int21-irqstorm-mutant`, `test-mcb-emu-mutant`, `test-process-mutant-build` are defined but never run by the aggregate gate. Law 2: an oracle that never runs is no oracle. |
| Wire + commit the Standard File WIP | `initech-gymo` (P1, claimed) | `os/flair/stdfile.{c,h}` + `test_stdfile.c` (1,021 LOC, untracked since WL-0066): verified 29/29 green, all 3 mutants bite, freestanding typecheck OK. Needs `test-stdfile` targets (copy the `test-list` pattern) + commit. |
| Real `make image` / `make run` | `initech-7s1z` (P2) | Both are M1 stubs; the documented CLAUDE.md workflow cannot boot the real OS despite flagship `*_IMG` targets existing. |
| Worktree hygiene | `initech-c921` (P3) | `.claude/worktrees/` excluded only in machine-local `.git/info/exclude`; prune the stale `agent-adeddefb35566901a` worktree (diff-check first). |

Phase-1 exit: `make clean && make test` green at the new, larger gate count.

### Phase 2 — Strengthen the weakest oracles (1–2 sessions)

| Step | Bead | Finding |
|---|---|---|
| Region generators: negative + near-INT16 coords | `initech-xi7x` (P2) | `gen_spec()` samples only [0,48)×[0,40); the live desktop produces negative bboxes on every top-left drag. The homomorphism suite is ADR-0005's ENTIRE correctness signal. |
| Mutation-prove the 6 unproven prog-diff goldens | `initech-0k2d` (P2) | Only `exact.out` has a proven mutant; the `bljs` heresy proved this golden class can encode the bug under test. |
| Seed mutant gates + negative div/mod fixture | `initech-tf3c` (P2) | Seed mutation proofs were one-off manual checks; `codegen.c:44-50` self-admits negative div/mod is untested. |
| softfp/ansi oracle audit | `initech-quke` (P3) | No dedicated golden/mutant found for two load-bearing shipped modules. |
| Promote `test-boot-bochs` into `make test` | `initech-in2g` (P2) | Default gate is QEMU-only; Rule 5. 86Box driver remains `initech-x0i` (P1, unchanged). |

### Phase 3 — Resume the product arcs on the hardened base

- **GUI P1/P2 tail:** `rl4v` (cross-menu drag — `menu.c:296` hits the bar
  once at click), `zvo6` + `a90f` (FILE COPY modal title + progress fill —
  Law 4, the frame shows both), `44ab` (RGN_ROW_X_MAX 256-vs-640 locked-spec
  self-contradiction — spec change, deliberate act), `hv7u` (full inactive
  chrome), `jh7m` (scrollbar fidelity).
- **MILTON redirection arc, true dependency order:** `bsy.9` FIRST (child
  PSP inherits parent JFT — `psp.c:141` hardcodes `jft[0..4]`, structurally
  blocking the rest) → `bsy.7` (`<`) → `bsy.8` (`|`) → `hsct` closes.
  Dep chain registered: `bsy.7 ← bsy.9`, `bsy.8 ← bsy.7`.
- **SAMIR parity:** `h5vg` (`.ndx` 2-level B-tree ceiling, new), `cq0j`.

### Phase 4 — Face the North Star

- `initech-qvtj` (P2): size the seed→Turbo Initech language gap into
  dependency-ordered beads under epic `rnh` (control flow, procedures,
  boolean/relational, types, units — the subset the compiler itself needs).
- `initech-79s` (decision): author + ratify ADR-0007.
- `initech-lk29` (P2): ADR-0001/ADR-0002 are cited as ratified but no files
  exist — reconstruct thin ratification records or amend the citations.
- `initech-586.4` (P2): the real-DBASE.EXE differential (the PRD's stated
  primary SAMIR oracle; currently loud-skipped everywhere).
- `initech-dam` (P0): frame stills into `fixtures/` — gates the entire
  Law-4 SSIM axis (`w06`, `f8v.4`).

### Docs track (any time; cheap)

- `initech-jk8t` (P2): HANDOFF restructure — archive PRIOR-STATE blocks,
  fix §6/§7 (which still describe WL-0029 as "latest").
- `initech-msol` (P3): six verified stale-prose sites (`test_int21_edge.c`
  tzq claim, `sft.c` teardown note, `test_chrome_fidelity.c` "RED by
  design", `spec/README.md` "planned", CLAUDE.md `CC_KERNEL`→`KERNEL_CC`,
  CLAUDE.md `box86.c` phantom).
- `initech-2h6j` (P3): `chimera_element_map.json` — wire an oracle or
  demote out of `spec/` (Rule 8).

## 4. Beads reconciliation performed with this audit (2026-07-09/10)

- **Closed:** `pipa`, `ax9.2`, `5l5z` (each verifiably shipped; evidence in
  close reasons).
- **Reverted in_progress → open with notes:** `bea`, `40oq`, `hdlb`,
  `hsct`, `jh7m`, `re30.4`.
- **Filed:** `ljck`, `7s1z`, `c921`, `xi7x`, `tf3c`, `qvtj`, `0k2d`,
  `h5vg`, `jk8t`, `msol`, `lk29`, `quke`, `2h6j`, `in2g`, epic `croo`.
- **Widened:** `6vr1` (desktop.o → all seven objects; P2→P1).
- **Not touched (operator call):** the open M0–M2 P0 milestone epics
  (`njv`/`2rb`/`509`/`f8v`) — recommend an operator roll-up review.

Governed by: **CLAUDE.md Laws 1–4, Rules 4/6/9; PRD §7–§9.**

# WL-0021 -- ti8 Layer-1 parity tranche + FLAIR GUI groundwork (region engine launched)

Branch: `command-com-default`. An orchestration-heavy session: the operator
directed sustained parallel work on the next steps, delegating coding to
subagents while the main thread monitored, verified, merged, and filed beads.
Two thrusts ran concurrently -- the DOS-parity kernel tranche (resume at
`initech-ti8`) and, in parallel, laying the GUI/Toolbox (FLAIR) groundwork via
an ADR-by-committee. See `WL-0020` for the ATKINSON region-engine detail.

## Context

Resume point was `initech-ti8` (subdir/path traversal). Per WL-0019 the
directory chain is a shared-file kernel chain that must be driven serially and
oracle-gated; raw parallelism was therefore applied only to the *independent*
axes: read-only grounding, non-overlapping hygiene beads, and GUI **research**
(documents/beads, which conflict with nothing).

## What changed (all merged to `command-com-default`, each re-gated on the main thread)

**Kernel-parity tranche:**
- **`initech-ti8` Layer 1** (`99b84bd`) -- additive FAT12 subdir traversal:
  `fat12_dir_t` + `fat12_read_dir` (is_root branch delegates to the UNCHANGED
  `fat12_read_root_dir`; else a cluster-chain walk, anti-hang bounded) +
  `fat12_resolve_path`. New `test-fat-subdir` 3-way differential on a real
  mtools nested image (40-file BIGDIR = 3 clusters, forcing a multi-cluster
  walk) + 2 mutants, mutation-proven. The 4 root primitives are byte-unchanged;
  zero spec edits. Verified: independent adversarial verifier (8/8) + my
  main-thread re-run.
- **Grounding discovery (Law 1):** the `ti8` build brief's `int21.c` line
  numbers had all drifted (do_open 795->858, etc.; int21.h:113->149). More
  importantly, `ti8` is two layers of very different risk: L1 (fat12, clean +
  additive) and L2 (the `int21_file_backend_t` vtable is root-only; threading a
  resolved dir is a Core-tier cross-cut touching int21.h + fileio_fat.c + 3 host
  mocks + g_cwd lifecycle + 5 rejection sites). Split out as **`initech-mzxa`**
  (depends on ti8 L1; `u6wa` now depends on mzxa). Conflict resolved on the main
  thread: int21.c:1476 is `do_exec`, not rename.
- **`initech-l6o0`** (`d4e7f74`) -- `test-assets` now SKIPS gracefully+loudly
  when the local-only gitignored frame fixture is absent (so `make test`
  self-completes on a clean checkout) and runs the FULL palette-honesty check
  when present. The fake-placeholder alternative was deliberately REJECTED (it
  would fake-pass the honesty check -- a Stop-condition). Verified both paths.
- **`initech-h58`** (`2330be8`) -- retired the redundant `SHELL_IMG`; `test-shell`
  boots `TRACER_IMG` (byte-identical `KERNEL_SHELL_BIN`) + a new `A:\>` prompt
  screendump gate (`PPM_TEXT_CHECK_BIN`). `SHELL_NAME` preserved.
- **Canon fix** (`4aad132`) -- `kmain.c` said "the wristwatch are canon";
  corrected (hourglass is canon, wristwatch is THE BUG, PRD Appendix A). Found by
  the FLAIR committee's fidelity reviewer.

**FLAIR GUI groundwork (the "in parallel" thrust):**
- An ADR-by-committee (5 research briefs -> 5 adversarial personas -> synthesis)
  ratified 6 decisions and honestly escalated 6 stop-condition decisions to the
  operator. Operator ratified: **specs + region engine now** (parallel with
  f8v.4), **indexed-8** canonical depth, **640x480**, **keep seafoam** desktop_bg.
- **`spec/region_algebra.h` locked** (`7cfc655`) -- clean-room per-scanline
  inversion-list contract (5 normal-form invariants, 4 op truth tables, verbatim
  QuickDraw op names, no-0x7FFF guardrail, a build-time `_Static_assert` oracle).
  Draft **ADR-0004** (FLAIR Toolbox) + **ADR-0005** (region engine), DRAFT/Proposed.
- **Region engine implemented** (`305c608`, see WL-0020) -- `os/flair/atkinson/`
  region rep + scanline merge + derived ops; `make test-region` green (homomorphism
  oracle over 16000 random pairs incl. raw-scanline-span generators + shrinker; 3
  mutants bite); freestanding+hosted dual-compile, no host malloc.
- **Spec reconciliation** (`3d19d80`) -- invariant (4) "no empty interior row" was
  literally wrong (it outlawed the load-bearing gap-closing empty row a vertical
  hole needs). Reworded to match the proven-correct engine. Flagged by the engine
  subagent's self-critique, confirmed by reading `region_assert_normal`.
- **Bead lattice filed** -- epic `initech-k8o5` (FLAIR Toolbox) + 13 children, 13
  existing region beads reparented; `jmo`'s `f8v.4` blocker removed per the
  operator's parallel-progress authorization; `bd` Total 216->230 (+14, no strays,
  independently re-verified).

## Why this shape

Read-only analysis/research fanned out (the ti8 grounding, the GUI committee);
shared-file implementation ran in ISOLATED worktrees and was adversarially
verified before an independent main-thread `make test` re-run (Rule 4). The
directory chain and the region chain are both hard dependency chains over shared
files, so each was a SERIAL worktree effort, not parallelized. The genuine
parallelism was across the independent axes (kernel tranche || hygiene || GUI
research || bead-filing).

## Frictions / lessons

- **Workflow guard bug (mine):** the region pipeline's `status==='green'` guard
  rejected the engine's verbose status string and SKIPPED its verify stage. The
  engine was actually green; I ran the adversarial verification on the main thread
  instead. Lesson: match on a status ENUM the agent is told to emit verbatim, or
  treat a present commit_sha + green oracle as success.
- **The locked spec can be wrong.** A committee-designed invariant (region (4))
  contradicted the only correct implementation. The homomorphism oracle + the
  subagent's self-critique caught it; a Rule-8 reconciliation fixed the contract
  to the proven code (the honest direction).
- **Grounding before code pays.** The ti8 brief's int21 line numbers had drifted
  and it under-counted the rejection sites and mis-scoped L2 as a one-liner. The
  read-only grounding pass corrected all of it before any code was written.

## Acceptance

`make test` GREEN (63 host + 22 emu gates) on the merged tree, re-run on the main
thread. `make test-region` + `make
test-region-mutant` green, mutants mutation-proven, region engine freestanding-
legal. `make test-fat-subdir`/`-mutant`, `test-fat` (no root regression),
`test-assets` (both present + skip paths), `test-shell` green. `initech-ti8`,
`initech-l6o0`, `initech-h58` closed; `initech-jmo`/`b5g`/`6dy` closed on merge.

## Pointers / next

- **Kernel parity:** `initech-mzxa` (ti8 Layer 2, the INT 21h vtable cross-cut) is
  the next kernel step, then `u6wa` (MKDIR/RMDIR/CHDIR) -> `ut6d`. Drive serially,
  oracle-gated. Grounded map is in the `mzxa` notes.
- **FLAIR:** the region engine (`jmo`->`b5g`->`6dy`) is green; next per the epic
  `initech-k8o5` is `SURFACE` (`k8o5.6`, extract `console.c` into one surface
  module), `i50` (blitter with region clipping, now unblocked by the green engine),
  then `26d`/`kg5` and the `87a` M3 window-drag gate. ADR-0004/0005 await operator
  ratification (`k8o5.2`).
- **Still-open operator questions:** FLAIR heap home (new high region vs MCB arena,
  `k8o5.5`); real-Bochs/86Box pixel-capture funding. Both recorded in the ADRs.

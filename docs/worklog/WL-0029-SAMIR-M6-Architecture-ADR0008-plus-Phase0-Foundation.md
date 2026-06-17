<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0029 -- SAMIR (InitechBase, M6): architecture (ADR-0008) + Phase-0 foundation, orchestrated

Branch: `command-com-default` (`main` fast-forwarded to the tip each landing). The first
SAMIR/M6 build session: ratified the engine architecture, materialized the full
implementation DAG into beads, and landed Phase 0 (the portability + harness foundation)
end-to-end -- delegated to subagents, graded + integrated by the orchestrator.

## Context

The operator deep-read the sister corpus `../dbase3-decomp` (the dBASE III PLUS 1.1
reconstruction knowledge base: byte-verified specs + minted ground truth + golden
fixtures + a live dosbox-x mint harness) and directed: build SAMIR as a **platform-agnostic**
engine (must survive InitechDOS drift without cascading refactors), use the corpus specs as
the **test harness** (fast mechanical feedback), produce a **granular (~200 LOC/step)
swarm-ready** plan that is also serial-able, and orchestrate the build via subagents (up to
5 sonnet / 2 opus in parallel) with the main loop owning grading + integration + the bead
ledger. Operator decisions this session: **dBASE III PLUS 1.1 ONLY for M6** (IV/`.mdx`
dropped); **commit per landing then parallelize**; **run autonomously through Phase 0,
report at the boundary**.

## What changed

### Design + governance
- **`docs/plans/SAMIR-implementation-plan.md`** -- the granular ~38-step DAG. Phases 0-8,
  each step with a contract, deps, parallel group, oracle+golden, gate, and a GATED register
  for unclear-until-info items. III+1.1-only.
- **`docs/adr/ADR-0008-SAMIR-InitechBase-Architecture.md`** -- **RATIFIED** (operator-delegated)
  via an ADR-by-committee: three adversarial Sonnet reviewers (technical/PAL, oracle/harness,
  dBASE-fidelity) all returned "ratify-with-revisions"; every revision was applied. Decisions:
  DEC-01 storage split (`os/samir/` engine + `harness/diff/dbf_diff/` grader + `spec/samir/`
  locked data); **DEC-02 the Platform Abstraction Layer** (`pal.h` is the engine's ONLY OS
  surface -> InitechDOS drift touches only `pal_milton.c`; Phases 0-7 are host-developable);
  DEC-03 III+1.1-only; DEC-04 freestanding engine + host-compiled oracles; DEC-05 three-tier
  corpus-backed grader (Tier-0 committed manifests / Tier-1 path-referenced goldens with
  loud-skip / Tier-2 minting); DEC-06 locked spec-data **pinned to the III+-only corpus
  tables** (NOT the brief -- prevents an IV `==`/`0x1C` leak); DEC-07 IEEE-double numerics,
  observable behavior confined to the formatter (computed-key x87 GATED).
- **`InitechOS-PRD.md` §6.6** -- reconciled with a III+1.1-only deviation note citing ADR-0008
  (the committee flagged the PRD/ADR scope contradiction; the operator decision is the sign-off).

### Beads
- Materialized the full DAG: new Phase-0 parent `initech-586.5` with children `586.5.1..8`;
  the rest decomposed under the existing coarse beads (`aul/gmo/7az/ahu/17n/0tl/ax9`) as
  `<parent>.N` step children, deps wired, no cycles (`bd dep cycles` clean). ~38 step beads.

### Phase 0 -- portability + harness foundation (CODE, all green)
- **S0.1 `586.5.1`** (Opus) -- `os/samir/include/samir/pal.h` (full `samir_pal` vtable,
  freestanding, INT 21h-mapped, terminal extension + seek=filesize) + `pal_null.c` proof.
- **S0.2 `586.5.3`** -- `os/samir/pal/pal_host.c` (libc binding, injectable clock,
  errno->symbolic PAL_*, bump arena).
- **S0.3 `586.5.2`** -- `os/samir/core/rt.c`+`rt.h` (freestanding JDN<->Gregorian, `dec_format`
  ties->+inf [minted], `dec_parse`, mem/str).
- **S0.5 `586.5.4`** -- `spec/samir/` LOCKED data imported from the III+-only corpus:
  `dbf_format.h`, `ndx_format.h`, `xbase_coercion.json` (no `==`), `dbf_normalization.json`
  (`0x1C`/`0x1F` NORMALIZE), `dbase_msg_codes.tsv` (151), `README.md` provenance.
- **S0.6 `586.5.5`** -- `os/samir/core/value.c`+`value.h` (`xb_val` C/N/D/L/M/U; N=double,
  D=JDN; ctor/typeof/eq -- raw equality, NOT the SET EXACT operator).
- **S6.1 `586.5.7`** + **S6.2 `586.5.8`** (pulled to Phase 0 as the oracle independence
  barrier) -- `harness/diff/dbf_diff/dbf_ref.py` + `ndx_ref.py`: independent .dbf/.dbt and
  .ndx readers from first principles (NO shared code/constants with the C engine).
- **S0.4 `586.5.6`** (orchestrator integration) -- `harness/diff/dbf_diff/` skeleton; the
  `test-samir` foundation umbrella + per-step host-oracle targets wired into `TEST_UNIT_GATES`;
  `DBASE3_DECOMP ?= ../dbase3-decomp` resolution + `need_goldens` loud-skip macro (DEC-05).

## Why

The PAL is the platform-agnosticism the operator required: an InitechDOS revert/drift touches
`pal_milton.c` alone; the engine + every oracle are insulated, and the whole engine grades on
the host at full speed (no kernel/boot/GUI dependency for Phases 0-7). The corpus is reused as
the grader (its byte-dumps -> Tier-0 manifests; its goldens -> the differential corpus; its
mint harness -> new goldens), and its MINT sessions let us LOCK the coercion table + NDX
numeric-key encoding now instead of leaving them DRAFT.

## Frictions / lessons
1. **The committee earned its keep again** (WL-0017 DEC-04a echo). Three independent reviewers
   caught real issues a single pass missed: a PRD/ADR scope contradiction (unpatched), an
   IV-leak risk if the spec-data were sourced from the brief instead of the corpus
   (`==` row, `0x1C` MEANINGFUL), the PAL vtable being incomplete for `@GET` (terminal ext
   added), DEC-07 overclaiming for computed NDX keys, and ~8 steps missing `+mutant` tags. All
   folded in before ratification.
2. **Orchestrator-owns-the-Makefile kills the shared-file race.** WL-0028's worktree-stale-main
   pain (`t6nc`) is sidestepped: subagents own DISJOINT file sets and NEVER touch the Makefile;
   the orchestrator wires the gate targets at integration. Parallel-in-main-tree is then
   race-free (wave 2 ran S0.2/S0.3/S0.5 concurrently with zero conflict).
3. **Re-grade, do not trust the report (Law 2/4).** Every subagent "green" was independently
   re-run by the orchestrator (the oracle, the ASCII scan, the freestanding compile, the
   disjoint-path check) before the bead closed and the commit landed.
4. **The independent readers immediately bit** -- they caught two corpus-doc prose typos
   (ndx.md ZIPCODE "50 keys" -> actual 49 = CLIENTS nrec; dbf.md CLIENTS ADDRESS "C(20)" ->
   actual C(25)). The readers assert the BYTES (Law 2). Feed back to `../dbase3-decomp` (doc
   nits only; no InitechOS impact).

## Acceptance
- `make test-samir` = the 7-sub-gate foundation umbrella GREEN (pal 21/0, rt 52/0, pal-host
  25/0, spec 111/0, value 59/0, dbf-ref 147/0 vs goldens, ndx-ref 116/0 vs goldens).
- Authoritative `make clean && make test-unit` = **ALL GREEN (104 host gates)** -- was 103;
  +1 = `test-samir` wired into the aggregate (WL-0028 "wire the gate + verify the count"
  discipline). Emu gates (27) untouched -- SAMIR is host-only, not yet in the boot image.
- All engine code (`pal.h`, `rt.c`, `value.c`, `pal_null.c`) compiles under the kernel's
  freestanding flags (`-m32 -ffreestanding -nostdlib`). ASCII-clean throughout (Rule 12).
- 4 commits, `main` fast-forwarded to the tip each landing.

## Pointers / next
- **The plan is the authority:** `docs/plans/SAMIR-implementation-plan.md`. ADR: `docs/adr/ADR-0008`.
- **Standing dependency:** the sister corpus `../dbase3-decomp` (Tier-1 gates resolve
  `DBASE3_DECOMP`; absent -> loud-skip exit-1). It holds the gitignored goldens + the dosbox-x
  mint harness for new goldens (`re/harness-setup.md`).
- **NEXT = Phase 1, the `.dbf` codec** (epic `aul`): the serial chain **S1.1 -> S1.2 -> S1.3
  -> S1.4 -> S1.5** (header parse+invariants -> field descriptors -> record read -> write/
  round-trip -> mutate). Each grades against the Tier-0 corpus values AND the `dbf_ref.py`
  independence barrier; mutation-prove each. `bd ready` surfaces S1.1 (`initech-aul.1`) once
  the foundation is the base (it depends on S0.6/S0.2/S0.5, all closed). Phase 1 is mostly
  serial (each step builds on the prior) -- pipeline rather than fan out.
- **Orchestration cadence (operator-set):** delegate each step to a subagent (sonnet default,
  opus for load-bearing); orchestrator owns DISJOINT file ownership + Makefile integration +
  re-grading + commit-per-landing + the bead ledger; report at phase boundaries.
- **GATED items** (plan §7): numfn-1..8 edges, proc by-ref/PUBLIC, incremental NDX split,
  REPLACE-overflow abort-vs-store (all MINT); the FLAIR window seam (S8.3, needs M4); the
  authenticity tier (S6.5, needs real DBASE.EXE); kernel x87/FPU readiness (S8.1).

---

*-- End of Shard WL-0029 --*

<!-- Tedium certified compliant with NFR-7. The Y2K bug and the rounding-error virus remain enforced canon. -->

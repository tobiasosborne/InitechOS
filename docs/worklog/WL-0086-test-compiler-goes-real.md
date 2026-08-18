<!-- INITECH CONFIDENTIAL - INTERNAL USE ONLY - DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0086 - test-compiler Goes Real (B9.5, M7 Close Candidate)

**Issuing Body:** Initech Systems Corporation - Platform Engineering
**Document Class:** Worklog Shard (lane record)
**Session:** 2026-08-18 (continuation of WL-0082 through WL-0085)
**Control bead:** `initech-6m52`

## Context

B9.4 proved one generated TINY program on metal but left deep recursion,
multiple simultaneous string temporaries, and frame-local arrays of records
behaviorally unexecuted. `test-compiler` was still the honest M7 failing
placeholder. PRD Sections 6.7 and 8 and ADR-0007 DEC-07 Rung 3 require a real
shared-corpus FPC differential gated at 100 percent.

## What changed

- Registered the 17-row Turbo Initech Shared Compiler Corpus in
  `os/tps/CORPUS.md`. Every `.pas` has a sibling hand-computed `.golden`;
  FPC is checked against the root rather than minting it.
- Added focused fixtures for thirteen recursive activations plus dynamic
  complete boolean evaluation, four simultaneous right-nested ShortString
  temporaries, and a negative-bound frame-local array of records.
- Replaced `test-compiler` with the real host aggregate. Every row is compiled
  and run by FPC against its golden; FPC-built TPS emits twice, and the `.s`,
  `.o`, bare ELF, and DOS ELF artifacts are byte-identical between passes.
- Wired `test-compiler-os`: the existing compile-on-InitechDOS TINY tooth is
  retained, then all 17 TPS-generated `.COM` files run from one bounded
  `AUTOEXEC.BAT` boot. Captured blocks diff directly against FPC stdout and the
  gate prints `differential_pass_rate=100% (17/17)` only on complete success.
- Added `TPS_GEN_MUT_VARPARAM`. It redirects var-parameter assignment stores
  to the callee slot; the registered `gen_func_deep` aliasing clauses must go
  red. `test-compiler-os-mutant` aggregates that bite with the existing
  OFFBYONE wrong-value bite on registered `gen_tiny`.
- Removed M7 placeholder bookkeeping while leaving M8 `selfhost` and `ddc`
  untouched.

## Acceptance run in this lane

- RED before implementation: `make test-compiler` failed at the intentional
  M7 placeholder.
- Direct FPC root check for all three new fixtures: green.
- `make test-compiler`: green,
  `compiler_host_pass_rate=100% (17/17)`.
- Corpus self-generation budget observed: TPS.COM 89,591 bytes; BSS 71,704;
  self assembly 765,205 bytes; DOS end `0x6d27c`, 7,556 bytes below `0x6f000`.

Per the lane brief, no emulator, QMP, X, or `make test-unit` command was run.
`test-compiler-os`, `test-compiler-os-mutant`, and the aggregate gate remain
orchestrator-owned execution obligations. No commit or push was made.

## Honest uncertainties / handoff

- The one-boot 17-program DOS batch, its per-fixture marker extraction, and
  both semantic mutant executions are wired but unrun in this lane.
- The existing B9.4 TINY-on-DOS evidence is green, but it is not substituted
  for the unrun full-corpus rail.
- M7 should be closed in beads only after the orchestrator runs both new
  emulator gates at green and records the full certificate. M8 inherits the
  7,556-byte self-image headroom watch item.

## Orchestrator Certification Outcome (2026-08-18, first-person)

The HOST half certifies clean: `test-compiler` re-proven 17/17, in the unit
vector. The EMULATOR half went RED — and not as a TPS defect: the 17-program
one-boot AUTOEXEC rail exposed a LATENT MILTON batch/EXEC interaction bug
(programs silently skip or the batch stalls, position- AND content-dependent;
every fixture's generated code proven correct at the prompt, bare-metal, and
in short batches; full forensic ladder on bead `initech-6gkm`, P1). Per Law 2
a red gate cannot join the vector: `test-compiler-os` and its mutant leg are
WIRED but held out of TEST_EMU_GATES behind an OMISSION note at the vector
head; they rejoin — and M7 closes — when `6gkm` lands. `6m52` is blocked on
`6gkm` in the tracker. The corpus rail's harness ceiling was re-sized
(120 s -> 420 s) with an in-gate note after the first failure mode proved to
be truncation; the surviving failures are the guest-side bug.

The find is the system working: three compilers agree everywhere they can
run, and the one red light points at a real, previously unreachable kernel
defect — exactly what the rail was built to catch.

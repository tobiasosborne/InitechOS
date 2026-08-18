<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0085 — Turbo Initech Emits x86, and Compiles Itself (B9.4)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Worklog Shard (session record)
**Session:** 2026-08-18 (continuation of the WL-0080..0084 sitting)

## What changed (commit `38b542e`)

The fourth pass: `tps.pas` (~5,000 lines) generates deterministic nasm x86
for the full accepted subset, written to `TPSOUT.S` through buffered B8
`BlockWrite`. The emission contract (register discipline, frame layout, RTL
entries — the SAME runtime pairing as the seed) is documented in `CODEGEN.md`
with a hand-checked excerpt. The contract is BEHAVIORAL, not textual (Law 2:
the seed is a different compiler; only execution is shared truth).

**The landmark:** the self-source gate has TPS compile `tps.pas` ITSELF —
764,500 bytes of generated assembly, twice-byte-identical, assembling and
linking under both runtime targets, with the DOS image ending 7,644 bytes
under the hard `0x6F000` ceiling. **The K₁-precursor artifact exists.** The
thin headroom is the recorded M8 watch item (K₂ lives in the same arena).

**The full loop on metal (orchestrator-run):** `test-tps-gen-os` boots
InitechDOS, TPS.COM compiles the TINY fixture ON the OS, the generated `.s`
is extracted, assembled, linked — and the TPS-compiled `.COM` boots and runs
correctly on the same InitechDOS, with three-way behavioral agreement
(TPS-compiled == seed-compiled == fpc-compiled). Rule 6 split-axis:
`TPS_GEN_MUT_LABELS` fails loud at assembly (duplicate labels);
`TPS_GEN_MUT_OFFBYONE` executes to a WRONG VALUE on the booted 386
(`test-tps-gen-os-mutant`) — the mutant killed by execution, not inspection.

Budgets: seed-built TPS.COM 89,519 B; BSS 71,704 B under a deliberately
raised 72 KiB soft reserve; hard ceiling untouched.

## Acceptance

`make clean && make test` **ALL GREEN — 338 host + 92 emu.**

## Pointers

- `6m52` IN PROGRESS: B9.1–B9.4 of 5 done. **Next: B9.5 — `test-compiler`
  goes REAL** (the shared-corpus fpc differential at 100% pass rate, PRD
  6.7/8) and M7 CLOSES. Brief staged; corpus must cover the families B9.4
  left behaviorally unexercised (deep recursion, string temporaries, local
  array-of-record) and respect the pinned `forward` divergence.
- M8 watch items: the 7,644 B self-assembly headroom; the four-pass
  re-read pattern's cost on metal (fine for fixtures; measure for
  self-compilation).

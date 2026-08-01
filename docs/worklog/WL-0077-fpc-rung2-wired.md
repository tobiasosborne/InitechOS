# WL-0077 — fpc lands: the DEC-07 Rung-2 differential is wired, the B5/B6 corpus backfilled, and a 16-bit-integer dialect hole closed

**Date:** 2026-08-01 · **Session:** continuation of the WL-0076 session (operator installed fpc mid-session)
**Baseline:** `8f637ba` — WL-0076's ALL GREEN 320 host + 85 emu certificate.
**Beads:** `initech-altq` (P1), `initech-4yvg` (P2) — both closed here.

## Context

The operator ran `sudo apt install fpc` (fpc 3.2.2), unblocking `altq`:
test-seed-fpc-diff — the ADR-0007 Sec 4.7 Rung-2 Free Pascal differential,
well-designed but orphaned (not in TEST_UNIT_GATES, fpc absent since it was
authored at B4). The bead's recorded B7-committee ruling (Q13) required the
wiring to land TOGETHER with the missing B5/B6 fixtures (`4yvg`), not
half-migrated.

## What changed

1. **First real run** of the differential: fpc 3.2.2 agrees byte-for-byte
   with the seed on the existing corpus (func_shared, string_shared).
2. **B5/B6 backfill** (`4yvg`): `seed/examples/fpc/array_shared.pas`
   (const-bound global fill/sum, var-param swap of INDEXED elements,
   function-call index, lower-bound-0 + NEGATIVE-lower-bound rebase, boolean
   array, frame-resident local array) and `record_shared.pas` (named record,
   field r/w, var-param record mutation, whole-record COPY semantics,
   array-of-record nested l/r-value, record-array walk) — both inside the
   shared-output envelope (single line, plain integers + TRUE/FALSE, no char
   output). `SEED_FPC_CORPUS` grew to four entries.
3. **THE FIND — the corpus was running fpc with 16-BIT integers.**
   array_shared's REV=54321 came back −11215 from fpc (54321 − 65536): fpc's
   DEFAULT mode makes `integer` 16-bit, and ADR-0007 DEC-02 dialect pin 1
   ("`integer` is 32-bit signed") was pinned in-source for boolean-eval
   (`{$B+}`) but NEVER for the mode. The prior corpus never exceeded 32767,
   so the Rung-2 differential would have silently diffed a dialect
   divergence — not a codegen bug — the first time any fixture crossed
   16 bits. Fixed corpus-wide: line 1 of every fixture is now
   `{$MODE DELPHI}{$B+}` (`{$H-}` too for string_shared), mode first (it
   resets directive state), documented in each header. This is the
   independent-golden discipline working exactly as intended: the reference
   disagreed, and the disagreement was REAL and in the reference's
   invocation, not the seed.
4. **Wiring** (`altq`): test-seed-fpc-diff is IN `TEST_UNIT_GATES`
   (Bochs-precedent, initech-in2g): default-ON, LOUD FAIL when fpc is
   absent, `SKIP_FPC=1` as the one documented shouting opt-out (Make-level
   `ifeq`, same mechanics as SKIP_BOCHS). CLAUDE.md's install line +
   toolchain note updated; the `make help` text updated.
5. **Rule-6 proof (one-shot, right-reason-verified):** mutated seeds
   (`-DSEED_MUT_CODEGEN_ARRAY_LO_SKIP`, `-DSEED_MUT_CODEGEN_FIELD_OFF4`)
   drive the differential RED on the new fixtures with WRONG VALUES, not
   crashes (LSUM 45→35; record line diverges at the array-of-record leg) —
   a clean-seed control ran the identical pipeline green first. (A first
   attempt showed empty mutant output; root cause was an unexpanded
   `$(SEED_RT_DIR)` in the ad-hoc pipeline, i.e. a wrong-reason RED —
   redone properly. The permanent per-family mutation gates
   test-seed-array-mutant / test-seed-record-mutant are unchanged and
   green.)

## Acceptance

- `make test-seed-fpc-diff`: all four fixtures byte-identical seed-vs-fpc.
- `SKIP_FPC=1 make test-seed-fpc-diff`: the shouting skip arm.
- Seed family green: test-seed, test-seed-array-mutant,
  test-seed-record-mutant, test-seed-repro (14 fixtures / 42 hashes).
- Full `make clean && make test`: **ALL GREEN 321 host + 85 emu**
  (test-seed-fpc-diff joins the host vector).

## Pointers

- Fixtures: `seed/examples/fpc/{func,array,record,string}_shared.pas`.
- Gate: Makefile `test-seed-fpc-diff` (+ the SKIP_FPC ifeq); corpus list
  `SEED_FPC_CORPUS`.
- Dialect pin: ADR-0007 Sec 4.2 DEC-02 pin 1 (32-bit integer) — now
  actually enforced at the fpc invocation via `{$MODE DELPHI}`.
- Next: B8 (`ogxv`, file-I/O RTL) now has its Rung-2 harness ready-made —
  add `file_shared.pas` when it lands; Wave B of the solidity arc remains
  the staged GUI work (WL-0076).

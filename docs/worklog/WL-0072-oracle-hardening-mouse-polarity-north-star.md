<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0072 — Oracle hardening, the mouse-polarity fix, the North Star sized

**Session:** 2026-07-11 (continuation of the WL-0070/71 orchestrated arc)
**Commits:** `c05baa2` (quke) → `afe760c` (h5vg) → `393b4ec` (in2g) →
`99724a7` (sync) → plan commit (qvtj) → `940ed8e` (yx4v) → `444c4a6`
(9op1) → `5dd5525` (rgt8+8f5p)
**Certificate:** `make clean && make test` — **ALL GREEN, 311 host + 83
emu** (from 306+78 at WL-0071; +5 host: softfp impl-mutant, ndx
multilevel pair, mouse-producer pair; +5 emu: three Bochs legs promoted,
crossdrag pair. Reconciles gate-by-gate.)

## What changed

- **THE MOUSE WAS INVERTED (`rgt8` P1, operator-reported) — fixed at the
  producer + the blindness that hid it removed (`8f5p`).** The producer
  packed raw PS/2 dy into the ring, violating the locked event_model.h
  Sec-5 contract; every existing mouse-Y oracle recomputed expectations
  from the artifact's own += rule (HER-02 on the sign axis) and every
  injection trace/golden had BAKED THE BUG IN. Landed: a pure
  `mouse_pack.h` (negate + saturation), the independent wire-packet
  oracle `test-mouse-producer` (expected values from the HARDWARE
  contract; independence proven by contrast — FLIP_SIGN reddens it while
  the old test_event stays 240/240 green), and the full trace/golden
  rebaseline (flair-mouse expectation, drag/dc4v/menu/crossdrag dy
  signs, the LOCKED appswitch trace with an in-file Rule-8 note).
  Physical-up now raises the cursor on the booted 386.
- **Composed-integration catch:** the `9op1` crossdrag trace (authored
  pre-fix in its lane, green there) broke on main under the fixed
  producer — the lane-report-vs-composed-main distinction (Rule 4)
  earning its keep again.
- **`9op1`:** the live per-tick menu redraw follows a cross-menu drag
  (the rl4v rule applied to the live loop, stale panel erased via the
  existing compositor path) + a deterministic marker-triggered crossdrag
  emu leg with a frozen-mi mutant.
- **`yx4v`:** the Apple slot renders a HAND-AUTHORED 16x15
  apple-with-bite strike (provenance per the CLAUDE.md callout,
  back-checked vs the s7 goldens), 5 glyph-property oracle legs +
  APPLE_SQUARE mutant; the old ppm threshold literally encoded the
  square bug and was recalibrated, declared.
- **`h5vg`:** the .ndx bulk build (INDEX ON/REINDEX/PACK) handles
  arbitrary B-tree depth; incremental insert/delete now FAIL LOUD on
  3-level trees instead of corrupting; structural grading honestly
  declared (no real 3-level golden exists; minting it gates the
  incremental follow-up).
- **`quke`:** softfp already had a dedicated 23,858-check oracle (stale
  premise, recorded) — its reference-perturbing mutant replaced with a
  real implementation mutant (round-half-even tie break); ansi gained
  edge-clamp + mid-sequence-garbage legs + a no-clamp mutant.
- **`in2g`:** the three real Bochs boot legs are IN `make test` (loud
  fail if bochs missing; SKIP_BOCHS=1 the one documented escape —
  implemented parse-time after the naive per-line exit-0 skip was found
  broken).
- **`qvtj` — the North Star is sized:** docs/plans/TPS-M7-subset-plan.md
  + 10 dependency-ordered beads under epic `rnh` (B1 booleans → B4 the
  codegen pivot → B9 real fpc differential closes M7), 3 deep-bug loci
  flagged, ADR-0007 scope draft attached to `79s`.

## Frictions / lessons

- **One main-tree writer at a time (bd memory):** the in2g lane's
  Makefile edits were swept into the quke/h5vg commits by concurrent
  main-tree integration (`393b4ec` records it). Content verified correct
  at HEAD; process rule now in memory.
- Every mouse-injection trace in the repo was calibrated to the bug it
  should have caught — when a producer-side convention flips, sweep ALL
  traces (`grep '_SPEC\s*:= m'`), not just the failing gates.

## Beads

Closed: `quke h5vg in2g qvtj yx4v 9op1 8f5p rgt8` (29 total across the
WL-0070..72 arc). Filed: ansi param-value overflow, ndx incremental
multi-level (golden-gated), kernel runway, 10 M7 subset beads.

## Acceptance

Closing gate ALL GREEN 311 host + 83 emu (wave4_gate.log).

## Pointers

- Ready next: `hv7u`/`jh7m` (chrome fidelity tail), `x0i` (86Box, P1),
  `dam` (frame stills — operator-gated), `79s` (ADR-0007 authoring,
  unblocked), `f0uc` (B1, gated on 79s), `586.4` (real-DBASE
  differential), GUI bug-hunt P2 tail.
- The mouse-polarity rebaseline note lives in `5dd5525` + the rgt8/8f5p
  close reasons.

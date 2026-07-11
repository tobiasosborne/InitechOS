<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0073 — ADR-0007 ratified; Turbo Initech B1-B3 land; the chrome tail closes

**Session:** 2026-07-11 (continuation of the WL-0070..72 orchestrated arc)
**Commits:** `940ed8e`/`444c4a6`/`5dd5525` (wave 4, certified in WL-0072) →
`b41dc64` (hv7u) → `b6fc800` (ADR-0007 RATIFIED) → `9c8b826` (B1) →
`f0c20ac` (B2) → `7fda058` (B3)
**Certificate:** `make clean && make test` ALL GREEN **313 host + 83 emu**
(post-B2 run; B3 adds test-seed-char-mutant, verified by the full seed
suite first-person on main — next certificate will read 314+83).

## What changed

- **ADR-0007 IS RATIFIED** (`79s`): authored in house style (DEC-01..08),
  then a three-seat committee (governance / technical / oracle lenses)
  returned unanimous ratify-with-amendments; all 13 amendments applied at
  ratification (rev-1.0 history itemizes them). Highlights: dialect
  pinning (32-bit integer, fpc {$B+} differential, case-insensitive
  idents, to/downto), the `forward` directive added to REQUIRED (the
  parser's own mutual recursion needs it), in-subset int-to-string
  boundary, sentinel-accessor + boolean-flag order-independence idioms,
  insertion-ordered symbol tables + ONE threaded label counter, FO-5 (a
  test-seed-repro gate must exist before B4), OQ-1 resolved option (b)
  with the PRD staging clause. Citation loop verifiably closed (zero live
  pending references; three sibling ADRs also cleaned). Operator sign-off
  row honestly left pending.
- **Turbo Initech B1-B3 (the first North-Star language steps, all
  RED-first + mutation-proven + double-compile-deterministic):**
  B1 `f0uc` — relational/boolean/and-or-not, ISO/TP precedence reshape,
  a NEW typecheck pass, complete evaluation enforced STRUCTURALLY (the
  branch-free gen_binop grep leg). B2 `80iw` — if/while primitives +
  for/repeat parser-time sugar (bounds-once via __forlim_N, ISO 6.8.3.9
  cited), one threaded label counter, rodata_walk recursion fix.
  B3 `7mo3` — interleaved const sections with front-end folding, TP
  char-literal disambiguation, chr 8-bit truncation (cited), total ord,
  char compares. The seed can now express scan loops over characters —
  the raw material of its own future lexer.
- **Chrome tail:** `hv7u` — full inactive-window WDEF fidelity (gray
  frame #777777, dim title #A5A5A5, absent gadgets) with the canon
  extended through the EXISTING gray-ramp mechanism (locked 9-entry
  table untouched; Rule-8 recorded in-JSON) and an a9iq-era wrong
  expectation corrected, declared. `jh7m` closed as ALREADY SHIPPED
  (commit 85d5ec9, 2026-06-25) — the audit's "no code landed" note was
  wrong; every oracle re-verified rather than trusted.
- **Reconciliation:** `vcq` closed as superseded (its "quantize from the
  frame still" ask is the HER-11 heresy post-ADR-0010; the ratified
  color canon IS the palette deliverable); live residual (stage2/console
  SEAFOAM vs canon teal) extracted to its own decision bead. The
  kmain 8bpp DAC-loop residual noted on `2gva`.

## Frictions / lessons

- Ratification committees work as verification: the three seats caught a
  missing REQUIRED language feature (`forward`), a fixture-dispute
  landmine (fpc's default short-circuit vs DEC-03), and an unwired
  precondition (`3yv`) — none visible to the author alone.
- Stale beads cut both ways: `jh7m` (work existed, bead said it didn't)
  and `vcq` (bead survived a re-ratification that made it heresy). The
  fix both times: first-person verification against git + the oracles,
  never the tracker text alone (Rule 4).

## Beads

Closed this shard: `79s hv7u jh7m vcq f0uc 80iw 7mo3` (38 total across
WL-0070..73). The M7 chain: B4 (`63ce`) is now gated ONLY on `3yv` (the
FO-5 repro gate) — next up.

## Acceptance

Clean gate ALL GREEN 313 host + 83 emu (wave6_gate.log); B3's addition
verified via the full seed suite + all five seed mutant gates on main.

## Pointers

- Next: `3yv` (test-seed-repro gate, FO-5) → `63ce` (B4, THE codegen
  pivot: frames/calls/params — the M7 watershed) → B5..B9.
- Also ready: `x0i` (86Box, P1, environment-dependent), `586.4`
  (real-DBASE differential), `dam` (operator-gated stills), GUI P2 tail.

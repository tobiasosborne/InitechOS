<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0056 -- the Initech Color Canon module + the ORACLE-FIRST value oracle (epic initech-qipc steps 2+3)

**Type:** implementation shard (steps 2+3 of the FLAIR re-ratification build-out).
**Date:** 2026-06-23.
**Beads:** `initech-h714` (step 2, canon generator) + `initech-mwpw` (step 3, value oracle, ORACLE-FIRST). Epic `initech-qipc`.
**Commits:** `d703820` (h714) + the mwpw commit.

## Context -- why this shard exists

After the region spine landed (WL-0055, n79q), the roadmap's oracle-first color
chain begins. The deep root cause the whole re-ratification exists to kill (heresy
HER-02): the old FLAIR oracle graded color **BY CONSTRUCTION** -- `ppm_flair_check`
computed its expected RGBs from `flair_palette_rgb`, the SAME function the kernel
paints with, so the operator-condemned `preview.webp` palette passed every gate.
The live `spec/assets/palette.h` still carries those revoked samples (muddy
`#7F7F86` "white", seafoam `#6FA08E` desktop, `#1E2F87` accent).

Steps 2+3 build the CORRECT color authority and PROVE it grades against
INDEPENDENT goldens -- but WITHOUT flipping any render yet (that is step 4,
initech-7x9k). This is the oracle-first discipline (ADR-0010 / DEC-09-D3): the
value oracle lands GREEN against the canon BEFORE any render change.

## What changed (orchestrated: delegate -> grade -> integrate; I owned grading)

**h714 (`d703820`, sonnet lane) -- the canon module generator:**
- `tools/color_canon_extract.c` (NEW) -- a deterministic host generator (hand-rolled
  JSON reader, no libs, Law 3; modeled on `palette_extract.c`) reading the LOCKED
  `spec/assets/color_canon.json` and emitting the DO-NOT-EDIT header. Rule 11
  byte-stable; fail-loud on malformed JSON.
- `spec/assets/color_canon.h` (GENERATED, committed) -- freestanding-safe: the
  `color_canon[9][3]` table (idx2 teal {141,220,220}, idx5 navy {0,0,128}, ...),
  `_Static_assert(sizeof==27)`, `CIDX_*` index constants, `INITECH_CANON_*_RGB`
  macros, the two teal bevel macros (`#8DDCDC`/`#4E9BA3`), the `flair_canon_rgb(idx)`
  accessor (idx<9 table; idx>=9 the `(v<<16)|(v<<8)|v` gray ramp identical to
  palette.h's default branch), the wctb crosswalk comments, and the P4
  authored-honesty note.
- Makefile: `COLOR_CANON_*` vars + generator build + `gen-color-canon` (deliberate
  regen) + `test-color-canon-gen` (the regen-consistency gate: committed header ==
  generator output -- catches a hand-edit of the DO-NOT-EDIT file).

**mwpw (mwpw commit, opus lane) -- the value oracle (ORACLE-FIRST, the anti-heresy leg):**
- `harness/proptest/test_color_canon.c` (NEW) -- grades the GENERATED `color_canon.h`
  against INDEPENDENT decomp goldens, 4 legs (TOL=0):
  - LEG A: the wctb_0_System_753.bin BINARY (idx0/1/3/4 vs wctb parts 0/1/2).
  - LEG B: the win31 default-colors-cross-check.txt (idx5 navy #000080 + the
    DEPTH-TRAP: #0000AA rejected; idx6 #C0C0C0; + #DFDFDF / #008080 guardrails).
  - LEG C: pinstripe.md rendered rows (idx7 #F3F3F3 / idx8 #969696).
  - LEG D: AUTHORED teal (idx2 #8DDCDC + bevels) -- NO external golden, graded by a
    locked-constant + a green-cyan octant bound + the seafoam-relapse mutant (P4
    honesty; never claimed decomp-sourced).
  Per-leg LOUD-SKIP if a sibling golden is absent (never silent-pass).
- Makefile: `test-color-canon` + `test-color-canon-mutant` (4 VALUE mutants), wired
  into TEST_UNIT_GATES + .PHONY. (`WIN31_DECOMP` was already wired by n79q.)

## Why (the principle: P3, grade against an INDEPENDENT golden, never by construction)

The value-under-test is `color_canon.h` (generated from `color_canon.json`); the
A/B/C goldens (wctb / win31 / pinstripe) are independent of `color_canon.json`, so
they are genuine independent grades. LEG D has no upstream golden, so it is
honestly labeled authored and gated by a locked-constant + a mutant -- never
claimed decomp-sourced. This is exactly the discipline HER-02 violated.

## Frictions / lessons

- **The DECISIVE anti-by-construction grade is a real-artifact perturbation, not a
  -D mutant.** I graded mwpw hardest by editing the REAL `color_canon.h` (navy
  0,0,128 -> 0,0,170 = #0000AA) and running the UNMUTATED oracle: it went RED,
  proving the oracle grades the real artifact against the external win31 golden,
  not a self-consistent copy. The -D mutants (which the subagent authors) are
  necessary but a subagent-controlled mutant is weaker evidence than perturbing
  the artifact the orchestrator controls.
- **wctb 16-bit channels:** each ColorSpec channel is a 16-bit big-endian word;
  the 8-bit value is the HIGH byte (0xffff -> 0xff). Entries are keyed by the
  `value` part-code field, not array position.
- **Quoting differs by golden-access mode:** mwpw's decomp macros are QUOTED
  string literals (the oracle `fopen`s a path string, like test_clut.c) -- the
  OPPOSITE of n79q's wine `#include`, which needed an UNQUOTED `WIN31_DECOMP`
  (stringized into an include token). The single `WIN31_DECOMP ?= ../win31-decomp`
  Make var (unquoted) serves both, quoted at each use site as needed.
- **Generated-header consistency gate must NOT auto-regenerate.** `color_canon.h`
  is a committed artifact (not a regenerating make target), so `test-color-canon-gen`
  diffs the committed file against a fresh regen MEANINGFULLY (catches a stale/
  hand-edited commit). Mutation-proven: a seafoam-relapse hand-edit drives it RED.

## Acceptance (Law 2)

- h714: committed `color_canon.h` byte-identical to the generator output (truly
  generated); every value matches the locked JSON; freestanding + hosted compile;
  `_Static_assert` holds; deterministic; `test-color-canon-gen` GREEN + mutation-
  proven (seafoam-relapse hand-edit -> RED).
- mwpw: `test-color-canon` 23 graded / 0 failures GREEN; the 4 VALUE mutants
  (CANON_MUTATE_TEAL/NAVY/WHITE/PIN) each redden their leg; loud-skip honest (3
  rows NOT graded, exit 0); the DECISIVE real-header perturbation reddens the
  unmutated oracle; builds clean -Werror; ASCII-clean.
- Full clean aggregate (`make clean && make test`): GREEN -- **259 host + 43 emu**
  (was 256 host; +3 = test-color-canon-gen + test-color-canon + test-color-canon-mutant).
- NO render flipped, NO live switch touched -- palette.h / chrome.c / control.c /
  dialog.c / render.c are UNCHANGED. The teal flip + the 5-switch collapse is
  step 4 (initech-7x9k), which now has a GREEN value oracle to land against.

## Pointers

- Generator + header: `tools/color_canon_extract.c`, `spec/assets/color_canon.h`.
- Value oracle: `harness/proptest/test_color_canon.c`.
- Locked source: `spec/assets/color_canon.json` (the grading_contract block).
- Goldens: `../system7-decomp/goldens/resources/wctb_0_System_753.bin`,
  `../win31-decomp/refs/win31-chrome/default-colors-cross-check.txt`,
  `../system7-decomp/specs/chrome/pinstripe.md`.
- Design: ADR-0010 (grading + goldens), ADR-0004-AMENDMENT-DEC-09 ARB-1 (the module).
- Roadmap: `bd show initech-qipc`; NEXT = `initech-7x9k` (step 4: collapse the 5
  index->RGB switches onto flair_canon_rgb + flip idx2 to teal + re-key
  ppm_flair_check -- the Rule-8 VALUE change, now landing against a GREEN oracle).
- Prior: WL-0055 (n79q region spine), WL-0054 (the ratified design).

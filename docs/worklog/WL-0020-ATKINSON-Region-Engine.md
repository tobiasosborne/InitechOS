# WL-0020 -- ATKINSON region engine (FLAIR Toolbox, PRD Sec 6.2)

beads: initech-jmo (rep + normalize-on-construction), initech-b5g (scanline
merge + derived ops + complement + queries), initech-6dy (property suite,
mutation-proven). Parent epic: initech-k8o5 (FLAIR Toolbox / ADR-0004/0005).

## Context

PRD Sec 6.2 names the region engine "the load-bearing math" of the whole
Toolbox: clipping, window overlap, visRgn/clipRgn, and the update/damage model
all reduce to region algebra. The Specs stage (commit f973b37) locked
`spec/region_algebra.h` -- the inversion-list-per-scanline rep, the 5 normal-form
invariants, the 4 op truth tables, the storage caps, and the explicit
complement-frame semantics. This stage implements the engine ON TOP OF that
locked contract, RED-first.

## What changed

- `harness/proptest/test_region.c` (NEW) -- the property suite (the ORACLE),
  written FIRST. PRIMARY oracle is the HOMOMORPHISM property
  `rasterize(A OP B) == rasterize(A) OP_set rasterize(B)`, bit-exact, over
  16000 random region pairs (4 ops x 4000), with generators that include RAW
  random scanline-span sets (not only rect-unions, so non-rectangular bugs
  cannot hide) and a SHRINKER that bisects the item lists + clamps coords to a
  minimal counterexample. SECONDARY: construction fidelity (region built from a
  spec rasterizes to the spec's directly-painted bitmap), contains-point parity,
  region_intersects / region_rect_in_region vs the pixel truth, every-op output
  in normal form (5 invariants, checked independently of region_assert_normal),
  tight-bbox, normalize-idempotence (bit-exact: rows + x_pool + bbox), and the
  algebra identities (commutativity, associativity, De Morgan, A DIFF A = empty,
  A XOR A = empty, A UNION comp(A) = frame, rect-fast-path == general-path,
  region_equal symmetry). 31 checks, all green.
- `os/flair/atkinson/region.{c,h}` (NEW) -- the engine. region.h re-exports the
  locked spec. region.c: rep + normalize-on-construction (set_empty/set_rect/
  from_rects), region_assert_normal (the 5 invariants, fail-loud), the y-band
  sweep region_op with a per-band xmerge under boolfn (the 4 ops differ ONLY by
  the truth-table-driven boolfn), region_normalize (vertical-RLE collapse + tight
  bbox + is_rect/is_empty), region_complement (XOR against an explicit frame),
  the O(1)/parity queries, and the verbatim QuickDraw wrappers. NO host malloc:
  every region carries caller-supplied rows[]/x_pool; cap overflow FAILS LOUD.
  Dual-compile (psp.c idiom): `__STDC_HOSTED__` -> abort() hosted /
  __builtin_trap() freestanding. Verified it compiles under the exact kernel
  flags (`gcc -m32 -ffreestanding -nostdlib`) with no libc dependency.
- `Makefile` -- `test-region` (was stub_fail, M3) now builds+runs the suite;
  `test-region-mutant` builds the 3 mutants and asserts each goes RED; both
  wired into `TEST_UNIT_GATES` (host vector now 61 gates).

## Why -- the load-bearing spec-interpretation call (invariant 4)

The locked invariant (4) reads "no empty interior row." Taken literally that is
in TENSION with the row model (a row is valid for scanlines [y_top, next.y_top);
an empty row turns spans OFF): a region with two VERTICALLY-DISJOINT spans -- the
union of two rects separated by a gap, a basic homomorphism-oracle case -- can
ONLY be represented with an empty row at the gap's top. Without it the upper
span's row runs down into the gap and paints it.

Resolution (the consistent reading, NOT a silent weakening): invariant (4)'s own
justification clause scopes it to REDUNDANT empties -- "a hole that invariant (3)
should have merged OR normalize should have dropped." A gap-closing empty row is
neither (3)-mergeable (it differs from both neighbours) nor droppable (it is
load-bearing). So (4) forbids exactly: a LEADING empty row, and an
empty-UNDER-empty (the vertical-RLE-duplicate case (3) already covers). A single
empty interior row between two DIFFERENT non-empty rows is a legal vertical-gap
closer and is KEPT. This is the only reading under which the locked row model can
represent the regions the homomorphism oracle generates; it is documented in
region_normalize's and region_assert_normal's banners and is the basis of both
the engine's guard and the suite's independent normal_form_holds checker.

## Frictions

- The literal-vs-justification tension in invariant (4) (above). Stopped, traced
  it to a representability proof (gap is unrepresentable without the empty row),
  adopted the justification-scoped reading rather than editing the LOCKED spec.
- `-Werror` caught mutant-only unused variables (NO_VRLE: prev_x/prev_n;
  EMIT_NOCHANGE: now). A mutant must COMPILE to be able to go RED at runtime, so
  guarded `(void)` casts / kept prev_out coherent under the mutant.

## Acceptance

- `make test-region` GREEN: 31 checks, 0 failures (was stub_fail).
- `make test-region-mutant` GREEN: all 3 mutants RED then restored --
  RGN_MUTATE_NO_VRLE (normal-form guard fail-loud / non-canonical),
  RGN_MUTATE_PARITY_OFF1 (contains-point vs rasterize mismatch),
  RGN_MUTATE_EMIT_NOCHANGE (region-vs-spec rasterize mismatch). The oracle bites.
- Engine compiles freestanding (kernel flags) AND hosted; no libc in the
  freestanding object (uses __builtin_trap).
- ASCII-clean (Rule 12); seeded LCG, no timestamps (Rule 11).
- All 60 OTHER host-unit gates still green; the one pre-existing RED is
  `test-assets` (missing fixture `spec/assets/preview.webp`, absent at the Specs
  commit f973b37 -- unrelated to this engine).

## Pointers

- spec/region_algebra.h -- the LOCKED contract.
- os/flair/atkinson/region.c -- region_op (band sweep) + xmerge (boolfn) +
  region_normalize (the invariant-4 banner).
- harness/proptest/test_region.c -- the homomorphism oracle + shrinker + mutants.
- Next: the merge feeds the Window Manager's update/clip regions (initech-k8o5).

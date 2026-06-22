<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0055 -- initech-n79q: the ATKINSON dual-heritage region spine (ADR-0005 Amendment AM-1) lands

**Type:** implementation shard (the FIRST build-out step of epic `initech-qipc`).
**Date:** 2026-06-22.
**Bead:** `initech-n79q` (epic `initech-qipc`, landing-sequence step 1 -- color-independent, lands FIRST per the chief-architect reconciliation's R9 ordering).
**Commits:** `0db6685` (Wave 1, engine side) + the Wave 2 commit (oracle + Makefile + ADR cross-refs).
**Follow-up filed:** `initech-rmsr` (FO-D2-8, P3 -- exercise CombineRgn on metal via the Photoshop menu clip).

## Context -- why this shard exists

Epic `initech-qipc` (the FLAIR re-ratification) ratified a 5-document design set
but landed NO implementation code (WL-0054). Its 7-step roadmap is sequenced
oracle-first, and the consistency critic ruled the **region spine** lands FIRST
because it is color-independent -- everything else (the canon generator, the
value oracle, the switch collapse, the era registry, the live loop) builds on
top of it. This shard implements ADR-0005 Amendment AM-1 (FO-D2-3 + FO-D2-5 +
FO-D2-7): re-cast the ATKINSON engine heritage-neutral, add the GDI/HRGN peer
facade as a strict peer of the QuickDraw facade over the ONE `region_op`, fix the
RectInRgn containment->overlap deep bug, and grade the GDI facade against an
INDEPENDENT golden (the real wine banded-rect engine) -- never by construction.

## What changed (orchestrated: delegate -> grade -> integrate; I owned grading)

**Wave 1 (`0db6685`, opus lane, engine core):**
- `spec/region_algebra.h`: identity re-cast heritage-neutral (Sec 0/1) with dual
  provenance (Inside Macintosh QuickDraw + Win-3.1 wingdi.h); Sec 3 three-name
  column + XOR-decomposition note; Sec 7 retitled HERITAGE FACADES; NEW Sec 7b
  GDI facade -- RGN_AND/OR/XOR/DIFF/COPY=1..5 + region-type codes 0..3 verbatim
  wingdi.h, CombineRgn/GetRgnBox/PtInRegion/RectInRegion protos, `_Static_assert`s
  pinning every constant AND the mode->op mapping (a swapped wiring is a COMPILE
  error). `region_rect_in_region` proto renamed `region_rect_fully_in` + new
  `region_rect_overlaps`.
- `os/flair/atkinson/region.c`: GDI facade shims (<=5 lines each) over the lone
  `region_op` + `region_type_classify` (reads the is_empty/is_rect flags
  normalize already maintains); the containment/overlap split; the QuickDraw
  RectInRgn DEEP-BUG FIX (re-pointed containment->overlap, AM-4/C-9).
- `harness/proptest/test_region.c`: ADDITIVE only -- renamed the containment call
  site + added an independent overlap property case (any-pixel-in vs
  all-pixels-in). The homomorphism core + 3 engine mutants are byte-unchanged
  (AM-6).

**Wave 2 (oracle + integration):**
- `harness/proptest/test_region_gdi.c` (NEW, opus lane) -- the dual-heritage
  conformance oracle: L1 cross-family equality (by-construction, labeled), L2 the
  INDEPENDENT golden (the ATKINSON GDI facade rasterized bit-exact vs the REAL
  wine `server/region.c` banded-rect engine, gdi_ref_-namespaced + host-only,
  Law 3; loud-skips when ../win31-decomp absent), L3 the single-engine grep gate
  (constraint C-7). Plus `gdi_ref_wine.{c,h}` + `gdi_ref_wine_shim.h` +
  `gdi_ref_winestubs/*.h` (the host-only wine extraction). The two GDI mutation
  hooks (`GDI_MUTATE_DISPATCH`/`GDI_MUTATE_RECTIN`) were added to region.c as
  additive `#ifdef`s (default build byte-unchanged).
- `Makefile` (orchestrator-owned): `WIN31_DECOMP ?= ../win31-decomp` (the FIRST
  win31-decomp Makefile wiring -- FO-D2-5, chips HER-09); `test-region-gdi` +
  `test-region-gdi-mutant` targets; wired into TEST_UNIT_GATES + .PHONY.
  `test-region` left UNTOUCHED.
- `docs/adr/ADR-0005-ATKINSON-Region-Engine.md` (+C-7 to Sec 5.1) +
  `docs/adr/ADR-0004-FLAIR-Toolbox-Architecture.md` (+`test-region-gdi` row to the
  Sec 3.8 D-8 oracle vector) -- FO-D2-7, docs-in-lockstep.

## Why (the principles this discharges)

- **P2 (one engine, two heritages):** the GDI facade is a peer of the QuickDraw
  facade over the SINGLE `region_op`; C-7's STRUCTURAL grep gate (not a
  link-error backstop -- the wine `region_op` has a different signature, so
  static linkage gives no collision) is the load-bearing single-spine guard.
- **P3 (grade against an INDEPENDENT golden, never by construction):** L2 diffs
  the GDI facade against the REAL wine banded-rect engine -- a different-heritage
  representation that shares zero code with ATKINSON's inversion lists. The
  M-VALUE/M-DISPATCH/M-RECTIN mutants prove the independent golden catches wrong
  VALUES that the by-construction L1 cannot.
- **Rule 3 (all bugs are deep):** RectInRgn was containment; BOTH heritages
  document OVERLAP (regions-api.md "any overlap"; wine rect_in_region "at least
  partially inside"). Fixed once for both facades, not frozen as a heritage
  policy fork (a Stop-condition violation the ADR explicitly forbids).

## Frictions / lessons

- **`static inline` is NOT a C11 constant expression.** The Wave-1 agent found
  that `_Static_assert(rgn_op_from_combine_mode(...) == ...)` does not compile, so
  the mode->op mapping was lifted into a constant-foldable `RGN_OP_FROM_MODE`
  macro as the single source of truth (the inline fn returns it). Mutation-proven:
  mis-mapping RGN_OR->INTERSECT fails the build. LESSON: to make a mapping
  genuinely compile-checked, the map must be a macro/enum, not a function call.
- **`WIN31_DECOMP` must be passed UNQUOTED** (unlike `SYSTEM7_DECOMP` which the
  clut gate quotes), because the whole path is stringized into the wine
  `#include` token; a quoted value breaks the include. The Makefile var stays
  unquoted and `-I.` makes the relative path resolve from the repo root.
- **L2-only mutants need the wine golden.** GDI_MUTATE_RECTIN and
  RGN_MUTATE_PARITY_OFF1 redden ONLY the L2 wine legs, so on a checkout without
  ../win31-decomp they would falsely "pass" and turn the mutant gate falsely RED.
  `test-region-gdi-mutant` therefore asserts those two ONLY when the wine
  reference is present (DISPATCH + EMIT bite regardless), and loud-skips them
  otherwise -- never a false RED on a clean checkout.
- **EMIT_NOCHANGE (M-VALUE) reddens via the engine fail-loud (exit 134), not a
  clean CHECK** -- it trips region_assert_normal during multi-rect construction,
  exactly as the existing test-region EMIT mutant does. The intended
  L1-green/L2-red separation was verified in isolation (single-rect L1 stays
  green); RGN_MUTATE_PARITY_OFF1 additionally demonstrates it cleanly.

## Acceptance (Law 2)

- `make test-region` GREEN: 32 checks, 0 failures (AM-6 -- the frozen homomorphism
  oracle, byte-unchanged). `make test-region-mutant` 3/3 RED.
- `make test-region-gdi` GREEN: 9 checks, 0 failures (L1 + L2 4 wine rows + L3).
  Loud-skip path (WIN31_DECOMP=/nonexistent) prints the banner + exits 0 (L1+L3
  still run; L2 never silent-passes).
- `make test-region-gdi-mutant`: all four mutants RED -- GDI_MUTATE_DISPATCH
  (L1+L2), RGN_MUTATE_EMIT_NOCHANGE (M-VALUE), GDI_MUTATE_RECTIN (RectInRegion vs
  wine), RGN_MUTATE_PARITY_OFF1 (L2 cross-redden).
- L3 negative control: planting a second `region_op` definition in os/ drove L3
  RED; removing it restored GREEN.
- Law 3: the wine reference is host-only -- referenced by no kernel/image rule;
  kernel region.o builds clean (-Werror, -ffreestanding).
- Full clean aggregate (`make clean && make test`): GREEN -- **256 host + 43 emu**
  gates (was 254 host; +2 = `test-region-gdi` + `test-region-gdi-mutant`). The
  kernel image still builds (region.o links; `_kernel_end` under the 0x40000 B0
  guard) and boots on QEMU; the GDI facade has no kernel call site yet (FO-D2-8).
- ASCII-clean (Rule 12); all source + my doc edits.

## Pointers

- Spec/engine: `spec/region_algebra.h`, `os/flair/atkinson/region.c`.
- Oracle: `harness/proptest/test_region_gdi.c` + `gdi_ref_wine.{c,h}` +
  `gdi_ref_wine_shim.h` + `gdi_ref_winestubs/`.
- Design: `docs/adr/ADR-0005-AMENDMENT-AM-1-Dual-Heritage-Region-Spine.md`
  (Sec 3/5/7); C-7 in ADR-0005 Sec 5.1; the D-8 row in ADR-0004 Sec 3.8.
- Roadmap: `bd show initech-qipc`; NEXT = `initech-h714` (the canon module
  generator) -> `initech-mwpw` (test-color-canon value oracle, ORACLE-FIRST).
- Follow-up: `initech-rmsr` (FO-D2-8, exercise CombineRgn on metal).
- Prior: WL-0054 (the ratified design this begins implementing).

<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0059 -- the FLAIR era-layering registry (flair_skins.h) + skin oracles (epic initech-qipc step 6)

**Type:** implementation shard (step 6 -- the era/heritage axis as a data-only view over the one canon).
**Date:** 2026-06-23.
**Bead:** `initech-m6qx` (epic `initech-qipc` step 6).
**Commit:** (this shard's commit).

## Context

The chimera desktop carries a System-7 base skin AND a Win-3.1 peer skin (the
Office Space frame is both at once). Step 6 makes that an explicit, locked DATA
record -- the era/heritage axis -- WITHOUT forking the one color authority. Per
DEC-09 D-9/ARB-4 the registry is a VIEW over color_canon.h, never a fourth table.

## What changed (1 opus lane; I owned the Makefile)

- `spec/flair_skins.h` (NEW, LOCKED) -- a DATA-ONLY `flair_skin_t` (named color
  slots each carrying the palette idx + the canon RGB pulled BY INCLUSION from
  color_canon.h; era_id/heritage_id tags; scalar pinstripe/bevel-width). NO
  function pointers, NO draw code -- the TYPE forbids the engine fork D-9 kills.
  A `const flair_skin_registry[]` holds the ERA_SYS7_0_1/QUICKDRAW base row + the
  ERA_WIN31/GDI peer row; ERA_SYS8_PLATINUM is a reserved enum with ZERO rows.
  `flair_skin_resolve(era, heritage)` is TOTAL + FAIL-LOUD (the region engine's
  RGN_FAIL_LOUD idiom -> #UD -> PC LOAD LETTER in-kernel). Default (0,0).
- `harness/proptest/test_skin_teal.c` (`test-skin-teal`) -- authored-grade: the
  teal/bevel/win31 slots == the canon authored datum (by-inclusion). Mutant RED.
- `harness/proptest/test_skin_era_frozen.c` (`test-skin-era-frozen`) -- an FNV-1a-32
  accretion digest over the two LOCKED base rows == the committed
  SKIN_FROZEN_DIGEST 0xDEF099AC; ERA_SYS8_PLATINUM stays zero-rows. Enforces
  "accretion = APPEND, never MUTATE a base row." Mutant (a base-row edit) RED.
- `harness/proptest/check_win95isms.c` (`check-win95isms`) -- a grep gate
  forbidding #DFDFDF / COLOR_3DLIGHT in the FLAIR color sources (the flat-2D
  Win-3.1 target rejects the Win95 3D-light import; ARB-8/Law 3). Mutant RED.
- Makefile (orchestrator): the 6 gates (3 oracles + 3 mutants) wired into
  TEST_UNIT_GATES + .PHONY.

## Why

The era/heritage registry is the home for the chimera's two-skin co-residency
(D-9b), but it MUST be a view, not a fourth color authority (the consistency
critic's original finding -- four committees each invented a color module). A
data-only record referencing color_canon.h by inclusion makes "neither heritage
owns the color, and there is no second copy of any value" structurally true; the
accretion digest makes the locked base rows tamper-evident (append-only).

## Frictions / lessons

- **win31 btnshadow #808080 has no canon macro.** Rather than hand-type a
  duplicated value, the lane derived it from the canon's OWN gray-ramp formula
  (`flair_canon_rgb(idx>=9) == (idx<<16)|(idx<<8)|idx`) via a
  `FLAIR_CANON_GRAY_RGB(0x80)` macro + a `_Static_assert` proving equality. The
  ramp IS part of the canon authority, so this stays by-inclusion -- the only
  6-hex literals in the header are that proof + a comment, never a slot value.
  LESSON: "by-inclusion" can mean "the canon's formula applied to an index," not
  only "a named macro" -- but it must be PROVEN equal, never assumed.
- **check-win95isms must not exempt comments.** The scanner flagged the forbidden
  tokens written into the header's own banner; the fix is to forbid the import
  WITHOUT typing the tokens (they live only in the scanner's FORBIDDEN[] table).

## Acceptance (Law 2)

- flair_skins.h compiles hosted AND freestanding (-m32 -ffreestanding -Werror).
- test-skin-teal / test-skin-era-frozen / check-win95isms GREEN (fresh builds);
  all 3 mutants RED (a canon-drifted slot; a mutated base-row field; a planted
  #DFDFDF).
- By-inclusion verified: ZERO hand-typed canon value in any registry slot.
- Full clean aggregate (`make clean && make test`): GREEN -- **269 host + 43 emu**
  (was 263; +6 = the 3 skin oracles + their mutants). Purely additive -- no
  existing source / render / kernel touched.
- ASCII-clean (Rule 12).

## Pointers

- Registry: `spec/flair_skins.h`. Oracles: `harness/proptest/test_skin_teal.c`,
  `test_skin_era_frozen.c`, `check_win95isms.c`.
- Design: ADR-0004-AMENDMENT-DEC-09 (D-9/D-9b Sec 3.7, ARB-4 Sec 3.3), ADR-0006
  (the skin oracles).
- Roadmap: `bd show initech-qipc`; NEXT = `initech-5l5z` (step 7, the FINALE: the
  live cooperative WaitNextEvent event loop + behavioural grading, ADR-0006,
  behind -DBOOT_FLAIR_LIVE).
- Prior: WL-0058 (C-8 mechanism/policy enforcement).

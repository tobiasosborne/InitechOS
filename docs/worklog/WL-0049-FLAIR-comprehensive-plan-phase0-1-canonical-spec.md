# WL-0049 -- FLAIR comprehensive plan + Phase 0/1 canonical-spec consolidation

beads: epics initech-v94x (Phase 0, 3/4) + initech-dh5k (Phase 1, 7/7) +
re30/4e35/t4hp (Phase 3/4/6 filed); children v94x.1/.2/.3 + dh5k.1-.7 closed.

## Context

Operator opened the FLAIR consolidation + expansion: turn FLAIR from a green
host-tested Toolbox library into a live, era-authentic GUI that hosts serious
era apps. A 7-lane "understand" workflow (wf_e7335f49-a55: impl / spec+ADR /
plan+beads+gates / system7-decomp / win31-decomp / PRD+frame / app-hosting-gap,
+ a completeness critic) mapped the state. Headline finding: **FLAIR does not
boot** -- only surface.o links into the kernel; the Manager/imaging stack is
host-rendered to PPM, never on the 386. The locked spec is also stale vs the
now-finished sister corpora, and parts of grafport.h/imaging.h are aspirational
decoration (no drawing verbs, grafProcs NULL-only).

Operator rulings folded into the plan: Doom = flat-32 SOURCE PORT (NO DPMI --
the OS is already 32-bit flat); Minecraft = native "Initech Mines" (literal JVM
out by Law 3 + ADR-0001); M5 native frame apps before the hosting arc; the
canonical productivity suite comes first -- InitechBase (SAMIR, done-ish),
Initech 123 (Lotus), InitechWord (WordPerfect); and the ERA AXIS -- build
System 7.0/7.1 now, ACCRETE System 8 Platinum later ("never delete, always
accrete"; the Office Space frame is likely Platinum, not Sys7).

Then a switch to ORCHESTRATION mode (operator: delegate coding to subagents
<=6 sonnet / <=2 opus; orchestrator owns grading + Makefile + commit + beads;
committee for serious forks).

## What changed

**Plan + DAG (orchestrator):**
- docs/plans/FLAIR-implementation-plan.md -- the comprehensive 7-phase plan
  (Phase 0 reconcile -> 1 canonical spec -> 2 goldens/oracles -> 3 boot
  integration -> 4 app contract ADR-0010 -> 5 toolbox+M5 frame apps -> 6
  app-hosting ADR-0011), dependency-ordered, era-layered, with DoD + oracles.
- Bead DAG: phase epics initech-v94x/dh5k/re30/4e35/t4hp + the true
  critical-path chain (re30 dep dh5k; 4e35 dep re30; t4hp dep 4e35).
- DAG cleanup: superseded the stale legacy M4 epic initech-8oi + M3 epic
  initech-ox7 with the live initech-k8o5 (bd ready is honest now).

**Phase 1 canonical spec (Wave 1, 6 sonnet lanes, disjoint files):**
- spec/control_record.h (ControlRecord + part-codes + CDEF proc IDs + a
  confined Win flat-button accent sub-section) -- test-control-record (77).
- spec/menu_record.h (MenuInfo + enableFlags + MenuSelect result word) --
  test-menu-record (101).
- spec/dialog_record.h (DialogRecord + DITL item-type bytes + alert stages +
  ok/cancel=1/2) -- test-dialog-record (87).
- spec/drawing_ops.h (the QuickDraw OPERATION semantics: 5 verbs/GrafVerb 0..4,
  CopyBits maskRgn/scale/colorize, half-open + local/global coords, pattern
  phase-lock) -- test-drawing-ops (128).
- spec/assets/clut.json + clut.h (the real 256-entry indexed-8 CLUT derived
  from ../system7-decomp goldens/resources/clut_8_rom.bin) -- test-clut (1047,
  incl. an entry-by-entry ROM diff that loud-skips if the golden is absent).
- chrome_metrics.json/.h: ingested the corpus-RESOLVED chrome RGBs out of
  golden_resolves (pinstripe #F3F3F3/#969696 + bevel #DADAFF/#B3B3DA, close/zoom,
  grow DrawGrowIcon, scrollbar thumb/track, win31 SM_CYCAPTION=18, SM_CXVSCROLL
  ~15 with 17px refuted), era-tagged, schema v2->3, .h==.json consistency held.
- spec/chimera_element_map.json: P0-a Law-3 fix #dfdfdf -> #ffffff (the Win95
  COLOR_3DLIGHT value the accent corpus forbids), schema 1->2.
- spec/CANON-MANIFEST.md (all 88 corpus specs mapped: in-scope projection vs
  out-of-scope vs future) + spec/win95ism_guardrails.md (the locked forbidden-
  Win95-ism checklist from cross-version-guardrails.md + a proposed grep gate).
- All headers era-tagged (system7.0-7.1 / win31-accent) for the Sys8 accretion.

**Phase 0 reconciliation (Wave 2):**
- CLAUDE.md: the MZ-deferred hallucination callout updated -- InitechMZ flat-32
  .EXE SHIPS (ADR-0003-DEC-08a); no Law/Rule text touched.
- InitechOS-PRD.md: 3 reconciliation notes (MZ shipped; M6 = dBASE III+ 1.1
  per ADR-0008).

**Makefile (orchestrator):** 5 new host gates + mutants wired into
TEST_UNIT_GATES (test-control-record/menu-record/dialog-record/drawing-ops/clut,
each +mutant), mirroring test-flair-headers. 244 -> 254 host gates.

## Why

Phase 1 is the red-green TDD TARGET for everything downstream: the boot
integration (Phase 3) and every app (Phases 5-6) are built and verified against
this canonical, era-layered spec. Consolidating the now-complete corpora and
clearing the drift first means later waves cite a single trustworthy contract.

## Frictions (orchestrator grading caught these -- Law 4; never trust the report)

- The host gates compile with native `cc` (CC?=cc, 64-bit), NOT -m32: 32-bit
  libc (gcc-multilib) is absent on this box. Kernel -m32 codegen still works
  (make image green); only 32-bit *libc* is missing -> host gates are native.
- test_assert.h lives in seed/ -> menu/dialog gates needed -Iseed.
- The menu + dialog mutant hooks were COMMENT-ONLY decoration (the -D macros
  existed only in header comments) -> the mutants PASSED. Orchestrator wired a
  real biting CHECK under each macro (Rule 6). Caught only by re-running the
  mutants -- exactly the grading discipline.
- drawing_ops test includes "spec/drawing_ops.h" (repo-root-relative) -> needed
  -I. ; flipping a -D flag does not rebuild the binary -> rm the stale mutant.

## Acceptance

- `make clean && make test-unit` = **ALL GREEN, 254 host gates**.
- All 5 new gates green; all 5 mutants BITE (verified by re-running each).
- test-chrome .h==.json consistency green after the RGB ingest; its 3 mutants
  still bite. `make image` green (1474560 bytes) -- chrome_metrics change clean.
- All new/changed sources ASCII-clean (Rule 12); no Laws/Rules altered.
- Emu gates UNAFFECTED (no kernel/EXEC-path change this wave -- none of the new
  headers link into the kernel image yet; that lands in Phase 3, where the full
  `make test` incl. emu + Bochs is the obligation).

## Open / next

- v94x.4 (P0-d frame-fixture path) DEFERRED: needs care re: gitignore/copyright
  of preview.webp before copying into fixtures/.
- dh5k.8 (P1-8 resource binary formats) not yet filed -- author as apps demand.
- Next wave = Phase 2 (wire SYSTEM7_DECOMP/WIN31_DECOMP + need_flair_goldens
  resolver; first pixel-diff oracle; SSIM tool) and/or Phase 3 pre-step (the
  GrafPort verb layer + clip/CLUT unification) -- the latter is opus/serial on
  os/flair/ shared files and likely warrants a committee read on grow-impl-to-
  meet-spec vs trim-spec.

## Pointers

- Plan: docs/plans/FLAIR-implementation-plan.md
- New spec: spec/{control_record,menu_record,dialog_record,drawing_ops}.h,
  spec/assets/clut.{json,h}, spec/CANON-MANIFEST.md, spec/win95ism_guardrails.md
- Gates: Makefile (the FLAIR Phase 1 block after test-flair-headers ~line 8721);
  TEST_UNIT_GATES.
- Beads: initech-v94x (Phase 0), initech-dh5k (Phase 1), initech-re30/4e35/t4hp.

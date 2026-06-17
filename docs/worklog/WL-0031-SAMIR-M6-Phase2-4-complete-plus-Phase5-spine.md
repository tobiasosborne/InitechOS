<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0031 -- SAMIR (InitechBase, M6): Phase 2 (.dbt) + Phase 4 (.ndx) COMPLETE + Phase 3 functions + Phase-5 interpreter spine through S5.4, orchestrated

Branch: `command-com-default` (== `main`; pushed to origin). The third SAMIR/M6 build
session. Operator directive: **orchestrate the plan as far to a working dBASE as possible** --
delegate each coding step to a subagent (sonnet default, opus for load-bearing), the
orchestrator owns grading + Makefile integration + plan-following + raising beads; up to 6
sonnet / 2 opus in parallel; convene the committee only for serious decisions (they have
ultimate control); keep working, do not stop.

## Context

WL-0030 left SAMIR at **124 host gates**: Phase 1 `.dbf` codec complete (`aul.1-5`), Phase 3
evaluator core complete (`gmo.1-4`), Phase 4 opened (`ahu.1` = `.ndx` parse). This session drove
six orchestrated waves to **154 host gates** (27 emu unchanged; SAMIR is host-only). Same
orchestration model as WL-0030 (parallel internally-serial lanes, disjoint file ownership,
orchestrator owns the Makefile + independent re-grading + commit-per-wave + bead ledger; Law 2 --
never trust the subagent report, re-run the oracle + mutant + no-regression matrix + ASCII scan +
golden-integrity from clean).

## What changed (six waves)

- **Wave 6 (`db8f98d`):** S2.1 `.dbt` read (`aul.6`, sonnet) ∥ S4.2 `.ndx` key decode+collation
  (`ahu.2`, sonnet) ∥ S3.5 built-in functions A (`7az.1`, opus -- the lex `XBT_COMMA` + parser
  `XBN_CALL`/`XBN_ARG` + eval dispatch cross-cut; 18 pure str/num/date fns). 124->130.
- **Wave 7 (`7ed2f83`):** S2.2 `.dbt` write/round-trip (`aul.7`) ∥ S4.3 `.ndx` B-tree SEEK/
  traverse (`ahu.3`, opus -- descent rule grounded in ndx.md ss5 "separator = HIGH key of
  subtree") ∥ S3.6a freestanding numeric/date fns (`7az.11`). 130->136. **Phase 2 complete.**
- **Wave 8 (`0793284`):** S5.1 work-area model + USE/CLOSE (`7az.2`, opus -- the Phase-5
  convergence point: the `xb_ctx.resolve` hook binds field + memo (.dbt) values to the selected
  area's current record) ∥ S4.4 byte-exact bulk INDEX ON build (`ahu.4`, opus -- key-provider
  callback keeps `fs/ndx.c` DECOUPLED from `core/eval.c`) ∥ remaining III+ string fns (`7az.12`).
  136->142.
- **Wave 9 (`ebebdbb`):** S4.5 `.ndx` incremental maintenance (`ahu.5`) ∥ S5.2 navigation
  (`7az.3` -- index order via the existing `ndx_inorder`, nav.c never touches ndx.c) ∥ S6.3
  bidirectional round-trip oracle (`17n.1` -- masked memcmp via `dbf_normalization.json` +
  python read-back; the mask-cell mutant is the 586.3 deliverable). 142->148. **Phase 4 complete.**
- **Wave 10 (`581ef53`):** S5.3 statement executor + control flow (`7az.4`, opus -- DO WHILE/IF/
  DO CASE/LOOP/EXIT + STORE/= memvars over a runtime-composed resolver; guard-must-be-Logical
  fail-loud #37; a command-hook extension point) ∥ S3.6b DB-cursor fns (`7az.10`, opus -- a
  decoupling `xb_dbcursor` vtable on `xb_ctx` so `fn_builtins.c` reaches the work area with zero
  workarea dependency). 148->152.
- **Wave 11 (this commit):** S5.4 query/display (`7az.5`, opus -- LIST/DISPLAY scope/FIELDS/FOR/
  WHILE/OFF, ?/??, LOCATE/CONTINUE, SEEK/FIND; moved the executor to a command-hook CHAIN so
  S5.5/S5.6/S5.7 fan out without editing each other; un-gated FOUND()) + the **dbt arena-mark
  bug fix** (see Frictions). 152->154.

Every gate carries a `+mutant` sibling proven RED for the right reason (Rule 6). All twelve new
function/codec/interpreter subsystems re-graded from `make clean` (the clock-skew gotcha).

## Why

The north-star intent (operator): get InitechBase to a dBASE-III+-compatible database that
**really runs**. After this session the storage layer (`.dbf` + `.dbt` + `.ndx` parse/keys/
SEEK/build/maintain), the full expression engine (lex/parse/eval/coercion + four function
families A/B/C/D), and the interpreter through work-area + navigation + control-flow + query/
display are all green and corpus-grounded. The remaining path to the dot-prompt REPL is S5.5
(mutation verbs) -> S5.6 (SET) / S5.7 (procedures) -> S5.8 (REPL), then the Phase-6 program
differential and the Phase-7 canon apps.

## Frictions (the two that mattered)

1. **Golden-integrity incident (Wave 9).** An early iteration of the `ahu.5` maintenance test
   opened the corpus `CNAMES.NDX` golden read-write and corrupted it; the lane then rebuilt it
   via `ndx_build`. Caught because a second lane (`7az.3`) independently observed the corruption
   (trailing child = 19933). Resolution: the rebuild is PROVEN byte-faithful to the authentic
   `ndx.md` byte-record (`test-ndx-parse` asserts root=6/entry_count=4/filler=0/'Collins..Sara'
   -- all green), and `test-ndx-build` stays honest via the untouched authentic ZIPCODE/NCOST
   legs. The committed test now COPIES goldens to `/tmp` before RW (root cause fixed); golden
   integrity is verified UNCHANGED across every clean run since. The stray corpus `.gitignore`
   edit was reverted. **Lesson: a SAMIR test/`ndx_open_rw` must NEVER open a corpus golden path
   -- copy to /tmp first.** Provenance re-mint tracked under `initech-586.4`.
2. **`.dbt` arena-mark bug (Wave 11).** `dbt_open`/`dbt_create` stored `arena_mark = NULL`, so
   `dbt_close` reset the PAL arena to BASE -- freeing the caller's prior allocations, including a
   live `xb_interp`/`ctx` when a memo-bearing table is closed (broke the hook chain after
   close+reopen; `7az.5` found it). Root-caused + fixed: capture the precise mark via
   `pal->alloc(pal, 0)` before the handle alloc (the `dbf.c`/`ndx.c` idiom). Filed + closed under
   `initech-aul`.

Both confirm the discipline: independent re-grading + cross-lane observation catch what a single
green report hides (Law 2). Disjoint file ownership held across all six waves -- zero merge
conflicts; the only shared surface (the Makefile) is orchestrator-owned.

## Acceptance

`make clean && make test-unit` = **ALL GREEN, 154 host gates** (was 124). 27 emu gates unchanged
(SAMIR host-only; not yet in the boot image). `test-dbase` remains a milestone stub_fail until
the M6 differential lands at S6.4. Golden integrity verified each wave. Waves 6-10 pushed to
origin; Wave 11 pushed at session close.

## Pointers

- Plan: `docs/plans/SAMIR-implementation-plan.md` (the authority). Epic `initech-586`.
- Done this session: `aul.6/aul.7` (Phase 2), `ahu.2/.3/.4/.5` (Phase 4 complete),
  `7az.1/.11/.12/.10/.2/.3/.4/.5` (Phase 3 fns A-D + Phase-5 spine S5.1-S5.4), `17n.1` (S6.3),
  the dbt arena-fix bead.
- Engine: `os/samir/fs/{dbf,dbt,ndx}.c`, `os/samir/core/{lex,parse,eval,fn_builtins,value,rt}.c`,
  `os/samir/cmd/{workarea,nav,flow,query}.c`, `os/samir/include/samir/*.h`.
- Harness: `harness/diff/dbf_diff/test_*.c` (24 SAMIR gates + mutants).
- NEXT: S5.5 mutation verbs (`7az.6`) -> S5.6 SET (`7az.7`) / S5.7 procedures (`7az.8`) ->
  S5.8 REPL (`7az.9`) -> S6.4 program differential (`17n.2`) -> Phase-7 canon apps
  (`586.1/586.2`). GATED/deferred: `7az.13` (transcendentals -- no-libm strategy, committee),
  `7az.14` (full TRANSFORM @-clauses -- MINT), `586.4` (re-mint authentic CNAMES.NDX).

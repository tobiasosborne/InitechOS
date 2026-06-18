<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0032 -- SAMIR (InitechBase, M6): interpreter COMPLETE + M6 `test-dbase` GREEN + canon apps, orchestrated

Branch: `command-com-default` (== `main`; pushed to origin). Continuation of the WL-0031
orchestration session (same model: parallel internally-serial lanes, disjoint file ownership,
orchestrator owns the Makefile + independent re-grading + commit-per-wave + bead ledger; Law 2 --
the oracle is truth, every lane re-graded from clean, never the subagent report).

## Context

WL-0031 left SAMIR at **154 host gates**: storage (`.dbf`/`.dbt`/`.ndx` parse/keys/SEEK/build/
maintain), the expression engine core, function families A/B, and the Phase-5 interpreter spine
through S5.4 (work-area, navigation, statement executor, query/display). This session drove ten
more waves to **174 host gates** and -- the headline -- turned the **M6 `make test-dbase`
milestone from a stub_fail to GREEN**: InitechBase now passes its own mechanical oracle (the
bidirectional round-trip + the xBase program differential, 100%). InitechBase **really runs** at
the dot prompt, the M6 north-star behavior.

## What changed (waves 12-21)

- **Wave 12 (`3aa3780`):** S5.5 mutation verbs (`7az.6` -- REPLACE/APPEND/DELETE/RECALL/PACK/ZAP
  with assignment-coercion + live master-key index re-file + the `wa_adopt_table`/`wa_refresh`
  writable seam) ‖ S5.6 SET state (`7az.7` -- EXACT->ctx, ORDER->wa_set_order, rest stored).
- **Wave 13 (`1e53f30`):** S5.7 procedures + scope + I/O (`7az.8` -- DO/PROCEDURE/PARAMETERS/
  RETURN, PUBLIC/PRIVATE downward-stacking scope, ACCEPT/INPUT/WAIT, ON ERROR; re-entrant DO).
- **Wave 14 (`eccf450`):** S5.8 dot-prompt REPL (`7az.9` -- `samir_repl` registers all four
  command modules; read->parse->execute->render via the 151-code catalog; QUIT/EOF). **Phase 5
  COMPLETE.**
- **Wave 15 (`66c52df`):** writable USE (`7az.16` -- `dbf_open_rw` + `wa_set_open_rw`; also fixed
  a latent flush-offset bug for `+2`-terminator III+ files).
- **Wave 16 (`b3ddd97`):** S6.4 xBase program differential (`17n.2`) -- a 7-program `.prg` corpus
  + authored Tier-0 goldens + driver; **`test-dbase` milestone flipped to a real GREEN aggregating
  umbrella** (round-trip + program diff). M6 oracle green end-to-end.
- **Wave 17 (`f0b0e68`):** full TRANSFORM() picture/function engine (`7az.14` -- numeric pictures,
  `@(`/`@X`/`@C`/`@B`, char pictures; GATED `@`-clauses loud-skip).
- **Wave 18 (`d2aeab2`):** REPL USE -> `wa_set_open_rw` (`7az.19` -- the dot prompt now opens
  tables for editing; plain USE -> REPLACE persists to disk).
- **Wave 19 (`94e116d`):** SET DATE/CENTURY -> DTOC/CTOD formatter wiring (`7az.15`).
- **Wave 20 (`b049e8a`):** canon -- Initech AR accounting app + the enforced Y2K bug (`586.1`).
- **Wave 21 (`4a984bb`):** canon -- Michael Bolton's salami-slicing rounding virus (`586.2`).

Every gate carries a mutation-proven `+mutant` sibling (Rule 6); all re-graded from `make clean`.
The two canon apps (Law 4) use an inverted mutant: the mutant is the bug *fixed*, which must go
RED against the canon golden -- **a "fix" breaks the gate**, so the deadpan bugs are enforced, not
decoration.

## Why

The operator directive was to get InitechBase "as fully done as possible." After this session the
engine is feature-complete on the host for III+ 1.1: a dBASE-III+-compatible database that boots a
`.` prompt, USEs/edits/indexes/queries tables, runs `.prg` programs with full control flow +
procedures + scope, honors SET state, and passes a 100% program differential against authored
goldens. The two canon apps deliver the Office-Space deadpan (Y2K, salami slicing) as genuine,
enforced bugs.

## Frictions (Law-2 catches that mattered)

1. **Law-1 grounding error caught in the SET-formatter lane (wave 19).** `7az.15` wired bare
   `STR(n)` to honor SET DECIMALS. The orchestrator's re-grade traced this to the authoritative
   `../dbase3-decomp/specs/runtime/numeric-and-string-formatting.md`: **[verified]** STR()'s
   default decimals is **0** always, and SET DECIMALS's scope is division/SQRT/LOG/VAL/computed-
   display -- **not** STR (corroborated by mint-001 `STR(-570,4)='-570'`). Reverted to the verified
   contract (STR ignores SET DECIMALS; the oracle now asserts `STR(3.14159)='         3'` across
   SET DECIMALS 4/0), kept the correct SET DATE/CENTURY wiring, filed the SET-DECIMALS-division
   follow-up. **A subagent's green report is not truth; the corpus is.**
2. **Harness liveness defect (wave 16).** The program-differential driver printed its
   `N checks, M failures` summary to stderr on failure, so the mutant gate's
   `2>/dev/null | grep -q 'checks,'` liveness check failed ("harness dead, RED meaningless"). The
   subagent's own self-check used a weaker assertion and missed it. Fixed at source (summary to
   stdout in both cases) so the mutant gate genuinely bites.
3. (Recap from WL-0031, both resolved earlier this session.) **Golden-integrity incident**: an
   `ahu.5` test iteration corrupted the corpus `CNAMES.NDX`; the rebuild is proven byte-faithful
   to the `ndx.md` record, the committed test now copies goldens to /tmp, re-mint tracked in
   `586.4.1`. **`.dbt` arena bug** (`aul.8`): `dbt_close` reset the arena to base; fixed with the
   `pal->alloc(pal,0)` precise-mark idiom.

Disjoint file ownership held across all ten waves; the only shared surface (the Makefile) is
orchestrator-owned. Two subagent runs dropped on infra connection errors (the 586.2 retry + the
7az.15 corrective continuation) -- handled by verifying the tree state and re-dispatching/finishing.

## Acceptance

`make clean && make test-unit` = **ALL GREEN, 174 host gates** (was 154 at WL-0031; 124 at the
session start). `make test-dbase` = **GREEN** (round-trip + program differential, 100%; was a
milestone stub_fail). 27 emu gates unchanged (SAMIR is host-only; not yet in the boot image).
Golden integrity verified UNCHANGED across every clean run. All 21 waves pushed to origin.

## Pointers

- Plan: `docs/plans/SAMIR-implementation-plan.md`. Epic `initech-586`.
- Engine: `os/samir/fs/{dbf,dbt,ndx}.c`, `os/samir/core/{lex,parse,eval,fn_builtins,value,rt}.c`,
  `os/samir/cmd/{workarea,nav,flow,query,mutate,set,proc}.c`, `os/samir/samir_main.c` (REPL),
  `os/samir/include/samir/*.h`.
- Harness: `harness/diff/dbf_diff/test_*.c` + `xbase_prog_diff/` (S6.4) + `canon/` (S7.1/S7.2).
- Done this session: `7az.6/.7/.8/.9/.14/.15/.16/.19/.10` (S5.5-S5.8 + fns + writable USE + REPL
  RW + SET-formatter), `17n.2` (S6.4 -> test-dbase GREEN), `aul.8` (dbt fix), `586.1`/`586.2`
  (canon). Phases 1-7 substantially complete on the host.
- **REMAINING (all GATED / deferred):**
  - `7az.13` -- transcendentals SQRT/LOG/EXP (no-libm strategy: x87-asm vs polynomial approx --
    **committee-worthy**; partly GATED on MINT for numeric edges).
  - `7az` SET-DECIMALS-division -- wire SET DECIMALS into its verified scope (division/VAL/computed
    display, NOT STR); partly GATED on SQRT/LOG.
  - `7az.17` (commands.h consolidation), `7az.18` (mutate.c #41->#111) -- small cleanups.
  - GATED TRANSFORM `@`-clauses + numeric/date MINT items (MOD-sign, INT-on-neg, ROUND-tie,
    ITALIAN/FRENCH dates) -- need a dosbox-x MINT session against real dBASE III+ 1.1.
  - `586.4`/`586.4.1`/`17n.3` -- Tier-2 real-`DBASE.EXE` authenticity minting + re-mint authentic
    CNAMES.NDX (needs dosbox-x + the sister mint harness).
  - `0tl` (@SAY/GET forms) + `ax9` (FLAIR text-console window) + S8.x (`pal_milton`, FPU-ready,
    SAMIR as a flat `.COM` on Milton) -- gated on M4 (FLAIR) / the boot image / `spec/hardware.json`.

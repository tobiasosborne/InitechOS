<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0030 -- SAMIR (InitechBase, M6): Phase 1 .dbf codec COMPLETE + Phase 3 core (gmo) COMPLETE + Phase 4 OPENED, orchestrated

Branch: `command-com-default` (`main` fast-forwarded to the tip each landing; both pushed to
origin). The second SAMIR/M6 build session: drove the engine forward from the Phase-0
foundation through the entire `.dbf` codec, the full expression evaluator core, and the first
`.ndx` index step -- delegated step-by-step to subagents, graded + integrated + committed by the
orchestrator.

## Context

WL-0029 left SAMIR with the Phase-0 foundation green (PAL, rt, value, locked `spec/samir/`,
the independent `dbf_ref.py`/`ndx_ref.py` readers; `make test-samir` = 7 sub-gates; 104 host
gates). The operator directed: **orchestrate the plan** -- delegate each coding step to a
subagent (sonnet default, opus for load-bearing), the orchestrator owns grading + Makefile
integration + plan-following + raising beads; up to 5 sonnet / 2 opus in parallel. The plan
(`docs/plans/SAMIR-implementation-plan.md`) is the authority; `bd` is the ledger.

Orchestration model used: **parallel serial lanes**. Each lane is an internally-serial chain;
lanes run concurrently because they own DISJOINT files and NEVER touch the Makefile (the
orchestrator wires every gate). Per landing the orchestrator INDEPENDENTLY re-grades (re-runs
the oracle + mutant + no-regression matrix + freestanding compile + ASCII scan + cross-checks
against the independent python readers -- Law 2/4, never trusts the subagent report), wires the
Makefile gate, runs `make clean && make test-unit`, commits, closes the bead, fast-forwards
`main`. Five waves landed:

- **Wave 1:** S1.1 `.dbf` header+invariants (`aul.1`, opus) ∥ S3.1 lexer (`gmo.1`, sonnet).
- **Wave 2:** S1.2 field descriptors (`aul.2`, sonnet) ∥ S3.2 precedence parser (`gmo.2`, opus).
- **Wave 3:** S1.3 record read (`aul.3`, sonnet) ∥ S3.3 evaluator+coercion (`gmo.3`, opus).
- **Wave 4:** S1.4 write/round-trip (`aul.4`, opus) ∥ S3.4 coercion fuzzer (`gmo.4`, sonnet).
- **Wave 5:** S1.5 mutation verbs (`aul.5`, sonnet) ∥ S4.1 `.ndx` parse (`ahu.1`, sonnet).

## What changed

### Phase 1 -- the `.dbf` codec (COMPLETE: S1.1-S1.5, epic `aul` part 1)
`os/samir/fs/dbf.c` + `os/samir/include/samir/dbf.h` -- a freestanding III+ 1.1 `.dbf`
reader/writer over the PAL:
- **S1.1** `dbf_open`: 32-byte header parse via the LOCKED `spec/samir/dbf_format.h` offsets;
  fail-loud on IV version bytes; nfields + `+1`/`+2` terminator by scan-to-`0x0D`; invariants
  1/1b/2. Invariant 2 admits trailing ghost data (`file_size >= body`) per the documented BANK
  not-truncated empty-table case (dbf.md ss8/9) while rejecting truncation.
- **S1.2** `dbf_field`: 32-byte field-descriptor decode (name-to-NUL, type C/N/D/L/M fail-loud,
  length, dec).
- **S1.3** `dbf_read_rec`: per-field decode to typed `xb_val` (C/N/D/L + M block-ptr) + delete
  flag. Blank date/logical -> `xb_u`; M text resolution deferred to S2.1.
- **S1.4** `dbf_create`/`dbf_append_rec`/`dbf_flush`: byte-DETERMINISTIC write (injectable date,
  NORMALIZE bytes -> 0 per `dbf_normalization.json`, version `0x83` iff memo, `+1` form);
  bidirectional round-trip (write-twice byte-identical; C + `dbf_ref.py` read-back; golden
  masked-cmp).
- **S1.5** `dbf_append_blank`/`dbf_replace`/`dbf_delete`/`dbf_recall`/`dbf_pack`/`dbf_zap`:
  assignment-coercion (C truncate/pad, N stars-fill, cross-type mismatch); deterministic PACK
  (survivors in original order); explicit 1-based recno cursor.

### Phase 3 core -- the expression engine (gmo epic COMPLETE: S3.1-S3.4)
`os/samir/core/{lex,parse,eval}.c` + `os/samir/include/samir/eval.h`:
- **S3.1** `xb_lex`: III+ tokenizer (C/N/dotted-logical literals, identifiers, all operators incl.
  two-char, `$`); rejects IV-only `==`/`!=`/`%` fail-loud.
- **S3.2** `xb_parse`: precedence-climbing AST with the corpus-MINTED dBASE rules -- `^`/`**`
  LEFT-associative (`2^3^2 = 64`), unary minus binds TIGHTER than `^` (`-2^2 = 4`), NON-standard
  vs math (mint-002 closes the static spec's `[oracle-resolves]`).
- **S3.3** `xb_eval`: post-order evaluator hardcoding the LOCKED `xbase_coercion.json` dispatch in
  C -- every cell incl. the III+ HAZARD `C+N -> "Data type mismatch." (#9, NOT auto-stringified)`,
  SET EXACT begins-with vs blank-pad, D+N/D-N/D-D arithmetic, blank-date-high ordering, `$`,
  `.AND./.OR./.NOT.` requiring L. `xb_ctx` carries `set_exact` + a pluggable identifier resolver
  (Phase 5 binds work-areas) + a C-concat scratch arena.
- **S3.4** `harness/diff/dbf_diff/dbf_coerce_fuzz.c`: a seeded property-test differential of the
  real evaluator vs a table-driven reference (directed all-34-cells + 2000-seed sweep) with
  structured signal + shrink-to-minimal + replay seed -- the gmo deliverable.

### Phase 4 -- the `.ndx` index (OPENED: S4.1, epic `ahu`)
`os/samir/fs/ndx.c` + `os/samir/include/samir/ndx.h` -- `ndx_open` + node-read parse the 10-field
header + 2+2 node header + {child-page, dbf-recno, key-bytes} group array via the LOCKED
`ndx_format.h`. Key expression VERBATIM (cap 100, NOT lowercased -- ndx.md ss2.1 resolved).
Structure only; typed key decode is S4.2.

### Harness / gates
New host oracles in `harness/diff/dbf_diff/`: `test_dbf_header/fields/read/roundtrip/mutate.c`,
`test_xbase_lex/parse/eval.c`, `dbf_coerce_fuzz.c`, `test_ndx_parse.c` -- each with a `-mutant`
sibling (Rule 6). All wired into `TEST_UNIT_GATES`: **104 -> 124 host gates (+20)**. The 27 emu
gates are unchanged (SAMIR is host-only, not yet in the boot image).

## Why

The plan's bet: the engine touches the OS only through the PAL, so Phases 0-7 are fully
host-developable and gradable at full speed against the sister corpus `../dbase3-decomp` (the
grader). This session realized that: every step was graded on the host against (a) Tier-0 manifests
transcribed from the corpus byte-dumps, (b) Tier-1 golden parse/round-trip, and (c) the independent
`dbf_ref.py`/`ndx_ref.py` readers -- the oracle-independence barrier. The `.dbf` codec + the
evaluator are the two load-bearing halves the interpreter (Phase 5) converges on; both are now
complete (codec) or core-complete (evaluator).

## Frictions / lessons

1. **The full-aggregate re-grade caught a cross-cut the per-step self-grade could not.** When S1.3
   added `dbf_read_rec` (which calls `xb_*`), `dbf.c` gained a dependency on `value.c` -- so the
   EXISTING `test-dbf-header`/`-fields` gate links (which did not list `value.c`) would fail to
   LINK under `make`. The subagent's self-grade passed (it linked value.c); only the orchestrator's
   `make clean && make test-unit` exposed it. Fix: added `$(SAMIR_VALUE_SRC)` to every dbf gate
   link. Lesson reaffirmed: ALWAYS run the full aggregate from clean at integration, not just the
   new gate (WL-0028 host-oracle-hang echo).
2. **Disjoint-file + orchestrator-owns-Makefile makes parallel lanes race-free** (WL-0029 echo).
   Two lanes edited the tree concurrently for 5 waves with zero conflict because each owned a
   disjoint file set and compiled only to private `/tmp` binaries (never `make`, never `build/`).
3. **The hallucination guardrails earned their keep.** S3.2's prompt explicitly warned against
   standard math precedence; the subagent confirmed the NON-standard dBASE rules against mint-002
   (`^` left-assoc, unary>`^`) and flagged that the static spec left them `[oracle-resolves]`.
   S3.3's prompt flagged the `C+N` auto-stringify HAZARD; the evaluator errors #9 as required.
4. **Subagent boundary decisions are real and must be surfaced, not buried.** S1.3 (blank
   date/logical -> `xb_u`; M block-ptr representation), S3.3 (div-by-zero -> #39; blank-date JDN<=0
   sentinel), S4.1 (`dummy`/`reserved`/`unique_flag` byte semantics) all made defensible,
   documented, corpus-grounded choices and flagged the genuinely-open ones GATED. Captured in the
   bead close-reasons.
5. **The Invariant-2 relaxation was scrutinized, not waved through.** S1.1 relaxed invariant 2 to
   `file_size >= body`; the orchestrator verified against the actual BANK bytes + dbf.md ss8/9
   before accepting -- a strict `==` would wrongly reject a genuine III+ file.

## Acceptance

- `make clean && make test-unit` = **ALL GREEN, 124 host gates** (was 104; +20 = 10 SAMIR oracles
  x unit+mutant). Every `-mutant` confirmed RED for the right reason; every prior gate
  no-regression.
- Every engine file (`dbf.c`, `ndx.c`, `lex.c`, `parse.c`, `eval.c`) compiles freestanding
  (`-m32 -ffreestanding -nostdlib`) and is ASCII-clean (Rule 12).
- SAMIR-written `.dbf` files read back correctly through the independent `dbf_ref.py`; `.ndx`
  parse agrees with `ndx_ref.py`.
- 6 commits (`d99d39d` S1.1, `5c35763` S3.1, `779b3f7` wave 2, `84a1df3` wave 3, `ed704b5` wave 4,
  `774c85a` wave 5); `main` == tip; both branches pushed to origin.
- Beads closed: `aul.1..5`, `gmo.1..4`, `ahu.1` (10 step beads). The `gmo` epic is fully discharged.

## Pointers / next

- **The plan is the authority:** `docs/plans/SAMIR-implementation-plan.md`. ADR: `docs/adr/ADR-0008`.
- **Remaining DAG:**
  - **Phase 2 `.dbt` memo** (`aul.6`=S2.1 read, `aul.7`=S2.2 write/round-trip) -- depends on
    S1.3/S1.4 (done). Resolves the S1.3 M-pointer -> text boundary.
  - **Phase 4 `.ndx`** (`ahu.2`=S4.2 key decode+collation -> `ahu.3`=S4.3 B-tree SEEK ->
    `ahu.4`=S4.4 bulk INDEX ON -> `ahu.5`=S4.5 incremental maintenance) -- S4.2 depends on S4.1
    (done) + rt; S4.4 depends on S4.3 + S3.3 (done).
  - **Phase 3 functions** (`7az`: S3.5 built-in fns A, S3.6 fns B [partial GATED numfn-1..8]) --
    S3.5 depends on S3.3 (done); needs the parser to parse call args (S3.2 left `XBN_CALL` a
    placeholder).
  - **Phase 5 interpreter** (`7az`: S5.1-S5.8) -- the convergence point; needs Phases 1,2,3,4. The
    REPL `samir_main.c` is S5.8.
  - **Phase 6 oracle assembly** (`17n`/`586.4`: S6.3 round-trip, S6.4 .prg differential -- the M6
    `test-dbase` gate goes green here), **Phase 7 canon apps** (`586.1`/`586.2`), **Phase 8 OS
    integration** (`pal_milton.c`, boot SAMIR -- GATED on M2-stable/M4-FLAIR).
- **Orchestration cadence (operator-set):** delegate each step to a subagent; orchestrator owns
  disjoint file ownership + Makefile integration + independent re-grading + commit-per-wave + the
  bead ledger. Run `make clean && make test-unit` at every integration.
- **Standing dependency:** the sister corpus `../dbase3-decomp` (Tier-1 gates resolve
  `DBASE3_DECOMP`; absent -> loud-skip). Goldens present this session (11 `.ndx`, 11 `.dbf`).
- **GATED register (plan sec.7):** numfn-1..8 (S3.6), proc by-ref/PUBLIC (S5.7), incremental NDX
  split byte-exactness (S4.5), REPLACE-overflow policy (S5.5), FLAIR window seam (S8.3, needs M4),
  authenticity tier (S6.5, needs real DBASE.EXE), kernel x87 (S8.1). No agent guesses these.

---

*-- End of Shard WL-0030 --*

<!-- Tedium certified compliant with NFR-7. The 116% pie chart, the Y2K bug, and the rounding-error virus remain enforced canon. -->

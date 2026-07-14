<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0074 — B7 strings land by committee: the seed speaks ShortString

**Session:** 2026-07-14 (orchestrated: 3-seat design committee + 1 opus
implementation lane; orchestrator graded everything first-person on main)
**Commits:** `62ce0db` (B7, one commit, authored on lane branch
`b7-strings-lane` cut at `5d0d45d`, fast-forwarded) + the docs/beads commit
carrying this shard.
**Certificate:** `make clean && make test` — recorded below at Acceptance.

## Context

`bd ready` pointed at `initech-39k2` (B7 of docs/plans/TPS-M7-subset-plan.md
Sec 4): fixed/ShortString strings + temporaries, the last language-feature
bead before B8 (file-I/O RTL, `initech-ogxv`) and B9 (author Turbo Initech,
`initech-6m52`). ADR-0007's own ratification amendment fixes the order:
B7 before B8 because `assign()` takes a filename string.

## What changed

- **A three-seat design committee ruled B7's contested surface** (2 opus:
  Technical Soundness, Language/Self-Host Fitness; 1 sonnet: Oracle/
  Process; chair = orchestrator). No gridlock; two 2-1 splits chair-resolved
  with the majority (string compute lives in codegen-emitted intrinsics,
  NOT the RTL — start.asm frozen, DEC-05 conservative; array-of-string OUT,
  deferred with the B9 symbol-table consequence recorded). The full binding
  record (D1-D12) lives on the bead's design field.
- **THE CHAIR'S CORRECTION (caught re-deriving both opus seats' claims,
  Rule 4):** in this subset NO string temporary is ever live across a call
  (string function results rejected; a concat is never a var-arg), so the
  bead's named deep bug — "temporary lifetime under nested expressions" —
  is TWO-LIVE-TEMPS-IN-ONE-STATEMENT (a right-nested concat `a + (b + c)`),
  NOT clobber-across-recursion as both opus seats asserted. string.pas's
  RNEST clause is the load-bearing mutant target; recursion is belt.
- **B7 itself (`62ce0db`):** `string`/`string[N]` globals+locals+`var`
  params; TP ShortString byte-packed `round4(N+1)` layout, ascending base
  for every storage class; frame-resident 256-byte temp pool sized by a
  deterministic per-statement pre-walk (in-place accumulation: left chains
  1 temp, right-nesting forces 2); `__str_assign/__str_concat/__str_cmp/
  __str_write` intrinsics emitted IFF `uses_strings` (a stringless
  program's .s is BYTE-IDENTICAL to pre-B7 — proven, see Acceptance);
  silent truncation to compile-time dest cap; `+` concat and all six
  relops with the D7 coercion table (char coerces at assign/concat/compare
  only; `char+char` a located error — recorded fpc divergence, fixtures
  pinned around it); `length()` reserved-keyword inline; `s[i]` 1-based
  `base+i` unchecked ({$R-} precedent, Rule-2 tension recorded verbatim);
  loud rejections: array-of-string, string record fields, value string
  params, `string[N<255]` var formals, string function results — each with
  a named self-host idiom in ast.h.
- **Oracles:** `string.pas` (Rung 1, 25 TAG clauses) folded into
  test-seed-codegen; NEW gate `test-seed-string-mutant` — leg (i)
  SEED_MUT_CODEGEN_STR_TEMP_CLOBBER corrupts exactly RNEST
  (`ghijkl -> ijklijkl`), leg (ii) SEED_MUT_CODEGEN_STR_CMP_NOLEN flips
  exactly EQF (`ab = abc` wrongly TRUE) — both RED for the right reason,
  then green unmutated; `string.pas` in SEED_REPRO_CORPUS (now 14 fixtures
  / 42 hashes); `string_shared.pas` ({$B+}{$H-}) added via a generalized
  per-file test-seed-fpc-diff loop.
- **Oracle-integrity findings filed as beads (found during orientation):**
  `initech-altq` (P1) — fpc is NOT installed on this box AND
  test-seed-fpc-diff is not in TEST_UNIT_GATES, so the DEC-07 Rung-2
  differential is effectively orphaned (committee Q13, unanimous: leave it
  out until fpc lands, then wire Bochs-precedent-style; operator action:
  `sudo apt install fpc`). `initech-4yvg` (P2) — B5/B6 never got their
  promised fpc fixtures (Seat C independently rediscovered this).
  `initech-lh83` (P3) — array-of-string vs char-pool idiom, linked into
  `initech-6m52`.

## Why

B7 unblocks B8 (`assign()` needs the string type) and B9 (Turbo Initech's
own source needs error messages, emitted-asm text, and the DEC-05
in-subset int-to-string idiom `s := chr(d) + s`, which the committee
verified stays expressible under every rejection above).

## Frictions

- **The stale-worktree trap fired AGAIN** (the ck22 class): the first
  implementation lane's auto-provisioned worktree spawned from a June
  FLAIR-docs commit. The lane's mandatory STEP-0 base gate (`git log -1`
  must start at the pinned SHA, else STOP with zero edits) caught it
  cleanly. Remediation now in bd memory
  (`worktree-lane-dispatch-pattern-2026-07-14-b7`): the orchestrator cuts
  the worktree ITSELF at the pinned SHA and passes the path explicitly.
  A leftover `lane-39k2` worktree+branch from a cut-off prior session sat
  at base with zero commits — pruned/deleted.
- One committee seat died on a mid-stream API stall; the workflow resume
  replayed the two finished seats from cache and re-ran only the dead one.
- sudo needs a password, so the agent cannot install fpc (initech-altq).

## Acceptance

Graded FIRST-PERSON on main after the fast-forward (never trusting the
lane report, Rule 4): full seed family (test-seed, test-seed-codegen, all
11 mutant gates incl. the new string pair, test-seed-repro 14/42,
test-seed-repro-mutant) ALL GREEN; ASCII sweep of all touched files clean
(Rule 12); byte-identity re-verified by hand (6 pre-B7 fixtures emit
byte-identical .s under the post-B7 compiler). Authoritative certificate:
`make clean && make test` = ALL GREEN **320 host + 83 emu** (test-seed-
string-mutant joins the vector; recorded from the closing run of this
session).

## Pointers

- Bead `initech-39k2` (design field = the full committee record) — CLOSED.
- Committee + lane transcripts: session-local; the binding decisions are
  in the bead, ast.h's B7 block, codegen.c's header, and `62ce0db`'s
  message.
- Next: B8 `initech-ogxv` (file-I/O RTL over INT-21h + FILEIO-class
  mutant), then B9 `initech-6m52` (author Turbo Initech; resolve the
  array-of-string decision bead first). Also open: `initech-altq` (fpc),
  `initech-4yvg` (B5/B6 fpc backfill), `initech-x0i` (86Box, P1),
  `initech-586.4`, `initech-dam` (operator-gated).

<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0083 — TPS Parses Itself (B9.2: the Parser)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Worklog Shard (session record)
**Session:** 2026-08-18 (continuation of the WL-0080/0081/0082 sitting)

## What changed (commit `3a928d2`)

`tps.pas` grew from 934 to ~1,950 lines: a single-lookahead recursive-descent
parser driving the B9.1 lexer on demand (no token buffer, no AST, no symbol
tables — deliberately B9.3 scope). The grammar was mirrored rule-by-rule from
`seed/parser.c` with file:line citations in the lane report — the seed IS the
M7 grammar reference. One verified divergence, pinned rather than papered
over: the seed lexes `case` but does not parse it; TPS rejects it identically,
with a dedicated negative golden.

Driver: dual brackets in one run — the unchanged `TPS-LEX` dump, then the
`TPS-PARSE` event trace (spec in `PARSER-TRACE.md`). No lexer golden was
re-keyed; the lex gates' extraction was hardened to the anchored singular
bracket (neutral-to-stronger, documented in LEXER-DUMP.md).

**Oracles:** a hand-computed 651-line grammar-rich trace golden (the
independent root); five located-error goldens (missing semi, unbalanced end,
bad factor, misplaced var, the case divergence); **the self-parse tooth —
tps.pas parses ITSELF to EOF, twice-deterministic** (the pre-echo of
self-hosting); and `test-tps-parse-os` in TEST_EMU_GATES — the seed-compiled
`TPS.COM` parse trace on the booted InitechDOS byte-identical to the
fpc-compiled binary's (two-level differential, orchestrator-run first-person).
Rule 6: two valid-input WRONG-TRACE mutants (precedence shift, dangling-else
mis-bind), double-guarded sed generation, both RED for the named reason.

Memory: BSS 62,780 -> 103,748 B under a deliberately-raised 128 KiB soft
budget (cited on `6m52`); the hard `0x6F000` arena ceiling check is untouched
(59,920 B headroom). `TPS.COM` is 28,588 B.

## Adjudication: `initech-gamn` CLOSED not-reproducible

The codex sandbox's aggregate run reported `test-dbf-read` red (`TAX
dbf_open_rw rc=-1`) and filed the bead with an arena-exhaustion hypothesis.
First-person: green standalone (118 checks) AND green in both bracketing
clean certificates (B9.1's 329+88, B9.2's 332+89). Verdict: stale build
state inside the sandbox (likely a partially-minted TAX fixture), not a
real defect; the oracle was not weakened; the record lives on the bead.

## Acceptance

`make clean && make test` **ALL GREEN — 332 host + 89 emu.**

## Pointers

- `6m52` IN PROGRESS: B9.1 + B9.2 of 5 done. Next: **B9.3 typecheck +
  insertion-ordered symbol tables on the char-pool** (DEC-04; the lh83 idiom
  is already load-bearing in the lexer), then B9.4 codegen, B9.5 the real
  `test-compiler` corpus gate (closes M7).
- Codex-window serialization with the rk session remains standing (their
  hostile-review lane ran during this slice's grading window).

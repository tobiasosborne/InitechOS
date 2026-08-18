<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# WL-0084 — TPS Typechecks Itself (B9.3)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Worklog Shard (session record)
**Session:** 2026-08-18 (continuation of the WL-0080..0083 sitting)

## What changed (commit `8392e4f`)

`tps.pas` (now ~3,800 lines) gained the semantic stage: an AST-free
check-as-you-parse typechecker with DEC-04 insertion-ordered symbol tables on
the ratified char pool (globals + routine locals/params, local-first
resolution). The semantic rules mirror `seed/typecheck.c` with file:line
citations recorded in `SYMBOLS-DUMP.md`, which also controls the new third
output bracket: a deterministic per-scope symbol dump — the oracle surface
and B9.4's codegen paper trail (layout/offset attributes are already
computed and pinned there).

Two structural facts for the remaining slices:

1. **The rolling window.** The lane replaced B9.1's whole-source arena with
   an exact two-ShortString rolling window: `TPSIN.PAS` is re-read per pass
   (lex, parse, typecheck — codegen will make four). BSS fell 103,748 ->
   52,136 B; hard-ceiling headroom rose to 82,280 B; the soft reserve
   returned to 64 KiB. A structural build assertion pins the window and the
   arena's absence. Bonus: every pass exercises the B8 Reset/BlockRead path
   again on metal.
2. **The `forward` divergence, pinned.** TPS requires an explicit `forward`
   for any call-before-definition; the seed sees later headers because it
   precollects from its AST. TPS is STRICTER — the safe direction for
   self-hosting (TPS-compilable source remains seed-compilable) — and the
   B9.5 corpus plus tps.pas itself must respect it.

**Oracles:** hand-computed declaration-rich dump golden (independent root);
six located-error goldens (duplicate, unknown type, assignment mismatch,
var-param non-variable, string-bound form, file-type misuse); **the
self-typecheck tooth — tps.pas typechecks ITSELF, twice-deterministic**; and
`test-tps-type-os` in TEST_EMU_GATES: the FULL triple-bracket output of the
seed-compiled `TPS.COM` on the booted InitechDOS byte-identical to the
fpc-compiled binary. Rule 6: `TPS_TYPE_MUT_DUPOK` (accepts a duplicate,
emits an extra symbol row) and `TPS_TYPE_MUT_ASSIGN` (loses a required
mismatch error) — both RED for the named wrong value. Zero lexer/parser
re-keys.

## Acceptance

`make clean && make test` **ALL GREEN — 335 host + 90 emu.**

## Pointers

- `6m52` IN PROGRESS: B9.1–B9.3 of 5 done. Next: **B9.4 codegen** — TPS
  emits x86 (.s to a B8-written output file), the dump contract feeds the
  frame/global layout, and the differential deepens to EXECUTION: a
  TPS-compiled fixture must run identically to its seed-compiled and
  fpc-compiled selves. Then B9.5: the real `test-compiler` corpus gate
  closes M7.
- Codex-window serialization with rk continues (their capture-verification
  pass occupied this slice's grading window).

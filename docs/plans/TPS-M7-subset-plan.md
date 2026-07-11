<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# TPS M7 Subset Plan — sizing the seed -> Turbo Initech language gap

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Implementation Plan (living document; supersede in place)
**Session Record:** WL-0072 (this analysis: beads `initech-qvtj`, 2026-07-11)
**Tracking Epic:** `initech-rnh` (M7); scope statement feeds ADR-0007 (`initech-79s`)

## 1. Grounding

`make test-compiler` / `selfhost` / `ddc` are honest `stub_fail`s (Makefile
:11653/:18444/:18447); there is ZERO Free Pascal integration in-tree today.
The seed's fixtures are hand-computed exact-serial goldens through the QEMU
harness (`--expect`); `seed/examples/arith/negative_divmod.pas` is the
canonical shape. Free Pascal becomes a real oracle only once the subset can
share a runnable corpus — a sized step below (B9, first stood up at B4).

## 2. The seed's CURRENT subset (verified, cited)

Program shell, integer-only `var` (parser.c:308-357), blocks, three
statement kinds (assignment / nested block / write-writeln, parser.c:259-
272), additive+multiplicative expressions with unary minus (parser.c:102-
193; `div`/`mod`, no `/`), decimal ints, `'...'` strings with `''` escape,
both comment styles. Codegen: 32-bit signed int only; EVERY variable is a
static `.bss` slot; stack-machine eax/ecx/edx; serial write helpers; single
cdecl frame (codegen.c:14-78, 286-361). NO stack locals, NO calls, NO
control flow, NO booleans/relational ops, NO types beyond integer.

## 3. Target: minimal self-hosting subset (R / A(idiom) / D)

REQUIRED: if/while; procedures+functions (recursion, value params);
relational ops + boolean + and/or/not (COMPLETE evaluation, decided at B1;
self-host source written order-independent); const; char + ord/chr; static
arrays; records (parallel-array idiom rejected as too painful); minimal
fixed/ShortString strings; thin file-I/O RTL over INT-21h handles; static-
arena memory.
AVOIDABLE with idiom: for/repeat/case (sugar), var params (globals idiom —
but included for clean source), enums (const-int), sets/`in` (predicate
fns — set-of-char codegen is a known trap).
DEFERRED past K2==K3: heap/pointers (index-arena idiom), units (single-file
bootstrap), reals, objects, inline-asm blocks (RTL is separately
assembled).

## 4. Dependency chain (each step: fixture in the negative_divmod shape + Rule-6 mutant)

| # | Step | Size | Deep-bug flag |
|---|------|------|---------------|
| B1 | relational + boolean + and/or/not (complete-eval DECISION) | S-M | eval-order/short-circuit |
| B2 | if/then/else + while/do (+for/repeat sugar); ordinal labels (Rule 11) | M | — |
| B3 | const + char + ord/chr (parallel to B1/B2) | S | — |
| B4 | procedures/functions/scopes/value+var params/recursion — THE CODEGEN PIVOT (first stack frames); stand up fpc differential here | L | var-param aliasing; frame offsets |
| B5 | static arrays + indexed l/r-value | M | — |
| B6 | records + field access; deterministic layout | M-L | field-offset math |
| B7 | fixed/ShortString strings | M-L | temporary lifetime |
| B8 | file-I/O RTL (INT-21h) + static-arena convention | M | — |
| B9 | author Turbo Initech in the subset (os/tps/); test-compiler becomes a REAL fpc corpus gate; closes M7 | L | — |
| B10 | scope guard: deferred families recorded, post-fixpoint only | P4 | — |

M8 (epic ls4): resident port + K2==K3 + DDC — depends on B9.

## 5. ADR-0007 scope paragraph (draft, for initech-79s to ratify)

Scope of Turbo Initech: the resident, self-hosting Pascal compiler — the
in-universe North Star (PRD 1.2/6.7), distinct from the C kernel/Toolbox
and from the C seed (its genesis). Language level: a deliberately minimal
Turbo-Pascal-flavoured subset, NOT TP7-complete — integer/boolean/char/
fixed-string scalars; const; static arrays and records; if/while (for/
repeat/case as sugar); procedures/functions with nested scopes, value and
var parameters, recursion; COMPLETE boolean evaluation (not short-circuit),
self-host source written order-independent. Explicitly out of the self-host
subset: heap/pointers (static index-arenas), enums (const-int), sets/in
(predicate functions), units (single-file bootstrap), reals, objects,
language-level inline asm — post-fixpoint niceties. Codegen: single-pass
stack-machine x86, no optimizer. RTL boundary: thin hand-assembled runtime
(entry + file I/O over InitechDOS INT-21h handles) mirroring seed/rt/.
Self-host criterion: K2==K3 bit-for-bit under reproducible build, scoped to
Turbo Initech and the Pascal it compiles, with DDC as the trusting-trust
antidote; the IDE is the resident blue screen certified by the M8 finale.

## 6. Sanity vs M7/M8

B1-B9 deliver exactly PRD 11 M7; M8 keeps the resident port + fixpoint.
Reached with NO objects/units/sets/enums/heap/reals — the smallest
sufficient language. The one true expansion is standing up fpc as a real
differential oracle, which PRD 6.7/8 already mandate.

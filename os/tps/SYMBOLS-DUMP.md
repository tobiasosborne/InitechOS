<!-- INITECH CONFIDENTIAL - INTERNAL USE ONLY - DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. -->

# Turbo Initech Symbol Table Dump Interface

**Document owner:** Platform Engineering / Compiler Systems  
**Control bead:** `initech-6m52`, B9.3  
**Status:** Implemented oracle interface and B9.4 code-generation paper trail  
**Character set:** ASCII

## 1. Purpose and authority

This document controls the deterministic text surface emitted by
`os/tps/tps.pas` after a clean syntax parse and semantic check. The dump is an
oracle, not a serialization format. B9.4 may consume the documented layout
decisions, but shall not parse this text at run time.

The governing sources are ADR-0007 DEC-04 (insertion-ordered arrays; no hash
tables), CLAUDE.md Rule 11 (reproducibility), and the semantic reference in
`seed/typecheck.c`. In particular, the seed reference defines local-first name
resolution (`seed/typecheck.c:248-358`), declaration/routine collection
(`seed/typecheck.c:443-778`), calls and `var` arguments
(`seed/typecheck.c:1015-1185`), expression compatibility
(`seed/typecheck.c:1189-1557`), assignment compatibility
(`seed/typecheck.c:1560-1868`), and routine scopes/forward completion
(`seed/typecheck.c:1944-2127`). The B8 file restrictions are at
`seed/typecheck.c:829-1007,1425-1429,1593-1597`.

## 2. Bracket contract

A clean run emits exactly one region of this form:

```text
TPS-TYPE-BEGIN
SCOPE global bytes=<decimal>
<global rows in declaration order>
<routine scopes in routine declaration order>
TPS-TYPE-OK
TPS-TYPE-END
```

A semantic failure emits exactly one located first-error line and no partial
symbol dump:

```text
TPS-TYPE-BEGIN
TPS-TYPE-ERROR line=<decimal> col=<decimal> <deterministic detail>
TPS-TYPE-END
```

If lexing or the preceding syntax-only parse fails, the type region contains
one `TPS-TYPE-SKIP lexer-error` or `TPS-TYPE-SKIP parser-error` line. The lexer
and parser brackets are unchanged from `LEXER-DUMP.md` and `PARSER-TRACE.md`.

## 3. Ordering and spelling

All tables are fixed-capacity insertion-ordered arrays. No hash table exists.
Names are case-folded by the lexer and stored as offset/length references into
the existing `PoolChars` character pool; symbol rows do not own string
storage.

The global scope is emitted in source declaration order. Fields follow their
record-type row in flattened declaration order. A forward declaration creates
one routine row at the forward's position; its later defining header updates
that row and its parameter spellings without moving it or adding a second row.
Routine scopes are then emitted in global routine order. Within a routine the
order is parameters, function result (when present), then locals in declaration
order. A local may shadow a global; a duplicate inside one scope is rejected.

These rules are the reproducibility contract. Iteration in any other order is
non-conforming even if name lookup still succeeds.

## 4. Row forms

The controlled row forms are:

```text
SYM kind=const name=<name> type=<type> value=<decimal>
SYM kind=const name=<name> type=string value-length=<decimal>
SYM kind=type name=<name> type=record fields=<decimal> size=<decimal>
FIELD owner=<record> index=<decimal> name=<name> type=<scalar> offset=<decimal> size=4
SYM kind=var name=<name> type=<type> [array=<lo>..<hi>] offset=<decimal> size=<decimal>
SYM kind=procedure name=<name> type=none params=<decimal> state=defined frame=<decimal>
SYM kind=function name=<name> type=<scalar> params=<decimal> state=defined frame=<decimal>
SCOPE routine name=<name> frame=<decimal>
SYM kind=<value-param|var-param|result|local> name=<name> type=<type> [array=<lo>..<hi>] offset=<signed-decimal> size=<decimal>
```

`<type>` is `integer`, `boolean`, `char`, `file`, `string`, `string[N]`, or a
case-folded named record type. A clean dump can never contain `state=forward`:
an unresolved forward is a located type error before dumping.

## 5. Layout attributes

Offsets are the logical values already required by the seed-compatible B9.4
layout; they are not addresses from the current checker process.

- Global offsets start at zero and advance in declaration order.
- Integer, boolean, and char storage is four bytes.
- Record fields are uniform four-byte slots; field `k` is at `4*k`. A record's
  size is `4*fields`.
- An array has `(hi-lo+1)` consecutive elements. Its size includes a named
  record element's full field count.
- A ShortString occupies `round4(N+1)` bytes. Bare `string` means capacity 255.
- A thin B8 file object occupies 260 bytes (handle plus ASCIIZ name storage).
- Parameters use cdecl displacements `+8,+12,...` and occupy one four-byte
  slot; a `var` slot contains an address.
- Function result and ordinary local slots descend from EBP at `-4,-8,...`.
  String/file locals report the lowest address of their contiguous block.
- `frame` is the total result-plus-local byte reservation; parameters are not
  included.

## 6. Fixed budgets and failure policy

The B9.3 implementation provides 704 symbol rows, 480 interned names, and
4,816 character-pool bytes. Header and call signatures allow 32 parameters,
matching `seed/typecheck.c`'s `TC_MAX_PARAMS`. Every overflow stops at the
first located `TPS-TYPE-ERROR`; truncation and wraparound are forbidden.

The rolling two-ShortString input window preserves exact source bytes and
locations across all three driver passes. B9.2's `SourceWords[0..20479]`
whole-source allocation is retired, returning 81,920 bytes of static BSS. The
structural build assertion now proves the rolling-window routines and the
absence of `v_sourcewords`; no lexer/parser golden or bracket output is re-keyed.

## 7. Deliberate phase divergence

The seed parses an AST, collects tables, and then checks bodies. TPS has no AST
by design. It reopens the source and performs one forward semantic parse after
the unchanged syntax-trace pass. The semantic compatibility rules match the
seed, but a TPS call to a routine declared later requires an explicit prior
`forward`; the seed's precollection can see that later header. This divergence
is architectural and recorded, not an invented language rule.

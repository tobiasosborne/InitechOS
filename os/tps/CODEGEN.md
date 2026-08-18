<!-- INITECH CONFIDENTIAL - INTERNAL USE ONLY - DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. -->

# Turbo Initech x86 Emission Contract

**Document owner:** Platform Engineering / Compiler Systems
**Control bead:** `initech-6m52`, B9.4-B9.5
**Status:** Implemented; real 17-fixture host corpus green; all-fixture on-OS
execution differential wired for orchestrator execution
**Character set:** ASCII

## 1. Authority and phase structure

The governing references are ADR-0007 DEC-04/DEC-05, `seed/codegen.c`,
`seed/rt/start.asm`, `seed/rt/start_dos.asm`, `seed/rt/fileio.asm`, and
`SYMBOLS-DUMP.md`. TPS intentionally does not copy the seed compiler's AST
phase structure. Its fourth read of fixed-name `TPSIN.PAS` rebuilds and checks
the insertion-ordered symbol tables while emitting fixed-name `TPSOUT.S`.

The driver surface is:

```text
TPS-GEN-BEGIN
TPS-GEN-OK bytes=<decimal> labels=<decimal>
TPS-GEN-END
```

Assembly bytes go only to `TPSOUT.S`, through `Assign`, `Rewrite`, and buffered
`BlockWrite`. An invariant failure emits one located `TPS-GEN-ERROR` and never
reports `TPS-GEN-OK`.

## 2. Register and expression discipline

- `eax` is the expression value, address, and function-result register.
- A scalar binary expression emits the left operand, pushes `eax`, emits the
  right operand, moves the right value to `ecx`, restores the left value to
  `eax`, and combines there. `edx:eax` is used by signed `idiv`.
- Relational results are normalized to 0 or 1 with `setcc`/`movzx`.
- `and` and `or` use the same unconditional two-operand path as arithmetic.
  Both operands therefore execute, as ADR-0007 DEC-03 requires.
- `edx` is the scalar l-value address scratch. `esi`/`edi` are used only at
  the string-intrinsic boundary. No emitted value is kept live in a
  caller-saved register across a call.

There is no optimizer and no register allocator.

## 3. Calls and frames

Routines use cdecl frames:

```text
[ebp+8]       first parameter
[ebp+12]      second parameter
[ebp+4]       return address
[ebp]         saved ebp
[ebp-4]       function result or first local slot
[ebp-8]       next local slot
```

The prologue is `push ebp; mov ebp,esp; sub esp,<frame>` and the epilogue is
`leave; ret`. Functions load their result slot into `eax`. A `var` parameter
slot holds a caller address and is dereferenced afresh for every read/write.

The parser necessarily sees arguments left-to-right. TPS evaluates them in
that order, saves each result, then pushes duplicate argument words in reverse
order immediately before the call. The callee therefore still observes the
pinned cdecl `[ebp+8+4*i]` layout, and the caller removes duplicates, saved
arguments, and any string materialization bytes in one deterministic cleanup.

## 4. Storage and aggregate addressing

- Globals are insertion-ordered `v_<lower-case-name>` labels in `.bss`.
- Routine labels are `pf_<lower-case-name>`; the entry body is `pas_main`.
- Scalars occupy one dword. Global arrays advance upward from their label;
  local array slots descend from EBP. The lower bound is subtracted before
  multiplying by the scalar or record stride.
- Record fields are declaration-ordered dwords. Global/var-parameter fields
  advance from the base. Plain local records and local array-of-record
  elements descend from their field-zero frame address.
- Whole records copy field-by-field with compile-time offsets. No runtime
  layout metadata or loop label is introduced.

`TPS_GEN_MUT_OFFBYONE` changes parameter displacements by one dword. The tiny
fixture's two-parameter sum then executes to a wrong value on the orchestrator
rail without requiring an assembly failure.

`TPS_GEN_MUT_VARPARAM` changes only assignment destinations for `var`
parameters: reads still dereference the caller address, while stores target
the callee parameter slot. The registered `gen_func_deep` aliasing clauses
then leave caller variables unchanged and make the execution differential red.

## 5. Strings and file RTL

ShortStrings retain the B7 representation: byte 0 is length and bytes 1..N
are content. Global/local/`var` storage follows `SYMBOLS-DUMP.md`. The emitted
`__str_assign`, `__str_concat`, `__str_cmp`, and `__str_write` entry points
mirror the seed intrinsic ABI and are emitted only when a string expression
uses them.

String expression temporaries are activation-local runtime-stack regions.
Nested concatenation can therefore keep more than one simultaneous value and
recursion cannot alias another activation's temporary. Each top-level
consumer removes the exact threaded byte count.

The file verbs call the frozen B8 entries:

```text
rtl_file_assign
rtl_file_reset
rtl_file_rewrite
rtl_file_blockread
rtl_file_blockwrite
```

They are declared only when a validated file call is emitted. Both bare and
InitechDOS links use the unchanged seed runtime objects.

## 6. Sections, labels, and determinism

The first section occurrences match the seed shape: `.rodata`, `.bss`, then
`.text`. Boolean print constants are in `.rodata`; globals are in `.bss`;
routines and `pas_main` are in `.text`. A literal reached during the one-pass
body emission is an inline byte sequence behind a deterministic jump in
`.text`; this avoids a second ordinal-derivation pass while preserving the
ShortString/C-string representation selected by its consumer.

Every generated branch and inline literal draws from one `GenData[3]` counter.
No label family owns a private counter. `TPS_GEN_MUT_LABELS` prevents `while`
from advancing that counter; the two adjacent loops in `gen_tiny.pas` then
produce duplicate labels and NASM rejects the file.

## 7. Fixture register

`CORPUS.md` is the controlled 17-fixture registry, DOS-name map, coverage
matrix, rate contract, and Rule-6 record. Every Pascal source carries the
derivation of its sibling hand-computed `.golden`. For every row, TPS emits,
assembles, and links twice byte-identically for both `seed.ld` and
`seed_dos.ld`; FPC executes the same source and must match the independent
golden. `test-compiler-os` owns generated-code execution for all rows.

`tps.pas` remains a separate self-source compile/assemble/link tooth over the
whole accepted implementation surface; it has no behavior golden and is not
counted in the 17-fixture differential rate.

## 8. Hand-checked tiny excerpt

The following excerpt was inspected from the deterministic `gen_tiny.pas`
output. It demonstrates the pinned frame offsets and stack-machine addition:

```nasm
pf_addpair:
    push ebp
    mov ebp, esp
    sub esp, 8
    lea eax, [ebp-8]
    push eax
    lea eax, [ebp+8]
    mov eax, [eax]
    push eax
    lea eax, [ebp+12]
    mov eax, [eax]
    mov ecx, eax
    pop eax
    add eax, ecx
    pop edx
    mov [edx], eax
```

The call site evaluates `n`, then `6`, duplicates them into reverse push order,
calls `pf_addpair`, and removes 16 bytes (two originals plus two cdecl words).

## 9. Budget and execution status

With the B9.5 assignment-mutation seam, the seed-built compiler is 89,591
bytes as a flat `.COM`; static BSS remains 71,704 bytes, and linked end
`0x67710` remains below `0x6f000`. The TPS-generated self assembly is 765,205
bytes of text; its linked DOS image ends at `0x6d27c`, leaving 7,556 bytes.
The 72 KiB BSS soft reserve and unchanged hard image ceiling remain enforced.
This thin margin is still the M8 watch item, not a B9.5 optimization target.

No emulator was run in the B9.5 lane. Host-proven claims are all 17 FPC
executions against hand goldens, twice-byte-identical TPS assembly/object/bare
ELF/DOS ELF per row, self-source generation and both links, and buildable
OFFBYONE/VARPARAM wrong-code artifacts. The all-row InitechDOS execution rate
and both semantic mutation bites are wired in `test-compiler-os` and
`test-compiler-os-mutant` for orchestrator certification.

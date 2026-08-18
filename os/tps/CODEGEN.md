<!-- INITECH CONFIDENTIAL - INTERNAL USE ONLY - DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. -->

# Turbo Initech x86 Emission Contract

**Document owner:** Platform Engineering / Compiler Systems
**Control bead:** `initech-6m52`, B9.4
**Status:** Implemented; host/link oracles green; on-OS execution oracles
wired for orchestrator execution
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

The host gate uses the following independent expectations:

- `gen_tiny.pas`: authored B9.4 tooth, hand result `TINY=10`.
- `gen_{func,array,record,string,file}_shared.pas`: byte-for-byte copies of
  the seed/FPC shared corpus. `gen_func_shared` already contains its required
  `IsEven` forward; no forward was added to any shared copy.
- `gen_{bool,control,char,func,array,record,string,fileio}_deep.pas`: copies
  of the seed hand-golden fixtures. Their only local amendment is the leading
  FPC dialect pin `MODE DELPHI`, complete booleans, and ShortStrings. Both
  function sources already contain every required forward; no forward was
  added.
- `tps.pas`: self-source compile/assemble/link tooth over the whole accepted
  implementation surface.

For every fixture, TPS emits twice byte-identically, NASM assembles the result,
and both `seed.ld` and `seed_dos.ld` links succeed. FPC compiles and executes
the same source against the hand-computed seed golden. Generated-code execution
is deliberately reserved for `test-tps-gen-os`.

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

The seed-built B9.4 compiler is 89,519 bytes as a flat `.COM`; static BSS is
71,704 bytes, and linked end `0x676c8` remains below `0x6f000`. The
TPS-generated self assembly is 764,500 bytes of text; its linked DOS image
ends at `0x6d224`, leaving 7,644 bytes. The 72 KiB BSS soft reserve and the
unchanged hard image ceiling are both enforced mechanically.

No emulator was run in the B9.4 lane. Consequently, generated behavior for
the full corpus, the on-OS compiler/file round trip, recursion under emitted
frames, deep string temporaries, local array-of-record access, and the
OFFBYONE wrong-value mutant remain wired but unexecuted here. The host-proven
claims are deterministic generation, assembly, both links, direct FPC golden
execution, self-source generation, and both Rule-6 failure axes.

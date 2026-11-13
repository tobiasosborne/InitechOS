<!-- INITECH CONFIDENTIAL - INTERNAL USE ONLY - DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. -->

# Turbo Initech Shared Compiler Corpus Register

**Document owner:** Platform Engineering / Compiler Systems
**Control bead:** `initech-6m52`, B9.5
**Status:** Registered M7 differential corpus
**Character set:** ASCII

## 1. Authority and oracle rule

This is the named **Turbo Initech Shared Compiler Corpus** required by PRD
Sections 6.7 and 8 and ADR-0007 DEC-07 Rung 3. Every registered Pascal source
has a sibling `.golden` containing its exact hand-computed stdout. FPC is an
independent compiler that must agree with that root; FPC output never creates
or rewrites a golden. Turbo Initech must then compile the same source and its
generated program must agree byte-for-byte with the same FPC output.

The copied `gen_*_shared` and `gen_*_deep` fixtures retain the seed examples'
hand-derived expectations. The three B9.5 focus fixtures were computed from
the arithmetic and Pascal semantics recorded in their source headers. All
fixtures use the FPC `MODE DELPHI`, complete-boolean (`B+`), and ShortString
(`H-` where strings matter) pins. Calls to later routines use `forward`,
respecting TPS's deliberate stricter-than-seed forward divergence.

## 2. Registry

| Source / golden stem | DOS name | Family | Load-bearing execution coverage |
|---|---:|---|---|
| `gen_tiny` | `CTINY` | B2/B4 | two loops, branch, two value params, frame local |
| `gen_bool_deep` | `CBOOL` | B1 | all relations, `and`/`or`/`not`, precedence |
| `gen_control_deep` | `CCTRL` | B2 | `if`/`else`, `while`, `for` both ways, `repeat` |
| `gen_char_deep` | `CCHAR` | B3 | constants, `char`, `ord`, `chr`, char scan/output |
| `gen_func_shared` | `CFUNCS` | B4 | recursion, mutual recursion/`forward`, var-param swap |
| `gen_func_deep` | `CFUNC` | B4 | recursion, locals, value/var params, aliasing, composition |
| `gen_recursion_depth` | `CDEPTH` | baseline/B1/B4 | negative `div`/`mod`, thirteen recursive frames, dynamic complete evaluation |
| `gen_array_shared` | `CARRAYS` | B5 | global/local arrays, bounds/rebase, indexed var params |
| `gen_array_deep` | `CARRAY` | B5 | integer/boolean/char arrays and frame-local array |
| `gen_record_shared` | `CRECS` | B6 | fields, record var param/copy, array of records |
| `gen_record_deep` | `CRECORD` | B6 | record layout, local record/record array, nested designators |
| `gen_local_record_array` | `CLRECORD` | B6 | focused negative-bound frame-local array of records |
| `gen_string_shared` | `CSTRS` | B7 | ShortString concat/length/index/compare |
| `gen_string_deep` | `CSTRING` | B7 | truncation, all compares, indices, recursion, two live temps |
| `gen_string_temporaries` | `CSTRTEMP` | B7 | four simultaneous right-nested temps and doubled-quote literal decoding |
| `gen_file_shared` | `CFILES` | B8 | byte/block write-read round trip and local file variable |
| `gen_fileio_deep` | `CFILEIO` | B8 | independent deep copy of the byte/block file oracle |

Together these rows execute-cover the complete M7 accepted subset: 32-bit
integer/boolean/char/ShortString scalars; constants; every relation and
arithmetic/boolean operator; primitive and sugar control flow; procedures and
functions with locals, value and var parameters, recursion and `forward`;
static arrays; records and arrays of records; string operations; and the thin
untyped-file RTL. Pointer/heap, units, reals, objects, sets/enums, and inline
assembly remain the ADR-0007 DEC-02 post-fixpoint deferrals, not corpus holes.

## 3. Gate split and rates

`make test-compiler` is the host half. For all 17 rows it requires FPC to
compile and execute the source against the hand golden, then requires the
FPC-built TPS to emit twice byte-identically and NASM/link both bare and DOS
artifacts twice byte-identically. Its success line is:

```text
compiler_host_pass_rate=100% (17/17)
```

`make test-compiler-os` is the execution differential. It retains the B9.4
compile-on-InitechDOS tiny tooth, then places all 17 TPS-generated `.COM`
files plus `compiler_corpus.bat` on one fresh FAT disk. One bounded DOS boot
runs every row between unique markers. Each captured block is diffed directly
against the already golden-checked FPC stdout. Its only green completion is:

```text
differential_pass_rate=100% (17/17)
```

This one-boot batching keeps DOS boot cost bounded without weakening the DOS
rail: every registered TPS-generated program executes under InitechDOS.

## 4. Rule 6

`test-compiler-os-mutant` composes two semantic execution bites over registered
corpus rows. `TPS_GEN_MUT_OFFBYONE` shifts parameter slots and makes
`gen_tiny` print a wrong value without crashing. `TPS_GEN_MUT_VARPARAM` sends
var-parameter assignment stores to the callee parameter slot instead of the
caller address; `gen_func_deep` then leaves the caller values unchanged and
differs from its aliasing golden. A mutant that prints the clean golden makes
the Rule-6 gate fail.

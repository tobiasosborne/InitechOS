# Turbo Initech: remaining path to the golden master

The seed is host C; Turbo Initech is Pascal. The seed emits NASM assembly.
The Pascal compiler emits native flat COM and extended InitechMZ containers,
as well as assembly for the independent factory oracle. No new source-language
feature was needed to compile the compiler. GOLDEN-MASTER.md item 7 remains
open because the minimal blue IDE and finale session remain to be built.

Current mechanical evidence:

- 17 flat corpus images and 18 extended images (including turbo.pas) equal
  NASM/binutils/mzlink byte for byte. mzlink derives relocations by two link
  bases independently of the Pascal emitter's recorded sites. mzcheck validates
  every emitted container and both compiler memory plans. Runtime tables are
  regenerated against NASM in tier 0. The 15 compiler patches include two
  restored semantic execution patches and a flat-only runtime fault.
- The full 17-program corpus and 19 code-generation regressions execute on
  QEMU and Bochs through direct seed, hosted Pascal native and resident Pascal
  native output, with both native forms exercised (360 comparisons, preserving
  all prior 330 and the original 280).
  Stdout/files agree with independent FPC;
  original corpus rows retain their authored goldens. Separate TEST disks
  preserve the product image. Resident output needs no host assembler/linker.
- In-OS K1(S) -> K2 and K2(S) -> K3 reproduce 164263 bytes exactly, SHA-256
  2c33ed0c7ee667138cfb3253630994be169cc8a22a1f4c54715a4eb40f9e0fc7.
  Fresh boots reset identical inputs on both QEMU and Bochs at 8 MiB.
  QEMU guest compile times were 60s/102s. Bochs harness times were
  1291.005s/1738.492s, with an explicit measured 3600s acceptance bound per
  invocation until factory's ordered phase waits are integrated (initech-b8vxr).
  Bochs also refuses a corrupted K2 in a real boot, and its deadline control
  fails. The computed-left-character seed repair preserves the fixed point.
  K1==K2 is not the certificate.
  The minimum-Integer div/mod guards and emission/label handling increase the
  previous 163087-byte compiler's module by 1064 bytes and header by 112 bytes.
- FPC 3.2.2 builds the independent host bootstrap and directly emits D1.
  Fresh in-OS D1(S) -> D2 took 104 QEMU guest seconds and 1546.689s in the
  Bochs harness; D2==K2==K3 on both, and both emulators' containers agree.
  A contaminated candidate and a changed used source constant both make the
  comparison red. This removes sole trust in the C seed's translation, not
  trust in source/runtime bytes, FPC, host tools, the OS or hardware.

Source identity, transcripts, images and hashes live under build/turbo/selfhost
and build/turbo/ddc; the committed scripts reproduce them. Native layout,
encoding authorities, runtime-table choice and oracles: compiler/NATIVE.md.
The complete warm tier-0 Turbo suite takes about 20 seconds.

Remaining item-7 work includes runtime-encoder authorship (initech-cd2yi),
the minimal blue IDE and finale. Edit, safe save,
compile diagnostics, run and return, followed by a boot-to-self-reproduction
session. All OS calls must use INT 21h. The inherited deferred gates are still
inventoried in tests/SMOKES.md; the new corpus/selfhost/DDC checks do not claim
that every historical gate has been restored.

Semantics remain bounded: signed integers, booleans, chars, ShortStrings,
static arrays, named scalar records and untyped byte files. No pointers/heap,
reals, units, sets, objects or general Pascal compatibility is claimed.
Scheduling is cooperative. Table capacities and worst-case stack use need
measurement if the compiler or IDE grows; current checks enforce real memory
placement limits and do not claim a static stack-usage bound.

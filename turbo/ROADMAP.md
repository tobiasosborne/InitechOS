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
- The full 17-program corpus and 16 code-generation regressions execute on
  QEMU and Bochs through direct seed, hosted Pascal native and resident Pascal
  native output, with both native forms exercised (330 comparisons, preserving
  the original 280).
  Stdout/files agree with independent FPC;
  original corpus rows retain their authored goldens. Separate TEST disks
  preserve the product image. Resident output needs no host assembler/linker.
- The last in-OS K1(S) -> K2 and K2(S) -> K3 proof reproduced 163087 bytes, SHA-256
  7bf6424c74a46106b01f5d75de2c422005a026d2624f1e044e626fc1ae801ff6.
  Fresh boots reset identical inputs and fit the unchanged emulator driver's
  120-second serial gate. Guest compile times were 41s/86s in the DDC refresh.
  K1==K2 is not the certificate.
- After the minimum-Integer div/mod guards, the host-generated candidate is
  164263 bytes, SHA-256
  2c33ed0c7ee667138cfb3253630994be169cc8a22a1f4c54715a4eb40f9e0fc7.
  The module grows by 1064 bytes and the relocation header by 112 bytes for
  guard emission and label handling. Fresh self-host/DDC is pending: the
  loaded machine's first run reached the factory driver's 120-second ceiling.
- The previous diverse proof used FPC 3.2.2 for the independent host bootstrap;
  it directly emitted D1. In-OS D1(S) -> D2 took 75 guest seconds; D2==K2==K3.
  A contaminated candidate and a changed used source constant both make the
  comparison red. This removes sole trust in the C seed's translation, not
  trust in source/runtime bytes, FPC, host tools, the OS or hardware.

Source identity, transcripts, images and hashes live under build/turbo/selfhost
and build/turbo/ddc; the committed scripts reproduce them. Native layout,
encoding authorities, runtime-table choice and oracles: compiler/NATIVE.md.
The complete warm tier-0 Turbo suite takes about 20 seconds.

Remaining item-7 work: the minimal blue IDE and finale. Edit, safe save,
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

# Turbo Initech

Two compilers: `seed/` is the C host bootstrap; `compiler/turbo.pas` is the
Pascal compiler. The seed emits assembly for NASM and binutils on the host.
The Pascal compiler emits native flat COM and extended InitechMZ executables,
plus assembly retained for independent factory byte comparisons. No host
assembler or linker finishes the programs compiled inside InitechOS.

`make` builds the seed, the FPC host compiler and resident `TURBO.COM`.
Despite its name, TURBO.COM is an extended InitechMZ container: a flat 32-bit
module linked at zero, relocated to 0x600000 by the content-dispatch loader.
Startup retains the loader's separate 64 KiB stack; descriptor reserve covers
BSS including alignment. Files and exit use INT 21h. Inherited serial helpers
now poll transmitter readiness. No private kernel header/code is included.

`make test` composes 6 seed and 40 compiler host entries, including 17 FPC
execution references and independent native byte comparisons (17 flat,
18 extended). The two compiler placements are packaged with factory/mz/mzlink
and checked by the independent mzcheck against the actual extended limits:
262144 module bytes, aligned module+reserve before 0x7ef000, native heap
header, separate guard and stack. The old conventional compiler placement
gates are replaced by these real limits. Small programs retain their flat
runtime/link route. The warm complete Turbo suite takes about 20 seconds.
Runtime is a checked-in NASM byte table with a tier-0 regeneration comparison;
see compiler/NATIVE.md. All 14 compiler patches are killed, including five
native encoder/fixup/relocation/layout and two generated-execution patches.

`make smoke` builds the product image and uses separate FAT12 TEST disks.
QEMU and Bochs each execute all 17 corpus programs and 11 regressions through
direct seed output, hosted Pascal native output, and resident Pascal native
compilation: 168 executions. Complete stdout and extracted files match FPC;
the original corpus also matches its authored goldens. mzcheck validates the
emitted containers. See tests/execution/README.md for the red-first probes and
the explicit 32-bit endpoint reference normalization.
The guest TIMER test utility brackets compilation through DOS GET TIME at its
1-second resolution. Compilation timing remains recorded per resident case.
No test material is placed on the product boot disk. Images/logs remain under
build/turbo/smoke. The old placement-only check is tests/smoke/placement.sh.

`make selfhost` builds K1 on the host, then uses fresh QEMU boots and TEST
disks for K1(S) -> K2 and K2(S) -> K3. Identical source/input names are reset
each time. Separate boots respect the unchanged factory driver's fixed
120-second serial gate. mtools extracts both whole containers, the comparator
asserts K2==K3 and prints SHA-256. A corruption control must fail. Transcripts,
source identity and artifacts remain under build/turbo/selfhost.

`make ddc` first refreshes that seed-path certificate, then uses the same
compiler source built with Free Pascal as the independent host bootstrap.
That compiler emits D1 directly; D1(S) runs in the OS to produce D2. The check
asserts D2==K2==K3. Contamination and changed-source controls must fail.
Evidence, FPC version and bootstrap hashes remain under build/turbo/ddc.

The fixed point is 163087 bytes, SHA-256
7bf6424c74a46106b01f5d75de2c422005a026d2624f1e044e626fc1ae801ff6.
After the record, character and for-loop code-generation fixes, self-hosting
took 41s/86s and DDC took 75s for D1(S). The checked-in NASM tables regenerate
byte-for-byte. Times use the guest clock with 1-second resolution. DDC removes sole
reliance on the C seed's translation and its bootstrap assembler pipeline.
It still trusts the compiler source (including runtime byte tables), the
independent FPC path, host tools, loader, emulator and hardware. The controls
show comparison sensitivity, not resistance to every possible shared attack.

FPC, NASM, GNU binutils and mtools are required; missing tools fail. No Pascal
language feature was added; seed string-character var arguments now accept
the documented form. Authored corpus/protocol goldens are unchanged. Emulator
work never runs in tier 0. See ROADMAP.md for remaining
GOLDEN-MASTER.md item-7 work, including the IDE.

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
see compiler/NATIVE.md. The 15 compiler patches include five native
encoder/fixup/relocation/layout and three generated-execution patches.

`make smoke` builds the product image and uses separate FAT12 TEST disks.
QEMU and Bochs each execute all 17 corpus programs and 19 regressions through
direct seed output, hosted Pascal native output, and resident Pascal native
compilation. Hosted and resident Pascal output also runs as flat COM:
360 executions, preserving all prior 330 and adding 30 comparisons for
computed characters at the start of concatenations, including higher temporary
bases. The earlier probes cover minimum-Integer div/mod and string temporaries
in Length and record indices.
Every registered case fits the flat arena; missing flat output
fails. Complete stdout and extracted files match FPC;
the original corpus also matches its authored goldens. mzcheck validates the
emitted containers. See tests/execution/README.md for the red-first probes and
the explicit 32-bit endpoint reference normalization.
The guest TIMER test utility brackets compilation through DOS GET TIME at its
1-second resolution. Compilation timing remains recorded per resident case.
No test material is placed on the product boot disk. Images/logs remain under
build/turbo/smoke. The old placement-only check is tests/smoke/placement.sh.

`make selfhost` builds K1 on the host, then performs K1(S) -> K2 and
K2(S) -> K3 on both QEMU and Bochs at 8 MiB. Every compilation uses a fresh
TEST disk and the ordinary product boot image; identical source/input names
are reset each time. The QEMU invocation is unchanged. mtools extracts whole
containers; K2==K3 must hold on each emulator, and both emulator results must
agree. QEMU artifacts remain under build/turbo/selfhost; Bochs artifacts,
configuration, boot-image identity and timing are under its bochs/ subdirectory.

Factory gives emulator runs independent resources. Bochs uses an explicit
3600000ms acceptance bound for each complete harness invocation, including
startup and cleanup. Calibration at 600s and 1200s expired during
native emission. A monotonic acceptance check also rejects late completion
regardless of the harness verdict, and timed_out must be zero. Override only
with TURBO_BOCHS_TIMEOUT_MS. Ordered per-phase waits are a separate factory
follow-up, initech-b8vxr; the operator authorized this bounded whole-run route
until that interface is available. No factory default or QEMU limit is changed.

Controls reject a corrupted QEMU candidate, a corrupted Bochs K2 attempted in
a real guest boot, and an impossible one-millisecond Bochs deadline. The Bochs
corruption preserves MZ/tag but changes the descriptor version, so its red is
a clean loader refusal rather than raw-COM execution or a timeout.

`make ddc` first refreshes that seed-path certificate, then uses the same
compiler source built with Free Pascal as the independent host bootstrap.
That compiler emits D1 directly; D1(S) runs in the OS on both emulators to
produce D2. Each D2 must equal its K2/K3 and the other emulator's D2.
Contamination and changed-source controls must fail. Evidence, FPC version
and bootstrap hashes remain under build/turbo/ddc, with a separate bochs/ tree.

The certified fixed point is K2==K3==D2: 164263 bytes, SHA-256
2c33ed0c7ee667138cfb3253630994be169cc8a22a1f4c54715a4eb40f9e0fc7.
The minimum-Integer div/mod guards and their emission/label handling increase
the previous 163087-byte compiler's module by 1064 bytes and relocation header
by 112 bytes. The computed-left-character seed-planner repair leaves the
Pascal source and seed-built TURBO.COM bytes unchanged, and its fresh proof
agrees at the same hash. Fresh QEMU DOS-clock times are 60s/102s for K2/K3
and 104s for D1(S). Bochs K2/K3/D2 agree with QEMU at this same hash;
measured host harness times are 1291.005s/1738.492s/1546.689s, each below
3600s. Qualification preceded factory's resource-isolation change, so those
measurements include its former Bochs host-lock waiting and cleanup. The
complete default selfhost/DDC command passed in 4855.22s, with its script
manifest unchanged throughout. The isolated driver also passes a fresh
resident native compilation against the FPC-built host compiler's output,
plus the corrupted-K2 and deadline controls. All negative controls are red.
Only proof drivers changed here;
compiler source and runtime bytes are unchanged. Runtime-encoder authorship
remains open separately as initech-cd2yi.
The checked-in NASM tables regenerate
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

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
Interface product-elf: build/turbo/compiler/extended/zero.elf
The package recipe links this ELF into shipped TURBO.COM; Multiboot startup
objects in all are judging fixtures, not DOS application inputs.
<!-- end-interface-declarations -->

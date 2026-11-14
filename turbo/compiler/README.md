# Turbo Initech compiler

Pascal compiler, also built by Free Pascal on the host for independent bootstrap
and differential checks. Four initial passes reopen TURBOIN.PAS for lexical,
syntax, semantic and NASM generation. Native counting/emission then writes
TURBOOUT.EXE (extended InitechMZ) and, when it fits the conventional arena,
TURBOOUT.COM. It still writes TURBOOUT.S as an independent factory-oracle input.
No guest assembler or linker is needed for the native output. See NATIVE.md.

`make` builds the host FPC compiler, seed-built extended TURBO.COM and a bare
Multiboot link-check artifact. Dependencies: seed's compiler and public runtime
artifacts/scripts, factory/mz's packaging/checking tools, and own sources and
fixtures. No private kernel header/code is used. The hosted bootstrap uses
FPC -B -O2; independent corpus execution references still use -O-. Delphi/B+/H-
semantics are preserved. NASM filenames/order and ELF scripts are fixed for determinism.
All 40 entries remain. The two self entries share two fresh full compiler
executions inside each invocation's private scratch directory; both entries
retain every protocol, determinism and artifact assertion. Generation stays
inside the timed runner, and no results survive into another invocation.
Each pass assembles once with NASM, reusing that fresh object for its bare,
DOS and two extended link bases. A native-fixup fault still fails after a good
run in the same OUT, then restoring the compiler passes. Profiling all 40
entries measured 21.94s before and 14.66s after at low load (an earlier loaded
baseline was 30.50s); the dominant work is CPU-bound.
The isolated-seed entry checks that a fresh seed
prerequisite is built into the requested SEED_OUT even with a different compiler
OUT. Recursive seed builds explicitly select their own output directory; cached
tools must not hide a missing prerequisite. The 30s suite budget and 20s entry
timeout are unchanged.

`make test`: 40 host entries, including independent FPC execution references,
17 flat and 18 extended byte-equality comparisons, runtime-table regeneration,
and both compiler placements checked against real extended limits. It does
not execute emitted x86 code. Parent `make -C turbo smoke` boots the actual
product image with separate TEST disks, compares generated-program stdout
against FPC on QEMU and Bochs, and reads files back with mtools.

`make mutants` uses mk/mutants.mk: seven inherited patches plus five native
encoder/relocation/layout patches and three execution patches. The
frame-offset and var-destination catchers execute tiny/function corpus output
against FPC on both emulators, exclusively in tier 2. The flat-runtime-digit
patch changes only conventional runtime decimal printing; execution-flat
checks both hosted forms, preserving the correct extended output as a control.
Generated files/logs remain in build/turbo/compiler; make clean removes them.
The protocol and corpus derivations are in the local *-DUMP/TRACE, CODEGEN and
CORPUS documents; tests/reference is parser-derived documentation, not an oracle.

Measured placement: seed module 111924, reserve 174408, module headroom 150220,
free window after aligned resident and heap header 1741168. Pascal-generated
module 149575, reserve 174409, module headroom 112569, free window 1703520.
Both have separate 65536-byte stack and 4096-byte guard. These are mechanical
layout limits, not a measured worst-case stack-usage claim.

The parent smoke executes the complete 17-program corpus plus 16 regressions
on both emulators, through seed, hosted Pascal native and resident Pascal
native paths. Both hosted and resident Pascal paths execute flat COM as well
as extended EXE, for 330 total comparisons (all original 280 plus 50 Phase 2
regression comparisons). No host assembler/linker finishes
resident-generated programs.
Stdout is compared with the independent goldens and FPC reference executions;
mtools checks the on-disk payloads. Guest DOS GET TIME brackets each compile
at its documented 1-second resolution. Tests use a separate 8 MiB FAT12 TEST
volume (8 sectors/cluster); the inherited executable loader does not bind FAT16.

`make -C turbo selfhost` is the in-OS K2==K3 certificate, not K1==K2. The
current certified fixed point is 164263 bytes, SHA-256
2c33ed0c7ee667138cfb3253630994be169cc8a22a1f4c54715a4eb40f9e0fc7.
Its module grows by 1064 bytes and relocation header by 112 bytes over the
previous 163087-byte compiler for div/mod guard emission and label handling.
The two compilations use fresh boots with identical source/disk input names.
Fresh guest times were 55s/103s, within the unchanged 120-second gate. A deliberately changed byte
makes the comparator red with its offset and container region. Full evidence
and source identity remain under build/turbo/selfhost. No stack/CPU fault or
host assembler/linker participates in either compilation.

`make -C turbo ddc` refreshes the seed certificate and runs the independent
FPC bootstrap path: host Pascal compiler -> D1, in-OS D1(S) -> D2. D2 equals
both K2 and K3 at the hash above; D1(S) took 109 guest seconds. Contaminated
candidate and changed-source controls are red. DDC evidence is retained under
build/turbo/ddc. This removes sole trust in seed translation; source/runtime,
FPC, host tools, the OS and hardware remain trusted. Both-emulator generated
execution additionally checks translation semantics that byte agreement and
self-hosting alone cannot establish.

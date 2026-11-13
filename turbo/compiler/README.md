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
All original 39 entries remain: hosted execution fell from 16s to 13s on the
integration machine. The added isolated-seed entry checks that a fresh seed
prerequisite is built into the requested SEED_OUT even with a different compiler
OUT. Recursive seed builds explicitly select their own output directory; cached
tools must not hide a missing prerequisite. The 30s suite budget and 20s entry
timeout are unchanged.

`make test`: 40 host entries, including independent FPC execution references,
17 flat and 18 extended byte-equality comparisons, runtime-table regeneration,
and both compiler placements checked against real extended limits. It does
not execute emitted x86 code. Parent `make -C turbo smoke` boots the actual
product image with a separate TEST disk, checks generated assembly against
host output on QEMU and Bochs, and reads files back with mtools.

`make mutants` uses mk/mutants.mk: seven inherited patches plus five native
encoder/relocation/layout patches. All five native patches have been killed.
Two inherited wrong-code cases still need guest execution (tests inventories).
Generated files/logs remain in build/turbo/compiler; make clean removes them.
The protocol and corpus derivations are in the local *-DUMP/TRACE, CODEGEN and
CORPUS documents; tests/reference is parser-derived documentation, not an oracle.

Measured placement: seed module 111645, reserve 174411, module headroom 150499,
free window after aligned resident and heap header 1741440. Pascal-generated
module 149219, reserve 174409, module headroom 112925, free window 1703872.
Both have separate 65536-byte stack and 4096-byte guard. These are mechanical
layout limits, not a measured worst-case stack-usage claim.

The parent smoke now compiles and executes the complete 17-program corpus on
QEMU and three cases on Bochs, with no host assembler/linker finishing programs.
Stdout is compared with the independent goldens and FPC reference executions;
mtools checks the on-disk payloads. Guest DOS GET TIME brackets each compile
at its documented 1-second resolution. Tests use a separate 8 MiB FAT12 TEST
volume (8 sectors/cluster); the inherited executable loader does not bind FAT16.

`make -C turbo selfhost` is the in-OS K2==K3 certificate, not K1==K2. The
current fixed point is 163439 bytes, SHA-256
08499523b5b74b394a05775008481162948b81054adf989785cee18902d55b3f.
The two compilations use fresh boots with identical source/disk input names.
Guest times after runtime hook removal were 45s/50s; mzcheck accepts both. A deliberately changed byte
makes the comparator red with its offset and container region. Full evidence
and source identity remain under build/turbo/selfhost. No stack/CPU fault or
host assembler/linker participates in either compilation.

`make -C turbo ddc` refreshes the seed certificate and runs the independent
FPC bootstrap path: host Pascal compiler -> D1, in-OS D1(S) -> D2. D2 equals
both K2 and K3 at the hash above; D1(S) took 59 guest seconds. Contaminated
candidate and changed-source controls are red. DDC evidence is retained under
build/turbo/ddc. This removes sole trust in seed translation; source/runtime,
FPC, host tools, the OS and hardware remain trusted. All 12 compiler mutants,
including the seven inherited protocol patches, have now been killed.

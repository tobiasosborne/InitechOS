# Turbo Initech compiler

Pascal compiler, also built by Free Pascal on the host for independent bootstrap
and differential checks. One `TURBO.COM` opens the IDE without arguments and
compiles a source filename from the command line. Ordinary compilation checks
semantics and emits native `TURBOOUT.EXE` (extended InitechMZ) and, when it fits
the conventional arena, `TURBOOUT.COM`. `/DUMP filename` additionally runs the
lexer/parser protocols and writes NASM assembly for the independent factory
oracle. No guest assembler or linker finishes native output. See NATIVE.md.

`make` builds the host FPC compiler, seed-built extended TURBO.COM and a bare
Multiboot link-check artifact. Dependencies: seed's public compiler/runtime
artifacts and scripts, factory/mz's packaging/checking tools, and own sources
and fixtures. No private kernel header or code is used. The hosted bootstrap
uses FPC -B -O2 with HostRTL injected only into that host build; independent
program references use FPC -O-. Integer stays signed 32-bit.

`make test` runs 44 host entries, including 19 independent FPC execution
references, 19 flat and 20 extended native byte comparisons against NASM/ld,
runtime-table regeneration and real extended placement limits. Emitted x86
executes only in tier 1/2. Every generation runs twice; both emissions compare
to one fresh foreign-tool result and to each other. Self protocols and
self-generation share their two fresh translations inside the invocation's
private scratch directory. The CLI check reuses those independently graded
dump outputs, checks input bytes before reuse, and still compiles ordinary
mode and DOS-saved source separately. Standalone selectors build their own
inputs. Nothing is cached across test invocations. The 30-second suite budget
and 20-second entry timeout are unchanged.

The CLI check compares all 19 ordinary/dump COM and EXE pairs, six invalid-type
diagnostics against independent goldens, and LF versus CRLF+DOS EOF whole
compiler containers. Input aliases of the compiler's output names are refused
before opening an output; conservation checks retain the source bytes. The
isolated-seed entry requires a fresh requested SEED_OUT even with a different
compiler OUT. The runtime entry inventory compares public intrinsic offsets
with the independent NASM/ld symbols from the component under test.

`make mutants` uses mk/mutants.mk and 28 checked-in patches. Frame-offset,
var-destination, flat-runtime-digit and recoverable-I/O catchers execute
programs against FPC on both emulators in tier 2. Native byte comparisons also
reject encoder, fixup, relocation and placement faults. Tests emit positive
counted summaries. Tool failures, harness timeouts and missing serial are
errors rather than kills; a comment-only patch survives. Evidence and source
identity remain under build/turbo/compiler. Protocol and corpus derivations
are in the local *-DUMP/TRACE, CODEGEN and CORPUS documents; tests/reference is
implementation-derived documentation, not an oracle.

The parent `make -C turbo smoke` executes all 19 corpus programs plus 19
regressions on QEMU and Bochs through seed, hosted Pascal and resident Pascal
paths. Both Pascal paths run extended EXE and flat COM: 380 executions.
Complete stdout and extracted files match independent FPC runs, and original
corpus stdout also matches authored goldens. Resident compilation uses ordinary
mode, without trace or assembly dumps. Bochs uses published progress renewal
with the existing 300-second initial budget and shared three-budget ceiling.
DOS GET TIME brackets resident compilation at its one-second resolution.
Separate FAT12 TEST volumes hold fixtures; the product boot disk is unchanged.

`make -C turbo selfhost` performs K1(S) -> K2 and K2(S) -> K3 inside the OS on
both emulators. The certificate is whole-container K2==K3, never K1==K2.
`make -C turbo ddc` requires that source-bound proof, builds an independent FPC
bootstrap D1, and compiles D1(S) -> D2 in the OS on both emulators. All K2/K3/D2
containers must agree across emulators. The parent README records the current
size, SHA-256 and timings. Evidence is retained under build/turbo/selfhost and
build/turbo/ddc. Corrupted-container, clean loader-refusal, impossible-deadline,
contamination and changed-source controls must fail for their stated reasons.
Neither proof writes factory trace dumps or assembly listings.

The loader supplies a separate 65536-byte stack and 4096-byte guard. Native
module/reserve limits are checked mechanically; this is not a measured
worst-case stack bound. Runtime-encoder authorship remains initech-cd2yi.
ParamStr(0) additionally needs the published DOS executable-name environment
trailer from the kernel (initech-311t2); the standalone raw-PSP probe records
that external gap without inventing a path.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
<!-- end-interface-declarations -->

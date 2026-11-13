# Turbo Initech compiler

The resident Pascal compiler, also buildable with Free Pascal for host checks.
Its four passes reopen a fixed input: lexical dump, syntax trace, semantic
parse/symbol dump and an AST-free NASM emitter. The output is assembly; host
NASM and binutils are still required. See ROADMAP.md for remaining work.

`make` builds the FPC host program and seed-built extended InitechMZ TURBO.COM, plus a bare
Multiboot link-check artifact (file calls cannot run without InitechDOS).
Dependencies: seed's host compiler and rt/ startup, file runtime and link scripts;
otherwise only this component's source/fixtures. No C/private kernel includes.
FPC flags preserve the legacy -B -O- and source Delphi/B+/H- pins. NASM input
names are identical in paired scratch directories to keep its FILE symbol
reproducible. GNU ld uses elf_i386, the inherited scripts and no build ID.
`make test` runs 38 host entries. FPC is mandatory even for cached artifacts.
`make mutants` uses mk/mutants.mk and seven component-relative patches; two
wrong-code mutants need guest execution and remain catalogued. Source comments
mark their historical loci but do not select deliberately wrong production code.

LEXER-DUMP.md, PARSER-TRACE.md, SYMBOLS-DUMP.md, CODEGEN.md and CORPUS.md preserve
the inherited contracts/derivations, with names updated in the rename commit.
The grammar claimed into tests/reference is documentation derived from our parser;
it is not an independent oracle. Publication build/replay tools stay with pubs.
The complete gate classification and deferred smokes are in tests/. No execution
of generated programs is claimed by host tests. Memory checks package both
compiler forms through tests/package.sh and the independent factory/mz checker.
The parent turbo smoke boots the extended compiler on QEMU and Bochs using a
separate TEST disk and extracts its assembly with mtools. Generated files/logs
go under build/turbo/compiler, and `make clean` removes them.

Placement evidence (M2): seed module 89617, reserve 71707, module headroom
172527, free high-window bytes after alignment and heap header 1866176.
Pascal-generated module 113027, reserve 71705, module headroom 149117,
free bytes 1842768. Stack 65536 and guard 4096 are separate in both cases.
QEMU/Bochs gen_control_deep assembly compares passed (2s/5s total sessions).
The inherited unchecked UART helper drops diagnostic bytes on Bochs; the smoke
uses shell completion and the complete on-disk assembly equality, not those
lossy diagnostics, as its oracle. Top-level make test and factory/mz test passed.

# Turbo Initech compiler

The resident Pascal compiler, also buildable with Free Pascal for host checks.
Its four passes reopen a fixed input: lexical dump, syntax trace, semantic
parse/symbol dump, and an AST-free NASM emitter. The fixed output is assembly;
assembling/linking still happens on the host. No IDE or native executable emitter
exists. See ROADMAP.md for the measured self-compilation result and next steps.

`make` builds the FPC host program and seed-built resident COM, plus a bare
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
of generated programs is claimed by host tests. All generated files/logs go under
build/turbo/compiler, and `make clean` removes them. Smoke fails pending the image.

# Seed mutation hooks awaiting patches

17 distinct C macro hooks, 24 conditional sites (22 in codegen.c, one lexer,
one parser; mutual-exclusion guard included). Retained unchanged: conversion
of the full codegen/RTL sweep is substantial and most catchers need the image.
`make mutants` fails explicitly, rather than reporting a completed zero sweep.
No wrong variant is selected by the normal build. Source line numbers below
are pre-rename/import locations; identifiers are stable.

| Source:line | Hook (SEED_MUT_ prefix) | Fault / intended catcher |
|---|---|---|
| lexer.c:274 | NUM_RANGE | omit signed-integer range rejection; lexer UNIT |
| parser.c:553 | PARSE_ADD_AS_MUL | addition becomes multiplication; parser UNIT (9 red checks demonstrated) |
| codegen.c:219,2727 | NONDET | emit process PID bytes; repro host pairs plus deferred live bool control |
| codegen.c:224,1943,2709 | FILEIO_SHORT_WRITE | select short-write entry; fileio-build call sites and guest fileio-os-mutant |
| codegen.c:224,1948,2711 | FILEIO_WRONG_HANDLE | select sink handle entry; fileio-build and guest fileio-os-mutant |
| codegen.c:570 | CODEGEN_FRAME_OFF4 | frame slots +4; func-mutant |
| codegen.c:594 | CODEGEN_FIELD_OFF4 | record fields +4; record-mutant |
| codegen.c:613 | CODEGEN_VARPARAM_COPY | var param copied; func-mutant |
| codegen.c:924 | CODEGEN_ARRAY_LO_SKIP | omit lower bound; array-mutant |
| codegen.c:940 | CODEGEN_STRIDE | scalar stride 2; array-mutant |
| codegen.c:949 | CODEGEN_REC_STRIDE | record stride one field short; record-mutant |
| codegen.c:1298,1305,1385,1405 | CODEGEN_SETL_AS_SETG | invert signed relation; bool-mutant |
| codegen.c:1351 | CODEGEN_MUL_AS_ADD | imul becomes add; codegen-mutant |
| codegen.c:1579 | CODEGEN_CHR_OFF1 | chr argument +1; char-mutant |
| codegen.c:1669 | CODEGEN_BRANCH_INVERT | jz becomes jnz; control-mutant |
| codegen.c:2039 | CODEGEN_STR_TEMP_CLOBBER | all temps index zero; string-mutant RNEST |
| codegen.c:2546 | CODEGEN_STR_CMP_NOLEN | ignore length tiebreak; string-mutant EQF |

Catchers without a host label are legacy test-seed-* emulator gates, enumerated
in SMOKES.md. Convert repeated sites together with component-relative patch -p1
files and mk/mutants.mk once the selected test really executes the emitted code.
The first three can be converted independently as a small follow-up. Do not
count different assembly as a semantic kill. The fileio.asm SHORT_WRITE and
WRONG_HANDLE entry points are also deliberately wrong legacy RTL support
(entry wrappers at lines 141-152); they are unreachable from normal emission and should
be removed into patches together with their C selectors, not independently.

# WL-0137 -- The Turbo Initech manuals: five F-BOR books gated against the shipped compiler

## Context

Bead initech-ipu5.12 (writing + gating lane, branch lane/tpman, base
4832d01). Write the Turbo Initech set of the Documentation Set (PUB-REF-0001
Sec 12, rows TIN-UG, TIN-PG, TIN-LR, TIN-SCG, TIN-QR) in family F-BOR on the
Turbo Pascal 6.0 model (docs/pubs/originals/borland-turbo-pascal.md), and
build the shipped inventory and correctness gates the Turbo Initech books
need (Sec 11, GC-01/02/03/09, CP-18/19), wired into the pubs vectors.
Ground truth: ADR-0007 (DEC-02 the language), os/tps/tps.pas (the shipped
compiler), its dump specifications (LEXER-DUMP, PARSER-TRACE,
SYMBOLS-DUMP, CODEGEN, CORPUS), seed/ (the genesis compiler and the
run-time objects), the compiler gates of the Makefile.

## What changed

Books (pubs/manuals/turbo-initech-*/):

- turbo-initech-programmers-guide (1077-0000-11): Introduction; Part 1 the
  language (tokens, blocks and scope, types, variables, expressions,
  statements, procedures and functions, programs) with one railroad
  diagram per production of the shipped parser (36) plus the lexical
  diagrams; Part 2 the standard routines; Part 3 memory, control (calling
  convention with replayed assembly), I/O, code generation, directives
  (none); Part 4 the assembly-language output; Part 5 App. A every
  diagnostic (AN-13, 37 entries, texts verbatim), App. B token kinds (63),
  operators, ASCII.
- turbo-initech-library-reference (1077-0000-12): the Sample entry and one
  AN-12 entry per routine A-Z (10) with a replayed example each; App. A
  one entry per run-time entry point (15); App. B routines programs write
  in Pascal (IntToStr, StrToInt, UpCase, Copy/Pos, ReadLine); guide words
  and stepped letter tabs.
- turbo-initech-users-guide (1077-0000-10): Introduction; AN-19 tutorial
  chapters 1-5 (getting started, programming, larger programs, debugging
  with the compiler's own listings, building programs); Chapters 6-7 (the
  integrated environment, the editor) DRAFT, stating only what ADR-0007
  DEC-08 ratifies; Chapter 8 the TPS command reference (every protocol
  line, dump row and capacity registered with its shipped value).
- turbo-initech-self-compilation-guide (1077-0000-13): the bootstrap (K1,
  K2, K3; the certificate is K2 == K3, never K1 == K2), the genesis
  compiler INITECHC (switches registered), building on the development
  system, the AN-15-style acceptance procedure TIN-P-01..05 with every make
  run replayed, independent compilation, reproducible builds, the corpus.
- turbo-initech-quick-reference (1077-0000-20): PK-11 booklet, 88.9 x
  228.6 mm, Notes pages to the signature, booklet cover + back cover.
- Every book: AN-20 verso (slogan under the edition notice), Contents,
  index (Adventor Oblique locators), colophon, cover + PK-05 back cover
  with the slogan; spines for the perfect-bound books.

Toolchain and gates:

- pubs/class/initech-borland.sty (new; only the Turbo Initech books load
  it): F-BOR furniture (letterspaced display words, parts, A/B/C heads,
  two-column contents, folio/foot rules), notation (\rw, \std, \id),
  listings with the keyword list generated from the shipped lexer, the
  AN-12/AN-13/AN-15 environments, railroad and lexical diagrams, the
  replayed-example macros (\Program, \DosProgram, \ProgramOutput,
  \CompilerOutput, \AssemblyOutput, \MakeRun, \Source), registration of
  every documented item through \RegisterCommand, <job>.msgs, .tpsmentions,
  .tpsexamples, .gates; the booklet trim.
- harness/pubs/tps_gate.c: the shipped inventory GENERATED from the
  product (reserved words by running the shipped lexer on a probe and
  cross-checking the seed lexer, divergence `case' recorded; routines,
  productions, token kinds, diagnostics with their code numbers and texts,
  protocol lines, dump rows, capacities with values, INITECHC switches,
  run-time entry points: 238 items) and the both-direction checks.
- harness/pubs/railroad.c: draws the printed grammar
  (grammar/tps.ebnf) and proves it against the shipped parser both ways:
  300 generated programs must parse (grammar => parser, every alternative
  covered, ordered choice honoured) and the corpus, tps.pas and every book
  program must be derived (parser => grammar).
- harness/pubs/tps_replay.sh: every printed program runs through Free
  Pascal, the seed and Turbo Initech (bare images under the harness), and
  every Turbo Initech .COM runs in one InitechDOS boot; compiler regions,
  assembly excerpts and make runs replayed byte for byte.
- harness/pubs/tps_gates_cited.sh (GC-09), tps_grammar.sh, tps_asmblock.sh,
  tps_mutants.sh (8 named mutants).
- pubs/tps.mk (included from pubs.mk): pubs-tps-gen, test-pubs-tps,
  test-pubs-tps-mutant; test-pubs and test-pubs-mutant depend on them;
  pubs.mk test-pubs-commands exempts turbo-initech-* (own inventory).

## Why

Law 1/2 for a manual: every sentence about the language was read from
tps.pas and checked by compiling probe programs with the shipped
compiler; every example is a replayed program, and the books cannot drift
from the compiler without test-pubs-tps going red.

## Frictions

1. The scratchpad is shared by all lanes: another lane's helper
   overwrote mine (one.sh) once and ran in their tree with my arguments
   (it failed harmlessly). Lane helpers now live in scratchpad/tpman/.
2. test-pubs-screens is RED on the base: initechdos-reference chkdsk.txt
   shows 118448 bytes of memory, the OS now reports 118320 (an OS change
   after the DOS books were captured). Not a Turbo Initech book; the DOS
   lane must re-capture. It blocks the test-pubs umbrella.
3. TPS output goes to COM1 only (seed/rt serial_*), so no console screen
   of TPS is meaningful; the books show replayed compiler output instead.
4. No resident assembler/linker: TPSOUT.S is assembled and linked on the
   development system (nasm, ld, objcopy). DRAFT where the books describe
   the InitechOS cycle; the SCG prints the development-system commands.
   Free Pascal and NASM are named in the SCG (post-1993 product names, not
   on the denylist) because the procedures must be exact.
5. Compatibility traps found while writing (all documented as shipped):
   a write argument that begins with a string literal is the literal alone
   ('A' + S needs parentheses); Reset/Rewrite default to record size 1 (Free
   Pascal 128); value string/record parameters are rejected; var string
   only of capacity 255; files cannot be parameters; a call needs ()
   always; a later routine needs forward; TPS exits 0 on compile errors and
   leaves an old TPSOUT.S in place.
6. GEN-UNKNOWN codes collide (200 + category 100..111 overlaps 300 + lex
   code 1..10 in tps.pas FailParse/FailLex gen mode); documented
   neutrally. Wants a bead.
7. The period-year lint flags any 4-digit number past 1993 in .tex/.txt:
   capacities print as 6,144 and the inventory formats them so.

## Acceptance

Books (build/pubs/gate, `make test-pubs-build` green, 0 overfull boxes,
page counts multiples of 4): User's Guide 68 pp (6 DRAFT markers),
Programmer's Guide 132 pp (3), Library Reference 44 pp (1), Self-
Compilation Guide 36 pp (5), Quick Reference 32 pp (2); covers 2 pp each
(front + back), spines 1 p. 202 printed items replayed (73 programs).

```text
>>> test-pubs-tps-commands: green (238 shipped == documented: 40 reserved, 10 routines, 36 productions, 63 tokens, 37 messages, 18 protocol, 9 dump rows, 7 limits, 3 switches, 15 run-time entries; 0 DRAFT)
>>> test-pubs-tps-messages: green (37 diagnostics printed verbatim)
>>> test-pubs-tps-mentions: green (50 distinct inline Pascal names, all shipped)
>>> test-pubs-tps-gates: green (62 distinct gate citations per book, every one a real target)
>>> test-pubs-tps-grammar: 300 generated programs accepted by the shipped parser (grammar => parser)
>>> test-pubs-tps-grammar: green (92 accepted programs derived by the printed rules: parser => grammar)
>>> test-pubs-tps-examples: green (202 printed items replayed; 73 programs: Free Pascal == printed, 59 also seed and Turbo Initech bare images == printed, 73 Turbo Initech .COM files == printed on InitechDOS)
>>> test-pubs-tps: green
>>> test-pubs-tps-mutant: green (8 named mutants rejected by their gate legs)
>>> test-pubs-repro: green (21 PDFs byte-identical across two clean builds)
>>> test-pubs-repro-mutant: green
>>> test-pubs-families: green (7 model families typeset with their required body faces)
>>> test-pubs-lint: green (ASCII, period vocabulary/years, part numbers and print codes)
>>> test-pubs-mentions: green (43 distinct inline commands)   [test-pubs-commands]
>>> test-makefile-vars: green
>>> test-pubs-mutant (DOS pilot legs, run directly): green (11 named mutants)
```

Mutants (tps_mutants.sh): FAKE_ROUTINE (DOC-NOT-SHIPPED Halt|routine),
DROPPED_ENTRY (SHIPPED-NOT-DOCUMENTED blockwrite|routine), WRONG_LIMIT
(DOC-NOT-SHIPPED SYMBOLMAX=1,025|limit), CHANGED_MESSAGE (MESSAGE-TEXT),
EXTRA_ALTERNATIVE (GRAMMAR-PARSER), MISSING_ALTERNATIVE (GRAMMAR-INCOMPLETE,
stops at KW downto), CHANGED_OUTPUT (TRANSCRIPT-DRIFT), FAKE_GATE
(GATE-CITATION).

Not green: test-pubs-screens (and so the test-pubs umbrella) is RED on the
pre-existing DOS drift of friction 2 (initechdos-reference chkdsk.txt).

## Where to resume

- Extents against the DS planning targets: UG 68 (180-260), PG 132
  (200-320), LR 44 (80-160), SCG 36 (60-120), QR 32 (32-48). Everything the
  compiler ships is documented; the model's remaining bulk is features
  Turbo Initech does not have (the IDE and editor, OOP, the debugger, the
  Graph/Dos/Crt/Overlay units, inline assembler, 220 RTL routines).
- 8. The two-column contents overfilled a page once (multicols, SCG);
  tightened spacing cleared it, the cause is not understood.
- When M8 lands: clear the DRAFT markers (UG 6, 7; SCG TIN-P-04/05; PG/UG
  resident assembler notes; QR editor keys) and add AN-13 IDE entries.

## Pointers

- pubs/manuals/turbo-initech-{users-guide,programmers-guide,library-reference,self-compilation-guide,quick-reference}/
- pubs/class/initech-borland.sty, pubs/tps.mk
- harness/pubs/{tps_gate.c,railroad.c,tps_replay.sh,tps_grammar.sh,tps_gates_cited.sh,tps_asmblock.sh,tps_mutants.sh}
- pubs/manuals/turbo-initech-programmers-guide/grammar/{tps,lexical}.ebnf

# Resident compiler import: legacy-final comparison

Legacy recipes: Makefile:16769-16773,17579-18698,31932-31936. Read together
with the local four protocol/codegen contracts and CORPUS.md. All 17-fixture
FPC roots and self-generation were actually run in the absolute legacy tree
listed in seed/tests/IMPORT.md. The final combined host/mutant run took 17.80s.
No live generated-code execution was run here.

| Legacy gate | Class | Destination / disposition |
|---|---|---|
| test-tps-lex-fpc | ORACLE-STRONG hand protocol + determinism property | 5 lex_* entries; self-protocols/LEX |
| test-tps-parse-fpc | ORACLE-STRONG hand protocol + determinism property | 6 parse_* entries; self-protocols/PARSE |
| test-tps-type-fpc | ORACLE-STRONG hand protocol + determinism property | 7 type_* entries; self-protocols/TYPE |
| test-tps-lex-build | META + SIZE/LAYOUT | all, seed-build: both links, rolling window, BSS<=73728, end<0x6f000 |
| test-tps-parse-build | META | seed-build: parser symbol, nonempty artifacts |
| test-tps-type-build | META + SIZE/LAYOUT | seed-build: symbol arena/driver |
| test-tps-gen-build | META | seed-build: generator, fixed output, BlockWrite |
| test-tps-gen-fpc | ORACLE-STRONG FPC roots + META/property + SIZE/LAYOUT | 17 gen_* entries plus self-generation; each emits twice, assembles, links both targets, compares all artifacts |
| test-compiler | META aggregate | turbo test / compiler test; preserves the 17/17 host build/root check, not an execution pass rate |
| test-tps-lex-mutant | protocol sensitivity | casefold, quote-escape patches |
| test-tps-parse-mutant | protocol sensitivity | precedence, dangling-else patches |
| test-tps-type-mutant | protocol sensitivity | duplicate, assignment patches |
| test-tps-gen-mutant | build sensitivity / wrong-code artifacts | labels patch; OFFBYONE and VARPARAM catalogued for execution follow-up |
| test-tps-lex-os | ORACLE-STRONG cross-build, EMULATOR | SMOKES.md |
| test-tps-parse-os | ORACLE-STRONG cross-build, EMULATOR | SMOKES.md |
| test-tps-type-os | ORACLE-STRONG cross-build, EMULATOR | SMOKES.md |
| test-tps-gen-os | ORACLE-STRONG execution, EMULATOR | SMOKES.md |
| test-tps-gen-os-mutant | execution sensitivity, EMULATOR | SMOKES.md |
| test-compiler-os | ORACLE-STRONG FPC differential, EMULATOR | SMOKES.md: 17 rows, 100% required |
| test-compiler-os-mutant | execution sensitivity, EMULATOR | SMOKES.md |
| selfhost | META stub, not a proof | turbo selfhost still explicitly fails; ROADMAP.md |
| ddc | META stub, not a proof | turbo ddc still explicitly fails; ROADMAP.md |

22 distinct compiler gates: 13 host-only including 4 mutation gates, 7 requiring
an emulator, 2 unimplemented stubs. Combined with seed: **41 gates = 19 host-only
+ 20 emulator-dependent + 2 stubs**. Gate names are not comparable to runner
entries: aliases and several assertions share a legacy recipe. Publication
ownership below is excluded from this component count.

| Comparable normal checks | Legacy actually run | New tier 0 |
|---|---:|---:|
| lexer protocol fixtures | 5 | 5 |
| parser protocol fixtures | 6 | 6 |
| type protocol fixtures | 7 | 7 |
| deterministic self lex/parse/type | 3 paired checks | same 3, grouped as 1 entry |
| corpus FPC stdout vs hand goldens | 17 | 17 |
| generated corpus paired assembly/object/bare/DOS | 17 x 4 | same + paired flat COM |
| deterministic self assembly, both links | 1 | same + paired object/ELFs/COM |
| seed-built compiler + static contracts | 4 gate names | 1 entry, same contracts |
| mutation faults checked on host | 9 (7 actual red, 2 changed artifacts only) | 7 patch mutants killed; 2 deferred |
| default host runner entries | 9 normal gate names | 38 |

Combined normal legacy: 13 gate names; new: 44 entries. Normal behavioural
quantities retained: 212 C assertions, 18 protocol fixtures, 3 self-protocol
pairs, 17 FPC execution roots, 15 seed repro fixtures/45 artifact pairs, and
successful paired self-generation. New seed reference-only work adds 4 FPC
runs from the deferred seed differential. Static grep/link/size assertions
are not added to the behavioural assertion total.

Important oracle distinction: the FPC binary built from our compiler source
is still OUR implementation. Protocol goldens were hand-computed, as stated
in LEXER-DUMP.md, PARSER-TRACE.md, SYMBOLS-DUMP.md and the legacy recipes; their
payload is not minted from the FPC-hosted compiler. gen_*.golden is the hand
stdout root independently checked by FPC compiling each PROGRAM. Tier 0 only
checks that root and our generation/link/determinism, not generated execution.
There was no seed-output assembly golden or seed-vs-Pascal assembly comparison:
legacy deliberately compared executed stdout instead. None is fabricated here.

Everything already in compiler/ (source, fixtures, goldens and contracts) is
retained. The grammar is claimed with git mv from the programmer's manual,
keeps history, and is labelled implementation-derived documentation. No attic
implementation is built. The seed test sources also move with git mv. The
Report Generator's pubs/spec/harness files are untouched: a distinct DOS product.

The related publication gates test-pubs-turbo-inputs (META transcript generation,
transitively EMULATOR), test-pubs-turbo (META inventories/grammar + UNIT generated
grammar properties + ORACLE-STRONG printed-program replay, EMULATOR), and
test-pubs-turbo-mutant (META/protocol/execution sensitivity, EMULATOR) remain
publication-owned in attic/pubs/turbo.mk and attic/harness/pubs/turbo*. They
also depend on TeX book inventory and the full legacy image; this import does
not build books or certify their transcripts. Shared test-gate-honesty,
gate_map/spec inventories, tenant image/headroom wrappers are factory META or
SIZE/LAYOUT, not additional compiler language tests. No old lane machinery is
ported. This is an import of existing behaviour, not a proof of item 7.

Red control: removed identifier lowercasing at the named Pascal seam;
`make test TESTS=lex_basic` exited 2 with mixed_id -> MiXeD_Id and _second ->
_Second golden differences. Source restored byte-for-byte, then full green.
Seven isolated patch mutants build successfully and fail selected host tests;
the LABELS fault is rejected by NASM during the generation test, as in legacy.

## Acceptance before rename

From empty build/turbo: `make -C turbo test` passed 6 seed + 38 compiler
entries in 16.74s. Top-level `make test` passed region, Initech 123 and Turbo
Initech, printed ALL GREEN, 29.58s. Seven Pascal patch mutants were killed.
The seed and Pascal manual red controls above both exited 2; originals restored.
`/usr/bin/make -C turbo test PATH=/nonexistent` exited 2 at the explicit FPC
preflight even with cached build outputs. POSIX shell syntax, git diff --check
and ASCII-only C/Pascal/assembly/link sources passed. No original product source
is edited in the import commit. Fixed input/output are still TPSIN.PAS/TPSOUT.S
and resident output TPS.COM until the separately tested rename commit.

## Rename and final acceptance

The separate rename commit follows green import 1d4de0b5. Migration:

| Historical | Current |
|---|---|
| tps.pas / tps.ebnf | turbo.pas / turbo.ebnf |
| TPS.COM | TURBO.COM |
| TPSIN.PAS | TURBOIN.PAS (DOS 8.3) |
| TPSOUT.S | TURBOOUT.S (DOS 8.3) |
| TPS-{LEX,PARSE,TYPE,GEN,CORPUS}-* | TURBO-{LEX,PARSE,TYPE,GEN,CORPUS}-* |
| TPS_* anchors / tps_* build artifact names | TURBO_* / turbo_* |

Source comments and protocol/corpus documentation now use the product name;
references to the absent retired subset-plan filename are described as the
legacy Pascal subset plan, not replaced by a fictitious renamed document.
All 17 gen_*.golden files are byte-for-byte unchanged. Live independent FPC
compiles/runs rechecked every one. The 18 protocol goldens change ONLY their
branding prefix, by the specified literal bijection TPS -> TURBO. Their
hand-derived token/trace/symbol/error payload is unchanged; no implementation
output was used to mint expectations. The assembly banner loses the retired
codename. No language, ABI, runtime or code-generation rule changes.
The TPS Report Generator and all its files remain untouched.

Final from empty build/turbo: `make -C turbo test`: 44/44, 15.77s.
Top-level `make test`: ALL GREEN, 20.70s. The renamed patch sweep reports
7 mutants, 7 killed, 0 survived (all built first). Cached compiler-only
`make test PATH=/nonexistent` also exits 2 with the explicit FPC-required error.
Before-rename times and red controls are recorded above. Final source is
156748 bytes / 5033 lines; self assembly 765329 bytes / 1938 labels; seed COM
89633 bytes, Pascal-generated COM 113043 bytes. BSS remains 71704 bytes;
seed DOS end=0x6773c, Pascal DOS end=0x6d2ac (7508 bytes below the ceiling).
No guest execution or native emission is claimed by these numbers.

`grep -rnw 'TPS\|tps' turbo` hits only compiler/tests/IMPORT.md (historical gate
names, pre-rename evidence, this migration) and compiler/tests/SMOKES.md (exact
legacy gate names needed to recover their recipes). There are no active old
identifiers, paths, protocol tags, source filenames or guest filenames. All
C/Pascal/ASM/link sources are ASCII; shell syntax and git diff --check pass.
Moves are git renames: the seed tests and claimed grammar keep history in the
import commit; the Pascal source and grammar name keep history in the rename.

# Seed import: legacy-final comparison

Authority: legacy Makefile:16782-17576,18898-19142; unchanged seed sources and
fixture header derivations. The legacy tree is /home/tobiasosborne/Projects/InitechOS-legacy.
Host targets were actually run there with `. tools/lane-env.sh`; no emulator
was run for this import. All host gates below passed, including their old mutant
build checks. Legacy compilation used cc -std=c11 -Wall -Wextra -Werror
-D_POSIX_C_SOURCE=200809L -Iseed. New compilation uses shared HOST_CFLAGS (-O1 -g
added); no language, target calling convention or runtime behaviour changed.

| Legacy gate | Class | New destination / disposition |
|---|---|---|
| test-seed | UNIT | lexer (97 checks), parser (115 checks, includes typechecking) |
| test-seed-mutant | UNIT sensitivity | 1 parser fault; hooks catalogued, patch sweep deferred |
| test-seed-num-range-mutant | UNIT sensitivity | 1 lexer fault; hooks catalogued, patch sweep deferred |
| test-seed-repro | META/property | repro: same 15 fixtures, 45 assembly/object/ELF pairs |
| test-seed-fileio-build | META + SIZE/LAYOUT | fileio-build: clean compile/link and static contracts; the two deliberately wrong variants remain hooks for follow-up |
| test-seed-fileio-fpc | ORACLE-STRONG reference half | fileio-fpc: exact stdout, NORTHSTAR bytes, two empty files |
| test-seed-fpc-diff | ORACLE-STRONG, EMULATOR | host reference half imported as fpc-shared (4) plus fileio-fpc (1); actual seed-vs-FPC differential deferred |
| test-seed-codegen | UNIT execution, EMULATOR | SMOKES.md: exact hand expectations for 13 bare programs; host source clause in repro |
| test-seed-codegen-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: multiplication |
| test-seed-bool-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: relation |
| test-seed-control-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: branch |
| test-seed-char-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: chr |
| test-seed-func-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: frame, var parameter |
| test-seed-array-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: stride, lower bound |
| test-seed-record-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: field offset, record stride |
| test-seed-string-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: temporary lifetime, length comparison |
| test-seed-fileio-os | UNIT execution, EMULATOR | SMOKES.md: actual DOS RTL/files |
| test-seed-fileio-os-mutant | UNIT sensitivity, EMULATOR | SMOKES.md: short write, wrong handle |
| test-seed-repro-mutant | META/property sensitivity + EMULATOR | two-build getpid fault plus live bool control; whole gate deferred |

19 distinct seed gates: 6 host-only (including two mutation gates), 13 needing
an emulator. test-seed-fpc-diff's duplicate conditional rule is one gate. Old
TEST_UNIT_GATES attribution is misleading: it contains emulator transitive work.
No seed-specific gate is dropped merely because of that legacy tier label.

| Comparable quantity | Legacy run | New tier 0 |
|---|---:|---:|
| C unit programs / assertions | 2 / 212 | 2 / 212 |
| repro fixtures / artifact pairs | 15 / 45 | 15 / 45 |
| file FPC reference cases | 1 | 1 |
| additional FPC roots (emulator gate's host portion) | 4 registered, not run in host target vector | 4 run |
| clean RTL static-contract gate | 1 | 1 |
| old host mutation invocations | 2 plus 2 fileio artifact builds | 0 (17 hooks retained) |
| default host runner entries | 4 normal legacy gate names | 6 |

No absolute assembly golden existed for compiling the resident compiler:
legacy checked successful assembly/linking, symbols, BSS and arena limits.
Those are compiler/tests/run.sh seed-build, not invented output goldens.
The shared test_assert.h had already been claimed to factory/testlib; the two
C test files moved into tests with git mv. All seed examples and RTL were already
in turbo/seed; none is compiled from attic. Report Generator fixtures/tools are
unrelated and remain untouched. Publication compiler replay/grammar/inventory
checks remain publication-owned, see compiler/tests/IMPORT.md.

Red control: SEED_MUT_PARSE_ADD_AS_MUL branch enabled temporarily in parser.c;
`make test TESTS=parser` exited 2: 115 checks, 9 failures including expression
precedence and loop lowering. Restored byte-for-byte before full green. Missing
FPC is checked with a PATH excluding fpc, including on a cached build; exit 2,
`FAIL: fpc is required (no skipped reference tests)`.

The bare runtime performs privileged COM1 out instructions and exits through
isa-debug-exit/hlt; the DOS runtime also needs INT 21h. There is no legacy Linux
execution path. A 32-bit Linux startup/output/syscall shim could run arithmetic
under IA32 compatibility, with a separate INT 21h translation for files. This
would test a different runtime and needs independent ABI/exception checks.
It is an option, not implemented or substituted for the guest oracle here.

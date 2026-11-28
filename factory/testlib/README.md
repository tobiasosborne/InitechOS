# Host test helpers

Factory-only code; never linked into the product. Public component-facing
assertions live in factory/include/test_assert.h; testlib/test_assert.h is a
compatibility forwarding header for existing clients. Both paths require a
positive reported check count. Behavioral requirements are
CLAUDE.md, The constitution and How the tree is organised, and the conventions
hardening task. test_assert.h is the canonical CHECK / TEST_HARNESS /
TEST_SUMMARY API; component copies should be deleted. TEST_SUMMARY retains the
human summary and ends with `INITECH_CHECKS N failures=N`. The runner requires
a positive count when compiler dependencies name this header, even if main
forgets TEST_SUMMARY. Inherited runners remain supported. Custom counts opt in
with `<test>_COUNT_PATTERN := ^name: ([0-9]+) checks.*$$` (one numeric capture).

Include config.mk first, after `TOP := $(abspath ../..)` and COMPONENT. Use
`$(abspath ..)` for a first-level component. config normalizes both TOP and
OUT; all generated files live beneath TOP/build. clean refuses empty, external,
and symlink-escaping OUT. Do not override OUT for source directories.

Fragments (one responsibility each): config.mk supplies tools/paths/safe clean;
command.mk compares the exact command per output inside make; target.mk builds freestanding
C/NASM objects and optional deterministic archives; host-test.mk builds and runs
host tests; import-check.mk checks import conditions; mutants.mk invokes the
strict patch runner. Use `$(call run-command,command text)` with FORCE on custom
output rules; no MAKEFILE_LIST dependency is needed. It prints/runs only when
inputs are newer or the command changed. Write generated headers to $@.tmp,
then rename. Don't put a comma-containing command directly in a make call;
put it in `command` and use `$(run-command)` instead.

host-test.mk keeps HOST_TESTS, EXTRA_HOST_TESTS, TESTS, HOST_INCLUDES, TEST_HDRS,
<test>_{SRCS,OBJS,CFLAGS,LIBS,ARGS,DEPS,ENV} and host_test_run compatible. SRCS
may contain existing .o files. Each C translation unit has its own .d and .cmd
under host/<test>.objs; TEST_HDRS now defaults empty. Delete blanket TEST_HDRS
lists once real dependencies suffice. DEPS is for generated inputs/wrappers.
`all` builds product outputs; `test-build` alone builds host programs. Existing
externally compiled OBJS still need their owner's command-aware rules.

TEST_TIMEOUT_S defaults to 20 seconds; `<test>_TIMEOUT_S` overrides it.
TEST_BUDGET_S defaults to 30 seconds. The make runner and SDK charge user and
system CPU time during test execution, excluding compilation. Their summaries
also report elapsed wall time. Exceeding the budget fails, with directions to
speed up or document a higher budget in the component README. GNU timeout
still uses wall time and kills the whole runner process group, including
inherited post-checks. Logs and scratch are private per component/test. One
make invocation owns a component OUT; concurrent independent invocations of
the same component should supply distinct OUT directories.

Import adoption: set TARGET_SRCS to component-relative C/ASM source paths,
TARGET_INCLUDES to product includes, TARGET_LIB to an archive path, then include
target.mk. For an executable, supply link rules and set TARGET_ARTIFACTS to
that executable instead. `include $(TOP)/mk/import-check.mk` after target.mk
adds all and the audit to test. The audit scans included makefiles and include
flags for attic, product sources (excluding tests/ and mutants/) for
IMPORT_MUT_PATTERN (default _(MUT|MUTATE|MUTANT)_), conditionals on macros no
checked-in build defines (factory/README.md, Fault branches) and escaping includes, and the actual make
graph for artifact reachability from all. It supplements, not replaces, the
host oracle. `make status` reruns the component test target and builds/audits every registered
component; logs explain failures. Undeclared artifacts are conservatively red.

expect_abort.h supplies success, diagnostic-marker and SIGABRT probes.
oracle_policy.h always fails missing references; no SKIP opt-outs exist.
run-mutants.sh runs one passing baseline per exact selection, zero-fuzz patches,
requires compilation, then the same failing test set twice. Success deletes
scratch; failure preserves it and prints its location. Empty selections fail.
The seeded region generators/shrinker remain private to the region engine.

The outer sweep uses per-invocation worker records and verifies every process
exit and count summary against the owned patch inventory. Discovery, missing
records/logs, malformed or partial counts, survivors and errors are all red.
The sweep prepares the pristine product image once. Each worker copies the
source snapshot and the prepared build inputs, then owns its `TOP/build`.
Within a component, each host runner invocation also has a private
dependency tree seeded from the pristine inputs. Scratch `OUT` and the
selected tests apply to the graded invocation. Recursive dependencies keep
their own output paths in that tree (initech-r6xvy, initech-ruu4t).
mutants/extra.list declares root-relative patches reused by an integration
grader. Pending-only targets are invoked, not silently replaced with success;
unimplemented targets fail. Pending patches remain explicitly unproved, and
certificates fail incomplete pending coverage at either CERT_STRICT setting. The testlib fixtures include killed-worker and false-summary
controls against the actual sweep.

Adoption for in-flight owners (no edits to their branches here):

- Initech 123: delete tests/test_assert.h; replace HOST_INCLUDES with
  `HOST_INCLUDES := $(LOCAL_INC) -I$(TOP)/factory/testlib -Itests`.
  Delete TEST_HDRS assignments, add `TARGET_SRCS := $(CORE) app/face_a.c`,
  `TARGET_INCLUDES := $(LOCAL_INC)`, and
  `$(OUT)/app/face_a.o: TARGET_INCLUDES += $(FACE_INC)`. Add
  `TARGET_EXTRA_CFLAGS := -DI123_MENU_DISK` and include target.mk; delete
  private TARGET_OBJS/all/C pattern/dependency rules. Keep SDK tenant
  linking from that owner's mk/tenant.mk and set
  `TARGET_ARTIFACTS := $(OUT)/123.EXE` once that is an all prerequisite.
- InitechWord: delete TEST_HDRS assignments; add
  `TARGET_SRCS := $(CORE) app/face_a.c`, `TARGET_INCLUDES := $(LOCAL_INC)`,
  `$(OUT)/app/face_a.o: TARGET_INCLUDES += $(FACE_INC)` and include target.mk;
  delete private TARGET_OBJS/all/C pattern/dependency rules. Retain tenant
  linking and set `TARGET_ARTIFACTS := $(OUT)/WORD.EXE` when wired.
- Both apps: change each external host-core/x87 recipe to
  `$(call run-command,<the existing compiler command>)`, replace its
  MAKEFILE_LIST/blanket header prerequisites with FORCE, retain -MMD/-MP
  and included .d rules. For the generated event header replace `> $@`
  with `> $@.tmp && mv $@.tmp $@`. Keep existing runner/post-check scripts.
  Add `.PHONY: pinned-corpus`, `test-build: pinned-corpus` and
  `pinned-corpus: ; @$(MAKE) -C $(TOP) corpus` for standalone pin verification.
  No budget increase is needed: both pass the default 30s. An explicit
  declaration is `TEST_BUDGET_S := 30 # full independent corpus suite`;
  document that reason in each README if adopting the explicit declaration.
- InitechBase, Turbo Initech, SDK and FLAIR: their Makefiles are not yet
  present on this base, so source lists/output names cannot be guessed.
  Set TARGET_SRCS to the owner's real C/ASM list, TARGET_INCLUDES to public
  product paths, then `include $(TOP)/mk/target.mk`. For pure libraries use
  TARGET_LIB; for an EXE or Pascal-built TURBO.COM retain the real linker/
  compiler recipe and set TARGET_ARTIFACTS to its actual all output.
  target.mk compiles C/ASM only, never Pascal. Use
  `HOST_INCLUDES += -I$(TOP)/factory/testlib`, delete copied test_assert.h
  files and call TEST_SUMMARY. Keep the default budget until measured.
  Add consumed corpus files/HEAD records and the pinned-corpus lines above;
  components with no external oracle need no external corpus records.
- All six owners: after removing product mutation hooks, attic dependencies
  and escaping includes, add exactly
  `include $(TOP)/mk/import-check.mk` after the target/link declarations.
  Existing contracts remain compatible until adoption. Custom count formats
  use `<test>_COUNT_PATTERN := ^name: ([0-9]+) checks.*$$`; helpers with no
  assertions stay EXTRA_HOST_TESTS rather than pretending to be test suites.

run-entries.sh retains the shell/Pascal entry interface and per-entry logs.

Current adoption uses TARGET_OUT for components retaining target/ or tenant/
object layouts. Keep a nonempty no-op recipe on unchanged stamps: GNU make
must stat the real output instead of treating an empty expansion as updated.
Do not add a separate mkdir recipe before run-command. corpus.mk verifies a
component's CORPORA selection unless the top-level preflight already ran.
run-entries.sh also bounds inherited shell/Pascal suites (20s per entry, 30s
per component by default); the entry interface remains unchanged.

The audit recognizes InitechBase's inherited MUTATE/MUTANT spellings as well
as MUT hooks. None is converted by this conventions pass.

The import audit strips C, NASM and Pascal comments/literals before looking for
live mutation identifiers. It scans sources reachable from all and exported
include/ headers; IMPORT_PRODUCT_SRCS adds recursive product sources whose
child make graphs are not visible in the parent. Parked source files are not
product artifacts. Escaping includes fail except for the exact resident command
entry includes in kernel/main.c and kernel/startup.c, authorized by CLAUDE.md
"Two interfaces" until separately loaded COMMAND.COM (initech-1frx2, Release 2).
`make -C factory test` runs tiny clean/red fixtures for comments, test-only and
parked files, live C/Pascal hooks, escaping/attic includes, unreachable artifacts
and missing build dependencies. mk/dependencies.mk owns the graph used by both
top-level ordering and the generated-path dependency audit.

The delimited `interface-declarations` section in each component README is the
interface inventory. `Interface role:` is checked against actual ownership.
`use-pure`, `use-binding`, `use-records` and `use-source` name a provider's exact
published contract; the provider must declare the matching `export-*` line.
Pure/binding exports list exact external symbols. Records grant headers only;
source exports permit a pure source to be compiled locally without treating all
its definitions as the consumer's own. `use-gate: sdk/include/tbx.h` names the
published Toolbox transport. Resident bindings cannot be granted to disk apps
by their own declarations. Existing resident serial/IRQ symbols are published
against their implementing source, without authorizing private header includes.

The audit selects the complete parent GNU make database and follows declared
target artifacts, then checks real target dependency files, external symbols,
source references to resident services and interrupt instructions. Flat NASM
inputs are preprocessed before vector checks. Shared disk helper code must
belong exclusively to authorized consuming executables. Turbo's packaged ELF
is the vector evidence for its recipe-built product; Multiboot judging outputs
are separate from the shipped DOS runtime. Component-local transcripts are
retained under build/<component>/interface-audit/.

The two law-authorized temporary exceptions have separate issue/removal records.
The four migration crossings have exact named, issue-linked declarations;
unknown or stale migration records fail, and no adjacent vector site is waived.
`factory/tests/interfaces.sh` compiles independent positive and negative callers,
including forbidden direct-call/vector patches in factory/tests/fixtures/interfaces/.

## Kill contract

This is the diagnostic the shared runner (run-mutants.sh, run-test.sh,
host-results.sh, host-verdict.awk) requires. A mutant is killed only when all
of these hold, in both of two test runs, with the same receipt:

1. The component's `make test-build` succeeded for the patched copy (a build
   error is `BUILD FAILED`, an error).
2. `make test-run` printed `INITECH_TEST_RUN n` and left a fresh `.result`
   for each of the n tests; at least one is `SEMANTIC`, the rest `PASS`.
3. A `SEMANTIC` test exited exactly 1 within its timeout (124, signals,
   2 and every other status are errors) and its own log holds at least one
   `FAIL` diagnostic line and a completed summary that counts checks and at
   least one failure: `INITECH_CHECKS n failures=k` (TEST_SUMMARY in
   factory/include/test_assert.h), or the retained `n checks, k failures`.
4. No build tool and no emulator run failed during the mutant's test run in a
   way the baseline did not. The runner puts recording shims for cc, gcc,
   clang, nasm, as, ld (and the ld.bfd/ld.gold/ld.lld variants FPC selects
   beside it), ar, objcopy and fpc first on PATH; a shim that sees a
   nonzero exit appends `<tool> exit <rc> in <cwd>: <args>` to
   `$INITECH_INFRA_LOG` (infra-record.sh). The run's own copy directory and
   mktemp names are normalised, and every existing input file argument
   carries a hash of its contents, so only the same command on the same
   inputs failing in the baseline excuses it: a baseline's expected refusal of one input never
   cancels a mutant's new failure of another (review F1). The QEMU
   and Bochs harnesses append there when they exit 2 (harness failure) or 3
   (the guest run never completed; factory/README.md, Emulator exit status).
   Any new line is `INFRASTRUCTURE FAILURE (not a kill)`, an error, however
   the test's own wrapper reported it. Tools called by absolute path bypass
   the shims; call them by name.

The tier-0 half is checked on every `make test`: a test that exits 0 must
print a completed summary with a positive check count and no failures
(summary-contract.awk). A passing test with no summary fails
`FAIL contract: ... printed no completed check summary`, and one whose summary
counts failures fails `FAIL count`. So a test either satisfies the contract
or fails loudly, before a mutation sweep finds out. Tests with a custom
`<test>_COUNT_PATTERN` are graded by that pattern instead.
summary-contract.known lists, temporarily and issue-linked, the tests that
did not print a summary when the rule was introduced (2026-11-15). The list
excuses a missing summary only: a listed test whose summary counts a failure
fails `FAIL count` like any other (initech-1lmgb). A listed test with no
summary passes with a NOTE and can never kill a mutant until its owner fixes it
and deletes the row. A row whose test now prints a summary prints a NOTE
asking for its deletion. The file is deleted when it is empty.

A wrapper that turns any exit 1 into `FAIL <name>` and a summary satisfies
items 2 and 3 without proving anything; item 4 is what stops it counting a
compiler, assembler, linker, FPC reference or emulator failure as a kill.
testlib/tests/infra-mutants.sh is the receipt: on such a wrapper a
comment-only mutant with a failing compiler, a comment-only mutant whose
emulator run never completed, and a build-breaking mutant compiled at test
time are errors; the unchanged comment-only mutant survives; a semantic
mutant is still killed twice. Before item 4 the first three were kills.

Shared host mutation kills require fresh per-test receipts, successful completion
of the whole selected runner, exit 1 from the test, a completed
INITECH_CHECKS or retained checks/failures summary with positive failures, and assertion diagnostics.
The same diagnostic hashes must recur twice. Timeouts, signals, startup/build
failures, absent receipts and unknown legacy failure formats count as errors.
Legacy assertion drivers must supply a completed assertion summary before
those faults qualify; a generic failing test name is never proof of a kill.

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
TEST_BUDGET_S defaults to 30 seconds, measured over test execution, excluding
compilation. Exceeding it fails, with directions to speed up or document a
higher budget in the component README. Integer wall seconds have one-second
resolution. GNU timeout kills the whole runner process group, including
inherited post-checks. Logs and scratch are private per component/test. One
make invocation owns a component OUT; concurrent independent invocations of
the same component should supply distinct OUT directories.

Import adoption: set TARGET_SRCS to component-relative C/ASM source paths,
TARGET_INCLUDES to product includes, TARGET_LIB to an archive path, then include
target.mk. For an executable, supply link rules and set TARGET_ARTIFACTS to
that executable instead. `include $(TOP)/mk/import-check.mk` after target.mk
adds all and the audit to test. The audit scans included makefiles and include
flags for attic, product sources (excluding tests/ and mutants/) for
IMPORT_MUT_PATTERN (default _(MUT|MUTATE|MUTANT)_) and escaping includes, and the actual make
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
mutants/extra.list declares root-relative patches reused by an integration
grader. Pending-only targets are invoked, not silently replaced with success;
unimplemented targets fail. Pending patches remain explicitly unproved, and
certificate summaries mark successful active checks with pending coverage as
PASS_PENDING. The testlib fixtures include killed-worker and false-summary
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
product artifacts. Escaping includes still fail, including the documented
resident-shell include in kernel: documentation does not waive CLAUDE.md.
`make -C factory test` runs tiny clean/red fixtures for comments, test-only and
parked files, live C/Pascal hooks, escaping/attic includes, unreachable artifacts
and missing build dependencies. mk/dependencies.mk owns the graph used by both
top-level ordering and the generated-path dependency audit.

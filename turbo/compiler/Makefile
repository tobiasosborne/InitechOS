TOP := $(abspath ../..)
COMPONENT := turbo/compiler
include $(TOP)/mk/config.mk
FPC ?= fpc
OBJCOPY ?= objcopy
SEED_OUT := $(TOP)/build/turbo/seed
SEED_BIN := $(SEED_OUT)/initechc
SEED_RT := $(SEED_OUT)/rt
RT_SRC := $(TOP)/turbo/seed/rt
PROTOCOLS := $(patsubst fixtures/%.golden,%,$(wildcard fixtures/lex*.golden fixtures/parse*.golden fixtures/type*.golden))
CORPUS := $(patsubst fixtures/%.pas,%,$(wildcard fixtures/gen*.pas))
HOST_TESTS := $(PROTOCOLS) self-protocols $(CORPUS) self-generation seed-build isolated-seed runtime-table execution-frame execution-var execution-flat
# Generated execution is tier 1/2 only; the default tier 0 stays emulator-free.
TESTS ?= $(filter-out execution-frame execution-var execution-flat,$(HOST_TESTS))
ifneq ($(filter-out $(HOST_TESTS),$(TESTS)),)
$(error unknown TESTS: $(filter-out $(HOST_TESTS),$(TESTS)))
endif
.PHONY: tools seed-tools test-build test-run
# Always preflight: cached binaries must not hide a missing independent compiler.
tools:
	@command -v $(FPC) >/dev/null 2>&1 || { echo 'FAIL: fpc is required (no skipped reference tests)' >&2; exit 1; }
seed-tools:
	+@$(MAKE) --no-print-directory -C $(TOP)/turbo/seed OUT=$(SEED_OUT) MAKEOVERRIDES= all TESTS=
$(OUT)/turbo_fpc: turbo.pas $(MAKEFILE_LIST) | $(OUT) tools
	@$(FPC) -B -O2 -v0 -FU$(OUT) -FE$(OUT) -o$@ $< > $(OUT)/fpc-build.log 2>&1 || { cat $(OUT)/fpc-build.log; exit 1; }
$(OUT)/turbo_seed.s: turbo.pas $(wildcard $(TOP)/turbo/seed/*.[ch]) $(MAKEFILE_LIST) | $(OUT) seed-tools
	@$(SEED_BIN) --emit-asm -o $@ $<
$(OUT)/turbo_seed.o: $(OUT)/turbo_seed.s
	@cd $(OUT) && $(NASM) -f elf32 turbo_seed.s -o turbo_seed.o
$(OUT)/turbo_seed_dos.elf: $(OUT)/turbo_seed.o $(wildcard $(RT_SRC)/*) | seed-tools
	@$(TARGET_LD) $(TARGET_LDFLAGS) -T $(RT_SRC)/seed_dos.ld -o $@ $(SEED_RT)/start_dos.o $(SEED_RT)/fileio.o $<
$(OUT)/turbo_seed_bare.elf: $(OUT)/turbo_seed.o $(wildcard $(RT_SRC)/*) | seed-tools
	@$(TARGET_LD) $(TARGET_LDFLAGS) -T $(RT_SRC)/seed.ld -o $@ $(SEED_RT)/start.o $(SEED_RT)/fileio.o $<
$(OUT)/TURBO.COM: $(OUT)/turbo_seed.s $(wildcard $(RT_SRC)/*) tests/package.sh | seed-tools
	+@$(MAKE) --no-print-directory -C $(TOP)/factory/mz all
	@sh tests/package.sh $(OUT)/turbo_seed.s $(OUT)/extended
	@cp $(OUT)/extended/program.exe $@
all: tools $(OUT)/turbo_fpc $(OUT)/TURBO.COM $(OUT)/turbo_seed_bare.elf
test-build: all
test: test-run
test-run: test-build
	@shared=$$(mktemp -d "$(OUT)/self-runs.XXXXXX"); \
	  trap 'rm -rf "$$shared"' 0; \
	  TURBO_SELF_RUNS=$$shared sh $(TOP)/factory/testlib/run-entries.sh $(COMPONENT) "$(OUT)" tests/run.sh $(TESTS)
export TOP FPC NASM TARGET_LD OBJCOPY SEED_OUT RT_SRC
include $(TOP)/mk/mutants.mk
smoke:
	@echo 'Tier 1 requires the product image; see tests/SMOKES.md' >&2; exit 1

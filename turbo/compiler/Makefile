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
HOST_TESTS := $(PROTOCOLS) self-protocols $(CORPUS) self-generation seed-build
TESTS ?= $(HOST_TESTS)
ifneq ($(filter-out $(HOST_TESTS),$(TESTS)),)
$(error unknown TESTS: $(filter-out $(HOST_TESTS),$(TESTS)))
endif
.PHONY: tools seed-tools test-build test-run
# Always preflight: cached binaries must not hide a missing independent compiler.
tools:
	@command -v $(FPC) >/dev/null 2>&1 || { echo 'FAIL: fpc is required (no skipped reference tests)' >&2; exit 1; }
seed-tools:
	+@$(MAKE) --no-print-directory -C $(TOP)/turbo/seed all TESTS=
$(OUT)/tps_fpc: tps.pas $(MAKEFILE_LIST) | $(OUT) tools
	@$(FPC) -B -O- -v0 -FU$(OUT) -FE$(OUT) -o$@ $< > $(OUT)/fpc-build.log 2>&1 || { cat $(OUT)/fpc-build.log; exit 1; }
$(OUT)/tps_seed.s: tps.pas $(wildcard $(TOP)/turbo/seed/*.[ch]) $(MAKEFILE_LIST) | $(OUT) seed-tools
	@$(SEED_BIN) --emit-asm -o $@ $<
$(OUT)/tps_seed.o: $(OUT)/tps_seed.s
	@cd $(OUT) && $(NASM) -f elf32 tps_seed.s -o tps_seed.o
$(OUT)/tps_seed_dos.elf: $(OUT)/tps_seed.o $(wildcard $(RT_SRC)/*) | seed-tools
	@$(TARGET_LD) $(TARGET_LDFLAGS) -T $(RT_SRC)/seed_dos.ld -o $@ $(SEED_RT)/start_dos.o $(SEED_RT)/fileio.o $<
$(OUT)/tps_seed_bare.elf: $(OUT)/tps_seed.o $(wildcard $(RT_SRC)/*) | seed-tools
	@$(TARGET_LD) $(TARGET_LDFLAGS) -T $(RT_SRC)/seed.ld -o $@ $(SEED_RT)/start.o $(SEED_RT)/fileio.o $<
$(OUT)/TPS.COM: $(OUT)/tps_seed_dos.elf
	@$(OBJCOPY) -O binary $< $@
all: tools $(OUT)/tps_fpc $(OUT)/TPS.COM $(OUT)/tps_seed_bare.elf
test-build: all
test: test-run
test-run: test-build
	@sh $(TOP)/factory/testlib/run-entries.sh $(COMPONENT) "$(OUT)" tests/run.sh $(TESTS)
export FPC NASM TARGET_LD OBJCOPY SEED_OUT RT_SRC
include $(TOP)/mk/mutants.mk
smoke:
	@echo 'Tier 1 requires the product image; see tests/SMOKES.md' >&2; exit 1

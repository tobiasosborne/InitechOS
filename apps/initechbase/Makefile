TOP := $(abspath ../..)
COMPONENT := apps/initechbase
include $(TOP)/mk/config.mk

# Public DOS loader/memory contracts; only the PAL and packaging use these.
LOADER_INC := -I$(TOP)/kernel/include
LOADER_HDRS := $(TOP)/kernel/include/memory_map.h $(TOP)/kernel/include/initechmz_extended.h
MZ_OUT := $(TOP)/build/factory/mz
LOCAL_INC := -Iinclude
HOST_INCLUDES := $(LOCAL_INC) -Itests -I$(TOP)/factory/testlib
include tests/sources.mk
# w2_host.c includes the REPL source instead of linking it separately.
test_w2_host_DEPS := ibase_main.c
test_ibase_w2_DEPS := ibase_main.c
STANDALONE := $(shell sed 's/ .*//' tests/standalone.cases)
HOST_TESTS := $(STANDALONE) test_ibase_dml test_ibase_fns test_ibase_w2 \
 test_ibase_ovl test_ibase_dbsafe test_ibase_datatype test_interp_do_file ibase_numeric_format \
 readers captured-hashes w4-files retained-screens mzlink-io messages-generated conformance_host
EXTRA_HOST_TESTS := ibase_core_ref
ifeq ($(THOROUGH),1)
TESTS := $(HOST_TESTS) core-live
endif
SHELL_SUITES := readers captured-hashes w4-files retained-screens mzlink-io messages-generated core-live
# The fragment registers C entries. Shell entries are added to its run vector
# after inclusion so no empty compiler command is generated.
RUN_TESTS := $(if $(TESTS),$(TESTS),$(HOST_TESTS))
HOST_TESTS := $(filter-out $(SHELL_SUITES),$(HOST_TESTS))
TESTS := $(filter-out $(SHELL_SUITES),$(RUN_TESTS))
include $(TOP)/mk/host-test.mk
HOST_TESTS += $(filter-out core-live,$(SHELL_SUITES))
TESTS := $(RUN_TESTS)
HOST_TEST_BINS := $(addprefix $(HOST_OUT)/,$(TESTS))
test-build: $(HOST_TEST_BINS)
MUTANT_PATCHES ?= $(wildcard mutants/*.patch)
TARGET_MUTANT_PATCHES ?= $(wildcard mutants/target/*.patch)
MUTANT_JOBS ?= 4
ifeq ($(MUTANT_SERIAL),1)
include $(TOP)/mk/mutants.mk
else
.PHONY: mutants
mutants:
	+@sh tools/run-mutants.sh "$(TOP)" "$(OUT)" "$(MAKE)" "$(MUTANT_JOBS)" $(MUTANT_PATCHES)
endif

# Native/static proofs are tier 2; they never run in the host-only test gate.
.PHONY: target-mutants target-mutant-controls
ifneq ($(strip $(TARGET_MUTANT_PATCHES)),)
mutants: target-mutants
endif
target-mutants: target-mutant-controls
	+@sh tools/run-target-mutants.sh "$(TOP)" "$(OUT)/target-mutants" "$(MAKE)" $(TARGET_MUTANT_PATCHES)
target-mutant-controls:
	+@sh tests/target-mutant-controls.sh "$(TOP)" "$(OUT)/target-mutant-controls" "$(MAKE)"

.PHONY: native-tools native-prepare native-check
native-tools: $(HOST_OUT)/screen_capture $(HOST_OUT)/test_console_review $(HOST_OUT)/screen_tool
native-prepare: package native-tools
	+$(MAKE) -C $(TOP) image
target-mutant-controls: native-prepare
NATIVE_MODES ?= forms enter keys color color-state controls continuation
native-check: native-prepare
	@set -e; for mode in $(NATIVE_MODES); do \
	 sh tests/native/check.sh "$$mode" "$(TOP)" "$(OUT)/INITBASE.EXE" "$(OUT)/INITBASE.OVL" "$(OUT)/native/$$mode"; \
	done

# Host-only sandbox compiles are identical except for debug paths. Cache them
# when available; omit debug information there so scratch paths do not defeat
# the cache. Product flags and the ordinary host oracle are unchanged.
ifneq ($(shell command -v ccache 2>/dev/null),)
mutants: export HOSTCC := $(if $(findstring ccache,$(HOSTCC)),$(HOSTCC),ccache $(HOSTCC))
mutants: export HOST_CFLAGS := $(filter-out -g,$(HOST_CFLAGS))
mutants: export CCACHE_DIR := $(OUT)/mutants/cache
endif

# Shell suites use the shared reporting protocol and retain independent postchecks.
define host_test_run
n=$$((n+1)); \
if INITECH_FACTORY_ROOT="$(TOP)" DBASE3_DECOMP="$(DBASE3_DECOMP)" sh tests/run.sh "$(1)" "$(abspath $(HOST_OUT))" > $(HOST_OUT)/$(1).log 2>&1; then \
 echo "  ok    $(1)"; \
else \
 fail=$$?; \
 echo "  FAIL  $(1) (log: $(HOST_OUT)/$(1).log)"; \
 tail -n 18 $(HOST_OUT)/$(1).log; \
fi;
endef

# Entry points are POSIX shell; these marker rules override the fragment's
# compile rule for entries without a C main. The same entries have real deps.
SHELL_SUITES := readers captured-hashes w4-files retained-screens mzlink-io messages-generated core-live
$(addprefix $(HOST_OUT)/,$(SHELL_SUITES)):
	@mkdir -p $(@D); touch $@
$(HOST_OUT)/messages-generated: $(OUT)/tools/ibase_msg_pack
$(HOST_OUT)/w4-files: $(addprefix $(HOST_OUT)/,$(addsuffix _diff,frm lbl qry scr fmt config prg vue exact)) $(HOST_OUT)/w4_mutate
$(HOST_OUT)/core-live: $(HOST_OUT)/ibase_core_ref $(HOST_OUT)/test_ibase_core
$(HOST_OUT)/w4_mutate: tests/w4_mutate.c FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -MMD -MP -MF $@.d -o $@ $<)
$(HOST_OUT)/%_diff: tests/w4_file_diff.c FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -DKIND='"$*"' -MMD -MP -MF $@.d -o $@ $<)
test-build: $(HOST_OUT)/mint_clients_dbf $(HOST_OUT)/w2_fixture
$(HOST_OUT)/mint_clients_dbf: $(mint_clients_dbf_SRCS) $(mint_clients_dbf_OBJS) FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -MMD -MP -MF $@.d -o $@ $(filter %.c %.o,$^))
$(HOST_OUT)/w2_fixture: $(w2_fixture_SRCS) $(w2_fixture_OBJS) FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -MMD -MP -MF $@.d -o $@ $(filter %.c %.o,$^))

$(OUT)/host-core/%.o: %.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -MMD -MP -c $< -o $@)
$(OUT)/host-core/dml/%.o: %.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -DFLOW_MEMVAR_ARENA=6000 -MMD -MP -c $< -o $@)
$(OUT)/host-core/lifetime/%.o: %.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) -DIBASE_OVERLAYS -ffunction-sections -fdata-sections -MMD -MP -c $< -o $@)
-include $(wildcard $(OUT)/host-core/*/*.d $(OUT)/host-core/*/*/*.d)

.PHONY: corpus
corpus:
	@test -d "$(DBASE3_DECOMP)/goldens/dbase-iii-plus-1.1-pristine/files" && test -d "$(DBASE3_DECOMP)/mint/work" || { echo 'FAIL: dBASE corpus missing; set DBASE3_DECOMP to dbase3-decomp' >&2; exit 1; }
	@command -v python3 >/dev/null || { echo 'FAIL: python3 required for independent dBASE readers' >&2; exit 1; }
test-run: corpus

TARGET_SRCS := $(wildcard core/*.c cmd/*.c fs/*.c) ibase_main.c pal/pal_dos.c boot/softfp.c
TARGET_OUT := $(OUT)/target
TARGET_INCLUDES := $(LOCAL_INC)
TARGET_EXTRA_CFLAGS = $(TARGET_PROFILE) $(TARGET_INC)
include $(TOP)/mk/target.mk
TARGET_ARTIFACTS := $(OUT)/INITBASE.EXE $(OUT)/INITBASE.OVL
# Soft-float is a semantic ABI contract, not a size optimisation. Keep all
# linked TUs coherent. FLOW settings preserve the shipped runtime profile.
TARGET_PROFILE := -msoft-float -mno-80387 -ffunction-sections -fdata-sections -DIBASE_OVERLAYS -DFLOW_MAX_REGISTRY=1 -DFLOW_MEMVAR_ARENA=6000
all: $(TARGET_OBJS) $(OUT)/target/crt0.o $(OUT)/target/ibase.ld $(OUT)/target/ibase-mz.args $(OUT)/tools/ibase_ovl_pack $(TARGET_ARTIFACTS) | mz-tools
.PHONY: package
package: $(OUT)/INITBASE.EXE $(OUT)/INITBASE.OVL
$(OUT)/target/pal/pal_dos.o: private TARGET_INC := $(LOADER_INC)
$(OUT)/target/pal/pal_dos.o: $(LOADER_HDRS)
$(OUT)/target/crt0.o: boot/ibase_crt0.asm FORCE
	$(call run-command,$(NASM) -f elf32 $< -o $@)
$(OUT)/tools/%: tools/%.c $(LOADER_HDRS) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $(LOCAL_INC) $(LOADER_INC) -MMD -MP -MF $@.d -o $@ $<)
.PHONY: mz-tools
mz-tools:
	+$(MAKE) -C $(TOP)/factory/mz all
$(OUT)/target/ibase.ld: boot/ibase.ld $(OUT)/tools/ibase_memory_ld
	@mkdir -p $(@D)
	@$(OUT)/tools/ibase_memory_ld > $@
	@cat $< >> $@
$(OUT)/IBASE.elf: $(TARGET_OBJS) $(OUT)/target/crt0.o $(OUT)/target/ibase.ld
	@if ! $(TARGET_LD) $(TARGET_LDFLAGS) --gc-sections -q -Map=$(OUT)/target/link.map --no-warn-rwx-segments -T $(OUT)/target/ibase.ld -o $@ $(OUT)/target/crt0.o $(TARGET_OBJS); then \
	 required=$$(awk '$$2=="IBASE_REQUIRED_RESERVE" {print $$1; exit}' $(OUT)/target/link.map); \
	 limit=$$(awk '$$2=="IBASE_DECLARED_RESERVE" {print $$1; exit}' $(OUT)/target/link.map); \
	 if test -n "$$required" && test -n "$$limit" && test $$((required)) -gt $$((limit)); then \
	  printf 'INITBASE.EXE packaging blocked: slot + BSS + PAL tail require %d bytes; IBASE_LOADER_RESERVE is %d bytes (%d over). See README.md, Loader and overlay contract, and target/link.map.\n' $$((required)) $$((limit)) $$((required-limit)) >&2; \
	 fi; exit 1; fi
$(OUT)/target/ibase-mz.args: $(OUT)/tools/ibase_memory_ld
	@mkdir -p $(@D)
	@$< --mz-args > $@
$(OUT)/INITBASE.EXE $(OUT)/INITBASE.OVL &: $(OUT)/IBASE.elf $(OUT)/tools/ibase_ovl_pack $(OUT)/target/ibase-mz.args $(wildcard $(TOP)/factory/mz/*.c) | mz-tools
	$(OUT)/tools/ibase_ovl_pack $< --res-bin $(OUT)/resident.bin --res-rel $(OUT)/resident.rel --ovl $(OUT)/INITBASE.OVL --overlay .ovl_file --overlay .ovl_report --overlay .ovl_status --overlay .ovl_set --overlay .ovl_memfile --overlay .ovl_forms --forbid ibase_ovl_call --forbid ibase_do --forbid ibase_repl --forbid run_block --forbid exec_command --forbid fl_trap --forbid fl_escape
	$(MZ_OUT)/mzlink $(OUT)/resident.bin $(OUT)/INITBASE.EXE --reloc-file $(OUT)/resident.rel --header-relocs $$(cat $(OUT)/target/ibase-mz.args)
smoke: package $(HOST_OUT)/prog_diff $(HOST_OUT)/mint_clients_dbf $(OUT)/smoke/fixtures
	+$(MAKE) -C $(TOP) image
	sh tests/smoke/run.sh
$(OUT)/smoke/fixtures: tests/smoke/fixtures.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)

$(HOST_OUT)/retained-screens: $(HOST_OUT)/screen_tool $(HOST_OUT)/w4_screen_check
$(HOST_OUT)/screen_tool: tests/screen_tool.c tests/screen_file.h FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -MMD -MP -MF $@.d -o $@ $<)
$(HOST_OUT)/screen_capture $(HOST_OUT)/test_console_review: $(HOST_OUT)/%: tests/%.c tests/screen_file.h FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Itests -MMD -MP -MF $@.d -o $@ $<)
$(HOST_OUT)/w4_screen_check: tests/w4_screen_check.c tests/screen_file.h FORCE | $(HOST_OUT)
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -MMD -MP -MF $@.d -o $@ $<)

test_ibase_ctl_LIBS := -pthread
.PHONY: pending
pending: test-build corpus
	@DBASE3_DECOMP="$(DBASE3_DECOMP)" CASE="$(CASE)" sh tests/run.sh pending "$(abspath $(HOST_OUT))"

# Full B5 validation is also available against an explicitly supplied pair.
.PHONY: pair-test
pair-test: $(HOST_OUT)/test_ibase_ovl
	@test -n "$(EXE)" || { echo 'set EXE to an executable with its matching .OVL' >&2; exit 1; }
	@$(HOST_OUT)/test_ibase_ovl "$(EXE)"

TEST_BUDGET_S := 30
CORPORA := DBASE3_DECOMP
include $(TOP)/mk/corpus.mk
-include $(wildcard $(OUT)/tools/*.d $(OUT)/host/*.d)

.PHONY: package-check
package-check: package $(HOST_OUT)/test_ibase_ovl
	$(MZ_OUT)/mzcheck $(OUT)/INITBASE.EXE $(OUT)/resident.bin $$(printf "%d" $$(awk '$$2=="IBASE_DECLARED_RESERVE" {print $$1; exit}' $(OUT)/target/link.map))
	$(HOST_OUT)/test_ibase_ovl $(OUT)/INITBASE.EXE
test: package-check
smoke: pinned-corpus

include $(TOP)/mk/import-check.mk

smoke: $(HOST_OUT)/test_ibase_core $(HOST_OUT)/test_ibase_dbsafe

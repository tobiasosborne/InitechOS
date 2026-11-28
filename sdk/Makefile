TOP := $(abspath ..)
COMPONENT := sdk
include $(TOP)/mk/config.mk
HOST_AR ?= ar
HOST_LIB := $(OUT)/host/libtbx_host.a
# layout and link are compile and link checks in test-build; nothing runs.
HOST_TESTS := abi host host32 mz search33 conformance dispatch sweep
TESTS ?= $(HOST_TESTS)
export TOP
TEST_BUDGET_S := 30
TARGET_SRCS := rt/tbx_rt.c
TARGET_INCLUDES := -Iinclude
TARGET_ARTIFACTS := $(OUT)/rt/tbx_rt.o
include $(TOP)/mk/target.mk
.PHONY: host abi layout link-check test-build test-run
all: host
host: $(HOST_LIB)
$(OUT)/host/tbx_host.o: host/tbx_host.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP -c $< -o $@)
$(HOST_LIB): $(OUT)/host/tbx_host.o FORCE
	$(call run-command,$(HOST_AR) rcsD $@ $<)
$(OUT)/tests/audit: tests/audit.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -MMD -MP $< -o $@)
abi: $(OUT)/tests/audit
	$< tests/traps.tsv include/toolbox_gate.h include/cp_dialog.h rt/tbx_rt.c $(TOP)/flair/tbxgate.c
$(OUT)/tests/layout.o: tests/layout.c FORCE
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -MMD -MP -c $< -o $@)
layout: $(OUT)/tests/layout.o
	@echo 'ABI: target i386 layouts pinned (compile-only)'
$(OUT)/tests/link.c: tests/traps.tsv tests/generate-link.sh include/tbx.h FORCE
	$(call run-command,sh tests/generate-link.sh $< include/tbx.h > $@.tmp && mv $@.tmp $@)
$(OUT)/tests/link: $(OUT)/tests/link.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -MMD -MP $< $(HOST_LIB) -o $@)
link-check: $(OUT)/tests/link
	$<
	@echo 'ABI: all bindings link against shared fake'
$(OUT)/tests/host: tests/host.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
# The published GM4 dead-command sweep, graded on a toy tenant live and dead.
$(OUT)/tests/sweep-live: tests/sweep.c tests/sweep/toy.c host/command_sweep.h $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -Itests -DSWEEP_TOY_DEAD=0 -DSWEEP_README='"$(CURDIR)/tests/sweep/INVENTORY.md"' -MMD -MP $< $(HOST_LIB) -o $@)
$(OUT)/tests/sweep-dead: tests/sweep.c tests/sweep/toy.c host/command_sweep.h $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -Itests -DSWEEP_TOY_DEAD=1 -DSWEEP_README='"$(CURDIR)/tests/sweep/INVENTORY.md"' -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/sweep-live $(OUT)/tests/sweep-dead
$(OUT)/tests/search33: tests/search33.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/search33
$(OUT)/tests/self-contained.o: tests/self-contained.c FORCE
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -MMD -MP -c $< -o $@)
test-build: $(OUT)/tests/audit $(OUT)/tests/layout.o $(OUT)/tests/link $(OUT)/tests/host $(OUT)/tests/self-contained.o $(OUT)/rt/tbx_rt.o
test: test-run
test-run: test-build
	@test -n '$(strip $(TESTS))' || { echo 'FAIL sdk: empty test selection'; exit 1; }
	@mkdir -p $(OUT)/host; start=$$(date +%s); fail=0; \
	for entry in $(TESTS); do \
	  sh $(TOP)/factory/testlib/run-test.sh "$$entry" "$${TEST_TIMEOUT_S:-20}" '' "$(OUT)/host/$$entry" \
	    sh -c 'sh tests/run.sh "$$1" "$$2" > "$$3.log" 2>&1' sh "$(OUT)" "$$entry" "$(OUT)/host/$$entry" || fail=$$((fail+1)); \
	done; \
	times > $(OUT)/host/suite.time || { echo 'FAIL budget: cannot record suite CPU time'; exit 1; }; \
	elapsed=$$(($$(date +%s)-start)); \
	echo 'INITECH_TEST_RUN $(words $(TESTS))'; \
	awk -v component=$(COMPONENT) -v count=$(words $(TESTS)) -v failed=$$fail \
	  -v wall=$$elapsed -v budget=$(TEST_BUDGET_S) \
	  -f $(TOP)/factory/testlib/test-budget.awk $(OUT)/host/suite.time && test $$fail -eq 0
-include $(wildcard $(OUT)/host/*.d $(OUT)/tests/*.d)
smoke:
	+$(MAKE) -C $(TOP) -j2 image TARGET_CFLAGS='$(TARGET_CFLAGS) -g'
	+$(MAKE) smoke-conformance
	+$(MAKE) smoke-abi
	+$(MAKE) smoke-file-drive
.PHONY: red-once
red-once: test
	sh tests/red-once.sh $(TOP) $(OUT)/red "$(MAKE)"

include $(TOP)/mk/mutants.mk
# The live patches change the tenant runtime only the booted gate can judge.
.PHONY: mutants-live
mutants: mutants-live
mutants-live:
	sh tests/live/mutants.sh '$(TARGET_CFLAGS) -g'
include $(TOP)/mk/import-check.mk

$(OUT)/tests/conformance: tests/live/host.c tests/live/cases.h $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/conformance
.PHONY: smoke-conformance smoke-abi smoke-file-drive
smoke-conformance: $(OUT)/tests/conformance
	+$(MAKE) -f tests/live/Makefile TOP=$(TOP) OUT=$(OUT)/live
	SDK_HOST=$(OUT)/tests/conformance SDK_TENANT=$(OUT)/live/SDKCHECK.EXE sh tests/live/smoke.sh
smoke-abi:
	+$(MAKE) -f tests/live/Makefile TOP=$(TOP) OUT=$(OUT)/abi-live ABI=1
	SDK_ABI_TENANT=$(OUT)/abi-live sh tests/live/abi-smoke.sh
smoke-file-drive:
	+$(MAKE) -C $(TOP) -f factory/desktop-tools.mk all
	+$(MAKE) -f tests/live/Makefile TOP=$(TOP) OUT=$(OUT)/drive-live FILE_DRIVE=1
	sh tests/live/file-drive.sh "$(OUT)/drive-live/SDKDRIVE.EXE"
$(OUT)/tests/dispatch: tests/dispatch.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/dispatch

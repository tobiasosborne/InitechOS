TOP := $(abspath ..)
COMPONENT := sdk
include $(TOP)/mk/config.mk
HOST_AR ?= ar
HOST_LIB := $(OUT)/host/libtbx_host.a
HOST_TESTS := abi layout link host host32 mz search33 conformance dispatch
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
$(OUT)/tests/search33: tests/search33.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/search33
$(OUT)/tests/self-contained.o: tests/self-contained.c FORCE
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -MMD -MP -c $< -o $@)
test-build: $(OUT)/tests/audit $(OUT)/tests/layout.o $(OUT)/tests/link $(OUT)/tests/host $(OUT)/tests/self-contained.o $(OUT)/rt/tbx_rt.o
test: test-run
test-run: test-build
	@TEST_BUDGET_S=$(TEST_BUDGET_S) sh $(TOP)/factory/testlib/run-entries.sh $(COMPONENT) $(OUT) tests/run.sh $(TESTS)
-include $(wildcard $(OUT)/host/*.d $(OUT)/tests/*.d)
smoke:
	+$(MAKE) -C $(TOP) -j2 image TARGET_CFLAGS='$(TARGET_CFLAGS) -g'
	+$(MAKE) smoke-conformance
	+$(MAKE) smoke-abi
.PHONY: red-once
red-once: test
	sh tests/red-once.sh $(TOP) $(OUT)/red "$(MAKE)"

include $(TOP)/mk/mutants.mk
include $(TOP)/mk/import-check.mk

$(OUT)/tests/conformance: tests/live/host.c tests/live/cases.h $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/conformance
.PHONY: smoke-conformance smoke-abi
smoke-conformance: $(OUT)/tests/conformance
	+$(MAKE) -f tests/live/Makefile TOP=$(TOP) OUT=$(OUT)/live
	SDK_HOST=$(OUT)/tests/conformance SDK_TENANT=$(OUT)/live/SDKCHECK.EXE sh tests/live/smoke.sh
smoke-abi:
	+$(MAKE) -f tests/live/Makefile TOP=$(TOP) OUT=$(OUT)/abi-live ABI=1
	SDK_ABI_TENANT=$(OUT)/abi-live sh tests/live/abi-smoke.sh
$(OUT)/tests/dispatch: tests/dispatch.c $(HOST_LIB) FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -MMD -MP $< $(HOST_LIB) -o $@)
test-build: $(OUT)/tests/dispatch

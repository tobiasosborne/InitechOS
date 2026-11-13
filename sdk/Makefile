TOP := $(abspath ..)
COMPONENT := sdk
include $(TOP)/mk/config.mk
HOST_AR ?= ar
HEADERS := $(wildcard include/*.h) host/tbx_host.h
HOST_LIB := $(OUT)/host/libtbx_host.a
.PHONY: host abi layout link-check
all: host $(OUT)/rt/tbx_rt.o
host: $(HOST_LIB)
$(OUT)/host/tbx_host.o: host/tbx_host.c $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost -c $< -o $@
$(HOST_LIB): $(OUT)/host/tbx_host.o
	$(HOST_AR) rcsD $@ $<
$(OUT)/rt/tbx_rt.o: rt/tbx_rt.c $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -c $< -o $@
$(OUT)/tests/audit: tests/audit.c $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
abi: $(OUT)/tests/audit
	$< tests/traps.tsv include/toolbox_gate.h include/cp_dialog.h rt/tbx_rt.c $(TOP)/flair/tbxgate.c
$(OUT)/tests/layout.o: tests/layout.c $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -c $< -o $@
layout: $(OUT)/tests/layout.o
	@echo 'ABI: target i386 layouts pinned (compile-only)'
$(OUT)/tests/link.c: tests/traps.tsv tests/generate-link.sh include/tbx.h $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	sh tests/generate-link.sh $< include/tbx.h > $@
$(OUT)/tests/link: $(OUT)/tests/link.c $(HOST_LIB) $(HEADERS)
	$(HOSTCC) $(HOST_CFLAGS) -Iinclude $< $(HOST_LIB) -o $@
link-check: $(OUT)/tests/link
	$<
	@echo 'ABI: all bindings link against shared fake'
$(OUT)/tests/host: tests/host.c $(HOST_LIB) $(HEADERS)
	$(HOSTCC) $(HOST_CFLAGS) -Iinclude -Ihost $< $(HOST_LIB) -o $@
$(OUT)/tests/self-contained.o: tests/self-contained.c $(HEADERS) $(MAKEFILE_LIST)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -Iinclude -c $< -o $@
test: abi layout link-check $(OUT)/tests/host $(OUT)/tests/self-contained.o $(OUT)/rt/tbx_rt.o
	$(OUT)/tests/host
	sh tests/host32.sh "$(HOSTCC)" $(OUT)/tests/host32
	+$(MAKE) -C $(TOP)/factory/mz test
smoke:
	@echo 'Tier 1: launch 123.EXE on the booted desktop, pending desktop import' >&2; exit 1

.PHONY: red-once
red-once: test
	sh tests/red-once.sh $(TOP) $(OUT)/red "$(MAKE)"

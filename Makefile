# Compose components; recursive make retains each component's private output.
override TOP := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
override REGISTERED_COMPONENTS := flair/region apps/initech123 apps/initechword
COMPONENTS ?= $(REGISTERED_COMPONENTS)
.DEFAULT_GOAL := help
.PHONY: help all test smoke image cert clean status corpus test-start $(addprefix test-,$(COMPONENTS))
help:
	@echo 'make [-j] test | image | smoke | repro | status | cert (not implemented)'
	@echo 'registered components: $(REGISTERED_COMPONENTS)'
cert:
	@echo 'make cert: not implemented yet (see STATUS.md)' >&2; exit 1
all:
	@for c in $(COMPONENTS); do $(MAKE) --no-print-directory -C $$c all || exit 1; done
# Per-invocation scratch, removed after summary; never a global green stamp.
ifneq ($(filter test%,$(MAKECMDGOALS)),)
override TEST_RUN := $(shell mkdir -p $(TOP)/build/test && mktemp -d $(TOP)/build/test/run.XXXXXX)
endif
corpus:
	@$(MAKE) --no-print-directory -f goldens/preflight.mk verify
test-start:
	@date +%s > $(TEST_RUN)/start
	@$(MAKE) --no-print-directory -f goldens/preflight.mk verify
$(addprefix test-,$(COMPONENTS)): test-%: | test-start
	+@rc=0; $(MAKE) --no-print-directory -C $* test || rc=1; \
	  echo $$rc > $(TEST_RUN)/$(subst /,_,$*).result
test: test-start $(addprefix test-,$(COMPONENTS))
	@fail=0; run=0; for f in $(TEST_RUN)/*.result; do \
	  [ -f "$$f" ] || continue; run=$$((run+1)); fail=$$((fail+$$(cat "$$f"))); done; \
	echo "registered $(words $(REGISTERED_COMPONENTS)), run $$run; elapsed $$(($$(date +%s)-$$(cat $(TEST_RUN)/start)))s"; \
	rm -rf $(TEST_RUN); \
	if [ $$fail -ne 0 ]; then echo "$$fail component(s) RED"; exit 1; \
	elif [ $$run -ne $(words $(REGISTERED_COMPONENTS)) ] || [ "$(sort $(COMPONENTS))" != "$(sort $(REGISTERED_COMPONENTS))" ] || [ "$(origin TESTS)" != undefined ]; then echo 'SUBSET GREEN (not ALL GREEN)'; \
	else echo 'ALL GREEN'; fi
status: corpus
	@sh factory/testlib/status.sh $(COMPONENTS)
clean:
	rm -rf -- "$(TOP)/build"

.PHONY: boot-build catalog dos-build kernel-build factory-build run run-bochs repro
boot-build: catalog
	$(MAKE) -C boot
catalog:
	$(MAKE) -C kernel $(TOP)/build/kernel/dos_messages.h $(TOP)/build/kernel/dos_messages.inc
dos-build: catalog
	$(MAKE) -C dos
kernel-build: dos-build
	$(MAKE) -C kernel
factory-build:
	$(MAKE) -C factory
image: boot-build kernel-build factory-build
	$(MAKE) -C factory image
smoke: image
	$(MAKE) -C factory smoke
run: image
	qemu-system-i386 -m 8 -machine acpi=off -cpu 486 -drive file=build/image/boot.img,format=raw,index=0 -drive file=build/image/data.img,format=raw,index=1 -serial stdio -d int,guest_errors,cpu_reset -D build/qemu.log -no-reboot
run-bochs: image
	INITECH_TEST_RAM_MB=8 build/factory/bochs_harness --disk build/image/boot.img --disk2 build/image/data.img --serial-stdout --timeout-ms 3600000 --out build/factory
repro:
	sh factory/repro.sh

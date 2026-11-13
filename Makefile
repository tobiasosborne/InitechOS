# InitechOS -- top level. This file only composes components. Each
# component builds and tests itself; see CLAUDE.md.
override TOP := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
# Preserve an optional subset while registering the complete list.
ifneq ($(origin COMPONENTS),undefined)
override SELECTED_COMPONENTS := $(COMPONENTS)
endif
# Components in the new tree, in dependency order, one per line so that
# parallel imports merge without conflicts.
override COMPONENTS :=
override COMPONENTS += kernel
override COMPONENTS += dos
override COMPONENTS += flair/region
override COMPONENTS += flair
override COMPONENTS += flair/desktop
override COMPONENTS += sdk
override COMPONENTS += apps/initech123
override COMPONENTS += apps/initechword
override COMPONENTS += apps/initechbase
override COMPONENTS += turbo
override COMPONENTS += system
override REGISTERED_COMPONENTS := $(COMPONENTS)
ifneq ($(origin SELECTED_COMPONENTS),undefined)
override COMPONENTS := $(SELECTED_COMPONENTS)
endif
.DEFAULT_GOAL := help
CERT_STRICT ?= 0
.PHONY: help all test smoke image cert clean status corpus test-start $(addprefix test-,$(COMPONENTS))
help:
	@echo 'make [-j] test | image | smoke | repro | status | cert'
	@echo 'registered components: $(REGISTERED_COMPONENTS)'
cert:
	+@CERT_STRICT=$(CERT_STRICT) sh factory/cert.sh
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
# The same declaration drives ordering, the graph display and boundary audit.
include mk/dependencies.mk
$(foreach c,$(COMPONENTS),$(eval test-$(c): $(addprefix test-,$(filter $(COMPONENTS),$(if $(filter undefined,$(origin TEST_DEPENDS_$(c))),$(DEPENDS_$(c)),$(TEST_DEPENDS_$(c)))))))
.PHONY: dependencies dependency-check
dependencies:
	@$(foreach c,$(REGISTERED_COMPONENTS),echo '$(c): $(DEPENDS_$(c))';)
dependency-check:
	@mkdir -p build/dependencies
	@$(foreach c,$(REGISTERED_COMPONENTS),$(MAKE) --no-print-directory -qp -C $(c) all test > build/dependencies/$(subst /,_,$(c)).make 2>/dev/null || test $$? -eq 1; awk -v root=$(TOP) -v component=$(c) -v components='$(REGISTERED_COMPONENTS) boot factory factory/mz turbo/seed turbo/compiler' -v declared='$(DEPENDS_$(c))' -f factory/testlib/build-deps.awk build/dependencies/$(subst /,_,$(c)).make || exit 1;)
test-start: dependency-check
$(addprefix test-,$(COMPONENTS)): private export INITECH_CORPUS_VERIFIED := 1
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
status: export INITECH_CORPUS_VERIFIED := 1
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
image: boot-build system-build factory-build
	$(MAKE) -C factory image
# Components with their own emulator session on the product image
# (`make -C <component> smoke`), one per line.
SMOKE_COMPONENTS :=
SMOKE_COMPONENTS += turbo
smoke: image
	$(MAKE) -C factory smoke
	@for c in $(SMOKE_COMPONENTS); do $(MAKE) --no-print-directory -C $$c smoke || exit 1; done
run: image
	qemu-system-i386 -m 8 -machine acpi=off -cpu 486 -drive file=build/image/boot.img,format=raw,index=0 -drive file=build/image/data.img,format=raw,index=1 -serial stdio -d int,guest_errors,cpu_reset -D build/qemu.log -no-reboot
run-bochs: image
	INITECH_TEST_RAM_MB=8 build/factory/bochs_harness --disk build/image/boot.img --disk2 build/image/data.img --serial-stdout --timeout-ms 3600000 --out build/factory
repro:
	sh factory/repro.sh
.PHONY: system-build
system-build: kernel-build
	$(MAKE) -C system
.PHONY: desktop-apps
desktop-apps:
	$(MAKE) -C apps/initech123
system-build: desktop-apps

# InitechOS -- top level. This file only composes components. Each
# component builds and tests itself; see CLAUDE.md.

# Components that have been imported into the new tree, in dependency
# order. Add a component here when its `make test` is green.
# One per line so that parallel imports merge without conflicts.
COMPONENTS :=
COMPONENTS += kernel
COMPONENTS += dos
COMPONENTS += flair/region
COMPONENTS += flair
COMPONENTS += sdk
COMPONENTS += apps/initech123
COMPONENTS += apps/initechword
COMPONENTS += apps/initechbase
COMPONENTS += turbo

.DEFAULT_GOAL := help
.PHONY: help all test smoke image cert clean

# Not built yet. A target that does nothing must fail, never succeed.
cert:
	@echo "make $@: not implemented yet (see STATUS.md)" >&2; exit 1

help:
	@echo "make test    tier 0: host tests of every imported component"
	@echo "make image   build the product disk set"
	@echo "make smoke   tier 1: drive QEMU and Bochs"
	@echo "make cert    tier 2: the full certificate   (not yet)"
	@echo "imported components: $(COMPONENTS)"

all:
	@for c in $(COMPONENTS); do $(MAKE) --no-print-directory -C $$c all || exit 1; done

test:
	@fail=0; for c in $(COMPONENTS); do \
	  $(MAKE) --no-print-directory -C $$c test || fail=$$((fail+1)); \
	done; \
	if [ $$fail -eq 0 ]; then echo "ALL GREEN"; else echo "$$fail component(s) RED"; exit 1; fi

clean:
	rm -rf build

.PHONY: boot-build catalog dos-build kernel-build factory-build run run-bochs repro
boot-build: catalog
	$(MAKE) -C boot
catalog:
	$(MAKE) -C kernel ../build/kernel/dos_messages.h ../build/kernel/dos_messages.inc
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

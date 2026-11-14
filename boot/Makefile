TOP := ..
COMPONENT := boot
include $(TOP)/mk/config.mk
all: $(TOP)/build/kernel/dos_messages.inc $(OUT)/mbr.bin $(OUT)/stage2.bin $(OUT)/IO.SYS
$(OUT)/mbr.bin: mbr.asm | $(OUT)
	$(NASM) -I$(TOP)/build/kernel/ -f bin $< -o $@
$(OUT)/IO.SYS: io_sys.asm $(TOP)/build/kernel/dos_messages.inc | $(OUT)
	$(NASM) -I$(TOP)/build/kernel/ -f bin $< -o $@
$(OUT)/stage2.bin: stage2.asm $(OUT)/IO.SYS | $(OUT)
	$(NASM) -I$(TOP)/build/kernel/ -f bin $< -o $@
TARGET_ARTIFACTS := $(OUT)/mbr.bin $(OUT)/stage2.bin $(OUT)/IO.SYS
IMPORT_MUT_PATTERN := _(MUT|MUTATE|MUTANT)_|DEC03A_|FORCE_VBE_101
HOST_TESTS := layout
HOST_INCLUDES := -I$(TOP)/factory/include
layout_SRCS := tests/layout.c
layout_ARGS := $(OUT)/mbr.bin $(OUT)/stage2.bin $(OUT)/IO.SYS
include $(TOP)/mk/host-test.mk
include $(TOP)/mk/import-check.mk
test-build: all
bigkernel-check:
	$(MAKE) -C $(TOP) clean
	$(MAKE) -C $(TOP) BIG_KERNEL=1 image
	sh $(TOP)/factory/bigkernel-check.sh
	$(MAKE) -C $(TOP) image
.PHONY: smoke smoke-build mutants
$(OUT)/rom-checksum: tests/rom-checksum.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/indexed.rom: tests/indexed-rom.asm $(OUT)/rom-checksum
	$(NASM) -DINDEXED_ONLY -f bin $< -o $@
	$(OUT)/rom-checksum $@
$(OUT)/monitor.rom: tests/indexed-rom.asm $(OUT)/rom-checksum
	$(NASM) -f bin $< -o $@
	$(OUT)/rom-checksum $@
smoke-build: all $(OUT)/indexed.rom $(OUT)/monitor.rom
	$(MAKE) -C $(TOP)/factory all
smoke: smoke-build
	+sh tests/boot.sh "$(TOP)" "$(OUT)"
mutants: smoke-build
	+sh tests/mutants.sh "$(TOP)" "$(OUT)" "$(MAKE)" $(MUTANT_PATCHES)
MUTANT_PATCHES ?= $(wildcard mutants/*.patch)

$(TOP)/build/kernel/dos_messages.inc: $(TOP)/kernel/include/dos_messages.json $(TOP)/kernel/gen_messages.c
	$(MAKE) -C $(TOP)/kernel $(TOP)/build/kernel/dos_messages.h $(TOP)/build/kernel/dos_messages.inc

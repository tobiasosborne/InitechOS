TOP := ..
COMPONENT := boot
include $(TOP)/mk/config.mk
TARGET_BIN_ASFLAGS := -I$(TOP)/build/kernel/ -f bin
include $(TOP)/mk/target.mk
all: $(TOP)/build/kernel/dos_messages.inc $(OUT)/mbr.bin $(OUT)/stage2.bin $(OUT)/IO.SYS
$(OUT)/IO.SYS: io_sys.asm $(TOP)/build/kernel/dos_messages.inc FORCE
	$(call run-command,$(NASM) $(TARGET_BIN_ASFLAGS) -MD $@.d -MP -MT $@ $< -o $@)
$(OUT)/stage2.bin: $(OUT)/IO.SYS
test: all
	@test "$$(wc -c < $(OUT)/mbr.bin)" -eq 512
	@test "$$(wc -c < $(OUT)/stage2.bin)" -eq 8192
	@echo "boot: layout sizes green"
bigkernel-check:
	$(MAKE) -C $(TOP) clean
	$(MAKE) -C $(TOP) BIG_KERNEL=1 image
	sh $(TOP)/factory/bigkernel-check.sh
	rm $(TOP)/build/kernel/kstart.o
	$(MAKE) -C $(TOP) image
smoke:
	$(MAKE) -C $(TOP) smoke

$(TOP)/build/kernel/dos_messages.inc: $(TOP)/kernel/include/dos_messages.json $(TOP)/kernel/gen_messages.c
	$(MAKE) -C $(TOP)/kernel $(TOP)/build/kernel/dos_messages.h $(TOP)/build/kernel/dos_messages.inc

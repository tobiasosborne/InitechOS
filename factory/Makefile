TOP := ..
COMPONENT := factory
include $(TOP)/mk/config.mk
all: $(OUT)/qemu_harness $(OUT)/bochs_harness $(OUT)/boot_system
$(OUT)/qemu_harness: emu/qemu.c emu/qemu_main.c emu/qemu.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) emu/qemu.c emu/qemu_main.c -o $@
$(OUT)/bochs_harness: emu/bochs.c emu/bochs_main.c emu/bochs.h emu/bochs_trace.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) emu/bochs.c emu/bochs_main.c -o $@
$(OUT)/boot_system: image/boot_system.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
image: all
	sh image/build.sh
smoke:
	sh smoke.sh
all: $(OUT)/keys
$(OUT)/keys: keys.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@

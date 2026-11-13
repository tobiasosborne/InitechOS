TOP := ..
COMPONENT := factory
include $(TOP)/mk/config.mk
all: $(OUT)/qemu_harness $(OUT)/bochs_harness $(OUT)/boot_system
$(OUT)/qemu_harness: emu/qemu.c emu/qemu_main.c emu/qemu.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) emu/qemu.c emu/qemu_main.c -o $@)
$(OUT)/bochs_harness: emu/bochs.c emu/bochs_main.c emu/bochs.h emu/bochs_trace.h FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) emu/bochs.c emu/bochs_main.c -o $@)
$(OUT)/boot_system: image/boot_system.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
image: all
	sh image/build.sh
smoke:
	sh smoke.sh
all: $(OUT)/keys
$(OUT)/keys: keys.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)

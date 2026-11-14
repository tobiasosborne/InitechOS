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
	SYSTEM_IMAGE=$(TOP)/build/system/INITDOS$(if $(BIG_KERNEL),-big).SYS sh image/build.sh
smoke:
	sh smoke.sh
	$(MAKE) smoke-programs
	sh smoke-desktop.sh
all: $(OUT)/keys
$(OUT)/keys: keys.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
all: $(OUT)/pixels-desktop
$(OUT)/pixels-desktop: pixels-desktop.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/trace-input-test: trace-input-test.c emu/bochs_trace.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
test: $(OUT)/trace-input-test
	$<

.PHONY: smoke-programs
smoke-programs:
	$(MAKE) -C $(TOP)/dos smoke-programs
	sh smoke-programs.sh
.PHONY: smoke-write-readback
smoke-write-readback:
	$(MAKE) -C $(TOP)/dos smoke-programs
	sh tests/write-readback.sh
.PHONY: testlib-test
test: testlib-test image-order-test desktop-tools-test dataloss-test
.PHONY: dataloss-test
dataloss-test:
	sh $(TOP)/goldens/dataloss/verify.sh
.PHONY: desktop-tools-test
desktop-tools-test:
	+$(MAKE) -C $(TOP) -f factory/desktop-tools.mk test
.PHONY: image-order-test
image-order-test:
	sh image/tests/order.sh
testlib-test:
	sh testlib/tests/audit.sh
	sh testlib/tests/mutants.sh
	sh testlib/tests/assertions.sh

.PHONY: mutants
mutants:
	+sh red-desktop.sh

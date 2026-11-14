TOP := ..
COMPONENT := factory
include $(TOP)/mk/config.mk
export LOTUS123_DECOMP WORDPERFECT51_DECOMP
all: $(OUT)/qemu_harness $(OUT)/bochs_harness $(OUT)/boot_system
$(OUT)/qemu_harness: emu/qemu.c emu/qemu_main.c emu/qemu.h emu/port_lock.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) emu/qemu.c emu/qemu_main.c -o $@
$(OUT)/bochs_harness: emu/bochs.c emu/bochs_main.c emu/bochs.h emu/bochs_trace.h emu/rfb_port.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) emu/bochs.c emu/bochs_main.c -o $@
$(OUT)/boot_system: image/boot_system.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
image: all
	SYSTEM_IMAGE=$(TOP)/build/system/INITDOS$(if $(BIG_KERNEL),-big).SYS sh image/build.sh
smoke:
	+@set -e; \
	sh smoke.sh; $(MAKE) smoke-programs; sh smoke-desktop.sh
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
test: testlib-test image-order-test desktop-tools-test dataloss-test host-paths-test
.PHONY: host-paths-test
host-paths-test:
	sh tests/host-paths.sh
.PHONY: dataloss-test
dataloss-test:
	+$(MAKE) -C $(TOP) -f goldens/preflight.mk verify CORPORA='LOTUS123_DECOMP WORDPERFECT51_DECOMP'
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
	sh testlib/tests/host-mutants.sh

.PHONY: mutants
mutants:
	+sh red-desktop.sh
	+sh gate-mutants.sh
	+sh smoke-mutants.sh
	+sh navigation-mutants.sh
	+sh testlib/tests/host-mutants.sh
	+sh desktop-wait-mutants.sh

.PHONY: gate-test
$(OUT)/bochs-verdict-test: tests/bochs-verdict.c emu/bochs.c emu/bochs.h emu/bochs_trace.h emu/rfb_port.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
test: gate-test
gate-test: $(OUT)/bochs-verdict-test $(OUT)/qemu-deadline-test $(OUT)/port-lock-test $(OUT)/rfb-port-test
	$(OUT)/bochs-verdict-test
	sh tests/worksheet.sh
	+sh tests/registration.sh
	+sh tests/selection.sh
	+sh tests/boot-registration.sh
	+sh tests/cert.sh
	+sh tests/dependencies.sh
	+sh tests/dependency-query.sh
	+sh tests/snapshots.sh
	+sh tests/desktop-sessions.sh
	$(OUT)/qemu-deadline-test
	$(OUT)/port-lock-test
	$(OUT)/rfb-port-test
$(OUT)/rfb-port-test: tests/rfb-port.c emu/rfb_port.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/port-lock-test: tests/port-lock.c emu/port_lock.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/qemu-deadline-test: tests/qemu-deadline.c emu/qemu.c emu/qemu.h emu/port_lock.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/desktop-reference: desktop-reference.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -lpng -o $@
all: $(OUT)/desktop-reference
.PHONY: pixels-test
test: pixels-test
pixels-test: $(OUT)/pixels-desktop
	sh tests/pixels.sh

$(OUT)/qemu-trace-test: tests/qemu-trace.c emu/qemu.c emu/qemu.h emu/port_lock.h | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
gate-test: qemu-trace-check
.PHONY: qemu-trace-check
qemu-trace-check: $(OUT)/qemu-trace-test
	$(OUT)/qemu-trace-test

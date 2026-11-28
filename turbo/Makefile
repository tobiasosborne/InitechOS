TOP := $(abspath ..)
COMPONENT := turbo
include $(TOP)/mk/config.mk

all test mutants:
	+@for c in seed compiler ide tests; do $(MAKE) --no-print-directory -C $$c $@ || exit 1; done

smoke:
	+$(MAKE) -C $(TOP) image
	+$(MAKE) smoke-on-image
.PHONY: smoke-on-image smoke-execution smoke-ide smoke-native smoke-interactive
smoke-on-image: smoke-execution smoke-ide smoke-native smoke-interactive
smoke-execution smoke-ide smoke-native smoke-interactive: all $(OUT)/smoke/TIMER.COM
smoke-execution:
	sh tests/execution/run.sh
smoke-ide:
	sh ide/tests/smoke.sh
smoke-native:
	sh tests/ide/native.sh
smoke-interactive:
	sh tests/ide/interactive.sh
$(OUT)/smoke/timer.o: tests/smoke/timer.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@
$(OUT)/smoke/TIMER.COM: $(OUT)/smoke/timer.o
	$(TARGET_LD) $(TARGET_LDFLAGS) -Ttext 0x40100 -e _start --oformat binary $< -o $@
$(OUT)/smoke/compare: tests/smoke/compare.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
$(OUT)/smoke/monotonic-ms: tests/smoke/monotonic-ms.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
selfhost: all $(OUT)/smoke/TIMER.COM $(OUT)/smoke/compare $(OUT)/smoke/monotonic-ms
	+$(MAKE) -C $(TOP) image
	sh tests/smoke/selfhost.sh
ddc: all $(OUT)/smoke/TIMER.COM $(OUT)/smoke/compare $(OUT)/smoke/monotonic-ms
	@test -s $(OUT)/selfhost/K2.COM -a -s $(OUT)/selfhost/K3.COM || { echo 'Run make selfhost first for this source.' >&2; exit 1; }
	sh tests/smoke/ddc.sh

TARGET_ARTIFACTS := $(TOP)/build/turbo/compiler/TURBO.COM
all: $(TARGET_ARTIFACTS)
$(TARGET_ARTIFACTS): | compiler-build
	@test -f $@
.PHONY: compiler-build
compiler-build:
	+@$(MAKE) --no-print-directory -C compiler all

IMPORT_PRODUCT_SRCS := $(wildcard seed/*.[ch] seed/rt/*.asm compiler/*.pas ide/*.pas)
include $(TOP)/mk/import-check.mk

TOP := $(abspath ..)
COMPONENT := turbo
include $(TOP)/mk/config.mk

all test mutants:
	+@for c in seed compiler; do $(MAKE) --no-print-directory -C $$c $@ || exit 1; done

smoke: all
	+$(MAKE) -C $(TOP) image
	sh tests/smoke/run.sh
$(OUT)/smoke/timer.o: tests/smoke/timer.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@
$(OUT)/smoke/TIMER.COM: $(OUT)/smoke/timer.o
	$(TARGET_LD) $(TARGET_LDFLAGS) -Ttext 0x40100 -e _start --oformat binary $< -o $@
smoke: $(OUT)/smoke/TIMER.COM
$(OUT)/smoke/compare: tests/smoke/compare.c
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $< -o $@
selfhost: all $(OUT)/smoke/TIMER.COM $(OUT)/smoke/compare
	+$(MAKE) -C $(TOP) image
	sh tests/smoke/selfhost.sh
ddc: selfhost
	sh tests/smoke/ddc.sh

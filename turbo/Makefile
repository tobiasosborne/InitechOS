TOP := $(abspath ..)
COMPONENT := turbo
include $(TOP)/mk/config.mk

all test mutants:
	+@for c in seed compiler; do $(MAKE) --no-print-directory -C $$c $@ || exit 1; done

selfhost ddc:
	@echo '$@: not implemented; see ROADMAP.md' >&2; exit 1
smoke: all
	+$(MAKE) -C $(TOP) image
	sh tests/smoke/run.sh
$(OUT)/smoke/timer.o: tests/smoke/timer.c
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c $< -o $@
$(OUT)/smoke/TIMER.COM: $(OUT)/smoke/timer.o
	$(TARGET_LD) $(TARGET_LDFLAGS) -Ttext 0x40100 -e _start --oformat binary $< -o $@
smoke: $(OUT)/smoke/TIMER.COM

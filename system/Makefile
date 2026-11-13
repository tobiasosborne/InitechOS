TOP := ..
COMPONENT := system
include $(TOP)/mk/config.mk
KERNEL := $(filter-out $(TOP)/build/kernel/kstart.o,$(wildcard $(TOP)/build/kernel/*.o))
COMMAND := $(addprefix $(TOP)/build/dos/,command.o batch.o env.o)
all: $(OUT)/INITDOS.SYS
.PHONY: components
components:
	+$(MAKE) -C $(TOP)/kernel objects resident-command
	+$(MAKE) -C $(TOP)/flair
	+$(MAKE) -C $(TOP)/flair/desktop target
$(OUT)/system.elf: components $(TOP)/kernel/kernel.ld | $(OUT)
	$(TARGET_LD) $(TARGET_LDFLAGS) -T $(TOP)/kernel/kernel.ld -o $@ $(TOP)/build/kernel/kstart.o $(KERNEL) $(COMMAND) --start-group --whole-archive $(TOP)/build/flair/desktop/libdesktop.a $(TOP)/build/flair/libflair.a --no-whole-archive $(TOP)/build/flair/region/target/region.o --end-group
$(OUT)/INITDOS.SYS: $(OUT)/system.elf
	objcopy -O binary $< $@
	@echo "Product loaded bytes: $$(wc -c < $@)"
.PHONY: test smoke mutants
test:
	+$(MAKE) -C $(TOP)/kernel all
	+$(MAKE) all
	+$(MAKE) -C $(TOP)/factory test
	sh test.sh
smoke:
	+$(MAKE) -C $(TOP) smoke
mutants:
	@echo 'Use factory/red-desktop.sh for the two slice mutations' >&2; exit 1

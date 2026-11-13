TOP := ..
COMPONENT := system
include $(TOP)/mk/config.mk
KERNEL := $(shell MAKEFLAGS= $(MAKE) --no-print-directory -s -C $(TOP)/kernel object-list BIG_KERNEL=$(BIG_KERNEL))
COMMAND := $(addprefix $(TOP)/build/dos/,command-resident.o batch.o env.o)
all: $(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS
.PHONY: components
components:
	+$(MAKE) -C $(TOP)/kernel objects resident-command
	+$(MAKE) -C $(TOP)/flair
	+$(MAKE) -C $(TOP)/flair/desktop target
$(KERNEL) $(COMMAND) $(TOP)/build/flair/desktop/libdesktop.a $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o: | components
	@test -f $@
$(OUT)/system$(if $(BIG_KERNEL),-big).elf: $(KERNEL) $(COMMAND) $(TOP)/build/flair/desktop/libdesktop.a $(TOP)/build/flair/libflair.a $(TOP)/build/flair/region/target/region.o $(TOP)/kernel/kernel.ld FORCE | components $(OUT)
	$(call run-command,$(TARGET_LD) $(TARGET_LDFLAGS) -T $(TOP)/kernel/kernel.ld -o $@ $(KERNEL) $(COMMAND) --start-group --whole-archive $(TOP)/build/flair/desktop/libdesktop.a $(TOP)/build/flair/libflair.a --no-whole-archive $(TOP)/build/flair/region/target/region.o --end-group)
$(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS: $(OUT)/system$(if $(BIG_KERNEL),-big).elf
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

TARGET_ARTIFACTS := $(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS
include $(TOP)/mk/import-check.mk

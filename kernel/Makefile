TOP := ..
COMPONENT := kernel
include $(TOP)/mk/config.mk
TARGET_INCLUDES := -Iinclude -I. -I$(OUT)
SOURCES := main startup console console_pixels idt pic panic int21 devices mcb psp sft config_sys sysinit loader ata block_device fat12 fileio_fat kbd mouse pit rtc irq
OBJECTS := $(addprefix $(OUT)/,$(addsuffix .o,$(SOURCES))) $(OUT)/kstart.o $(OUT)/isr.o
TARGET_SRCS := $(addsuffix .c,$(SOURCES)) kstart.asm isr.asm
include $(TOP)/mk/target.mk
all: $(OUT)/INITDOS.SYS
$(OUT)/kstart.o: TARGET_ASFLAGS += -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC $(if $(BIG_KERNEL),-DMK_BIG_KERNEL)
$(OUT)/kstart.o: include/memory_map.h
$(OUT)/gen_messages: gen_messages.c FORCE
	$(call run-command,$(HOSTCC) $(HOST_CFLAGS) $< -o $@)
$(OUT)/dos_messages.h: include/dos_messages.json $(OUT)/gen_messages
	$(OUT)/gen_messages < $< > $@.tmp && mv $@.tmp $@
$(OBJECTS): $(OUT)/dos_messages.h
$(OUT)/kernel.elf: $(OBJECTS) $(TOP)/build/dos/command.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o kernel.ld FORCE
	$(call run-command,$(TARGET_LD) $(TARGET_LDFLAGS) -T kernel.ld -o $@ $(OUT)/kstart.o $(filter-out $(OUT)/kstart.o,$(OBJECTS)) $(TOP)/build/dos/command.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o)
$(OUT)/INITDOS.SYS: $(OUT)/kernel.elf
	objcopy -O binary $< $@
	@echo "InitechDOS loaded bytes: $$(wc -c < $@)"
test:
	@echo "kernel: tier 0 import pending (tests parked under tests/)"
smoke:
	$(MAKE) -C $(TOP) smoke
all: $(OUT)/dos_messages.inc
$(OUT)/dos_messages.inc: $(OUT)/dos_messages.h
	sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$$/%define \1 `\2`/p' $< > $@.tmp && mv $@.tmp $@
# A direct component build also refreshes the temporary resident shell objects.
.PHONY: resident-command
resident-command: $(OUT)/dos_messages.h
	$(MAKE) -C $(TOP)/dos resident
$(OUT)/kernel.elf: $(wildcard $(TOP)/dos/command/*.[ch]) | resident-command
$(TOP)/build/dos/command.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o: | resident-command
	@test -f $@

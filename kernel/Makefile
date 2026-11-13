TOP := ..
COMPONENT := kernel
include $(TOP)/mk/config.mk
TARGET_INCLUDES := -Iinclude -I. -I$(OUT)
SOURCES := main startup console console_pixels idt pic panic int21 devices mcb psp sft config_sys sysinit loader ata block_device fat12 fileio_fat kbd mouse pit rtc irq
OBJECTS := $(addprefix $(OUT)/,$(addsuffix .o,$(SOURCES))) $(OUT)/kstart.o $(OUT)/isr.o
include $(TOP)/mk/target.mk
all: $(OUT)/INITDOS.SYS
$(OUT)/dos_messages.h: include/dos_messages.json gen_messages.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) gen_messages.c -o $(OUT)/gen_messages
	$(OUT)/gen_messages < $< > $@
$(OBJECTS): $(OUT)/dos_messages.h
$(OUT)/kstart.o: kstart.asm include/memory_map.h Makefile | $(OUT)
	$(NASM) -f elf32 -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC $(if $(BIG_KERNEL),-DMK_BIG_KERNEL) $< -o $@
$(OUT)/isr.o: isr.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@
$(OUT)/kernel.elf: $(OBJECTS) $(TOP)/build/dos/command.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o kernel.ld
	$(TARGET_LD) $(TARGET_LDFLAGS) -T kernel.ld -o $@ $(OUT)/kstart.o $(filter-out $(OUT)/kstart.o,$(OBJECTS)) $(TOP)/build/dos/command.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o
$(OUT)/INITDOS.SYS: $(OUT)/kernel.elf
	objcopy -O binary $< $@
	@echo "InitechDOS loaded bytes: $$(wc -c < $@)"
test:
	@echo "kernel: tier 0 import pending (tests parked under tests/)"
smoke:
	$(MAKE) -C $(TOP) smoke
all: $(OUT)/dos_messages.inc
$(OUT)/dos_messages.inc: $(OUT)/dos_messages.h
	sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$$/%define \1 `\2`/p' $< > $@

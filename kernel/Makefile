TOP := ..
COMPONENT := kernel
include $(TOP)/mk/config.mk
TARGET_INCLUDES := -Iinclude -I. -I$(OUT)
SOURCES := main startup environment console console_pixels idt pic panic int21 devices mcb psp sft config_sys sysinit loader ata block_device fat12 fileio_fat kbd mouse pit rtc irq
OBJECTS := $(addprefix $(OUT)/,$(addsuffix .o,$(SOURCES))) $(OUT)/kstart$(if $(BIG_KERNEL),-big).o $(OUT)/isr.o
include $(TOP)/mk/target.mk
all: $(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS
$(OUT)/dos_messages.h: include/dos_messages.json gen_messages.c | $(OUT)
	$(HOSTCC) $(HOST_CFLAGS) gen_messages.c -o $(OUT)/gen_messages
	$(OUT)/gen_messages < $< > $@
$(OBJECTS): $(OUT)/dos_messages.h
$(OUT)/kstart$(if $(BIG_KERNEL),-big).o: kstart.asm include/memory_map.h FORCE | $(OUT)
	$(call run-command,$(NASM) -f elf32 -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC $(if $(BIG_KERNEL),-DMK_BIG_KERNEL) $< -o $@)
$(OUT)/isr.o: isr.asm | $(OUT)
	$(NASM) -f elf32 $< -o $@
$(OUT)/kernel$(if $(BIG_KERNEL),-big).elf: $(OBJECTS) $(TOP)/build/dos/command-resident.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o kernel.ld
	$(TARGET_LD) $(TARGET_LDFLAGS) -T kernel.ld -o $@ $(OUT)/kstart$(if $(BIG_KERNEL),-big).o $(filter-out $(OUT)/kstart$(if $(BIG_KERNEL),-big).o,$(OBJECTS)) $(TOP)/build/dos/command-resident.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o
$(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS: $(OUT)/kernel$(if $(BIG_KERNEL),-big).elf
	objcopy -O binary $< $@
	@echo "InitechDOS loaded bytes: $$(wc -c < $@)"
smoke:
	$(MAKE) smoke-shell-config
	$(MAKE) smoke-ctrl-break
	$(MAKE) -C $(TOP) smoke
smoke: smoke-conpair
.PHONY: smoke-shell-config
smoke-shell-config:
	$(MAKE) -C $(TOP) image
	sh tests/smoke/shell-config33.sh "$(TOP)" "$(OUT)/smoke/shell-config33"
.PHONY: smoke-ctrl-break
smoke-ctrl-break:
	$(MAKE) -C $(TOP) image
	sh tests/smoke/ctrl-break.sh "$(TOP)" "$(OUT)/smoke/ctrl-break"
.PHONY: smoke-conpair
smoke-conpair: $(OUT)/smoke/CONPAIR10.COM $(OUT)/smoke/CONPAIR13.COM
	$(MAKE) -C $(TOP) image
	sh tests/smoke/conpair.sh "$(TOP)" "$(OUT)/smoke/conpair"
$(OUT)/smoke/CONPAIR10.COM: tests/programs/conpair_program.asm
	@mkdir -p $(@D)
	$(NASM) -f bin -DSCAN=0x10 $< -o $@
$(OUT)/smoke/CONPAIR13.COM: tests/programs/conpair_program.asm
	@mkdir -p $(@D)
	$(NASM) -f bin -DSCAN=0x13 $< -o $@
all: $(OUT)/dos_messages.inc
$(OUT)/dos_messages.inc: $(OUT)/dos_messages.h
	sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$$/%define \1 `\2`/p' $< > $@
# A direct component build also refreshes the temporary resident shell objects.
.PHONY: resident-command
resident-command: $(OUT)/dos_messages.h
	$(MAKE) -C $(TOP)/dos resident
$(OUT)/kernel$(if $(BIG_KERNEL),-big).elf: $(wildcard $(TOP)/dos/command/*.[ch]) | resident-command
$(TOP)/build/dos/command-resident.o $(TOP)/build/dos/batch.o $(TOP)/build/dos/env.o: | resident-command
	@test -f $@

# Host test wiring (separate from target build rules).
include tests.mk

# The product composition also consumes these standalone kernel objects.
.PHONY: objects
objects: $(OBJECTS)

TARGET_ARTIFACTS := $(OUT)/INITDOS$(if $(BIG_KERNEL),-big).SYS
.PHONY: object-list
object-list:
	@echo $(OBJECTS)

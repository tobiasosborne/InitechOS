FDISK_SRC := os/milton/fdisk.c
FDISK_FLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Ios/milton
FDISK_PROG_BIN := $(BUILD)/fdisk_program.bin
$(BUILD)/test_fdisk: harness/diff/fat_diff/test_fdisk.c $(FDISK_SRC) os/milton/fdisk.h | $(BUILD)
	$(CC) $(FDISK_FLAGS) harness/diff/fat_diff/test_fdisk.c $(FDISK_SRC) -o $@
.PHONY: test-fdisk test-fdisk-mutant
test-fdisk: $(BUILD)/test_fdisk $(FAT_DUMP_BIN)
	@sh harness/diff/fat_diff/fdisk.sh $(BUILD) $<
define fdisk-mutant
$(BUILD)/test_fdisk_$(1): harness/diff/fat_diff/test_fdisk.c $(FDISK_SRC) os/milton/fdisk.h | $(BUILD)
	$$(CC) $$(FDISK_FLAGS) -DFDISK_MUT_$(1) harness/diff/fat_diff/test_fdisk.c $$(FDISK_SRC) -o $$@
endef
$(foreach m,CHS SIGNATURE ACTIVE CONFIRM,$(eval $(call fdisk-mutant,$(m))))
test-fdisk-mutant: $(FAT_DUMP_BIN) $(foreach m,CHS SIGNATURE ACTIVE CONFIRM,$(BUILD)/test_fdisk_$(m))
	@set -eu; for m in CHS SIGNATURE ACTIVE CONFIRM; do \
	 case $$m in CHS|SIGNATURE) why='RED fdisk CHS/relative-sector/type/signature vs mpartition';; ACTIVE) why='RED fdisk active flag on wrong entry';; CONFIRM) why='RED fdisk delete without confirmation';; esac; \
	 if sh harness/diff/fat_diff/fdisk.sh $(BUILD) $(BUILD)/test_fdisk_$$m > $(BUILD)/fdisk-mut-$$m.log 2>&1; then echo "FAIL fdisk $$m survived"; exit 1; fi; \
	 grep -Fx "$$why" $(BUILD)/fdisk-mut-$$m.log || { cat $(BUILD)/fdisk-mut-$$m.log; exit 1; }; done
	@echo 'GREEN test-fdisk-mutant: CHS SIGNATURE ACTIVE CONFIRM rejected'
TEST_UNIT_GATES += test-fdisk test-fdisk-mutant
$(BUILD)/fdisk_start.o: os/milton/fdisk_start.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/fdisk_program.o: os/milton/fdisk_program.c os/milton/fdisk.h os/milton/power.h os/milton/diskutil.h spec/drive_letters.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS_CDECL) -Os -Ispec -Ios/milton -c $< -o $@
$(BUILD)/fdisk.o: $(FDISK_SRC) os/milton/fdisk.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS_CDECL) -Os -Ispec -Ios/milton -c $< -o $@
$(BUILD)/fdisk_program.elf: $(BUILD)/fdisk_start.o $(BUILD)/fdisk_program.o $(BUILD)/fdisk.o os/milton/fdisk.ld
	$(LD) -m elf_i386 -T os/milton/fdisk.ld -o $@ $(filter %.o,$^)
$(FDISK_PROG_BIN): $(BUILD)/fdisk_program.elf
	$(OBJCOPY) -O binary $< $@
.PHONY: test-fdisk-emu test-fdisk-emu-mutant
$(BUILD)/fdisk_fixture.bin: os/milton/fdisk_fixture.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
test-fdisk-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(CONSOLE_TEXT_BIN) $(FAT_DUMP_BIN) $(FDISK_PROG_BIN) $(BUILD)/fdisk_fixture.bin
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/fdisk_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FDISK_PROG_BIN)
define fdisk-emu-mutant
$(BUILD)/fdisk_$(1).o: $(FDISK_SRC) os/milton/fdisk.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS_CDECL) -Os -DFDISK_MUT_$(1) -Ios/milton -c $$< -o $$@
$(BUILD)/fdisk_$(1).elf: $(BUILD)/fdisk_start.o $(BUILD)/fdisk_program.o $(BUILD)/fdisk_$(1).o os/milton/fdisk.ld
	$$(LD) -m elf_i386 -T os/milton/fdisk.ld -o $$@ $$(filter %.o,$$^)
$(BUILD)/fdisk_$(1).bin: $(BUILD)/fdisk_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
endef
$(foreach m,CHS SIGNATURE ACTIVE CONFIRM LOADED,$(eval $(call fdisk-emu-mutant,$(m))))
test-fdisk-emu-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(CONSOLE_TEXT_BIN) $(FAT_DUMP_BIN) $(BUILD)/fdisk_fixture.bin $(foreach m,CHS SIGNATURE ACTIVE CONFIRM LOADED,$(BUILD)/fdisk_$(m).bin)
	@set -eu; for m in CHS SIGNATURE ACTIVE CONFIRM LOADED; do \
	 case $$m in CHS|SIGNATURE) why='RED fdisk-emu CHS/signature create MBR vs mpartition';; ACTIVE) why='RED fdisk-emu active flag on wrong entry';; CONFIRM) why='RED fdisk-emu delete without confirmation';; LOADED) why='RED fdisk-emu loaded volume was not refused';; esac; \
	 if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/fdisk_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/fdisk_$$m.bin $$m > $(BUILD)/fdisk-emu-mut-$$m.log 2>&1; then echo "FAIL fdisk-emu $$m survived"; exit 1; fi; \
	 grep -Fx "$$why" $(BUILD)/fdisk-emu-mut-$$m.log || { cat $(BUILD)/fdisk-emu-mut-$$m.log; exit 1; }; done
	@echo 'GREEN test-fdisk-emu-mutant: CHS SIGNATURE ACTIVE CONFIRM LOADED rejected'
TEST_EMU_GATES += test-fdisk-emu test-fdisk-emu-mutant

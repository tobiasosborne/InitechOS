# Oracle and emu definitions authored before the implementation (Rule 1).
DU_FLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -I$(BUILD)
DU_HOST := harness/diff/fat_diff/formatsys_host.c
DU_SRC := os/milton/diskutil.c
DU_MUTANTS := MEDIA FAT0 COMMAND ORDER COMPARE RUNNING RESERVE LABEL
$(BUILD)/formatsys_check: harness/diff/fat_diff/formatsys_check.c | $(BUILD)
	$(CC) $(DU_FLAGS) $< -o $@
$(BUILD)/formatsys_host: $(DU_HOST) $(DU_SRC) os/milton/block_device.c os/milton/diskutil.h spec/drive_letters.h $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) $(DU_FLAGS) -DBLOCK_DEVICE_HOST $(DU_HOST) $(DU_SRC) os/milton/block_device.c -o $@
.PHONY: test-formatsys test-formatsys-mutant test-formatsys-emu test-formatsys-emu-mutant test-format-system-boot test-diskcopy-emu
test-formatsys: $(BUILD)/formatsys_host $(BUILD)/formatsys_check $(SECONDARY_TRACER_IMG) $(BUILD)/transient_flairtenants.bin
	@sh harness/diff/fat_diff/formatsys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/formatsys_host

define du-mutant
$(BUILD)/formatsys_host_$(1): $(DU_HOST) $(DU_SRC) os/milton/block_device.c spec/drive_letters.h | $(BUILD)
	$$(CC) $$(DU_FLAGS) -DBLOCK_DEVICE_HOST -DDU_MUT_$(1) $$(DU_HOST) $$(DU_SRC) os/milton/block_device.c -o $$@
$(BUILD)/diskutil_$(1).o: $(DU_SRC) os/milton/diskutil.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) -Os -DDU_MUT_$(1) -Ispec -Ios/milton -c $$< -o $$@
$(BUILD)/format_$(1).elf: $(BUILD)/diskutil_start.o $(BUILD)/format_adapter.o $(BUILD)/diskutil_$(1).o os/milton/fdisk.ld
	$$(LD) -m elf_i386 -T os/milton/fdisk.ld -o $$@ $$(filter %.o,$$^)
$(BUILD)/format_$(1).bin: $(BUILD)/format_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
endef
$(foreach m,$(DU_MUTANTS),$(eval $(call du-mutant,$(m))))
test-formatsys-mutant: $(BUILD)/formatsys_check $(SECONDARY_TRACER_IMG) $(BUILD)/transient_flairtenants.bin $(foreach m,$(DU_MUTANTS),$(BUILD)/formatsys_host_$(m))
	@set -eu; for m in $(DU_MUTANTS); do \
	 case $$m in MEDIA) why='RED formatsys BPB media byte';; FAT0) why='RED formatsys FAT[0] media/EOC';; COMMAND) why='RED formatsys /S COMMAND.COM';; ORDER) why='RED formatsys root entry order';; COMPARE) why='RED formatsys DISKCOMP missing differing sector';; RUNNING) why='RED formatsys running volume not refused';; RESERVE) why='RED formatsys /B reserve bytes';; LABEL) why='RED formatsys /V repeat label fsck';; esac; \
	 if sh harness/diff/fat_diff/formatsys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/formatsys_host_$$m > $(BUILD)/formatsys-mut-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep -Fx "$$why" $(BUILD)/formatsys-mut-$$m.log || { cat $(BUILD)/formatsys-mut-$$m.log; exit 1; }; done
	@echo 'GREEN test-formatsys-mutant: MEDIA FAT0 COMMAND ORDER COMPARE RUNNING RESERVE LABEL rejected'
$(BUILD)/diskutil_start.o: os/milton/diskutil_start.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/diskutil.o: $(DU_SRC) os/milton/diskutil.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Os -Ispec -Ios/milton -c $< -o $@
define du-program
$(BUILD)/$(1)_adapter.o: os/milton/diskutil_program.c os/milton/diskutil.h spec/drive_letters.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) -Os -DDU_KIND=$(2) -Ispec -Ios/milton -c $$< -o $$@
$(BUILD)/$(1)_program.elf: $(BUILD)/diskutil_start.o $(BUILD)/$(1)_adapter.o $(BUILD)/diskutil.o os/milton/fdisk.ld
	$$(LD) -m elf_i386 -T os/milton/fdisk.ld -o $$@ $$(filter %.o,$$^)
$(BUILD)/$(1)_program.bin: $(BUILD)/$(1)_program.elf
	$$(OBJCOPY) -O binary $$< $$@
endef
$(eval $(call du-program,format,0))
$(eval $(call du-program,sys,1))
$(eval $(call du-program,diskcopy,2))
$(eval $(call du-program,diskcomp,3))
DU_BINS = $(FORMAT_PROG_BIN) $(SYS_PROG_BIN) $(DISKCOPY_PROG_BIN) $(DISKCOMP_PROG_BIN)
test-formatsys-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(CONSOLE_TEXT_BIN) $(DU_BINS) $(BUILD)/formatsys_check
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN)
test-formatsys-emu-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(CONSOLE_TEXT_BIN) $(DU_BINS) $(BUILD)/formatsys_check $(foreach m,MEDIA FAT0 COMMAND ORDER RUNNING,$(BUILD)/format_$(m).bin)
	@set -eu; for m in MEDIA FAT0 COMMAND ORDER RUNNING; do \
	 case $$m in MEDIA) why='RED formatsys BPB media byte';; FAT0) why='RED formatsys FAT[0] media/EOC';; COMMAND) why='RED formatsys /S COMMAND.COM';; ORDER) why='RED formatsys root entry order';; RUNNING) why='RED formatsys-emu running volume not refused';; esac; \
	 if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/format_$$m.bin $$m > $(BUILD)/formatsys-emu-mut-$$m.log 2>&1; then echo "FAIL emu $$m survived"; exit 1; fi; \
	 grep -Fx "$$why" $(BUILD)/formatsys-emu-mut-$$m.log || { cat $(BUILD)/formatsys-emu-mut-$$m.log; exit 1; }; done
	@echo 'GREEN test-formatsys-emu-mutant: MEDIA FAT0 COMMAND ORDER RUNNING rejected'
test-format-system-boot: test-formatsys-emu
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN) boot
test-diskcopy-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(DISKCOPY_PROG_BIN) $(DISKCOMP_PROG_BIN)
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/diskcopy_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG)
TEST_UNIT_GATES += test-formatsys test-formatsys-mutant
# PM 2026-10-17: test-diskcopy-emu is a named PREFLIGHT until the harness can swap the
# --disk2 medium through QMP at the 'Insert TARGET diskette' prompt (lane formatsys stopped
# there honestly); a known-red gate never sits in the certificate vector (gate honesty).
TEST_EMU_GATES += test-formatsys-emu test-formatsys-emu-mutant test-format-system-boot

$(BUILD)/gen_drive_letters: tools/gen_drive_letters.c | $(BUILD)
	$(CC) $(DU_FLAGS) $< -o $@
.PHONY: test-drive-letters-spec test-drive-letters-spec-mutant test-drive-letters test-drive-letters-mutant
test-drive-letters-spec: $(BUILD)/gen_drive_letters spec/drive_letters.json spec/drive_letters.h
	@$< spec/drive_letters.json > $(BUILD)/drive_letters.h
	@cmp spec/drive_letters.h $(BUILD)/drive_letters.h
	@echo 'GREEN test-drive-letters-spec: JSON/header equal'
test-drive-letters-spec-mutant: $(BUILD)/gen_drive_letters
	@sed 's/"a_disk": 129/"a_disk": 128/' spec/drive_letters.json > $(BUILD)/drive_letters_mut.json
	@$< $(BUILD)/drive_letters_mut.json > $(BUILD)/drive_letters_mut.h
	@if cmp -s spec/drive_letters.h $(BUILD)/drive_letters_mut.h; then echo 'FAIL drive letters spec mutant survived'; exit 1; fi
	@echo 'RED drive-letters SPEC A disk drift'
	@echo 'GREEN test-drive-letters-spec-mutant: A disk drift rejected'
$(BUILD)/test_drive_letters: harness/diff/fat_diff/test_drive_letters.c os/milton/block_device.c spec/drive_letters.h | $(BUILD)
	$(CC) $(DU_FLAGS) -DBLOCK_DEVICE_HOST -no-pie $(filter %.c,$^) -o $@
test-drive-letters: $(BUILD)/test_drive_letters
	@$<
define dl-mutant
$(BUILD)/test_drive_letters_$(1): harness/diff/fat_diff/test_drive_letters.c os/milton/block_device.c spec/drive_letters.h | $(BUILD)
	$$(CC) $$(DU_FLAGS) -DBLOCK_DEVICE_HOST -no-pie -DDRIVE_MUT_$(1) $$(filter %.c,$$^) -o $$@
endef
$(foreach m,DISK RUNNING,$(eval $(call dl-mutant,$(m))))
test-drive-letters-mutant: $(BUILD)/test_drive_letters_DISK $(BUILD)/test_drive_letters_RUNNING
	@set -eu; for m in DISK RUNNING; do \
	 case $$m in DISK) why='RED drive-letters A physical identity';; RUNNING) why='RED drive-letters running pair';; esac; \
	 if $(BUILD)/test_drive_letters_$$m > $(BUILD)/drive-letters-mut-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep -Fx "$$why" $(BUILD)/drive-letters-mut-$$m.log; done
	@echo 'GREEN test-drive-letters-mutant: DISK RUNNING rejected'
$(KERNEL_BLOCK_DEVICE_OBJ): spec/drive_letters.h os/milton/diskutil.h
TEST_UNIT_GATES += test-drive-letters-spec test-drive-letters-spec-mutant test-drive-letters test-drive-letters-mutant
test-spec: test-drive-letters-spec

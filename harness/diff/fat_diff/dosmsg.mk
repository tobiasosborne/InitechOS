# initech-vlhp/nstg. Gates authored before implementation, no shipped utility.
# int21.c now consumes the typed disk-driver error contract.
$(BUILD)/test_dos_break $(BUILD)/test_dos_break_mut $(BUILD)/int21_dosbreak_mut.o $(BUILD)/int21_dosmsg_mut.o: $(MILTON_DIR)/ata.h
$(BUILD)/test_qemu_paths: harness/emu/test_qemu_paths.c harness/emu/qemu.c harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD)/test_qemu_paths_mut: harness/emu/test_qemu_paths.c harness/emu/qemu.c harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) -DQEMU_MUTATE_PATH_PREFLIGHT $< -o $@
$(BUILD)/test_dos_break: harness/diff/fat_diff/test_dos_break.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Ibuild $< $(TEST_CONIN_DEPS) -o $@
$(BUILD)/test_dos_break_mut: harness/diff/fat_diff/test_dos_break.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_DISK_BREAK -Ispec -I$(MILTON_DIR) -Ibuild $< $(TEST_CONIN_DEPS) -o $@
$(BUILD)/dos_break_program.bin: harness/diff/fat_diff/dos_break_program.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/int21_dosbreak_mut.o: $(KERNEL_INT21_C) $(TEST_CONIN_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_INT21_OPT) -DINT21_MUTATE_DISK_BREAK -Ispec -I$(MILTON_DIR) -Ibuild -c $< -o $@
$(eval $(call j1lu-emu-variant,dosbreak,$(KERNEL_INT21_OBJ),$(BUILD)/int21_dosbreak_mut.o))
.PHONY: test-qemu-paths test-qemu-paths-mutant test-dos-break test-dos-break-mutant test-dos-break-emu test-dos-break-emu-mutant
test-qemu-paths: $(BUILD)/test_qemu_paths
	@$<
test-qemu-paths-mutant: $(BUILD)/test_qemu_paths_mut
	@if $< >$(BUILD)/qemu_paths_mut.log 2>&1; then echo 'FAIL QEMU_MUTATE_PATH_PREFLIGHT survived'; exit 1; fi
	@grep '^RED qemu-paths: QMP socket path too long must fail loudly BEFORE launch' $(BUILD)/qemu_paths_mut.log
	@echo 'GREEN test-qemu-paths-mutant: QEMU_MUTATE_PATH_PREFLIGHT caught RED'
test-dos-break: $(BUILD)/test_dos_break
	@$<
test-dos-break-mutant: $(BUILD)/test_dos_break_mut
	@if $< >$(BUILD)/dos_break_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_DISK_BREAK survived'; exit 1; fi
	@grep '^RED dos-break: ON must abort non-console call before service' $(BUILD)/dos_break_mut.log
	@echo 'GREEN test-dos-break-mutant: INT21_MUTATE_DISK_BREAK caught RED'
test-dos-break-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/dos_break_program.bin $(BUILD)/console_text
	@sh harness/diff/fat_diff/dos_break.sh $(SECONDARY_TRACER_IMG)
test-dos-break-emu-mutant: $(HARNESS_BIN) $(BUILD)/j1lu_dosbreak.img $(BUILD)/dos_break_program.bin $(BUILD)/console_text
	@if sh harness/diff/fat_diff/dos_break.sh $(BUILD)/j1lu_dosbreak.img mutant >$(BUILD)/dos_break_emu_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_DISK_BREAK survived QEMU'; exit 1; fi
	@grep '^RED dos-break-emu: on did not return after disk Ctrl-C' $(BUILD)/dos_break_emu_mut.log
	@echo 'GREEN test-dos-break-emu-mutant: INT21_MUTATE_DISK_BREAK caught RED'
TEST_UNIT_GATES += test-qemu-paths test-qemu-paths-mutant test-dos-break test-dos-break-mutant
TEST_EMU_GATES += test-dos-break-emu test-dos-break-emu-mutant

$(BUILD)/test_dos_not_ready: harness/diff/fat_diff/test_dos_not_ready.c $(TEST_INT24_WIRED_DEPS) $(TEST_INT24_WIRED_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -I$(FAT_DIFF_DIR) -Ibuild $< $(TEST_INT24_WIRED_DEPS) -o $@
$(BUILD)/test_dos_not_ready_mut: harness/diff/fat_diff/test_dos_not_ready.c $(TEST_INT24_WIRED_DEPS) $(TEST_INT24_WIRED_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_MUTATE_NOT_READY -Ispec -I$(MILTON_DIR) -I$(FAT_DIFF_DIR) -Ibuild $< $(TEST_INT24_WIRED_DEPS) -o $@
$(BUILD)/dosmsg_fault.o: harness/diff/fat_diff/dosmsg_fault.c $(MILTON_DIR)/ata.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -I$(MILTON_DIR) -c $< -o $@
$(BUILD)/kmain_dosmsg.o: $(KERNEL_MAIN_C) $(KERNEL_DIR)/command.h $(KERNEL_DIR)/sysinit.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DBOOT_SHELL -DBOOT_SECONDARY_SHELL -DBOOT_DOSMSG_FAULT -Ispec -I$(MILTON_DIR) -c $< -o $@
$(BUILD)/int21_dosmsg_mut.o: $(KERNEL_INT21_C) $(TEST_CONIN_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_INT21_OPT) -DINT21_MUTATE_NOT_READY -Ispec -I$(MILTON_DIR) -Ibuild -c $< -o $@
DOSMSG_OBJS := $(filter-out $(KERNEL_SECONDARY_SHELL_MAIN_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/kmain_dosmsg.o $(BUILD)/dosmsg_fault.o
$(BUILD)/dosmsg.elf: $(DOSMSG_OBJS) $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(DOSMSG_OBJS)
$(BUILD)/dosmsg_mut.elf: $(filter-out $(KERNEL_INT21_OBJ),$(DOSMSG_OBJS)) $(BUILD)/int21_dosmsg_mut.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_INT21_OBJ),$(DOSMSG_OBJS)) $(BUILD)/int21_dosmsg_mut.o
$(BUILD)/dosmsg.bin $(BUILD)/dosmsg_mut.bin: %.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
$(BUILD)/dosmsg.img $(BUILD)/dosmsg_mut.img: %.img: %.bin $(MBR_BIN) $(STAGE2_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $< $(BOOT_SYSTEM_TOOL)
.PHONY: test-dos-not-ready test-dos-not-ready-mutant test-dos-not-ready-emu test-dos-not-ready-emu-mutant
test-dos-not-ready: $(BUILD)/test_dos_not_ready
	@mkdir -p $(BUILD)/dos-not-ready-host
	@dd if=/dev/zero of=$(BUILD)/dos-not-ready-host/data.img bs=512 count=2880 status=none
	@mformat -i $(BUILD)/dos-not-ready-host/data.img -f 1440 -N 102 ::
	@cp -f $(BUILD)/dos-not-ready-host/data.img $(BUILD)/dos-not-ready-host/reference.img
	@$< $(BUILD)/dos-not-ready-host/data.img
	@cmp $(BUILD)/dos-not-ready-host/data.img $(BUILD)/dos-not-ready-host/reference.img
	@fsck.fat -n $(BUILD)/dos-not-ready-host/data.img >$(BUILD)/dos-not-ready-host/fsck.log
	@echo 'GREEN test-dos-not-ready: identical mformat image + dosfstools clean'
test-dos-not-ready-mutant: test-dos-not-ready $(BUILD)/test_dos_not_ready_mut
	@if $(BUILD)/test_dos_not_ready_mut $(BUILD)/dos-not-ready-host/data.img >$(BUILD)/dos_not_ready_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_NOT_READY survived'; exit 1; fi
	@grep '^RED dos-not-ready: missing exact MSG-DOS-0006' $(BUILD)/dos_not_ready_mut.log
	@echo 'GREEN test-dos-not-ready-mutant: INT21_MUTATE_NOT_READY caught RED'
test-dos-not-ready-emu: $(HARNESS_BIN) $(BUILD)/dosmsg.img $(BUILD)/console_text
	@sh harness/diff/fat_diff/dos_not_ready.sh $(BUILD)/dosmsg.img
test-dos-not-ready-emu-mutant: $(HARNESS_BIN) $(BUILD)/dosmsg_mut.img $(BUILD)/console_text
	@if sh harness/diff/fat_diff/dos_not_ready.sh $(BUILD)/dosmsg_mut.img mutant >$(BUILD)/dos_not_ready_emu_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_NOT_READY survived QEMU'; exit 1; fi
	@grep '^RED dos-not-ready-emu: MSG-DOS-0006 missing' $(BUILD)/dos_not_ready_emu_mut.log
	@echo 'GREEN test-dos-not-ready-emu-mutant: INT21_MUTATE_NOT_READY caught RED'
TEST_UNIT_GATES += test-dos-not-ready test-dos-not-ready-mutant
TEST_EMU_GATES += test-dos-not-ready-emu test-dos-not-ready-emu-mutant

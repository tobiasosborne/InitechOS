# Manual parity lane: all new green gates and mutation proofs join the vectors.
DOS_PARITY_SRC := harness/diff/fat_diff/dos_parity_host.c
$(BUILD)/dos_parity_host: $(DOS_PARITY_SRC) $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -Ibuild -o $@ $< $(TEST_FILEIO_SUBDIR_DEPS)
$(BUILD)/dos_parity_host_mut: $(DOS_PARITY_SRC) $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -DFILEIO_MUTATE_NO_STAMP -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -Ibuild -o $@ $< $(TEST_FILEIO_SUBDIR_DEPS)

$(eval $(call dos-safety-shell-mutant,dirargs,CMD_MUTATE_DIR_ARGS))
$(eval $(call dos-safety-shell-mutant,pathcmd,CMD_MUTATE_PATH_COMMAND))
# 'permanent' selects the real primary processor object vector in the existing
# m0dc/safety image machinery. Use a separate name to retain its EXIT mutant.
$(BUILD)/command_startup_mut.o: $(KERNEL_COMMAND_C) $(KERNEL_DIR)/command.h $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DCOMMAND_KERNEL_REPL -DCMD_MUTATE_STARTUP -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
$(BUILD)/kernel_startup_mut.elf: $(filter-out $(KERNEL_COMMAND_OBJ),$(KERNEL_SHELL_OBJS)) $(BUILD)/command_startup_mut.o $(KERNEL_LD)
	$(LD) $(KERNEL_GC_LDFLAGS) -m elf_i386 -T $(KERNEL_LD) -o $@ $(filter-out $(KERNEL_COMMAND_OBJ),$(KERNEL_SHELL_OBJS)) $(BUILD)/command_startup_mut.o
$(BUILD)/kernel_startup_mut.bin: $(BUILD)/kernel_startup_mut.elf
	$(OBJCOPY) -O binary $< $@
	@test $$(wc -c < $@) -le $$(( $(KERNEL_SECTORS) * 512 ))
	@truncate -s $$(( $(KERNEL_SECTORS) * 512 )) $@
$(BUILD)/tracer_startup_mut.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_startup_mut.bin
	@dd if=/dev/zero of=$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(BUILD)/kernel_startup_mut.bin of=$@ bs=512 seek=17 conv=notrunc status=none

$(BUILD)/fileio_stamp_mut.o: $(KERNEL_DIR)/fileio_fat.c $(KERNEL_DIR)/fileio_fat.h $(KERNEL_DIR)/int21.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FILEIO_FAT_OPT) -DFILEIO_MUTATE_NO_STAMP -Ispec -I$(KERNEL_DIR) -c $< -o $@
$(eval $(call j1lu-emu-variant,stamp,$(KERNEL_FILEIO_OBJ),$(BUILD)/fileio_stamp_mut.o))

.PHONY: test-dos-stamp-host test-dos-stamp-mutant test-dos-stamp test-dos-stamp-emu-mutant test-dir-args test-dir-page test-dir-args-mutant test-path-command test-path-command-mutant test-dos-startup test-dos-startup-mutant
test-dos-stamp-host: $(BUILD)/dos_parity_host
	@sh harness/diff/fat_diff/dos_parity.sh stamp-host
test-dos-stamp-mutant: $(BUILD)/dos_parity_host_mut
	@DOS_PARITY_HOST=$(BUILD)/dos_parity_host_mut sh harness/diff/fat_diff/dos_parity.sh stamp-host unused mutant > $(BUILD)/dos_stamp_mutant.log 2>&1; rc=$$?; \
	[ $$rc -ne 0 ] && grep 'stamp-host RED:.*stamp' $(BUILD)/dos_stamp_mutant.log || { cat $(BUILD)/dos_stamp_mutant.log; exit 1; }; \
	printf 'PASS stamp mutant: FILEIO_MUTATE_NO_STAMP caught RED\n'
test-dos-stamp-emu-mutant: $(HARNESS_BIN) $(BUILD)/j1lu_stamp.img $(BUILD)/dos_parity_host
	@if sh harness/diff/fat_diff/dos_parity.sh stamp $(BUILD)/j1lu_stamp.img mutant > $(BUILD)/dos_stamp_emu_mutant.log 2>&1; then echo 'FAIL: stamp emulator mutant survived'; exit 1; fi
	@grep 'stamp RED: NEW.TXT stamp' $(BUILD)/dos_stamp_emu_mutant.log
	@printf 'PASS stamp emulator mutant: FILEIO_MUTATE_NO_STAMP caught RED\n'
test-dos-stamp test-dir-args test-path-command: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(GREET_PROG_BIN) $(BUILD)/dos_parity_host
	@sh harness/diff/fat_diff/dos_parity.sh $(if $(filter test-dos-stamp,$@),stamp,$(if $(filter test-dir-args,$@),dir,path)) $(SECONDARY_TRACER_IMG)
test-dir-page: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/dos_parity.sh page $(SECONDARY_TRACER_IMG)
test-dir-args-mutant: $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_dirargs.img
	@if sh harness/diff/fat_diff/dos_parity.sh dir $(BUILD)/tracer_mut_safety_dirargs.img mutant > $(BUILD)/dir_args_mutant.log 2>&1; then echo 'FAIL: DIR mutant survived'; exit 1; fi
	@grep 'dir RED: DIR B: ignored its drive' $(BUILD)/dir_args_mutant.log
	@printf 'PASS DIR mutant: CMD_MUTATE_DIR_ARGS caught RED\n'
test-path-command-mutant: $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_pathcmd.img $(GREET_PROG_BIN)
	@if sh harness/diff/fat_diff/dos_parity.sh path $(BUILD)/tracer_mut_safety_pathcmd.img mutant > $(BUILD)/path_command_mutant.log 2>&1; then echo 'FAIL: PATH mutant survived'; exit 1; fi
	@grep 'path RED: bare PATH / PATH ;' $(BUILD)/path_command_mutant.log
	@printf 'PASS PATH mutant: CMD_MUTATE_PATH_COMMAND caught RED\n'
test-dos-startup: $(HARNESS_BIN) $(TRACER_IMG)
	@sh harness/diff/fat_diff/dos_parity.sh startup $(TRACER_IMG)
	@sh harness/diff/fat_diff/dos_parity.sh startup-set $(TRACER_IMG)
	@sh harness/diff/fat_diff/dos_parity.sh startup-autoexec $(TRACER_IMG)
test-dos-startup-mutant: $(HARNESS_BIN) $(BUILD)/tracer_startup_mut.img
	@if sh harness/diff/fat_diff/dos_parity.sh startup $(BUILD)/tracer_startup_mut.img mutant > $(BUILD)/dos_startup_mutant.log 2>&1; then echo 'FAIL: startup mutant survived'; exit 1; fi
	@grep 'startup RED: startup date/prompt missing' $(BUILD)/dos_startup_mutant.log
	@printf 'PASS startup mutant: CMD_MUTATE_STARTUP caught RED\n'
TEST_UNIT_GATES += test-dos-stamp-host test-dos-stamp-mutant
TEST_EMU_GATES += test-dos-stamp test-dos-stamp-emu-mutant test-dir-args test-dir-page test-dir-args-mutant test-path-command test-path-command-mutant test-dos-startup test-dos-startup-mutant

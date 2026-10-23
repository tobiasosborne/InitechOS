# dos-shell-cmds: host and injected QEMU oracles authored before behavior.
SHELL_FILES_DIR := harness/diff/fat_diff
$(MAKEFILE_STAMP): harness/diff/fat_diff/shell_files.mk
SHELL_FILES_SRC := $(SHELL_FILES_DIR)/shell_files.c
SHELL_FILES_DEPS := $(SHELL_FILES_SRC) $(SHELL_FILES_DIR)/shell_files_host.h os/milton/command.c os/milton/command.h $(DOS_MESSAGES_H)
$(BUILD)/shell_files: $(SHELL_FILES_DEPS) $(TEST_FILEIO_SUBDIR_DEPS)
	$(CC) $(CFLAGS) -Wno-unused-function -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections -Ispec -Ios/milton -Ibuild -I$(SHELL_FILES_DIR) $(SHELL_FILES_SRC) $(TEST_FILEIO_SUBDIR_DEPS) -o $@
.PHONY: test-shell-files-host test-shell-files-emu
test-shell-files-host: $(BUILD)/shell_files
	@sh $(SHELL_FILES_DIR)/shell_files.sh host $(BUILD)/shell_files
test-shell-files-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh $(SHELL_FILES_DIR)/shell_files.sh emu $(SECONDARY_TRACER_IMG)
TEST_UNIT_GATES += test-shell-files-host
TEST_EMU_GATES += test-shell-files-emu
.PHONY: test-command-c-emu test-xcopy-cwd-host test-xcopy-cwd-emu
test-command-c-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh $(SHELL_FILES_DIR)/command_c.sh $(SECONDARY_TRACER_IMG)
test-xcopy-cwd-host: $(ZUVO_HOST)
	@sh $(SHELL_FILES_DIR)/xcopy_cwd.sh host $(ZUVO_HOST)
test-xcopy-cwd-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN)
	@sh $(SHELL_FILES_DIR)/xcopy_cwd.sh emu $(SECONDARY_TRACER_IMG)
TEST_UNIT_GATES += test-xcopy-cwd-host
TEST_EMU_GATES += test-command-c-emu test-xcopy-cwd-emu
$(BUILD)/dir_abi: $(SHELL_FILES_DIR)/dir_abi.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-dir-abi test-dir-abi-mutant
test-dir-abi: $(BUILD)/dir_abi
	@$< spec/int21h_calling_convention.json
test-dir-abi-mutant: $(BUILD)/dir_abi
	@set -eu; for ah in 39 3A 3B 47 40; do \
	 sed "s/\"ah\": \"$${ah}h\"/\"ah\": \"FFh\"/" spec/int21h_calling_convention.json > $(BUILD)/dir_abi_$$ah.json; \
	 if $< $(BUILD)/dir_abi_$$ah.json >$(BUILD)/dir_abi_$$ah.log 2>&1; then echo "FAIL ABI_$$ah survived";exit 1;fi; \
	 grep "^RED dir-abi: $${ah}h row missing" $(BUILD)/dir_abi_$$ah.log; done
	@sed 's/ECX=0/ECX=1/g' spec/int21h_calling_convention.json > $(BUILD)/dir_abi_40_zero.json
	@if $< $(BUILD)/dir_abi_40_zero.json >$(BUILD)/dir_abi_40_zero.log 2>&1; then echo 'FAIL ABI_40_ZERO survived';exit 1;fi
	@grep '^RED dir-abi: 40h zero-count size contract' $(BUILD)/dir_abi_40_zero.log
	@echo 'GREEN test-dir-abi-mutant: ABI_39 ABI_3A ABI_3B ABI_47 ABI_40 ABI_40_ZERO caught RED'
TEST_UNIT_GATES += test-dir-abi test-dir-abi-mutant
SHELL_FILES_MUTANTS := CMD_MUTATE_REN_ENUM CMD_MUTATE_REN_MAP CMD_MUTATE_COPY_PLUS CMD_MUTATE_COPY_ASCII CMD_MUTATE_COPY_EOF CMD_MUTATE_COPY_SWITCH CMD_MUTATE_COPY_VERIFY CMD_MUTATE_COPY_WILD FAT12_MUTATE_CREATE_CHARS INT21_MUTATE_MKDIR_DEVICE
$(BUILD)/shell_files_mut_%: $(SHELL_FILES_DEPS) $(TEST_FILEIO_SUBDIR_DEPS)
	$(CC) $(CFLAGS) -Wno-unused-function -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections -D$* -Ispec -Ios/milton -Ibuild -I$(SHELL_FILES_DIR) $(SHELL_FILES_SRC) $(TEST_FILEIO_SUBDIR_DEPS) -o $@
.PHONY: test-shell-files-host-mutant test-shell-files-emu-mutant test-command-c-emu-mutant test-xcopy-cwd-host-mutant test-xcopy-cwd-emu-mutant
test-shell-files-host-mutant: $(addprefix $(BUILD)/shell_files_mut_,$(SHELL_FILES_MUTANTS))
	@set -eu; for m in $(SHELL_FILES_MUTANTS); do \
	 if sh $(SHELL_FILES_DIR)/shell_files.sh host $(BUILD)/shell_files_mut_$$m $$m > $(BUILD)/shell_files_$$m.log 2>&1; then echo "FAIL $$m survived";exit 1;fi; \
	 grep '^RED shell-files:' $(BUILD)/shell_files_$$m.log; \
	 echo "GREEN test-shell-files-host-mutant: $$m caught RED";done
$(eval $(call dos-safety-shell-mutant,doscmds_ren,CMD_MUTATE_REN_MAP))
$(eval $(call dos-safety-shell-mutant,doscmds_wild,CMD_MUTATE_COPY_WILD))
$(eval $(call dos-safety-shell-mutant,command_c,CMD_MUTATE_COMMAND_C))
$(eval $(call dos-safety-shell-mutant,command_env,CMD_MUTATE_COMMAND_ENV))
test-shell-files-emu-mutant: $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_doscmds_ren.img $(BUILD)/tracer_mut_safety_doscmds_wild.img
	@set -eu; for m in ren wild; do \
	 if sh $(SHELL_FILES_DIR)/shell_files.sh emu $(BUILD)/tracer_mut_safety_doscmds_$$m.img $$m >$(BUILD)/shell_files_emu_$$m.log 2>&1; then echo "FAIL $$m survived QEMU";exit 1;fi; \
	 grep '^RED shell-files:' $(BUILD)/shell_files_emu_$$m.log; \
	 grep -q SHELL-FILES-DONE $(BUILD)/shell-files-emu-$$m/text; \
	 echo "GREEN test-shell-files-emu-mutant: $$m booted and caught RED";done
test-command-c-emu-mutant: $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_command_c.img $(BUILD)/tracer_mut_safety_command_env.img
	@set -eu; for m in c env; do \
	 if sh $(SHELL_FILES_DIR)/command_c.sh $(BUILD)/tracer_mut_safety_command_$$m.img $$m >$(BUILD)/command_c_mut_$$m.log 2>&1; then echo "FAIL COMMAND_$$m survived";exit 1;fi; \
	 grep '^RED command-c:' $(BUILD)/command_c_mut_$$m.log; \
	 echo "GREEN test-command-c-emu-mutant: COMMAND_$$m caught RED";done
$(BUILD)/test_xcopy_cwd_mut: $(ZUVO_HOST_SRC) os/milton/xcopy_tree.h os/milton/dospath.h $(DOS_MESSAGES_H)
	$(CC) $(CFLAGS) -no-pie $(ZUVO_INC) -DDU_MUT_REL_ROOT -o $@ $(ZUVO_HOST_SRC)
$(eval $(call zuvo-com,xcopy_cwd_mut,-DDU_MUT_REL_ROOT,))
test-xcopy-cwd-host-mutant: $(BUILD)/test_xcopy_cwd_mut
	@if sh $(SHELL_FILES_DIR)/xcopy_cwd.sh host $< mutant >$(BUILD)/xcopy_cwd_host_mut.log 2>&1; then echo 'FAIL DU_MUT_REL_ROOT survived';exit 1;fi
	@grep '^RED xcopy-cwd: relative source selected root decoy' $(BUILD)/xcopy_cwd_host_mut.log
	@echo 'GREEN test-xcopy-cwd-host-mutant: DU_MUT_REL_ROOT caught RED'
test-xcopy-cwd-emu-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/xcopy_cwd_mut_program.bin
	@if XCOPY_CWD_PROGRAM=$(BUILD)/xcopy_cwd_mut_program.bin sh $(SHELL_FILES_DIR)/xcopy_cwd.sh emu $(SECONDARY_TRACER_IMG) mutant >$(BUILD)/xcopy_cwd_emu_mut.log 2>&1; then echo 'FAIL DU_MUT_REL_ROOT survived QEMU';exit 1;fi
	@grep '^RED xcopy-cwd: relative source selected root decoy' $(BUILD)/xcopy_cwd_emu_mut.log
	@echo 'GREEN test-xcopy-cwd-emu-mutant: DU_MUT_REL_ROOT booted and caught RED'
TEST_UNIT_GATES += test-shell-files-host-mutant test-xcopy-cwd-host-mutant
TEST_EMU_GATES += test-shell-files-emu-mutant test-command-c-emu-mutant test-xcopy-cwd-emu-mutant

# SETFILE-size prerequisite for COPY A+B. IBM DOS 3.30 TR p.6-140.
$(BUILD)/truncate: $(SHELL_FILES_DIR)/truncate.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS)
	$(CC) $(CFLAGS) -no-pie -Ispec -Ios/milton -Ibuild -I$(SHELL_FILES_DIR) $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
$(BUILD)/truncate_mut: $(SHELL_FILES_DIR)/truncate.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS)
	$(CC) $(CFLAGS) -no-pie -DFAT12_MUTATE_TRUNCATE_NO_FREE -Ispec -Ios/milton -Ibuild -I$(SHELL_FILES_DIR) $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
$(BUILD)/truncate_program.bin: $(SHELL_FILES_DIR)/truncate_program.asm
	$(NASM) -f bin $< -o $@
$(BUILD)/fat12_mut_truncate.o: $(KERNEL_FAT12_C) $(KERNEL_DIR)/fat12.h spec/dos_structs.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FAT12_OPT) -DFAT12_MUTATE_TRUNCATE_NO_FREE -Ispec -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/kernel_shell_mut_truncate.elf: $(filter-out $(KERNEL_FAT12_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fat12_mut_truncate.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_FAT12_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fat12_mut_truncate.o
$(BUILD)/kernel_shell_mut_truncate.bin: $(BUILD)/kernel_shell_mut_truncate.elf
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	 dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$((max - sz))" conv=notrunc status=none
$(BUILD)/tracer_mut_truncate.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_shell_mut_truncate.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_shell_mut_truncate.bin $(BOOT_SYSTEM_TOOL)
.PHONY: test-fat-truncate-host test-fat-truncate-host-mutant test-fat-truncate-emu test-fat-truncate-emu-mutant
test-fat-truncate-host: $(BUILD)/truncate
	@sh $(SHELL_FILES_DIR)/truncate.sh host $<
test-fat-truncate-emu: $(BUILD)/truncate $(BUILD)/truncate_program.bin $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh $(SHELL_FILES_DIR)/truncate.sh emu $(SECONDARY_TRACER_IMG)
test-fat-truncate-host-mutant: $(BUILD)/truncate $(BUILD)/truncate_mut
	@if sh $(SHELL_FILES_DIR)/truncate.sh host $(BUILD)/truncate_mut mutant >$(BUILD)/truncate_host_mut.log 2>&1; then echo 'FAIL FAT12_MUTATE_TRUNCATE_NO_FREE survived';exit 1;fi
	@grep '^RED truncate: released clusters != mtools' $(BUILD)/truncate_host_mut.log
	@echo 'GREEN test-fat-truncate-host-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE caught RED'
test-fat-truncate-emu-mutant: $(BUILD)/truncate $(BUILD)/truncate_program.bin $(HARNESS_BIN) $(BUILD)/tracer_mut_truncate.img
	@if sh $(SHELL_FILES_DIR)/truncate.sh emu $(BUILD)/tracer_mut_truncate.img mutant >$(BUILD)/truncate_emu_mut.log 2>&1; then echo 'FAIL FAT12_MUTATE_TRUNCATE_NO_FREE survived QEMU';exit 1;fi
	@grep '^RED truncate: released clusters != mtools' $(BUILD)/truncate_emu_mut.log
	@grep -qx TRUNCATE-PASS $(BUILD)/truncate-emu-mutant/text
	@echo 'GREEN test-fat-truncate-emu-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE booted and caught RED'
TEST_UNIT_GATES += test-fat-truncate-host test-fat-truncate-host-mutant
TEST_EMU_GATES += test-fat-truncate-emu test-fat-truncate-emu-mutant

# initech-66oc: redirection and pipes on a full or protected volume (audit
# 2026-10-23-dos L001/L002/L003/L004/L027). Two inline-minted A: volumes, each
# checked against its declared contract before boot (initech-a5sr pattern).
# MS-DOS 3.3 User's Guide pp.18-20 + App. F pp.334/337; IBM DOS 3.30 TR
# pp.6-139/6-140 (AH=40h short count) and p.6-215 (AH=5Bh).
REDIR_FULL_MUTANTS := clobber:CMD_MUTATE_PIPE_CLOBBER:L001 badcreate:CMD_MUTATE_REDIR_CREATE_BADCMD:L027 ignorefull:CMD_MUTATE_REDIR_IGNORE_FULL:L002 pipefull:CMD_MUTATE_PIPE_IGNORE_FULL:L003 copyfull:CMD_MUTATE_COPY_FULL_BADCMD:L004
$(foreach m,$(REDIR_FULL_MUTANTS),$(eval $(call dos-safety-shell-mutant,redirfull_$(word 1,$(subst :, ,$(m))),$(word 2,$(subst :, ,$(m))))))
.PHONY: test-redir-full-emu test-redir-full-emu-mutant
test-redir-full-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SORT_PROG_BIN) $(SHELL_FILES_DIR)/redir_full.sh
	@sh $(SHELL_FILES_DIR)/redir_full.sh $(SECONDARY_TRACER_IMG) $(SORT_PROG_BIN)
test-redir-full-emu-mutant: $(HARNESS_BIN) $(SORT_PROG_BIN) $(SHELL_FILES_DIR)/redir_full.sh $(foreach m,$(REDIR_FULL_MUTANTS),$(BUILD)/tracer_mut_safety_redirfull_$(word 1,$(subst :, ,$(m))).img)
	@set -eu; for m in $(REDIR_FULL_MUTANTS); do \
	 tag=$${m%%:*}; rest=$${m#*:}; macro=$${rest%%:*}; finding=$${rest#*:}; \
	 if sh $(SHELL_FILES_DIR)/redir_full.sh $(BUILD)/tracer_mut_safety_redirfull_$$tag.img $(SORT_PROG_BIN) $$tag >$(BUILD)/redir_full_mut_$$tag.log 2>&1; then echo "FAIL $$macro survived QEMU";exit 1;fi; \
	 grep "^RED redir-full: $$finding " $(BUILD)/redir_full_mut_$$tag.log; \
	 echo "GREEN test-redir-full-emu-mutant: $$macro booted and caught RED ($$finding)";done
TEST_EMU_GATES += test-redir-full-emu test-redir-full-emu-mutant

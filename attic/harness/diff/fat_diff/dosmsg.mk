# initech-vlhp/nstg. Gates authored before implementation, no shipped utility.
# int21.c now consumes the typed disk-driver error contract.
$(BUILD)/test_dos_break $(BUILD)/test_dos_break_mut $(BUILD)/int21_dosbreak_mut.o $(BUILD)/int21_dosmsg_mut.o: $(MILTON_DIR)/ata.h
$(BUILD)/test_qemu_paths: harness/emu/test_qemu_paths.c harness/emu/qemu.c harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD)/test_qemu_paths_mut: harness/emu/test_qemu_paths.c harness/emu/qemu.c harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) -DQEMU_MUTATE_PATH_PREFLIGHT $< -o $@
$(BUILD)/test_dos_break: harness/diff/fat_diff/test_dos_break.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -Ispec -I$(MILTON_DIR) -Ibuild $< $(TEST_CONIN_DEPS) -o $@
$(BUILD)/test_dos_break_mut: harness/diff/fat_diff/test_dos_break.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -DINT21_MUTATE_DISK_BREAK -Ispec -I$(MILTON_DIR) -Ibuild $< $(TEST_CONIN_DEPS) -o $@
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

# initech-bcg.13: extend test-dosmsg's source-purity tooth, not a parallel lint.
# PM 2026-10-17: os/milton/tpsrep_program.asm is NOT audited here -- every literal it prints is
# LOCKED in spec/tps_ritual.json and linted verbatim by test-tps-report [spec]; the TPS messages
# belong to the TPS Operating Procedures manual (initech-ipu5.14), not the InitechDOS catalogue.
DOSMSG_AUDIT_SRCS := $(addprefix os/milton/,command.c batch.c config_sys.c sysinit.c dosutil.c chkdsk.c xcopy_tree.c edlin.c edlin_program.c find_program.asm more_program.asm sort_program.asm)
DOSMSG_ALLOW := harness/diff/fat_diff/dosmsg.allow
DOSMSG_BASELINE := harness/diff/fat_diff/dosmsg.allow.baseline
DOSMSG_SCAN := $(BUILD)/dosmsg_scan
# This include is parsed after the external-program variables are defined.
test-dosmsg: $(DOSMSG_SCAN) $(DOSMSG_ALLOW) $(DOSMSG_BASELINE) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) $(CHKDSK_PROG_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN) $(EDLIN_PROG_BIN) $(FIND_PROG_BIN) $(SORT_PROG_BIN)
$(DOSMSG_SCAN): harness/diff/fat_diff/dosmsg_scan.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-dosmsg-source-mutant
test-dosmsg-source-mutant: $(DOSMSG_SCAN) $(DOS_MESSAGES_H) $(DOSMSG_ALLOW) $(DOSMSG_BASELINE)
	@sh harness/diff/fat_diff/dosmsg_mutants.sh $(DOSMSG_SCAN) $(DOS_MESSAGES_H) $(DOSMSG_ALLOW) $(DOSMSG_BASELINE) $(DOSMSG_AUDIT_SRCS)
TEST_UNIT_GATES += test-dosmsg-source-mutant

$(BUILD)/dir_match: harness/diff/fat_diff/dir_match.c $(TEST_INT21_DEPS) $(TEST_INT21_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_WILDCARD_TESTSEAM -Ispec -I$(MILTON_DIR) -Ibuild -o $@ $< $(TEST_INT21_DEPS)
$(BUILD)/dir_match_mut: harness/diff/fat_diff/dir_match.c $(TEST_INT21_DEPS) $(TEST_INT21_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DINT21_WILDCARD_TESTSEAM -DINT21_MUTATE_WILD_QMARK_LITERAL -Ispec -I$(MILTON_DIR) -Ibuild -o $@ $< $(TEST_INT21_DEPS)
.PHONY: test-dir-match-host test-dir-match-host-mutant test-dir-match-emu test-dir-match-emu-mutant
test-dir-match-host: $(BUILD)/dir_match
	@sh harness/diff/fat_diff/dir_match.sh host
test-dir-match-host-mutant: $(BUILD)/dir_match_mut
	@if DIR_MATCH_HOST=$(BUILD)/dir_match_mut sh harness/diff/fat_diff/dir_match.sh host unused mutant >$(BUILD)/dir_match_host_mutant.log 2>&1; then echo 'FAIL INT21_MUTATE_WILD_QMARK_LITERAL survived'; exit 1; fi
	@grep '^RED dir-match: \*.TXT host != mdir' $(BUILD)/dir_match_host_mutant.log
	@echo 'GREEN test-dir-match-host-mutant: INT21_MUTATE_WILD_QMARK_LITERAL caught RED'
test-dir-match-emu: $(BUILD)/dir_match $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/dir_match.sh emu $(SECONDARY_TRACER_IMG)
test-dir-match-emu-mutant: $(BUILD)/dir_match $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_dirargs.img
	@if sh harness/diff/fat_diff/dir_match.sh emu $(BUILD)/tracer_mut_safety_dirargs.img mutant >$(BUILD)/dir_match_mutant.log 2>&1; then echo 'FAIL CMD_MUTATE_DIR_ARGS survived'; exit 1; fi
	@grep '^RED dir-match: TXT shell membership != mdir/test-80k matcher' $(BUILD)/dir_match_mutant.log
	@echo 'GREEN test-dir-match-emu-mutant: CMD_MUTATE_DIR_ARGS caught RED'
TEST_UNIT_GATES += test-dir-match-host test-dir-match-host-mutant
TEST_EMU_GATES += test-dir-match-emu test-dir-match-emu-mutant

# initech-c80y: returning resident TYPE/DIR cancellation, independent disk bytes.
$(BUILD)/dos_resident_break_check: harness/diff/fat_diff/dos_resident_break_check.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-dos-resident-break-emu test-dos-resident-break-mutant
test-dos-resident-break-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/console_text $(BUILD)/dos_resident_break_check
	@sh harness/diff/fat_diff/dos_resident_break.sh $(SECONDARY_TRACER_IMG)
TEST_EMU_GATES += test-dos-resident-break-emu

$(BUILD)/test_dos_break_cleanup_mut: harness/diff/fat_diff/test_dos_break.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) $(MILTON_DIR)/ata.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -DINT21_MUTATE_BREAK_CLEANUP -Ispec -I$(MILTON_DIR) -Ibuild $< $(TEST_CONIN_DEPS) -o $@
RESIDENT_BREAK_KNOB_type := CMD_MUTATE_TYPE_BREAK
RESIDENT_BREAK_KNOB_dir := CMD_MUTATE_DIR_BREAK
$(BUILD)/command_resident_break_%.o: $(KERNEL_COMMAND_C) $(KERNEL_DIR)/command.h $(TEST_CONIN_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DCOMMAND_KERNEL_REPL -D$(RESIDENT_BREAK_KNOB_$*) -Ispec -I$(MILTON_DIR) -Ibuild -c $< -o $@
$(foreach m,type dir,$(eval $(call j1lu-emu-variant,resident_$(m),$(KERNEL_COMMAND_OBJ),$(BUILD)/command_resident_break_$(m).o)))
.PHONY: test-dos-break-cleanup-mutant
test-dos-break-cleanup-mutant: $(BUILD)/test_dos_break_cleanup_mut
	@rc=0; $< >$(BUILD)/dos_break_cleanup_mut.log 2>&1 || rc=$$?; test $$rc -eq 1
	@grep '^RED dos-break: cancel cleanup must CLOSE and restore stdin/stdout on every path' $(BUILD)/dos_break_cleanup_mut.log
	@echo 'GREEN test-dos-break-cleanup-mutant: queued Ctrl-C cleanup regression caught'
test-dos-resident-break-mutant: $(HARNESS_BIN) $(BUILD)/console_text $(BUILD)/dos_resident_break_check $(BUILD)/j1lu_resident_type.img $(BUILD)/j1lu_resident_dir.img
	@set -eu; for m in type dir; do \
	  mode=controlshort; test $$m != dir || mode=gaps2; \
	  rc=0; sh harness/diff/fat_diff/dos_resident_break.sh $(BUILD)/j1lu_resident_$$m.img mutant-$$m $$mode >$(BUILD)/dos_resident_break_$$m.log 2>&1 || rc=$$?; \
	  test $$rc -eq 1; \
	  if test $$m = type; then why='TYPE emitted bytes after cancellation'; else why='gaps2 did not return a usable prompt'; fi; \
	  grep -E "^RED dos-resident-break: ($$why|listing reached the final line after cancellation)$$" $(BUILD)/dos_resident_break_$$m.log; \
	  root=$$(sed -n 's/^oracle artifacts: //p' $(BUILD)/dos_resident_break_$$m.log); \
	  grep -q INT23-BREAK $$root/$$mode/run.serial; \
	  echo "GREEN test-dos-resident-break-mutant: $$m cancellation caught"; \
	done
TEST_UNIT_GATES += test-dos-break-cleanup-mutant
TEST_EMU_GATES += test-dos-resident-break-mutant

# A guest write must be visible to the same independent whole-disk cmp.
$(BUILD)/resident_boot_write.bin: harness/diff/fat_diff/resident_boot_write.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
.PHONY: test-dos-resident-boot-write-mutant
test-dos-resident-boot-write-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/console_text $(BUILD)/dos_resident_break_check $(BUILD)/resident_boot_write.bin
	@rc=0; sh harness/diff/fat_diff/dos_resident_break.sh $(SECONDARY_TRACER_IMG) bootwrite controlshort yes > $(BUILD)/resident-bootwrite.log 2>&1 || rc=$$?; \
	 test $$rc = 1 && grep -Fx 'RED dos-resident-break: boot disk changed' $(BUILD)/resident-bootwrite.log || { cat $(BUILD)/resident-bootwrite.log; exit 1; }; \
	 root=$$(sed -n 's/^oracle artifacts: //p' $(BUILD)/resident-bootwrite.log); \
	 grep -q BOOT-WRITE-COMMITTED $$root/controlshort/run.serial
	@echo 'GREEN test-dos-resident-boot-write-mutant: actual guest boot-disk write rejected by independent cmp'
TEST_EMU_GATES += test-dos-resident-boot-write-mutant

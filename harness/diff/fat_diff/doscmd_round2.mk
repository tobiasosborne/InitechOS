# f8f0 / 0qw1: MS-DOS 3.3 Reference pp.56,149 (PDF141,234).
.PHONY: test-del-directory-host test-del-directory-emu test-del-directory-mutant
test-del-directory-host: $(BUILD)/shell_files
	@sh harness/diff/fat_diff/del_directory.sh host $<
test-del-directory-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(DOS_SAFETY_FIXTURE)
	@sh harness/diff/fat_diff/del_directory.sh emu $(SECONDARY_TRACER_IMG)
TEST_UNIT_GATES += test-del-directory-host
TEST_EMU_GATES += test-del-directory-emu
BATCH_BREAK_SRC := harness/diff/fat_diff/batch_break.c
BATCH_BREAK_DEPS := $(BATCH_BREAK_SRC) $(wildcard harness/diff/fat_diff/batch_break_mint/*) harness/diff/fat_diff/batch_break_host.h os/milton/command.c os/milton/batch.c os/milton/env.c $(DOS_MESSAGES_H)
$(BUILD)/batch_break: $(BATCH_BREAK_DEPS)
	$(CC) $(CFLAGS) -Wno-unused-function -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff $< -o $@
.PHONY: test-batch-break-host
test-batch-break-host: $(BUILD)/batch_break
	@$<
TEST_UNIT_GATES += test-batch-break-host
.PHONY: test-batch-break-emu test-batch-break-mutant
$(BUILD)/batch_break_child.bin: harness/diff/fat_diff/batch_break_mint/exec_child.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
test-batch-break-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/batch_break_child.bin
	@sh harness/diff/fat_diff/batch_break.sh $(SECONDARY_TRACER_IMG)
$(BUILD)/batch_break_mut_%: $(BATCH_BREAK_DEPS)
	$(CC) $(CFLAGS) -Wno-unused-function -D$* -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff $< -o $@
test-batch-break-mutant: $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_TERMINATE $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_RESUME $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_DROP_CHILD_BREAK
	@set -eu; for m in TERMINATE RESUME; do \
	 if $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_$$m > $(BUILD)/batch_break_mut_$$m.log 2>&1; then echo "RED batch mutant survived: $$m";exit 1;fi; \
	 grep '^RED batch-break:' $(BUILD)/batch_break_mut_$$m.log; done
	@echo 'GREEN batch-break-mutant: always terminate and always resume each caught'
	@rc=0; $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_DROP_CHILD_BREAK > $(BUILD)/batch_break_mut_CHILD.log 2>&1 || rc=$$?; test $$rc -eq 1
	@grep '^RED batch-break: EXEC child asks after cleanup' $(BUILD)/batch_break_mut_CHILD.log
	@echo 'GREEN batch-break-mutant: dropped EXEC child propagation caught'
TEST_UNIT_GATES += test-batch-break-mutant
TEST_EMU_GATES += test-batch-break-emu
DEL_DIRECTORY_MUTANTS := CMD_MUTATE_DEL_DIRECTORY_UNLINK CMD_MUTATE_DEL_NO_CONFIRM CMD_MUTATE_DEL_DIRECTORY_RECURSE
test-del-directory-mutant: $(addprefix $(BUILD)/shell_files_mut_,$(DEL_DIRECTORY_MUTANTS))
	@set -eu; for m in $(DEL_DIRECTORY_MUTANTS); do \
	 if sh harness/diff/fat_diff/del_directory.sh host $(BUILD)/shell_files_mut_$$m $$m > $(BUILD)/del_directory_mut_$$m.log 2>&1; then echo "RED DEL mutant survived: $$m";exit 1;fi; \
	 grep '^RED del-directory:' $(BUILD)/del_directory_mut_$$m.log; done
	@echo 'GREEN del-directory-mutant: raw UNLINK / no confirmation / nested recursion each caught'
$(eval $(call dos-safety-shell-mutant,del_directory,CMD_MUTATE_DEL_DIRECTORY_UNLINK))
$(eval $(call dos-safety-shell-mutant,batch_terminate,CMD_MUTATE_BATCH_ALWAYS_TERMINATE))
$(eval $(call dos-safety-shell-mutant,batch_resume,CMD_MUTATE_BATCH_ALWAYS_RESUME))
$(eval $(call dos-safety-shell-mutant,batch_child,CMD_MUTATE_BATCH_DROP_CHILD_BREAK))
.PHONY: test-del-directory-emu-mutant test-batch-break-emu-mutant
test-del-directory-emu-mutant: $(HARNESS_BIN) $(DOS_SAFETY_FIXTURE) $(BUILD)/tracer_mut_safety_del_directory.img
	@if sh harness/diff/fat_diff/del_directory.sh emu $(BUILD)/tracer_mut_safety_del_directory.img raw >$(BUILD)/del_directory_emu_mut.log 2>&1; then echo 'RED directory UNLINK mutant survived QEMU';exit 1;fi
	@grep '^RED del-directory: directory confirmation missing' $(BUILD)/del_directory_emu_mut.log
	@echo 'GREEN del-directory-emu-mutant: raw directory UNLINK caught'
test-batch-break-emu-mutant: $(HARNESS_BIN) $(BUILD)/batch_break_child.bin $(BUILD)/tracer_mut_safety_batch_terminate.img $(BUILD)/tracer_mut_safety_batch_resume.img $(BUILD)/tracer_mut_safety_batch_child.img
	@set -eu; for pair in terminate:pause-n resume:pause-y; do \
	 m=$${pair%%:*};mode=$${pair#*:}; \
	 if sh harness/diff/fat_diff/batch_break.sh $(BUILD)/tracer_mut_safety_batch_$$m.img $$m $$mode >$(BUILD)/batch_break_emu_mut_$$m.log 2>&1; then echo "RED batch $$m survived QEMU";exit 1;fi; \
	 grep '^RED batch-break-emu: .* sentinel' $(BUILD)/batch_break_emu_mut_$$m.log; done
	@echo 'GREEN batch-break-emu-mutant: opposite post-question sentinels catch both mutants'
	@rc=0; sh harness/diff/fat_diff/batch_break.sh $(BUILD)/tracer_mut_safety_batch_child.img drop-child exec-n >$(BUILD)/batch_break_emu_mut_child.log 2>&1 || rc=$$?; test $$rc -eq 1
	@grep '^RED batch-break-emu: exec-n catalogue question missing' $(BUILD)/batch_break_emu_mut_child.log
	@echo 'GREEN batch-break-emu-mutant: dropped EXEC child propagation caught'
TEST_UNIT_GATES += test-del-directory-mutant
TEST_EMU_GATES += test-del-directory-emu-mutant test-batch-break-emu-mutant
.PHONY: test-del-directory-b023
test-del-directory-b023: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/del_b023.sh $(SECONDARY_TRACER_IMG)
TEST_EMU_GATES += test-del-directory-b023

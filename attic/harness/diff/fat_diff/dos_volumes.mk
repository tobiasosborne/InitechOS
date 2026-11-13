# initech-slvd / bcg.16: oracles precede implementation. Never run full test here.
DOS_VOLUMES_FLAGS := $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -Ispec -Ios/milton -Iseed -I$(FAT_DIFF_DIR) -Ibuild
$(BUILD)/dos_volumes_host: harness/diff/fat_diff/dos_volumes_host.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(DOS_VOLUMES_FLAGS) $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
.PHONY: test-dos-volumes-host test-dos-volumes test-fat16-emu test-dos-volumes-mutant
test-dos-volumes-host: $(BUILD)/dos_volumes_host $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/dos_volumes.sh host $(SECONDARY_TRACER_IMG)
test-dos-volumes: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/transient_flairtenants.bin
	@sh harness/diff/fat_diff/dos_volumes.sh emu $(SECONDARY_TRACER_IMG)
test-fat16-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/dos_volumes.sh fat16 $(SECONDARY_TRACER_IMG)

# One named perturbation per volume rule, compiled from the shipped sources.
DOS_VOLUMES_MUTANTS := SELECT PATH CWD HANDLE SEARCH CACHE INVALID LIMIT LASTDRIVE
$(BUILD)/dos_volumes_host_%: harness/diff/fat_diff/dos_volumes_host.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(DOS_VOLUMES_FLAGS) -DDOS_VOLUMES_MUT_$* $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
test-dos-volumes-mutant: test-dos-volumes-host $(addprefix $(BUILD)/dos_volumes_host_,$(DOS_VOLUMES_MUTANTS)) $(SECONDARY_TRACER_IMG)
	@set -eu; for m in $(DOS_VOLUMES_MUTANTS); do \
	 log=$(BUILD)/dos-volumes/mut-$$m.log; mkdir -p $(BUILD)/dos-volumes; \
	 if DOS_VOLUMES_HOST=$(BUILD)/dos_volumes_host_$$m sh harness/diff/fat_diff/dos_volumes.sh host $(SECONDARY_TRACER_IMG) $$m > $$log 2>&1; then echo "FAIL dos-volumes $$m survived"; exit 1; fi; \
	 grep '^RED dos-volumes ' $$log; done
	@echo 'GREEN test-dos-volumes-mutant: SELECT PATH CWD HANDLE SEARCH CACHE INVALID LIMIT LASTDRIVE rejected'
TEST_UNIT_GATES += test-dos-volumes-host test-dos-volumes-mutant
TEST_EMU_GATES += test-dos-volumes test-fat16-emu

$(eval $(call dos-safety-shell-mutant,volume_prompt,CMD_MUTATE_VOLUME_PROMPT))
$(eval $(call dos-safety-shell-mutant,volume_message,CMD_MUTATE_VOLUME_MESSAGE))
$(BUILD)/fileio_volumes_fat16.o: os/milton/fileio_fat.c $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FILEIO_FAT_OPT) -DDOS_VOLUMES_MUT_FAT16 -Ispec -Ios/milton -c $< -o $@
$(eval $(call j1lu-emu-variant,volumes_fat16,$(KERNEL_FILEIO_OBJ),$(BUILD)/fileio_volumes_fat16.o))
.PHONY: test-dos-volumes-emu-mutant test-fat16-emu-mutant
test-dos-volumes-emu-mutant: test-dos-volumes $(HARNESS_BIN) $(BUILD)/tracer_mut_safety_volume_prompt.img $(BUILD)/tracer_mut_safety_volume_message.img $(BUILD)/transient_flairtenants.bin
	@mkdir -p $(BUILD)/dos-volumes
	@if sh harness/diff/fat_diff/dos_volumes.sh emu $(BUILD)/tracer_mut_safety_volume_prompt.img prompt > $(BUILD)/dos-volumes/prompt.log 2>&1; then echo 'FAIL dos-volumes PROMPT survived'; exit 1; fi
	@grep '^RED dos-volumes PROMPT current C' $(BUILD)/dos-volumes/prompt.log
	@if sh harness/diff/fat_diff/dos_volumes.sh emu $(BUILD)/tracer_mut_safety_volume_message.img message > $(BUILD)/dos-volumes/message.log 2>&1; then echo 'FAIL dos-volumes MESSAGE survived'; exit 1; fi
	@grep '^RED dos-volumes INVALID B/D message' $(BUILD)/dos-volumes/message.log
	@echo 'GREEN test-dos-volumes-emu-mutant: PROMPT MESSAGE rejected'
test-fat16-emu-mutant: test-fat16-emu $(HARNESS_BIN) $(BUILD)/j1lu_volumes_fat16.img
	@mkdir -p $(BUILD)/dos-volumes
	@if sh harness/diff/fat_diff/dos_volumes.sh fat16 $(BUILD)/j1lu_volumes_fat16.img corrupt > $(BUILD)/dos-volumes/fat16.log 2>&1; then echo 'FAIL dos-volumes FAT16 survived'; exit 1; fi
	@grep '^RED dos-volumes FAT16 ATA TYPE bytes' $(BUILD)/dos-volumes/fat16.log
	@echo 'GREEN test-fat16-emu-mutant: FAT16 byte corruption rejected'
TEST_EMU_GATES += test-dos-volumes-emu-mutant test-fat16-emu-mutant

# The corrected AH40/CX0 model exposed stale retained-tail bytes (seed 7).
# Host + ATA gates were RED on GAPIN before the positioned-write fix.
$(BUILD)/fat12_fuzz_mut_stale_gap: $(FAT_DIFF_DIR)/fat12_fuzz.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUTATE_PARTIAL_STALE_GAP -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -o $@ $^
$(BUILD)/fat12_stale_gap.o: os/milton/fat12.c os/milton/fat12.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FAT12_OPT) -DFAT12_MUTATE_PARTIAL_STALE_GAP -Ispec -Ios/milton -c $< -o $@
$(eval $(call j1lu-emu-variant,stale_gap,$(KERNEL_FAT12_OBJ),$(BUILD)/fat12_stale_gap.o))
.PHONY: test-fat-fuzz-gap-mutant test-fat-truncate-gap-emu-mutant
test-fat-fuzz-gap-mutant: $(BUILD)/fat12_fuzz_mut_stale_gap $(FAT12_FUZZ_BLANK_IMG)
	@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
	@mkdir -p $(BUILD)/dos-volumes
	@if $< --sweep 7 7 --ops $(FAT12_FUZZ_OPS) --no-external "$(FUZZ_GATE_BLANK)" > $(BUILD)/dos-volumes/stale-gap.log 2>&1; then echo 'FAIL STALE_GAP survived fuzzer'; exit 1; fi
	@grep 'DIVERGENCE on seed 7' $(BUILD)/dos-volumes/stale-gap.log
	@grep 'SHRUNK REPRODUCER (seed=7)' $(BUILD)/dos-volumes/stale-gap.log
	@echo 'GREEN test-fat-fuzz-gap-mutant: STALE_GAP found and shrunk'
test-fat-truncate-gap-emu-mutant: $(BUILD)/truncate $(BUILD)/truncate_program.bin $(HARNESS_BIN) $(BUILD)/j1lu_stale_gap.img
	@mkdir -p $(BUILD)/dos-volumes
	@if sh harness/diff/fat_diff/truncate.sh emu $(BUILD)/j1lu_stale_gap.img stale-gap > $(BUILD)/dos-volumes/stale-gap-emu.log 2>&1; then echo 'FAIL STALE_GAP survived ATA'; exit 1; fi
	@grep '^RED truncate: GAPIN bytes != mcopy' $(BUILD)/dos-volumes/stale-gap-emu.log
	@echo 'GREEN test-fat-truncate-gap-emu-mutant: STALE_GAP rejected after real AH40/ATA'
TEST_UNIT_GATES += test-fat-fuzz-gap-mutant
TEST_EMU_GATES += test-fat-truncate-gap-emu-mutant

$(BUILD)/int21_coverage: harness/diff/fat_diff/int21_coverage.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 $< -o $@
$(BUILD)/int21_inventory: harness/pubs/int21_inventory.c os/milton/int21.c os/milton/int21.h $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) $(CFLAGS) -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections -Ispec -Ios/milton -Ibuild $< -o $@
.PHONY: dos-volumes-gate-vector test-kernel-coverage test-kernel-coverage-mutant test-kernel-certificate
dos-volumes-gate-vector:
	@printf '%s\n' $(TEST_UNIT_GATES) $(TEST_EMU_GATES) $(TEST_MUTANT_GATES)
define kernel-coverage-inputs
$(MAKE) --no-print-directory dos-volumes-gate-vector > $(BUILD)/int21_gate_vector.txt
$(BUILD)/int21_inventory > $(BUILD)/int21_inventory.txt
endef
define kernel-coverage-run
$(BUILD)/int21_coverage $(1) $(BUILD)/int21_gate_vector.txt harness/diff/fat_diff/int21_witnesses.tsv $(BUILD)/int21_inventory.txt
endef
test-kernel-coverage: $(BUILD)/int21_coverage $(BUILD)/int21_inventory
	@$(kernel-coverage-inputs)
	@$(call kernel-coverage-run,spec/int21h_register.json)
test-kernel-coverage-mutant: $(BUILD)/int21_coverage $(BUILD)/int21_inventory
	@$(kernel-coverage-inputs)
	@sed '/"ah": "30h"/d' spec/int21h_register.json > $(BUILD)/int21_register_drop_getver.json
	@if $(call kernel-coverage-run,$(BUILD)/int21_register_drop_getver.json) > $(BUILD)/int21_coverage_mutant.log 2>&1; then echo 'FAIL kernel-coverage DROP_REGISTER survived'; exit 1; fi
	@grep '^RED kernel-coverage register dropped dispatcher AH=30' $(BUILD)/int21_coverage_mutant.log
	@echo 'GREEN test-kernel-coverage-mutant: DROP_REGISTER_GETVER rejected'
test-kernel-certificate: test-40oq test-dos-volumes test-fat16-emu test-kernel-coverage
	@echo 'GREEN test-kernel-certificate: Appendix A + two volumes + FAT16 ATA'
# Restored 2026-10-17 (WL-0167, bead initech-dpb1): AH=1Fh/32h landed with
# their DPB witnesses (harness/diff/fat_diff/int21_dpb.mk), so the strict
# certificate is green and back in the default vectors.
TEST_UNIT_GATES += test-kernel-coverage test-kernel-coverage-mutant
TEST_EMU_GATES += test-kernel-certificate

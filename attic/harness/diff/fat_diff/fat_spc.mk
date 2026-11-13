# initech-fol7: multi-sector clusters, without a cluster-sized kernel buffer.
$(BUILD)/test_fat_spc: $(FAT_DIFF_DIR)/test_fat_spc.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) os/milton/fat12.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -Ios/milton -I$(FAT_DIFF_DIR) $(filter %.c,$^) -o $@
$(BUILD)/test_fat_spc_mut: $(FAT_DIFF_DIR)/test_fat_spc.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) os/milton/fat12.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT_MUT_SPC_ONE -Ispec -Ios/milton -I$(FAT_DIFF_DIR) $(filter %.c,$^) -o $@
.PHONY: test-fat-spc test-fat-spc-mutant test-fat-spc-emu
test-fat-spc: $(BUILD)/test_fat_spc $(FAT_DUMP_BIN)
	@sh $(FAT_DIFF_DIR)/fat_spc.sh $<
test-fat-spc-mutant: $(BUILD)/test_fat_spc_mut $(FAT_DUMP_BIN)
	@mkdir -p $(BUILD)/fat-spc
	@set -eu; for type in 12 16; do \
	 if FAT_SPC_TYPES=$$type sh $(FAT_DIFF_DIR)/fat_spc.sh $< mutant-$$type > $(BUILD)/fat-spc/mutant-$$type.log 2>&1; then echo "FAIL FAT_MUT_SPC_ONE survived FAT$$type"; exit 1; fi; \
	 grep -Fx 'RED fat-spc whole-file bytes vs mcopy' $(BUILD)/fat-spc/mutant-$$type.log; done
	@echo '>>> test-fat-spc-mutant: green (FAT_MUT_SPC_ONE correctly RED for FAT12 and FAT16)'
test-fat-spc-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@INITECH_QMP_STDIO=1 FAT_SPC=4 sh $(FAT_DIFF_DIR)/dos_volumes.sh fat16 $(SECONDARY_TRACER_IMG) spc4
	@echo '>>> test-fat-spc-emu: green (spc=4 FAT16 ATA mount, DIR/TYPE vs mtools)'
TEST_UNIT_GATES += test-fat-spc test-fat-spc-mutant
TEST_EMU_GATES += test-fat-spc-emu

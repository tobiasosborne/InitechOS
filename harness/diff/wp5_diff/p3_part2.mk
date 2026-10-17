# WL-0146 Part 2: host oracles and locked QEMU trace written before artifact.
IWORD_P32_DEPS := harness/proptest/test_iword_p3_part2.c $(IWORD_P3_DEPS)
$(BUILD)/test_iword_p32: $(IWORD_P32_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ $< $(IWORD_CORE_SRC)
$(BUILD)/test_iword_p32_mut_%: $(IWORD_P32_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DIWORD_MUT_$* -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ $< $(IWORD_CORE_SRC)
.PHONY: test-iword-replace test-iword-replace-mutant test-flair-iword-replace test-flair-iword-replace-mutant
test-iword-replace: $(BUILD)/test_iword_p32 $(FAT_DUMP_BIN)
	@$(BUILD)/test_iword_p32 replace
	@sh harness/diff/wp5_diff/replace.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_replace $(FAT_DUMP_BIN) $(BUILD)/test_iword_p32
	@printf '>>> test-iword-replace: green\n'
IWORD_REPLACE_MUTANTS := REPLACE_CASE REPLACE_SKIP REPLACE_CODE REPLACE_CONFIRM REPLACE_PROMPT
test-iword-replace-mutant: test-iword-replace $(foreach m,$(IWORD_REPLACE_MUTANTS),$(BUILD)/test_iword_p32_mut_$(m))
	@for m in $(IWORD_REPLACE_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; \
	 if $$b replace > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
	 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON replace:' $$b.log || exit 1; \
	 printf '>>> test-iword-replace-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL replace' $$b.log)"; done
	@$(FAT_DUMP_BIN) $(BUILD)/iword_replace/search.img --cat M06.WP5 > $(BUILD)/iword_replace/M06.WP5
	@if $(BUILD)/test_iword_p32_mut_REPLACE_CODE replace-corpus $(BUILD)/iword_replace/M06.WP5 > $(BUILD)/iword_replace_code_corpus_mut.log 2>&1; then printf 'FAIL: corpus let REPLACE_CODE survive\n'; exit 1; fi
	@grep -F 'RED-REASON replace-corpus:' $(BUILD)/iword_replace_code_corpus_mut.log
	@printf '>>> test-iword-replace-mutant: real MINT06 REPLACE_CODE correctly RED: '; grep -m1 'FAIL replace-corpus' $(BUILD)/iword_replace_code_corpus_mut.log
	@printf '>>> test-iword-replace-mutant: green\n'
test-flair-iword-replace: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/test_iword_p32 $(PPM_REGION_COUNT_BIN)
	$(call appl-boot,flair_iword_replace,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_REPLACE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_replace.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_replace.serial)
	@$(BUILD)/test_iword_p32 replace-serial $(BUILD)/flair_iword_replace.serial
	@mcopy -o -i $(BUILD)/flair_iword_replace_data.img ::REPLACE $(BUILD)/iword_replace_saved.WP5
	@$(call iword_ref_text,$(BUILD)/iword_replace_saved.WP5,Cat fox)
	@tail -c +328 $(BUILD)/iword_replace_saved.WP5 > $(BUILD)/iword_replace_saved.body
	@printf 'Cat fox' > $(BUILD)/iword_replace_expected.body
	@cmp $(BUILD)/iword_replace_expected.body $(BUILD)/iword_replace_saved.body
	@fsck.fat -n $(BUILD)/flair_iword_replace_data.img
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_replace.ppm,7 62 63 78)); [ "$$1" -gt 10 ]
	@printf '>>> test-flair-iword-replace: green\n'
$(eval $(call tenant-exe-rules,iword_replace_confirm,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_REPLACE_CONFIRM,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_REPLACE_CONFIRM_IMG := $(BUILD)/flair_data_iword_replace_confirm.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_REPLACE_CONFIRM_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_replace_confirm.exe,))
test-flair-iword-replace-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_REPLACE_CONFIRM_IMG) $(BUILD)/test_iword_p32
	$(call appl-boot,flair_iword_replace_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_REPLACE_CONFIRM_IMG),FLAIR_IWORD_REPLACE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_replace_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_replace_mut.serial)
	@if $(BUILD)/test_iword_p32 replace-serial $(BUILD)/flair_iword_replace_mut.serial > $(BUILD)/iword_replace_mut.log 2>&1; then printf 'FAIL: REPLACE_CONFIRM survived\n'; exit 1; fi
	@grep -F 'RED-REASON replace-serial:' $(BUILD)/iword_replace_mut.log
	@printf '>>> test-flair-iword-replace-mutant: green (REPLACE_CONFIRM correctly RED)\n'
TEST_UNIT_GATES += test-iword-replace test-iword-replace-mutant
TEST_EMU_GATES += test-flair-iword-replace test-flair-iword-replace-mutant

.PHONY: test-iword-undo test-iword-undo-mutant test-flair-iword-undo test-flair-iword-undo-mutant
test-iword-undo: $(BUILD)/test_iword_p32 $(FAT_DUMP_BIN)
	@$< undo
	@sh harness/diff/wp5_diff/replace.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_undo $(FAT_DUMP_BIN) $(BUILD)/test_iword_p32 undo-corpus
	@printf '>>> test-iword-undo: green\n'
IWORD_UNDO_MUTANTS := UNDO_LEVEL UNDO_ORDER UNDO_CANCEL UNDO_PREVIEW UNDO_BLOCK UNDO_MOVEMENT
# UNDO_FIXUP is killed by the real-document byte oracle (MINT09's tab moves).
test-iword-undo-mutant: test-iword-undo $(BUILD)/test_iword_p32_mut_UNDO_FIXUP $(foreach m,$(IWORD_UNDO_MUTANTS),$(BUILD)/test_iword_p32_mut_$(m))
	@for m in $(IWORD_UNDO_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; \
	 if $$b undo > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
	 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON undo:' $$b.log || exit 1; \
	 printf '>>> test-iword-undo-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL undo' $$b.log)"; done
	@$(FAT_DUMP_BIN) $(BUILD)/iword_undo/search.img --cat M09.WP5 > $(BUILD)/iword_undo/M09.WP5
	@if $(BUILD)/test_iword_p32_mut_UNDO_FIXUP undo-corpus $(BUILD)/iword_undo/M09.WP5 > $(BUILD)/iword_undo_fixup_mut.log 2>&1; then printf 'FAIL: UNDO_FIXUP survived\n'; exit 1; fi
	@grep -F 'RED-REASON undo-corpus:' $(BUILD)/iword_undo_fixup_mut.log
	@printf '>>> test-iword-undo-mutant: UNDO_FIXUP correctly RED: '; grep -m1 'undo-corpus byte diff:' $(BUILD)/iword_undo_fixup_mut.log
	@printf '>>> test-iword-undo-mutant: green\n'
test-flair-iword-undo: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/test_iword_p32 $(PPM_REGION_COUNT_BIN)
	$(call appl-boot,flair_iword_undo,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo.serial)
	@$(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo.serial
	@mcopy -o -i $(BUILD)/flair_iword_undo_data.img ::UNDO $(BUILD)/iword_undo_saved.WP5
	@$(call iword_ref_text,$(BUILD)/iword_undo_saved.WP5,c)
	@mcopy -o -i $(BUILD)/flair_iword_undo_data.img ::MOTION $(BUILD)/iword_motion_saved.WP5
	@$(call iword_ref_text,$(BUILD)/iword_motion_saved.WP5,abc)
	@fsck.fat -n $(BUILD)/flair_iword_undo_data.img
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_undo.ppm,7 62 23 78)); [ "$$1" -gt 2 ]
	@printf '>>> test-flair-iword-undo: green\n'
$(eval $(call tenant-exe-rules,iword_undo_level,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_UNDO_LEVEL,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_UNDO_LEVEL_IMG := $(BUILD)/flair_data_iword_undo_level.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_UNDO_LEVEL_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_undo_level.exe,))
$(eval $(call tenant-exe-rules,iword_undo_movement,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_UNDO_MOVEMENT,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_UNDO_MOVEMENT_IMG := $(BUILD)/flair_data_iword_undo_movement.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_UNDO_MOVEMENT_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_undo_movement.exe,))
test-flair-iword-undo-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_UNDO_LEVEL_IMG) $(FLAIR_DATA_IWORD_UNDO_MOVEMENT_IMG) $(BUILD)/test_iword_p32
	$(call appl-boot,flair_iword_undo_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_UNDO_LEVEL_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo_mut.serial)
	@if $(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo_mut.serial > $(BUILD)/iword_undo_mut.log 2>&1; then printf 'FAIL: UNDO_LEVEL survived\n'; exit 1; fi
	@grep -F 'RED-REASON undo-serial:' $(BUILD)/iword_undo_mut.log
	@printf '>>> test-flair-iword-undo-mutant: UNDO_LEVEL correctly RED\n'
	$(call appl-boot,flair_iword_undo_movement_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_UNDO_MOVEMENT_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo_movement_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo_movement_mut.serial)
	@if $(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo_movement_mut.serial > $(BUILD)/iword_undo_movement_mut.log 2>&1; then printf 'FAIL: UNDO_MOVEMENT survived\n'; exit 1; fi
	@grep -F 'FAIL undo emulator Search cursor motion' $(BUILD)/iword_undo_movement_mut.log
	@printf '>>> test-flair-iword-undo-mutant: green (UNDO_LEVEL and UNDO_MOVEMENT correctly RED)\n'
TEST_UNIT_GATES += test-iword-undo test-iword-undo-mutant
TEST_EMU_GATES += test-flair-iword-undo test-flair-iword-undo-mutant

.PHONY: test-iword-setup test-iword-setup-mutant test-flair-iword-setup test-flair-iword-setup-mutant
test-iword-setup: $(BUILD)/test_iword_p32 $(FAT_DUMP_BIN)
	@mkdir -p $(BUILD)/iword_setup_fs
	@rm -f $(BUILD)/iword_setup_fs/IWORD.SET
	@$< setup $(BUILD)/iword_setup_fs
	@sh harness/diff/wp5_diff/settings.sh $(BUILD)/iword_setup_fs $(FAT_DUMP_BIN)
	@printf '>>> test-iword-setup: green\n'
IWORD_SETUP_MUTANTS := SETUP_SAVE SETUP_LOAD SETUP_VALIDATE SETUP_SHORT
test-iword-setup-mutant: $(foreach m,$(IWORD_SETUP_MUTANTS),$(BUILD)/test_iword_p32_mut_$(m))
	@mkdir -p $(BUILD)/iword_setup_mut_fs
	@for m in $(IWORD_SETUP_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; rm -f $(BUILD)/iword_setup_mut_fs/IWORD.SET; \
	 if $$b setup $(BUILD)/iword_setup_mut_fs > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
	 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON setup:' $$b.log || exit 1; \
	 printf '>>> test-iword-setup-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL setup' $$b.log)"; done
	@printf '>>> test-iword-setup-mutant: green\n'
test-flair-iword-setup: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/test_iword_p32 $(PPM_REGION_COUNT_BIN)
	$(call appl-boot,flair_iword_setup,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_SETUP_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_setup.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_setup.serial)
	@$(BUILD)/test_iword_p32 setup-serial $(BUILD)/flair_iword_setup.serial
	@mcopy -o -i $(BUILD)/flair_iword_setup_data.img ::IWORD.SET $(BUILD)/iword_setup_saved.SET
	@printf 'IWS1cp\001\000' > $(BUILD)/iword_setup_expected.SET
	@cmp $(BUILD)/iword_setup_expected.SET $(BUILD)/iword_setup_saved.SET
	@fsck.fat -n $(BUILD)/flair_iword_setup_data.img
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_setup.ppm,7 62 175 78)); [ "$$1" -gt 10 ]
	@printf '>>> test-flair-iword-setup: green\n'
$(eval $(call tenant-exe-rules,iword_setup_load,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_SETUP_LOAD,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_SETUP_LOAD_IMG := $(BUILD)/flair_data_iword_setup_load.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_SETUP_LOAD_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_setup_load.exe,))
test-flair-iword-setup-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_SETUP_LOAD_IMG) $(BUILD)/test_iword_p32
	$(call appl-boot,flair_iword_setup_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_SETUP_LOAD_IMG),FLAIR_IWORD_SETUP_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_setup_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_setup_mut.serial)
	@if $(BUILD)/test_iword_p32 setup-serial $(BUILD)/flair_iword_setup_mut.serial > $(BUILD)/iword_setup_mut.log 2>&1; then printf 'FAIL: SETUP_LOAD survived\n'; exit 1; fi
	@grep -F 'RED-REASON setup-serial:' $(BUILD)/iword_setup_mut.log
	@printf '>>> test-flair-iword-setup-mutant: green (SETUP_LOAD correctly RED)\n'
TEST_UNIT_GATES += test-iword-setup test-iword-setup-mutant
TEST_EMU_GATES += test-flair-iword-setup test-flair-iword-setup-mutant

# harness/diff/wp5_diff/p4.mk -- IWORD P4 (bead initech-8q6j): the page
# features and the print stream vs what the REAL WordPerfect 5.1 wrote and
# printed (../wordperfect51-decomp MINT33..44, PRTMINTnn.TXT; re/mint-results-
# 005.md). Host oracle first, then the emulator gates (the m0dc / P3 pattern).
TEST_IWORD_P4  := $(BUILD)/test_iword_p4
IWORD_P4_DEPS  := $(IWORD_WP5_DIR)/test_iword_p4.c $(IWORD_WP5_DIR)/iword_keys.h $(IWORD_CORE_SRC) $(IWORD_CORE_HDR) seed/test_assert.h $(ORACLE_POLICY_H)
$(TEST_IWORD_P4): $(IWORD_P4_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -Iharness $(IWORD_CORE_INC) -o $@ $(IWORD_WP5_DIR)/test_iword_p4.c $(IWORD_CORE_SRC)
$(BUILD)/test_iword_p4_mut_%: $(IWORD_P4_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DIWORD_MUT_$* -Iseed -Iharness $(IWORD_CORE_INC) -o $@ $(IWORD_WP5_DIR)/test_iword_p4.c $(IWORD_CORE_SRC)

.PHONY: test-iword-p4 test-iword-p4-mutant
test-iword-p4: $(TEST_IWORD_P4)
	$(call need_wp_goldens,test-iword-p4)
	@printf '>>> test-iword-p4: soft pages, headers/footers, notes, columns, tables, print vs real WP 5.1 (initech-8q6j)\n'
	@$(TEST_IWORD_P4) $(WORDPERFECT51_DECOMP)
	@printf '>>> test-iword-p4: green\n'
# one named mutant per rule, each RED for the part that owns the rule
IWORD_P4_MUTANTS := HF_OCC:typed HF_GAP:print SPG_NONE:pages NOTE_RENUM:notes NOTE_TOP:print \
                    COL_BREAK:print COL_BELOW:print TBL_WIDTH:typed TBL_ROWH:typed PRT_PAGEEND:print
test-iword-p4-mutant: $(foreach m,$(IWORD_P4_MUTANTS),$(BUILD)/test_iword_p4_mut_$(firstword $(subst :, ,$(m))))
	$(call need_wp_goldens,test-iword-p4-mutant)
	@for pair in $(IWORD_P4_MUTANTS); do \
		m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_p4_mut_$$m; \
		if $$bin $(WORDPERFECT51_DECOMP) > $$bin.log 2>&1; then printf '!!! test-iword-p4-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
		grep -q 'checks,' $$bin.log || { printf '!!! test-iword-p4-mutant FAIL: %s died without a summary\n' "$$m"; cat $$bin.log; exit 1; }; \
		grep -qF "RED-REASON $$why:" $$bin.log || { printf '!!! test-iword-p4-mutant FAIL: %s went RED for the wrong reason (want %s)\n' "$$m" "$$why"; cat $$bin.log; exit 1; }; \
		printf '>>> test-iword-p4-mutant: %s correctly RED (%s; %s)\n' "$$m" "$$(grep -m1 -F "RED-REASON $$why:" $$bin.log | sed 's/^ *//')" "$$(grep -m1 'FAIL' $$bin.log | sed 's/^ *//')"; \
	done
	@printf '>>> test-iword-p4-mutant: green (all ten mutants RED for the named reason)\n'
TEST_UNIT_GATES += test-iword-p4 test-iword-p4-mutant

# ---- the P4 screens on the host (harness/proptest/test_iword_p4_face.c) ----
TEST_IWORD_P4_FACE := $(BUILD)/test_iword_p4_face
IWORD_P4_FACE_DEPS := harness/proptest/test_iword_p4_face.c $(IWORD_FACE_DEPS)
$(TEST_IWORD_P4_FACE): $(IWORD_P4_FACE_DEPS) | $(BUILD)
	$(CC) $(IWORD_FACE_CFLAGS) -o $@ harness/proptest/test_iword_p4_face.c $(IWORD_CORE_SRC)
$(BUILD)/test_iword_p4_face_mut_%: $(IWORD_P4_FACE_DEPS) | $(BUILD)
	$(CC) $(IWORD_FACE_CFLAGS) -DIWORD_MUT_$* -o $@ harness/proptest/test_iword_p4_face.c $(IWORD_CORE_SRC)
IWORD_P4_FACE_PARTS := hf notes cols table print
.PHONY: test-iword-p4-face test-iword-p4-face-mutant
test-iword-p4-face: $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-iword-p4-face)
	@for p in $(IWORD_P4_FACE_PARTS); do d=$(BUILD)/iword_p4_face_$$p; rm -rf $$d; mkdir -p $$d; \
	 $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $$d $$p || exit 1; done
	@printf '>>> test-iword-p4-face: green (Format: Page / headers, notes, columns, table, print screens == the WP shots; saved == MINT34/36/37/38; PRINT.TXT == PRTMINT34; LPT1 -> PC LOAD LETTER)\n'
# one named mutant per rule, RED for its part
IWORD_P4_FACE_MUTANTS := HF_STATUS:hf NOTE_NUMBER:notes CELL_TAB:table PRINT_NOFAIL:print COL_STATUS:cols
test-iword-p4-face-mutant: $(foreach m,$(IWORD_P4_FACE_MUTANTS),$(BUILD)/test_iword_p4_face_mut_$(firstword $(subst :, ,$(m))))
	$(call need_wp_goldens,test-iword-p4-face-mutant)
	@for pair in $(IWORD_P4_FACE_MUTANTS); do m=$${pair%%:*}; p=$${pair#*:}; b=$(BUILD)/test_iword_p4_face_mut_$$m; \
	 d=$(BUILD)/iword_p4_face_mut_$$m; rm -rf $$d; mkdir -p $$d; \
	 if $$b $(WORDPERFECT51_DECOMP) $$d $$p > $$b.log 2>&1; then printf '!!! test-iword-p4-face-mutant FAIL: %s survived\n' "$$m"; exit 1; fi; \
	 grep -q 'checks, .* failures' $$b.log && grep -qF "RED-REASON $$p:" $$b.log || { printf '!!! %s RED for the wrong reason\n' "$$m"; cat $$b.log; exit 1; }; \
	 printf '>>> test-iword-p4-face-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 'FAIL' $$b.log | sed 's/^ *//')"; done
	@printf '>>> test-iword-p4-face-mutant: green (all five mutants RED for the named reason)\n'
TEST_UNIT_GATES += test-iword-p4-face test-iword-p4-face-mutant

# ---- the emulator gates (QEMU; the P3 pattern: harness/diff/wp5_diff/p3.mk) ----
# Each boots $(FLAIR_IWORD_GATE_IMG), types a locked P4 trace (spec/
# flair_iword_traces.mk), and checks: WORD up and the desktop alive; every
# screen of the session drawn (test_iword_p4_face <capture> <part>-serial,
# the strings typed from the WP shots); the document WORD.EXE saved == the
# real WP file (test_iword_p4 --cmp; the print gate: PRINT.TXT == the real
# WP print PRTMINT34.TXT, byte for byte); fsck.fat. Its mutant boots a WORD.EXE
# built with one named knob on its own data volume and must go RED.

.PHONY: test-flair-iword-hf test-flair-iword-hf-mutant
test-flair-iword-hf: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(TEST_IWORD_P4) $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-flair-iword-hf)
	$(call appl-boot,flair_iword_hf,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_HF_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_hf.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_hf.serial) || { printf '!!! test-flair-iword-hf FAIL: WORD did not come up or the desktop died\n'; exit 1; }
	@$(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_hf.serial hf-serial
	@mcopy -o -i $(BUILD)/flair_iword_hf_data.img ::HF $(BUILD)/flair_iword_hf_saved
	@$(TEST_IWORD_P4) $(WORDPERFECT51_DECOMP) --cmp $(BUILD)/flair_iword_hf_saved MINT34 x
	@fsck.fat -n $(BUILD)/flair_iword_hf_data.img > /dev/null
	@printf '>>> test-flair-iword-hf: green\n'
$(eval $(call tenant-exe-rules,iword_hf_status,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_HF_STATUS,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_HF_STATUS_IMG := $(BUILD)/flair_data_iword_hf_status.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_HF_STATUS_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_hf_status.exe,))
test-flair-iword-hf-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_HF_STATUS_IMG) $(TEST_IWORD_P4_FACE)
	$(call appl-boot,flair_iword_hf_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_HF_STATUS_IMG),FLAIR_IWORD_HF_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_hf_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_hf_mut.serial) || { printf '!!! test-flair-iword-hf-mutant: HF_STATUS RED for the wrong reason (WORD must come up)\n'; exit 1; }
	@if $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_hf_mut.serial hf-serial > $(BUILD)/flair_iword_hf_mut.log 2>&1; then printf '!!! test-flair-iword-hf-mutant FAIL: HF_STATUS survived\n'; exit 1; fi
	@grep -qF 'RED-REASON hf-serial:' $(BUILD)/flair_iword_hf_mut.log
	@printf '>>> test-flair-iword-hf-mutant: green (HF_STATUS correctly RED: %s -- the header screen names no header)\n' "$$(grep -m1 'missing draw' $(BUILD)/flair_iword_hf_mut.log)"
TEST_EMU_GATES += test-flair-iword-hf test-flair-iword-hf-mutant

.PHONY: test-flair-iword-notes test-flair-iword-notes-mutant
test-flair-iword-notes: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(TEST_IWORD_P4) $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-flair-iword-notes)
	$(call appl-boot,flair_iword_notes,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_NOTES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_notes.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_notes.serial) || { printf '!!! test-flair-iword-notes FAIL: WORD did not come up or the desktop died\n'; exit 1; }
	@$(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_notes.serial notes-serial
	@mcopy -o -i $(BUILD)/flair_iword_notes_data.img ::NOTES $(BUILD)/flair_iword_notes_saved
	@$(TEST_IWORD_P4) $(WORDPERFECT51_DECOMP) --cmp $(BUILD)/flair_iword_notes_saved MINT36 
	@fsck.fat -n $(BUILD)/flair_iword_notes_data.img > /dev/null
	@printf '>>> test-flair-iword-notes: green\n'
$(eval $(call tenant-exe-rules,iword_note_number,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_NOTE_NUMBER,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_NOTE_NUMBER_IMG := $(BUILD)/flair_data_iword_note_number.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_NOTE_NUMBER_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_note_number.exe,))
test-flair-iword-notes-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_NOTE_NUMBER_IMG) $(TEST_IWORD_P4_FACE)
	$(call appl-boot,flair_iword_notes_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOTE_NUMBER_IMG),FLAIR_IWORD_NOTES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_notes_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_notes_mut.serial) || { printf '!!! test-flair-iword-notes-mutant: NOTE_NUMBER RED for the wrong reason (WORD must come up)\n'; exit 1; }
	@if $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_notes_mut.serial notes-serial > $(BUILD)/flair_iword_notes_mut.log 2>&1; then printf '!!! test-flair-iword-notes-mutant FAIL: NOTE_NUMBER survived\n'; exit 1; fi
	@grep -qF 'RED-REASON notes-serial:' $(BUILD)/flair_iword_notes_mut.log
	@printf '>>> test-flair-iword-notes-mutant: green (NOTE_NUMBER correctly RED: %s -- the note screen shows no number)\n' "$$(grep -m1 'missing draw' $(BUILD)/flair_iword_notes_mut.log)"
TEST_EMU_GATES += test-flair-iword-notes test-flair-iword-notes-mutant

.PHONY: test-flair-iword-cols test-flair-iword-cols-mutant
test-flair-iword-cols: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(TEST_IWORD_P4) $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-flair-iword-cols)
	$(call appl-boot,flair_iword_cols,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_COLS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_cols.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_cols.serial) || { printf '!!! test-flair-iword-cols FAIL: WORD did not come up or the desktop died\n'; exit 1; }
	@$(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_cols.serial cols-serial
	@mcopy -o -i $(BUILD)/flair_iword_cols_data.img ::COLS $(BUILD)/flair_iword_cols_saved
	@$(TEST_IWORD_P4) $(WORDPERFECT51_DECOMP) --cmp $(BUILD)/flair_iword_cols_saved MINT37 
	@fsck.fat -n $(BUILD)/flair_iword_cols_data.img > /dev/null
	@printf '>>> test-flair-iword-cols: green\n'
$(eval $(call tenant-exe-rules,iword_col_status,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_COL_STATUS,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_COL_STATUS_IMG := $(BUILD)/flair_data_iword_col_status.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_COL_STATUS_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_col_status.exe,))
test-flair-iword-cols-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_COL_STATUS_IMG) $(TEST_IWORD_P4_FACE)
	$(call appl-boot,flair_iword_cols_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_COL_STATUS_IMG),FLAIR_IWORD_COLS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_cols_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_cols_mut.serial) || { printf '!!! test-flair-iword-cols-mutant: COL_STATUS RED for the wrong reason (WORD must come up)\n'; exit 1; }
	@if $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_cols_mut.serial cols-serial > $(BUILD)/flair_iword_cols_mut.log 2>&1; then printf '!!! test-flair-iword-cols-mutant FAIL: COL_STATUS survived\n'; exit 1; fi
	@grep -qF 'RED-REASON cols-serial:' $(BUILD)/flair_iword_cols_mut.log
	@printf '>>> test-flair-iword-cols-mutant: green (COL_STATUS correctly RED: %s -- no Col 2 on the status line)\n' "$$(grep -m1 'missing draw' $(BUILD)/flair_iword_cols_mut.log)"
TEST_EMU_GATES += test-flair-iword-cols test-flair-iword-cols-mutant

.PHONY: test-flair-iword-table test-flair-iword-table-mutant
test-flair-iword-table: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(TEST_IWORD_P4) $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-flair-iword-table)
	$(call appl-boot,flair_iword_table,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_TABLE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_table.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_table.serial) || { printf '!!! test-flair-iword-table FAIL: WORD did not come up or the desktop died\n'; exit 1; }
	@$(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_table.serial table-serial
	@mcopy -o -i $(BUILD)/flair_iword_table_data.img ::TBL $(BUILD)/flair_iword_table_saved
	@$(TEST_IWORD_P4) $(WORDPERFECT51_DECOMP) --cmp $(BUILD)/flair_iword_table_saved MINT38 
	@fsck.fat -n $(BUILD)/flair_iword_table_data.img > /dev/null
	@printf '>>> test-flair-iword-table: green\n'
$(eval $(call tenant-exe-rules,iword_cell_tab,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_CELL_TAB,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_CELL_TAB_IMG := $(BUILD)/flair_data_iword_cell_tab.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_CELL_TAB_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_cell_tab.exe,))
test-flair-iword-table-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_CELL_TAB_IMG) $(TEST_IWORD_P4_FACE)
	$(call appl-boot,flair_iword_table_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_CELL_TAB_IMG),FLAIR_IWORD_TABLE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_table_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_table_mut.serial) || { printf '!!! test-flair-iword-table-mutant: CELL_TAB RED for the wrong reason (WORD must come up)\n'; exit 1; }
	@if $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_table_mut.serial table-serial > $(BUILD)/flair_iword_table_mut.log 2>&1; then printf '!!! test-flair-iword-table-mutant FAIL: CELL_TAB survived\n'; exit 1; fi
	@grep -qF 'RED-REASON table-serial:' $(BUILD)/flair_iword_table_mut.log
	@printf '>>> test-flair-iword-table-mutant: green (CELL_TAB correctly RED: %s -- Tab types a [Tab] in a table)\n' "$$(grep -m1 'missing draw' $(BUILD)/flair_iword_table_mut.log)"
TEST_EMU_GATES += test-flair-iword-table test-flair-iword-table-mutant

.PHONY: test-flair-iword-print test-flair-iword-print-mutant
test-flair-iword-print: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(TEST_IWORD_P4) $(TEST_IWORD_P4_FACE)
	$(call need_wp_goldens,test-flair-iword-print)
	$(call appl-boot,flair_iword_print,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_PRINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_print.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_print.serial) || { printf '!!! test-flair-iword-print FAIL: WORD did not come up or the desktop died\n'; exit 1; }
	@$(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_print.serial print-serial
	@mcopy -o -i $(BUILD)/flair_iword_print_data.img ::PRINT.TXT $(BUILD)/flair_iword_print_saved
	@cmp $(BUILD)/flair_iword_print_saved $(WORDPERFECT51_DECOMP)/mint/work/WP51/PRTMINT34.TXT && printf '>>> test-flair-iword-print: PRINT.TXT == PRTMINT34.TXT (the real WP 5.1 print, byte for byte)\n'
	@fsck.fat -n $(BUILD)/flair_iword_print_data.img > /dev/null
	@printf '>>> test-flair-iword-print: green\n'
$(eval $(call tenant-exe-rules,iword_print_nofail,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_PRINT_NOFAIL,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_PRINT_NOFAIL_IMG := $(BUILD)/flair_data_iword_print_nofail.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_PRINT_NOFAIL_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_print_nofail.exe,))
test-flair-iword-print-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IWORD_PRINT_NOFAIL_IMG) $(TEST_IWORD_P4_FACE)
	$(call appl-boot,flair_iword_print_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_PRINT_NOFAIL_IMG),FLAIR_IWORD_PRINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_print_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_print_mut.serial) || { printf '!!! test-flair-iword-print-mutant: PRINT_NOFAIL RED for the wrong reason (WORD must come up)\n'; exit 1; }
	@if $(TEST_IWORD_P4_FACE) $(WORDPERFECT51_DECOMP) $(BUILD)/flair_iword_print_mut.serial print-serial > $(BUILD)/flair_iword_print_mut.log 2>&1; then printf '!!! test-flair-iword-print-mutant FAIL: PRINT_NOFAIL survived\n'; exit 1; fi
	@grep -qF 'RED-REASON print-serial:' $(BUILD)/flair_iword_print_mut.log
	@printf '>>> test-flair-iword-print-mutant: green (PRINT_NOFAIL correctly RED: %s -- a refused LPT1 print is silently ignored)\n' "$$(grep -m1 'missing draw' $(BUILD)/flair_iword_print_mut.log)"
TEST_EMU_GATES += test-flair-iword-print test-flair-iword-print-mutant

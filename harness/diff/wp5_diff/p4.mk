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

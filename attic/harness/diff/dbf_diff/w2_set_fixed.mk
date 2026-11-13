# W2 SET FIXED: ACCEPTED real dBASE III PLUS 1.1 golden (initech-2e6lp).
# The unchanged capture xbase_prog_diff/set-mint/fixed/GOLD.OUT must go RED when
# xb_fixed_picture (the FIXED ON re-picture) is made a no-op.
.PHONY: test-samir-w2-set-fixed-mutant
$(BUILD)/test_samir_w2_mutant_set_fixed: $(DBF_DIFF_DIR)/test_samir_fns.c $(DBF_DIFF_DIR)/w2_host.c $(DBF_DIFF_DIR)/w2_host.h $(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) $(SAMIR_HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_W2 -DSAMIR_W2_MUTANT_set_fixed -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $(filter-out $(SAMIR_MAIN_SRC),$(filter %.c,$^))
test-samir-w2-set-fixed-mutant: test-samir-w2-set-fixed $(BUILD)/test_samir_w2_mutant_set_fixed
	@set +e; $(BUILD)/test_samir_w2_mutant_set_fixed set-fixed host > $(BUILD)/samir-w2-mut-set-fixed.log 2>&1; rc=$$?; set -e; \
	 test $$rc -eq 1 && grep -q 'FAIL.*W2 transcript' $(BUILD)/samir-w2-mut-set-fixed.log || { cat $(BUILD)/samir-w2-mut-set-fixed.log; exit 1; }; \
	 printf '>>> SAMIR_W2_MUTANT_set_fixed RED: '; grep 'FAIL.*W2 transcript' $(BUILD)/samir-w2-mut-set-fixed.log
TEST_UNIT_GATES += test-samir-w2-set-fixed test-samir-w2-set-fixed-mutant

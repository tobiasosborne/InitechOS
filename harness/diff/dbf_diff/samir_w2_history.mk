# W2a SET HISTORY / SET DOHISTORY / LIST|DISPLAY HISTORY: ACCEPTED gates.
# Real III+ 1.1 captures set-mint/history and set-mint/dohistory (pinned by
# samir-w2-goldens). The named PENDING gate is promoted into the default unit
# vector; each mutant perturbs one rule and must turn the transcript RED
# (Rule 6). Mutant binaries are private builds of the same W2 test driver.
SAMIR_W2_HISTORY_CASES := set-history set-dohistory
SAMIR_W2_HISTORY_MUTANTS := dohistory histtoggle histcap histlist
.PHONY: test-samir-w2-history test-samir-w2-history-mutant
test-samir-w2-history: $(addprefix test-samir-w2-,$(SAMIR_W2_HISTORY_CASES))
	@echo '>>> test-samir-w2-history: green (history, dohistory real III+ captures)'
test-samir-w2-history-mutant: $(SAMIR_W2_TEST) $(DBF_DIFF_DIR)/test_samir_fns.c os/samir/cmd/set.c | $(BUILD)
	@set -e; for m in $(SAMIR_W2_HISTORY_MUTANTS); do \
	 $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_W2 -DSAMIR_W2_MUTANT_$$m -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $(BUILD)/test_samir_w2_mut_$$m \
	  $(DBF_DIFF_DIR)/test_samir_fns.c $(DBF_DIFF_DIR)/w2_host.c $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) ; \
	 red=0; for c in $(SAMIR_W2_HISTORY_CASES); do \
	  set +e; $(BUILD)/test_samir_w2_mut_$$m $$c host > $(BUILD)/samirw2-mut-$$m-$$c.log 2>&1; rc=$$?; set -e; \
	  if [ $$rc -ne 0 ] && grep -q 'FAIL.*transcript' $(BUILD)/samirw2-mut-$$m-$$c.log; then red=1; fi; \
	 done; \
	 test $$red -eq 1 || { echo "!!! SAMIR_W2_MUTANT_$$m stayed green: the history goldens are decoration"; exit 1; }; \
	 echo ">>> SAMIR_W2_MUTANT_$$m RED"; \
	done
TEST_UNIT_GATES += test-samir-w2-history test-samir-w2-history-mutant

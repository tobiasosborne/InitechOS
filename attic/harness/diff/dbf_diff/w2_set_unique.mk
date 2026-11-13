# SET UNIQUE (initech-2e6lp W2a): ACCEPTED real-III+ gates, in the default vector.
# Evidence: set-mint/unique (build/omit/restore/bad toggle) and set-mint/uniquerebuild
# (INDEX ... UNIQUE keyword, SET UNIQUE ON, REINDEX keeps the index's own flag).
# Ref: ../dbase3-decomp/specs/commands/set-commands.md:165.
SAMIR_W2_UNIQUE_EXTRA := set/uniquerebuild
$(foreach rule,$(SAMIR_W2_UNIQUE_EXTRA),$(eval $(call samir-w2-pending-gate,$(rule))))
SAMIR_W2_UNIQUE_MUTANTS := dedupe rebuild set
.PHONY: samir-w2-unique-goldens test-samir-w2-unique test-samir-w2-unique-mutant
samir-w2-unique-goldens:
	@set -e; d=$(SAMIR_W2_MINT_TREE)/set-mint/uniquerebuild; (cd $$d && sha256sum --quiet -c sha256.txt); \
	 cmp $(SAMIR_W2_MINT_TREE)/corpus/w2/set/uniquerebuild.prg $$d/FNSTEST.PRG
	@echo '>>> samir-w2-unique-goldens: green'
test-samir-w2-unique: test-samir-w2-set-unique test-samir-w2-set-uniquerebuild samir-w2-unique-goldens
# Each mutant must turn at least one of the two real-III+ transcripts RED.
test-samir-w2-unique-mutant: test-samir-w2-unique
	@set -e; mkdir -p $(BUILD)/samirw2mut; for m in $(SAMIR_W2_UNIQUE_MUTANTS); do \
	 $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_W2 -DSAMIR_W2_MUTANT_unique_$$m -Iseed -I$(SAMIR_INC_DIR) -Ispec \
	  -o $(BUILD)/samirw2mut/unique_$$m $(DBF_DIFF_DIR)/test_samir_fns.c $(DBF_DIFF_DIR)/w2_host.c \
	  $(filter-out $(SAMIR_MAIN_SRC),$(filter %.c,$(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC))); \
	 red=0; for c in set-unique set-uniquerebuild; do \
	  if $(BUILD)/samirw2mut/unique_$$m $$c host > $(BUILD)/samirw2mut/unique_$$m.$$c.log 2>&1; then :; else red=1; fi; done; \
	 test $$red -eq 1 || { printf '!!! SAMIR_W2_MUTANT_unique_%s stayed GREEN\n' $$m; exit 1; }; \
	 printf '>>> SAMIR_W2_MUTANT_unique_%s RED\n' $$m; done
	@echo '>>> test-samir-w2-unique-mutant: green (every mutant caught)'
TEST_UNIT_GATES += test-samir-w2-unique test-samir-w2-unique-mutant

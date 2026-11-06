# initech-104j3: independent real III+ disk DO differential and mutations.
DO_FILE_TEST := $(BUILD)/test_interp_do_file
DO_FILE_ENG := $(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC)
$(DO_FILE_TEST): $(DBF_DIFF_DIR)/test_interp_do_file.c $(DO_FILE_ENG) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $^
define do-file-mutant
$(BUILD)/test_interp_do_file_$(1): $(DBF_DIFF_DIR)/test_interp_do_file.c $(DO_FILE_ENG) | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -D$(2) -Iseed -I$$(SAMIR_INC_DIR) -Ispec -o $$@ $$^
endef
$(eval $(call do-file-mutant,skip,PROC_MUTATE_SKIP_DISK))
$(eval $(call do-file-mutant,depth,PROC_MUTATE_DO_OFF_BY_ONE))
$(eval $(call do-file-mutant,master,PROC_MUTATE_MASTER_ONE_LEVEL))
$(eval $(call do-file-mutant,close,PROC_MUTATE_KEEP_LIBRARY_CLOSE))
$(eval $(call do-file-mutant,active,PROC_MUTATE_CLOSE_ACTIVE))
.PHONY: mint-interp-do-file test-interp-do-file test-interp-do-file-mutant test-interp-do-file-emu test-samir-do-file test-samir-do-file-mutant test-samir-do-file-emu
mint-interp-do-file:
	@sh $(DBF_DIFF_DIR)/do_file_mint.sh $(abspath $(DBASE3_DECOMP))
test-interp-do-file: $(DO_FILE_TEST)
	@sh $(DBF_DIFF_DIR)/do_file_test.sh host $<
	@printf '>>> test-interp-do-file: green\n'
test-interp-do-file-emu: $(SAMIR_EXE) $(SAMIR_OVL) $(SECONDARY_TRACER_IMG) $(HARNESS_BIN)
	@sh $(DBF_DIFF_DIR)/do_file_test.sh emu
	@printf '>>> test-interp-do-file-emu: green\n'
test-interp-do-file-mutant: test-interp-do-file $(addprefix $(BUILD)/test_interp_do_file_,skip depth master close active)
	@set -e; for mutant in skip depth master close active; do \
	 log=$(BUILD)/do-file-test/$$mutant.log; \
	 set +e; sh $(DBF_DIFF_DIR)/do_file_test.sh host $(BUILD)/test_interp_do_file_$$mutant > $$log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED DO-FILE' $$log || { cat $$log; exit 1; }; \
	 printf '>>> DO-FILE mutant %s correctly RED: ' "$$mutant"; grep -m1 '^RED DO-FILE' $$log; \
	 done
	@printf '>>> test-interp-do-file-mutant: green\n'
test-samir-do-file: test-interp-do-file
test-samir-do-file-mutant: test-interp-do-file-mutant
test-samir-do-file-emu: test-interp-do-file-emu
TEST_UNIT_GATES += test-samir-do-file test-samir-do-file-mutant
TEST_EMU_GATES += test-samir-do-file-emu

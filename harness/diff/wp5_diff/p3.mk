# initech-qvgd. Host oracle and QEMU gate precede implementation.
$(BUILD)/iword_fkeys_gen: tools/iword_fkeys_gen.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
spec/iword/fkeys.h: spec/iword/fkeys.json $(BUILD)/iword_fkeys_gen
	@# Source-tree header from a build-dir tool (initech-1x6x2.1): replace only
	@# when the bytes change, via a per-process temp name (private-BUILD sub-makes).
	@$(BUILD)/iword_fkeys_gen $< > $@.tmp.$$$$ \
		&& { if cmp -s $@.tmp.$$$$ $@; then rm -f $@.tmp.$$$$; else mv -f $@.tmp.$$$$ $@; fi; } \
		|| { rm -f $@.tmp.$$$$; exit 1; }
IWORD_P3_DEPS := harness/proptest/test_iword_p3.c harness/proptest/test_iword_face.c $(IWORD_APP_SRC) $(IWORD_APP_DEP)
$(BUILD)/test_iword_p3: $(IWORD_P3_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ harness/proptest/test_iword_p3.c $(IWORD_CORE_SRC)
$(BUILD)/test_iword_p3_mut_%: $(IWORD_P3_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DIWORD_MUT_$* -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ harness/proptest/test_iword_p3.c $(IWORD_CORE_SRC)
.PHONY: test-iword-fkeys test-iword-fkeys-mutant test-flair-iword-fkeys test-flair-iword-fkeys-mutant
test-iword-fkeys: $(BUILD)/test_iword_p3
	@$< fkeys
	@$(BUILD)/iword_fkeys_gen spec/iword/fkeys.json > $(BUILD)/iword_fkeys.expected.h
	@cmp spec/iword/fkeys.h $(BUILD)/iword_fkeys.expected.h
	@printf '>>> test-iword-fkeys: green\n'
test-iword-fkeys-mutant: $(BUILD)/test_iword_p3_mut_FKEY_DROP $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED
	@if $< fkeys > $<.log 2>&1; then printf 'FAIL: FKEY_DROP survived\n'; exit 1; fi
	@grep -F 'RED-REASON fkeys:' $<.log
	@if $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED fkeys > $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED.log 2>&1; then printf 'FAIL: FKEY_UNMAPPED survived\n'; exit 1; fi
	@grep -F 'RED-REASON fkeys:' $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED.log
	@printf '>>> test-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)\n'
test-flair-iword-fkeys: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/test_iword_p3 $(PPM_REGION_COUNT_BIN)
	$(call appl-boot,flair_iword_fkeys,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys.serial)
	@$(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys.serial
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_fkeys.ppm,7 430 79 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON fkeys: missing disabled-text pixels\n'; exit 1; }
	@printf '>>> test-flair-iword-fkeys: green\n'
$(eval $(call tenant-exe-rules,iword_fkey_drop,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_FKEY_DROP,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_FKEY_DROP_IMG := $(BUILD)/flair_data_iword_fkey_drop.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_FKEY_DROP_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_fkey_drop.exe,))
$(eval $(call tenant-exe-rules,iword_fkey_unmapped,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_FKEY_UNMAPPED,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_FKEY_UNMAPPED_IMG := $(BUILD)/flair_data_iword_fkey_unmapped.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_FKEY_UNMAPPED_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_fkey_unmapped.exe,))
test-flair-iword-fkeys-mutant: test-flair-iword-fkeys $(FLAIR_DATA_IWORD_FKEY_DROP_IMG) $(FLAIR_DATA_IWORD_FKEY_UNMAPPED_IMG)
	$(call appl-boot,flair_iword_fkeys_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_FKEY_DROP_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys_mut.serial)
	@if $(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys_mut.serial > $(BUILD)/iword_fkeys_mut.log 2>&1; then printf 'FAIL: FKEY_DROP survived emulator gate\n'; exit 1; fi
	@grep -F 'RED-REASON fkeys-serial:' $(BUILD)/iword_fkeys_mut.log
	@printf '>>> test-flair-iword-fkeys-mutant: FKEY_DROP correctly RED: End Field missing\n'
	$(call appl-boot,flair_iword_fkeys_unmapped,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_FKEY_UNMAPPED_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys_unmapped.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys_unmapped.serial)
	@if $(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys_unmapped.serial > $(BUILD)/iword_fkeys_unmapped_mut.log 2>&1; then printf 'FAIL: FKEY_UNMAPPED survived emulator gate\n'; exit 1; fi
	@grep -F 'FAIL template unmapped keys do not clear' $(BUILD)/iword_fkeys_unmapped_mut.log
	@printf '>>> test-flair-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)\n'
TEST_UNIT_GATES += test-iword-fkeys test-iword-fkeys-mutant
TEST_EMU_GATES += test-flair-iword-fkeys test-flair-iword-fkeys-mutant

.PHONY: test-iword-search test-flair-iword-search
test-iword-search: $(BUILD)/test_iword_p3
	@$< search
	@sh harness/diff/wp5_diff/search.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_search $(FAT_DUMP_BIN) $(BUILD)/test_iword_search_corpus
	@printf '>>> test-iword-search: green\n'
test-iword-search: $(BUILD)/test_iword_search_corpus $(FAT_DUMP_BIN)
$(BUILD)/test_iword_search_corpus: harness/diff/wp5_diff/test_iword_search_corpus.c $(IWORD_CORE_SRC) $(IWORD_CORE_HDR) | $(BUILD)
	$(CC) $(CFLAGS) $(IWORD_CORE_INC) -o $@ $< $(IWORD_CORE_SRC)
$(BUILD)/test_iword_search_corpus_mut_%: harness/diff/wp5_diff/test_iword_search_corpus.c $(IWORD_CORE_SRC) $(IWORD_CORE_HDR) | $(BUILD)
	$(CC) $(CFLAGS) -DIWORD_MUT_$* $(IWORD_CORE_INC) -o $@ $< $(IWORD_CORE_SRC)
IWORD_SEARCH_MUTANTS := SEARCH_BACK SEARCH_CASE SEARCH_CODE SEARCH_WRITE SEARCH_PROMPT SEARCH_CANCEL SEARCH_NOTFOUND SEARCH_CURSOR
IWORD_SEARCH_CORPUS_MUTANTS := SEARCH_BACK SEARCH_CASE SEARCH_CODE SEARCH_WRITE
.PHONY: test-iword-search-mutant
test-iword-search-mutant: test-iword-search $(foreach m,$(IWORD_SEARCH_MUTANTS),$(BUILD)/test_iword_p3_mut_$(m)) $(foreach m,$(IWORD_SEARCH_CORPUS_MUTANTS),$(BUILD)/test_iword_search_corpus_mut_$(m))
	@for m in $(IWORD_SEARCH_MUTANTS); do b=$(BUILD)/test_iword_p3_mut_$$m; \
		if $$b search > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
		grep -F 'RED-REASON search:' $$b.log || exit 1; \
		printf '>>> test-iword-search-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL search' $$b.log)"; \
	done
	@for m in $(IWORD_SEARCH_CORPUS_MUTANTS); do b=$(BUILD)/test_iword_search_corpus_mut_$$m; \
		if $$b $(BUILD)/iword_search/milton.wp5 > $$b.log 2>&1; then printf 'FAIL: corpus oracle let %s survive\n' "$$m"; exit 1; fi; \
		grep -F 'RED-REASON search-corpus:' $$b.log || exit 1; \
		printf '>>> test-iword-search-mutant: corpus %s correctly RED\n' "$$m"; \
	done
	@printf '>>> test-iword-search-mutant: green\n'
test-flair-iword-search: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/test_iword_p3 $(PPM_REGION_COUNT_BIN)
	$(call appl-boot,flair_iword_search,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_SEARCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_search.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_search.serial)
	@$(BUILD)/test_iword_p3 search-serial $(BUILD)/flair_iword_search.serial
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_search.ppm,7 430 111 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON search: missing message pixels\n'; exit 1; }
	@fsck.fat -n $(BUILD)/flair_iword_search_data.img
	@printf '>>> test-flair-iword-search: green\n'

define iword-search-emu-mutant
$(eval $(call tenant-exe-rules,iword_$(1),$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_$(2),$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_$(2)_IMG := $(BUILD)/flair_data_iword_$(1).img
$(eval $(call flair-data-rules,$(BUILD)/flair_data_iword_$(1).img,$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_$(1).exe,))
.PHONY: test-flair-iword-$(1)-mutant
test-flair-iword-$(1)-mutant: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(BUILD)/test_iword_p3 $(BUILD)/flair_data_iword_$(1).img
	$$(call appl-boot,flair_iword_$(1),$$(FLAIR_IWORD_GATE_IMG),$$(FLAIR_DATA_IWORD_$(2)_IMG),FLAIR_IWORD_SEARCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$$(call APPL_ALIVE,$$(BUILD)/flair_iword_$(1).serial) && $$(call IWORD_UP,$$(BUILD)/flair_iword_$(1).serial)
	@if $$(BUILD)/test_iword_p3 search-serial $$(BUILD)/flair_iword_$(1).serial > $$(BUILD)/iword_$(1)_mut.log 2>&1; then printf 'FAIL: $(2) survived emulator gate\n'; exit 1; fi
	@grep -F 'RED-REASON search-serial:' $$(BUILD)/iword_$(1)_mut.log
	@printf '>>> test-flair-iword-search-mutant: $(2) correctly RED: '; grep -m1 'FAIL search' $$(BUILD)/iword_$(1)_mut.log
	@printf '>>> test-flair-iword-$(1)-mutant: green ($(2) RED)\n'
endef
.PHONY: test-flair-iword-search-mutant
test-flair-iword-search-mutant: test-flair-iword-search test-flair-iword-search_back-mutant test-flair-iword-search_notfound-mutant
	@printf '>>> test-flair-iword-search-mutant: green (both named mutants RED)\n'
$(eval $(call iword-search-emu-mutant,search_back,SEARCH_BACK))
$(eval $(call iword-search-emu-mutant,search_notfound,SEARCH_NOTFOUND))
TEST_UNIT_GATES += test-iword-search test-iword-search-mutant
TEST_EMU_GATES += test-flair-iword-search test-flair-iword-search-mutant

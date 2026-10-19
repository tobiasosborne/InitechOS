# initech-qvgd remainder: List Files (F5) and Look. The independent golden is
# the REAL WordPerfect 5.1 mint ../wordperfect51-decomp mint/work/LISTF
# (mint-results-005.md): its cell-exact grids and its fixture volume, pinned
# here by md5 (the samir-use-goldens pattern). Host oracle and emulator gate
# were written and seen RED before the List Files implementation.
IWORD_LF_MINT := $(WORDPERFECT51_DECOMP)/mint/work/LISTF
IWORD_LF_MD5  := harness/diff/wp5_diff/listfiles_mint005.md5
iword-lf-goldens = cd "$(IWORD_LF_MINT)" && md5sum -c --quiet "$(CURDIR)/$(IWORD_LF_MD5)" || { printf '!!! $(1) FAIL: mint 005 files absent or changed under %s\n' "$(IWORD_LF_MINT)"; exit 1; }
IWORD_LF_DEPS := harness/proptest/test_iword_listfiles.c harness/proptest/test_iword_face.c $(IWORD_APP_SRC) $(IWORD_APP_DEP)
$(BUILD)/test_iword_listfiles: $(IWORD_LF_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ harness/proptest/test_iword_listfiles.c $(IWORD_CORE_SRC)
$(BUILD)/test_iword_listfiles_mut_%: $(IWORD_LF_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DIWORD_MUT_$* -Iseed -Ios/apps/tbx $(IWORD_CORE_INC) -o $@ harness/proptest/test_iword_listfiles.c $(IWORD_CORE_SRC)
# The files' bytes for the host mock: extracted from the md5-pinned mint volume.
IWORD_LF_HOST := $(BUILD)/iword_lf_host
IWORD_LF_FILES := MEMO LETTER.WP5 TABS.DOC README.TXT ZERO.TXT BIG.DAT HUGE.BIN HIDDEN.SYS
$(IWORD_LF_HOST)/.stamp: $(IWORD_LF_MD5) | $(BUILD)
	@$(call iword-lf-goldens,iword-lf-host)
	@rm -rf $(IWORD_LF_HOST) && mkdir -p $(IWORD_LF_HOST)/DOCS
	@set -e; for f in $(IWORD_LF_FILES); do mcopy -n -i "$(IWORD_LF_MINT)/lf005.img" "::DOCS/$$f" $(IWORD_LF_HOST)/DOCS/$$f; done
	@touch $@
.PHONY: test-iword-listfiles test-iword-listfiles-mutant
test-iword-listfiles: $(BUILD)/test_iword_listfiles $(IWORD_LF_HOST)/.stamp
	@$(call iword-lf-goldens,test-iword-listfiles)
	@$< "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST)
	@printf '>>> test-iword-listfiles: green\n'
IWORD_LF_MUTANTS := LF_COLUMN LF_ZERO_DATE
test-iword-listfiles-mutant: test-iword-listfiles $(foreach m,$(IWORD_LF_MUTANTS),$(BUILD)/test_iword_listfiles_mut_$(m))
	@for m in $(IWORD_LF_MUTANTS); do b=$(BUILD)/test_iword_listfiles_mut_$$m; \
	 if $$b "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
	 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON listfiles:' $$b.log || exit 1; \
	 printf '>>> test-iword-listfiles-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL ' $$b.log)"; done
	@printf '>>> test-iword-listfiles-mutant: green\n'

# The emulator volume: the stock data volume + the mint's A:\DOCS, every file's
# bytes and stamp copied out of the md5-pinned mint volume (mcopy -m keeps the
# DOS stamp), the directories made at their mint stamps (mkfixture.sh), and
# HIDDEN.SYS hidden. Its Free figure differs from the mint's (the stock
# volume's APPS / DOS fill it): the gate grades Free against mtools on the
# booted copy instead (the AH=36h figure; spec/toolbox_gate.h Sec 10a).
# $(1) the image, $(2) the stock-recipe base image.
define iword-lf-volume
$(1): $(2) $(IWORD_LF_MD5) | $(BUILD)
	@$$(call iword-lf-goldens,iword-lf-volume)
	@rm -rf $(1).d && mkdir -p $(1).d
	@cp -f $(2) $(1)
	@set -e; for f in $(IWORD_LF_FILES); do TZ=UTC mcopy -n -m -i "$(IWORD_LF_MINT)/lf005.img" "::DOCS/$$$$f" $(1).d/$$$$f; done
	@SOURCE_DATE_EPOCH=699350400 TZ=UTC mmd -i $(1) ::DOCS
	@SOURCE_DATE_EPOCH=699351000 TZ=UTC mmd -i $(1) ::DOCS/ARCHIVE
	@SOURCE_DATE_EPOCH=699488100 TZ=UTC mmd -i $(1) ::DOCS/WORK
	@set -e; for f in $(IWORD_LF_FILES); do TZ=UTC mcopy -m -i $(1) $(1).d/$$$$f ::DOCS/$$$$f; done
	@mattrib -i $(1) +h ::DOCS/HIDDEN.SYS
	@rm -rf $(1).d
	@printf '>>> iword list-files volume: %s (stock volume + the mint 005 A:\\DOCS)\n' '$(1)'
	@$$(call fixture-check,recipe,$(1))
endef
FLAIR_IWORD_LF_DATA := $(BUILD)/flair_iword_lf.img
$(eval $(call iword-lf-volume,$(FLAIR_IWORD_LF_DATA),$(FLAIR_DATA_IMG)))
$(eval $(call tenant-exe-rules,iword_lf_column,$(IWORD_APP_SRC),-Ios/iword/core -DIWORD_MUT_LF_COLUMN,$(IWORD_APP_DEP),))
FLAIR_DATA_IWORD_LF_COLUMN_IMG := $(BUILD)/flair_data_iword_lf_column.img
$(eval $(call flair-data-rules,$(FLAIR_DATA_IWORD_LF_COLUMN_IMG),$(TENANTFX_EXE),$(I123_EXE),$(BUILD)/tenant/iword_lf_column.exe,))
FLAIR_IWORD_LF_COLUMN_DATA := $(BUILD)/flair_iword_lf_column.img
$(eval $(call iword-lf-volume,$(FLAIR_IWORD_LF_COLUMN_DATA),$(FLAIR_DATA_IWORD_LF_COLUMN_IMG)))

# The gate: the locked trace; every screen the face drew (the TENANT-GATE
# DRAWCELLS trace, replayed into the recording mock between delivered keys)
# against the mint shot of the same key, Free against mdir of the booted copy.
iword-lf-free = mdir -a -i $(1) :: | sed -n 's/^ *\([0-9 ]*\) bytes free.*/\1/p' | tr -d ' '
.PHONY: test-flair-iword-listfiles test-flair-iword-listfiles-mutant
test-flair-iword-listfiles: $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_IWORD_LF_DATA) $(BUILD)/test_iword_listfiles $(IWORD_LF_HOST)/.stamp $(PPM_REGION_COUNT_BIN)
	@$(call iword-lf-goldens,test-flair-iword-listfiles)
	$(call appl-boot,flair_iword_lf,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_LF_DATA),FLAIR_IWORD_LISTFILES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_lf.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_lf.serial)
	@free=$$($(call iword-lf-free,$(BUILD)/flair_iword_lf_data.img)); [ -n "$$free" ] || { printf 'RED-REASON listfiles-serial: no mdir free figure\n'; exit 1; }; \
	 $(BUILD)/test_iword_listfiles "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) $(BUILD)/flair_iword_lf.serial $$free
	@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_lf.ppm,7 430 103 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON listfiles: no status-line pixels\n'; exit 1; }
	@fsck.fat -n $(BUILD)/flair_iword_lf_data.img > /dev/null
	@printf '>>> test-flair-iword-listfiles: green\n'
test-flair-iword-listfiles-mutant: test-flair-iword-listfiles $(FLAIR_IWORD_LF_COLUMN_DATA)
	$(call appl-boot,flair_iword_lf_column,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_LF_COLUMN_DATA),FLAIR_IWORD_LISTFILES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_iword_lf_column.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_lf_column.serial)
	@free=$$($(call iword-lf-free,$(BUILD)/flair_iword_lf_column_data.img)); \
	 if $(BUILD)/test_iword_listfiles "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) $(BUILD)/flair_iword_lf_column.serial $$free > $(BUILD)/iword_lf_column_mut.log 2>&1; then printf 'FAIL: LF_COLUMN survived the emulator gate\n'; exit 1; fi
	@grep -F 'RED-REASON listfiles-serial:' $(BUILD)/iword_lf_column_mut.log
	@printf '>>> test-flair-iword-listfiles-mutant: LF_COLUMN correctly RED: '; grep -m1 'FAIL ' $(BUILD)/iword_lf_column_mut.log
	@printf '>>> test-flair-iword-listfiles-mutant: green\n'
TEST_UNIT_GATES += test-iword-listfiles test-iword-listfiles-mutant
TEST_EMU_GATES += test-flair-iword-listfiles test-flair-iword-listfiles-mutant

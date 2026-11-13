# Ref: operator brief initech-2ter (a); PRD 6.3/6.4; Rule 6.
JTC_INC := -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -I$(BUILD)
JTC_MODEL := os/flair/egg_jtc_model.c
JTC_JSON := spec/easter/jump_to_conclusions.json
$(BUILD)/egg_jtc_pack: tools/egg_jtc_pack.c os/flair/egg_jtc_model.h | $(BUILD)
	$(CC) $(CFLAGS) $(JTC_INC) -o $@ $<
$(BUILD)/egg_jtc_data.bin: $(JTC_JSON) $(BUILD)/egg_jtc_pack
	$(BUILD)/egg_jtc_pack $< > $@
$(BUILD)/egg_jtc_data.c: $(BUILD)/egg_jtc_data.bin $(BIN2C_BIN)
	$(BIN2C_BIN) $< egg_jtc_data > $@
JTC_HOST_SRC := harness/proptest/test_egg_jtc.c $(JTC_MODEL) $(BUILD)/egg_jtc_data.c os/flair/event.c
$(BUILD)/test_egg_jtc: $(JTC_HOST_SRC) os/flair/egg_jtc_model.h
	$(CC) $(CFLAGS) $(JTC_INC) -o $@ $(JTC_HOST_SRC)
$(BUILD)/test_egg_jtc_mutant: $(JTC_HOST_SRC) os/flair/egg_jtc_model.h
	$(CC) $(CFLAGS) $(JTC_INC) -DEGG_MUT_PAD_MISSING -o $@ $(JTC_HOST_SRC)
.PHONY: test-egg-jtc test-egg-jtc-mutant
test-egg-jtc: $(BUILD)/test_egg_jtc
	@$<
test-egg-jtc-mutant: test-egg-jtc $(BUILD)/test_egg_jtc_mutant
	@if $(BUILD)/test_egg_jtc_mutant > $(BUILD)/egg_jtc_mutant.log 2>&1; then printf '!!! EGG_MUT_PAD_MISSING survived\n'; exit 1; fi
	@grep -x 'FAIL JTC pad count (expected 10)' $(BUILD)/egg_jtc_mutant.log
	@printf '>>> test-egg-jtc-mutant: EGG_MUT_PAD_MISSING RED (pad count)\n'
TEST_UNIT_GATES += test-egg-jtc test-egg-jtc-mutant
# Size round 7 (initech-5qsm): the strings are packed back to back; a walker
# that skips one string too many must go RED on the literal title.
$(BUILD)/test_egg_jtc_cell_mutant: $(JTC_HOST_SRC) os/flair/egg_jtc_model.h
	$(CC) $(CFLAGS) $(JTC_INC) -DEGG_MUT_CELL_WALK -o $@ $(JTC_HOST_SRC)
.PHONY: test-egg-jtc-cell-mutant
test-egg-jtc-cell-mutant: test-egg-jtc $(BUILD)/test_egg_jtc_cell_mutant
	@if $(BUILD)/test_egg_jtc_cell_mutant > $(BUILD)/egg_jtc_cell_mutant.log 2>&1; then printf '!!! EGG_MUT_CELL_WALK survived\n'; exit 1; fi
	@grep -x 'FAIL JTC title' $(BUILD)/egg_jtc_cell_mutant.log
	@printf '>>> test-egg-jtc-cell-mutant: EGG_MUT_CELL_WALK RED (title)\n'
TEST_UNIT_GATES += test-egg-jtc-cell-mutant

.PHONY: test-egg-jtc-pubs
test-egg-jtc-pubs: pubs-inventory
	@! grep -Fi 'Jump to Conclusions' $(PUBS_CHECK)/shipped
	@! grep -RFi 'Jump to Conclusions' pubs --include='*.tex' --include='*.cmds'
	@printf '>>> test-egg-jtc-pubs: green (hidden title absent from shipped inventory and manuals; no exemption needed)\n'
TEST_UNIT_GATES += test-egg-jtc-pubs

# ADR-0014 lane 1: egg_jtc.o is in section COLD of C:\FLAIR.OVL, not a resident
# object -- native (non-LTO) code, kernel ABI, linked by overlay.ld.
$(BUILD)/egg_jtc.o: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(JTC_INC) -c $< -o $@
# ADR-0014 lane 1: About This Computer joins section COLD (measured -396 B resident).
$(BUILD)/about_box_ovl.o: os/milton/about_box.c os/milton/about_box.h os/milton/kmain_cold.h os/milton/int21.h os/milton/boot_info.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) -Ios/milton -Ispec -c $< -o $@
$(BUILD)/kmain_cold_ovl.o: os/milton/kmain_cold.c os/milton/kmain_cold.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) -Ios/milton -c $< -o $@
$(KERNEL_FLAIRTENANTS_MAIN_OBJ): os/milton/about_box.h
$(BUILD)/egg_jtc_model.o: $(JTC_MODEL) os/flair/egg_jtc_model.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) $(JTC_INC) -c $< -o $@
$(BUILD)/egg_jtc_model_mutant.o: $(JTC_MODEL) os/flair/egg_jtc_model.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) $(JTC_INC) -DEGG_MUT_TRIGGER_OFF_BY_ONE -c $< -o $@
$(BUILD)/egg_jtc_data.o: $(BUILD)/egg_jtc_data.c
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) -c $< -o $@
$(KERNEL_FLAIRTENANTS_MAIN_OBJ): os/flair/egg_jtc.h os/flair/egg_jtc_model.h

include spec/flair_egg_jtc_traces.mk
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_TEN_TICK_BUDGET=1000,jtc_gate))
JTC_IMAGE := $(BUILD)/flair_tenants_mut_jtc_gate.img
JTC_CHECK := $(BUILD)/ppm_flair_egg_jtc_check
$(JTC_CHECK): tools/ppm_flair_egg_jtc_check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lpng

# Own data copies, fixed RTC, bounded gate budget. No Bochs legs in this lane.
define jtc-boot
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(1)_data.img
	@$(HARNESS_BIN) --disk "$(2)" --disk2 "$(BUILD)/$(1)_data.img" --name "$(1)" --out "$(BUILD)" \
	 --mouse "$($(3))" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK \
	 --rtc-base "1997-08-01T09:00:00" --timeout-ms 40000 --screendump --screendump-after "$(4)" \
	 2> $(BUILD)/$(1).report || true
	@! grep -Eq 'triple_fault=1' $(BUILD)/$(1).report
	@grep -qx 'FLAIR-LIVE-OK' $(BUILD)/$(1).serial && ! grep -Eq '^(PANIC|TENANT-CRASH)|KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/$(1).serial
endef
define jtc-short-check
if grep -q '^EGG-JTC-' $(1); then printf 'FAIL JTC trigger: two clicks opened mat\n'; exit 1; fi
endef
.PHONY: test-flair-egg-jtc
test-flair-egg-jtc: $(HARNESS_BIN) $(JTC_IMAGE) $(FLAIR_DATA_IMG) $(JTC_CHECK)
	$(call jtc-boot,egg_jtc_mat,$(JTC_IMAGE),FLAIR_EGG_JTC_OPEN_SPEC,EGG-JTC-OPEN click=3)
	@grep -qx 'EGG-JTC-OPEN click=3' $(BUILD)/egg_jtc_mat.serial || { printf 'FAIL JTC trigger: expected third click\n'; exit 1; }
	@$(JTC_CHECK) $(BUILD)/egg_jtc_mat.ppm fixtures/easter/jtc_mat.png
	$(call jtc-boot,egg_jtc_alert,$(JTC_IMAGE),FLAIR_EGG_JTC_ALERT_SPEC,EGG-JTC-ALERT up)
	@grep -qx 'EGG-JTC-JUMP pad=8 x=218 y=372' $(BUILD)/egg_jtc_alert.serial
	@grep -qx 'FLAIR-CURSOR x=218 y=372' $(BUILD)/egg_jtc_alert.serial
	@grep -qx 'EGG-JTC-ALERT closed' $(BUILD)/egg_jtc_alert.serial
	@$(JTC_CHECK) $(BUILD)/egg_jtc_alert.ppm fixtures/easter/jtc_alert.png
	$(call jtc-boot,egg_jtc_return,$(JTC_IMAGE),FLAIR_EGG_JTC_SPEC,EGG-JTC-ALERT up)
	@grep -qx 'EGG-JTC-ALERT closed' $(BUILD)/egg_jtc_return.serial && ! grep -q 'FLAIR-TRACK-EXPIRED' $(BUILD)/egg_jtc_return.serial
	$(call jtc-boot,egg_jtc_short,$(JTC_IMAGE),FLAIR_EGG_JTC_SHORT_SPEC,FLAIR-LIVE-OK)
	@$(call jtc-short-check,$(BUILD)/egg_jtc_short.serial)
	@printf '>>> test-flair-egg-jtc: green (third-click trigger, modeless mat, cursor jump, alert, exact authored stills)\n'
TEST_EMU_GATES += test-flair-egg-jtc

.PHONY: test-flair-egg-jtc-mutant
test-flair-egg-jtc-mutant: test-flair-egg-jtc $(BUILD)/flair_tenants_mut_jtc_trigger.img
	$(call jtc-boot,egg_jtc_mutant,$(BUILD)/flair_tenants_mut_jtc_trigger.img,FLAIR_EGG_JTC_SHORT_SPEC,FLAIR-LIVE-OK)
	@if ( $(call jtc-short-check,$(BUILD)/egg_jtc_mutant.serial) ) > $(BUILD)/egg_jtc_trigger_mutant.log 2>&1; then printf 'FAIL JTC mutant: oracle accepted two clicks\n'; exit 1; fi
	@grep -qx 'EGG-JTC-OPEN click=2' $(BUILD)/egg_jtc_mutant.serial || { printf 'FAIL JTC mutant: did not fail on early trigger\n'; exit 1; }
	@grep -x 'FAIL JTC trigger: two clicks opened mat' $(BUILD)/egg_jtc_trigger_mutant.log
	@printf '>>> test-flair-egg-jtc-mutant: EGG_MUT_TRIGGER_OFF_BY_ONE RED (two clicks opened mat)\n'
TEST_EMU_GATES += test-flair-egg-jtc-mutant

RECORD_SCRIPTS += jump_to_conclusions
RECORD_SPEC_jump_to_conclusions := $(FLAIR_EGG_JTC_SPEC)
RECORD_MARKER_jump_to_conclusions := EGG-JTC-ALERT closed
RECORD_IMAGE_jump_to_conclusions := $(FLAIRTENANTS_RECORD_IMG)

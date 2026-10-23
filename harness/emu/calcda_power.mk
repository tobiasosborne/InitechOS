# initech-0xs6; no locked spec is changed. Inputs are derived from
# spec/flair_calcda_traces.mk (Apple launch), spec/flair_power_traces.mk
# (Finder Special rows), and the audited disk click (600,64).
CALC_POWER_OPEN := m-125:-85,m-125:-85,l1,l0,w400,m-50:-61,l1,w300,m40:41,w300,l0,w800
# (60,50) -> (600,64), single click makes Finder foreground;
# (600,64) -> Special (170,30). Restart y109, Shut Down y125.
CALC_POWER_FINDER := m100:0,m100:0,m100:0,m100:0,m100:0,m40:14,l1,l0,w200,m-100:0,m-100:0,m-100:0,m-100:0,m-30:-34,l1,w100
CALC_POWER_SD := $(CALC_POWER_OPEN),$(CALC_POWER_FINDER),m0:95,l0,w500,m100:67,m100:67,m100:67,m100:67,m50:67,kctrl-n
CALC_POWER_RS := $(CALC_POWER_OPEN),$(CALC_POWER_FINDER),m0:79,l0

CALC_ROUTE_SRC := harness/proptest/test_calcda_route.c $(CALCDA_APP_SRC) $(CALCDA_APP_DEP) os/apps/tbx/tbx.h
$(BUILD)/test_calcda_route: $(CALC_ROUTE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -Ios/apps/tbx -Ios/apps/calcda -Ispec -Ispec/assets -o $@ harness/proptest/test_calcda_route.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c
.PHONY: test-calcda-route
test-calcda-route: $(BUILD)/test_calcda_route
	@$<
test-calcda: test-calcda-power

define calc-power-boot
@$(call fixture-preflight,$(3))
cp -f $(3) $(BUILD)/$(1)_data.img
INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(FLAIR_CALCDA_GATE_IMG)" --disk2 "$(BUILD)/$(1)_data.img" --name "$(1)" --out "$(BUILD)" --mouse "$(2)" --keys-after FLAIR-LIVE-READY --quit-after "FLAIR-EVT what=3 where=" --timeout-ms 40000 --screendump --screendump-after "$(4)" 2> $(BUILD)/$(1).report || true
@grep -q 'OK=1' $(BUILD)/$(1).report && ! grep -q 'triple_fault=1' $(BUILD)/$(1).report && ! grep -qE '^(PANIC|TENANT-CRASH)' $(BUILD)/$(1).serial || { cat $(BUILD)/$(1).report; exit 1; }
endef
.PHONY: test-flair-calcda-power
test-flair-calcda-power: $(HARNESS_BIN) $(FLAIR_CALCDA_GATE_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_SHUTDOWN_CHECK_BIN)
	$(call calc-power-boot,calc_power_sd,$(CALC_POWER_SD),$(FLAIR_DATA_IMG),FLAIR-EVT what=3 where=)
	@sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_sd.serial SHUTDOWN
	@$(PPM_FLAIR_SHUTDOWN_CHECK_BIN) safe $(BUILD)/calc_power_sd.ppm
	$(call pw-reboot,$(FLAIR_CALCDA_GATE_IMG),calc_power_rs,$(BUILD)/calc_power_rs_data.img,$(CALC_POWER_RS))
	@sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_rs.serial RESTART
	@grep -q 'OK=1' $(BUILD)/calc_power_rs.report

PPM_POWER_NOTICE := $(BUILD)/ppm_flair_power_notice_check
$(PPM_POWER_NOTICE): tools/ppm_flair_power_notice_check.c tools/ppm_flair_trash_check.c harness/render/alert_capture.h harness/render/icon_capture.h spec/assets/color_canon.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec/assets $(ICON_CAPTURES_DEF) -o $@ $< -lpng

# Close-only is the exact pre-fix Calculator, on a private fixture volume.
$(eval $(call tenant-exe-rules,calcda_close_only,$(CALCDA_APP_SRC),-Ios/apps/calcda -Ispec/assets -DCALCDA_MUT_CLOSE_ONLY,$(CALCDA_APP_DEP),))
$(BUILD)/flair_data_calcda_close_only.img: $(FLAIR_DATA_IMG) $(BUILD)/tenant/calcda_close_only.exe
	cp -f $< $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/tenant/calcda_close_only.exe ::APPS/CALCULAT.EXE

$(BUILD)/test_calcda_close_only: $(CALC_ROUTE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -DCALCDA_MUT_CLOSE_ONLY -Ios/apps/tbx -Ios/apps/calcda -Ispec -Ispec/assets -o $@ harness/proptest/test_calcda_route.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c
$(BUILD)/test_power_notice: harness/proptest/test_power_notice.c os/flair/power_notice.h spec/assets/chicago12.h | $(BUILD)
	$(CC) $(CFLAGS) -Ios/flair -Ispec/assets -o $@ $<
$(BUILD)/test_power_notice_mutant: harness/proptest/test_power_notice.c os/flair/power_notice.h spec/assets/chicago12.h | $(BUILD)
	$(CC) $(CFLAGS) -DPOWER_MUT_SILENT_CANCEL -Ios/flair -Ispec/assets -o $@ $<
.PHONY: test-calcda-power test-calcda-power-mutant test-flair-calcda-power-mutant test-flair-power-notice

test-calcda-power: test-calcda-route $(BUILD)/test_power_notice
	@$(BUILD)/test_power_notice
test-calcda-power-mutant: test-calcda-power $(BUILD)/test_calcda_close_only $(BUILD)/test_power_notice_mutant
	@if $(BUILD)/test_calcda_close_only > $(BUILD)/calcda_close_only.log; then echo 'FAIL Close-only survived'; exit 1; fi
	@grep -qxF 'FAIL shutdown has an explicit enabled Quit route' $(BUILD)/calcda_close_only.log
	@if $(BUILD)/test_power_notice_mutant > $(BUILD)/power_notice_mutant.log; then echo 'FAIL silent refusal survived'; exit 1; fi
	@grep -qxF 'FAIL power-notice: cancellation must name the app and reason' $(BUILD)/power_notice_mutant.log
	@echo 'GREEN calcda-power mutants: Close-only and silent cancellation both RED'

test-flair-calcda-power-mutant: $(HARNESS_BIN) $(FLAIR_CALCDA_GATE_IMG) $(BUILD)/flair_data_calcda_close_only.img $(PPM_POWER_NOTICE)
	$(call calc-power-boot,calc_power_close_only,$(CALC_POWER_SD),$(BUILD)/flair_data_calcda_close_only.img,FLAIR-EVT what=3 where=)
	@if sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_close_only.serial SHUTDOWN > $(BUILD)/calc_power_close_only.check; then echo 'FAIL Close-only survived emulator'; exit 1; fi
	@grep -qxF 'FAIL calcda-power: Calculator was not asked to quit' $(BUILD)/calc_power_close_only.check
	@grep -qxF 'FINDER-POWER-CANCEL app=CALCULAT.EXE cannot-ask' $(BUILD)/calc_power_close_only.serial
	@! grep -qE '^(TENANT-EXIT|FLAIR-SHUTDOWN|FLAIR-RESTART)' $(BUILD)/calc_power_close_only.serial
	@$(PPM_POWER_NOTICE) $(BUILD)/calc_power_close_only.ppm
	@echo 'GREEN calcda-power: Close-only mutant RED; unaskable app explained on screen'

# The existing declining-app control must acknowledge the new visible notice
# before its Ctrl-N probe. The locked original trace remains unchanged.
# Dirty TEXT: launch prefix from flair_itext_traces.mk, then one edit.
CALC_POWER_TEXT_OPEN := m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:72,l1,l0,l1,l0,w400,ka
# TEXT launch ends (211,174); click volume (600,64), then Finder Special.
CALC_POWER_TEXT_HELD := $(CALC_POWER_TEXT_OPEN),m100:-55,m100:-55,m100:0,m89:0,l1,l0,m-100:0,m-100:0,m-100:0,m-100:0,m-30:-34,l1,m0:95,l0,w400,m100:67,m100:67,m100:67,m100:67,m50:67,kesc,w700
CALC_POWER_TEXT_SD := $(CALC_POWER_TEXT_HELD),kret,w200,kctrl-n

define calc-text-cancel-boot
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/$(1)_data.img
INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(2)" --disk2 "$(BUILD)/$(1)_data.img" --name "$(1)" --out "$(BUILD)" --mouse "$(3)" --keys-after FLAIR-LIVE-READY --quit-after '$(4)' --timeout-ms 40000 $(if $(5),--screendump --screendump-after '$(5)') 2> $(BUILD)/$(1).report || true
@grep -q 'OK=1' $(BUILD)/$(1).report && ! grep -q 'triple_fault=1' $(BUILD)/$(1).report || { cat $(BUILD)/$(1).report; exit 1; }
endef

test-flair-power-notice: $(HARNESS_BIN) $(FLAIR_CALCDA_GATE_IMG) $(FLAIR_DATA_IMG) $(PPM_POWER_NOTICE)
	$(call calc-text-cancel-boot,power_text_notice,$(FLAIR_CALCDA_GATE_IMG),$(CALC_POWER_TEXT_HELD),FINDER-POWER-NOTICE,FINDER-POWER-NOTICE)
	@grep -qxF 'FINDER-POWER-CANCEL app=TEXT.EXE declined' $(BUILD)/power_text_notice.serial
	@$(PPM_POWER_NOTICE) $(BUILD)/power_text_notice.ppm
	$(call calc-text-cancel-boot,power_text_cancel,$(FLAIR_CALCDA_GATE_IMG),$(CALC_POWER_TEXT_SD),FINDER-NEW-FOLDER name=)
	@sh harness/emu/power_notice_check.sh $(BUILD)/power_text_cancel.serial
	@fsck.fat -n $(BUILD)/power_text_cancel_data.img > $(BUILD)/power_text_cancel.fsck

$(eval $(call flair-tenants-kmain-mutant-rules,POWER_MUT_SILENT_CANCEL -DFLAIR_TEN_TICK_BUDGET=1000,power_silent_cancel))
.PHONY: test-flair-power-notice-mutant
test-flair-power-notice-mutant: $(HARNESS_BIN) $(BUILD)/flair_tenants_mut_power_silent_cancel.img $(FLAIR_DATA_IMG) $(PPM_POWER_NOTICE)
	$(call calc-text-cancel-boot,power_text_silent,$(BUILD)/flair_tenants_mut_power_silent_cancel.img,$(CALC_POWER_TEXT_SD),FINDER-NEW-FOLDER name=,FINDER-NEW-FOLDER name=)
	@if sh harness/emu/power_notice_check.sh $(BUILD)/power_text_silent.serial > $(BUILD)/power_text_silent.check; then echo 'FAIL silent cancellation survived emulator'; exit 1; fi
	@grep -qxF 'FAIL power-notice: no visible explanation' $(BUILD)/power_text_silent.check
	@if $(PPM_POWER_NOTICE) $(BUILD)/power_text_silent.ppm > $(BUILD)/power_text_silent.pixels 2>&1; then echo 'FAIL absent notice survived pixels'; exit 1; fi
	@grep -qxF 'FAIL power-notice pixels' $(BUILD)/power_text_silent.pixels
	@echo 'GREEN power-notice: serial-only cancellation mutant RED'

test-calcda-mutant: test-calcda-power-mutant
TEST_UNIT_GATES += test-calcda-power test-calcda-power-mutant
TEST_EMU_GATES += test-flair-calcda-power test-flair-calcda-power-mutant test-flair-power-notice test-flair-power-notice-mutant

# PM records on main per BEAD.txt: make record-flair SCRIPT=calculator_power
RECORD_SCRIPTS += calculator_power calculator_refused
RECORD_SPEC_calculator_power = $(CALC_POWER_SD)
RECORD_MARKER_calculator_power = APPLE-LAUNCH name=CALCULAT.EXE dir=APPS
RECORD_IMAGE_calculator_power = $(FLAIRTENANTS_RECORDLONGDBL_IMG)
RECORD_DATA_calculator_power = $(FLAIR_DATA_IMG)
RECORD_SETTLE_calculator_power = 300
RECORD_SPEC_calculator_refused = $(CALC_POWER_TEXT_SD)
RECORD_MARKER_calculator_refused = FINDER-POWER-NOTICE
RECORD_IMAGE_calculator_refused = $(FLAIRTENANTS_RECORDLONGDBL_IMG)
RECORD_DATA_calculator_refused = $(FLAIR_DATA_IMG)
RECORD_SETTLE_calculator_refused = 300

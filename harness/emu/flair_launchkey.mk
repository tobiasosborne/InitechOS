# initech-rex3 / Rule 1/6/8/14. Manual p. 1-17 pins nine '-' in A1
# (the locked Face A default column width is nine). No rendered golden minted.
include spec/flair_launchkey_traces.mk
LAUNCHKEY_DATA := $(BUILD)/launchkey_data.img
$(LAUNCHKEY_DATA): $(FLAIR_DATA_IMG) $(BUILD)/SAMIR.EXE
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $(FLAIR_DATA_IMG) $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/SAMIR.EXE ::SAMIR.EXE
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/SAMIR.OVL ::SAMIR.OVL
	@$(call fixture-check,recipe,$@,$(FLAIR_FIXTURES) SAMIR.EXE SAMIR.OVL)

# Shared verbatim by the clean gate and its one-branch mutant.
LAUNCHKEY_IMAGE ?= $(BUILD)/flair_tenants_mut_launchkey_gate.img
LAUNCHKEY_TAG ?= launchkey
LAUNCHKEY_REPEAT = grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 .* fg=1 bg=0 -> 0$$' $(1) | tail -1 | grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=33 y=64 s="---------                                                               " fg=1 bg=0 -> 0'
LAUNCHKEY_RECOVERY = ! grep -q '^PANIC' $(1) && \
 [ "$$(grep -c '^FLAIR-SUSPEND$$' $(1))" = 1 ] && \
 [ "$$(grep -c '^FLAIR-RESUME$$' $(1))" = 1 ] && \
 grep -qx 'FLAIR-TEXT-EXIT rc=0' $(1) && $(call LAUNCHKEY_REPEAT,$(1)) && \
 sed -n '/^FLAIR-RESUME$$/,$$p' $(1) | grep -q '^TENANT-GATE ax=0x0033 win=1 x=105 y=64 '

.PHONY: test-flair-launchkey test-flair-launchkey-mutant
test-flair-launchkey: $(HARNESS_BIN) $(LAUNCHKEY_IMAGE) $(LAUNCHKEY_DATA)
	$(call appl-boot,$(LAUNCHKEY_TAG)_plain,$(LAUNCHKEY_IMAGE),$(LAUNCHKEY_DATA),FLAIR_LAUNCHKEY_PLAIN_SPEC,)
	@grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/$(LAUNCHKEY_TAG)_plain.serial && \
	 ! grep -q '^FLAIR-SUSPEND' $(BUILD)/$(LAUNCHKEY_TAG)_plain.serial && \
	 $(call LAUNCHKEY_REPEAT,$(BUILD)/$(LAUNCHKEY_TAG)_plain.serial) \
	 || { printf '!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR\n'; exit 1; }
	@$(call APPL_ALIVE,$(BUILD)/$(LAUNCHKEY_TAG)_plain.serial) || { printf '!!! test-flair-launchkey FAIL: plain desktop not alive\n'; exit 1; }
	@printf '>>> test-flair-launchkey: plain backslash-minus draws nine hyphens in A1; no SAMIR launch\n'
	$(call appl-boot,$(LAUNCHKEY_TAG)_resume,$(LAUNCHKEY_IMAGE),$(LAUNCHKEY_DATA),FLAIR_LAUNCHKEY_RESUME_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/$(LAUNCHKEY_TAG)_resume.serial) && \
	 $(call LAUNCHKEY_RECOVERY,$(BUILD)/$(LAUNCHKEY_TAG)_resume.serial) \
	 || { printf '!!! test-flair-launchkey FAIL: chord / QUIT / sheet recovery\n'; exit 1; }
	@printf '>>> test-flair-launchkey: green (Ctrl+Alt+backslash launches SAMIR; QUIT resumes sheet; Right reaches B1)\n'

test-flair-launchkey-mutant: $(HARNESS_BIN) $(LAUNCHKEY_DATA) $(BUILD)/flair_tenants_mut_plain_bsl_launch.img
	@set +e; INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory -j3 test-flair-launchkey \
	 LAUNCHKEY_IMAGE=$(BUILD)/flair_tenants_mut_plain_bsl_launch.img LAUNCHKEY_TAG=launchkey_mut \
	 >$(BUILD)/launchkey-mutant.log 2>&1; rc=$$?; set -e; \
	 test $$rc -ne 0 && grep -q '^FLAIR-SUSPEND$$' $(BUILD)/launchkey_mut_plain.serial && \
	 ! grep -q 'triple_fault=1' $(BUILD)/launchkey_mut_plain.report && \
	 grep -F '!!! test-flair-launchkey FAIL: plain backslash did not draw repeat label without SAMIR' $(BUILD)/launchkey-mutant.log \
	 || { cat $(BUILD)/launchkey-mutant.log; exit 1; }
	@printf '>>> test-flair-launchkey-mutant: green (FLAIR_MUT_PLAIN_BSL_LAUNCH RED: plain backslash launched SAMIR)\n'

# Register before record-flair's prerequisite expansion. Same locked trace.
RECORD_SCRIPTS += launchkey
RECORD_SPEC_launchkey = $(FLAIR_LAUNCHKEY_RESUME_SPEC)
RECORD_IMAGE_launchkey = $(FLAIRTENANTS_RECORDDBL_IMG)
RECORD_DATA_launchkey = $(LAUNCHKEY_DATA)
RECORD_MARKER_launchkey = FLAIR-RESUME
RECORD_SETTLE_launchkey = 400
.PHONY: test-flair-launchkey-record
test-flair-launchkey-record:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory -j3 record-flair SCRIPT=launchkey
	@# A recorder stops after its final frame, before the bounded LIVE-OK.
	@grep -qx '\[harness\] OK=1' $(RECORD_CLIPS_DIR)/rec_launchkey.report
	@$(call LAUNCHKEY_RECOVERY,$(RECORD_CLIPS_DIR)/rec_launchkey.serial) \
	 || { printf '!!! test-flair-launchkey-record FAIL: repeating label / SAMIR / QUIT / resumed sheet\n'; exit 1; }
	@printf '>>> test-flair-launchkey-record: green (locked trace clip + repeat label + SAMIR exit + resumed B1)\n'
# Standalone lane gates: acceptance failures stay visible by name; the PM
# owns default-vector admission after the bounded trace and clip are green.

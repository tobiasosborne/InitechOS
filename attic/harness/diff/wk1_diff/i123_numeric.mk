# B07/B08: fixed entry and clock trace, Reference pp. 1-19 and 13-46.
# Same real app and desktop as neighbouring gates. w2000 waits two seconds
# while the running guest clock advances; saved serials are checked separately.
I123_NUMERIC_TRACE = $(FLAIR_I123_FILE_LAUNCH),kshift-4,k1,k2,kdot,k3,k4,kret,kright,kshift-2,kn,ko,kw,kret,kslash,kf,ks,kn,ko,kw,k1,kret,WSaved now1.wk1,w2000,kf9,kslash,kf,ks,kn,ko,kw,k2,kret,WSaved now2.wk1
RECORD_SCRIPTS += i123_numeric
RECORD_SPEC_i123_numeric = $(I123_NUMERIC_TRACE)
RECORD_IMAGE_i123_numeric = $(FLAIRTENANTS_RECORDLONGDBL_IMG)
RECORD_RTC_BASE_i123_numeric = 1992-03-09T12:00:00
RECORD_MARKER_i123_numeric = TENANT-REGISTER ok app=123
# The first repro differed ONLY at frame 21: one grab preceded 123's window
# paint, the other followed it. Match the neighbouring i123_files capture
# settle; the exact GIF/MP4 byte comparison and all application gates stay.
RECORD_SETTLE_i123_numeric = 400

$(BUILD)/check_i123_numeric: harness/diff/wk1_diff/check_i123_numeric.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
.PHONY: test-flair-i123-numeric
test-flair-i123-numeric: export INITECH_QMP_STDIO=1
test-flair-i123-numeric: $(HARNESS_BIN) $(FLAIRTENANTS_RECORDDBL_IMG) $(FLAIR_DATA_IMG) $(BUILD)/check_i123_numeric
	$(call need_lotus_goldens,test-flair-i123-numeric)
	$(call appl-boot,flair_i123_numeric,$(FLAIRTENANTS_RECORDDBL_IMG),$(FLAIR_DATA_IMG),I123_NUMERIC_TRACE,--rtc-base 1992-03-09T12:00:00 --rtc-deterministic --screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_i123_numeric.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_numeric.serial
	@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_numeric.serial
	@mcopy -o -i $(BUILD)/flair_i123_numeric_data.img ::NOW1.WK1 $(BUILD)/i123-NOW1.WK1
	@mcopy -o -i $(BUILD)/flair_i123_numeric_data.img ::NOW2.WK1 $(BUILD)/i123-NOW2.WK1
	@$(BUILD)/check_i123_numeric $(BUILD)/i123-NOW1.WK1 $(BUILD)/i123-NOW2.WK1
	@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-NOW2.WK1 > $(BUILD)/i123-NOW2.ref
	@grep -q 'consumed=.* OK$$' $(BUILD)/i123-NOW2.ref
	@printf '>>> test-flair-i123-numeric: green\n'
TEST_EMU_GATES += test-flair-i123-numeric

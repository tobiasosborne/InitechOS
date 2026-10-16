# initech-cqej. Fixture ONLY: replace CTENANT at the same directory slot;
# do not grow the shipped APPS volume or change the consenting tenant.
# Ref: kmain.c finder_power_ask_apps, tbxgate.c tbx_ask_quit; Inside Macintosh
# VI PDF p.345 Listing 6-8, cited at both seams. The existing LOCKED trace
# FLAIR_SHUTDOWN_APP_SPEC brings Finder forward and probes with Ctrl-N.
$(eval $(call tenant-exe-rules,ctenant_decline,os/apps/ctenant/ctenant.c,-DCTENANT_DECLINE_QUIT,,))
FLAIR_DATA_POWER_CANCEL_IMG := $(BUILD)/flair_data_power_cancel.img
$(FLAIR_DATA_POWER_CANCEL_IMG): $(FLAIR_DATA_IMG) $(BUILD)/tenant/ctenant_decline.exe
	cp -f $< $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/tenant/ctenant_decline.exe ::APPS/CTENANT.EXE
	@$(call fixture-check,recipe,$@)

define power-cancel-boot
@$(call fixture-preflight,$(FLAIR_DATA_POWER_CANCEL_IMG))
cp -f $(FLAIR_DATA_POWER_CANCEL_IMG) $(BUILD)/$(2)_data.img
@$(call fixture-check,harness,$(BUILD)/$(2)_data.img,$(call fixture-spec,$(FLAIR_DATA_POWER_CANCEL_IMG)))
INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(1)" --disk2 "$(BUILD)/$(2)_data.img" \
 --name "$(2)" --out "$(BUILD)" --expect FLAIR-FAT-MOUNT-OK \
 --mouse "$(FLAIR_SHUTDOWN_APP_SPEC)" --keys-after FLAIR-LIVE-READY \
 --quit-after "$(FLAIR_SD_DONE)" --timeout-ms 40000 \
 2> $(BUILD)/$(2).report || true
@grep -q 'OK=1' $(BUILD)/$(2).report && ! grep -q 'triple_fault=1' $(BUILD)/$(2).report || { cat $(BUILD)/$(2).report; exit 1; }
endef

.PHONY: test-flair-shutdown-cancel test-flair-shutdown-cancel-mutant
test-flair-shutdown-cancel: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_POWER_CANCEL_IMG)
	$(call power-cancel-boot,$(FLAIRTENANTS_IMG),flair_shutdown_cancel)
	@sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel.serial
	@mcopy -o -i $(BUILD)/flair_shutdown_cancel_data.img ::APPS/CTENANT.EXE $(BUILD)/flair_shutdown_cancel_tenant.exe
	@cmp $(BUILD)/tenant/ctenant_decline.exe $(BUILD)/flair_shutdown_cancel_tenant.exe
	@fsck.fat -n $(BUILD)/flair_shutdown_cancel_data.img > $(BUILD)/flair_shutdown_cancel.fsck
	@printf '>>> test-flair-shutdown-cancel: green (declining fixture retained byte-exact by mtools; fsck.fat clean)\n'

test-flair-shutdown-cancel-mutant: $(HARNESS_BIN) $(BUILD)/flair_tenants_mut_power_no_ask.img $(FLAIR_DATA_POWER_CANCEL_IMG)
	$(call power-cancel-boot,$(BUILD)/flair_tenants_mut_power_no_ask.img,flair_shutdown_cancel_mut_noask)
	@if sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel_mut_noask.serial > $(BUILD)/flair_shutdown_cancel_mut_noask.check 2>&1; then printf 'FAIL POWER_NO_ASK survived the cancel leg\n'; exit 1; fi
	@grep -qxF 'SDC: the application did not cancel Shut Down' $(BUILD)/flair_shutdown_cancel_mut_noask.check
	@printf '>>> test-flair-shutdown-cancel-mutant: POWER_NO_ASK RED: %s\n' "$$(cat $(BUILD)/flair_shutdown_cancel_mut_noask.check)"

# The original gate now always includes the cancel leg. Both also stay visible
# by name in the vector (make itself deduplicates the prerequisite).
test-flair-shutdown: test-flair-shutdown-cancel
test-flair-shutdown-mutant: test-flair-shutdown-cancel-mutant
# TEST_EMU_GATES is assigned later in Makefile; append in bochs_legs.mk.

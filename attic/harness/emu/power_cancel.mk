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

# initech-89sb: the cancel leg's completion marker is the PROBE'S WRITE, not
# its keyDown. A declined Shut Down leaves a live Finder, so Ctrl-N really
# runs New Folder: fat12_mkdir (os/milton/fat12.c) flushes BOTH FAT copies,
# then the '.'/'..' sector, then the parent entry, and kmain.c
# finder_report_outcome prints FINDER-NEW-FOLDER only after it returned.
# Quitting on FLAIR_SD_DONE (the keyDown, printed BEFORE the command runs)
# let QMP quit land inside that sequence: the preserved evidence volume held
# cluster 563 = EOC in both FATs, its data sector still zero, linked by no
# entry (fsck "Reclaimed 1 unused cluster"); a loaded run cut between FAT
# copy 1 and 2 ("FATs differ"). That is the harness truncating a healthy
# guest (the qemu.c 6gkm class), not an OS leak: allocate-then-link is the
# crash-safe order (a cut leaves a lost cluster, never a cross-link).
# The consenting-leg mutant shuts down, so it keeps FLAIR_SD_DONE (printed
# by the shut-down screen too). $(3) = the --quit-after marker.
FLAIR_SDC_DONE := FINDER-NEW-FOLDER name=

define power-cancel-boot
# B04: acknowledge the visible refused-quit notice, then repeat the original
# Ctrl-N probe. The locked trace and on-volume oracle remain unchanged.
@$(call fixture-preflight,$(FLAIR_DATA_POWER_CANCEL_IMG))
cp -f $(FLAIR_DATA_POWER_CANCEL_IMG) $(BUILD)/$(2)_data.img
@$(call fixture-check,harness,$(BUILD)/$(2)_data.img,$(call fixture-spec,$(FLAIR_DATA_POWER_CANCEL_IMG)))
INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(1)" --disk2 "$(BUILD)/$(2)_data.img" \
 --name "$(2)" --out "$(BUILD)" --expect FLAIR-FAT-MOUNT-OK \
 --mouse "$(FLAIR_SHUTDOWN_APP_SPEC),kret,kctrl-n" --keys-after FLAIR-LIVE-READY \
 --quit-after "$(3)" --timeout-ms 40000 \
 2> $(BUILD)/$(2).report || true
@grep -q 'OK=1' $(BUILD)/$(2).report && ! grep -q 'triple_fault=1' $(BUILD)/$(2).report || { cat $(BUILD)/$(2).report; exit 1; }
endef

.PHONY: test-flair-shutdown-cancel test-flair-shutdown-cancel-mutant
test-flair-shutdown-cancel: $(HARNESS_BIN) $(FLAIR_SHUTDOWN_GATE_IMG) $(FLAIR_DATA_POWER_CANCEL_IMG)
	$(call power-cancel-boot,$(FLAIR_SHUTDOWN_GATE_IMG),flair_shutdown_cancel,$(FLAIR_SDC_DONE))
	@sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel.serial $(BUILD)/flair_shutdown_cancel_data.img
	@mcopy -o -i $(BUILD)/flair_shutdown_cancel_data.img ::APPS/CTENANT.EXE $(BUILD)/flair_shutdown_cancel_tenant.exe
	@cmp $(BUILD)/tenant/ctenant_decline.exe $(BUILD)/flair_shutdown_cancel_tenant.exe
	@fsck.fat -n $(BUILD)/flair_shutdown_cancel_data.img > $(BUILD)/flair_shutdown_cancel.fsck
	@printf '>>> test-flair-shutdown-cancel: green (declining fixture retained byte-exact by mtools; Ctrl-N folder complete on the volume; fsck.fat clean)\n'

# initech-89sb mutant: kmain.c KMAIN_MUT_MKDIR_CUT -- New Folder reports
# success over the evidence volume's state (a claimed, flushed, unlinked
# cluster). Same gate, same marker; the on-volume check must go RED, and
# fsck.fat must independently see the orphan.
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_MKDIR_CUT -DFLAIR_TEN_TICK_BUDGET=$(FLAIR_SHUTDOWN_TICK_BUDGET),shutdown_gate_mkdir_cut))

test-flair-shutdown-cancel-mutant: $(HARNESS_BIN) $(BUILD)/flair_tenants_mut_shutdown_gate_no_ask.img $(BUILD)/flair_tenants_mut_shutdown_gate_mkdir_cut.img $(FLAIR_DATA_POWER_CANCEL_IMG)
	$(call power-cancel-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_no_ask.img,flair_shutdown_cancel_mut_noask,$(FLAIR_SD_DONE))
	@if sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel_mut_noask.serial > $(BUILD)/flair_shutdown_cancel_mut_noask.check 2>&1; then printf 'FAIL POWER_NO_ASK survived the cancel leg\n'; exit 1; fi
	@grep -qxF 'SDC: the application did not cancel Shut Down' $(BUILD)/flair_shutdown_cancel_mut_noask.check
	@printf '>>> test-flair-shutdown-cancel-mutant: POWER_NO_ASK RED: %s\n' "$$(cat $(BUILD)/flair_shutdown_cancel_mut_noask.check)"
	$(call power-cancel-boot,$(BUILD)/flair_tenants_mut_shutdown_gate_mkdir_cut.img,flair_shutdown_cancel_mut_mkdircut,$(FLAIR_SDC_DONE))
	@if sh harness/emu/shutdown_cancel_check.sh $(BUILD)/flair_shutdown_cancel_mut_mkdircut.serial $(BUILD)/flair_shutdown_cancel_mut_mkdircut_data.img > $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check 2>&1; then printf 'FAIL MKDIR_CUT survived the cancel leg\n'; exit 1; fi
	@grep -qxF 'SDC: folder NEWFOLD (parent 3) reported created but absent from the volume' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check
	@if fsck.fat -n $(BUILD)/flair_shutdown_cancel_mut_mkdircut_data.img > $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck 2>&1; then printf 'FAIL MKDIR_CUT orphan passed fsck.fat\n'; exit 1; fi
	@grep -qF 'Reclaimed 1 unused cluster' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck
	@printf '>>> test-flair-shutdown-cancel-mutant: MKDIR_CUT RED: %s; fsck.fat: %s\n' "$$(cat $(BUILD)/flair_shutdown_cancel_mut_mkdircut.check)" "$$(grep -F 'Reclaimed' $(BUILD)/flair_shutdown_cancel_mut_mkdircut.fsck)"

# The original gate now always includes the cancel leg. Both also stay visible
# by name in the vector (make itself deduplicates the prerequisite).
test-flair-shutdown: test-flair-shutdown-cancel
test-flair-shutdown-mutant: test-flair-shutdown-cancel-mutant
# TEST_EMU_GATES is assigned later in Makefile; append in bochs_legs.mk.

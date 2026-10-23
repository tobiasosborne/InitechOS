# R4.4 desk accessories; existing appl-boot/tenant-exe-rules/fixture contracts.
# Every QEMU invocation uses the documented stdio seam (no Unix QMP socket).
export INITECH_QMP_STDIO := 1
DA_PIXEL := $(BUILD)/ppm_flair_notepad_check
DA_DISK := $(BUILD)/da_store_check
$(DA_PIXEL): tools/ppm_flair_notepad_check.c spec/scrollbar_look_golden.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec -o $@ $< -lpng
$(DA_DISK): tools/da_store_check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
DA_CAP = $(SYSTEM7_DECOMP)/goldens/captures
DA_PIXEL_CHECK = $(DA_PIXEL) $(1) $(BUILD)/$(2).ppm $(DA_CAP)/s8_doc_window_active.png $(DA_CAP)/s8_simpletext_empty.png $(DA_CAP)/s8_doc_window_inactive.png
DA_UP = $(call APPL_ALIVE,$(1)) && grep -qx 'APPLE-LAUNCH name=$(2).EXE dir=APPS' $(1) && grep -qx 'TENANT-REGISTER ok app=$(2)' $(1) && ! grep -q '^TENANT-CRASH' $(1)
DA_NOTE_DISK = mcopy -o -i $(BUILD)/$(1)_data.img ::NOTEPAD.DAT $(BUILD)/$(1).dat && $(DA_DISK) pad $(BUILD)/$(1).dat 1 alpha alpha2
DA_SB_DISK = mcopy -o -i $(BUILD)/$(1)_data.img ::SCRAPBK.DAT $(BUILD)/$(1).dat && $(DA_DISK) scrapbook $(BUILD)/$(1).dat 0 alpha alpha
.PHONY: test-flair-notepad test-flair-scrapbook test-flair-notepad-itext test-flair-notepad-restart
# Page-turn writes page0, close writes edited page1, relaunch reads both.
test-flair-notepad: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(FLAIR_DATA_IMG) $(DA_PIXEL) $(DA_DISK)
	$(call appl-boot,flair_notepad,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_NOTEPAD_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call DA_UP,$(BUILD)/flair_notepad.serial,NOTEPAD)
	@grep -qx 'TENANT-SCRAP put n=5 count=1' $(BUILD)/flair_notepad.serial && grep -qx 'TENANT-SCRAP get n=5' $(BUILD)/flair_notepad.serial
	@$(call DA_NOTE_DISK,flair_notepad)
	@grep -q 's="2 of 8"' $(BUILD)/flair_notepad.serial && grep -q 's="alpha2"' $(BUILD)/flair_notepad.serial
	@$(call DA_PIXEL_CHECK,pad,flair_notepad)
	@fsck.fat -n $(BUILD)/flair_notepad_data.img
	@printf '>>> test-flair-notepad: green (Apple launch; dog-ear; TEXT Scrap; turn/close/reopen; independent FAT + PNG)\n'
test-flair-scrapbook: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(FLAIR_DATA_IMG) $(DA_PIXEL) $(DA_DISK) test-scrollbar-look
	$(call appl-boot,flair_scrapbook,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_SCRAPBOOK_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call DA_UP,$(BUILD)/flair_scrapbook.serial,SCRAPBK)
	@$(call DA_SB_DISK,flair_scrapbook)
	@grep -q 's="1 of 2"' $(BUILD)/flair_scrapbook.serial && grep -q '^DA-LOADED SCRAPBK.DAT' $(BUILD)/flair_scrapbook.serial
	@$(call DA_PIXEL_CHECK,scrapbook,flair_scrapbook)
	@fsck.fat -n $(BUILD)/flair_scrapbook_data.img
	@printf '>>> test-flair-scrapbook: green (TEXT-only append; page bar; Copy; close/reopen; independent FAT + PNG)\n'
test-flair-notepad-itext: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(FLAIR_DATA_IMG) $(DA_DISK)
	$(call appl-boot,flair_note_itext,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_NOTEPAD_ITEXT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call DA_UP,$(BUILD)/flair_note_itext.serial,NOTEPAD) && $(call ITEXT_UP,$(BUILD)/flair_note_itext.serial)
	@[ "$$(grep -c '^TENANT-SCRAP put n=5 ' $(BUILD)/flair_note_itext.serial)" = 2 ] && grep -qx 'TENANT-SCRAP get n=5' $(BUILD)/flair_note_itext.serial
	@grep -q 's="alpha" fg=65536 bg=1' $(BUILD)/flair_note_itext.serial
	@grep -qx 'TENANT-ALERT-HIT item=3' $(BUILD)/flair_note_itext.serial && [ "$$(grep -c '^TENANT-REGISTER ok app=NOTEPAD' $(BUILD)/flair_note_itext.serial)" = 2 ]
	@mcopy -o -i $(BUILD)/flair_note_itext_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_itext.dat
	@$(DA_DISK) pad $(BUILD)/flair_note_itext.dat 1 alpha alphar
	@printf '>>> test-flair-notepad-itext: green (Note Pad -> InitechText -> Note Pad; two Copy puts; independent FAT alpha/alphar)\n'

# Mutants are gate-local volume swaps, with the unchanged locked traces.
define da-mut-volume
FLAIR_DATA_DA_$(1)_IMG := $(BUILD)/flair_data_da_$(1).img
$$(FLAIR_DATA_DA_$(1)_IMG): $(FLAIR_DATA_IMG) $(BUILD)/tenant/$(2).exe
	cp -f $(FLAIR_DATA_IMG) $$@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $$@ $(BUILD)/tenant/$(2).exe ::APPS/$(3).EXE
	@$$(call fixture-check,recipe,$$@)
endef
$(eval $(call da-mut-volume,TURN,notepad_turn,NOTEPAD))
$(eval $(call da-mut-volume,CLOSE,notepad_close,NOTEPAD))
$(eval $(call da-mut-volume,PASTE,notepad_paste,NOTEPAD))
$(eval $(call da-mut-volume,SBPASTE,scrapbook_paste,SCRAPBK))
.PHONY: test-flair-notepad-mutant test-flair-scrapbook-mutant
define da-note-mut-leg
	$(call appl-boot,flair_note_mut_$(1),$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_DA_$(1)_IMG),FLAIR_NOTEPAD_SPEC,)
	@$(call DA_UP,$(BUILD)/flair_note_mut_$(1).serial,NOTEPAD)
	@if $(call DA_NOTE_DISK,flair_note_mut_$(1)) > $(BUILD)/flair_note_mut_$(1).red 2>&1; then printf 'FAIL $(2) survived\n'; exit 1; fi
	@grep -q '^RED-REASON disk:' $(BUILD)/flair_note_mut_$(1).red
	@printf '>>> test-flair-notepad-mutant: $(2) correctly RED: '; cat $(BUILD)/flair_note_mut_$(1).red
endef
test-flair-notepad-mutant: test-flair-notepad $(FLAIR_DATA_DA_TURN_IMG) $(FLAIR_DATA_DA_CLOSE_IMG) $(FLAIR_DATA_DA_PASTE_IMG)
	$(call da-note-mut-leg,TURN,DA_MUT_TURN_OFFBYONE)
	$(call da-note-mut-leg,CLOSE,DA_MUT_SKIP_CLOSE_SAVE)
	$(call da-note-mut-leg,PASTE,DA_MUT_PASTE_WRONG_PAGE)
test-flair-scrapbook-mutant: test-flair-scrapbook $(FLAIR_DATA_DA_SBPASTE_IMG)
	$(call appl-boot,flair_sb_mut,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_DA_SBPASTE_IMG),FLAIR_SCRAPBOOK_SPEC,)
	@$(call DA_UP,$(BUILD)/flair_sb_mut.serial,SCRAPBK)
	@if $(call DA_SB_DISK,flair_sb_mut) > $(BUILD)/flair_sb_mut.red 2>&1; then printf 'FAIL DA_MUT_PASTE_WRONG_PAGE survived\n'; exit 1; fi
	@grep -q '^RED-REASON disk:' $(BUILD)/flair_sb_mut.red
	@printf '>>> test-flair-scrapbook-mutant: DA_MUT_PASTE_WRONG_PAGE correctly RED: '; cat $(BUILD)/flair_sb_mut.red
TEST_EMU_GATES += test-flair-notepad test-flair-notepad-mutant test-flair-scrapbook test-flair-scrapbook-mutant test-flair-notepad-itext test-flair-notepad-restart

test-flair-notepad-restart: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(FLAIR_DATA_IMG) $(DA_PIXEL) $(DA_DISK)
	$(call pw-reboot,$(FLAIR_ITEXT_GATE_IMG),flair_note_restart,$(BUILD)/flair_note_restart_data.img,$(FLAIR_NOTEPAD_RESTART_SPEC))
	@grep -qx 'FLAIR-RESTART' $(BUILD)/flair_note_restart.serial && [ "$$(grep -c '^FLAIR-LIVE-READY' $(BUILD)/flair_note_restart.serial)" = 2 ]
	@mcopy -o -i $(BUILD)/flair_note_restart_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_restart.dat
	@$(DA_DISK) pad $(BUILD)/flair_note_restart.dat 1 alpha beta
	$(call appl-boot,flair_note_reopen,$(FLAIR_ITEXT_GATE_IMG),$(BUILD)/flair_note_restart_data.img,FLAIR_NOTEPAD_OPEN_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call DA_UP,$(BUILD)/flair_note_reopen.serial,NOTEPAD) && grep -q '^DA-LOADED NOTEPAD.DAT' $(BUILD)/flair_note_reopen.serial
	@grep -q 's="beta"' $(BUILD)/flair_note_reopen.serial && grep -q 's="2 of 8"' $(BUILD)/flair_note_reopen.serial
	@$(call DA_PIXEL_CHECK,pad,flair_note_reopen)
	@fsck.fat -n $(BUILD)/flair_note_reopen_data.img
	@printf '>>> test-flair-notepad-restart: green (Special Restart -> second LIVE-READY; independent disk alpha/beta; fresh-boot reopen beta page2)\n'

DA_CORRUPT_DATA := $(BUILD)/da_corrupt_source.img
$(DA_CORRUPT_DATA): $(FLAIR_DATA_IMG)
	cp -f $(FLAIR_DATA_IMG) $@
	@printf 'bad' > $(BUILD)/da_corrupt.dat
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@ $(BUILD)/da_corrupt.dat ::NOTEPAD.DAT
	@$(call fixture-check,recipe,$@)
.PHONY: test-flair-notepad-corrupt
test-flair-notepad-corrupt: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(DA_CORRUPT_DATA) $(DA_PIXEL)
	$(call appl-boot,flair_note_corrupt,$(FLAIR_ITEXT_GATE_IMG),$(DA_CORRUPT_DATA),FLAIR_NOTEPAD_OPEN_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call DA_UP,$(BUILD)/flair_note_corrupt.serial,NOTEPAD) && grep -q '^DA-CORRUPT NOTEPAD.DAT empty pages' $(BUILD)/flair_note_corrupt.serial
	@grep -q 's="1 of 8"' $(BUILD)/flair_note_corrupt.serial
	@$(call DA_PIXEL_CHECK,pad,flair_note_corrupt)
	@printf '>>> test-flair-notepad-corrupt: green (malformed FAT file -> empty pages + DA-CORRUPT serial line)\n'
TEST_EMU_GATES += test-flair-notepad-corrupt

# initech-mcep (audit 2026-10-23-das L008): a store whose ONLY defect is its
# checksum is KEPT -- Note Pad opens empty pages, never writes over it, and
# closing says so. Fixture: an authored NOTEPAD.DAT (spec/notepad/store.h
# layout: NPAD, version 1, 8 pages, page 0 "KEEP THIS NOTE") with a zero
# checksum; da_store_check proves the checksum is its one and only defect.
# Traces: spec/flair_fullsave_traces.mk (included here, before fullsave.mk).
include spec/flair_fullsave_traces.mk
$(eval $(call tenant-exe-rules,notepad_overwrite,$(DA_APP_SRC),$(ITEXT_APP_CF) -DDA_MUT_OVERWRITE_DAMAGED,$(DA_APP_DEP),--gc-sections))
DA_BADSUM_DAT := $(BUILD)/da_badsum.dat
$(DA_BADSUM_DAT): $(DA_DISK) harness/emu/notepad.mk | $(BUILD)
	@{ printf 'NPAD\001\000\010\000\000\000\010\000\000\000\000\000\016\000KEEP THIS NOTE'; head -c 1010 /dev/zero; head -c 7182 /dev/zero; } > $@
	@test "$$(wc -c < $@)" = 8224 || { printf '!!! da_badsum.dat: not 8224 bytes\n'; rm -f $@; exit 1; }
	@if $(DA_DISK) pad $@ 0 'KEEP THIS NOTE' > $@.why; then printf '!!! da_badsum.dat: the checksum is not bad\n'; rm -f $@; exit 1; fi
	@grep -qx 'RED-REASON disk: pad count/current/pages/checksum differ (1)' $@.why || { cat $@.why; printf '!!! da_badsum.dat: more than the checksum is wrong\n'; rm -f $@; exit 1; }
DA_BADSUM_DATA := $(BUILD)/da_badsum_source.img
DA_BADSUM_MUT_DATA := $(BUILD)/da_badsum_mut_source.img
$(DA_BADSUM_DATA) $(DA_BADSUM_MUT_DATA): $(FLAIR_DATA_IMG) $(DA_BADSUM_DAT) $(BUILD)/tenant/notepad_overwrite.exe
	cp -f $(FLAIR_DATA_IMG) $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@ $(DA_BADSUM_DAT) ::NOTEPAD.DAT
	@$(if $(filter $(DA_BADSUM_MUT_DATA),$@),SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/tenant/notepad_overwrite.exe ::APPS/NOTEPAD.EXE,:)
	@$(call fixture-check,recipe,$@)
DA_KEEP_UP = $(call DA_UP,$(1),NOTEPAD) && grep -q '^DA-CORRUPT NOTEPAD.DAT empty pages' $(1) && \
	grep -qx 'TENANT-EXIT rc=0 via=exit' $(1)
DA_KEEP_KEPT = grep -q '^DA-KEPT NOTEPAD.DAT damaged, not replaced' $(1) && \
	grep -qx 'TENANT-ALERT-LINE The Note Pad file is damaged.' $(1) && \
	mcopy -o -i $(2) ::NOTEPAD.DAT $(2).dat && cmp -s $(DA_BADSUM_DAT) $(2).dat
.PHONY: test-flair-notepad-keep test-flair-notepad-keep-mutant
test-flair-notepad-keep: $(HARNESS_BIN) $(FLAIR_ITEXT_GATE_IMG) $(DA_BADSUM_DATA)
	$(call appl-boot,flair_note_keep,$(FLAIR_ITEXT_GATE_IMG),$(DA_BADSUM_DATA),FLAIR_NOTEPAD_KEEP_SPEC,)
	@$(call DA_KEEP_UP,$(BUILD)/flair_note_keep.serial) || { printf '!!! test-flair-notepad-keep FAIL: UP -- Note Pad up, the store refused (DA-CORRUPT), closed (EXIT 0)\n'; grep -E '^(DA-|TENANT-(REG|EXIT|ALERT|CRASH)|PANIC)' $(BUILD)/flair_note_keep.serial; exit 1; }
	@$(call DA_KEEP_KEPT,$(BUILD)/flair_note_keep.serial,$(BUILD)/flair_note_keep_data.img) || { printf '!!! test-flair-notepad-keep FAIL: KEPT -- the damaged store was written over / no warning\n'; grep -E '^(DA-|TENANT-ALERT)' $(BUILD)/flair_note_keep.serial; exit 1; }
	@fsck.fat -n $(BUILD)/flair_note_keep_data.img > /dev/null
	@printf '>>> test-flair-notepad-keep: green (bad-checksum store: empty pages, Close warns "damaged", NOTEPAD.DAT byte-identical (mtools); fsck clean)\n'
test-flair-notepad-keep-mutant: test-flair-notepad-keep $(DA_BADSUM_MUT_DATA)
	$(call appl-boot,flair_note_keep_mut,$(FLAIR_ITEXT_GATE_IMG),$(DA_BADSUM_MUT_DATA),FLAIR_NOTEPAD_KEEP_SPEC,)
	@$(call DA_KEEP_UP,$(BUILD)/flair_note_keep_mut.serial) || { printf '!!! test-flair-notepad-keep-mutant FAIL: OVERWRITE_DAMAGED went RED for the wrong reason (want: Note Pad up, DA-CORRUPT, EXIT 0)\n'; exit 1; }
	@if $(call DA_KEEP_KEPT,$(BUILD)/flair_note_keep_mut.serial,$(BUILD)/flair_note_keep_mut_data.img); then printf '!!! test-flair-notepad-keep-mutant FAIL: OVERWRITE_DAMAGED kept the store -- the leg is decoration\n'; exit 1; fi
	@mcopy -o -i $(BUILD)/flair_note_keep_mut_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_keep_mut.dat && ! cmp -s $(DA_BADSUM_DAT) $(BUILD)/flair_note_keep_mut.dat || { printf '!!! test-flair-notepad-keep-mutant FAIL: wrong RED reason (the store survived)\n'; exit 1; }
	@printf '>>> test-flair-notepad-keep-mutant: DA_MUT_OVERWRITE_DAMAGED correctly RED (the bad-checksum store was written over)\n'
TEST_EMU_GATES += test-flair-notepad-keep test-flair-notepad-keep-mutant

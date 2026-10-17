# initech-zc68/2kgh: authored RED before flair-data-rules was changed.
include spec/dos_folder_traces.mk
DOS_FOLDER_DIR := harness/diff/fat_diff
.PHONY: test-dos-folder-host test-dos-folder test-dos-folder-host-mutant test-dos-folder-mutant
test-dos-folder-host: $(FLAIR_DATA_IMG) $(FAT_DUMP_BIN)
	@sh $(DOS_FOLDER_DIR)/dos_folder.sh host $(FLAIR_DATA_IMG) $(BUILD)/dos-folder-host

test-dos-folder: $(FLAIR_DATA_IMG) $(FLAIRTENANTS_IMG) $(HARNESS_BIN) $(PPM_FLAIR_DISKWIN_CHECK_BIN)
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),dos_folder,$(FLAIR_DATA_IMG),$(DOS_FOLDER_EXIT_SPEC),DOS-FOLDER-DONE,0,60000)
	@sh $(DOS_FOLDER_DIR)/dos_folder.sh emu $(BUILD)/dos_folder_data.img $(BUILD)/dos-folder-emu $(BUILD)/dos_folder.serial
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),dos_folder_finder,$(FLAIR_DATA_IMG),$(DOS_FOLDER_FINDER_SPEC),FINDER-OPEN-FOLDER name=DOS win=1,1,40000)
	@sh $(DOS_FOLDER_DIR)/dos_folder.sh finder $(BUILD)/dos_folder_finder_data.img $(BUILD)/dos-folder-finder $(BUILD)/dos_folder_finder.serial
	@$(PPM_FLAIR_DISKWIN_CHECK_BIN) dosfolder $(BUILD)/dos_folder_finder.ppm

# Runtime fixture copies, deliberately outside FLAIR_DATA_*_IMG inventory:
# the production fixture remains immutable, and the same oracle judges them.
test-dos-folder-host-mutant: test-dos-folder-host
	@set -eu; for m in EXTERNAL_IN_APPS EXTRA_DOS_FILE PATH_UNSET NO_AUTOEXEC; do \
	 img=$(BUILD)/dos-folder-$$m.img; log=$(BUILD)/dos-folder-$$m.log; \
	 cp -f $(FLAIR_DATA_IMG) $$img; \
	 case $$m in EXTERNAL_IN_APPS) mmove -i $$img ::DOS/MORE.COM ::APPS/MORE.COM; rule=external-set;; \
	 EXTRA_DOS_FILE) mcopy -i $$img $(FLAIR_DATA_README) ::DOS/JUNK.TXT; rule=external-set;; \
	 PATH_UNSET) printf '@ECHO OFF\r\n' | mcopy -o -i $$img - ::AUTOEXEC.BAT; rule=autoexec;; \
	 NO_AUTOEXEC) mdel -i $$img ::AUTOEXEC.BAT; rule=autoexec;; esac; \
	 if sh $(DOS_FOLDER_DIR)/dos_folder.sh host $$img $(BUILD)/dos-folder-mut-host > $$log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep -x "RED dos-folder $$rule" $$log; echo "GREEN test-dos-folder-host-mutant $$m rejected"; done

# initech-tdnl.106 (DEC-19): DOS_NOT_DIRECTORY loses \DOS\FLAIR.COM with the
# folder, so its volume starts the desktop as APPS\FLAIR (the root still holds
# exactly README.TXT, APPS, DOS, AUTOEXEC.BAT): the one defect is DOS itself.
test-dos-folder-mutant: test-dos-folder-host-mutant $(FLAIRTENANTS_IMG) $(HARNESS_BIN) $(PPM_FLAIR_DISKWIN_CHECK_BIN) $(FLAIR_PROG_BIN)
	@set -eu; for m in EXTERNAL_IN_APPS EXTRA_DOS_FILE PATH_UNSET NO_AUTOEXEC DOS_NOT_DIRECTORY LABEL_TAIL; do \
	 img=$(BUILD)/dos-folder-$$m.img; name=dos_folder_mut_$$m; log=$(BUILD)/dos-folder-emu-$$m.log; \
	 trace='$(DOS_FOLDER_EXIT_SPEC)'; done='DOS-FOLDER-DONE'; mode=emu; rule=serial-external-set; dump=; \
	 case $$m in PATH_UNSET) rule='PATH: Bad command or file name';; \
	 NO_AUTOEXEC) rule=startup-prompts;; \
	 DOS_NOT_DIRECTORY) cp -f $(FLAIR_DATA_IMG) $$img; mdeltree -i $$img ::DOS; printf '' | mcopy -i $$img - ::DOS; mcopy -i $$img $(FLAIR_PROG_BIN) ::APPS/FLAIR.COM; printf '@ECHO OFF\r\nPATH=A:\\DOS\r\nAPPS\\FLAIR\r\n' | mcopy -o -i $$img - ::AUTOEXEC.BAT; trace='$(DOS_FOLDER_FINDER_SPEC)'; done='FINDER-OPEN-FOLDER name=DOS'; mode=finder; rule=finder-folder;; \
	 LABEL_TAIL) cp -f $(FLAIR_DATA_IMG) $$img; mren -i $$img ::AUTOEXEC.BAT ::AUTOEXEC.BAI; trace='$(FLAIR_FOLLOW_ZOOM_SPEC)'; done=FLAIR-LIVE-OK; mode=label; dump='--screendump --screendump-after FLAIR-LIVE-OK';; esac; \
	 $(HARNESS_BIN) --disk $(FLAIRTENANTS_IMG) --disk2 $$img --out $(BUILD) --name $$name \
	 --mouse "$$trace" --keys-after FLAIR-LIVE-READY --quit-after "$$done" $$dump --timeout-ms 15000 > $$log.report 2>&1 || true; \
	 if [ $$mode = label ]; then \
	  grep -qx 'FINDER-OPEN-VOLUME win=0 n=4' $(BUILD)/$$name.serial; \
	  if $(PPM_FLAIR_DISKWIN_CHECK_BIN) zoomedwin $(BUILD)/$$name.ppm > $$log 2>&1; then echo 'FAIL LABEL_TAIL survived'; exit 1; fi; \
	  grep 'grid cell 4 .*unexpected pixels' $$log; echo 'GREEN test-dos-folder-mutant LABEL_TAIL rejected'; continue; fi; \
	 if sh $(DOS_FOLDER_DIR)/dos_folder.sh $$mode $$img $(BUILD)/dos-folder-mut-emu $(BUILD)/$$name.serial > $$log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep -x "RED dos-folder $$rule" $$log || { cat $$log; exit 1; }; \
	 echo "GREEN test-dos-folder-mutant $$m rejected"; done
TEST_UNIT_GATES += test-dos-folder-host test-dos-folder-host-mutant
TEST_EMU_GATES += test-dos-folder test-dos-folder-mutant

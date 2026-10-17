# Finder limits, alerts and input. Ref: initech-tdnl.61/.63/.84/.62/.66/
# .72/.57/.64/.65/.88/.79; PRD 6.3-6.5. Shared manager link vectors.
$(BUILD)/test_finder_limits: harness/proptest/test_finder_limits.c $(FINDER_WIN_LINK) $(FINDER_WIN_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(FINDER_WIN_INC) -o $@ $< $(FINDER_WIN_LINK)
test-finder-limits: $(BUILD)/test_finder_limits
	@$<
TEST_UNIT_GATES += test-finder-limits
$(BUILD)/test_finder_notices: harness/proptest/test_finder_notices.c $(FINDER_WIN_LINK) $(FINDER_WIN_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(FINDER_WIN_INC) -o $@ $< $(FINDER_WIN_LINK)
test-finder-notices: $(BUILD)/test_finder_notices
	@$<
TEST_UNIT_GATES += test-finder-notices
$(BUILD)/test_finder_restore: harness/proptest/test_finder_restore.c $(FINDER_OPS_LINK) $(FINDER_OPS_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(WINDOW_R1_CFLAGS) $(FINDER_WIN_INC) -o $@ $< $(FINDER_OPS_LINK)
test-finder-restore: $(BUILD)/test_finder_restore
	@$<
TEST_UNIT_GATES += test-finder-restore
$(BUILD)/test_finder_diskfull: harness/diff/fat_diff/test_finder_diskfull.c $(KERNEL_DESKTOP_DB_C) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -o $@ $< $(KERNEL_DESKTOP_DB_C) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
$(BUILD)/finder_diskfull_host.img: $(BUILD)/test_finder_diskfull
	@dd if=/dev/zero of=$(BUILD)/finder_diskfull_host.img bs=512 count=2880 status=none
	@mkfs.fat -F 12 $(BUILD)/finder_diskfull_host.img >/dev/null
	@$< $(BUILD)/finder_diskfull_host.img
test-finder-diskfull: $(BUILD)/finder_diskfull_host.img
	@$(BUILD)/test_finder_diskfull $<
TEST_UNIT_GATES += test-finder-diskfull
$(BUILD)/test_finder_demo: harness/proptest/test_finder_demo.c os/apps/ref_demo_menu.h $(MENU_LINK) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(MENU_INC) -Ios/apps -o $@ $< $(MENU_LINK) -lpng
test-finder-demo: $(BUILD)/test_finder_demo
	@$<
TEST_UNIT_GATES += test-finder-demo
test-menu-handlers: test-finder-demo
$(BUILD)/test_finder_replace: harness/proptest/test_finder_replace.c $(FINDER_OPS_LINK) $(FINDER_OPS_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(WINDOW_R1_CFLAGS) $(FINDER_WIN_INC) -o $@ $< $(FINDER_OPS_LINK)
test-finder-replace: $(BUILD)/test_finder_replace
	@$<
TEST_UNIT_GATES += test-finder-replace
$(BUILD)/test_finder_input: harness/proptest/test_finder_input.c $(TEST_PROCESS_SRC) $(PROCESS_LINK) os/flair/process.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(PROCESS_INC) -o $@ $< $(PROCESS_LINK)
$(BUILD)/test_dialog_tracking: harness/proptest/test_dialog_tracking.c $(TEST_DIALOG_DEPS) $(DIALOG_LINK) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(DIALOG_INC) -o $@ $< $(DIALOG_LINK) -lpng
test-finder-input: $(BUILD)/test_finder_input
	@$<
test-dialog-tracking: $(BUILD)/test_dialog_tracking
	@$<
$(BUILD)/test_finder_input_mutant: harness/proptest/test_finder_input.c $(TEST_PROCESS_SRC) $(PROCESS_LINK) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DPROC_MUT_CLICK_THROUGH $(PROCESS_INC) -o $@ $< $(PROCESS_LINK)
$(BUILD)/test_dialog_tracking_mutant: harness/proptest/test_dialog_tracking.c $(TEST_DIALOG_DEPS) $(DIALOG_LINK) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DDIALOG_MUT_ON_PRESS $(DIALOG_INC) -o $@ $< $(DIALOG_LINK) -lpng
test-finder-input-mutant: $(BUILD)/test_finder_input_mutant
	@if $< > $(BUILD)/finder_input_mutant.log 2>&1; then printf '!!! CLICK_THROUGH survived\n'; exit 1; fi
	@grep 'FAIL.*ACTIVATE_ONLY consumes' $(BUILD)/finder_input_mutant.log
	@printf '>>> test-finder-input-mutant: PROC_MUT_CLICK_THROUGH RED\n'
test-dialog-tracking-mutant: $(BUILD)/test_dialog_tracking_mutant
	@if $< > $(BUILD)/dialog_tracking_mutant.log 2>&1; then printf '!!! ON_PRESS survived\n'; exit 1; fi
	@grep 'FAIL.*RELEASE_INSIDE press' $(BUILD)/dialog_tracking_mutant.log
	@printf '>>> test-dialog-tracking-mutant: DIALOG_MUT_ON_PRESS RED\n'
TEST_UNIT_GATES += test-finder-input test-finder-input-mutant test-dialog-tracking test-dialog-tracking-mutant
# tdnl.88 host leg (test-control PRESSED-CAPTURE vs the mint-010 pair): the
# retired accent press must go RED there too (Rule 6).
$(BUILD)/test_control_mut_press_accent: $(TEST_CONTROL_SRC) $(TEST_CONTROL_DEPS) harness/render/alert_capture.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DCTRL_MUT_PRESS_ACCENT $(CONTROL_INC) -o $@ $(TEST_CONTROL_SRC) $(CONTROL_LINK) -lpng
.PHONY: test-control-press-mutant
test-control-press-mutant: test-control $(BUILD)/test_control_mut_press_accent
	@if $(BUILD)/test_control_mut_press_accent > $(BUILD)/test_control_mut_press_accent.log 2>&1; then printf '!!! test-control-press-mutant: CTRL_MUT_PRESS_ACCENT survived\n'; exit 1; fi
	@grep -m1 'FAIL.*PRESSED-CAPTURE' $(BUILD)/test_control_mut_press_accent.log
	@printf '>>> test-control-press-mutant: CTRL_MUT_PRESS_ACCENT RED\n'
TEST_UNIT_GATES += test-control-press-mutant
.PHONY: test-finder-input test-finder-input-mutant test-dialog-tracking test-dialog-tracking-mutant

test-flair-finder-input: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_APPL_CHECK_BIN)
	$(call appl-boot,finder_activate,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_ACTIVATE_ONLY_SPEC,--screendump --screendump-after "FLAIR-LIVE-OK")
	@$(call APPL_ALIVE,$(BUILD)/finder_activate.serial) || { printf '!!! finder_activate did not finish alive\n'; exit 1; }
	@! grep -q '^TENANT-EXIT' $(BUILD)/finder_activate.serial || { printf '!!! ACTIVATE_ONLY content click quit the inactive tenant\n'; exit 1; }
	@$(PPM_FLAIR_APPL_CHECK_BIN) bar-tenant $(BUILD)/finder_activate.ppm
	@printf '>>> test-flair-finder-input: green\n'
test-flair-dialog-tracking: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,dialog_tracking,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_DIALOG_TRACK_SPEC,--screendump --screendump-after "FINDER-TRASH-ALERT")
	@$(call APPL_ALIVE,$(BUILD)/dialog_tracking.serial) || { printf '!!! dialog_tracking did not finish alive\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/dialog_tracking.ppm
	@! grep -q '^FINDER-TRASH-EMPTIED' $(BUILD)/dialog_tracking.serial
	@printf '>>> test-flair-dialog-tracking: green\n'
TEST_EMU_GATES += test-flair-finder-input test-flair-dialog-tracking

FINDER_LIMITS_DATA := $(BUILD)/finder_limits_fixture.img
$(FINDER_LIMITS_DATA): $(FLAIR_DATA_IMG)
	cp -f $< $@
	@i=0; while [ $$i -lt 69 ]; do name=$$(printf 'F%02d' $$i); mmd -i $@ ::/$$name || exit 1; i=$$((i+1)); done
	@$(call fixture-check,recipe,$@)
test-flair-finder-limits: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FINDER_LIMITS_DATA) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,finder_limits,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_LIMITS_DATA),FLAIR_FINDER_LIMITS_SPEC,--screendump --screendump-after "FINDER-ALERT reason=windows")
	@$(call APPL_ALIVE,$(BUILD)/finder_limits.serial) || { printf '!!! finder_limits did not finish alive\n'; exit 1; }
	@grep -qx 'FINDER-OPEN-VOLUME win=0 n=73' $(BUILD)/finder_limits.serial
	@grep -q '^FINDER-OPEN-FOLDER name=APPS ' $(BUILD)/finder_limits.serial
	@grep -q '^FINDER-OPEN-FOLDER name=F68 ' $(BUILD)/finder_limits.serial
	@grep -q '^FINDER-ALERT reason=windows' $(BUILD)/finder_limits.serial
	@awk '/^FINDER-OPEN-COUNT n=/{split($$2,a,"=");if(a[2]>4)ok=1}END{exit !ok}' $(BUILD)/finder_limits.serial
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_limits.ppm
	@printf '>>> test-flair-finder-limits: green\n'
TEST_EMU_GATES += test-flair-finder-limits
$(BUILD)/ppm_finder_demo_check: tools/ppm_finder_demo_check.c
	$(CC) $(CFLAGS) -o $@ $< -lpng
test-flair-finder-demo: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_IMG) $(BUILD)/ppm_finder_demo_check
	$(call appl-boot,finder_demo,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_DEMO_SPEC,--screendump --screendump-after "FLAIR-MENU-DROP menu=256")
	@$(call APPL_ALIVE,$(BUILD)/finder_demo.serial) || { printf '!!! finder_demo did not finish alive\n'; exit 1; }
	@$(BUILD)/ppm_finder_demo_check $(BUILD)/finder_demo.ppm ../system7-decomp/goldens/captures/s8_menu_dropped.png
	@grep -q '^FLAIR-MENU menu=256 item=0 ' $(BUILD)/finder_demo.serial
	@printf '>>> test-flair-finder-demo: green\n'
TEST_EMU_GATES += test-flair-finder-demo

FINDER_REPLACE_DATA := $(BUILD)/finder_replace_fixture.img
FINDER_RESTORE_DATA := $(BUILD)/finder_restore_fixture.img
$(FINDER_REPLACE_DATA): $(FLAIR_DATA_IMG)
	cp -f $< $@
	mcopy -o -i $< ::README.TXT $(BUILD)/finder_original.txt
	mcopy -o -i $@ $(BUILD)/finder_original.txt ::APPS/README.TXT
	@$(call fixture-check,recipe,$@)
$(FINDER_RESTORE_DATA): $(FLAIR_DATA_IMG)
	cp -f $< $@
	mmd -i $@ ::TRASH
	mcopy -o -i $< ::README.TXT $(BUILD)/finder_original.txt
	mcopy -o -i $@ $(BUILD)/finder_original.txt ::TRASH/README.TXT
	@$(call fixture-check,recipe,$@)

# The dump follows the LAST notice (the cycle refusal, which stays up until the
# image's life ends). The first (samedir) notice is dismissed by the tdnl.72
# kesc, and the post-injection dump keyed to its marker raced the guest: the
# harness finished injecting before the guest drew the cycle notice and
# grabbed the frame in between (no alert on screen; ALERT-BANDS 369/6496).
test-flair-finder-refused: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,finder_refused,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_FINDER_REFUSED_SPEC,--screendump --screendump-after "FINDER-ALERT-LINE A folder cannot be moved inside itself.")
	@grep -q '^FINDER-ALERT-LINE The item is already in this folder.' $(BUILD)/finder_refused.serial
	@$(call APPL_ALIVE,$(BUILD)/finder_refused.serial) || { printf '!!! finder_refused did not finish alive\n'; exit 1; }
	@grep -q '^FINDER-ALERT reason=move' $(BUILD)/finder_refused.serial
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_refused.ppm
	@printf '>>> test-flair-finder-refused: green\n'
test-flair-finder-replace: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FINDER_REPLACE_DATA) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,finder_replace,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_SPEC,--screendump --screendump-after "FINDER-ALERT reason=replace")
	@$(call APPL_ALIVE,$(BUILD)/finder_replace.serial) || { printf '!!! finder_replace did not finish alive\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/finder_replace.ppm
	$(call appl-boot,finder_replace_ok,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_OK_SPEC,)
	@$(call APPL_ALIVE,$(BUILD)/finder_replace_ok.serial) || { printf '!!! finder_replace_ok did not finish alive\n'; exit 1; }
	@mcopy -o -i $(BUILD)/finder_replace_ok_data.img ::APPS/README.TXT $(BUILD)/finder_replaced.txt
	@cmp $(BUILD)/finder_original.txt $(BUILD)/finder_replaced.txt
	@! mdir -i $(BUILD)/finder_replace_ok_data.img ::README.TXT >/dev/null 2>&1
	@printf '>>> test-flair-finder-replace: green (Replace alert; mtools incoming bytes match)\n'
test-flair-finder-restore: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FINDER_RESTORE_DATA) $(PPM_FLAIR_DISKWIN_CHECK_BIN)
	$(call appl-boot,finder_restore,$(FLAIRTENANTS_HOLD_IMG),$(FINDER_RESTORE_DATA),FLAIR_FINDER_RESTORE_SPEC,--screendump --screendump-after "FINDER-PUT-AWAY n=1")
	@$(call APPL_ALIVE,$(BUILD)/finder_restore.serial) || { printf '!!! finder_restore did not finish alive\n'; exit 1; }
	@grep -qx 'FINDER-PUT-AWAY n=1' $(BUILD)/finder_restore.serial
	@$(PPM_FLAIR_DISKWIN_CHECK_BIN) trashwin $(BUILD)/finder_restore.ppm
	@mcopy -o -i $(BUILD)/finder_restore_data.img ::README.TXT $(BUILD)/finder_restored.txt
	@cmp $(BUILD)/finder_original.txt $(BUILD)/finder_restored.txt
	@! mdir -i $(BUILD)/finder_restore_data.img ::TRASH/READM001.TXT >/dev/null 2>&1
	@printf '>>> test-flair-finder-restore: green (original name/place; mtools byte-identical)\n'
test-flair-finder-diskfull: test-finder-diskfull $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,finder_diskfull,$(FLAIRTENANTS_HOLD_IMG),$(BUILD)/finder_diskfull_host.img,FLAIR_FINDER_DISKFULL_SPEC,--screendump --screendump-after "FINDER-ALERT reason=disk-full")
	@$(call APPL_ALIVE,$(BUILD)/finder_diskfull.serial) || { printf '!!! finder_diskfull did not finish alive\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_diskfull.ppm
	@! mdir -a -i $(BUILD)/finder_diskfull_data.img ::DESKTOP.DB >/dev/null 2>&1
	@printf '>>> test-flair-finder-diskfull: green (alert, no phantom DB)\n'
test-flair-finder-launch-alerts: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_TFX_NOREG_IMG) $(FLAIR_DATA_TFX_CRASH_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,finder_noreg,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_TFX_NOREG_IMG),FLAIR_APP_LAUNCH_SPEC,--screendump --screendump-after "FINDER-ALERT reason=launch")
	@$(call APPL_ALIVE,$(BUILD)/finder_noreg.serial) || { printf '!!! finder_noreg did not finish alive\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_noreg.ppm
	$(call appl-boot,finder_crash,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_TFX_CRASH_IMG),FLAIR_APP_LAUNCH_SPEC,--screendump --screendump-after "FINDER-ALERT reason=crash")
	@$(call APPL_ALIVE,$(BUILD)/finder_crash.serial) || { printf '!!! finder_crash did not finish alive\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/finder_crash.ppm
	@printf '>>> test-flair-finder-launch-alerts: green (unregistered and crashed alerts)\n'
TEST_EMU_GATES += test-flair-finder-refused test-flair-finder-replace test-flair-finder-restore test-flair-finder-diskfull test-flair-finder-launch-alerts

# One implementation perturbation per rule. The oracle is unchanged.
define finder-host-mutant
$(BUILD)/finder_host_$(2): harness/proptest/test_finder_$(3).c $(FINDER_WIN_LINK) $(FINDER_WIN_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) $(WINDOW_R1_CFLAGS) -D$(1) $(FINDER_WIN_INC) -o $$@ $$< $(FINDER_WIN_LINK)
test-finder-$(2)-mutant: $(BUILD)/finder_host_$(2)
	@if $$< > $(BUILD)/finder_host_$(2).log 2>&1; then printf '!!! $(1) survived\n'; exit 1; fi
	@grep 'FAIL.*$(4)' $(BUILD)/finder_host_$(2).log
	@printf '>>> test-finder-$(2)-mutant: $(1) RED\n'
TEST_UNIT_GATES += test-finder-$(2)-mutant
endef
$(eval $(call finder-host-mutant,FINDER_MUT_ICON64,entries,limits,ICONS_HEAP))
$(eval $(call finder-host-mutant,FINDER_MUT_WINDOW4,window-limit,limits,WINDOWS_HEAP))
$(eval $(call finder-host-mutant,FINDER_MUT_OPEN_ONE,open-all,limits,OPEN_ALL))
$(eval $(call finder-host-mutant,FINDER_MUT_NO_KEYS,keys,limits,KEY_SELECT))
$(eval $(call finder-host-mutant,FINDER_MUT_NO_DISK_ALERT,disk-alert,notices,The disk is full))
$(eval $(call finder-host-mutant,FINDER_MUT_NO_LAUNCH_ALERT,launch-alert,notices,unexpectedly quit))
$(eval $(call finder-host-mutant,FINDER_MUT_NO_MOVE_ALERT,move-alert,notices,inside itself))
$(eval $(call finder-host-mutant,FINDER_MUT_ORIGINAL_LOST,restore,restore,ORIGINAL_NAME encoded origin pair))
$(eval $(call finder-host-mutant,FINDER_MUT_REPLACE_CANCEL,replace,replace,REPLACE confirmed))

$(BUILD)/test_finder_demo_mutant: harness/proptest/test_finder_demo.c os/apps/ref_demo_menu.h $(MENU_LINK)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFINDER_MUT_DEMO_LIVE $(MENU_INC) -Ios/apps -o $@ $< $(MENU_LINK) -lpng
test-finder-demo-mutant: $(BUILD)/test_finder_demo_mutant
	@if $< > $(BUILD)/finder_demo_mutant.log 2>&1; then exit 1; fi
	@grep -m1 'FAIL.*DEMO_GRAY' $(BUILD)/finder_demo_mutant.log
	@printf '>>> test-finder-demo-mutant: FINDER_MUT_DEMO_LIVE RED\n'
$(BUILD)/test_finder_diskfull_mutant: harness/diff/fat_diff/test_finder_diskfull.c $(KERNEL_DESKTOP_DB_C) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DDB_MUT_PHANTOM -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -o $@ $< $(KERNEL_DESKTOP_DB_C) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
test-finder-diskfull-mutant: $(BUILD)/test_finder_diskfull_mutant
	@dd if=/dev/zero of=$(BUILD)/finder_diskfull_mut.img bs=512 count=2880 status=none
	@mkfs.fat -F 12 $(BUILD)/finder_diskfull_mut.img >/dev/null
	@if $< $(BUILD)/finder_diskfull_mut.img > $(BUILD)/finder_diskfull_mutant.log 2>&1; then exit 1; fi
	@grep 'FAIL.*NO_PHANTOM' $(BUILD)/finder_diskfull_mutant.log
	@printf '>>> test-finder-diskfull-mutant: DB_MUT_PHANTOM RED\n'
TEST_UNIT_GATES += test-finder-demo-mutant test-finder-diskfull-mutant

$(eval $(call flair-tenants-proc-mutant-rules,PROC_MUT_CLICK_THROUGH,finder_click))
$(eval $(call alert-chrome-mutant-rules,DIALOG_MUT_ON_PRESS,finder_press,os/flair/dialog.c,$(KERNEL_DIALOG_OBJ),$(FLAIR_DIALOG_OPT)))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_ICON64,finder_entries))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_WINDOW4,finder_windows))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_OPEN_ONE,finder_open_one))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_NO_KEYS,finder_keys))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_NO_DISK_ALERT,finder_disk_alert))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_NO_LAUNCH_ALERT,finder_launch_alert))
$(eval $(call flair-tenants-finderwin-mutant-rules,FINDER_MUT_NO_MOVE_ALERT,finder_move_alert))
$(eval $(call flair-tenants-finderops-mutant-rules,FINDER_MUT_ORIGINAL_LOST,finder_origin))
$(eval $(call flair-tenants-finderops-mutant-rules,FINDER_MUT_REPLACE_CANCEL,finder_replace))
$(eval $(call flair-tenants-kmain-mutant-rules,FINDER_MUT_DEMO_LIVE,finder_demo))

test-flair-finder-input-mutant: test-flair-finder-input $(BUILD)/flair_tenants_mut_finder_click.img
	$(call appl-boot,finder_click_mut,$(BUILD)/flair_tenants_mut_finder_click.img,$(FLAIR_DATA_IMG),FLAIR_ACTIVATE_ONLY_SPEC,--screendump)
	@$(call APPL_ALIVE,$(BUILD)/finder_click_mut.serial) || { printf '!!! finder_click_mut did not finish alive\n'; exit 1; }
	@grep -q '^TENANT-EXIT' $(BUILD)/finder_click_mut.serial
	@printf '>>> test-flair-finder-input-mutant: PROC_MUT_CLICK_THROUGH RED (activating click exits tenant)\n'
test-flair-dialog-tracking-mutant: test-flair-dialog-tracking $(BUILD)/alert_chrome_finder_press.img
	$(call appl-boot,finder_press_mut,$(BUILD)/alert_chrome_finder_press.img,$(FLAIR_DATA_IMG),FLAIR_DIALOG_TRACK_SPEC,--screendump)
	@$(call APPL_ALIVE,$(BUILD)/finder_press_mut.serial) || { printf '!!! finder_press_mut did not finish alive\n'; exit 1; }
	@if $(PPM_FLAIR_TRASH_CHECK_BIN) chrome $(BUILD)/finder_press_mut.ppm > $(BUILD)/finder_press_mut.pixels 2>&1; then exit 1; fi
	@grep 'FAIL ALERT-BANDS' $(BUILD)/finder_press_mut.pixels
	@printf '>>> test-flair-dialog-tracking-mutant: DIALOG_MUT_ON_PRESS RED (release-outside lost alert)\n'

# Same injected limits trace, same independent pixel checker, named failures.
define finder-limits-emu-mutant
test-flair-finder-$(1)-mutant: test-flair-finder-limits $(BUILD)/flair_tenants_mut_$(2).img
	$$(call appl-boot,mut_$(2),$(BUILD)/flair_tenants_mut_$(2).img,$(FINDER_LIMITS_DATA),FLAIR_FINDER_LIMITS_SPEC,--screendump)
	@$$(call APPL_ALIVE,$(BUILD)/mut_$(2).serial) || { printf '!!! mut_$(2) did not finish alive\n'; exit 1; }
	@if $(3); then printf '!!! $(4) survived\n'; exit 1; fi
	@printf '>>> test-flair-finder-$(1)-mutant: $(4) RED ($(5))\n'
TEST_EMU_GATES += test-flair-finder-$(1)-mutant
endef
$(eval $(call finder-limits-emu-mutant,entries,finder_entries,grep -qx 'FINDER-OPEN-VOLUME win=0 n=73' $(BUILD)/mut_finder_entries.serial,FINDER_MUT_ICON64,73-entry listing absent))
$(eval $(call finder-limits-emu-mutant,windows,finder_windows,awk '/^FINDER-OPEN-COUNT n=/{split($$$$2,a,"=");if(a[2]>4)ok=1}END{exit !ok}' $(BUILD)/mut_finder_windows.serial,FINDER_MUT_WINDOW4,Open-all stops at four-window ceiling))
$(eval $(call finder-limits-emu-mutant,open-all,finder_open_one,grep -q '^FINDER-ALERT reason=windows' $(BUILD)/mut_finder_open_one.serial,FINDER_MUT_OPEN_ONE,only one folder opened))
$(eval $(call finder-limits-emu-mutant,keys,finder_keys,grep -q '^FINDER-OPEN-FOLDER name=F68 ' $(BUILD)/mut_finder_keys.serial,FINDER_MUT_NO_KEYS,F68 not opened))

define finder-notice-emu-mutant
test-flair-finder-$(1)-mutant: test-flair-finder-$(2) $(BUILD)/flair_tenants_mut_$(3).img
	$$(call appl-boot,mut_$(3),$(BUILD)/flair_tenants_mut_$(3).img,$(4),$(5),--screendump $(7))
	@$$(call APPL_ALIVE,$(BUILD)/mut_$(3).serial) || { printf '!!! mut_$(3) did not finish alive\n'; exit 1; }
	@if $(PPM_FLAIR_TRASH_CHECK_BIN) notice $(BUILD)/mut_$(3).ppm > $(BUILD)/mut_$(3).pixels 2>&1; then printf '!!! $(6) survived\n';exit 1;fi
	@grep 'FAIL ALERT-BANDS' $(BUILD)/mut_$(3).pixels
	@printf '>>> test-flair-finder-$(1)-mutant: $(6) RED (notice absent from framebuffer)\n'
TEST_EMU_GATES += test-flair-finder-$(1)-mutant
endef
$(eval $(call finder-notice-emu-mutant,disk-alert,diskfull,finder_disk_alert,$(BUILD)/finder_diskfull_host.img,FLAIR_FINDER_DISKFULL_SPEC,FINDER_MUT_NO_DISK_ALERT,))
$(eval $(call finder-notice-emu-mutant,launch-alert,launch-alerts,finder_launch_alert,$(FLAIR_DATA_TFX_NOREG_IMG),FLAIR_APP_LAUNCH_SPEC,FINDER_MUT_NO_LAUNCH_ALERT,))
# $(7): the mutant's dump waits for the guest to reach the cycle refusal (the
# point the clean leg grades), so RED means "no notice there", not a race.
$(eval $(call finder-notice-emu-mutant,move-alert,refused,finder_move_alert,$(FLAIR_DATA_IMG),FLAIR_FINDER_REFUSED_SPEC,FINDER_MUT_NO_MOVE_ALERT,--screendump-after "FINDER-MOVE-REFUSED reason=cycle"))
test-flair-finder-restore-mutant: test-flair-finder-restore $(BUILD)/flair_tenants_mut_finder_origin.img
	$(call appl-boot,mut_finder_origin,$(BUILD)/flair_tenants_mut_finder_origin.img,$(FINDER_RESTORE_DATA),FLAIR_FINDER_RESTORE_SPEC,--screendump)
	@$(call APPL_ALIVE,$(BUILD)/mut_finder_origin.serial) || { printf '!!! mut_finder_origin did not finish alive\n'; exit 1; }
	@grep -q '^FINDER-PUT-AWAY n=0' $(BUILD)/mut_finder_origin.serial
	@grep -q '^TRASH-RENAME from=README.TXT to=READM001.TXT' $(BUILD)/mut_finder_origin.serial
	@! mdir -i $(BUILD)/mut_finder_origin_data.img ::README.TXT >/dev/null 2>&1
	@printf '>>> test-flair-finder-restore-mutant: FINDER_MUT_ORIGINAL_LOST RED (original name not restored)\n'
test-flair-finder-replace-mutant: test-flair-finder-replace $(BUILD)/flair_tenants_mut_finder_replace.img
	$(call appl-boot,mut_finder_replace,$(BUILD)/flair_tenants_mut_finder_replace.img,$(FINDER_REPLACE_DATA),FLAIR_FINDER_REPLACE_OK_SPEC,--screendump)
	@$(call APPL_ALIVE,$(BUILD)/mut_finder_replace.serial) || { printf '!!! mut_finder_replace did not finish alive\n'; exit 1; }
	@grep -q '^FINDER-ALERT reason=replace' $(BUILD)/mut_finder_replace.serial
	@mdir -i $(BUILD)/mut_finder_replace_data.img ::README.TXT >/dev/null 2>&1
	@printf '>>> test-flair-finder-replace-mutant: FINDER_MUT_REPLACE_CANCEL RED (confirmed source never moved)\n'
test-flair-finder-demo-mutant: test-flair-finder-demo $(BUILD)/flair_tenants_mut_finder_demo.img
	$(call appl-boot,mut_finder_demo,$(BUILD)/flair_tenants_mut_finder_demo.img,$(FLAIR_DATA_IMG),FLAIR_FINDER_DEMO_SPEC,--screendump --screendump-after "FLAIR-MENU-DROP menu=256")
	@$(call APPL_ALIVE,$(BUILD)/mut_finder_demo.serial) || { printf '!!! mut_finder_demo did not finish alive\n'; exit 1; }
	@if $(BUILD)/ppm_finder_demo_check $(BUILD)/mut_finder_demo.ppm ../system7-decomp/goldens/captures/s8_menu_dropped.png > $(BUILD)/mut_finder_demo.pixels 2>&1; then exit 1;fi
	@grep 'FAIL DEMO_GRAY' $(BUILD)/mut_finder_demo.pixels
	@printf '>>> test-flair-finder-demo-mutant: FINDER_MUT_DEMO_LIVE RED (native disabled ink missing)\n'
TEST_EMU_GATES += test-flair-finder-input-mutant test-flair-dialog-tracking-mutant test-flair-finder-restore-mutant test-flair-finder-replace-mutant test-flair-finder-demo-mutant

FINDER_ALERTS_HOST_CLEAN := test-finder-input test-dialog-tracking test-finder-limits test-finder-notices test-finder-restore test-finder-replace test-finder-demo test-finder-diskfull
FINDER_ALERTS_HOST_MUT := test-finder-input-mutant test-dialog-tracking-mutant test-control-press-mutant $(addprefix test-finder-,$(addsuffix -mutant,entries window-limit open-all keys disk-alert launch-alert move-alert restore replace demo diskfull))
FINDER_ALERTS_EMU_CLEAN := test-flair-finder-input test-flair-dialog-tracking $(addprefix test-flair-finder-,limits demo refused replace restore diskfull launch-alerts)
FINDER_ALERTS_EMU_MUT := test-flair-finder-input-mutant test-flair-dialog-tracking-mutant $(addprefix test-flair-finder-,$(addsuffix -mutant,entries windows open-all keys disk-alert launch-alert move-alert restore replace demo))
.PHONY: test-finder-alerts test-flair-finder-alerts $(FINDER_ALERTS_HOST_CLEAN) $(FINDER_ALERTS_HOST_MUT) $(FINDER_ALERTS_EMU_CLEAN) $(FINDER_ALERTS_EMU_MUT)
test-finder-alerts: $(FINDER_ALERTS_HOST_CLEAN) $(FINDER_ALERTS_HOST_MUT)
	@printf '>>> test-finder-alerts: green (8 host suites; 14 named mutants RED)\n'
test-flair-finder-alerts: $(FINDER_ALERTS_EMU_CLEAN) $(FINDER_ALERTS_EMU_MUT) test-flair-dialog-pressed-golden test-flair-dialog-pressed-golden-mutant
	@printf '>>> test-flair-finder-alerts: green (10 QEMU families incl. the native pressed OK; 13 named mutants RED)\n'

# tdnl.88 / Law 1+2: the held default OK graded against the Mac OS 8.1
# PRESSED pair minted for this bead (system7-decomp re/mint-results-010.md;
# sys8/controls.md Sec 5.3). Both captures are required; a missing one is a
# hard RED, never a skipped leg.
FINDER_PRESSED_CAPTURE := ../system7-decomp/goldens/captures/s8_alert_ok_pressed.png
FINDER_PRESSED_CAPTURE_TL := ../system7-decomp/goldens/captures/s8_alert_ok_pressed_tl.png
.PHONY: test-flair-dialog-pressed-golden finder-dialog-pressed-probe test-flair-dialog-pressed-golden-mutant
test-flair-dialog-pressed-golden:
	@for c in $(FINDER_PRESSED_CAPTURE) $(FINDER_PRESSED_CAPTURE_TL); do test -f $$c || { printf '!!! test-flair-dialog-pressed-golden RED: missing Mac OS 8.1 native held-OK capture: %s (re/mint-results-010.md)\n' "$$c"; exit 1; }; done
	@$(MAKE) --no-print-directory finder-dialog-pressed-probe
finder-dialog-pressed-probe: $(HARNESS_BIN) $(FLAIRTENANTS_HOLD_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	$(call appl-boot,dialog_held,$(FLAIRTENANTS_HOLD_IMG),$(FLAIR_DATA_IMG),FLAIR_DIALOG_HELD_SPEC,--screendump --screendump-after "FINDER-TRASH-ALERT")
	@$(call APPL_ALIVE,$(BUILD)/dialog_held.serial) || { printf '!!! dialog_held did not finish alive\n'; exit 1; }
	@! grep -q '^FINDER-TRASH-EMPTIED' $(BUILD)/dialog_held.serial
	@$(PPM_FLAIR_TRASH_CHECK_BIN) pressed $(BUILD)/dialog_held.ppm $(FINDER_PRESSED_CAPTURE) $(FINDER_PRESSED_CAPTURE_TL)
	@printf '>>> test-flair-dialog-pressed-golden: green (held OK == native mint-010 pressed button)\n'
# Rule 6: the retired pre-mint accent-fill press (CTRL_MUT_PRESS_ACCENT) on
# the same hold image must fail the native pressed grade.
$(BUILD)/control_mut_press_accent.o: os/flair/control.c os/flair/control.h $(FLAIRLOOK_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_CONTROL_OPT) -DCTRL_MUT_PRESS_ACCENT -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c $< -o $@
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_CONTROL_OBJ),$(BUILD)/kmain_flairtenants_mut_hold.o $(BUILD)/control_mut_press_accent.o,press_accent))
test-flair-dialog-pressed-golden-mutant: test-flair-dialog-pressed-golden $(BUILD)/flair_tenants_mut_press_accent.img
	$(call appl-boot,mut_press_accent,$(BUILD)/flair_tenants_mut_press_accent.img,$(FLAIR_DATA_IMG),FLAIR_DIALOG_HELD_SPEC,--screendump --screendump-after "FINDER-TRASH-ALERT")
	@$(call APPL_ALIVE,$(BUILD)/mut_press_accent.serial) || { printf '!!! mut_press_accent did not finish alive\n'; exit 1; }
	@if $(PPM_FLAIR_TRASH_CHECK_BIN) pressed $(BUILD)/mut_press_accent.ppm $(FINDER_PRESSED_CAPTURE) $(FINDER_PRESSED_CAPTURE_TL) > $(BUILD)/mut_press_accent.pixels 2>&1; then printf '!!! test-flair-dialog-pressed-golden-mutant: CTRL_MUT_PRESS_ACCENT survived\n'; exit 1; fi
	@grep 'FAIL PRESSED-CAPTURE' $(BUILD)/mut_press_accent.pixels
	@printf '>>> test-flair-dialog-pressed-golden-mutant: CTRL_MUT_PRESS_ACCENT RED (accent press is not the native pressed button)\n'
TEST_EMU_GATES += test-flair-dialog-pressed-golden test-flair-dialog-pressed-golden-mutant

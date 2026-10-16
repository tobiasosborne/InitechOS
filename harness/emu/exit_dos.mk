# exit_dos.mk -- Special > Exit to InitechDOS and the TPS Report Generator
# journey on the FLAGSHIP image (bead initech-3p1a). Included from the Makefile
# after harness/emu/fcb_tps.mk (it reuses that file's TPS goldens and key
# script) and before harness/fixture_volumes.mk.
#
# THE STORY (operator 2026-06-14 / 2026-10-16): the TPS Report Generator runs
# ONLY from the DOS prompt, so the user must LEAVE the desktop -- Special >
# Exit to InitechDOS, "Are you sure you want to exit to InitechDOS?", Exit --
# land at A:\>, CD APPS, TPSREP TPS.DBF, and go through the whole operating
# procedure (spec/tps_ritual.json). The report must really be written.
#
# GATES (QEMU; the Bochs leg is owed -- Bochs 2.7 ENOMODEs 640x480 and the
# desktop guard halts before the pump, see test-flair-appswitch-bochs):
#   test-flair-exit-to-dos  [1] ALERT: Special item 10 selects EXIT_DOS and
#     the confirmation comes up -- pixel-graded by ppm_flair_trash_check leg
#     exitdos (the s8_alert_modal.png capture's frame bands / Cancel bevel /
#     corners, two message lines, Exit ringed, Cancel not); nothing exits.
#     [2] CANCEL (Escape): FINDER-EXIT-DOS-CANCEL, nothing put away, and the
#     desktop still answers (a Ctrl-N New Folder).
#     [3] EXIT: put away like Shut Down (FINDER-POWER name=EXITDOS, the
#     DESKTOP.DB commit, then FLAIR-EXIT-TO-DOS), the transient COMMAND.COM
#     validated (COMMAND-TRANSIENT-OK), the banner on serial == the locked
#     spec/dos_banner.txt, SHELL-READY, and a DIR typed at the prompt lists
#     the FLAGSHIP volume as drive A:. The screen is the TEXT CONSOLE again:
#     ppm_text_check (the test-boot grader) twice -- the banner inked at its
#     origin and the DIR band [48,240) inked, then the console paper below the
#     last prompt row (y >= 336, full width; the four startup date/time rows -- initech-qskz -- sit above the DIR listing, 20 text rows of 16 px) AND down the right margin
#     (x >= 560 from y 32, where no line of this session reaches but the
#     desktop's volume and Trash icons sat): no desktop pixel survives.
#   test-flair-exit-to-dos-mutant (Rule 6): NO_TEARDOWN (KMAIN_MUT_EXIT_NO_
#     TEARDOWN: straight to the prompt, desktop still painted) RED on the
#     text-console grader; NO_TRANSIENT (the flagship kernel on an image whose
#     boot disk carries no transient) RED: MSG-DOS-0014 + PANIC, no
#     SHELL-READY.
#   test-flair-tps-journey: the whole journey, keyed EXACTLY as
#     test-tps-report leg A (the trace's keys are linted against
#     harness/diff/tps/keys_a.txt here); the console transcript from the
#     program banner through "TPS REPORT WRITTEN TO TPSREPT.TXT." == the
#     real-MS-DOS 6.22 golden's same span (the golden's next line, PREVIOUS
#     REPORT RETAINED, belongs to a volume that HAD a report; the flagship's
#     has none, so no .BAK may appear), the end line RETURN CODE 0, and
#     APPS\TPSREPT.TXT read back by mtools byte-identical to the real-DOS
#     tpsrept_expected.txt.
#   test-flair-tps-journey-mutant (Rule 6): SIX (six presses where the
#     procedure wants seven -- refused, FORM ALIGNED never printed, no report)
#     and NOFILES (the flagship volume without TPSREP.COM / TPSPARM.DAT /
#     TPS.DBF -- "Bad command or file name", no report) and CWDRESET (loader.o
#     -DLOADER_MUT_CWD_RESET: a launched program starts at the root again, as
#     before this bead, so TPSREP looks for TPSPARM.DAT in A:\ -- TPS-001) all
#     RED.
# Rule 14 clip: record-flair SCRIPT=tps_journey (the block sits before the
# record-flair target in the Makefile; the PM records it on main).

EXITDOS_ALERT_NAME   := flair_exitdos_alert
EXITDOS_CANCEL_NAME  := flair_exitdos_cancel
EXITDOS_EXIT_NAME    := flair_exitdos_exit
TPSJ_NAME            := flair_tps_journey
EXITDOS_DONE         := bytes free
TPSJ_DONE            := TPS REPORT GENERATOR TERMINATED. RETURN CODE 0.

# Mutant images. NO_TEARDOWN: the kmain template's kernel + its OWN transient.
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_EXIT_NO_TEARDOWN,exit_no_teardown))
$(eval $(call exitdos-transient-rules,exit_no_teardown,$(BUILD)/kernel_flairtenants_mut_exit_no_teardown.elf))
EXITDOS_MUT_NOTEARDOWN_IMG := $(BUILD)/flair_exitdos_mut_no_teardown.img
$(EXITDOS_MUT_NOTEARDOWN_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_flairtenants_mut_exit_no_teardown.bin $(BUILD)/transient_exit_no_teardown.bin | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_mut_exit_no_teardown.bin,$(BUILD)/transient_exit_no_teardown.bin)
	@printf ">>> exit-to-DOS MUTANT image (-DKMAIN_MUT_EXIT_NO_TEARDOWN): %s\n" "$@"
# NO_TRANSIENT: the REAL flagship kernel, the boot disk's transient window
# left zero (what a boot disk without COMMAND.COM looks like).
EXITDOS_MUT_NOTRANSIENT_IMG := $(BUILD)/flair_exitdos_mut_no_transient.img
$(EXITDOS_MUT_NOTRANSIENT_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_FLAIRTENANTS_BIN) | $(BUILD)
	$(call exitdos-image-recipe,$(KERNEL_FLAIRTENANTS_BIN),/dev/null)
	@printf ">>> exit-to-DOS MUTANT image (no transient on the boot disk): %s\n" "$@"

# The clip image: the DOUBLE-CLICK record kernel (the trace opens the volume)
# with a transient linked against IT.
$(eval $(call exitdos-transient-rules,recorddbl,$(BUILD)/kernel_flairtenants_recorddbl.elf))
$(FLAIRTENANTS_RECORDDBL_EXITDOS_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_flairtenants_recorddbl.bin $(BUILD)/transient_recorddbl.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_recorddbl.bin,$(BUILD)/transient_recorddbl.bin)
	@printf ">>> flair-tenants DOUBLE-CLICK RECORD image + transient COMMAND.COM: %s\n" "$@"

# CWDRESET: the flagship objects with loader.o built -DLOADER_MUT_CWD_RESET
# (the pre-3p1a reset-to-root at every launch), + its own transient.
$(BUILD)/loader_mut_cwdreset.o: $(KERNEL_LOADER_C) $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/psp.h spec/memory_map.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/blockdev.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DLOADER_MUT_CWD_RESET -Ispec -I$(KERNEL_DIR) -c $(KERNEL_LOADER_C) -o $@
$(BUILD)/kernel_flairtenants_mut_cwdreset.elf: $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/loader_mut_cwdreset.o $(KERNEL_LD) $(TRANSIENT_IMPORTS_LD) | $(BUILD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/loader_mut_cwdreset.o @$(TRANSIENT_IMPORTS_LD)
$(BUILD)/kernel_flairtenants_mut_cwdreset.bin: $(BUILD)/kernel_flairtenants_mut_cwdreset.elf | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); [ "$$sz" -le "$$max" ] || { printf '!!! %s (%s B) exceeds the KERNEL_SECTORS window\n' "$@" "$$sz"; exit 1; }; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none
	$(call kernel-end-guard,$<,flairtenants-mut-cwdreset)
$(eval $(call exitdos-transient-rules,cwdreset,$(BUILD)/kernel_flairtenants_mut_cwdreset.elf))
EXITDOS_MUT_CWDRESET_IMG := $(BUILD)/flair_exitdos_mut_cwdreset.img
$(EXITDOS_MUT_CWDRESET_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_flairtenants_mut_cwdreset.bin $(BUILD)/transient_cwdreset.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_mut_cwdreset.bin,$(BUILD)/transient_cwdreset.bin)
	@printf ">>> exit-to-DOS MUTANT image (-DLOADER_MUT_CWD_RESET): %s\n" "$@"

# The journey volume mutant: the flagship volume WITHOUT the three TPS files.
EXITDOS_NOFILES_DATA := $(BUILD)/flair_data_no_tps.img
$(EXITDOS_NOFILES_DATA): $(FLAIR_DATA_IMG) | $(BUILD)
	cp -f $(FLAIR_DATA_IMG) $@
	mdel -i $@ ::APPS/TPSREP.COM ::APPS/TPSPARM.DAT ::APPS/TPS.DBF
	@! mdir -b -i $@ ::APPS | grep -q 'TPS' || { printf '!!! %s still carries a TPS file\n' "$@"; exit 1; }

# $(call exitdos-boot,<image>,<name>,<data src>,<spec>,<done marker>,<dump 1|0>,<timeout ms>)
# A fresh copy of <data src> per boot; the harness waits for <done marker>.
define exitdos-boot
@$(call fixture-preflight,$(3))
cp -f $(3) $(BUILD)/$(2)_data.img
$(HARNESS_BIN) --disk "$(1)" --disk2 "$(BUILD)/$(2)_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(2)" --out "$(BUILD)" --mouse "$(4)" --keys-after "FLAIR-LIVE-READY" \
	--quit-after "$(5)" $(if $(filter 1,$(6)),--screendump --screendump-after "$(5)") \
	--timeout-ms $(7) 2> "$(BUILD)/$(2).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(2).report"; then printf '!!! %s FAIL: TRIPLE FAULT in boot %s\n' "$@" "$(2)"; exit 1; fi
endef

# $(call exitdos-exit-check,<name>) -- leg 3's serial + screen, shared VERBATIM
# by the gate and its mutants. Exit 0 iff green.
define exitdos-exit-check
tr -d '\r' < "$(BUILD)/$(1).serial" > "$(BUILD)/$(1).sx"; s="$(BUILD)/$(1).sx"; \
grep -qxF 'FLAIR-MENU menu=515 item=10 (sel=0x0203000A)' $$s || { printf 'EXIT: the release did not select Special > Exit to InitechDOS\n'; exit 1; }; \
grep -qxF 'FINDER-CMD id=16 name=EXIT_DOS src=mouse sel=0' $$s || { printf 'EXIT: Exit to InitechDOS did not reach the command spine\n'; exit 1; }; \
seq=$$(sed -n '/^FINDER-CMD id=16 name=EXIT_DOS/,$$p' $$s | grep -E '^(FINDER-EXIT-DOS-ALERT|FINDER-POWER name=EXITDOS|DESKTOP-DB-SAVE|FLAIR-EXIT-TO-DOS|COMMAND-TRANSIENT-OK|BANNER-BEGIN|SHELL-READY)' | sed 's/ bytes=.*//;s/ views=.*//;s/ n=.*//' | tr '\n' '|'); \
[ "$$seq" = 'FINDER-EXIT-DOS-ALERT|FINDER-POWER name=EXITDOS|DESKTOP-DB-SAVE|FLAIR-EXIT-TO-DOS|COMMAND-TRANSIENT-OK|BANNER-BEGIN|SHELL-READY|' ] || { printf 'EXIT: the hand-off did not run alert -> put away -> commit -> FLAIR-EXIT-TO-DOS -> transient -> banner -> SHELL-READY: %s\n' "$$seq"; exit 1; }; \
sed -n '/^FLAIR-EXIT-TO-DOS$$/,$$p' $$s | sed -n '/^BANNER-BEGIN$$/,/^BANNER-END$$/p' | sed '1d;$$d' | tr -d '\r' > $(BUILD)/$(1).banner; \
cmp -s $(BUILD)/$(1).banner spec/dos_banner.txt || { printf 'EXIT: the banner after the exit != spec/dos_banner.txt\n'; exit 1; }; \
after=$$(sed -n '/^SHELL-READY$$/,$$p' $$s | tr -d '\r'); \
printf '%s\n' "$$after" | grep -qF ' Volume in drive A is INITECH' || { printf 'EXIT: DIR at the prompt did not list the flagship volume as drive A:\n'; exit 1; }; \
printf '%s\n' "$$after" | grep -qE '^README +TXT ' && printf '%s\n' "$$after" | grep -qE '^APPS +<DIR>' || { printf 'EXIT: the A: listing lacks README.TXT / APPS\n'; exit 1; }; \
! grep -qE 'Enter new (date|time)' $$s || { printf 'EXIT: AUTOEXEC exists but startup prompts appeared\n'; exit 1; }; \
! grep -qE '^(PANIC|FLAIR-LIVE-OK|DESKTOP-DB-WRITE-FAIL)' $$s || { printf 'EXIT: %s\n' "$$(grep -E '^(PANIC|FLAIR-LIVE-OK|DESKTOP-DB-WRITE-FAIL)' $$s | head -1)"; exit 1; }; \
[ -s "$(BUILD)/$(1).ppm" ] || { printf 'EXIT: no screendump at the prompt\n'; exit 1; }; \
{ $(PPM_TEXT_CHECK_BIN) "$(BUILD)/$(1).ppm" 48 240 200 304 && $(PPM_TEXT_CHECK_BIN) "$(BUILD)/$(1).ppm" 48 240 200 32 560; } > $(BUILD)/$(1).ppmcheck 2>&1 || { printf 'EXIT: the screen is not the text console (the desktop is still painted): %s\n' "$$(grep -i fail $(BUILD)/$(1).ppmcheck | head -1)"; exit 1; }
endef

.PHONY: test-flair-exit-to-dos test-flair-exit-to-dos-mutant test-flair-tps-journey test-flair-tps-journey-mutant
test-flair-exit-to-dos: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN) $(PPM_TEXT_CHECK_BIN) spec/dos_banner.txt
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-flair-exit-to-dos : Special > Exit to InitechDOS (initech-3p1a)\n'
	@printf '  the alert, the cancel, and the hand-off of the console to COMMAND.COM\n'
	@printf '======================================================================\n'
	@# ---- leg 1: the ALERT, left up ----
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_ALERT_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_ALERT_SPEC),FINDER-EXIT-DOS-ALERT,1,40000)
	@s=$(BUILD)/$(EXITDOS_ALERT_NAME).serial; \
	grep -qxF 'FLAIR-MENU menu=515 item=10 (sel=0x0203000A)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: the release at (170,163) did not select Special item 10: %s\n' "$$(grep '^FLAIR-MENU menu=515' $$s | tr '\n' ' ')"; exit 1; }; \
	grep -qxF 'FINDER-CMD id=16 name=EXIT_DOS src=mouse sel=0' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: no FINDER-CMD EXIT_DOS\n'; exit 1; }; \
	grep -qxF 'FINDER-EXIT-DOS-ALERT' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: the confirmation never came up\n'; exit 1; }; \
	! grep -qE '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [1]: something happened while the alert was still up: %s\n' "$$(grep -E '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s | head -1)"; exit 1; }
	@[ -s "$(BUILD)/$(EXITDOS_ALERT_NAME).ppm" ] || { printf '!!! test-flair-exit-to-dos FAIL [1]: no screendump\n'; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) exitdos "$(BUILD)/$(EXITDOS_ALERT_NAME).ppm" || { printf '!!! test-flair-exit-to-dos FAIL [1]: the alert is not drawn as graded\n'; exit 1; }
	@printf '>>> test-flair-exit-to-dos [1/3]: ALERT -- Special item 10 -> EXIT_DOS -> the confirmation up, graded against s8_alert_modal.png; nothing put away\n'
	@# ---- leg 2: CANCEL (Escape) ----
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_CANCEL_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_CANCEL_SPEC),name=NEW_FOLDER,0,40000)
	@s=$(BUILD)/$(EXITDOS_CANCEL_NAME).serial; \
	grep -qxF 'FINDER-EXIT-DOS-CANCEL' $$s || { printf '!!! test-flair-exit-to-dos FAIL [2]: Escape did not cancel\n'; exit 1; }; \
	! grep -qE '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s || { printf '!!! test-flair-exit-to-dos FAIL [2]: the cancel still exited / put away: %s\n' "$$(grep -E '^(FINDER-POWER|FLAIR-EXIT-TO-DOS|SHELL-READY|PANIC)' $$s | head -1)"; exit 1; }; \
	sed -n '/^FINDER-EXIT-DOS-CANCEL$$/,$$p' $$s | grep -qE '^FINDER-CMD id=2 name=NEW_FOLDER ' || { printf '!!! test-flair-exit-to-dos FAIL [2]: the desktop did not answer after the cancel\n'; exit 1; }
	@printf '>>> test-flair-exit-to-dos [2/3]: CANCEL -- Escape dismissed it, nothing put away, the Finder still answers (Ctrl-N)\n'
	@# ---- leg 3: EXIT ----
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXITDOS_EXIT_NAME),$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
	@( $(call exitdos-exit-check,$(EXITDOS_EXIT_NAME)) ) || { printf '!!! test-flair-exit-to-dos FAIL [3]: (above)\n'; grep -E '^(FINDER-|FLAIR-EXIT|COMMAND-|SHELL-|DESKTOP-DB|PANIC)' $(BUILD)/$(EXITDOS_EXIT_NAME).serial; exit 1; }
	@mtype -i $(BUILD)/$(EXITDOS_EXIT_NAME)_data.img ::DESKTOP.DB > /dev/null 2>&1 || { printf '!!! test-flair-exit-to-dos FAIL [3]: DESKTOP.DB unreadable by mtools after the exit\n'; exit 1; }
	@printf '>>> test-flair-exit-to-dos [3/3]: EXIT -- put away + DESKTOP.DB commit -> FLAIR-EXIT-TO-DOS -> transient COMMAND.COM (%s) -> banner == spec -> SHELL-READY -> DIR lists A: (INITECH); the screen is the text console (ppm_text_check)\n' "$$(grep -m1 '^COMMAND-TRANSIENT-OK' $(BUILD)/$(EXITDOS_EXIT_NAME).serial)"
	@printf '>>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)\n'

test-flair-exit-to-dos-mutant: $(HARNESS_BIN) $(EXITDOS_MUT_NOTEARDOWN_IMG) $(EXITDOS_MUT_NOTRANSIENT_IMG) $(FLAIR_DATA_IMG) $(PPM_TEXT_CHECK_BIN)
	$(call exitdos-boot,$(EXITDOS_MUT_NOTEARDOWN_IMG),flair_exitdos_mut_noteardown,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
	@out=$$( ( $(call exitdos-exit-check,flair_exitdos_mut_noteardown) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TEARDOWN PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'the desktop is still painted' || { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TEARDOWN RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-exit-to-dos-mutant: NO_TEARDOWN correctly RED (%s)\n' "$$out"
	$(call exitdos-boot,$(EXITDOS_MUT_NOTRANSIENT_IMG),flair_exitdos_mut_notransient,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),HALTED,0,40000)
	@out=$$( ( $(call exitdos-exit-check,flair_exitdos_mut_notransient) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'did not run alert -> put away' || { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	grep -qxF 'Bad or missing Command Interpreter' $(BUILD)/flair_exitdos_mut_notransient.sx && grep -q '^PANIC exit-to-dos: no COMMAND3 transient' $(BUILD)/flair_exitdos_mut_notransient.serial \
		|| { printf '!!! test-flair-exit-to-dos-mutant FAIL: NO_TRANSIENT did not say MSG-DOS-0014 + PANIC\n'; exit 1; }; \
	printf '>>> test-flair-exit-to-dos-mutant: NO_TRANSIENT correctly RED (%s; MSG-DOS-0014 + %s)\n' "$$out" "$$(grep -m1 '^PANIC' $(BUILD)/flair_exitdos_mut_notransient.serial)"
	@printf '>>> test-flair-exit-to-dos-mutant: green (2/2 RED for the named reason)\n'

# The trace's TPS keys must BE test-tps-report leg A's keys (keys_a.txt).
TPSJ_KEYS_FROM_GOLDEN = k$(subst $(comma),$(comma)k,$(call tps-keys,$(TPS_DIR)/keys_a.txt))
comma := ,

# $(call tpsj-check,<name>) -- the journey's verdict, shared by the gate and
# its mutants. Exit 0 iff green.
define tpsj-check
tr -d '\r' < "$(BUILD)/$(1).serial" > "$(BUILD)/$(1).sx"; s="$(BUILD)/$(1).sx"; \
grep -qxF 'FLAIR-EXIT-TO-DOS' $$s && grep -qxF 'SHELL-READY' $$s || { printf 'TPSJ: the desktop never handed over to the prompt\n'; exit 1; }; \
grep -qiF 'cd apps' $$s && grep -qiF 'tpsrep tps.dbf' $$s || { printf 'TPSJ: CD APPS / TPSREP TPS.DBF were not typed at the prompt\n'; exit 1; }; \
[ "$$(sed -n '/tpsrep tps.dbf$$/{n;p;q;}' $$s)" != 'Bad command or file name' ] || { printf 'TPSJ: COMMAND.COM could not find the TPS Report Generator (Bad command or file name)\n'; exit 1; }; \
! grep -qxF 'TPS-001 PARAMETER FILE TPSPARM.DAT NOT FOUND.' $$s || { printf 'TPSJ: TPSREP did not find TPSPARM.DAT in the current directory (TPS-001)\n'; exit 1; }; \
grep -qxF 'FORM ALIGNED.' $$s || { printf 'TPSJ: the form-feed ritual was never satisfied (no FORM ALIGNED.)\n'; exit 1; }; \
sed -n '/^INITECH CORPORATION$$/,/^TPS REPORT WRITTEN TO TPSREPT.TXT.$$/p' $$s > $(BUILD)/$(1).got; \
sed -n '1,/^TPS REPORT WRITTEN TO TPSREPT.TXT.$$/p' $(TPS_DIR)/tps_a_expected.txt > $(BUILD)/$(1).want; \
cmp -s $(BUILD)/$(1).want $(BUILD)/$(1).got || { printf 'TPSJ: console transcript != the real-DOS golden span (diff %s %s)\n' $(BUILD)/$(1).want $(BUILD)/$(1).got; exit 1; }; \
grep -qxF '$(TPSJ_DONE)' $$s || { printf 'TPSJ: no RETURN CODE 0 end line\n'; exit 1; }; \
mtype -i $(BUILD)/$(1)_data.img ::APPS/TPSREPT.TXT > $(BUILD)/$(1).rept 2>/dev/null || { printf 'TPSJ: no APPS\\TPSREPT.TXT on the volume\n'; exit 1; }; \
cmp -s $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/$(1).rept || { printf 'TPSJ: APPS\\TPSREPT.TXT != the real-DOS report (cmp %s %s)\n' $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/$(1).rept; exit 1; }; \
! mdir -b -i $(BUILD)/$(1)_data.img ::APPS 2>/dev/null | grep -q 'TPSREPT.BAK' || { printf 'TPSJ: a TPSREPT.BAK appeared on a volume that had no previous report\n'; exit 1; }; \
! grep -qE '^PANIC' $$s || { printf 'TPSJ: %s\n' "$$(grep -m1 '^PANIC' $$s)"; exit 1; }
endef

test-flair-tps-journey: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(TPS_GOLDENS) $(TPS_DIR)/keys_a.txt
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-flair-tps-journey : desktop -> Exit to InitechDOS -> A:\\> -> TPSREP (initech-3p1a)\n'
	@printf '  the ritual keyed as test-tps-report leg A; report bytes == real MS-DOS 6.22\n'
	@printf '======================================================================\n'
	@[ '$(FLAIR_TPS_KEYS_A)' = '$(TPSJ_KEYS_FROM_GOLDEN)' ] || { printf '!!! test-flair-tps-journey FAIL: spec/flair_exit_dos_traces.mk FLAIR_TPS_KEYS_A != harness/diff/tps/keys_a.txt\n  spec  : %s\n  golden: %s\n' '$(FLAIR_TPS_KEYS_A)' '$(TPSJ_KEYS_FROM_GOLDEN)'; exit 1; }
	@printf '>>> test-flair-tps-journey [lint]: the trace keys the ritual EXACTLY as keys_a.txt (%s keystrokes)\n' "$(words $(subst $(comma), ,$(FLAIR_TPS_KEYS_A)))"
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(TPSJ_NAME),$(FLAIR_DATA_IMG),$(FLAIR_TPS_JOURNEY_SPEC),$(TPSJ_DONE),1,60000)
	@( $(call tpsj-check,$(TPSJ_NAME)) ) || { printf '!!! test-flair-tps-journey FAIL: (above)\n'; grep -E '^(FINDER-|FLAIR-EXIT|COMMAND-|SHELL-|PANIC|A:)' $(BUILD)/$(TPSJ_NAME).serial | head -20; exit 1; }
	@printf '>>> test-flair-tps-journey: Exit -> A:\\> -> CD APPS -> TPSREP TPS.DBF -> 6/8 presses refused, 7 aligned, number keyed twice, memo 88-114, cover sheet Y -> transcript (%s lines) == real DOS; APPS\\TPSREPT.TXT (%s B) == real DOS\n' "$$(wc -l < $(BUILD)/$(TPSJ_NAME).got)" "$$(wc -c < $(BUILD)/$(TPSJ_NAME).rept)"
	@printf '>>> test-flair-tps-journey: green (QEMU; Bochs leg owed)\n'

test-flair-tps-journey-mutant: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(EXITDOS_NOFILES_DATA) $(EXITDOS_MUT_CWDRESET_IMG) $(TPS_GOLDENS)
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),flair_tps_journey_mut_six,$(FLAIR_DATA_IMG),$(FLAIR_TPS_SIX_SPEC),FORM NOT ALIGNED (0 PRESSES),0,60000)
	@out=$$( ( $(call tpsj-check,flair_tps_journey_mut_six) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-tps-journey-mutant FAIL: SIX PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'never satisfied' || { printf '!!! test-flair-tps-journey-mutant FAIL: SIX RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	[ "$$(grep -cxF 'FORM NOT ALIGNED (6 PRESSES). THE COUNT HAS BEEN RESET.' $(BUILD)/flair_tps_journey_mut_six.sx)" -ge 2 ] || { printf '!!! test-flair-tps-journey-mutant FAIL: SIX -- the six presses were not REFUSED twice\n'; exit 1; }; \
	printf '>>> test-flair-tps-journey-mutant: SIX correctly RED (%s; six presses refused)\n' "$$out"
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),flair_tps_journey_mut_nofiles,$(EXITDOS_NOFILES_DATA),$(FLAIR_TPS_JOURNEY_SPEC),Bad command or file name,0,60000)
	@out=$$( ( $(call tpsj-check,flair_tps_journey_mut_nofiles) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-tps-journey-mutant FAIL: NOFILES PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'could not find the TPS Report Generator' || { printf '!!! test-flair-tps-journey-mutant FAIL: NOFILES RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-tps-journey-mutant: NOFILES correctly RED (%s)\n' "$$out"
	$(call exitdos-boot,$(EXITDOS_MUT_CWDRESET_IMG),flair_tps_journey_mut_cwdreset,$(FLAIR_DATA_IMG),$(FLAIR_TPS_JOURNEY_SPEC),TPS REPORT GENERATOR TERMINATED. RETURN CODE 1.,0,60000)
	@out=$$( ( $(call tpsj-check,flair_tps_journey_mut_cwdreset) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-tps-journey-mutant FAIL: CWDRESET PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'did not find TPSPARM.DAT in the current directory' || { printf '!!! test-flair-tps-journey-mutant FAIL: CWDRESET RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-tps-journey-mutant: CWDRESET correctly RED (%s)\n' "$$out"
	@printf '>>> test-flair-tps-journey-mutant: green (3/3 RED for the named reason)\n'

TEST_EMU_GATES += test-flair-exit-to-dos test-flair-exit-to-dos-mutant test-flair-tps-journey test-flair-tps-journey-mutant

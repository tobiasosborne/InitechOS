# flair_cmd.mk -- FLAIR AS A COMMAND (bead initech-tdnl.106; ADR-0003
# Amendment DEC-19 "The resident Toolbox entry"). Included from the Makefile
# right after harness/emu/exit_dos.mk (it reuses that file's exitdos-boot /
# exit trace, spec/flair_exit_dos_traces.mk, and the appswitch grader).
#
# THE STORY (operator 2026-10-17: "we can boot to dos, then start flair
# right?"): the system boots to the InitechDOS banner and COMMAND.COM; FLAIR
# (\DOS\FLAIR.COM, run by AUTOEXEC.BAT on the shipped volume) starts the
# desktop through INT 81h AX=TBX_DESKSTART; Special > Exit to InitechDOS puts
# it away and RETURNS to the prompt; FLAIR works again.
#
# GATES
#   test-flair-cmd-host (+ -mutant): harness/proptest/test_flair_cmd.c -- D1
#     the gate rule (os/flair/tbxdesk.h), D2 the transient checksum rule
#     (os/milton/transient.h; IBM DOS 3.30 TR p1-5), D3 FLAIR.COM carries the
#     two locked lines. Mutants TBXDESK_MUT_FLAIR_TWICE, TBXDESK_MUT_LEAVE_KEEPS,
#     TRANSIENT_MUT_NO_CHECK, TRANSIENT_MUT_SUM_DATA, each RED for its reason.
#   test-flair-from-dos (QEMU; the Bochs leg is owed -- Bochs 2.7 ENOMODEs
#     640x480 and the desktop guard halts the session):
#     [1] PROMPT: the flagship boots, with a data volume whose AUTOEXEC.BAT has
#         no FLAIR line, to A:\> -- COMMAND-TRANSIENT-OK, no desktop.
#     [2] ROUND TRIPS: type FLAIR -> desktop; Exit -> SHELL-READY; FLAIR ->
#         desktop; Exit -> SHELL-READY; FLAIR -> desktop (three desktops, two
#         exits, serial-asserted in order; the transient is never reloaded).
#         The THIRD desktop's screendump (FLAIR-TENANTS-READY, 3rd) is
#         byte-identical to a fresh boot's, and the appswitch grader
#         (test-flair-appswitch's ppm_flair_appswitch_check, the independent
#         canon) passes on (third desktop, its click on NOTES).
#     [3] TWICE: from a running desktop, a second FLAIR (FLAIR.COM's bytes
#         run as the system hotkey's text tenant, on a gate-local volume) is
#         REFUSED: FLAIR-DESKSTART rc=-4, "FLAIR is already running.", the
#         desktop resumes.
#     [4] RELOAD: a fault-injection kernel (KMAIN_FIX_TRANSIENT_OVERLAY)
#         overlays the transient's code from the desktop; at Exit the checksum
#         catches it (COMMAND-TRANSIENT-RELOAD), the code is reloaded from
#         COMMAND.COM, the prompt answers and FLAIR works again.
#   test-flair-from-dos-mutant (Rule 6):
#     EXIT_RESETS_CONSOLE_ONLY (kmain: Exit rebuilds the console but leaves the
#       Toolbox armed) -> the second FLAIR is refused (rc=-4), no 2nd desktop;
#     TRANSIENT_NOT_PRESERVED (kmain: the desktop overlays the transient and
#       the checksum is never made) -> the prompt after Exit runs on garbage
#       (PANIC vec=03 from the INT3 overlay), no 2nd desktop;
#     FLAIR_TWICE (tbxgate: os/flair/tbxdesk.h's BUSY check gone) -> leg [3]'s
#       refusal is missing.
# Rule 14 clip: record-flair SCRIPT=flair_from_dos (the block sits before the
# record-flair target in the Makefile; the PM records it on main).

# ---------------------------------------------------------------------------
# The host oracle.
TEST_FLAIR_CMD     := $(BUILD)/test_flair_cmd
TEST_FLAIR_CMD_SRC := harness/proptest/test_flair_cmd.c
TEST_FLAIR_CMD_DEPS := $(TEST_FLAIR_CMD_SRC) os/flair/tbxdesk.h os/milton/transient.h \
                       spec/toolbox_gate.h spec/memory_map.h seed/test_assert.h
$(TEST_FLAIR_CMD): $(TEST_FLAIR_CMD_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) -Iseed -Ios/flair -Ios/milton -Ispec -o $@ $(TEST_FLAIR_CMD_SRC)
$(BUILD)/test_flair_cmd_mutant_%: $(TEST_FLAIR_CMD_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) -D$* -Iseed -Ios/flair -Ios/milton -Ispec -o $@ $(TEST_FLAIR_CMD_SRC)

FLAIR_CMD_HOST_MUTANTS := TBXDESK_MUT_FLAIR_TWICE TBXDESK_MUT_LEAVE_KEEPS TRANSIENT_MUT_NO_CHECK TRANSIENT_MUT_SUM_DATA

.PHONY: test-flair-cmd-host test-flair-cmd-host-mutant
test-flair-cmd-host: $(TEST_FLAIR_CMD) $(FLAIR_PROG_BIN)
	@printf '>>> test-flair-cmd-host: the gate rule, the transient checksum rule, FLAIR.COM'"'"'s lines (initech-tdnl.106)\n'
	@$(TEST_FLAIR_CMD) $(FLAIR_PROG_BIN)
	@printf '>>> test-flair-cmd-host: green\n'

test-flair-cmd-host-mutant: $(foreach m,$(FLAIR_CMD_HOST_MUTANTS),$(BUILD)/test_flair_cmd_mutant_$(m)) $(FLAIR_PROG_BIN)
	@for pair in 'TBXDESK_MUT_FLAIR_TWICE:D1 a second FLAIR while armed is refused' \
	             'TBXDESK_MUT_LEAVE_KEEPS:D1 Exit to InitechDOS makes the Toolbox dormant' \
	             'TRANSIENT_MUT_NO_CHECK:D2 code byte changed: the transient is overlaid' \
	             'TRANSIENT_MUT_SUM_DATA:D2 data byte changed: the live state is not an overlay'; do \
		m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_flair_cmd_mutant_$$m; \
		if $$bin $(FLAIR_PROG_BIN) > $$bin.log 2>&1; then printf '!!! test-flair-cmd-host-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
		grep -F "$$why" $$bin.log | grep -q FAIL || { printf '!!! test-flair-cmd-host-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
		printf '>>> test-flair-cmd-host-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -F "$$why" $$bin.log | head -1 | sed 's/^ *//')"; \
	done
	@printf '>>> test-flair-cmd-host-mutant: green (4/4 RED for the named reason)\n'

TEST_UNIT_GATES += test-flair-cmd-host test-flair-cmd-host-mutant

# ---------------------------------------------------------------------------
# The emulator gate.
# Volumes. NOFLAIR: the shipped volume with AUTOEXEC.BAT as it was before
# FLAIR became a command (@ECHO OFF, PATH=A:\DOS) -- the machine stays at the
# prompt. TWICE: the shipped volume (AUTOEXEC.BAT runs FLAIR) whose root
# SAMIR.COM -- the name the system hotkey's text tenant runs -- is FLAIR.COM.
# Both carry the stock FLAIR_FIXTURES contract (fixture_volumes.mk derives it
# from the FLAIR_DATA_<X>_IMG names).
FLAIR_DATA_TWICE_IMG := $(BUILD)/flair_data_twice.img
FLAIR_CMD_NOFLAIR_BAT := $(BUILD)/flaircmd_noflair_autoexec.bat
$(FLAIR_CMD_NOFLAIR_BAT): harness/emu/flair_cmd.mk | $(BUILD)
	@printf '@ECHO OFF\r\nPATH=A:\\DOS\r\n' > $@
$(FLAIR_DATA_NOFLAIR_IMG): $(FLAIR_DATA_IMG) $(FLAIR_CMD_NOFLAIR_BAT) | $(BUILD)
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $(FLAIR_DATA_IMG) $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(FLAIR_CMD_NOFLAIR_BAT) ::AUTOEXEC.BAT
	@! mtype -i $@ ::AUTOEXEC.BAT | grep -q FLAIR || { printf '!!! %s still runs FLAIR\n' "$@"; rm -f $@; exit 1; }
	@$(call fixture-check,recipe,$@)
$(FLAIR_DATA_TWICE_IMG): $(FLAIR_DATA_IMG) $(FLAIR_PROG_BIN) | $(BUILD)
	@$(call fixture-preflight,$(FLAIR_DATA_IMG))
	cp -f $(FLAIR_DATA_IMG) $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@ $(FLAIR_PROG_BIN) ::SAMIR.COM
	@$(call fixture-check,recipe,$@)

# Kernel images: the RELOAD fault-injection fixture and the two kmain mutants
# (kmain template: each carries its own transient), the FLAIR_TWICE tbxgate
# mutant (tbxgate template).
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_FIX_TRANSIENT_OVERLAY,transient_overlay))
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_EXIT_RESETS_CONSOLE_ONLY,exit_console_only))
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_TRANSIENT_NOT_PRESERVED,transient_not_preserved))
$(eval $(call flair-tenants-tbx-mutant-rules,TBXDESK_MUT_FLAIR_TWICE,flair_twice))
FLAIRCMD_OVERLAY_IMG   := $(BUILD)/flair_tenants_mut_transient_overlay.img
FLAIRCMD_MUT_CONS_IMG  := $(BUILD)/flair_tenants_mut_exit_console_only.img
FLAIRCMD_MUT_TRANS_IMG := $(BUILD)/flair_tenants_mut_transient_not_preserved.img
FLAIRCMD_MUT_TWICE_IMG := $(BUILD)/flair_tenants_mut_flair_twice.img

# $(call flaircmd-boot,<image>,<name>,<data src>,<keys-after>,<spec>,<quit marker>,<extra harness args>,<timeout ms>)
# A fresh copy of <data src> per boot; the harness quits at <quit marker>.
define flaircmd-boot
@$(call fixture-preflight,$(3))
cp -f $(3) $(BUILD)/$(2)_data.img
$(HARNESS_BIN) --disk "$(1)" --disk2 "$(BUILD)/$(2)_data.img" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(2)" --out "$(BUILD)" --mouse "$(5)" --keys-after "$(4)" \
	--quit-after "$(6)" $(7) --timeout-ms $(8) 2> "$(BUILD)/$(2).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(2).report"; then printf '!!! %s FAIL: TRIPLE FAULT in boot %s\n' "$@" "$(2)"; exit 1; fi
tr -d '\r' < "$(BUILD)/$(2).serial" > "$(BUILD)/$(2).sx"
endef

# The marker sequence of the round trips (the transient is never reloaded).
FLAIRCMD_SEQ_RE := ^(COMMAND-TRANSIENT-(OK|RELOAD)|FLAIR-DESKSTART|FLAIR-LIVE-READY|FLAIR-EXIT-TO-DOS|SHELL-READY|FLAIR-LIVE-OK|PANIC)
FLAIRCMD_SEQ_OK := COMMAND-TRANSIENT-OK|FLAIR-DESKSTART rc=0|FLAIR-LIVE-READY|FLAIR-EXIT-TO-DOS|COMMAND-TRANSIENT-OK|SHELL-READY|FLAIR-DESKSTART rc=0|FLAIR-LIVE-READY|FLAIR-EXIT-TO-DOS|COMMAND-TRANSIENT-OK|SHELL-READY|FLAIR-DESKSTART rc=0|FLAIR-LIVE-READY|

# $(call flaircmd-trips-check,<name>) -- legs [1]+[2]'s serial verdict, shared
# VERBATIM by the gate and its mutants. Exit 0 iff green.
define flaircmd-trips-check
s="$(BUILD)/$(1).sx"; \
[ -s $$s ] || { printf 'TRIPS: no serial\n'; exit 1; }; \
p=$$(grep -n -m1 -F 'A:\>' $$s | cut -d: -f1); d=$$(grep -n -m1 -x 'FLAIR-DESKTOP' $$s | cut -d: -f1); \
[ -n "$$p" ] || { printf 'TRIPS: the boot never reached the A:\\> prompt\n'; exit 1; }; \
[ -z "$$d" ] || [ "$$p" -lt "$$d" ] || { printf 'TRIPS: the desktop came before the prompt (AUTOEXEC.BAT has no FLAIR line)\n'; exit 1; }; \
grep -qi -F 'A:\>flair' $$s || { printf 'TRIPS: FLAIR was never typed at the prompt\n'; exit 1; }; \
! grep -q -x 'COMMAND-TRANSIENT-RELOAD' $$s || { printf 'TRIPS: the transient was overlaid and reloaded (it must be preserved)\n'; exit 1; }; \
! grep -qE '^PANIC' $$s || { printf 'TRIPS: %s (the prompt after Exit ran on garbage)\n' "$$(grep -m1 -E '^PANIC' $$s)"; exit 1; }; \
[ "$$(grep -cx 'FLAIR-LIVE-READY' $$s)" -ge 2 ] || { printf 'TRIPS: the second FLAIR did not bring the desktop back (%s)\n' "$$(grep -E '^FLAIR-DESKSTART|already running|cannot start' $$s | tr '\n' ' ')"; exit 1; }; \
seq=$$(grep -E '$(FLAIRCMD_SEQ_RE)' $$s | grep -v '^FLAIR-LIVE-OK' | sed 's/ bytes=.*//' | tr '\n' '|'); \
[ "$$seq" = '$(FLAIRCMD_SEQ_OK)' ] || { printf 'TRIPS: the round trips did not run prompt -> FLAIR -> desktop -> Exit -> prompt x2 -> desktop: %s\n' "$$seq"; exit 1; }; \
[ "$$(grep -cx 'BANNER-BEGIN' $$s)" -eq 3 ] || { printf 'TRIPS: the console was not rebuilt with the banner at each Exit\n'; exit 1; }
endef

.PHONY: test-flair-from-dos test-flair-from-dos-mutant
test-flair-from-dos: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(FLAIR_DATA_NOFLAIR_IMG) $(FLAIR_DATA_TWICE_IMG) $(FLAIRCMD_OVERLAY_IMG) $(PPM_FLAIR_APPSWITCH_CHECK_BIN)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-flair-from-dos : FLAIR as a command (initech-tdnl.106)\n'
	@printf '  boot to A:\\> ; FLAIR -> desktop ; Exit -> A:\\> ; twice ; a second FLAIR refused ; a reload\n'
	@printf '  Ref: ADR-0003 Amendment DEC-19; spec/toolbox_gate.h Sec 17; IBM DOS 3.30 TR pp1-5/1-6\n'
	@printf '======================================================================\n'
	@# ---- the reference: a fresh flagship boot's desktop ----
	$(call flaircmd-boot,$(FLAIRTENANTS_IMG),flaircmd_fresh,$(FLAIR_DATA_IMG),FLAIR-LIVE-READY,,FLAIR-LIVE-OK,--screendump --screendump-after FLAIR-TENANTS-READY,30000)
	@[ -s $(BUILD)/flaircmd_fresh.ppm ] || { printf '!!! test-flair-from-dos FAIL: no fresh-boot desktop screendump\n'; exit 1; }
	@# ---- [1]+[2] the prompt and the round trips; the third desktop dumped ----
	$(call flaircmd-boot,$(FLAIRTENANTS_IMG),flaircmd_trips,$(FLAIR_DATA_NOFLAIR_IMG),COMMAND-TRANSIENT-OK,$(FLAIR_FROM_DOS_SPEC),FLAIR-LIVE-OK,--screendump --screendump-after FLAIR-TENANTS-READY --screendump-after-count 3,90000)
	@( $(call flaircmd-trips-check,flaircmd_trips) ) || { printf '!!! test-flair-from-dos FAIL [1/2]: (above)\n'; exit 1; }
	@printf '>>> test-flair-from-dos [1/4]: PROMPT -- the boot stops at A:\\> (no FLAIR in AUTOEXEC.BAT): COMMAND-TRANSIENT-OK, then the prompt, no desktop\n'
	@[ -s $(BUILD)/flaircmd_trips.ppm ] || { printf '!!! test-flair-from-dos FAIL [2]: no screendump of the third desktop\n'; exit 1; }
	@cmp -s $(BUILD)/flaircmd_fresh.ppm $(BUILD)/flaircmd_trips.ppm || { printf '!!! test-flair-from-dos FAIL [2]: the third desktop is not the boot desktop, pixel for pixel (cmp %s %s)\n' $(BUILD)/flaircmd_fresh.ppm $(BUILD)/flaircmd_trips.ppm; exit 1; }
	$(call flaircmd-boot,$(FLAIRTENANTS_IMG),flaircmd_switch,$(FLAIR_DATA_NOFLAIR_IMG),COMMAND-TRANSIENT-OK,$(FLAIR_FROM_DOS_SWITCH_SPEC),FLAIR-LIVE-OK,--screendump --screendump-after "FLAIR-DISPATCH app=NOTES",90000)
	@( $(call flaircmd-trips-check,flaircmd_switch) ) || { printf '!!! test-flair-from-dos FAIL [2] (switch run): (above)\n'; exit 1; }
	@[ -s $(BUILD)/flaircmd_switch.ppm ] || { printf '!!! test-flair-from-dos FAIL [2]: no screendump after the third desktop'"'"'s app switch\n'; exit 1; }
	@$(PPM_FLAIR_APPSWITCH_CHECK_BIN) $(BUILD)/flaircmd_trips.ppm $(BUILD)/flaircmd_switch.ppm > $(BUILD)/flaircmd_switch.grade 2>&1 || { cat $(BUILD)/flaircmd_switch.grade; printf '!!! test-flair-from-dos FAIL [2]: the third desktop does not grade as the App Contract desktop (ppm_flair_appswitch_check)\n'; exit 1; }
	@printf '>>> test-flair-from-dos [2/4]: ROUND TRIPS -- FLAIR -> desktop -> Exit -> A:\\> twice, FLAIR a third time; the transient preserved (no reload); desktop 3 == the boot desktop pixel for pixel and grades PASS under ppm_flair_appswitch_check\n'
	@# ---- [3] a second FLAIR while the desktop runs ----
	$(call flaircmd-boot,$(FLAIRTENANTS_IMG),flaircmd_twice,$(FLAIR_DATA_TWICE_IMG),FLAIR-LIVE-READY,$(FLAIR_TWICE_SPEC),FLAIR-LIVE-OK,,60000)
	@( $(call flaircmd-twice-check,flaircmd_twice) ) || { printf '!!! test-flair-from-dos FAIL [3]: (above)\n'; exit 1; }
	@printf '>>> test-flair-from-dos [3/4]: TWICE -- FLAIR run from the running desktop is refused (FLAIR-DESKSTART rc=-4, "FLAIR is already running.", return code 1), the desktop resumes, Exit and FLAIR still work\n'
	@# ---- [4] the checksum rule: an overlaid transient is reloaded ----
	$(call flaircmd-boot,$(FLAIRCMD_OVERLAY_IMG),flaircmd_reload,$(FLAIR_DATA_IMG),FLAIR-LIVE-READY,$(FLAIR_CMD_RELOAD_SPEC),FLAIR-LIVE-OK,,60000)
	@( $(call flaircmd-reload-check,flaircmd_reload) ) || { printf '!!! test-flair-from-dos FAIL [4]: (above)\n'; exit 1; }
	@printf '>>> test-flair-from-dos [4/4]: RELOAD -- the desktop overlaid the transient; at Exit the checksum caught it (COMMAND-TRANSIENT-RELOAD), the code was reloaded from COMMAND.COM, the batch and the prompt went on, FLAIR worked again\n'
	@printf '>>> test-flair-from-dos: green (QEMU; Bochs leg owed)\n'

# $(call flaircmd-twice-check,<name>) -- leg [3]'s verdict. Exit 0 iff green.
define flaircmd-twice-check
s="$(BUILD)/$(1).sx"; \
grep -qx 'FLAIR-SUSPEND' $$s || { printf 'TWICE: the hotkey never ran the text tenant\n'; exit 1; }; \
grep -qx 'FLAIR-DESKSTART rc=-4' $$s || { printf 'TWICE: a second FLAIR while the desktop runs was not refused (no FLAIR-DESKSTART rc=-4): %s\n' "$$(grep '^FLAIR-DESKSTART' $$s | tr '\n' ' ')"; exit 1; }; \
grep -qx 'FLAIR is already running.' $$s || { printf 'TWICE: FLAIR.COM did not say "FLAIR is already running."\n'; exit 1; }; \
grep -qx 'FLAIR-TEXT-EXIT rc=1' $$s || { printf 'TWICE: the refused FLAIR did not end with return code 1\n'; exit 1; }; \
grep -qx 'FLAIR-RESUME' $$s || { printf 'TWICE: the desktop did not resume\n'; exit 1; }; \
[ "$$(grep -cx 'FLAIR-DESKTOP' $$s)" -eq 2 ] && [ "$$(grep -cx 'SHELL-READY' $$s)" -eq 1 ] || { printf 'TWICE: after the refusal Exit and FLAIR did not still work (desktops %s, prompts %s)\n' "$$(grep -cx FLAIR-DESKTOP $$s)" "$$(grep -cx SHELL-READY $$s)"; exit 1; }; \
[ "$$(grep -c '^FLAIR-DESKSTART rc=0' $$s)" -eq 2 ] || { printf 'TWICE: the Toolbox was armed other than by the two real FLAIRs\n'; exit 1; }; \
! grep -qE '^PANIC' $$s || { printf 'TWICE: %s\n' "$$(grep -m1 -E '^PANIC' $$s)"; exit 1; }
endef

# $(call flaircmd-reload-check,<name>) -- leg [4]'s verdict. Exit 0 iff green.
define flaircmd-reload-check
s="$(BUILD)/$(1).sx"; \
seq=$$(grep -E '$(FLAIRCMD_SEQ_RE)' $$s | grep -v '^FLAIR-LIVE-OK' | sed 's/ bytes=.*//' | tr '\n' '|'); \
[ "$$seq" = 'COMMAND-TRANSIENT-OK|FLAIR-DESKSTART rc=0|FLAIR-LIVE-READY|FLAIR-EXIT-TO-DOS|COMMAND-TRANSIENT-RELOAD|COMMAND-TRANSIENT-OK|SHELL-READY|FLAIR-DESKSTART rc=0|FLAIR-LIVE-READY|' ] || { printf 'RELOAD: the overlaid transient was not reloaded before the prompt, or FLAIR did not work again: %s\n' "$$seq"; exit 1; }; \
grep -qi -F 'A:\>flair' $$s || { printf 'RELOAD: the reloaded COMMAND.COM never took FLAIR at its prompt\n'; exit 1; }
endef

test-flair-from-dos-mutant: $(HARNESS_BIN) $(FLAIR_DATA_NOFLAIR_IMG) $(FLAIR_DATA_TWICE_IMG) $(FLAIRCMD_MUT_CONS_IMG) $(FLAIRCMD_MUT_TRANS_IMG) $(FLAIRCMD_MUT_TWICE_IMG)
	$(call flaircmd-boot,$(FLAIRCMD_MUT_CONS_IMG),flaircmd_mut_console_only,$(FLAIR_DATA_NOFLAIR_IMG),COMMAND-TRANSIENT-OK,$(FLAIR_FROM_DOS_SPEC),FLAIR is already running.,,120000)
	@out=$$( ( $(call flaircmd-trips-check,flaircmd_mut_console_only) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-from-dos-mutant FAIL: EXIT_RESETS_CONSOLE_ONLY PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'the second FLAIR did not bring the desktop back' && grep -qx 'FLAIR-DESKSTART rc=-4' $(BUILD)/flaircmd_mut_console_only.sx \
		|| { printf '!!! test-flair-from-dos-mutant FAIL: EXIT_RESETS_CONSOLE_ONLY RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-from-dos-mutant: EXIT_RESETS_CONSOLE_ONLY correctly RED (%s)\n' "$$out"
	$(call flaircmd-boot,$(FLAIRCMD_MUT_TRANS_IMG),flaircmd_mut_transient,$(FLAIR_DATA_NOFLAIR_IMG),COMMAND-TRANSIENT-OK,$(FLAIR_FROM_DOS_SPEC),HALTED,,120000)
	@out=$$( ( $(call flaircmd-trips-check,flaircmd_mut_transient) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-from-dos-mutant FAIL: TRANSIENT_NOT_PRESERVED PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'the prompt after Exit ran on garbage' \
		|| { printf '!!! test-flair-from-dos-mutant FAIL: TRANSIENT_NOT_PRESERVED RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-from-dos-mutant: TRANSIENT_NOT_PRESERVED correctly RED (%s)\n' "$$out"
	$(call flaircmd-boot,$(FLAIRCMD_MUT_TWICE_IMG),flaircmd_mut_twice,$(FLAIR_DATA_TWICE_IMG),FLAIR-LIVE-READY,$(FLAIR_TWICE_SPEC),FLAIR-LIVE-OK,,90000)
	@out=$$( ( $(call flaircmd-twice-check,flaircmd_mut_twice) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-from-dos-mutant FAIL: FLAIR_TWICE PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'was not refused' \
		|| { printf '!!! test-flair-from-dos-mutant FAIL: FLAIR_TWICE RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	printf '>>> test-flair-from-dos-mutant: FLAIR_TWICE correctly RED (%s)\n' "$$out"
	@printf '>>> test-flair-from-dos-mutant: green (3/3 RED for the named reason)\n'

TEST_EMU_GATES += test-flair-from-dos test-flair-from-dos-mutant

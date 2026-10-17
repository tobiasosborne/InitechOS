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

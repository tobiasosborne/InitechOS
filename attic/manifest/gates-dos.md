## test-autoexec

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30830):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin harness/diff/fat_diff/fixtures/autoexec_xw1.bat harness/diff/fat_diff/fixtures/sub_xw1.bat build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-autoexec : AUTOEXEC.BAT + .BAT execution\n'
@printf '  beads initech-xw1. AUTOEXEC.BAT runs at REPL entry: @ECHO OFF, IF\n'
@printf '  string/ERRORLEVEL, FOR, external EXEC (GREET rc=7), CALL SUB.BAT.\n'
@printf '  Ref: MS-DOS 3.3 Tech Ref Ch.3; ADR-0003 DEC-08; WL-0038 HARD LESSON.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-autoexec FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-autoexec FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call autoexec-mint-disk,$(AUTOEXEC_IMG))
@printf 'Booting   : %s + data disk %s (AUTOEXEC.BAT + SUB.BAT + GREET.COM)\n' "$(SECONDARY_TRACER_IMG)" "$(AUTOEXEC_IMG)"
@printf 'Expecting : SHELL-READY then 10 batch markers, then clean EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(AUTOEXEC_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(AUTOEXEC_IMG)" \
	--name "$(AUTOEXEC_NAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(AUTOEXEC_REPORT)" || true
@cat "$(AUTOEXEC_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(AUTOEXEC_REPORT)"; then \
	printf '!!! test-autoexec FAIL: TRIPLE FAULT -- AUTOEXEC/.BAT execution crashed\n'; exit 1; \
fi
@printf '>>> test-autoexec [1/4]: no triple-fault\n'
@if [ ! -s "$(AUTOEXEC_SERIAL)" ]; then \
	printf '!!! test-autoexec FAIL: no serial captured at %s\n' "$(AUTOEXEC_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(AUTOEXEC_SERIAL)" \
	|| { printf '!!! test-autoexec FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-autoexec [2/4]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(AUTOEXEC_SERIAL)" | tr -d '\r' > "$(BUILD)/$(AUTOEXEC_NAME).repl"
@for m in $(AUTOEXEC_MARKERS); do \
	grep -qF "$$m" "$(BUILD)/$(AUTOEXEC_NAME).repl" \
		|| { printf '!!! test-autoexec FAIL: marker %s missing -- AUTOEXEC/.BAT did not execute that step (root-cause batch driver / EXEC / CALL-reload, Rule 3)\n' "$$m"; exit 1; }; \
done
@printf '>>> test-autoexec [3/4]: all 10 batch markers present (ECHO state, IF str/ERRORLEVEL, FOR, EXEC, CALL+reload)\n'
@grep -q '^SHELL-EXIT$$' "$(AUTOEXEC_SERIAL)" \
	|| { printf '!!! test-autoexec FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run\n'; exit 1; }
@printf '>>> test-autoexec [4/4]: post-AUTOEXEC `exit` reached the REPL + halted cleanly\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- AUTOEXEC.BAT ran end-to-end on the emulated 386\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-autoexec: green (oracle completed)\n'
```

## test-autoexec-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30910):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_autoexec.img build/greet_program.bin harness/diff/fat_diff/fixtures/autoexec_xw1.bat harness/diff/fat_diff/fixtures/sub_xw1.bat build/.makefile.stamp`
```make
@printf '>>> test-autoexec-mutant: confirming the NO-AUTOEXEC mutant goes RED (Rule 6)\n'
$(call autoexec-mint-disk,$(AUTOEXEC_MUT_IMG))
@$(call fixture-preflight,$(AUTOEXEC_MUT_IMG))
@$(HARNESS_BIN) --disk "$(AUTOEXEC_TRACER_MUT_IMG)" --disk2 "$(AUTOEXEC_MUT_IMG)" \
	--name "$(AUTOEXEC_MUT_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(AUTOEXEC_MUT_REPORT)" || true
@if [ ! -s "$(AUTOEXEC_MUT_SERIAL)" ]; then \
	printf '!!! test-autoexec-mutant FAIL: no serial from the mutant boot\n'; exit 1; \
fi
@sed -n '/^SHELL-READY$$/,$$p' "$(AUTOEXEC_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(AUTOEXEC_MUT_NAME).repl"
@if grep -qF 'AUTOEXEC-RAN' "$(BUILD)/$(AUTOEXEC_MUT_NAME).repl"; then \
	printf '!!! test-autoexec-mutant FAIL: AUTOEXEC-RAN present under CMD_MUTATE_NO_AUTOEXEC -- the gate is decoration (it would pass even if the hook were dead)\n'; \
	exit 1; \
fi
@printf '>>> test-autoexec-mutant: green (no-autoexec mutant correctly RED -- AUTOEXEC markers absent, the oracle bites)\n'
```

## test-batch

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7640):
Prerequisites: `build/test_batch build/.makefile.stamp`
```make
@printf ">>> test-batch: expand (%%0-9/%%VAR%%/%%%%) + classify + label_matches\n"
@$(TEST_BATCH)
@printf ">>> test-batch: green\n"
```

## test-batch-break-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 22):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/batch_break_child.bin build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch_break.sh $(SECONDARY_TRACER_IMG)
@printf '>>> test-batch-break-emu: green (oracle completed)\n'
```

## test-batch-break-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 53):
Prerequisites: `build/qemu_harness build/batch_break_child.bin build/tracer_mut_safety_batch_terminate.img build/tracer_mut_safety_batch_resume.img build/tracer_mut_safety_batch_child.img build/.makefile.stamp`
```make
@set -eu; for pair in terminate:pause-n resume:pause-y; do \
 m=$${pair%%:*};mode=$${pair#*:}; \
 if sh harness/diff/fat_diff/batch_break.sh $(BUILD)/tracer_mut_safety_batch_$$m.img $$m $$mode >$(BUILD)/batch_break_emu_mut_$$m.log 2>&1; then echo "RED batch $$m survived QEMU";exit 1;fi; \
 grep '^RED batch-break-emu: .* sentinel' $(BUILD)/batch_break_emu_mut_$$m.log; done
@echo 'GREEN batch-break-emu-mutant: opposite post-question sentinels catch both mutants'
@rc=0; sh harness/diff/fat_diff/batch_break.sh $(BUILD)/tracer_mut_safety_batch_child.img drop-child exec-n >$(BUILD)/batch_break_emu_mut_child.log 2>&1 || rc=$$?; test $$rc -eq 1
@grep '^RED batch-break-emu: exec-n catalogue question missing' $(BUILD)/batch_break_emu_mut_child.log
@echo 'GREEN batch-break-emu-mutant: dropped EXEC child propagation caught'
@printf '>>> test-batch-break-emu-mutant: green (oracle completed)\n'
```

## test-batch-break-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 15):
Prerequisites: `build/batch_break build/.makefile.stamp`
```make
@$<
@printf '>>> test-batch-break-host: green (oracle completed)\n'
```

## test-batch-break-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 27):
Prerequisites: `build/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_TERMINATE build/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_RESUME build/batch_break_mut_CMD_MUTATE_BATCH_DROP_CHILD_BREAK build/.makefile.stamp`
```make
@set -eu; for m in TERMINATE RESUME; do \
 if $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_ALWAYS_$$m > $(BUILD)/batch_break_mut_$$m.log 2>&1; then echo "RED batch mutant survived: $$m";exit 1;fi; \
 grep '^RED batch-break:' $(BUILD)/batch_break_mut_$$m.log; done
@echo 'GREEN batch-break-mutant: always terminate and always resume each caught'
@rc=0; $(BUILD)/batch_break_mut_CMD_MUTATE_BATCH_DROP_CHILD_BREAK > $(BUILD)/batch_break_mut_CHILD.log 2>&1 || rc=$$?; test $$rc -eq 1
@grep '^RED batch-break: EXEC child asks after cleanup' $(BUILD)/batch_break_mut_CHILD.log
@echo 'GREEN batch-break-mutant: dropped EXEC child propagation caught'
@printf '>>> test-batch-break-mutant: green (oracle completed)\n'
```

## test-batch-exec

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7693):
Prerequisites: `build/test_batch_exec build/.makefile.stamp`
```make
@printf ">>> test-batch-exec: IF [NOT] ERRORLEVEL/EXIST/str==str + FOR parse/tokenize/subst\n"
@$(TEST_BATCH_EXEC)
@printf ">>> test-batch-exec: green\n"
```

## test-batch-exec-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7698):
Prerequisites: `build/test_batch_exec_mutant_ifignorenot build/test_batch_exec_mutant_fornotokens build/.makefile.stamp`
```make
@printf ">>> test-batch-exec-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_BATCH_EXEC_MUT_NOT) >/dev/null 2>&1; then \
	printf '!!! test-batch-exec-mutant FAIL: if-ignore-not mutant PASSED -- the IF-NOT test is decoration\n'; exit 1; \
else printf '>>> test-batch-exec-mutant: green (if-ignore-not mutant correctly RED)\n'; fi
@if $(TEST_BATCH_EXEC_MUT_FOR) >/dev/null 2>&1; then \
	printf '!!! test-batch-exec-mutant FAIL: for-no-tokens mutant PASSED -- the FOR test is decoration\n'; exit 1; \
else printf '>>> test-batch-exec-mutant: green (for-no-tokens mutant correctly RED -- the oracle bites)\n'; fi
```

## test-batch-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7646):
Prerequisites: `build/test_batch_mutant_nopctpct build/test_batch_mutant_echooff build/test_batch_mutant_gotonolabel build/.makefile.stamp`
```make
@printf ">>> test-batch-mutant: confirming all three mutants go RED (Rule 6)\n"
@if $(TEST_BATCH_MUT_PCTPCT) >/dev/null 2>&1; then \
	printf '!!! test-batch-mutant FAIL: no-pct-pct mutant PASSED -- the %%%% test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-batch-mutant: green (no-pct-pct mutant correctly RED)\n'; \
fi
@if $(TEST_BATCH_MUT_ECHOOFF) >/dev/null 2>&1; then \
	printf '!!! test-batch-mutant FAIL: echo-off-miss mutant PASSED -- the ECHO OFF test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-batch-mutant: green (echo-off-miss mutant correctly RED)\n'; \
fi
@if $(TEST_BATCH_MUT_GOTO) >/dev/null 2>&1; then \
	printf '!!! test-batch-mutant FAIL: goto-nolabel mutant PASSED -- the GOTO target test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-batch-mutant: green (goto-nolabel mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-bsy7-redir

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31257):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/gobble_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy7.bat harness/diff/fat_diff/fixtures/gobble_in.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-bsy7-redir : EXTERNAL .COM `<` INPUT redirect\n'
@printf '  beads initech-bsy.7. AUTOEXEC.BAT: GOBBLE.COM < GIN.TXT ; GOBBLE.COM < NOPE.TXT.\n'
@printf '  Proves the shell opens GIN.TXT + DUP2s it onto handle 0 so the EXEC\n'
@printf '  child reads the FILE from stdin (not the keyboard); and that a `<` on a\n'
@printf '  MISSING source prints "File not found" + does NOT run the command (DOS 3.3).\n'
@printf '  Prereqs bsy.9+hsct.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-bsy7-redir FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-bsy7-redir FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call bsy7-redir-mint-disk,$(BSY7_REDIR_IMG))
@printf 'Booting   : %s + data disk %s (AUTOEXEC.BAT redirect script + GOBBLE.COM + GIN.TXT)\n' "$(SECONDARY_TRACER_IMG)" "$(BSY7_REDIR_IMG)"
@printf 'Expecting : GOBBLE-BEGIN once, GOBBLED42 once (echoed from stdin), "File not found", then clean EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(BSY7_REDIR_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BSY7_REDIR_IMG)" \
	--name "$(BSY7_REDIR_NAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY7_REDIR_REPORT)" || true
@cat "$(BSY7_REDIR_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(BSY7_REDIR_REPORT)"; then \
	printf '!!! test-bsy7-redir FAIL: TRIPLE FAULT -- INPUT redirect crashed (root-cause the dup2/JFT-inherit path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-bsy7-redir [1/6]: no triple-fault\n'
@if [ ! -s "$(BSY7_REDIR_SERIAL)" ]; then \
	printf '!!! test-bsy7-redir FAIL: no serial captured at %s\n' "$(BSY7_REDIR_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(BSY7_REDIR_SERIAL)" \
	|| { printf '!!! test-bsy7-redir FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-bsy7-redir [2/6]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY7_REDIR_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY7_REDIR_NAME).repl"
@bcnt=$$(grep -oF 'GOBBLE-BEGIN' "$(BUILD)/$(BSY7_REDIR_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$bcnt" != "1" ]; then \
	printf '!!! test-bsy7-redir FAIL: GOBBLE-BEGIN seen %s time(s), expected EXACTLY 1.  count==0 means GOBBLE.COM never ran; count==2 means the MISSING-source `< NOPE.TXT` line WRONGLY ran GOBBLE instead of aborting with "File not found" (root-cause the input-open fail-loud path, Rule 3)\n' "$$bcnt"; \
	exit 1; \
fi
@printf '>>> test-bsy7-redir [3/6]: GOBBLE-BEGIN appears EXACTLY once (GOBBLE.COM ran for the valid source; the missing-source line did NOT run it)\n'
@gcnt=$$(grep -oF 'GOBBLED42' "$(BUILD)/$(BSY7_REDIR_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" != "1" ]; then \
	printf '!!! test-bsy7-redir FAIL: GOBBLED42 seen %s time(s), expected EXACTLY 1 (echoed once from the redirected stdin).  count==0 means GOBBLE read the KEYBOARD, not GIN.TXT; the `<` redirect did NOT re-point handle 0 (root-cause cmd_redir_parse `<` / dos_open+dup2(0) / child JFT-inherit of handle 0, Rule 3)\n' "$$gcnt"; \
	exit 1; \
fi
@printf '>>> test-bsy7-redir [4/6]: GOBBLED42 appears EXACTLY once -- `GOBBLE.COM < GIN.TXT` fed the FILE to stdin (the EXEC child read handle 0 = GIN.TXT, not the keyboard)\n'
@grep -qF 'File not found' "$(BUILD)/$(BSY7_REDIR_NAME).repl" \
	|| { printf '!!! test-bsy7-redir FAIL: "File not found" missing -- `GOBBLE.COM < NOPE.TXT` on a nonexistent source did not print the DOS 3.3 diagnostic (root-cause the dos_open<0 fail-loud path in run_with_redirect, Rule 3)\n'; exit 1; }
@printf '>>> test-bsy7-redir [5/6]: `GOBBLE.COM < NOPE.TXT` (missing source) printed "File not found" + did NOT run the command (authentic DOS 3.3; MS-DOS 3.3 Tech Ref Ch.6)\n'
@grep -q '^SHELL-EXIT$$' "$(BSY7_REDIR_SERIAL)" \
	|| { printf '!!! test-bsy7-redir FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run (handle 0 may not have been restored to CON after the child, or a refcount over-free panicked)\n'; exit 1; }
@printf '>>> test-bsy7-redir [6/6]: post-redirect `exit` reached the REPL + halted cleanly (handle 0 restored to CON; no SFT over-free)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- external `GOBBLE.COM < file` really redirects stdin on the 386\n'
@printf '            (child inherits parent JFT; QEMU only, tri-emulator pending initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-bsy7-redir: green (oracle completed)\n'
```

## test-bsy7-redir-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31351):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_nolt.img build/gobble_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy7.bat harness/diff/fat_diff/fixtures/gobble_in.txt build/.makefile.stamp`
```make
@printf '>>> test-bsy7-redir-mutant: confirming the NO-LT (`<` parse dropped) mutant goes RED (Rule 6)\n'
$(call bsy7-redir-mint-disk,$(BSY7_REDIR_MUT_IMG))
@$(call fixture-preflight,$(BSY7_REDIR_MUT_IMG))
@$(HARNESS_BIN) --disk "$(BSY7_REDIR_TRACER_MUT_IMG)" --disk2 "$(BSY7_REDIR_MUT_IMG)" \
	--name "$(BSY7_REDIR_MUT_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY7_REDIR_MUT_REPORT)" || true
@if [ ! -s "$(BSY7_REDIR_MUT_SERIAL)" ]; then \
	printf '!!! test-bsy7-redir-mutant FAIL: no serial from the mutant boot\n'; exit 1; \
fi
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY7_REDIR_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY7_REDIR_MUT_NAME).repl"
@gcnt=$$(grep -oF 'GOBBLED42' "$(BUILD)/$(BSY7_REDIR_MUT_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" = "1" ]; then \
	printf '!!! test-bsy7-redir-mutant FAIL: GOBBLED42 seen 1x under CMD_MUTATE_REDIR_NO_LT -- the `<` redirect would pass even with the parse dead; the emu gate is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-bsy7-redir-mutant: green (no-LT mutant correctly RED -- GOBBLED42 count != 1, the `<` parse is load-bearing for INPUT redirect)\n'
```

## test-bsy9-redir

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31113):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy9.bat build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-bsy9-redir : EXTERNAL .COM `>` OUTPUT redirect\n'
@printf '  beads initech-bsy.9. AUTOEXEC.BAT: GREET.COM> GOUT.TXT ; TYPE GOUT.TXT (x2).\n'
@printf '  Proves an EXEC child inherits the parent JFT so its stdout redirects to\n'
@printf '  the file (GREETINGS to GOUT.TXT, not the screen). Prereqs hsct+k36g+o0td.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-bsy9-redir FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-bsy9-redir FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call bsy9-redir-mint-disk,$(BSY9_REDIR_IMG))
@printf 'Booting   : %s + data disk %s (AUTOEXEC.BAT redirect script + GREET.COM)\n' "$(SECONDARY_TRACER_IMG)" "$(BSY9_REDIR_IMG)"
@printf 'Expecting : GREETINGS twice (2 TYPEs of GOUT.TXT), then clean EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(BSY9_REDIR_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BSY9_REDIR_IMG)" \
	--name "$(BSY9_REDIR_NAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY9_REDIR_REPORT)" || true
@cat "$(BSY9_REDIR_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(BSY9_REDIR_REPORT)"; then \
	printf '!!! test-bsy9-redir FAIL: TRIPLE FAULT -- EXEC redirect crashed (root-cause the JFT-inherit / sft_inherit refcount path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-bsy9-redir [1/4]: no triple-fault\n'
@if [ ! -s "$(BSY9_REDIR_SERIAL)" ]; then \
	printf '!!! test-bsy9-redir FAIL: no serial captured at %s\n' "$(BSY9_REDIR_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(BSY9_REDIR_SERIAL)" \
	|| { printf '!!! test-bsy9-redir FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-bsy9-redir [2/4]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY9_REDIR_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY9_REDIR_NAME).repl"
@gcnt=$$(grep -oF 'GREETINGS' "$(BUILD)/$(BSY9_REDIR_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" != "2" ]; then \
	printf '!!! test-bsy9-redir FAIL: GREETINGS seen %s time(s), expected EXACTLY 2 (one per TYPE of GOUT.TXT).  count==1 means the EXEC child printed to SCREEN and GOUT.TXT is empty; the child did NOT inherit the parent JFT (root-cause psp_build parent_jft copy / loader_run_plan / sft_inherit, Rule 3)\n' "$$gcnt"; \
	exit 1; \
fi
@printf '>>> test-bsy9-redir [3/4]: GREETINGS appears EXACTLY twice -- `GREET.COM > GOUT.TXT` REDIRECTED (the EXEC child inherited the parent`s redirected stdout; it did NOT leak to screen)\n'
@grep -q '^SHELL-EXIT$$' "$(BSY9_REDIR_SERIAL)" \
	|| { printf '!!! test-bsy9-redir FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run (handle 1 may not have been restored to CON after the child, or a refcount over-free panicked)\n'; exit 1; }
@printf '>>> test-bsy9-redir [4/4]: post-redirect `exit` reached the REPL + halted cleanly (handle 1 restored to CON; no SFT over-free)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- external `GREET.COM > file` really redirects on the 386\n'
@printf '            (child inherits parent JFT; QEMU only, tri-emulator pending initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-bsy9-redir: green (oracle completed)\n'
```

## test-bsy9-redir-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31196):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_no_jft_inherit.img build/greet_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy9.bat build/.makefile.stamp`
```make
@printf '>>> test-bsy9-redir-mutant: confirming the NO-JFT-INHERIT mutant goes RED (Rule 6)\n'
$(call bsy9-redir-mint-disk,$(BSY9_REDIR_MUT_IMG))
@$(call fixture-preflight,$(BSY9_REDIR_MUT_IMG))
@$(HARNESS_BIN) --disk "$(BSY9_REDIR_TRACER_MUT_IMG)" --disk2 "$(BSY9_REDIR_MUT_IMG)" \
	--name "$(BSY9_REDIR_MUT_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY9_REDIR_MUT_REPORT)" || true
@if [ ! -s "$(BSY9_REDIR_MUT_SERIAL)" ]; then \
	printf '!!! test-bsy9-redir-mutant FAIL: no serial from the mutant boot\n'; exit 1; \
fi
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY9_REDIR_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY9_REDIR_MUT_NAME).repl"
@gcnt=$$(grep -oF 'GREETINGS' "$(BUILD)/$(BSY9_REDIR_MUT_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" = "2" ]; then \
	printf '!!! test-bsy9-redir-mutant FAIL: GREETINGS seen 2x under PSP_MUT_NO_JFT_INHERIT -- the redirect would pass even with JFT inheritance dead; the emu gate is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-bsy9-redir-mutant: green (no-JFT-inherit mutant correctly RED -- GREETINGS count != 2, the JFT inheritance is load-bearing for external EXEC redirect)\n'
```

## test-chkdsk

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/chkdsk.mk', line 55):
Prerequisites: `build/chkdsk_host build/chkdsk_fixture build/.makefile.stamp`
```make
@sh $(CHKDSK_DIR)/chkdsk_label.sh $(BUILD)/chkdsk/labels $(CHKDSK_HOST)
@sh $(CHKDSK_DIR)/chkdsk.sh host $(BUILD) $(CHKDSK_HOST) $(CHKDSK_FIXTURE)
```

## test-chkdsk-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/chkdsk.mk', line 58):
Prerequisites: `build/chkdsk_program.bin build/chkdsk_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(CHKDSK_PROG_BIN) $(CHKDSK_FIXTURE) $(SECONDARY_TRACER_IMG)
```

## test-chkdsk-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 126):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-chkdsk-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-chkdsk-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/chkdsk-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/chkdsk-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "chkdsk-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/chkdsk-emu_bochs.report.txt" || true
@cat "$(BUILD)/chkdsk-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-chkdsk-emu-bochs "$(BUILD)/chkdsk-emu_bochs.report.txt" "$(BUILD)/chkdsk-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-chkdsk-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/chkdsk.mk', line 68):
Prerequisites: `build/chkdsk_REPORT.bin build/chkdsk_LOST.bin build/chkdsk_CROSS.bin build/chkdsk_INVALID.bin build/chkdsk_VERBOSE.bin build/chkdsk_FRAG.bin build/chkdsk_NOFIX.bin build/chkdsk_RECOVER.bin build/chkdsk_DISCARD.bin build/chkdsk_MEMORY.bin build/chkdsk_MIRROR.bin build/chkdsk_LABEL.bin build/chkdsk_fixture build/qemu_harness build/tracer_secondary.img build/chkdsk_cache_boot.img build/chkdsk_program.bin build/.makefile.stamp`
```make
@set -eu; for m in $(CHKDSK_MUTANTS); do \
 log=$(BUILD)/chkdsk-emu-mutant-$$m.log; \
 if INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(BUILD)/chkdsk_$$m.bin $(CHKDSK_FIXTURE) $(SECONDARY_TRACER_IMG) > $$log 2>&1; then \
  echo "FAIL CHKDSK emu mutant $$m survived"; exit 1; fi; \
 grep 'RED CHKDSK-' $$log | tail -1 || { cat $$log; exit 1; }; \
 echo "GREEN CHKDSK emu mutant $$m rejected"; done
@log=$(BUILD)/chkdsk-emu-mutant-CACHE.log; \
 if INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(CHKDSK_PROG_BIN) $(CHKDSK_FIXTURE) $(BUILD)/chkdsk_cache_boot.img > $$log 2>&1; then \
 echo 'FAIL CHKDSK CACHE mutant survived'; exit 1; fi; \
 grep -F 'RED CHKDSK-CACHE' $$log || { cat $$log; exit 1; }; \
 echo 'GREEN CHKDSK emu mutant CACHE rejected'
```

## test-chkdsk-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/chkdsk.mk', line 60):
Prerequisites: `build/chkdsk_host_REPORT build/chkdsk_host_LOST build/chkdsk_host_CROSS build/chkdsk_host_INVALID build/chkdsk_host_VERBOSE build/chkdsk_host_FRAG build/chkdsk_host_NOFIX build/chkdsk_host_RECOVER build/chkdsk_host_DISCARD build/chkdsk_host_MEMORY build/chkdsk_host_MIRROR build/chkdsk_host_LABEL build/chkdsk_fixture build/.makefile.stamp`
```make
@set -eu; for m in $(CHKDSK_MUTANTS); do \
 log=$(BUILD)/chkdsk-mutant-$$m.log; \
 if { sh $(CHKDSK_DIR)/chkdsk_label.sh $(BUILD)/chkdsk/labels $(BUILD)/chkdsk_host_$$m && \
      sh $(CHKDSK_DIR)/chkdsk.sh host $(BUILD) $(BUILD)/chkdsk_host_$$m $(CHKDSK_FIXTURE); } > $$log 2>&1; then \
  echo "FAIL CHKDSK mutant $$m survived"; exit 1; fi; \
 grep 'RED CHKDSK-' $$log | tail -1 || { cat $$log; exit 1; }; \
 echo "GREEN CHKDSK mutant $$m rejected"; done
```

## test-copy-selfcopy

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26924):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-copy-selfcopy : COPY <file> <file> no data loss\n'
@printf '  Ref: DOS 3.3 COMMAND.COM COPY; spec/dos_messages.json (MSG-DOS-0020). Law 2/Rule 2.\n'
@printf '  beads initech-ojxn (P0). Inject: copy test.txt test.txt / type test.txt / exit.\n'
@printf '======================================================================\n'
$(ojxn-mint-disk)
@printf 'Booting   : %s + FRESH WRITABLE disk %s (multi-cluster TEST.TXT, TAIL=%s)\n' "$(SECONDARY_TRACER_IMG)" "$(OJXN_IMG)" "$(OJXN_TAIL)"
@printf 'Expecting : "File cannot be copied onto itself" + "0 File(s) copied" + TEST.TXT intact\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(OJXN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(OJXN_IMG)" \
	--name "$(OJXN_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(OJXN_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(OJXN_REPORT)" || true
@cat "$(OJXN_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(OJXN_REPORT)"; then \
	printf '!!! test-copy-selfcopy FAIL: TRIPLE FAULT -- the shell boot or COPY crashed\n'; exit 1; \
fi
@printf '>>> test-copy-selfcopy [1/5]: no triple-fault\n'
@if [ ! -s "$(OJXN_SERIAL)" ]; then \
	printf '!!! test-copy-selfcopy FAIL: no serial captured at %s\n' "$(OJXN_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(OJXN_SERIAL)" \
	|| { printf '!!! test-copy-selfcopy FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-copy-selfcopy [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to the REPL region (after SHELL-READY) -- the shell's own output.
@sed -n '/^SHELL-READY$$/,$$p' "$(OJXN_SERIAL)" | tr -d '\r' > "$(BUILD)/$(OJXN_NAME).repl"
@grep -qF 'File cannot be copied onto itself' "$(BUILD)/$(OJXN_NAME).repl" \
	|| { printf '!!! test-copy-selfcopy FAIL: the onto-itself diagnostic (MSG-DOS-0020) MISSING -- the guard did not fire\n'; cat "$(BUILD)/$(OJXN_NAME).repl"; exit 1; }
@printf '>>> test-copy-selfcopy [3/5]: "File cannot be copied onto itself" (MSG-DOS-0020) emitted\n'
@grep -qF '0 File(s) copied' "$(BUILD)/$(OJXN_NAME).repl" \
	|| { printf '!!! test-copy-selfcopy FAIL: the ZERO-count footer "0 File(s) copied" MISSING\n'; cat "$(BUILD)/$(OJXN_NAME).repl"; exit 1; }
@printf '>>> test-copy-selfcopy [4/5]: "0 File(s) copied" zero-count footer (nothing copied)\n'
@# THE no-data-loss assertion: TYPE (AFTER the COPY) still shows the TAIL marker,
@# which lives in the LAST cluster -- so the multi-cluster file was NOT truncated.
@grep -qF '$(OJXN_TAIL)' "$(BUILD)/$(OJXN_NAME).repl" \
	|| { printf '!!! test-copy-selfcopy FAIL: DATA LOSS -- TEST.TXT TAIL marker GONE after COPY onto itself (multi-cluster truncation; root-cause the same-file guard, Rule 3)\n'; cat "$(BUILD)/$(OJXN_NAME).repl"; exit 1; }
@printf '>>> test-copy-selfcopy [5/5]: TEST.TXT TAIL marker intact -- multi-cluster file PRESERVED (NO DATA LOSS)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- COPY onto itself refused (MSG-DOS-0020 + zero footer), file intact (initech-ojxn)\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-copy-selfcopy-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26973):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_ojxn.img build/.makefile.stamp`
```make
@printf '>>> test-copy-selfcopy-mutant: confirming the no-samefile mutant DESTROYS TEST.TXT (Rule 6)\n'
$(ojxn-mint-disk)
@$(call fixture-preflight,$(OJXN_IMG))
@$(HARNESS_BIN) --disk "$(OJXN_MUT_TRACER_IMG)" --disk2 "$(OJXN_IMG)" \
	--name "$(OJXN_MUT_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(OJXN_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(OJXN_MUT_REPORT)" || true
@if grep -q 'triple_fault=1' "$(OJXN_MUT_REPORT)"; then \
	printf '!!! test-copy-selfcopy-mutant FAIL: mutant TRIPLE FAULT -- cannot attribute the loss\n'; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(OJXN_MUT_SERIAL)" \
	|| { printf '!!! test-copy-selfcopy-mutant FAIL: mutant never entered the REPL -- RED is meaningless\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' "$(OJXN_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(OJXN_MUT_NAME).repl"
@# The mutant COPY ran to completion in the BUGGY path (reports success)...
@grep -qF '1 File(s) copied' "$(BUILD)/$(OJXN_MUT_NAME).repl" \
	|| { printf '!!! test-copy-selfcopy-mutant FAIL: mutant COPY did not report "1 File(s) copied" -- the cycle did not run as expected\n'; cat "$(BUILD)/$(OJXN_MUT_NAME).repl"; exit 1; }
@# ...the onto-itself diagnostic is ABSENT (the guard is disabled)...
@if grep -qF 'File cannot be copied onto itself' "$(BUILD)/$(OJXN_MUT_NAME).repl"; then \
	printf '!!! test-copy-selfcopy-mutant FAIL: the guard fired in the mutant -- CMD_MUTATE_NO_SAMEFILE not effective\n'; exit 1; \
fi
@# ...the HEAD marker (cluster 1) SURVIVES (confirms COPY genuinely ran + a
@# partial file remains -- so an ABSENT tail is real truncation, not a dead boot)...
@grep -qF '$(OJXN_HEAD)' "$(BUILD)/$(OJXN_MUT_NAME).repl" \
	|| { printf '!!! test-copy-selfcopy-mutant FAIL: HEAD marker absent -- the mutant TYPE produced nothing; cannot attribute a clean truncation\n'; cat "$(BUILD)/$(OJXN_MUT_NAME).repl"; exit 1; }
@# ...and the TAIL marker (last cluster) is GONE -> the multi-cluster file was
@# TRUNCATED by the copy-onto-itself (data loss reproduced -- the gate BITES).
@if grep -qF '$(OJXN_TAIL)' "$(BUILD)/$(OJXN_MUT_NAME).repl"; then \
	printf '!!! test-copy-selfcopy-mutant FAIL: TAIL marker still present -- the mutant did NOT truncate; the gate does not bite\n'; \
	cat "$(BUILD)/$(OJXN_MUT_NAME).repl"; exit 1; \
else \
	printf '>>> test-copy-selfcopy-mutant: green (mutant truncated TEST.TXT -- HEAD kept, TAIL GONE; the no-data-loss gate BITES)\n'; \
fi
```

## test-diskcopy-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 77):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/diskcopy_program.bin build/diskcomp_program.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/diskcopy_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG)
```

## test-dos-batch2-body

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 23):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-batch2-console

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 23):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-batch2-cwd

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 23):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-batch2-for

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 23):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-batch2-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 21):
Prerequisites: `build/batch2_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh host
```

## test-dos-batch2-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 47):
Prerequisites: `build/batch2_host_cwd build/batch2_host_ctrlz build/batch2_host_ctrlc build/.makefile.stamp`
```make
@set -e; for m in cwd ctrlz ctrlc; do \
  log=$(BUILD)/batch2_host_mutant_$$m.log; \
  if BATCH2_HOST=$(BUILD)/batch2_host_$$m sh harness/diff/fat_diff/batch2.sh host unused $$m >$$log 2>&1; then echo "FAIL batch2 host mutant survived: $$m"; exit 1; fi; \
  grep 'batch2-host RED: \(CD root\|Ctrl-Z caret\|Ctrl-C caret\)' $$log; \
  printf 'PASS batch2 host mutant %s caught RED\n' "$$m"; \
done
```

## test-dos-batch2-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 26):
Prerequisites: `build/qemu_harness build/console_text build/tracer_mut_safety_batch2body.img build/tracer_mut_safety_batch2shift.img build/tracer_mut_safety_batch2date.img build/tracer_mut_safety_batch2dollar.img build/tracer_mut_safety_batch2for.img build/tracer_mut_safety_batch2wild.img build/j1lu_batch2cwd.img build/j1lu_batch2ctrlz.img build/j1lu_batch2ctrlc.img build/.makefile.stamp`
```make
@set -e; for pair in body:body shift:body date:prompt dollar:prompt for:for wild:for cwd:cwd ctrlz:console ctrlc:console; do \
  m=$${pair%:*}; mode=$${pair#*:}; boot=$(BUILD)/tracer_mut_safety_batch2$$m.img; \
  case $$m in cwd|ctrlz|ctrlc) boot=$(BUILD)/j1lu_batch2$$m.img;; esac; \
  log=$(BUILD)/batch2_mutant_$$m.log; \
  if sh harness/diff/fat_diff/batch2.sh $$mode $$boot $$m >$$log 2>&1; then echo "FAIL batch2 mutant survived: $$m"; exit 1; fi; \
  case $$m in \
    body) why='IF/FOR body bypassed batch dispatcher';; shift) why='missing SHIFT-first-second';; \
    date) why='prompt date did not read clock';; dollar) why='literal dollar truncated prompt';; \
    for) why='interactive FOR differs from mdir';; wild) why='batch wildcard set differs from mdir';; \
    cwd) why='CD root/parent/absolute/getcwd';; ctrlz) why='console contains an undecodable control glyph';; \
    ctrlc) why='Ctrl-C did not cancel input with ^C';; esac; \
  grep -F "batch2-$$mode RED: $$why" $$log; \
  printf 'PASS batch2 mutant %s caught RED\n' "$$m"; \
done
```

## test-dos-batch2-prompt

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/batch2.mk', line 23):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-folder

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_folder.mk', line 9):
Prerequisites: `build/flair_data.img build/flair_tenants.img build/qemu_harness build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
$(call exitdos-boot,$(FLAIRTENANTS_IMG),dos_folder,$(FLAIR_DATA_IMG),$(DOS_FOLDER_EXIT_SPEC),$(DOS_FOLDER_DONE),0,60000)
@sh $(DOS_FOLDER_DIR)/dos_folder.sh emu $(BUILD)/dos_folder_data.img $(BUILD)/dos-folder-emu $(BUILD)/dos_folder.serial
$(call exitdos-boot,$(FLAIRTENANTS_IMG),dos_folder_finder,$(FLAIR_DATA_IMG),$(DOS_FOLDER_FINDER_SPEC),FINDER-OPEN-FOLDER name=DOS win=1,1,40000)
@sh $(DOS_FOLDER_DIR)/dos_folder.sh finder $(BUILD)/dos_folder_finder_data.img $(BUILD)/dos-folder-finder $(BUILD)/dos_folder_finder.serial
@$(PPM_FLAIR_DISKWIN_CHECK_BIN) dosfolder $(BUILD)/dos_folder_finder.ppm
```

## test-dos-folder-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 220):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dos-folder-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-dos-folder-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/dos-folder_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/dos-folder_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "dos-folder_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/dos-folder_bochs.report.txt" || true
@cat "$(BUILD)/dos-folder_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-dos-folder-bochs "$(BUILD)/dos-folder_bochs.report.txt" "$(BUILD)/dos-folder_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-dos-folder-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_folder.mk', line 6):
Prerequisites: `build/flair_data.img build/fat_dump build/.makefile.stamp`
```make
@sh $(DOS_FOLDER_DIR)/dos_folder.sh host $(FLAIR_DATA_IMG) $(BUILD)/dos-folder-host
```

## test-dos-folder-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_folder.mk', line 18):
Prerequisites: `test-dos-folder-host build/.makefile.stamp`
```make
@set -eu; for m in EXTERNAL_IN_APPS EXTRA_DOS_FILE PATH_UNSET NO_AUTOEXEC; do \
 img=$(BUILD)/dos-folder-$$m.img; log=$(BUILD)/dos-folder-$$m.log; \
 cp -f $(FLAIR_DATA_IMG) $$img; \
 case $$m in EXTERNAL_IN_APPS) mmove -i $$img ::DOS/MORE.COM ::APPS/MORE.COM; rule=external-set;; \
 EXTRA_DOS_FILE) mcopy -i $$img $(FLAIR_DATA_README) ::DOS/JUNK.TXT; rule=external-set;; \
 PATH_UNSET) printf '@ECHO OFF\r\n' | mcopy -o -i $$img - ::AUTOEXEC.BAT; rule=autoexec;; \
 NO_AUTOEXEC) mdel -i $$img ::AUTOEXEC.BAT; rule=autoexec;; esac; \
 if sh $(DOS_FOLDER_DIR)/dos_folder.sh host $$img $(BUILD)/dos-folder-mut-host > $$log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 grep -x "RED dos-folder $$rule" $$log; echo "GREEN test-dos-folder-host-mutant $$m rejected"; done
```

## test-dos-folder-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_folder.mk', line 29):
Prerequisites: `test-dos-folder-host-mutant build/flair_tenants.img build/qemu_harness build/ppm_flair_disk_windows_check build/.makefile.stamp`
```make
@set -eu; for m in EXTERNAL_IN_APPS EXTRA_DOS_FILE PATH_UNSET NO_AUTOEXEC DOS_NOT_DIRECTORY LABEL_TAIL; do \
 img=$(BUILD)/dos-folder-$$m.img; name=dos_folder_mut_$$m; log=$(BUILD)/dos-folder-emu-$$m.log; \
 trace='$(DOS_FOLDER_EXIT_SPEC)'; done="$(DOS_FOLDER_DONE)"; mode=emu; rule=serial-external-set; dump=; \
 case $$m in PATH_UNSET) rule='PATH: Bad command or file name';; \
 NO_AUTOEXEC) rule=startup-prompts;; \
 DOS_NOT_DIRECTORY) cp -f $(FLAIR_DATA_IMG) $$img; mdeltree -i $$img ::DOS; printf '' | mcopy -i $$img - ::DOS; trace='$(DOS_FOLDER_FINDER_SPEC)'; done='FINDER-OPEN-FOLDER name=DOS'; mode=finder; rule=finder-folder;; \
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
```

## test-dos-safety-audit

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30515):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh audit $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-audit-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30529):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_boot_mut_ojxn.img build/tracer_mut_safety_k02.img build/tracer_mut_safety_k03.img build/.makefile.stamp`
```make
@for item in 'k01:$(OJXN_MUT_TRACER_IMG):SELF.BIN bytes changed' \
    'k02:$(BUILD)/tracer_mut_safety_k02.img:FILLED/README.TXT missing' \
    'k03:$(BUILD)/tracer_mut_safety_k03.img:KEEP.TXT missing'; do \
    tag=$${item%%:*}; rest=$${item#*:}; image=$${rest%%:*}; why=$${rest#*:}; \
    log=$(BUILD)/dos_safety_audit_$$tag.log; \
    sh harness/diff/fat_diff/dos_safety.sh audit "$$image" _$$tag > "$$log" 2>&1; rc=$$?; \
    [ $$rc -ne 0 ] && grep -qF "FAIL audit: $$why" "$$log" \
    || { cat "$$log"; exit 1; }; \
    printf '>>> test-dos-safety-audit-mutant: %s RED -- %s\n' "$$tag" "$$why"; \
done
@printf 'VERDICT: PASS -- test-dos-safety-audit-mutant (K01/K02/K03 regressions each RED on stopped disk)\n'
```

## test-dos-safety-copy-ro

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30652):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh $(patsubst test-dos-safety-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-copy-ro-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30636):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_protection_copy.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_safety.sh copy-ro $(BUILD)/tracer_mut_protection_copy.img _mut > $(BUILD)/copy_ro_mutant.log 2>&1; then echo 'FAIL: COPY protection mutant passed'; exit 1; fi
@grep -q 'read-only refusal changed disk' $(BUILD)/copy_ro_mutant.log
@printf 'VERDICT: PASS -- test-dos-safety-copy-ro-mutant (read-only refusal changed disk, RED)\n'
```

## test-dos-safety-create

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30551):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh create unused
```

## test-dos-safety-create-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30553):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety_mut_create build/.makefile.stamp`
```make
@DOS_SAFETY_HOST=$(DOS_SAFETY_CREATE_MUT) sh harness/diff/fat_diff/dos_safety.sh create unused _mutant > $(BUILD)/dos_safety_create_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL .*CREAT rejects nonregular entry' $(BUILD)/dos_safety_create_mutant.log \
|| { cat $(BUILD)/dos_safety_create_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-create-mutant (nonregular guard removed: CREAT rejection, RED)\n'
```

## test-dos-safety-create-ro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30650):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh $(patsubst test-dos-safety-%,%,$@) unused
```

## test-dos-safety-del-failure

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30508):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_denied.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh failure $(BUILD)/tracer_mut_safety_denied.img
```

## test-dos-safety-del-failure-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30510):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_silent.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh failure $(BUILD)/tracer_mut_safety_silent.img _mutant > $(BUILD)/dos_safety_failure_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL failure: deletion failure diagnostic missing' $(BUILD)/dos_safety_failure_mutant.log \
|| { cat $(BUILD)/dos_safety_failure_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-del-failure-mutant (silent failed unlink: diagnostic missing, RED)\n'
```

## test-dos-safety-del-ro

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30652):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh $(patsubst test-dos-safety-%,%,$@) $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-del-ro-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30640):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_protection_del.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_safety.sh del-ro $(BUILD)/tracer_mut_protection_del.img _mut > $(BUILD)/del_ro_mutant.log 2>&1; then echo 'FAIL: DEL protection mutant passed'; exit 1; fi
@grep -q 'read-only refusal changed disk' $(BUILD)/del_ro_mutant.log
@printf 'VERDICT: PASS -- test-dos-safety-del-ro-mutant (read-only refusal changed disk, RED)\n'
```

## test-dos-safety-del-ro-progress-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30645):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_rostop.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_safety.sh del-ro $(BUILD)/tracer_mut_safety_rostop.img _stop > $(BUILD)/del_ro_progress_mutant.log 2>&1; then echo 'FAIL: protected-stop mutant passed'; exit 1; fi
@grep -q 'LOCK0.TXT still present' $(BUILD)/del_ro_progress_mutant.log
@printf 'VERDICT: PASS -- test-dos-safety-del-ro-progress-mutant (writable LOCK0.TXT still present, RED)\n'
```

## test-dos-safety-identity

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30492):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh identity unused
```

## test-dos-safety-identity-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30494):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety_mut_identity build/.makefile.stamp`
```make
@DOS_SAFETY_HOST=$(DOS_SAFETY_HOST_MUT) sh harness/diff/fat_diff/dos_safety.sh identity unused _mutant > $(BUILD)/dos_safety_identity_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL .*resolved entry identity' $(BUILD)/dos_safety_identity_mutant.log \
|| { cat $(BUILD)/dos_safety_identity_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-identity-mutant (false identity: resolved entry identity, RED)\n'
```

## test-dos-safety-k01

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30499):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k01 $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-k01-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30564):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_boot_mut_ojxn.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k01 $(OJXN_MUT_TRACER_IMG) _mutant > $(BUILD)/dos_safety_k01_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL k01: SELF.BIN bytes changed' $(BUILD)/dos_safety_k01_mutant.log \
|| { cat $(BUILD)/dos_safety_k01_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-k01-mutant (self guard removed: SELF.BIN bytes changed, RED)\n'
```

## test-dos-safety-k02

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30502):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k02 $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-k02-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30558):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_k02.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k02 $(BUILD)/tracer_mut_safety_k02.img _mutant > $(BUILD)/dos_safety_k02_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL k02: FILLED/README.TXT missing' $(BUILD)/dos_safety_k02_mutant.log \
|| { cat $(BUILD)/dos_safety_k02_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-k02-mutant (directory basename omitted: child missing, RED)\n'
```

## test-dos-safety-k03

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30504):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k03 $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-k03-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30541):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_k03.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k03 $(BUILD)/tracer_mut_safety_k03.img _mutant > $(BUILD)/dos_safety_k03_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL k03: KEEP.TXT missing' $(BUILD)/dos_safety_k03_mutant.log \
|| { cat $(BUILD)/dos_safety_k03_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-k03-mutant (parent lost: root KEEP.TXT missing, RED)\n'
```

## test-dos-safety-k04

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30506):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k04 $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-k04-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30546):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_k04.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k04 $(BUILD)/tracer_mut_safety_k04.img _mutant > $(BUILD)/dos_safety_k04_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep -q 'FAIL k04: D017/Z\*.TXT still present' $(BUILD)/dos_safety_k04_mutant.log \
|| { cat $(BUILD)/dos_safety_k04_mutant.log; exit 1; }; \
printf 'VERDICT: PASS -- test-dos-safety-k04-mutant (sixteen cap restored: seventeenth match remains, RED)\n'
```

## test-dos-safety-k15

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30517):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh k15n $(SECONDARY_TRACER_IMG)
@sh harness/diff/fat_diff/dos_safety.sh k15y $(SECONDARY_TRACER_IMG)
```

## test-dos-safety-k15-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30520):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_k15.img build/tracer_mut_safety_yesall.img build/tracer_mut_safety_refusey.img build/.makefile.stamp`
```make
@for item in 'k15:k15n:non-Y response changed disk' 'yesall:k15n:non-Y response changed disk' 'refusey:k15y:DELTEST/*.TXT still present'; do \
    tag=$${item%%:*}; rest=$${item#*:}; mode=$${rest%%:*}; why=$${rest#*:}; log=$(BUILD)/dos_safety_$$tag.log; \
    sh harness/diff/fat_diff/dos_safety.sh "$$mode" $(BUILD)/tracer_mut_safety_$$tag.img _$$tag > "$$log" 2>&1; rc=$$?; \
    [ $$rc -ne 0 ] && grep -qF "FAIL $$mode: $$why" "$$log" \
    || { cat "$$log"; exit 1; }; \
    printf '>>> test-dos-safety-k15-mutant: %s RED -- %s\n' "$$tag" "$$why"; \
done
@printf 'VERDICT: PASS -- test-dos-safety-k15-mutant (no prompt / non-Y accepted / Y refused each RED)\n'
```

## test-dos-safety-ro-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30629):
Prerequisites: `build/dos_safety_fixture build/test_dos_protection_create build/test_dos_protection_unlink build/test_dos_protection_nonregular build/test_dos_protection_write build/test_dos_protection_open build/.makefile.stamp`
```make
@set -e; for pair in 'create:create-ro:CREAT read-only refusal' 'unlink:unlink-ro:UNLINK read-only refusal' 'nonregular:unlink-ro:UNLINK nonregular refusal' 'write:write-ro:positioned write read-only refusal' 'open:write-ro:OPEN write read-only refusal'; do \
  tag=$${pair%%:*}; rest=$${pair#*:}; mode=$${rest%%:*}; why=$${rest#*:}; log=$(BUILD)/dos_protection_$$tag.log; \
  if DOS_SAFETY_HOST=$(BUILD)/test_dos_protection_$$tag sh harness/diff/fat_diff/dos_safety.sh $$mode unused _mut_$$tag > $$log 2>&1; then echo "FAIL: $$tag mutant passed"; exit 1; fi; \
  grep -F "$$why" $$log | grep -q FAIL || { cat $$log; exit 1; }; \
  printf 'VERDICT: PASS -- protection mutant %s RED (%s)\n' "$$tag" "$$why"; \
done
```

## test-dos-safety-unlink-ro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30650):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh $(patsubst test-dos-safety-%,%,$@) unused
```

## test-dos-safety-write-ro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 30650):
Prerequisites: `build/dos_safety_fixture build/test_dos_safety build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_safety.sh $(patsubst test-dos-safety-%,%,$@) unused
```

## test-dosutil-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 83):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/attrib_program.bin build/label_program.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/dosutil_emu.sh $(SECONDARY_TRACER_IMG) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) normal
```

## test-dosutil-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 128):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dosutil-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-dosutil-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/dosutil-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/dosutil-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "dosutil-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/dosutil-emu_bochs.report.txt" || true
@cat "$(BUILD)/dosutil-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-dosutil-emu-bochs "$(BUILD)/dosutil-emu_bochs.report.txt" "$(BUILD)/dosutil-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-dosutil-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 85):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/attrib_program.bin build/label_program.bin build/attrib_mut_program.bin build/label_mut_program.bin build/dosutil_cache.img build/.makefile.stamp`
```make
@set -e; for m in attrib label cache; do \
  a=$(ATTRIB_PROG_BIN); l=$(LABEL_PROG_BIN); boot=$(SECONDARY_TRACER_IMG); \
  case $$m in attrib) a=$(BUILD)/attrib_mut_program.bin; why='E1 ATTRIB';; \
    label) l=$(BUILD)/label_mut_program.bin; why='E4 VOL no-label after Y';; \
    cache) boot=$(BUILD)/dosutil_cache.img; why='E4 VOL no-label after Y';; esac; \
  log=$(BUILD)/dosutil_emu_mut_$$m.log; \
  if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/dosutil_emu.sh $$boot $$a $$l mut_$$m >$$log 2>&1; then echo "FAIL: emu $$m survived"; exit 1; fi; \
  grep "^FAIL: $$why" $$log || { cat $$log; exit 1; }; \
  echo "PASS: $$m QEMU mutant caught RED"; \
done
```

## test-fdisk

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/fdisk.mk', line 8):
Prerequisites: `build/test_fdisk build/fat_dump build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/fdisk.sh $(BUILD) $<
```

## test-fdisk-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/fdisk.mk', line 35):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/fat_dump build/fdisk_program.bin build/fdisk_fixture.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/fdisk_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FDISK_PROG_BIN)
```

## test-fdisk-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 219):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fdisk-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-fdisk-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/fdisk-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/fdisk-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "fdisk-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/fdisk-emu_bochs.report.txt" || true
@cat "$(BUILD)/fdisk-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-fdisk-emu-bochs "$(BUILD)/fdisk-emu_bochs.report.txt" "$(BUILD)/fdisk-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-fdisk-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/fdisk.mk', line 46):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/fat_dump build/fdisk_fixture.bin build/fdisk_CHS.bin build/fdisk_SIGNATURE.bin build/fdisk_ACTIVE.bin build/fdisk_CONFIRM.bin build/fdisk_LOADED.bin build/.makefile.stamp`
```make
@set -eu; for m in CHS SIGNATURE ACTIVE CONFIRM LOADED; do \
 case $$m in CHS|SIGNATURE) why='RED fdisk-emu CHS/signature create MBR vs mpartition';; ACTIVE) why='RED fdisk-emu active flag on wrong entry';; CONFIRM) why='RED fdisk-emu delete without confirmation';; LOADED) why='RED fdisk-emu loaded volume was not refused';; esac; \
 if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/fdisk_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/fdisk_$$m.bin $$m > $(BUILD)/fdisk-emu-mut-$$m.log 2>&1; then echo "FAIL fdisk-emu $$m survived"; exit 1; fi; \
 grep -Fx "$$why" $(BUILD)/fdisk-emu-mut-$$m.log || { cat $(BUILD)/fdisk-emu-mut-$$m.log; exit 1; }; done
@echo 'GREEN test-fdisk-emu-mutant: CHS SIGNATURE ACTIVE CONFIRM LOADED rejected'
```

## test-fdisk-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/fdisk.mk', line 15):
Prerequisites: `build/fat_dump build/test_fdisk_CHS build/test_fdisk_SIGNATURE build/test_fdisk_ACTIVE build/test_fdisk_CONFIRM build/.makefile.stamp`
```make
@set -eu; for m in CHS SIGNATURE ACTIVE CONFIRM; do \
 case $$m in CHS|SIGNATURE) why='RED fdisk CHS/relative-sector/type/signature vs mpartition';; ACTIVE) why='RED fdisk active flag on wrong entry';; CONFIRM) why='RED fdisk delete without confirmation';; esac; \
 if sh harness/diff/fat_diff/fdisk.sh $(BUILD) $(BUILD)/test_fdisk_$$m > $(BUILD)/fdisk-mut-$$m.log 2>&1; then echo "FAIL fdisk $$m survived"; exit 1; fi; \
 grep -Fx "$$why" $(BUILD)/fdisk-mut-$$m.log || { cat $(BUILD)/fdisk-mut-$$m.log; exit 1; }; done
@echo 'GREEN test-fdisk-mutant: CHS SIGNATURE ACTIVE CONFIRM rejected'
```

## test-find-filter

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31651):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/find_program.bin harness/diff/fat_diff/fixtures/autoexec_find.bat harness/diff/fat_diff/fixtures/find_in.txt harness/diff/fat_diff/fixtures/find_expected.txt harness/diff/fat_diff/fixtures/find_expected_v.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-find-filter : the FIND filter (initech-m0dc)\n'
@printf '  AUTOEXEC.BAT: TYPE FIN.TXT | FIND "needle" ; ... | FIND /V "needle".\n'
@printf '  DIFFs the matched / inverted line sets on serial against independent goldens.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-find-filter FAIL: mtools `mformat` not found (apt install mtools).\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-find-filter FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call find-filter-mint-disk,$(FIND_IMG),$(FIND_PROG_BIN))
@$(call fixture-preflight,$(FIND_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FIND_IMG)" \
	--name "$(FIND_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(FIND_REPORT)" || true
@cat "$(FIND_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(FIND_REPORT)"; then \
	printf '!!! test-find-filter FAIL: TRIPLE FAULT -- FIND crashed (root-cause tail parse / substring scan, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-find-filter [1/5]: no triple-fault\n'
@if [ ! -s "$(FIND_SERIAL)" ]; then printf '!!! test-find-filter FAIL: no serial captured\n'; exit 1; fi
@grep -q '^SHELL-READY$$' "$(FIND_SERIAL)" || { printf '!!! test-find-filter FAIL: SHELL-READY missing\n'; exit 1; }
@printf '>>> test-find-filter [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(FIND_SERIAL)" | tr -d '\r' > "$(BUILD)/$(FIND_FNAME).repl"
@awk '/^FIND-BEGIN$$/{f=1;next} /^FIND-END$$/{f=0} f' "$(BUILD)/$(FIND_FNAME).repl" > "$(BUILD)/$(FIND_FNAME).got"
@awk '/^FINDV-BEGIN$$/{f=1;next} /^FINDV-END$$/{f=0} f' "$(BUILD)/$(FIND_FNAME).repl" > "$(BUILD)/$(FIND_FNAME).got_v"
@diff "$(FIND_GOLD)" "$(BUILD)/$(FIND_FNAME).got" \
	|| { printf '!!! test-find-filter FAIL: matched-line set != golden (case-sensitivity wrong, or the quoted string / pipe stdin not read; root-cause line_contains / the cmd-tail parse, Rule 3)\n'; exit 1; }
@printf '>>> test-find-filter [3/5]: `FIND "needle"` matched exactly the golden lines (case-sensitive; uppercase NEEDLE excluded)\n'
@diff "$(FIND_GOLD_V)" "$(BUILD)/$(FIND_FNAME).got_v" \
	|| { printf '!!! test-find-filter FAIL: /V (invert) set != golden (the /V switch was not honored; root-cause the F_V flag path, Rule 3)\n'; exit 1; }
@printf '>>> test-find-filter [4/5]: `FIND /V "needle"` emitted exactly the complementary lines\n'
@grep -q '^SHELL-EXIT$$' "$(FIND_SERIAL)" || { printf '!!! test-find-filter FAIL: SHELL-EXIT missing\n'; exit 1; }
@printf '>>> test-find-filter [5/5]: post-filter `exit` reached the REPL + halted cleanly\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- FIND really filters piped stdin by a case-sensitive literal on the 386 (QEMU only)\n'
@printf '======================================================================\n'
```

## test-find-filter-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31690):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/find_program_mut.bin harness/diff/fat_diff/fixtures/autoexec_find.bat harness/diff/fat_diff/fixtures/find_in.txt harness/diff/fat_diff/fixtures/find_expected.txt build/.makefile.stamp`
```make
@printf '>>> test-find-filter-mutant: confirming the INVERTED-match mutant goes RED (Rule 6)\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-find-filter-mutant FAIL: mtools not found\n'; exit 1; }
$(call find-filter-mint-disk,$(FIND_MUT_IMG),$(FIND_PROG_MUT_BIN))
@$(call fixture-preflight,$(FIND_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FIND_MUT_IMG)" \
	--name "$(FIND_MUT_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(FIND_MUT_REPORT)" || true
@if [ ! -s "$(FIND_MUT_SERIAL)" ]; then printf '!!! test-find-filter-mutant FAIL: no serial from the mutant boot\n'; exit 1; fi
@sed -n '/^SHELL-READY$$/,$$p' "$(FIND_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(FIND_MUT_FNAME).repl"
@awk '/^FIND-BEGIN$$/{f=1;next} /^FIND-END$$/{f=0} f' "$(BUILD)/$(FIND_MUT_FNAME).repl" > "$(BUILD)/$(FIND_MUT_FNAME).got"
@if diff -q "$(FIND_GOLD)" "$(BUILD)/$(FIND_MUT_FNAME).got" >/dev/null 2>&1; then \
	printf '!!! test-find-filter-mutant FAIL: matched set MATCHED the golden under FIND_MUTATE_INVERT -- the gate would pass even with the match sense flipped; it is decoration\n'; exit 1; \
fi
@printf '>>> test-find-filter-mutant: green (inverted-match mutant correctly RED -- matched set != golden, the match sense is load-bearing)\n'
```

## test-flair-shell

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15776):
Prerequisites: `build/test_shell build/.makefile.stamp`
```make
@printf ">>> test-flair-shell: M4 capstone -- composed desktop reproduces the Office Space frame (seafoam + 2 stacked menu bars + windows + modal FILE COPY on top)\n"
@$(TEST_SHELL) $(BUILD)/desktop_scene.ppm
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_SHELL_OPT) $(SHELL_INC) -c $(SHELL_C) -o $(BUILD)/shell_freestanding.o \
	|| { printf '!!! test-flair-shell FAIL: shell.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-flair-shell: green (wrote $(BUILD)/desktop_scene.ppm)\n"
```

## test-flair-shell-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15783):
Prerequisites: `build/test_shell_mutant_onemenubar build/test_shell_mutant_nomodal build/test_shell_mutant_modalbehind build/test_shell_mutant_ignoreoverlay build/.makefile.stamp`
```make
@printf ">>> test-flair-shell-mutant: confirming all four mutants go RED (Rule 6)\n"
@if $(TEST_SHELL_MUT_ONEBAR)  >/dev/null 2>&1; then printf '!!! test-flair-shell-mutant FAIL: ONE_MENUBAR PASSED -- the two-bar oracle is decoration\n'; exit 1; else printf '>>> test-flair-shell-mutant: green (ONE_MENUBAR correctly RED)\n'; fi
@if $(TEST_SHELL_MUT_NOMODAL) >/dev/null 2>&1; then printf '!!! test-flair-shell-mutant FAIL: NO_MODAL PASSED -- the modal-present oracle is decoration\n'; exit 1; else printf '>>> test-flair-shell-mutant: green (NO_MODAL correctly RED)\n'; fi
@if $(TEST_SHELL_MUT_BEHIND)  >/dev/null 2>&1; then printf '!!! test-flair-shell-mutant FAIL: MODAL_BEHIND PASSED -- the z-order oracle is decoration\n'; exit 1; else printf '>>> test-flair-shell-mutant: green (MODAL_BEHIND correctly RED)\n'; fi
@if $(TEST_SHELL_MUT_OVERLAY) >/dev/null 2>&1; then printf '!!! test-flair-shell-mutant FAIL: IGNORE_OVERLAY PASSED -- the forward window-across-modal overlay proof is decoration\n'; exit 1; else printf '>>> test-flair-shell-mutant: green (IGNORE_OVERLAY correctly RED -- original pipa mechanism retained after dc4v reverse re-key)\n'; fi
```

## test-format-missing-source-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 58):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/transient_flairtenants.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN) MISSING
```

## test-format-system-boot

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 60):
Prerequisites: `test-formatsys-emu build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN) boot
```

## test-format-system-boot-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 219):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-format-system-boot-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-format-system-boot-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/format-system-boot_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/format-system-boot_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "format-system-boot_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/format-system-boot_bochs.report.txt" || true
@cat "$(BUILD)/format-system-boot_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-format-system-boot-bochs "$(BUILD)/format-system-boot_bochs.report.txt" "$(BUILD)/format-system-boot_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-formatsys

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 12):
Prerequisites: `build/formatsys_host build/formatsys_check build/tracer_secondary.img build/transient_flairtenants.bin build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/formatsys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/formatsys_host
```

## test-formatsys-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 49):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/formatsys_check build/transient_flairtenants.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN)
```

## test-formatsys-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 219):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-formatsys-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-formatsys-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/formatsys-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/formatsys-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "formatsys-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/formatsys-emu_bochs.report.txt" || true
@cat "$(BUILD)/formatsys-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-formatsys-emu-bochs "$(BUILD)/formatsys-emu_bochs.report.txt" "$(BUILD)/formatsys-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-formatsys-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 51):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/formatsys_check build/transient_flairtenants.bin build/format_MEDIA.bin build/format_FAT0.bin build/format_COMMAND.bin build/format_ORDER.bin build/format_RUNNING.bin build/format_MISSING.bin build/.makefile.stamp`
```make
@set -eu; for m in MEDIA FAT0 COMMAND ORDER RUNNING MISSING; do \
 case $$m in MEDIA) why='RED formatsys BPB media byte';; FAT0) why='RED formatsys FAT[0] media/EOC';; COMMAND) why='RED formatsys /S COMMAND.COM';; ORDER) why='RED formatsys root entry order';; RUNNING) why='RED formatsys-emu running volume not refused';; MISSING) why='RED formatsys-emu missing source message';; esac; \
 if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/format_$$m.bin $$m > $(BUILD)/formatsys-emu-mut-$$m.log 2>&1; then echo "FAIL emu $$m survived"; exit 1; fi; \
 grep -Fx "$$why" $(BUILD)/formatsys-emu-mut-$$m.log || { cat $(BUILD)/formatsys-emu-mut-$$m.log; exit 1; }; done
@echo 'GREEN test-formatsys-emu-mutant: MEDIA FAT0 COMMAND ORDER RUNNING MISSING rejected'
```

## test-formatsys-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 26):
Prerequisites: `build/formatsys_check build/tracer_secondary.img build/transient_flairtenants.bin build/formatsys_host_MEDIA build/formatsys_host_FAT0 build/formatsys_host_COMMAND build/formatsys_host_ORDER build/formatsys_host_COMPARE build/formatsys_host_RUNNING build/formatsys_host_RESERVE build/formatsys_host_LABEL build/formatsys_host_MISSING build/.makefile.stamp`
```make
@set -eu; for m in $(DU_MUTANTS); do \
 case $$m in MEDIA) why='RED formatsys BPB media byte';; FAT0) why='RED formatsys FAT[0] media/EOC';; COMMAND) why='RED formatsys /S COMMAND.COM';; ORDER) why='RED formatsys root entry order';; COMPARE) why='RED formatsys DISKCOMP missing differing sector';; RUNNING) why='RED formatsys running volume not refused';; RESERVE) why='RED formatsys /B reserve bytes';; LABEL) why='RED formatsys /V repeat label fsck';; MISSING) why='RED formatsys missing source message';; esac; \
 if sh harness/diff/fat_diff/formatsys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(BUILD)/formatsys_host_$$m > $(BUILD)/formatsys-mut-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 grep -Fx "$$why" $(BUILD)/formatsys-mut-$$m.log || { cat $(BUILD)/formatsys-mut-$$m.log; exit 1; }; done
@echo 'GREEN test-formatsys-mutant: MEDIA FAT0 COMMAND ORDER COMPARE RUNNING RESERVE LABEL MISSING rejected'
```

## test-formatsys-syswrite-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 65):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/transient_flairtenants.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FORMAT_PROG_BIN) syswrite
```

## test-formatsys-syswrite-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 72):
Prerequisites: `build/qemu_harness build/j1lu_syswrite_watch.img build/console_text build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/transient_flairtenants.bin build/.makefile.stamp`
```make
@if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/formatsys_emu.sh $(BUILD) $(BUILD)/j1lu_syswrite_watch.img $(FORMAT_PROG_BIN) syswrite > $(BUILD)/formatsys-syswrite-mut.log 2>&1; then echo 'FAIL formatsys syswrite WATCH survived'; exit 1; fi
@grep -Fx 'RED formatsys-emu syswrite filesystem' $(BUILD)/formatsys-syswrite-mut.log || { cat $(BUILD)/formatsys-syswrite-mut.log; exit 1; }
@echo 'GREEN test-formatsys-syswrite-emu-mutant: WATCH (stale mounted D: FAT after SYS) rejected by fsck.fat'
```

## test-hsct-redir

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30964):
Prerequisites: `build/qemu_harness build/tracer_secondary.img harness/diff/fat_diff/fixtures/autoexec_hsct.bat build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-hsct-redir : `>` / `>>` OUTPUT redirect\n'
@printf '  beads initech-hsct. AUTOEXEC.BAT: ECHO RZZHELLO> OUT.TXT ; TYPE OUT.TXT\n'
@printf '  ; ECHO RZZWORLD>> OUT.TXT ; TYPE OUT.TXT.  Proves echo>file redirects\n'
@printf '  (HELLO to the FILE, not the screen) and >> appends. Prereqs k36g+o0td.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-hsct-redir FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-hsct-redir FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call hsct-redir-mint-disk,$(HSCT_REDIR_IMG))
@printf 'Booting   : %s + data disk %s (AUTOEXEC.BAT redirect script)\n' "$(SECONDARY_TRACER_IMG)" "$(HSCT_REDIR_IMG)"
@printf 'Expecting : RZZHELLO twice (2 TYPEs), RZZWORLD once, then clean EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(HSCT_REDIR_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(HSCT_REDIR_IMG)" \
	--name "$(HSCT_REDIR_NAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(HSCT_REDIR_REPORT)" || true
@cat "$(HSCT_REDIR_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(HSCT_REDIR_REPORT)"; then \
	printf '!!! test-hsct-redir FAIL: TRIPLE FAULT -- redirect execution crashed\n'; exit 1; \
fi
@printf '>>> test-hsct-redir [1/5]: no triple-fault\n'
@if [ ! -s "$(HSCT_REDIR_SERIAL)" ]; then \
	printf '!!! test-hsct-redir FAIL: no serial captured at %s\n' "$(HSCT_REDIR_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(HSCT_REDIR_SERIAL)" \
	|| { printf '!!! test-hsct-redir FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-hsct-redir [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(HSCT_REDIR_SERIAL)" | tr -d '\r' > "$(BUILD)/$(HSCT_REDIR_NAME).repl"
@hcnt=$$(grep -oF 'RZZHELLO' "$(BUILD)/$(HSCT_REDIR_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$hcnt" != "2" ]; then \
	printf '!!! test-hsct-redir FAIL: RZZHELLO seen %s time(s), expected EXACTLY 2 (one per TYPE).  count==1 means `>` leaked to screen / never wrote OUT.TXT; the `>` redirect did NOT work (root-cause the driver/DUP2/k36g path, Rule 3)\n' "$$hcnt"; \
	exit 1; \
fi
@printf '>>> test-hsct-redir [3/5]: RZZHELLO appears EXACTLY twice -- `echo RZZHELLO > OUT.TXT` REDIRECTED to the file (both TYPEs read it back; the ECHO did NOT leak to screen)\n'
@wcnt=$$(grep -oF 'RZZWORLD' "$(BUILD)/$(HSCT_REDIR_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$wcnt" != "1" ]; then \
	printf '!!! test-hsct-redir FAIL: RZZWORLD seen %s time(s), expected EXACTLY 1 (only the 2nd TYPE).  The `>>` append did not land in OUT.TXT (root-cause the open-existing+LSEEK-to-end path, Rule 3)\n' "$$wcnt"; \
	exit 1; \
fi
@printf '>>> test-hsct-redir [4/5]: RZZWORLD appears exactly once -- `echo RZZWORLD >> OUT.TXT` APPENDED (only the 2nd TYPE shows it)\n'
@grep -q '^SHELL-EXIT$$' "$(HSCT_REDIR_SERIAL)" \
	|| { printf '!!! test-hsct-redir FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run (handle 1 may not have been restored to CON)\n'; exit 1; }
@printf '>>> test-hsct-redir [5/5]: post-redirect `exit` reached the REPL + halted cleanly (handle 1 restored to CON)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- `echo > file` and `>>` append really redirect on the 386\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-hsct-redir-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31052):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_redirbypass.img harness/diff/fat_diff/fixtures/autoexec_hsct.bat build/.makefile.stamp`
```make
@printf '>>> test-hsct-redir-mutant: confirming the REDIR-BYPASS mutant goes RED (Rule 6)\n'
$(call hsct-redir-mint-disk,$(HSCT_REDIR_MUT_IMG))
@$(call fixture-preflight,$(HSCT_REDIR_MUT_IMG))
@$(HARNESS_BIN) --disk "$(HSCT_REDIR_TRACER_MUT_IMG)" --disk2 "$(HSCT_REDIR_MUT_IMG)" \
	--name "$(HSCT_REDIR_MUT_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(HSCT_REDIR_MUT_REPORT)" || true
@if [ ! -s "$(HSCT_REDIR_MUT_SERIAL)" ]; then \
	printf '!!! test-hsct-redir-mutant FAIL: no serial from the mutant boot\n'; exit 1; \
fi
@sed -n '/^SHELL-READY$$/,$$p' "$(HSCT_REDIR_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(HSCT_REDIR_MUT_NAME).repl"
@hcnt=$$(grep -oF 'RZZHELLO' "$(BUILD)/$(HSCT_REDIR_MUT_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$hcnt" = "2" ]; then \
	printf '!!! test-hsct-redir-mutant FAIL: RZZHELLO seen 2x under CMD_MUTATE_REDIR_BYPASS -- the redirect would pass even with the driver dead; the emu gate is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-hsct-redir-mutant: green (redir-bypass mutant correctly RED -- RZZHELLO count != 2, the redirect driver is load-bearing)\n'
```

## test-more-filter

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31741):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/more_program.bin harness/diff/fat_diff/fixtures/autoexec_more.bat harness/diff/fat_diff/fixtures/more_in.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-more-filter : the MORE filter (initech-m0dc)\n'
@printf '  AUTOEXEC.BAT: MORE < MIN.TXT > MOUT.TXT  (stdin + stdout both redirected).\n'
@printf '  stdout is a FILE (AH=44h AL=00 bit15 clear) -> MORE passes through verbatim;\n'
@printf '  MOUT.TXT (read back from the image) must equal MIN.TXT byte-for-byte.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-more-filter FAIL: mtools `mformat` not found (apt install mtools).\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-more-filter FAIL: mtools `mcopy` not found.\n'; exit 1; }
@command -v mtype   >/dev/null 2>&1 || { printf '!!! test-more-filter FAIL: mtools `mtype` not found.\n'; exit 1; }
$(call more-filter-mint-disk,$(MORE_IMG),$(MORE_PROG_BIN))
@$(call fixture-preflight,$(MORE_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(MORE_IMG)" \
	--name "$(MORE_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(MORE_REPORT)" || true
@cat "$(MORE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(MORE_REPORT)"; then \
	printf '!!! test-more-filter FAIL: TRIPLE FAULT -- MORE crashed (root-cause the IOCTL/passthrough path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-more-filter [1/4]: no triple-fault\n'
@if [ ! -s "$(MORE_SERIAL)" ]; then printf '!!! test-more-filter FAIL: no serial captured\n'; exit 1; fi
@grep -q '^SHELL-READY$$' "$(MORE_SERIAL)" || { printf '!!! test-more-filter FAIL: SHELL-READY missing\n'; exit 1; }
@printf '>>> test-more-filter [2/4]: SHELL-READY (COMMAND.COM REPL entered)\n'
@mtype -i "$(MORE_IMG)" ::MOUT.TXT 2>/dev/null | tr -d '\r' > "$(BUILD)/$(MORE_FNAME).got" \
	|| { printf '!!! test-more-filter FAIL: MOUT.TXT not present on the image -- MORE never wrote its stdout (root-cause the passthrough / `>` redirect, Rule 3)\n'; exit 1; }
@diff "$(MORE_IN)" "$(BUILD)/$(MORE_FNAME).got" \
	|| { printf '!!! test-more-filter FAIL: MOUT.TXT != MIN.TXT (MORE dropped/garbled bytes, or did not detect the file stdout; root-cause the AH=44h device fork / passthrough copy, Rule 3)\n'; exit 1; }
@printf '>>> test-more-filter [3/4]: `MORE < MIN.TXT > MOUT.TXT` passed 30 lines through verbatim (file stdout -> no pager keywait)\n'
@grep -q '^SHELL-EXIT$$' "$(MORE_SERIAL)" || { printf '!!! test-more-filter FAIL: SHELL-EXIT missing -- MORE may have paginated a file stdout and blocked on a keywait past line 24 (device fork broken)\n'; exit 1; }
@printf '>>> test-more-filter [4/4]: post-filter `exit` reached the REPL + halted cleanly (no pager keywait -> file stdout was detected)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- MORE detects a file stdout and passes input through on the 386 (QEMU only)\n'
@printf '======================================================================\n'
```

## test-more-filter-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31778):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/more_program_mut.bin harness/diff/fat_diff/fixtures/autoexec_more.bat harness/diff/fat_diff/fixtures/more_in.txt build/.makefile.stamp`
```make
@printf '>>> test-more-filter-mutant: confirming the LOSSY-passthrough mutant goes RED (Rule 6)\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-more-filter-mutant FAIL: mtools not found\n'; exit 1; }
$(call more-filter-mint-disk,$(MORE_MUT_IMG),$(MORE_PROG_MUT_BIN))
@$(call fixture-preflight,$(MORE_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(MORE_MUT_IMG)" \
	--name "$(MORE_MUT_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(MORE_MUT_REPORT)" || true
@if [ ! -s "$(MORE_MUT_SERIAL)" ]; then printf '!!! test-more-filter-mutant FAIL: no serial from the mutant boot\n'; exit 1; fi
@mtype -i "$(MORE_MUT_IMG)" ::MOUT.TXT 2>/dev/null | tr -d '\r' > "$(BUILD)/$(MORE_MUT_FNAME).got" || true
@if diff -q "$(MORE_IN)" "$(BUILD)/$(MORE_MUT_FNAME).got" >/dev/null 2>&1; then \
	printf '!!! test-more-filter-mutant FAIL: MOUT.TXT MATCHED the input under MORE_MUTATE_DROP -- the gate would pass even with lossy passthrough; it is decoration\n'; exit 1; \
fi
@printf '>>> test-more-filter-mutant: green (lossy-passthrough mutant correctly RED -- round-trip != input, the full byte forward is load-bearing)\n'
```

## test-ovl-format

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 42):
Prerequisites: `build/ovl_reader build/flairovl_kernel_flairtenants.ovl build/flairovl_kernel_flairtenants.sys build/flair_tenants.img build/INITDOS.SYS build/.makefile.stamp | build/ovl-gates`
```make
@$(call ovl-read,$(OVL_READER),$(OVL_FLAG).ovl,kernel_flairtenants,$(OVL_FLAG).elf)
@mcopy -i $(FLAIRTENANTS_IMG)@@32256 ::FLAIR.OVL - | cmp - $(OVL_FLAG).ovl
@mcopy -i $(FLAIRTENANTS_IMG)@@32256 ::INITDOS.SYS - | cmp - $(OVL_FLAG).sys
@cmp $(INITDOS_SYS_BIN) $(OVL_FLAG).sys
@mattrib -i $(FLAIRTENANTS_IMG)@@32256 ::FLAIR.OVL | grep -qx '  A          ::/FLAIR.OVL' || { printf '!!! test-ovl-format FAIL: C:\\FLAIR.OVL is not archive-only (DEC-14-4)\n'; exit 1; }
@mdir -i $(FLAIRTENANTS_IMG)@@32256 :: | grep -Eq '^FLAIR +OVL +[0-9]+ 1980-01-01 +0:00' || { printf '!!! test-ovl-format FAIL: C:\\FLAIR.OVL directory date is not 1980-01-01 00:00 (DEC-14-4, Rule 11)\n'; exit 1; }
@printf '>>> test-ovl-format: green (independent reader; C:\\FLAIR.OVL and INITDOS.SYS on the image == the packed pair; archive-only, 1980-01-01 00:00)\n'
```

## test-ovl-format-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 53):
Prerequisites: `test-ovl-format build/ovl_reader_mut_nocrc build/.makefile.stamp`
```make
@cp -f $(OVL_FLAG).ovl $(OVL_GATE_DIR)/crcflip.ovl
@set -eu; len=$$(od -An -tu4 -j28 -N4 $(OVL_FLAG).ovl | tr -d ' '); \
 b=$$(od -An -tu1 -j$$(( len - 1 )) -N1 $(OVL_FLAG).ovl | tr -d ' '); \
 printf "$$(printf '\\%03o' $$(( b ^ 1 )))" | dd of=$(OVL_GATE_DIR)/crcflip.ovl bs=1 seek=$$(( len - 1 )) conv=notrunc status=none
@if ( $(call ovl-read,$(OVL_READER),$(OVL_GATE_DIR)/crcflip.ovl,kernel_flairtenants,$(OVL_FLAG).elf) ) > $(OVL_GATE_DIR)/crcflip.log 2>&1; then \
	printf '!!! test-ovl-format-mutant FAIL: a payload flip PASSED the reader\n'; exit 1; fi
@grep -x 'RED ovl-reader: crc32 field != CRC-32 of every other byte' $(OVL_GATE_DIR)/crcflip.log
@$(call ovl-read,$(OVL_READER_MUT),$(OVL_GATE_DIR)/crcflip.ovl,kernel_flairtenants,$(OVL_FLAG).elf) > /dev/null
@printf '>>> test-ovl-format-mutant: green (payload flip RED on the CRC property; the no-CRC reader accepts it)\n'
```

## test-redir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5806):
Prerequisites: `build/test_redir build/.makefile.stamp`
```make
@printf ">>> test-redir: AH=02h/09h/06h output via redirectable STDOUT handle 1 (default CON / DUP2(file,1) / no-PSP fallback) -- beads initech-k36g\n"
@$(TEST_REDIR)
@printf ">>> test-redir: green\n"
```

## test-redir-full-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 120):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/sort_program.bin harness/diff/fat_diff/redir_full.sh build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/redir_full.sh $(SECONDARY_TRACER_IMG) $(SORT_PROG_BIN)
```

## test-redir-full-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 122):
Prerequisites: `build/qemu_harness build/sort_program.bin harness/diff/fat_diff/redir_full.sh build/tracer_mut_safety_redirfull_clobber.img build/tracer_mut_safety_redirfull_badcreate.img build/tracer_mut_safety_redirfull_ignorefull.img build/tracer_mut_safety_redirfull_pipefull.img build/tracer_mut_safety_redirfull_copyfull.img build/.makefile.stamp`
```make
@set -eu; for m in $(REDIR_FULL_MUTANTS); do \
 tag=$${m%%:*}; rest=$${m#*:}; macro=$${rest%%:*}; finding=$${rest#*:}; \
 if sh $(SHELL_FILES_DIR)/redir_full.sh $(BUILD)/tracer_mut_safety_redirfull_$$tag.img $(SORT_PROG_BIN) $$tag >$(BUILD)/redir_full_mut_$$tag.log 2>&1; then echo "FAIL $$macro survived QEMU";exit 1;fi; \
 grep "^RED redir-full: $$finding " $(BUILD)/redir_full_mut_$$tag.log; \
 echo "GREEN test-redir-full-emu-mutant: $$macro booted and caught RED ($$finding)";done
```

## test-redir-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5814):
Prerequisites: `build/test_redir_mutant_noredir build/.makefile.stamp`
```make
@printf ">>> test-redir-mutant: confirming the no-redirect mutant goes RED (Rule 6)\n"
@if $(TEST_REDIR_MUT_NOREDIR) >/dev/null 2>&1; then \
	printf '!!! test-redir-mutant FAIL: no-redirect mutant PASSED -- the DUP2(file,1) redirect oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-redir-mutant: green (no-redirect mutant correctly RED -- the redirect oracle bites)\n'; \
fi
```

## test-redir-parse

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7385):
Prerequisites: `build/test_redir_parse build/.makefile.stamp`
```make
@printf ">>> test-redir-parse: cmd_redir_parse -- '>' truncate / '>>' append / target+clean extraction / no-redir passthrough / last-wins / bound guards (beads initech-hsct)\n"
@$(TEST_REDIR_PARSE)
@printf ">>> test-redir-parse: green\n"
```

## test-redir-parse-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7391):
Prerequisites: `build/test_redir_parse_mutant_noappend build/test_redir_parse_mutant_keeptarget build/test_redir_parse_mutant_nolt build/test_redir_parse_mutant_nopipe build/.makefile.stamp`
```make
@printf ">>> test-redir-parse-mutant: confirming all mutants go RED (Rule 6)\n"
@if $(TEST_REDIR_PARSE_MUT_NOAPP) >/dev/null 2>&1; then \
	printf '!!! test-redir-parse-mutant FAIL: no-append mutant PASSED -- the >>-append parse test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-redir-parse-mutant: green (no-append mutant correctly RED -- the >>-append parse bites)\n'; \
fi
@if $(TEST_REDIR_PARSE_MUT_KEEP) >/dev/null 2>&1; then \
	printf '!!! test-redir-parse-mutant FAIL: keep-target mutant PASSED -- the clean-command parse test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-redir-parse-mutant: green (keep-target mutant correctly RED -- the clean-command parse bites)\n'; \
fi
@if $(TEST_REDIR_PARSE_MUT_NOLT) >/dev/null 2>&1; then \
	printf '!!! test-redir-parse-mutant FAIL: no-LT mutant PASSED -- the `<` input-redirect parse test is decoration (bsy.7)\n'; \
	exit 1; \
else \
	printf '>>> test-redir-parse-mutant: green (no-LT mutant correctly RED -- the `<` input-redirect parse bites; bsy.7)\n'; \
fi
@if $(TEST_REDIR_PARSE_MUT_NOPIPE) >/dev/null 2>&1; then \
	printf '!!! test-redir-parse-mutant FAIL: no-split mutant PASSED -- the multi-stage `|` pipe parse test is decoration (bsy.8)\n'; \
	exit 1; \
else \
	printf '>>> test-redir-parse-mutant: green (no-split mutant correctly RED -- the `|` pipe-split parse bites; bsy.8)\n'; \
fi
```

## test-shell

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26713):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_exec.img build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-shell : BOOT -> COMMAND.COM -> DIR/TYPE/run\n'
@printf '  Ref: ADR-0003 DEC-11/DEC-12; DOS 3.3 COMMAND.COM; spec/dos_messages.json.\n'
@printf '  beads initech-7pc (M2 capstone). Inject: dir / type hello.txt / greet / badcmd / exit.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + disk %s (HELLO.TXT + GREET.COM, primary slave)\n' "$(SECONDARY_TRACER_IMG)" "$(FAT_EXEC_IMG)"
@printf 'Expecting : SHELL-READY + DIR{HELLO.TXT,GREET.COM} + "%s" + "%s" + "%s" + SHELL-EXIT\n' "$(SHELL_TYPE_CONTENT)" "$(SHELL_EXEC_OUTPUT)" "$(SHELL_BADCMD)"
@printf '%s\n' '----------------------------------------------------------------------'
@# Run 1 (serial): inject the command sequence so the DIR/TYPE/greet/badcmd/EXIT
@# assertions below bite the REPL transcript on serial. No screendump here: in a
@# single QMP session the harness injects --keys FIRST and only then screendumps
@# (harness/emu/qemu.c qmp_session steps 3->4), so a grab after key injection
@# would catch the FULL transcript scrolled down past y=240 -- which trips
@# ppm_text_check's [C] "seafoam below the banner" guard. The visual half gets
@# its OWN clean boot (Run 2, below).
@$(call fixture-preflight,$(FAT_EXEC_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(SHELL_NAME)" --out "$(BUILD)" --timeout-ms 12000 \
	--keys "d,i,r,ret,t,y,p,e,spc,h,e,l,l,o,dot,t,x,t,ret,g,r,e,e,t,ret,b,a,d,c,m,d,ret,e,x,i,t,ret" \
	--keys-after "SHELL-READY" \
	--quit-after "SHELL-DONE" \
	2> "$(SHELL_REPORT)" || true
@cat "$(SHELL_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SHELL_REPORT)"; then \
	printf '!!! test-shell FAIL: TRIPLE FAULT -- the shell boot or a command crashed\n'; \
	exit 1; \
fi
@printf '>>> test-shell [1/7]: no triple-fault\n'
@if [ ! -s "$(SHELL_SERIAL)" ]; then \
	printf '!!! test-shell FAIL: no serial captured at %s\n' "$(SHELL_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(SHELL_SERIAL)" \
	|| { printf '!!! test-shell FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-shell [2/7]: SHELL-READY (COMMAND.COM REPL entered after CONIN-LIVE)\n'
@# Extract ONLY the post-SHELL-READY region (the REPL's own output) into a
@# separate file. The boot's earlier baked demos (proto-DIR + the baked
@# TYPE/DIR programs) also print these filenames + HELLO.TXT contents BEFORE
@# the prompt; asserting on the whole serial would let the gate pass on the
@# demo output instead of the shell's. Scoping to after SHELL-READY makes the
@# DIR/TYPE/greet assertions bite the REPL specifically (Law 2 / Rule 6).
@sed -n '/^SHELL-READY$$/,$$p' "$(SHELL_SERIAL)" > "$(BUILD)/$(SHELL_NAME).repl"
@# ---- DIR listed the two files (FINDFIRST/FINDNEXT into the DTA). ----
@grep -qF 'HELLO    TXT' "$(BUILD)/$(SHELL_NAME).repl" \
	|| { printf '!!! test-shell FAIL: DIR did not list HELLO.TXT (root-cause the DIR built-in / FINDFIRST, Rule 3)\n'; exit 1; }
@grep -qF 'GREET    COM' "$(BUILD)/$(SHELL_NAME).repl" \
	|| { printf '!!! test-shell FAIL: DIR did not list GREET.COM\n'; exit 1; }
@printf '>>> test-shell [3/7]: DIR listed HELLO.TXT + GREET.COM (FINDFIRST/FINDNEXT)\n'
@# ---- TYPE printed HELLO.TXT's contents (in the REPL region). ----
@grep -qF '$(SHELL_TYPE_CONTENT)' "$(BUILD)/$(SHELL_NAME).repl" \
	|| { printf '!!! test-shell FAIL: TYPE did not print HELLO.TXT contents "%s"\n' "$(SHELL_TYPE_CONTENT)"; exit 1; }
@printf '>>> test-shell [4/7]: TYPE printed HELLO.TXT contents (OPEN/READ/WRITE/CLOSE)\n'
@# ---- `greet` ran GREET.COM via AH=4Bh EXEC (in the REPL region). ----
@grep -qF '$(SHELL_EXEC_OUTPUT)' "$(BUILD)/$(SHELL_NAME).repl" \
	|| { printf '!!! test-shell FAIL: `greet` did not run GREET.COM ("%s" missing -- root-cause external EXEC, Rule 3)\n' "$(SHELL_EXEC_OUTPUT)"; exit 1; }
@printf '>>> test-shell [5/7]: external command `greet` ran GREET.COM via AH=4Bh EXEC\n'
@# ---- `badcmd` printed the controlled diagnostic + EXIT halted cleanly. ----
@grep -qF '$(SHELL_BADCMD)' "$(BUILD)/$(SHELL_NAME).repl" \
	|| { printf '!!! test-shell FAIL: `badcmd` did not print "%s" (the controlled MSG-DOS-0002)\n' "$(SHELL_BADCMD)"; exit 1; }
@grep -q '^SHELL-EXIT$$' "$(SHELL_SERIAL)" \
	|| { printf '!!! test-shell FAIL: SHELL-EXIT missing -- EXIT did not leave the REPL cleanly\n'; exit 1; }
@grep -q '^SHELL-DONE$$' "$(SHELL_SERIAL)" \
	|| { printf '!!! test-shell FAIL: SHELL-DONE missing -- the REPL did not return to the halt loop\n'; exit 1; }
@printf '>>> test-shell [6/7]: `badcmd` -> "%s"; EXIT halted cleanly (SHELL-EXIT + SHELL-DONE)\n' "$(SHELL_BADCMD)"
@# ---- 7. SCREENDUMP (Run 2): the "A:\>" prompt actually RENDERED on the
@# framebuffer -- the visual half of the shell (Law 4). This is a SEPARATE,
@# clean secondary processor with NO --keys. qskz deliberately moves the
@# mount listing to serial-only debug, preserving test-dir/test-dos* markers.
@# Banner rows 0..1 are followed by the exact prompt on row 2 (y=32..48).
@# Primary startup prompts are graded by test-dos-startup (Guide pp.19-20).
@# --screendump-after SHELL-READY syncs the grab to the prompt's blit. Same
@# ppm_text_check pattern as test-boot (~4112) / test-fs (~4270): the 1-arg
@# checks (banner [A]/[B] + intact seafoam bg [C]) always run; the 4-arg form
@# adds the row-2 prompt-band assertion. Exact cells below also reject the
@# old console mount listing, whose header formerly occupied that row.
@$(call fixture-preflight,$(FAT_EXEC_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--screendump --screendump-after SHELL-READY \
	--name "$(SHELL_SCRN_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(SHELL_SCRN_REPORT)" || true
@if grep -q 'triple_fault=1' "$(SHELL_SCRN_REPORT)"; then \
	printf '!!! test-shell FAIL: TRIPLE FAULT on the screendump boot\n'; exit 1; \
fi
@if [ ! -s "$(SHELL_PPM)" ]; then \
	printf '!!! test-shell FAIL: no screendump captured at %s (live guest required)\n' "$(SHELL_PPM)"; exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(SHELL_PPM)" 32 48 40 \
	|| { printf '!!! test-shell FAIL: the COMMAND.COM "A:\\>" prompt did not render on the framebuffer (band [32,48))\n'; exit 1; }
@printf '2 0 A:\\>\n' > $(BUILD)/shell_prompt.cells
@$(PPM_TEXT_CHECK_BIN) --cells "$(SHELL_PPM)" $(BUILD)/shell_prompt.cells
@printf '>>> test-shell [7/7]: screendump shows the "A:\\>" COMMAND.COM prompt rendered on the seafoam desktop\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- booted InitechDOS, got an A:\\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-shell-dir-look

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 26):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/flair_data.img build/ppm_text_check build/test_dir_disk build/.makefile.stamp`
```make
@BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(SECONDARY_TRACER_IMG) $(FLAIR_DATA_IMG) shell_dir_look
@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/shell_dir_unlabelled.img none
@BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(SECONDARY_TRACER_IMG) $(BUILD)/shell_dir_unlabelled.img shell_dir_unlabelled none
```

## test-shell-dir-look-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-shell-dir-look-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-shell-dir-look-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/shell-dir-look_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/shell-dir-look_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "shell-dir-look_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/shell-dir-look_bochs.report.txt" || true
@cat "$(BUILD)/shell-dir-look_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-shell-dir-look-bochs "$(BUILD)/shell-dir-look_bochs.report.txt" "$(BUILD)/shell-dir-look_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-shell-dir-look-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 94):
Prerequisites: `build/qemu_harness build/doslook_mut_volume.img build/doslook_mut_date.img build/doslook_mut_joined.img build/doslook_mut_lower.img build/doslook_mut_home.img build/doslook_mut_cursor.img build/flair_data.img build/ppm_text_check build/test_dir_disk build/.makefile.stamp`
```make
@set -e; for tag in volume date joined lower home cursor; do for attempt in 1 2; do \
  name=doslook_mut_$$tag; log=$(BUILD)/$$name.log; \
  if BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(BUILD)/$$name.img $(FLAIR_DATA_IMG) $$name > $$log 2>&1; then echo "FAIL: $$tag live mutant passed"; exit 1; fi; \
  grep -q '^SHELL-READY$$' $(BUILD)/$$name.serial; \
  grep -qE '(DIR LOOK RED:|CELL RED|CURSOR RED)' $$log || { cat $$log; echo 'FAIL: mutant RED for wrong reason'; exit 1; }; \
  printf 'SHELL DIR LOOK MUTANT PASS: %s attempt %s booted, named LOOK assertion RED\n' $$tag $$attempt; \
done; done
```

## test-shell-files-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 12):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/shell_files.sh emu $(SECONDARY_TRACER_IMG)
```

## test-shell-files-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-shell-files-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-shell-files-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/shell-files-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/shell-files-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "shell-files-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/shell-files-emu_bochs.report.txt" || true
@cat "$(BUILD)/shell-files-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-shell-files-emu-bochs "$(BUILD)/shell-files-emu_bochs.report.txt" "$(BUILD)/shell-files-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-shell-files-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 53):
Prerequisites: `build/qemu_harness build/tracer_mut_safety_doscmds_ren.img build/tracer_mut_safety_doscmds_wild.img build/.makefile.stamp`
```make
@set -eu; for m in ren wild; do \
 if sh $(SHELL_FILES_DIR)/shell_files.sh emu $(BUILD)/tracer_mut_safety_doscmds_$$m.img $$m >$(BUILD)/shell_files_emu_$$m.log 2>&1; then echo "FAIL $$m survived QEMU";exit 1;fi; \
 grep '^RED shell-files:' $(BUILD)/shell_files_emu_$$m.log; \
 grep -q SHELL-FILES-DONE $(BUILD)/shell-files-emu-$$m/text; \
 echo "GREEN test-shell-files-emu-mutant: $$m booted and caught RED";done
```

## test-shell-files-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 10):
Prerequisites: `build/shell_files build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/shell_files.sh host $(BUILD)/shell_files
```

## test-shell-files-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 44):
Prerequisites: `build/shell_files_mut_CMD_MUTATE_REN_ENUM build/shell_files_mut_CMD_MUTATE_REN_MAP build/shell_files_mut_CMD_MUTATE_COPY_PLUS build/shell_files_mut_CMD_MUTATE_COPY_ASCII build/shell_files_mut_CMD_MUTATE_COPY_EOF build/shell_files_mut_CMD_MUTATE_COPY_SWITCH build/shell_files_mut_CMD_MUTATE_COPY_VERIFY build/shell_files_mut_CMD_MUTATE_COPY_WILD build/shell_files_mut_FAT12_MUTATE_CREATE_CHARS build/shell_files_mut_INT21_MUTATE_MKDIR_DEVICE build/.makefile.stamp`
```make
@set -eu; for m in $(SHELL_FILES_MUTANTS); do \
 if sh $(SHELL_FILES_DIR)/shell_files.sh host $(BUILD)/shell_files_mut_$$m $$m > $(BUILD)/shell_files_$$m.log 2>&1; then echo "FAIL $$m survived";exit 1;fi; \
 grep '^RED shell-files:' $(BUILD)/shell_files_$$m.log; \
 echo "GREEN test-shell-files-host-mutant: $$m caught RED";done
```

## test-shell-permanent

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30597):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_boot.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/shell_permanent.sh $(TRACER_IMG)
```

## test-shell-permanent-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30600):
Prerequisites: `build/dos_safety_fixture build/qemu_harness build/tracer_mut_safety_permanent.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/shell_permanent.sh $(BUILD)/tracer_mut_safety_permanent.img _mut > $(BUILD)/shell_permanent_mutant.log 2>&1; then echo 'FAIL: permanent EXIT mutant passed'; exit 1; fi
@grep -q 'primary processor exited' $(BUILD)/shell_permanent_mutant.log
@printf 'VERDICT: PASS -- test-shell-permanent-mutant (primary processor exited, RED)\n'
```

## test-sort-filter

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31566):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/sort_program.bin harness/diff/fat_diff/fixtures/autoexec_sort.bat harness/diff/fat_diff/fixtures/sort_in.txt harness/diff/fat_diff/fixtures/sort_expected_a.txt harness/diff/fat_diff/fixtures/sort_expected_r.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-sort-filter : the SORT filter (initech-m0dc)\n'
@printf '  AUTOEXEC.BAT: SORT < SIN.TXT (ascending) ; SORT /R < SIN.TXT (descending).\n'
@printf '  DIFFs each ECHO-bracketed block on serial against an independent golden.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-sort-filter FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-sort-filter FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call sort-filter-mint-disk,$(SORT_IMG),$(SORT_PROG_BIN))
@$(call fixture-preflight,$(SORT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SORT_IMG)" \
	--name "$(SORT_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SORT_REPORT)" || true
@cat "$(SORT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SORT_REPORT)"; then \
	printf '!!! test-sort-filter FAIL: TRIPLE FAULT -- SORT crashed (root-cause the read/split/sort path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-sort-filter [1/5]: no triple-fault\n'
@if [ ! -s "$(SORT_SERIAL)" ]; then printf '!!! test-sort-filter FAIL: no serial captured at %s\n' "$(SORT_SERIAL)"; exit 1; fi
@grep -q '^SHELL-READY$$' "$(SORT_SERIAL)" || { printf '!!! test-sort-filter FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-sort-filter [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(SORT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(SORT_FNAME).repl"
@awk '/^SORT-A-BEGIN$$/{f=1;next} /^SORT-A-END$$/{f=0} f' "$(BUILD)/$(SORT_FNAME).repl" > "$(BUILD)/$(SORT_FNAME).got_a"
@awk '/^SORT-R-BEGIN$$/{f=1;next} /^SORT-R-END$$/{f=0} f' "$(BUILD)/$(SORT_FNAME).repl" > "$(BUILD)/$(SORT_FNAME).got_r"
@diff "$(SORT_GOLD_A)" "$(BUILD)/$(SORT_FNAME).got_a" \
	|| { printf '!!! test-sort-filter FAIL: ascending output != golden (case-insensitive collation wrong, or SORT did not run / read stdin; root-cause compare_lines / the `<` stdin repoint, Rule 3)\n'; exit 1; }
@printf '>>> test-sort-filter [3/5]: `SORT < SIN.TXT` ascending == golden (case-insensitive collation correct)\n'
@diff "$(SORT_GOLD_R)" "$(BUILD)/$(SORT_FNAME).got_r" \
	|| { printf '!!! test-sort-filter FAIL: `/R` output != reverse golden (the /R switch was not honored; root-cause the tail parse + rflag, Rule 3)\n'; exit 1; }
@printf '>>> test-sort-filter [4/5]: `SORT /R < SIN.TXT` descending == golden (the /R switch reverses)\n'
@grep -q '^SHELL-EXIT$$' "$(SORT_SERIAL)" || { printf '!!! test-sort-filter FAIL: SHELL-EXIT missing -- the post-AUTOEXEC EXIT did not run (handle 0 not restored, or a panic)\n'; exit 1; }
@printf '>>> test-sort-filter [5/5]: post-filter `exit` reached the REPL + halted cleanly\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- SORT really sorts stdin (case-insensitive + /R) on the 386 (QEMU only)\n'
@printf '======================================================================\n'
```

## test-sort-filter-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31605):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/sort_program_mut.bin harness/diff/fat_diff/fixtures/autoexec_sort.bat harness/diff/fat_diff/fixtures/sort_in.txt harness/diff/fat_diff/fixtures/sort_expected_a.txt build/.makefile.stamp`
```make
@printf '>>> test-sort-filter-mutant: confirming the REVERSED-comparator mutant goes RED (Rule 6)\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-sort-filter-mutant FAIL: mtools not found\n'; exit 1; }
$(call sort-filter-mint-disk,$(SORT_MUT_IMG),$(SORT_PROG_MUT_BIN))
@$(call fixture-preflight,$(SORT_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SORT_MUT_IMG)" \
	--name "$(SORT_MUT_FNAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SORT_MUT_REPORT)" || true
@if [ ! -s "$(SORT_MUT_SERIAL)" ]; then printf '!!! test-sort-filter-mutant FAIL: no serial from the mutant boot\n'; exit 1; fi
@sed -n '/^SHELL-READY$$/,$$p' "$(SORT_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(SORT_MUT_FNAME).repl"
@awk '/^SORT-A-BEGIN$$/{f=1;next} /^SORT-A-END$$/{f=0} f' "$(BUILD)/$(SORT_MUT_FNAME).repl" > "$(BUILD)/$(SORT_MUT_FNAME).got_a"
@if diff -q "$(SORT_GOLD_A)" "$(BUILD)/$(SORT_MUT_FNAME).got_a" >/dev/null 2>&1; then \
	printf '!!! test-sort-filter-mutant FAIL: ascending output MATCHED the golden under SORT_MUTATE_REVERSE -- the gate would pass even with the comparator flipped; it is decoration\n'; exit 1; \
fi
@printf '>>> test-sort-filter-mutant: green (reversed-comparator mutant correctly RED -- ascending block != golden, the collation is load-bearing)\n'
```

## test-tbx-mutant-SHELL_MUT_MENU_COUNT

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 34):
Prerequisites: `build/tbx_mut_SHELL_MUT_MENU_COUNT build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SHELL_MUT_MENU_COUNT: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*uskw' $<.log || { printf 'FAIL SHELL_MUT_MENU_COUNT: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SHELL_MUT_MENU_COUNT: RED: '; grep -Em1 'FAIL.*uskw' $<.log
```

## test-tbx-mutant-SHELL_MUT_SHORT_SURFACE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 35):
Prerequisites: `build/tbx_mut_SHELL_MUT_SHORT_SURFACE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SHELL_MUT_SHORT_SURFACE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*mqsn' $<.log || { printf 'FAIL SHELL_MUT_SHORT_SURFACE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SHELL_MUT_SHORT_SURFACE: RED: '; grep -Em1 'FAIL.*mqsn' $<.log
```

## test-xcopy-cwd-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 21):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/xcopy_program.bin build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/xcopy_cwd.sh emu $(SECONDARY_TRACER_IMG)
```

## test-xcopy-cwd-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-xcopy-cwd-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-xcopy-cwd-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/xcopy-cwd-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/xcopy-cwd-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "xcopy-cwd-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/xcopy-cwd-emu_bochs.report.txt" || true
@cat "$(BUILD)/xcopy-cwd-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-xcopy-cwd-emu-bochs "$(BUILD)/xcopy-cwd-emu_bochs.report.txt" "$(BUILD)/xcopy-cwd-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-xcopy-cwd-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 71):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/xcopy_cwd_mut_program.bin build/.makefile.stamp`
```make
@if XCOPY_CWD_PROGRAM=$(BUILD)/xcopy_cwd_mut_program.bin sh $(SHELL_FILES_DIR)/xcopy_cwd.sh emu $(SECONDARY_TRACER_IMG) mutant >$(BUILD)/xcopy_cwd_emu_mut.log 2>&1; then echo 'FAIL DU_MUT_REL_ROOT survived QEMU';exit 1;fi
@grep '^RED xcopy-cwd: relative source selected root decoy' $(BUILD)/xcopy_cwd_emu_mut.log
@echo 'GREEN test-xcopy-cwd-emu-mutant: DU_MUT_REL_ROOT booted and caught RED'
```

## test-xcopy-cwd-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 19):
Prerequisites: `build/test_xcopy_tree build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/xcopy_cwd.sh host $(ZUVO_HOST)
```

## test-xcopy-cwd-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 67):
Prerequisites: `build/test_xcopy_cwd_mut build/.makefile.stamp`
```make
@if sh $(SHELL_FILES_DIR)/xcopy_cwd.sh host $< mutant >$(BUILD)/xcopy_cwd_host_mut.log 2>&1; then echo 'FAIL DU_MUT_REL_ROOT survived';exit 1;fi
@grep '^RED xcopy-cwd: relative source selected root decoy' $(BUILD)/xcopy_cwd_host_mut.log
@echo 'GREEN test-xcopy-cwd-host-mutant: DU_MUT_REL_ROOT caught RED'
```

## test-xcopy-tree

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/xcopy_tree.mk', line 37):
Prerequisites: `build/test_xcopy_tree build/.makefile.stamp`
```make
@sh $(ZUVO_DIR)/xcopy_tree.sh host $(ZUVO_HOST) $(BUILD)/zuvo_host
```

## test-xcopy-tree-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/xcopy_tree.mk', line 47):
Prerequisites: `build/test_xcopy_tree build/qemu_harness build/tracer_secondary.img build/xcopy_program.bin build/tree_program.bin build/.makefile.stamp`
```make
@sh $(ZUVO_DIR)/xcopy_tree.sh emu $(ZUVO_HOST) $(BUILD)/zuvo_emu $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
```

## test-xcopy-tree-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 127):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-xcopy-tree-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-xcopy-tree-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/xcopy-tree-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/xcopy-tree-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "xcopy-tree-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/xcopy-tree-emu_bochs.report.txt" || true
@cat "$(BUILD)/xcopy-tree-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-xcopy-tree-emu-bochs "$(BUILD)/xcopy-tree-emu_bochs.report.txt" "$(BUILD)/xcopy-tree-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-xcopy-tree-emu-cancel

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/xcopy_tree.mk', line 49):
Prerequisites: `build/test_xcopy_tree build/qemu_harness build/tracer_secondary.img build/xcopy_program.bin build/tree_program.bin build/.makefile.stamp`
```make
@sh $(ZUVO_DIR)/xcopy_tree.sh cancel $(ZUVO_HOST) $(BUILD)/zuvo_cancel $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
```

## test-xcopy-tree-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/xcopy_tree.mk', line 51):
Prerequisites: `build/test_xcopy_tree build/qemu_harness build/tracer_secondary.img build/xcopy_mut_program.bin build/tree_mut_program.bin build/xcopy_ctrlc_program.bin build/xcopy_program.bin build/tree_program.bin build/.makefile.stamp`
```make
@set -e; for m in xcopy tree ctrlc; do \
  x=$(XCOPY_PROG_BIN); t=$(TREE_PROG_BIN); \
  mode=emu; \
  case $$m in \
    xcopy) x=$(BUILD)/xcopy_mut_program.bin; want='directory membership';; \
    tree) t=$(BUILD)/tree_mut_program.bin; want='TREE manual layout';; \
    ctrlc) x=$(BUILD)/xcopy_ctrlc_program.bin; mode=cancel; want='/W Ctrl-C ignored by XCOPY (INT 23h abort reached the batch prompt';; \
  esac; \
  log=$(BUILD)/zuvo_emu_mut_$$m.log; work=$(BUILD)/zuvo_emu_mut_$$m; serial=$$work/zuvo_emu_mut_$$m.serial; \
  if sh $(ZUVO_DIR)/xcopy_tree.sh $$mode $(ZUVO_HOST) $$work $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $$x $$t > $$log 2>&1; then echo "FAIL: $$m live mutant survived"; exit 1; fi; \
  if [ $$m = ctrlc ]; then \
    grep -q '^SHELL-READY' $$serial; grep -q '^INT23-BREAK' $$serial; \
    grep -q 'Terminate batch job (Y/N)?' $$serial; \
    if grep -q '^ZUVO-CANCEL2' $$serial; then echo "FAIL: $$m exited 2, not the batch-prompt route"; exit 1; fi; \
  else grep -q '^ZUVO-DONE' $$serial; fi; \
  grep '^XCOPY/TREE RED: QEMU ' $$log | grep -F "$$want"; \
  echo "XCOPY/TREE EMU MUTANT GREEN: $$m booted and caught"; \
done
@echo 'XCOPY/TREE EMU MUTANTS GREEN: S TREE_LAYOUT CTRL_C booted and caught'
```

## test-xcopy-tree-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/xcopy_tree.mk', line 39):
Prerequisites: `build/test_xcopy_tree_mut_S build/test_xcopy_tree_mut_E build/test_xcopy_tree_mut_A build/test_xcopy_tree_mut_M build/test_xcopy_tree_mut_D build/test_xcopy_tree_mut_P build/test_xcopy_tree_mut_V build/test_xcopy_tree_mut_W build/test_xcopy_tree_mut_TARGET build/test_xcopy_tree_mut_COUNT build/test_xcopy_tree_mut_TREE_F build/test_xcopy_tree_mut_TREE_LAYOUT build/test_xcopy_tree_mut_DEFAULT build/test_xcopy_tree_mut_WILD build/test_xcopy_tree_mut_DESCEND build/test_xcopy_tree_mut_TIME build/test_xcopy_tree_mut_HIDDEN build/test_xcopy_tree_mut_E_REQUIRES_S build/test_xcopy_tree_mut_CYCLIC build/test_xcopy_tree_mut_SELF build/test_xcopy_tree_mut_LABEL build/test_xcopy_tree_mut_OMIT build/test_xcopy_tree_mut_CTRL_C build/test_xcopy_tree_mut_DATE_RANGE build/.makefile.stamp`
```make
@set -e; for m in $(ZUVO_MUTANTS); do \
  log=$(BUILD)/zuvo_mut_$$m.log; \
  if sh $(ZUVO_DIR)/xcopy_tree.sh host $(BUILD)/test_xcopy_tree_mut_$$m $(BUILD)/zuvo_mut_$$m > $$log 2>&1; then echo "FAIL: $$m mutant survived"; exit 1; fi; \
  grep '^XCOPY/TREE RED:' $$log; \
  echo "XCOPY/TREE MUTANT GREEN: $$m caught"; \
done
@echo 'XCOPY/TREE MUTANTS GREEN: 24 named mutants caught'
```

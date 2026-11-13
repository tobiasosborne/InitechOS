## test-calcda

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33712):
Prerequisites: `build/test_calcda test-calcda-power build/.makefile.stamp`
```make
@$< spec/calcda/calcda_vectors.tsv "$(SYSTEM7_DECOMP)"
@printf '>>> test-calcda: green (vectors vs the real Calculator, keymap, 20000-session reference property, click/key parity, readout + face pixels vs the captures)\n'
```

## test-calcda-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33715):
Prerequisites: `test-calcda build/test_calcda_mut_CALCDA_MUT_PRECEDENCE build/test_calcda_mut_CALCDA_MUT_DIV0_ZERO build/test_calcda_mut_CALCDA_MUT_ENTRY_CAP14 build/test_calcda_mut_CALCDA_MUT_FIXED_E3 build/test_calcda_mut_CALCDA_MUT_KEY_CLICK_DIVERGE build/test_calcda_mut_CALCDA_MUT_READOUT_SHIFT test-calcda-power-mutant build/.makefile.stamp`
```make
@for p in $(CALCDA_HOST_MUTANTS); do m=$${p%%:*}; leg=$${p##*:}; \
 if $(BUILD)/test_calcda_mut_$$m spec/calcda/calcda_vectors.tsv "$(SYSTEM7_DECOMP)" > $(BUILD)/test_calcda_mut_$$m.log 2>&1; then \
   printf '!!! test-calcda-mutant: %s survived -- decoration\n' "$$m"; exit 1; fi; \
 grep -q "^FAIL $$leg" $(BUILD)/test_calcda_mut_$$m.log || { cat $(BUILD)/test_calcda_mut_$$m.log; printf '!!! %s RED, but not on %s\n' "$$m" "$$leg"; exit 1; }; \
 printf '>>> test-calcda-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^FAIL $$leg" $(BUILD)/test_calcda_mut_$$m.log | cut -c1-110)"; done
```

## test-calcda-power

Declared tier: HOST; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 54):
Prerequisites: `test-calcda-route build/test_power_notice build/.makefile.stamp`
```make
@$(BUILD)/test_power_notice
```

## test-calcda-power-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 56):
Prerequisites: `test-calcda-power build/test_calcda_close_only build/test_power_notice_mutant build/.makefile.stamp`
```make
@if $(BUILD)/test_calcda_close_only > $(BUILD)/calcda_close_only.log; then echo 'FAIL Close-only survived'; exit 1; fi
@grep -qxF 'FAIL shutdown has an explicit enabled Quit route' $(BUILD)/calcda_close_only.log
@if $(BUILD)/test_power_notice_mutant > $(BUILD)/power_notice_mutant.log; then echo 'FAIL silent refusal survived'; exit 1; fi
@grep -qxF 'FAIL power-notice: cancellation must name the app and reason' $(BUILD)/power_notice_mutant.log
@echo 'GREEN calcda-power mutants: Close-only and silent cancellation both RED'
```

## test-calcda-route

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 16):
Prerequisites: `build/test_calcda_route build/.makefile.stamp`
```make
@$<
```

## test-flair-calcda

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33848):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/ppm_flair_calcda_check build/.makefile.stamp`
```make
$(call calcda-boot,flair_calcda_keys,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CALCDA_KEYS_SPEC,FLAIR-LIVE-OK)
@$(call CALCDA_UP,$(BUILD)/flair_calcda_keys.serial) && [ "$$(grep -c '^TENANT-EVT what=3$$' $(BUILD)/flair_calcda_keys.serial)" = 5 ] \
 || { printf '!!! test-flair-calcda FAIL [1/2] KEYS: Calculator not up, or not all five keys reached it\n'; grep -E '^(APPLE|TENANT-(REG|EVT|EXIT|CRASH))' $(BUILD)/flair_calcda_keys.serial; exit 1; }
@out=$$($(PPM_FLAIR_CALCDA_CHECK) window $(BUILD)/flair_calcda_keys.ppm "$(CALCDA_S7_CAPTURES)/s8_calc_12plus3eq.png"); rc=$$?; printf '%s\n' "$$out" | sed 's/^/    /'; \
 [ $$rc -eq 0 ] || { printf '!!! test-flair-calcda FAIL [1/2] KEYS: the window differs from s8_calc_12plus3eq.png\n'; exit 1; }
@printf '>>> test-flair-calcda [1/2] KEYS: typed 1 2 + 3 = -> the Mac'"'"'s window, readout 15\n'
$(call calcda-boot,flair_calcda_clicks,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CALCDA_CLICKS_SPEC,FLAIR-LIVE-OK)
@$(call CALCDA_UP,$(BUILD)/flair_calcda_clicks.serial) && [ "$$(grep -c '^TENANT-EVT what=1$$' $(BUILD)/flair_calcda_clicks.serial)" = 4 ] \
 || { printf '!!! test-flair-calcda FAIL [2/2] CLICKS: Calculator not up, or not all four clicks reached it\n'; grep -E '^(APPLE|TENANT-(REG|EVT|EXIT|CRASH))' $(BUILD)/flair_calcda_clicks.serial; exit 1; }
@out=$$($(PPM_FLAIR_CALCDA_CHECK) window $(BUILD)/flair_calcda_clicks.ppm "$(CALCDA_S7_CAPTURES)/s8_calc_inactive.png"); rc=$$?; printf '%s\n' "$$out" | sed 's/^/    /'; \
 [ $$rc -eq 0 ] || { printf '!!! test-flair-calcda FAIL [2/2] CLICKS: the window differs from s8_calc_inactive.png\n'; exit 1; }
@printf '>>> test-flair-calcda [2/2] CLICKS: clicked 9 - 2 =, then the desktop -> the Mac'"'"'s inactive window, readout 7\n'
@printf '>>> test-flair-calcda: green\n'
```

## test-flair-calcda-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 122):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-calcda-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_calcda_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-calcda-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-calcda_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_calcda_gate.img" --disk2 "$(BUILD)/flair-calcda_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-calcda_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-calcda_bochs.report.txt" || true
@cat "$(BUILD)/flair-calcda_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-calcda-bochs "$(BUILD)/flair-calcda_bochs.report.txt" "$(BUILD)/flair-calcda_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-calcda-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33862):
Prerequisites: `test-flair-calcda build/flair_data_calcda_diverge.img build/flair_tenants_calcda_rdoc_no_close.img build/.makefile.stamp`
```make
$(call calcda-boot,flair_calcda_diverge,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_CALCDA_DIVERGE_IMG),FLAIR_CALCDA_CLICKS_SPEC,FLAIR-LIVE-OK)
@$(call CALCDA_UP,$(BUILD)/flair_calcda_diverge.serial) || { printf '!!! test-flair-calcda-mutant: diverge RED for the wrong reason (Calculator not up)\n'; exit 1; }
@out=$$($(PPM_FLAIR_CALCDA_CHECK) window $(BUILD)/flair_calcda_diverge.ppm "$(CALCDA_S7_CAPTURES)/s8_calc_inactive.png"); rc=$$?; \
 [ $$rc -ne 0 ] || { printf '!!! test-flair-calcda-mutant: CALCDA_MUT_KEY_CLICK_DIVERGE passed -- decoration\n'; exit 1; }; \
 printf '%s\n' "$$out" | grep -q '^!!! CW3 READOUT' && printf '%s\n' "$$out" | grep -q '^>>> CW1 FRAME' && printf '%s\n' "$$out" | grep -q '^>>> CW2 FACE' \
 || { printf '%s\n' "$$out"; printf '!!! test-flair-calcda-mutant: diverge RED, but not on CW3 alone\n'; exit 1; }; \
 printf '>>> test-flair-calcda-mutant: CALCDA_MUT_KEY_CLICK_DIVERGE correctly RED (%s)\n' "$$(printf '%s\n' "$$out" | grep -m1 '^!!! CW3')"
$(call calcda-boot,flair_calcda_noclose,$(BUILD)/flair_tenants_calcda_rdoc_no_close.img,$(FLAIR_DATA_IMG),FLAIR_CALCDA_KEYS_SPEC,FLAIR-LIVE-OK)
@$(call CALCDA_UP,$(BUILD)/flair_calcda_noclose.serial) || { printf '!!! test-flair-calcda-mutant: rdoc_no_close RED for the wrong reason\n'; exit 1; }
@out=$$($(PPM_FLAIR_CALCDA_CHECK) window $(BUILD)/flair_calcda_noclose.ppm "$(CALCDA_S7_CAPTURES)/s8_calc_12plus3eq.png"); rc=$$?; \
 [ $$rc -ne 0 ] && printf '%s\n' "$$out" | grep -q '^!!! CW1 FRAME' && printf '%s\n' "$$out" | grep -q '^>>> CW2 FACE' || { printf '%s\n' "$$out"; printf '!!! test-flair-calcda-mutant: CHROME_MUT_RDOC_NO_CLOSE not RED on CW1 alone\n'; exit 1; }; \
 printf '>>> test-flair-calcda-mutant: CHROME_MUT_RDOC_NO_CLOSE correctly RED (%s)\n' "$$(printf '%s\n' "$$out" | grep -m1 '^!!! CW1')"
@printf '>>> test-flair-calcda-mutant: green (two mutants RED for the named reason)\n'
```

## test-flair-calcda-power

Declared tier: EMU; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 27):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_calcda_gate.img build/flair_data.img build/ppm_flair_shutdown_check build/.makefile.stamp`
```make
$(call calc-power-boot,calc_power_sd,$(CALC_POWER_SD),$(FLAIR_DATA_IMG),FLAIR-EVT what=3 where=)
@sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_sd.serial SHUTDOWN
@$(PPM_FLAIR_SHUTDOWN_CHECK_BIN) safe $(BUILD)/calc_power_sd.ppm
$(call pw-reboot,$(FLAIR_CALCDA_GATE_IMG),calc_power_rs,$(BUILD)/calc_power_rs_data.img,$(CALC_POWER_RS))
@sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_rs.serial RESTART
@grep -q 'OK=1' $(BUILD)/calc_power_rs.report
```

## test-flair-calcda-power-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/calcda_power.mk', line 63):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_calcda_gate.img build/flair_data_calcda_close_only.img build/ppm_flair_power_notice_check build/.makefile.stamp`
```make
$(call calc-power-boot,calc_power_close_only,$(CALC_POWER_SD),$(FLAIR_DATA_CALCDA_CLOSE_ONLY_IMG),FLAIR-EVT what=3 where=)
@if sh harness/emu/calcda_power_check.sh $(BUILD)/calc_power_close_only.serial SHUTDOWN > $(BUILD)/calc_power_close_only.check; then echo 'FAIL Close-only survived emulator'; exit 1; fi
@grep -qxF 'FAIL calcda-power: Calculator was not asked to quit' $(BUILD)/calc_power_close_only.check
@grep -qxF 'FINDER-POWER-CANCEL app=CALCULAT.EXE cannot-ask' $(BUILD)/calc_power_close_only.serial
@! grep -qE '^(TENANT-EXIT|FLAIR-SHUTDOWN|FLAIR-RESTART)' $(BUILD)/calc_power_close_only.serial
@$(PPM_POWER_NOTICE) $(BUILD)/calc_power_close_only.ppm
@echo 'GREEN calcda-power: Close-only mutant RED; unaskable app explained on screen'
```

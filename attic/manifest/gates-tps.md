## test-compiler

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16770):
Prerequisites: `test-tps-gen-fpc test-tps-gen-build test-tps-gen-mutant build/.makefile.stamp`
```make
@printf 'compiler_host_pass_rate=100%% (%s/%s)\n' \
	"$(words $(TPS_COMPILER_CORPUS))" "$(words $(TPS_COMPILER_CORPUS))"
@printf '>>> test-compiler: M7 host corpus GREEN -- FPC roots + deterministic TPS both-target builds\n'
```

## test-compiler-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18588):
Prerequisites: `test-compiler build/tps_compiler.img test-tps-gen-os build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-compiler-os: one bounded InitechDOS boot executes all %s TPS-generated corpus programs\n' \
	"$(words $(TPS_COMPILER_CORPUS))"
@rm -f "$(TPS_COMPILER_SERIAL)" "$(TPS_COMPILER_REPORT)"
@# COMPLETION NOTE (bead initech-6gkm, supersedes the 2026-08-18 ceiling
@# note): kmain prints SHELL-READY BEFORE command_repl runs AUTOEXEC.BAT, so
@# `exit` is TYPED AHEAD at SHELL-READY and sits in the kbd ring while the
@# 17-program batch runs (authentic DOS type-ahead; no corpus program reads
@# the keyboard). The old rail let the harness QUIT qemu ~400 ms after that
@# injection -- mid-batch, varying with host load: the "batch stalls /
@# programs silently skip" symptom was that truncation, never a MILTON bug.
@# $$(TPS_OS_COMPLETION) makes the harness wait for SHELL-EXIT (printed only
@# after the whole batch ran and the typed-ahead exit reached the prompt);
@# the ceiling only bounds a hung guest.
@$(call fixture-preflight,$(TPS_COMPILER_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_COMPILER_IMG)" \
	--name "$(TPS_COMPILER_NAME)" --out "$(BUILD)" --timeout-ms 120000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_COMPILER_REPORT)" || true
@if grep -q 'triple_fault=1' "$(TPS_COMPILER_REPORT)"; then \
	printf '!!! test-compiler-os FAIL: corpus boot triple-faulted\n'; exit 1; fi
@test -s "$(TPS_COMPILER_SERIAL)" \
	|| { printf '!!! test-compiler-os FAIL: no serial capture\n'; exit 1; }
@tr -d '\r' < "$(TPS_COMPILER_SERIAL)" > $(TPS_LEX_BUILD_DIR)/compiler_os.normalized
@passed=0; total=$(words $(TPS_COMPILER_CORPUS)); failed=0; \
for spec in $(TPS_COMPILER_CORPUS); do \
	name=$${spec%%:*}; begin="TPS-CORPUS-BEGIN $$name"; end="TPS-CORPUS-END $$name"; \
	got=$(TPS_LEX_BUILD_DIR)/$${name}_os.out; \
	fpc=$(TPS_LEX_BUILD_DIR)/run_$${name}_fpc/stdout.txt; \
	if test "$$(grep -Fxc "$$begin" $(TPS_LEX_BUILD_DIR)/compiler_os.normalized)" -ne 1 \
	   || test "$$(grep -Fxc "$$end" $(TPS_LEX_BUILD_DIR)/compiler_os.normalized)" -ne 1; then \
		printf '!!! test-compiler-os FAIL: %s markers missing/non-singular\n' "$$name"; \
		failed=1; continue; \
	fi; \
	sed -n "/^$$begin$$/,/^$$end$$/p" $(TPS_LEX_BUILD_DIR)/compiler_os.normalized \
		| sed '1d;$$d' > "$$got"; \
	if diff -u "$$fpc" "$$got" >/dev/null; then \
		passed=$$((passed + 1)); \
		printf '>>> test-compiler-os: PASS %s -- TPS/InitechDOS == FPC == hand golden\n' "$$name"; \
	else \
		printf '!!! test-compiler-os FAIL: TPS/InitechDOS vs FPC differs for %s:\n' "$$name"; \
		diff -u "$$fpc" "$$got" || true; failed=1; \
	fi; \
done; \
rate=$$((passed * 100 / total)); \
printf 'differential_pass_rate=%s%% (%s/%s)\n' "$$rate" "$$passed" "$$total"; \
test "$$failed" -eq 0 && test "$$rate" -eq 100 \
	|| { printf '!!! test-compiler-os FAIL: differential_pass_rate must be 100%%\n'; exit 1; }
@grep -q '^SHELL-EXIT$$' "$(TPS_COMPILER_SERIAL)" \
	|| { printf '!!! test-compiler-os FAIL: shell did not exit cleanly after corpus batch\n'; exit 1; }
@printf '>>> test-compiler-os: M7 differential GREEN (all TPS-generated programs executed on InitechDOS)\n'
```

## test-compiler-os-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18651):
Prerequisites: `test-tps-gen-mutant build/tps_gen_mut_varparam.img test-tps-gen-os-mutant build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-compiler-os-mutant: VARPARAM must make registered gen_func_deep differ without crashing\n'
@rm -f "$(TPS_GEN_MUT_VAR_SERIAL)" "$(TPS_GEN_MUT_VAR_REPORT)"
@$(call fixture-preflight,$(TPS_GEN_MUT_VAR_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_GEN_MUT_VAR_IMG)" \
	--name "$(TPS_GEN_MUT_VAR_NAME)" --out "$(BUILD)" --timeout-ms 30000 \
	--keys "m,v,a,r,p,a,r,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_GEN_MUT_VAR_REPORT)" || true
@if grep -q 'triple_fault=1' "$(TPS_GEN_MUT_VAR_REPORT)"; then \
	printf '!!! test-compiler-os-mutant FAIL: VARPARAM crashed instead of returning wrong semantics\n'; exit 1; fi
@tr -d '\r' < $(TPS_GEN_MUT_VAR_SERIAL) > $(TPS_LEX_BUILD_DIR)/gen_mut_var.normalized
@if grep -Fqx -f $(TPS_LEX_FIXTURE_DIR)/gen_func_deep.golden $(TPS_LEX_BUILD_DIR)/gen_mut_var.normalized; then \
	printf '!!! TPS_GEN_MUT_VARPARAM printed the clean corpus golden -- execution oracle is decoration\n'; exit 1; fi
@grep -q '^FACT5=' $(TPS_LEX_BUILD_DIR)/gen_mut_var.normalized \
	|| { printf '!!! TPS_GEN_MUT_VARPARAM did not execute to a semantic result\n'; exit 1; }
@grep -qF 'SWAPA=3 SWAPB=8 ALIASG=40 ALIASG2=40' $(TPS_LEX_BUILD_DIR)/gen_mut_var.normalized \
	|| { printf '!!! TPS_GEN_MUT_VARPARAM failed for the wrong semantic reason\n'; exit 1; }
@printf '>>> test-compiler-os-mutant: green (OFFBYONE + VARPARAM both make registered corpus execution RED)\n'
```

## test-flair-tps-journey

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exit_dos.mk', line 206):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img harness/diff/tps/tps_a_expected.txt harness/diff/tps/tpsrept_expected.txt harness/diff/tps/tps_b_expected.txt harness/diff/tps/tps_c_expected.txt harness/diff/tps/keys_a.txt build/.makefile.stamp`
```make
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
```

## test-flair-tps-journey-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 116):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-tps-journey-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-tps-journey-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-tps-journey_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-tps-journey_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-tps-journey_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-tps-journey_bochs.report.txt" || true
@cat "$(BUILD)/flair-tps-journey_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-tps-journey-bochs "$(BUILD)/flair-tps-journey_bochs.report.txt" "$(BUILD)/flair-tps-journey_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-tps-journey-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exit_dos.mk', line 218):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/flair_data_no_tps.img build/flair_exitdos_mut_cwdreset.img harness/diff/tps/tps_a_expected.txt harness/diff/tps/tpsrept_expected.txt harness/diff/tps/tps_b_expected.txt harness/diff/tps/tps_c_expected.txt build/.makefile.stamp`
```make
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
```

## test-pubs-tps

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 487):
Prerequisites: `test-pubs-build pubs-inventory build/pubs_tps_inventory build/pubs_gate build/.makefile.stamp`
```make
@$(PUBS_TPS_INV_BIN) inventory spec/tps_ritual.json > $(PUBS_CHECK)/tps.inventory
@$(PUBS_TPS_INV_BIN) check $(PUBS_CHECK)/tps.inventory $(PUBS_TPS_GATE_DIR)/main.cmds $(PUBS_CHECK)/shipped
@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(PUBS_TPS_GATE_DIR)/main.mentions
@$(PUBS_TPS_INV_BIN) reports $(PUBS_TPS_GATE_DIR)/main.reports $(PUBS_TPS_PROVED)
@set -e; n=0; while IFS='|' read -r f why; do [ -n "$$f" ] || continue; \
	d=$(PUBS_DIR)/manuals/$(PUBS_TPS_BOOK); \
	[ -f "$$d/$$f" ] && [ -f "$$d/$${f%.draft}.keys" ] || { printf '!!! test-pubs RED DRAFT-SCREEN-PAIR: %s/%s\n' "$$d" "$$f"; exit 1; }; \
	printf '!!! test-pubs DRAFT-SCREEN: %s NOT replayed (%s) -- re-capture after the change ships\n' "$$f" "$$why"; n=$$((n + 1)); \
done < $(PUBS_TPS_GATE_DIR)/main.draftscreens; \
printf '>>> test-pubs-tps: green (%s DRAFT transcripts listed above)\n' "$$n"
@# InitechBase (initech-ipu5.8): Using InitechBase (Volume One) and the
@# Quick Reference Guide each document the whole shipped surface, both
@# directions, by NAME|KIND; Volume Two teaches and only mentions.  Every
@# printed message is the catalogue text; Volume One has one entry per
@# catalogue message (GC-02).
@$(PUBS_GATE) commands $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) $(PUBS_BUILD)/gate/initechbase-volume-one/main.cmds
@$(PUBS_GATE) commands $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) $(PUBS_BUILD)/gate/initechbase-quick-reference/main.cmds
@[ -z "$$(grep -v '^#' $(PUBS_BUILD)/gate/initechbase-volume-two/main.cmds || true)" ] || { \
	printf '!!! test-pubs RED COMMAND-SURFACE: Volume Two teaches; its commands are documented in Using InitechBase\n'; exit 1; }
@$(PUBS_GATE) samirmentions $(PUBS_CHECK)/samir.shipped $(foreach b,$(PUBS_SAMIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
@$(PUBS_GATE) messages $(PUBS_CHECK)/samir.catalogue $(foreach b,$(PUBS_SAMIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.msgs)
@sh harness/pubs/check_allowlist.sh $(PUBS_GATE) $(PUBS_SAMIR_ALLOW) $(PUBS_CHECK)
```

## test-seed

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16783):
Prerequisites: `build/test_lexer build/test_parser build/.makefile.stamp`
```make
@printf ">>> test-seed: running seed front-end unit tests\n"
@$(SEED_TEST_LEXER)
@$(SEED_TEST_PARSER)
@printf ">>> test-seed: all green\n"
```

## test-seed-array-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17234):
Prerequisites: `build/qemu_harness build/seed_arith_array_mut_stride.elf build/seed_arith_array_mut_loskip.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-array-mutant: confirming BOTH B5 deep-bug legs go RED (Rule 6; ADR-0007 DEC-02/DEC-07, beads initech-54uu)\n"
@printf ">>> test-seed-array-mutant: leg (a) STRIDE -- element stride *2 instead of *4 (the bead's own named mutant)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_ARRAY_MUT_STRIDE_ELF)" --expect "$(SEED_ARRAY_EXPECT)" \
	--name seed_array_mut_stride --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-array-mutant FAIL: STRIDE mutant PASSED -- the uniform 4-byte element stride is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-array-mutant: leg (a) green (array.pas correctly failed under the *2-stride mutant)\n'; \
fi
@printf ">>> test-seed-array-mutant: leg (b) LO_SKIP -- the (index - lo) subtraction is omitted\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_ARRAY_MUT_LOSKIP_ELF)" --expect "$(SEED_ARRAY_EXPECT)" \
	--name seed_array_mut_loskip --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-array-mutant FAIL: LO_SKIP mutant PASSED -- the lo-offset subtraction is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-array-mutant: leg (b) green (array.pas correctly failed under the lo-skip mutant)\n'; \
fi
@printf ">>> test-seed-array-mutant: all green (both B5 deep-bug loci mutation-proven)\n"
```

## test-seed-bool-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17058):
Prerequisites: `build/qemu_harness build/seed_arith_bool_mut.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-bool-mutant: confirming the setl/setg-flip relational mutant goes RED (Rule 6; ADR-0007 DEC-03, beads initech-f0uc)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_BOOL_MUT_ELF)" --expect "$(SEED_BOOL_EXPECT)" \
	--name seed_bool_mut --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-bool-mutant FAIL: mutant PASSED -- the setl/setg distinction is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-bool-mutant: green (bool.pas correctly failed to reproduce its golden under the setl/setg-flip mutant)\n'; \
fi
```

## test-seed-char-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17126):
Prerequisites: `build/qemu_harness build/seed_arith_char_mut.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-char-mutant: confirming the chr(i)+1 off-by-one mutant goes RED (Rule 6; ADR-0007 DEC-02/DEC-07, beads initech-7mo3)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_CHAR_MUT_ELF)" --expect "$(SEED_CHAR_EXPECT)" \
	--name seed_char_mut --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-char-mutant FAIL: mutant PASSED -- chr() 8-bit truncation value is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-char-mutant: green (char.pas correctly failed to reproduce its golden under the chr(i)+1 off-by-one mutant)\n'; \
fi
```

## test-seed-codegen

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16893):
Prerequisites: `build/qemu_harness build/seed_smoke.elf build/seed_arith_precedence.elf build/seed_arith_parens.elf build/seed_arith_divmod.elf build/seed_arith_negative.elf build/seed_arith_negative_divmod.elf build/seed_arith_bool.elf build/seed_arith_control.elf build/seed_arith_char.elf build/seed_arith_func.elf build/seed_arith_array.elf build/seed_arith_record.elf build/seed_arith_string.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-codegen: SMOKE -- expect serial marker '%s'\n" "$(SEED_SMOKE_MARKER)"
@$(HARNESS_BIN) --kernel "$(SEED_SMOKE_ELF)" --expect "$(SEED_SMOKE_MARKER)" \
	--name seed_smoke --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: smoke marker missing (marker, fault, or timeout)\n"; exit 1; }
@printf ">>> test-seed-codegen: ARITH precedence -- expect 'R=14' (2 + 3 * 4)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_precedence.elf" --expect "R=14" \
	--name seed_prec --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: 2 + 3 * 4 did not print R=14\n"; exit 1; }
@printf ">>> test-seed-codegen: ARITH parens -- expect 'R=20' ((2 + 3) * 4)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_parens.elf" --expect "R=20" \
	--name seed_paren --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: (2 + 3) * 4 did not print R=20\n"; exit 1; }
@printf ">>> test-seed-codegen: ARITH div/mod -- expect 'D=3 M=2' (17 div 5, 17 mod 5)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_divmod.elf" --expect "D=3 M=2" \
	--name seed_divmod --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: 17 div 5 / 17 mod 5 did not print D=3 M=2\n"; exit 1; }
@printf ">>> test-seed-codegen: ARITH negative -- expect 'N=-5' (0 - 5)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_negative.elf" --expect "N=-5" \
	--name seed_neg --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: 0 - 5 did not print N=-5\n"; exit 1; }
@printf ">>> test-seed-codegen: ARITH negative div/mod -- expect 'A=-3 B=-2 C=-3 D=2 E=3 F=-2' (beads initech-tf3c)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_negative_divmod.elf" --expect "A=-3 B=-2 C=-3 D=2 E=3 F=-2" \
	--name seed_neg_divmod --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: negative-operand div/mod did not print A=-3 B=-2 C=-3 D=2 E=3 F=-2\n"; exit 1; }
@printf ">>> test-seed-codegen: BOOL -- relational + boolean + and/or/not, hand-computed truth table (beads initech-f0uc)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_bool.elf" --expect "$(SEED_BOOL_EXPECT)" \
	--name seed_bool --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: bool.pas did not print the full hand-computed truth-table golden\n"; exit 1; }
@printf ">>> test-seed-codegen: CONTROL -- if/while (+for/repeat sugar), hand-computed golden (beads initech-80iw)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_control.elf" --expect "$(SEED_CONTROL_EXPECT)" \
	--name seed_control --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: control.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: CHAR -- const decls (front-end folded) + char + ord/chr, hand-computed golden (beads initech-7mo3)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_char.elf" --expect "$(SEED_CHAR_EXPECT)" \
	--name seed_char --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: char.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: FUNC -- procedures/functions + stack frames + value/var params + recursion + forward (beads initech-63ce)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_func.elf" --expect "$(SEED_FUNC_EXPECT)" \
	--name seed_func --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: func.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: ARRAY -- static arrays (global/local, const-folded bound, lo=0, negative lo, boolean/char element types) + indexed l/r-value + indexed var-param args (beads initech-54uu)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_array.elf" --expect "$(SEED_ARRAY_EXPECT)" \
	--name seed_array --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: array.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: RECORD -- named record types (type/record), field access, arrays of records, deterministic layout, var-param/whole-record-assignment of record type (beads initech-rug7)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_record.elf" --expect "$(SEED_RECORD_EXPECT)" \
	--name seed_record --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: record.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: STRING -- fixed/ShortString strings (length/index/compare/concat) + frame-resident temporaries incl. the right-nested-concat two-live-temps clause (beads initech-39k2)\n"
@$(HARNESS_BIN) --kernel "$(BUILD)/seed_arith_string.elf" --expect "$(SEED_STRING_EXPECT)" \
	--name seed_string --timeout-ms 5000 \
	|| { printf "!!! test-seed-codegen FAIL: string.pas did not print the full hand-computed golden\n"; exit 1; }
@printf ">>> test-seed-codegen: BOOL complete-evaluation structural proof -- gen_binop (the ONE shared\n"
@printf "    code path for every binop, incl. and/or -- ADR-0007 DEC-03) must never emit a jump\n"
@printf "    mnemonic, so no branch could ever skip evaluating an operand.\n"
@if awk '/^static void gen_binop/,/^}/' seed/codegen.c | grep -qE '\bj[a-z]+\b'; then \
	printf "!!! test-seed-codegen FAIL: gen_binop contains a jump instruction -- complete-eval structural guarantee violated\n"; exit 1; \
else \
	printf ">>> test-seed-codegen: green (gen_binop is branch-free -- complete evaluation is structural, not incidental)\n"; \
fi
@printf ">>> test-seed-codegen: all green (smoke marker + arithmetic exact-output checks)\n"
```

## test-seed-codegen-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17024):
Prerequisites: `build/qemu_harness build/seed_arith_precedence_mut.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-codegen-mutant: confirming the imul->add MUL-emission mutant goes RED (Rule 6; initech-znb/initech-tf3c)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_PRECEDENCE_MUT_ELF)" --expect "R=14" \
	--name seed_prec_mut --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-codegen-mutant FAIL: mutant PASSED -- imul vs add for * is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-codegen-mutant: green (2 + 3 * 4 correctly failed to print R=14 under the add-for-imul mutant)\n'; \
fi
```

## test-seed-control-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17092):
Prerequisites: `build/qemu_harness build/seed_arith_control_mut.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-control-mutant: confirming the jz->jnz branch-invert mutant goes RED (Rule 6; ADR-0007 DEC-02/DEC-04, beads initech-80iw)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_CONTROL_MUT_ELF)" --expect "$(SEED_CONTROL_EXPECT)" \
	--name seed_control_mut --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-control-mutant FAIL: mutant PASSED -- the if/while guard-jump direction is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-control-mutant: green (control.pas correctly failed to reproduce its golden under the jz->jnz branch-invert mutant)\n'; \
fi
```

## test-seed-fileio-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17405):
Prerequisites: `build/seed_arith_fileio.elf build/seed_arith_fileio_mut_short.elf build/seed_arith_fileio_mut_wrong_handle.elf build/.makefile.stamp`
```make
@printf '>>> test-seed-fileio-build: host-native compile/assemble/link + distinct B8 RTL call-site contract\n'
@test "$$(grep -c 'call rtl_file_blockwrite$$' $(BUILD)/seed_arith_fileio.s)" -eq 2 \
	|| { printf '!!! clean fileio.s does not call rtl_file_blockwrite exactly twice\n'; exit 1; }
@test "$$(grep -c 'call rtl_file_blockwrite_short$$' $(BUILD)/seed_arith_fileio_mut_short.s)" -eq 2 \
	|| { printf '!!! SHORT_WRITE fileio.s does not call its mutant RTL entry exactly twice\n'; exit 1; }
@test "$$(grep -c 'call rtl_file_blockwrite_wrong_handle$$' $(BUILD)/seed_arith_fileio_mut_wrong_handle.s)" -eq 2 \
	|| { printf '!!! WRONG_HANDLE fileio.s does not call its mutant RTL entry exactly twice\n'; exit 1; }
@grep -q '^v_targetfile: resd 65$$' $(BUILD)/seed_arith_fileio.s \
	|| { printf '!!! file object is not the fixed 260-byte static-arena layout\n'; exit 1; }
@grep -q '^    sub esp, 260$$' $(BUILD)/seed_arith_fileio.s \
	|| { printf '!!! local file object is not a fixed 260-byte frame allocation\n'; exit 1; }
@grep -q '^    mov dword \[ebp-260\], 0$$' $(BUILD)/seed_arith_fileio.s \
	|| { printf '!!! local file handle is not initialized closed\n'; exit 1; }
@grep -q '^    mov byte \[ebp-256\], 0$$' $(BUILD)/seed_arith_fileio.s \
	|| { printf '!!! local file filename buffer is not initialized empty\n'; exit 1; }
@grep -q '^    call pf_exerciselocalfile$$' $(BUILD)/seed_arith_fileio.s \
	|| { printf '!!! local file procedure is not called by the fixture\n'; exit 1; }
@for fn in rtl_file_assign rtl_file_reset rtl_file_rewrite rtl_file_blockread rtl_file_blockwrite rtl_file_blockwrite_short rtl_file_blockwrite_wrong_handle; do \
	nm $(SEED_FILEIO_RT_OBJ) | grep -q " $$fn$$" \
		|| { printf '!!! seed RTL object is missing %s\n' "$$fn"; exit 1; }; \
done
@if nm $(SEED_RT_OBJ) | grep -q 'rtl_file_'; then \
	printf '!!! always-linked start.asm object contains B8 file RTL (fileless link identity broken)\n'; exit 1; \
fi
@printf '>>> test-seed-fileio-build: green (clean + both named mutant artifacts built; runtime RED proof remains on-OS)\n'
```

## test-seed-fileio-fpc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17435):
Prerequisites: `seed/examples/fpc/file_shared.pas build/.makefile.stamp`
```make
@printf '>>> test-seed-fileio-fpc: host Free Pascal byte/content round-trip oracle\n'
@command -v fpc >/dev/null 2>&1 \
	|| { printf '!!! test-seed-fileio-fpc FAIL: fpc is not installed\n'; exit 1; }
@mkdir -p $(SEED_FPC_FILEIO_DIR)
@cp -f $< $(SEED_FPC_FILEIO_DIR)/file_shared.pas
@fpc -B -O- -v0 -FE$(SEED_FPC_FILEIO_DIR) -o$(SEED_FPC_FILEIO_DIR)/file_shared_fpc $(SEED_FPC_FILEIO_DIR)/file_shared.pas >/dev/null 2>&1 \
	|| { printf '!!! test-seed-fileio-fpc FAIL: fpc could not compile file_shared.pas\n'; exit 1; }
@out=$$(cd $(SEED_FPC_FILEIO_DIR) && ./file_shared_fpc) \
	|| { printf '!!! test-seed-fileio-fpc FAIL: fpc-built fixture exited non-zero\n'; exit 1; }; \
 test "$$out" = "$(SEED_FILEIO_EXPECT)" \
	|| { printf '!!! test-seed-fileio-fpc FAIL: stdout differs\nexpected: %s\nactual:   %s\n' "$(SEED_FILEIO_EXPECT)" "$$out"; exit 1; }
@printf 'NORTHSTAR' | cmp -s - $(SEED_FPC_FILEIO_DIR)/B8FILE.BIN \
	|| { printf '!!! test-seed-fileio-fpc FAIL: B8FILE.BIN did not round-trip NORTHSTAR byte-exact\n'; exit 1; }
@test "$$(wc -c < $(SEED_FPC_FILEIO_DIR)/B8SINK.BIN | tr -d ' ')" -eq 0 \
	|| { printf '!!! test-seed-fileio-fpc FAIL: clean control wrote bytes to the sink file\n'; exit 1; }
@test "$$(wc -c < $(SEED_FPC_FILEIO_DIR)/B8LOCAL.BIN | tr -d ' ')" -eq 0 \
	|| { printf '!!! test-seed-fileio-fpc FAIL: local-file procedure did not create an empty file\n'; exit 1; }
@printf '>>> test-seed-fileio-fpc: green -- %s; file content=NORTHSTAR (9 bytes), sink=0 bytes\n' "$(SEED_FILEIO_EXPECT)"
```

## test-seed-fileio-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 17521):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/seed_fileio.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-seed-fileio-os : B8 file I/O ON InitechDOS\n'
@printf '  Ref: beads initech-ogxv; ADR-0007 DEC-05/DEC-07; the samir-boot precedent.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(SEED_FILEIO_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SEED_FILEIO_IMG)" \
	--name seed_fileio_os --out "$(BUILD)" --timeout-ms 30000 \
	--keys "$(SEED_FILEIO_OS_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BUILD)/seed_fileio_os.report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/seed_fileio_os.report"; then \
	printf '!!! test-seed-fileio-os FAIL: TRIPLE FAULT running FILEIO.COM\n'; exit 1; fi
@grep -q '^SHELL-READY$$' "$(BUILD)/seed_fileio_os.serial" \
	|| { printf '!!! test-seed-fileio-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' "$(BUILD)/seed_fileio_os.serial"; then \
	printf '!!! test-seed-fileio-os FAIL: the RTL hit a CF error path:\n'; \
	grep 'FILEIO-ERROR' "$(BUILD)/seed_fileio_os.serial"; exit 1; fi
@grep -qF '$(SEED_FILEIO_EXPECT)' "$(BUILD)/seed_fileio_os.serial" \
	|| { printf '!!! test-seed-fileio-os FAIL: exact FILEIO line missing on serial\nexpected: %s\ngot:\n' "$(SEED_FILEIO_EXPECT)"; \
	     grep '^FILEIO W1=' "$(BUILD)/seed_fileio_os.serial" || printf '  (no FILEIO result line at all)\n'; exit 1; }
@printf 'VERDICT   : PASS -- seed-compiled fileio.pas ran ON InitechDOS: %s\n' "$(SEED_FILEIO_EXPECT)"
@printf '======================================================================\n'
```

## test-seed-fileio-os-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 17546):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/seed_fileio_mut_short.img build/seed_fileio_mut_wrong.img build/.makefile.stamp`
```make
@printf '>>> test-seed-fileio-os-mutant: Rule 6 -- both compiler mutants must yield WRONG VALUES on metal\n'
@rc=0; \
for spec in "short:$(SEED_FILEIO_MUT_SHORT_IMG):SHORT_WRITE drops tail bytes" \
            "wrong:$(SEED_FILEIO_MUT_WRONG_IMG):WRONG_HANDLE writes to the next live handle"; do \
	tag=$${spec%%:*}; rest=$${spec#*:}; img=$${rest%%:*}; desc=$${rest#*:}; \
	$(call fixture-check,harness,$$img,FILEIO.COM) || exit 1; \
	$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$$img" \
		--name "seed_fileio_os_mut_$$tag" --out "$(BUILD)" --timeout-ms 30000 \
		--keys "$(SEED_FILEIO_OS_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
		2> "$(BUILD)/seed_fileio_os_mut_$$tag.report" || true; \
	if grep -q 'triple_fault=1' "$(BUILD)/seed_fileio_os_mut_$$tag.report"; then \
		printf '!!! test-seed-fileio-os-mutant FAIL: %s TRIPLE-FAULTED (crash, not wrong values)\n' "$$tag"; rc=1; continue; fi; \
	if grep -qF '$(SEED_FILEIO_EXPECT)' "$(BUILD)/seed_fileio_os_mut_$$tag.serial"; then \
		printf '!!! test-seed-fileio-os-mutant FAIL: %s printed the CLEAN line -- the oracle is decoration (%s)\n' "$$tag" "$$desc"; rc=1; continue; fi; \
	if ! grep -q '^FILEIO W1=' "$(BUILD)/seed_fileio_os_mut_$$tag.serial"; then \
		printf '!!! test-seed-fileio-os-mutant FAIL: %s produced NO FILEIO result line (did not run to completion; cannot judge wrong-values). NB: match the ^FILEIO W1= marker, NEVER bare "FILEIO" -- the shell DIR listing echoes FILEIO.COM\n' "$$tag"; rc=1; continue; fi; \
	printf '>>> mutant %s correctly RED (wrong values, no crash): ' "$$tag"; \
	grep -m1 '^FILEIO W1=' "$(BUILD)/seed_fileio_os_mut_$$tag.serial"; \
done; \
if [ "$$rc" != "0" ]; then exit 1; fi
@printf '>>> test-seed-fileio-os-mutant: green (both B8 mutants bite on metal)\n'
```

## test-seed-fpc-diff

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18921):
Prerequisites: `build/initechc build/seed_rt.o build/seed_fileio_rt.o seed/rt/seed.ld build/qemu_harness test-seed-fileio-fpc build/.makefile.stamp`
```make
@printf ">>> test-seed-fpc-diff: DEC-07 Rung 2 -- Turbo Initech seed vs Free Pascal, byte-exact stdout diff on the shared-subset corpus (beads initech-63ce/initech-39k2, ADR-0007 Sec 4.7)\n"
@if ! command -v fpc >/dev/null 2>&1; then \
	printf '!!! test-seed-fpc-diff FAIL (loud, not skipped -- Law 2): `fpc` (Free Pascal Compiler) is NOT installed in this environment.\n'; \
	printf '    The DEC-07 Rung 2 differential requires fpc to mint the INDEPENDENT golden. Install it (apt install fpc) to make this gate green.\n'; \
	printf '    Verified WITHOUT fpc: the seed compiles+runs each shared fixture and its serial output matches the hand-derived expectation (Rung 1 fallback below).\n'; \
	for pair in $(SEED_FPC_EMU_CORPUS); do \
		name=$${pair%%=*}; expect=$${pair#*=}; \
		printf '    Seed-side self-check on %s/%s.pas (expect "%s"):\n' "$(SEED_FPC_DIR)" "$$name" "$$expect"; \
		$(SEED_BIN) --emit-asm -o $(BUILD)/seed_fpc_$$name.s $(SEED_FPC_DIR)/$$name.pas \
			|| { printf '    !!! seed failed to compile %s\n' "$$name"; exit 1; }; \
		$(NASM) -f elf32 $(BUILD)/seed_fpc_$$name.s -o $(BUILD)/seed_fpc_$$name.o; \
		$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(BUILD)/seed_fpc_$$name.elf $(SEED_RT_OBJ) $(BUILD)/seed_fpc_$$name.o 2>/dev/null; \
		$(HARNESS_BIN) --kernel "$(BUILD)/seed_fpc_$$name.elf" --expect "$$expect" \
			--name seed_fpc_$$name --timeout-ms 5000 \
			|| { printf '    !!! seed serial output did not match the expectation for %s\n' "$$name"; exit 1; }; \
	done; \
	printf '    (all seed-side self-checks green; the fpc leg is BLOCKED on fpc being installed -- gate is RED by design until then)\n'; \
	exit 1; \
fi; \
printf '>>> test-seed-fpc-diff: fpc found (%s)\n' "$$(fpc -iV 2>/dev/null)"; \
rm -rf $(BUILD)/fpc_diff; mkdir -p $(BUILD)/fpc_diff; \
for pair in $(SEED_FPC_EMU_CORPUS); do \
	name=$${pair%%=*}; \
	cp $(SEED_FPC_DIR)/$$name.pas $(BUILD)/fpc_diff/$$name.pas; \
	fpc -B -O- -v0 -FE$(BUILD)/fpc_diff -o$(BUILD)/fpc_diff/$${name}_fpc $(BUILD)/fpc_diff/$$name.pas >/dev/null 2>&1 \
		|| { printf '!!! test-seed-fpc-diff FAIL: fpc could not compile %s\n' "$$name"; exit 1; }; \
	$(BUILD)/fpc_diff/$${name}_fpc > $(BUILD)/fpc_diff/out_$${name}_fpc.txt 2>&1 \
		|| { printf '!!! test-seed-fpc-diff FAIL: the fpc-built %s exited non-zero\n' "$$name"; exit 1; }; \
	$(SEED_BIN) --emit-asm -o $(BUILD)/fpc_diff/$${name}_seed.s $(SEED_FPC_DIR)/$$name.pas \
		|| { printf '!!! test-seed-fpc-diff FAIL: seed could not compile %s\n' "$$name"; exit 1; }; \
	$(NASM) -f elf32 $(BUILD)/fpc_diff/$${name}_seed.s -o $(BUILD)/fpc_diff/$${name}_seed.o; \
	$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(BUILD)/fpc_diff/$${name}_seed.elf $(SEED_RT_OBJ) $(BUILD)/fpc_diff/$${name}_seed.o 2>/dev/null; \
	$(HARNESS_BIN) --kernel "$(BUILD)/fpc_diff/$${name}_seed.elf" --serial-stdout \
		--name seed_fpc_diff_$$name --timeout-ms 5000 2>/dev/null > $(BUILD)/fpc_diff/out_$${name}_seed.txt || true; \
	if diff -u $(BUILD)/fpc_diff/out_$${name}_fpc.txt $(BUILD)/fpc_diff/out_$${name}_seed.txt >/dev/null 2>&1; then \
		printf '>>> test-seed-fpc-diff: green -- seed and fpc byte-identical on %s\n' "$$name"; \
	else \
		printf '!!! test-seed-fpc-diff FAIL: seed vs fpc stdout DIFFERS on %s:\n' "$$name"; \
		diff -u $(BUILD)/fpc_diff/out_$${name}_fpc.txt $(BUILD)/fpc_diff/out_$${name}_seed.txt || true; \
		exit 1; \
	fi; \
done; \
$(SEED_BIN) --emit-asm -o $(BUILD)/fpc_diff/file_shared_seed.s $(SEED_FPC_DIR)/file_shared.pas \
	|| { printf '!!! test-seed-fpc-diff FAIL: seed could not compile file_shared\n'; exit 1; }; \
$(NASM) -f elf32 $(BUILD)/fpc_diff/file_shared_seed.s -o $(BUILD)/fpc_diff/file_shared_seed.o; \
$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(BUILD)/fpc_diff/file_shared_seed.elf $(SEED_RT_OBJ) $(SEED_FILEIO_RT_OBJ) $(BUILD)/fpc_diff/file_shared_seed.o 2>/dev/null; \
printf '>>> test-seed-fpc-diff: green -- four direct-QEMU seed/fpc diffs + file_shared fpc host round-trip + seed compile/link\n'; \
printf '!!! test-seed-fpc-diff B8 NOTICE: file_shared seed execution/diff is NOT graded here; it requires the orchestrator-owned seed-on-InitechDOS leg (no such runner exists in-tree).\n'
```

## test-seed-func-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17178):
Prerequisites: `build/qemu_harness build/seed_arith_func_mut_frame.elf build/seed_arith_func_mut_varparam.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-func-mutant: confirming BOTH B4 deep-bug mutants go RED (Rule 6; ADR-0007 DEC-02/DEC-07, beads initech-63ce)\n"
@printf ">>> test-seed-func-mutant: leg (a) FRAME_OFF4 -- locals/result slots off by +4 (frame-offset locus)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_FUNC_MUT_FRAME_ELF)" --expect "$(SEED_FUNC_EXPECT)" \
	--name seed_func_mut_frame --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-func-mutant FAIL: FRAME_OFF4 mutant PASSED -- frame-offset arithmetic is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-func-mutant: leg (a) green (func.pas correctly failed under the +4 frame-offset mutant)\n'; \
fi
@printf ">>> test-seed-func-mutant: leg (b) VARPARAM_COPY -- var params degrade to value copies (aliasing locus)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_FUNC_MUT_VARPARAM_ELF)" --expect "$(SEED_FUNC_EXPECT)" \
	--name seed_func_mut_varparam --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-func-mutant FAIL: VARPARAM_COPY mutant PASSED -- var-parameter by-reference semantics is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-func-mutant: leg (b) green (func.pas correctly failed under the var->value-copy mutant)\n'; \
fi
@printf ">>> test-seed-func-mutant: all green (both B4 deep-bug loci mutation-proven)\n"
```

## test-seed-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16976):
Prerequisites: `build/test_parser_mut test-seed-num-range-mutant build/.makefile.stamp`
```make
@printf ">>> test-seed-mutant: confirming the OP_ADD->OP_MUL parser mutant goes RED (Rule 6; initech-znb/initech-tf3c)\n"
@$(SEED_TEST_PARSER_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-seed-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(SEED_TEST_PARSER_MUT) >/dev/null 2>&1; then \
	printf '!!! test-seed-mutant FAIL: mutant PASSED -- the +/* precedence distinction is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-mutant: green (OP_ADD->OP_MUL correctly RED)\n'; \
fi
```

## test-seed-num-range-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 16995):
Prerequisites: `build/test_lexer_mut_num_range build/.makefile.stamp`
```make
@printf '>>> test-seed-num-range-mutant: confirming SEED_MUT_NUM_RANGE goes RED (Rule 6; initech-veav/initech-cfc5)\n'
@$(SEED_TEST_LEXER_MUT_NUM_RANGE) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-seed-num-range-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(SEED_TEST_LEXER_MUT_NUM_RANGE) >/dev/null 2>&1; then \
	printf '!!! test-seed-num-range-mutant FAIL: mutant PASSED -- integer range checks are decoration\n'; exit 1; \
else \
	printf '>>> test-seed-num-range-mutant: green (SEED_MUT_NUM_RANGE correctly RED)\n'; \
fi
```

## test-seed-record-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17294):
Prerequisites: `build/qemu_harness build/seed_arith_record_mut_fieldoff4.elf build/seed_arith_record_mut_stride.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-record-mutant: confirming BOTH B6 deep-bug legs go RED (Rule 6; ADR-0007 DEC-02/DEC-07, beads initech-rug7)\n"
@printf ">>> test-seed-record-mutant: leg (a) FIELD_OFF4 -- every field offset shifted +4 bytes (the committee's own named locus)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_RECORD_MUT_FIELDOFF4_ELF)" --expect "$(SEED_RECORD_EXPECT)" \
	--name seed_record_mut_fieldoff4 --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-record-mutant FAIL: FIELD_OFF4 mutant PASSED -- the deterministic field-offset layout is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-record-mutant: leg (a) green (record.pas correctly failed under the +4 field-offset mutant)\n'; \
fi
@printf ">>> test-seed-record-mutant: leg (b) REC_STRIDE -- array-of-record element stride = field-count-1 slots\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_RECORD_MUT_STRIDE_ELF)" --expect "$(SEED_RECORD_EXPECT)" \
	--name seed_record_mut_stride --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-record-mutant FAIL: REC_STRIDE mutant PASSED -- the array-of-record element stride is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-record-mutant: leg (b) green (record.pas correctly failed under the field-count-1 stride mutant)\n'; \
fi
@printf ">>> test-seed-record-mutant: all green (both B6 deep-bug loci mutation-proven)\n"
```

## test-seed-repro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 19034):
Prerequisites: `build/initechc build/seed_rt.o build/seed_fileio_rt.o build/.makefile.stamp`
```make
@printf ">>> test-seed-repro: seed pipeline (initechc --emit-asm -> nasm -> ld) byte-identical across two independent scratch-dir builds (Rule 11; ADR-0007 Sec 4.6/Sec 7 FO-5, bead initech-3yv)\n"
@rm -rf $(SEED_REPRO_A) $(SEED_REPRO_B); mkdir -p $(SEED_REPRO_A) $(SEED_REPRO_B)
@n=0; h=0; \
 for f in $(SEED_REPRO_CORPUS); do \
	name=$$(basename "$$f" .pas); \
	rt_objs="$(SEED_RT_OBJ)"; \
	if [ "$$name" = fileio ]; then rt_objs="$$rt_objs $(SEED_FILEIO_RT_OBJ)"; fi; \
	n=$$((n+1)); \
	$(SEED_BIN) --emit-asm -o $(SEED_REPRO_A)/$$name.s "$$f" \
		|| { printf '!!! test-seed-repro FAIL: initechc failed on %s (build A)\n' "$$f"; exit 1; }; \
	$(SEED_BIN) --emit-asm -o $(SEED_REPRO_B)/$$name.s "$$f" \
		|| { printf '!!! test-seed-repro FAIL: initechc failed on %s (build B)\n' "$$f"; exit 1; }; \
	(cd $(SEED_REPRO_A) && $(NASM) -f elf32 $$name.s -o $$name.o) \
		|| { printf '!!! test-seed-repro FAIL: nasm failed on %s.s (build A)\n' "$$name"; exit 1; }; \
	(cd $(SEED_REPRO_B) && $(NASM) -f elf32 $$name.s -o $$name.o) \
		|| { printf '!!! test-seed-repro FAIL: nasm failed on %s.s (build B)\n' "$$name"; exit 1; }; \
	$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(SEED_REPRO_A)/$$name.elf $$rt_objs $(SEED_REPRO_A)/$$name.o 2>/dev/null \
		|| { printf '!!! test-seed-repro FAIL: ld failed on %s.o (build A)\n' "$$name"; exit 1; }; \
	$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(SEED_REPRO_B)/$$name.elf $$rt_objs $(SEED_REPRO_B)/$$name.o 2>/dev/null \
		|| { printf '!!! test-seed-repro FAIL: ld failed on %s.o (build B)\n' "$$name"; exit 1; }; \
	for stage in s o elf; do \
		ha=$$(sha256sum < $(SEED_REPRO_A)/$$name.$$stage | cut -d' ' -f1); \
		hb=$$(sha256sum < $(SEED_REPRO_B)/$$name.$$stage | cut -d' ' -f1); \
		h=$$((h+1)); \
		if [ "$$ha" != "$$hb" ]; then \
			printf '!!! test-seed-repro FAIL: %s.%s is NOT reproducible (fixture %s, stage %s)\n' "$$name" "$$stage" "$$f" "$$stage"; \
			printf '    build A sha256 = %s\n    build B sha256 = %s\n' "$$ha" "$$hb"; \
			printf '    nondeterminism in the seed pipeline breaks the DEC-06 K2==K3 precondition (ADR-0007 FO-5).\n'; \
			exit 1; \
		fi; \
	done; \
 done; \
 printf '>>> test-seed-repro: green -- %s corpus fixtures, %s hashes (.s+.o+.elf per fixture), byte-identical across two independent builds\n' "$$n" "$$h"
```

## test-seed-repro-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 19101):
Prerequisites: `build/initechc_mut_nondet build/seed_rt.o build/qemu_harness build/.makefile.stamp`
```make
@printf ">>> test-seed-repro-mutant: confirming SEED_MUT_NONDET (genuine getpid()-seeded emission) makes the reproducible-build diff go RED (Rule 6; bead initech-3yv, ADR-0007 FO-5)\n"
@rm -rf $(SEED_REPRO_MUT_A) $(SEED_REPRO_MUT_B); mkdir -p $(SEED_REPRO_MUT_A) $(SEED_REPRO_MUT_B)
@$(SEED_BIN_MUT_NONDET) --emit-asm -o $(SEED_REPRO_MUT_A)/bool.s $(ARITH_DIR)/bool.pas
@$(SEED_BIN_MUT_NONDET) --emit-asm -o $(SEED_REPRO_MUT_B)/bool.s $(ARITH_DIR)/bool.pas
@(cd $(SEED_REPRO_MUT_A) && $(NASM) -f elf32 bool.s -o bool.o)
@(cd $(SEED_REPRO_MUT_B) && $(NASM) -f elf32 bool.s -o bool.o)
@$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(SEED_REPRO_MUT_A)/bool.elf $(SEED_RT_OBJ) $(SEED_REPRO_MUT_A)/bool.o 2>/dev/null
@$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(SEED_REPRO_MUT_B)/bool.elf $(SEED_RT_OBJ) $(SEED_REPRO_MUT_B)/bool.o 2>/dev/null
@sa=$$(sha256sum < $(SEED_REPRO_MUT_A)/bool.s | cut -d' ' -f1); \
 sb=$$(sha256sum < $(SEED_REPRO_MUT_B)/bool.s | cut -d' ' -f1); \
 if [ "$$sa" = "$$sb" ]; then \
	printf '!!! test-seed-repro-mutant FAIL: SEED_MUT_NONDET produced an IDENTICAL .s across two runs -- getpid() nondeterminism did not manifest (PID reuse? a no-op hook?) -- this mutant proves nothing\n'; \
	exit 1; \
 fi; \
 printf '>>> test-seed-repro-mutant: .s differs across the two builds (sha256 %s vs %s) -- genuine process-to-process nondeterminism confirmed\n' "$$sa" "$$sb"
@oa=$$(sha256sum < $(SEED_REPRO_MUT_A)/bool.o | cut -d' ' -f1); \
 ob=$$(sha256sum < $(SEED_REPRO_MUT_B)/bool.o | cut -d' ' -f1); \
 ea=$$(sha256sum < $(SEED_REPRO_MUT_A)/bool.elf | cut -d' ' -f1); \
 eb=$$(sha256sum < $(SEED_REPRO_MUT_B)/bool.elf | cut -d' ' -f1); \
 if [ "$$oa" = "$$ob" ] || [ "$$ea" = "$$eb" ]; then \
	printf '!!! test-seed-repro-mutant FAIL: .s differed but .o/.elf did NOT -- the injected symbol never reached the linked artifact, so test-seed-repro would only ever catch this at the .s stage\n'; exit 1; \
 fi; \
 printf '>>> test-seed-repro-mutant: .o and .elf ALSO differ -- the injected dead symbol reaches every downstream stage, so test-seed-repro correctly fails loud at every one\n'
@if $(HARNESS_BIN) --kernel "$(SEED_REPRO_MUT_A)/bool.elf" --expect "$(SEED_BOOL_EXPECT)" \
	--name seed_repro_mut_bool --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '>>> test-seed-repro-mutant: green (SEED_MUT_NONDET is dead-data-only -- bool.pas single-run golden still holds; the reproducible-build diff correctly goes RED)\n'; \
else \
	printf '!!! test-seed-repro-mutant FAIL: SEED_MUT_NONDET broke bool.pas single-run behavior -- this mutant must perturb BYTES only, never semantics (Rule 6)\n'; exit 1; \
fi
```

## test-seed-string-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17354):
Prerequisites: `build/qemu_harness build/seed_arith_string_mut_clobber.elf build/seed_arith_string_mut_cmpnolen.elf build/.makefile.stamp`
```make
@printf ">>> test-seed-string-mutant: confirming BOTH B7 deep-bug legs go RED (Rule 6; ADR-0007 DEC-02/DEC-07, beads initech-39k2)\n"
@printf ">>> test-seed-string-mutant: leg (i) STR_TEMP_CLOBBER -- every string temp collapsed to index 0 (the right-nested-concat two-live-temps locus)\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_STRING_MUT_CLOBBER_ELF)" --expect "$(SEED_STRING_EXPECT)" \
	--name seed_string_mut_clobber --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-string-mutant FAIL: STR_TEMP_CLOBBER mutant PASSED -- the two-simultaneously-live-temps model is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-string-mutant: leg (i) green (string.pas correctly failed -- RNEST ghijkl -> ijklijkl under the collapse-to-one-temp mutant)\n'; \
fi
@printf ">>> test-seed-string-mutant: leg (ii) STR_CMP_NOLEN -- __str_cmp drops the prefix-equal length tiebreak\n"
@if $(HARNESS_BIN) --kernel "$(SEED_ARITH_STRING_MUT_CMPNOLEN_ELF)" --expect "$(SEED_STRING_EXPECT)" \
	--name seed_string_mut_cmpnolen --timeout-ms 5000 >/dev/null 2>&1; then \
	printf '!!! test-seed-string-mutant FAIL: STR_CMP_NOLEN mutant PASSED -- the string-compare length tiebreak is decoration\n'; exit 1; \
else \
	printf '>>> test-seed-string-mutant: leg (ii) green (string.pas correctly failed -- EQF FALSE -> TRUE, i.e. ''ab'' = ''abc'' wrongly equal, under the no-length-tiebreak mutant)\n'; \
fi
@printf ">>> test-seed-string-mutant: all green (both B7 deep-bug loci mutation-proven)\n"
```

## test-tps-backup

Declared tier: EMU; #  recipe to execute (from 'harness/emu/tps_backup.mk', line 60):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tpsrep_program.bin harness/diff/tps/autoexec_backup.bat harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat harness/diff/tps/tps_a_expected.txt harness/diff/tps/tpsrept_expected.txt harness/diff/tps/tps_b_expected.txt harness/diff/tps/tps_c_expected.txt build/.makefile.stamp`
```make
$(call tps-backup-run,tps_backup_ro,$(TPSREP_BIN),ro)
@( $(call tps-backup-check-refused,tps_backup_ro) )
@printf '>>> test-tps-backup: read-only .BAK: AH=13h refused, both files/attributes preserved, TPS-012 / ERRORLEVEL 10\n'
$(call tps-backup-run,tps_backup_hidden,$(TPSREP_BIN),hidden)
@( $(call tps-backup-check-refused,tps_backup_hidden) )
@printf '>>> test-tps-backup: hidden .BAK: AH=17h refused, both files/attributes preserved, TPS-012 / ERRORLEVEL 10\n'
$(call tps-backup-run,tps_backup_hiddentxt,$(TPSREP_BIN),hiddentxt)
@( $(call tps-backup-check-refused,tps_backup_hiddentxt) )
@printf '>>> test-tps-backup: hidden .TXT is an existing report, both files/attributes preserved\n'
$(call tps-backup-run,tps_backup_nobak,$(TPSREP_BIN),nobak)
$(call tps-backup-run,tps_backup_fresh,$(TPSREP_BIN),fresh)
$(call tps-backup-run,tps_backup_orphan,$(TPSREP_BIN),orphan)
@set -e; for c in nobak fresh orphan; do \
  ( $(call tps-backup-check-boot,tps_backup_$$c) ); \
  cmp $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/tps_backup_$$c.rept; \
  grep -qxF 'TPS REPORT GENERATOR TERMINATED. RETURN CODE 0.' $(BUILD)/tps_backup_$$c.got; \
  ! grep -q '^TPS-ERRORLEVEL-GE-' $(BUILD)/tps_backup_$$c.got; \
done
@mtype -i $(BUILD)/tps_backup_nobak.img ::TPSREPT.BAK | cmp $(TPS_DIR)/old_tpsrept.txt -
@grep -qxF 'PREVIOUS REPORT RETAINED AS TPSREPT.BAK.' $(BUILD)/tps_backup_nobak.got
@! mdir -a -b -i $(BUILD)/tps_backup_fresh.img ::TPSREPT.BAK >/dev/null 2>&1
@mtype -i $(BUILD)/tps_backup_orphan.img ::TPSREPT.BAK | cmp $(TPS_DIR)/old_tpsrept.bak -
@$(call fixture-check,harness,$(BUILD)/tps_backup_orphan.img,TPSREPT.BAK=AR)
@! grep -q '^PREVIOUS REPORT RETAINED' $(BUILD)/tps_backup_fresh.got $(BUILD)/tps_backup_orphan.got
@printf '>>> test-tps-backup: green (3 refusal legs + missing .BAK / first report / orphan read-only .BAK controls; QEMU)\n'
```

## test-tps-backup-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/tps_backup.mk', line 87):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tpsrep_program_unchecked_backup.bin harness/diff/tps/autoexec_backup.bat harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat build/.makefile.stamp`
```make
$(call tps-backup-run,tps_backup_mut_ro,$(TPS_BACKUP_MUT),ro)
$(call tps-backup-run,tps_backup_mut_hidden,$(TPS_BACKUP_MUT),hidden)
@set -e; for c in ro hidden; do \
  ( $(call tps-backup-check-boot,tps_backup_mut_$$c) ); \
  out=$$( ( $(call tps-backup-check-refused,tps_backup_mut_$$c) ) 2>&1 ) && { printf '!!! test-tps-backup-mutant FAIL: unchecked rotation passed\n'; exit 1; }; \
  printf '%s\n' "$$out" | grep -qxF 'TPSBAK: TPSREPT.TXT changed after failed rotation'; \
  grep -qxF 'TPS REPORT GENERATOR TERMINATED. RETURN CODE 0.' $(BUILD)/tps_backup_mut_$$c.got; \
  printf '>>> test-tps-backup-mutant: unchecked rotation correctly RED (%s): %s\n' $$c "$$out"; \
done
```

## test-tps-gen-build

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 18374):
Prerequisites: `test-tps-lex-build build/.makefile.stamp`
```make
@grep -q '^pf_runcodegenerator:' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-gen-build FAIL: seed output lacks fourth pass\n'; exit 1; }
@grep -q 'call rtl_file_blockwrite$$' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-gen-build FAIL: TPS does not write TPSOUT.S through B8 BlockWrite\n'; exit 1; }
@grep -q "Assign(OutputFile, 'TPSOUT.S')" $(TPS_LEX_SRC) \
	|| { printf '!!! test-tps-gen-build FAIL: fixed TPSOUT.S surface drifted\n'; exit 1; }
@printf '>>> test-tps-gen-build: green (seed compile + both links + B9.4 BSS/hard-ceiling discipline)\n'
```

## test-tps-gen-fpc

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 18299):
Prerequisites: `build/tps_lex/gen_tiny_tps_bare.elf build/tps_lex/gen_tiny_tps_dos.elf build/seed_fileio_rt.o seed/rt/seed.ld seed/rt/seed_dos.ld build/.makefile.stamp`
```make
@printf '>>> test-tps-gen-fpc: named 17-fixture shared corpus -- FPC root + deterministic TPS assembly/both links\n'
@for spec in $(TPS_COMPILER_CORPUS); do \
	name=$${spec%%:*}; fixture=$(TPS_LEX_FIXTURE_DIR)/$$name.pas; \
	golden=$(TPS_LEX_FIXTURE_DIR)/$$name.golden; \
	test -s "$$fixture" && test -s "$$golden" \
		|| { printf '!!! test-tps-gen-fpc FAIL: %s source/golden pair missing\n' "$$name"; exit 1; }; \
	for pass in a b; do \
		run=$(TPS_LEX_BUILD_DIR)/run_$${name}_$$pass; \
		rm -rf "$$run"; mkdir -p "$$run"; \
		cp -f "$$fixture" "$$run/TPSIN.PAS"; \
		(cd "$$run" && ../tps_fpc > ../$${name}_$$pass.raw) \
			|| { printf '!!! test-tps-gen-fpc FAIL: TPS failed on %s pass %s\n' "$$name" "$$pass"; exit 1; }; \
		test "$$(grep -c '^TPS-GEN-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$${name}_$$pass.raw)" -eq 1 \
			&& test "$$(grep -c '^TPS-GEN-END$$' $(TPS_LEX_BUILD_DIR)/$${name}_$$pass.raw)" -eq 1 \
			&& test "$$(grep -c '^TPS-GEN-OK bytes=[0-9][0-9]* labels=[0-9][0-9]*$$' $(TPS_LEX_BUILD_DIR)/$${name}_$$pass.raw)" -eq 1 \
			&& ! grep -q '^TPS-GEN-ERROR' $(TPS_LEX_BUILD_DIR)/$${name}_$$pass.raw \
			&& test -s "$$run/TPSOUT.S" \
			|| { printf '!!! test-tps-gen-fpc FAIL: %s pass %s lacks a clean fourth bracket/TPSOUT.S\n' "$$name" "$$pass"; exit 1; }; \
		(cd "$$run" && $(NASM) -f elf32 TPSOUT.S -o TPSOUT.o) \
			|| { printf '!!! test-tps-gen-fpc FAIL: nasm rejected %s pass %s\n' "$$name" "$$pass"; exit 1; }; \
		(cd "$$run" && $(LD) -m elf_i386 -T $(abspath $(SEED_RT_LD)) -o TPSOUT_bare.elf \
			$(abspath $(SEED_RT_OBJ)) $(abspath $(SEED_FILEIO_RT_OBJ)) TPSOUT.o); \
		(cd "$$run" && $(LD) -m elf_i386 -T $(abspath $(SEED_DOS_LD)) -o TPSOUT_dos.elf \
			$(abspath $(SEED_DOS_RT_OBJ)) $(abspath $(SEED_FILEIO_RT_OBJ)) TPSOUT.o); \
	done; \
	for artifact in TPSOUT.S TPSOUT.o TPSOUT_bare.elf TPSOUT_dos.elf; do \
		cmp -s $(TPS_LEX_BUILD_DIR)/run_$${name}_a/$$artifact \
		       $(TPS_LEX_BUILD_DIR)/run_$${name}_b/$$artifact \
			|| { printf '!!! test-tps-gen-fpc FAIL: %s %s is nondeterministic\n' "$$name" "$$artifact"; exit 1; }; \
	done; \
	cp -f $(TPS_LEX_BUILD_DIR)/run_$${name}_a/TPSOUT.S $(TPS_LEX_BUILD_DIR)/$${name}_tps.s; \
	cp -f $(TPS_LEX_BUILD_DIR)/run_$${name}_a/TPSOUT.o $(TPS_LEX_BUILD_DIR)/$${name}_tps.o; \
	cp -f $(TPS_LEX_BUILD_DIR)/run_$${name}_a/TPSOUT_bare.elf $(TPS_LEX_BUILD_DIR)/$${name}_tps_bare.elf; \
	cp -f $(TPS_LEX_BUILD_DIR)/run_$${name}_a/TPSOUT_dos.elf $(TPS_LEX_BUILD_DIR)/$${name}_tps_dos.elf; \
	$(OBJCOPY) -O binary $(TPS_LEX_BUILD_DIR)/$${name}_tps_dos.elf \
		$(TPS_LEX_BUILD_DIR)/$${name}_tps.com; \
	fpcdir=$(TPS_LEX_BUILD_DIR)/run_$${name}_fpc; \
	rm -rf "$$fpcdir"; mkdir -p "$$fpcdir"; \
	cp -f "$$fixture" "$$fpcdir/$$name.pas"; \
	fpc -B -O- -v0 -FU"$$fpcdir" -FE"$$fpcdir" -o$${name}_fpc "$$fpcdir/$$name.pas" >/dev/null \
		|| { printf '!!! test-tps-gen-fpc FAIL: fpc rejected corpus fixture %s\n' "$$name"; exit 1; }; \
	(cd "$$fpcdir" && ./$${name}_fpc > stdout.txt) \
		|| { printf '!!! test-tps-gen-fpc FAIL: fpc fixture %s exited non-zero\n' "$$name"; exit 1; }; \
	diff -u "$$golden" "$$fpcdir/stdout.txt" \
		|| { printf '!!! test-tps-gen-fpc FAIL: fpc stdout differs from hand golden for %s\n' "$$name"; exit 1; }; \
	printf '>>> test-tps-gen-fpc: green -- %s FPC==golden; TPS .s/.o/bare/DOS deterministic\n' "$$name"; \
done
@printf '>>> test-tps-gen-fpc: TPS generates itself twice, byte-identically, and both runtime pairs link\n'
@for pass in a b; do \
	run=$(TPS_LEX_BUILD_DIR)/run_gen_self_$$pass; \
	rm -rf "$$run"; mkdir -p "$$run"; \
	cp -f $(TPS_LEX_SRC) "$$run/TPSIN.PAS"; \
	(cd "$$run" && ../tps_fpc > ../gen_self_$$pass.raw) \
		|| { printf '!!! test-tps-gen-fpc FAIL: TPS self-generation pass %s failed\n' "$$pass"; exit 1; }; \
	grep -q '^TPS-GEN-OK bytes=[0-9][0-9]* labels=[0-9][0-9]*$$' $(TPS_LEX_BUILD_DIR)/gen_self_$$pass.raw \
		&& test -s "$$run/TPSOUT.S" \
		|| { printf '!!! test-tps-gen-fpc FAIL: TPS self-generation pass %s incomplete\n' "$$pass"; exit 1; }; \
done
@cmp -s $(TPS_LEX_BUILD_DIR)/run_gen_self_a/TPSOUT.S $(TPS_LEX_BUILD_DIR)/run_gen_self_b/TPSOUT.S \
	|| { printf '!!! test-tps-gen-fpc FAIL: TPS self-generated assembly differs\n'; exit 1; }
@cp -f $(TPS_LEX_BUILD_DIR)/run_gen_self_a/TPSOUT.S $(TPS_LEX_BUILD_DIR)/gen_self_tps.s
@$(NASM) -f elf32 $(TPS_LEX_BUILD_DIR)/gen_self_tps.s -o $(TPS_LEX_BUILD_DIR)/gen_self_tps.o
@$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(TPS_LEX_BUILD_DIR)/gen_self_tps_bare.elf \
	$(SEED_RT_OBJ) $(SEED_FILEIO_RT_OBJ) $(TPS_LEX_BUILD_DIR)/gen_self_tps.o
@$(LD) -m elf_i386 -T $(SEED_DOS_LD) -o $(TPS_LEX_BUILD_DIR)/gen_self_tps_dos.elf \
	$(SEED_DOS_RT_OBJ) $(SEED_FILEIO_RT_OBJ) $(TPS_LEX_BUILD_DIR)/gen_self_tps.o
@bss_end_hex=$$($(NM) -n $(TPS_LEX_BUILD_DIR)/gen_self_tps_dos.elf | awk '$$3 == "__bss_end" { print $$1 }'); \
	test -n "$$bss_end_hex" && test $$((0x$$bss_end_hex)) -lt $$((0x6f000)) \
		|| { printf '!!! test-tps-gen-fpc FAIL: TPS-generated self image reaches 0x6f000 (end=0x%s)\n' "$$bss_end_hex"; exit 1; }; \
	printf '>>> test-tps-gen-fpc: self output=%s bytes, linked DOS end=0x%s below 0x6f000\n' \
		"$$(stat -c%s $(TPS_LEX_BUILD_DIR)/gen_self_tps.s)" "$$bss_end_hex"
@printf '>>> test-tps-gen-fpc: green (17-fixture named corpus + deterministic self-assembly; independent hand goldens)\n'
```

## test-tps-gen-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 18418):
Prerequisites: `test-tps-gen-fpc build/tps_lex/tps_mut_gen_offbyone build/tps_lex/tps_mut_gen_labels build/tps_lex/tps_mut_gen_varparam build/.makefile.stamp`
```make
@for spec in off:tps_mut_gen_offbyone labels:tps_mut_gen_labels; do \
	tag=$${spec%%:*}; bin=$${spec#*:}; run=$(TPS_LEX_BUILD_DIR)/run_gen_mut_$$tag; \
	rm -rf "$$run"; mkdir -p "$$run"; cp -f $(TPS_GEN_TINY) "$$run/TPSIN.PAS"; \
	(cd "$$run" && ../$$bin > ../gen_mut_$$tag.raw) \
		|| { printf '!!! test-tps-gen-mutant FAIL: %s compiler failed before emission\n' "$$tag"; exit 1; }; \
	grep -q '^TPS-GEN-OK ' $(TPS_LEX_BUILD_DIR)/gen_mut_$$tag.raw && test -s "$$run/TPSOUT.S" \
		|| { printf '!!! test-tps-gen-mutant FAIL: %s did not reach a clean (but mutant) emit\n' "$$tag"; exit 1; }; \
done
@printf '>>> test-tps-gen-mutant: LABELS must fail NASM for duplicate labels\n'
@if $(NASM) -f elf32 $(TPS_LEX_BUILD_DIR)/run_gen_mut_labels/TPSOUT.S \
	-o $(TPS_LEX_BUILD_DIR)/gen_mut_labels.o >$(TPS_LEX_BUILD_DIR)/gen_mut_labels.nasm 2>&1; then \
	printf '!!! TPS_GEN_MUT_LABELS PASSED assembly -- label oracle is decoration\n'; exit 1; fi
@grep -qi 'inconsistently redefined\|redefined' $(TPS_LEX_BUILD_DIR)/gen_mut_labels.nasm \
	|| { printf '!!! TPS_GEN_MUT_LABELS failed NASM for the wrong reason\n'; cat $(TPS_LEX_BUILD_DIR)/gen_mut_labels.nasm; exit 1; }
@printf '>>> test-tps-gen-mutant: OFFBYONE assembles/links but differs at parameter offsets; execution RED is orchestrator-owned\n'
@$(NASM) -f elf32 $(TPS_LEX_BUILD_DIR)/run_gen_mut_off/TPSOUT.S -o $(TPS_LEX_BUILD_DIR)/gen_mut_off.o
@$(LD) -m elf_i386 -T $(SEED_DOS_LD) -o $(TPS_LEX_BUILD_DIR)/gen_mut_off_dos.elf \
	$(SEED_DOS_RT_OBJ) $(TPS_LEX_BUILD_DIR)/gen_mut_off.o
@! cmp -s $(TPS_GEN_FPC_TINY_ASM) $(TPS_LEX_BUILD_DIR)/run_gen_mut_off/TPSOUT.S \
	|| { printf '!!! TPS_GEN_MUT_OFFBYONE did not perturb emitted bytes\n'; exit 1; }
@grep -q 'lea eax, \[ebp+16\]' $(TPS_LEX_BUILD_DIR)/run_gen_mut_off/TPSOUT.S \
	|| { printf '!!! TPS_GEN_MUT_OFFBYONE did not expose the one-dword parameter shift\n'; exit 1; }
@printf '>>> test-tps-gen-mutant: VARPARAM must emit callee-slot stores for corpus fixture gen_func_deep\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_gen_mut_var
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_gen_mut_var
@cp -f $(TPS_LEX_FIXTURE_DIR)/gen_func_deep.pas $(TPS_LEX_BUILD_DIR)/run_gen_mut_var/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_gen_mut_var && ../tps_mut_gen_varparam > ../gen_mut_var.raw
@grep -q '^TPS-GEN-OK ' $(TPS_LEX_BUILD_DIR)/gen_mut_var.raw \
	&& test -s $(TPS_LEX_BUILD_DIR)/run_gen_mut_var/TPSOUT.S \
	|| { printf '!!! test-tps-gen-mutant FAIL: VARPARAM did not reach a clean mutant emit\n'; exit 1; }
@$(NASM) -f elf32 $(TPS_LEX_BUILD_DIR)/run_gen_mut_var/TPSOUT.S -o $(TPS_GEN_MUT_VAR_OBJ)
@$(LD) -m elf_i386 -T $(SEED_DOS_LD) -o $(TPS_GEN_MUT_VAR_DOS_ELF) \
	$(SEED_DOS_RT_OBJ) $(SEED_FILEIO_RT_OBJ) $(TPS_GEN_MUT_VAR_OBJ)
@cp -f $(TPS_LEX_BUILD_DIR)/run_gen_mut_var/TPSOUT.S $(TPS_GEN_MUT_VAR_ASM)
@! cmp -s $(TPS_LEX_BUILD_DIR)/gen_func_deep_tps.s $(TPS_GEN_MUT_VAR_ASM) \
	|| { printf '!!! TPS_GEN_MUT_VARPARAM did not perturb gen_func_deep emission\n'; exit 1; }
@grep -q 'lea eax, \[ebp+8\]' $(TPS_GEN_MUT_VAR_ASM) \
	|| { printf '!!! TPS_GEN_MUT_VARPARAM did not expose a callee-slot assignment\n'; exit 1; }
@printf '>>> test-tps-gen-mutant: green (LABELS assembly-RED; OFFBYONE and VARPARAM wrong-code artifacts await corpus execution)\n'
```

## test-tps-gen-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18485):
Prerequisites: `test-tps-gen-fpc build/qemu_harness build/tracer_secondary.img build/tps_gen.img build/tps_lex/STINY.COM build/.makefile.stamp`
```make
@printf '>>> test-tps-gen-os: TPS.COM generates TPSOUT.S on InitechDOS\n'
@$(call fixture-preflight,$(TPS_GEN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_GEN_IMG)" \
	--name tps_gen_compile --out "$(BUILD)" --timeout-ms 120000 \
	--keys "$(TPS_GEN_OS_KEYS),e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/gen_os_compile.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/gen_os_compile.report"; then \
	printf '!!! test-tps-gen-os FAIL: TPS.COM triple-faulted while generating\n'; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_gen_compile.serial > $(TPS_LEX_BUILD_DIR)/gen_os_compile.normalized
@grep -q '^TPS-GEN-OK bytes=[0-9][0-9]* labels=[0-9][0-9]*$$' $(TPS_LEX_BUILD_DIR)/gen_os_compile.normalized \
	|| { printf '!!! test-tps-gen-os FAIL: clean fourth-pass completion missing\n'; exit 1; }
@if grep -q 'TPS-GEN-ERROR\|FILEIO-ERROR' $(BUILD)/tps_gen_compile.serial; then \
	printf '!!! test-tps-gen-os FAIL: generator/file RTL error\n'; exit 1; fi
@mcopy -i $(TPS_GEN_IMG) ::TPSOUT.S $(TPS_GEN_OS_ASM) \
	|| { printf '!!! test-tps-gen-os FAIL: TPSOUT.S was not persisted to FAT\n'; exit 1; }
@test -s $(TPS_GEN_OS_ASM) \
	|| { printf '!!! test-tps-gen-os FAIL: extracted TPSOUT.S is empty\n'; exit 1; }
@$(NASM) -f elf32 $(TPS_GEN_OS_ASM) -o $(TPS_GEN_OS_OBJ)
@$(LD) -m elf_i386 -T $(SEED_RT_LD) -o $(TPS_GEN_OS_BARE_ELF) \
	$(SEED_RT_OBJ) $(TPS_GEN_OS_OBJ)
@$(LD) -m elf_i386 -T $(SEED_DOS_LD) -o $(TPS_GEN_OS_DOS_ELF) \
	$(SEED_DOS_RT_OBJ) $(TPS_GEN_OS_OBJ)
@$(OBJCOPY) -O binary $(TPS_GEN_OS_DOS_ELF) $(TPS_GEN_OS_COM)
@mcopy -o -i $(TPS_GEN_IMG) $(TPS_GEN_OS_COM) ::TINY.COM
@$(call fixture-check,recipe,$(TPS_GEN_IMG),$(call fixture-spec,$(TPS_GEN_IMG)) TINY.COM)
@printf '>>> test-tps-gen-os: execute TPS-generated TINY.COM on the same OS\n'
@$(call fixture-preflight,$(TPS_GEN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_GEN_IMG)" \
	--name tps_gen_execute --out "$(BUILD)" --timeout-ms 30000 \
	--keys "t,i,n,y,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/gen_os_execute.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/gen_os_execute.report"; then \
	printf '!!! test-tps-gen-os FAIL: TPS-generated program triple-faulted\n'; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_gen_execute.serial > $(TPS_LEX_BUILD_DIR)/gen_os_execute.normalized
@test "$$(grep -Fxc -f $(TPS_GEN_TINY_GOLDEN) $(TPS_LEX_BUILD_DIR)/gen_os_execute.normalized)" -eq 1 \
	|| { printf '!!! test-tps-gen-os FAIL: TPS-generated execution output is not exact\n'; exit 1; }
@mcopy -o -i $(TPS_GEN_IMG) $(TPS_GEN_SEED_COM) ::STINY.COM
@$(call fixture-check,recipe,$(TPS_GEN_IMG),$(call fixture-spec,$(TPS_GEN_IMG)) TINY.COM STINY.COM)
@printf '>>> test-tps-gen-os: execute seed-compiled fixture for three-way behavioral agreement\n'
@$(call fixture-preflight,$(TPS_GEN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_GEN_IMG)" \
	--name tps_gen_seed_execute --out "$(BUILD)" --timeout-ms 30000 \
	--keys "s,t,i,n,y,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/gen_os_seed_execute.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/gen_os_seed_execute.report"; then \
	printf '!!! test-tps-gen-os FAIL: seed fixture triple-faulted\n'; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_gen_seed_execute.serial > $(TPS_LEX_BUILD_DIR)/gen_os_seed_execute.normalized
@test "$$(grep -Fxc -f $(TPS_GEN_TINY_GOLDEN) $(TPS_LEX_BUILD_DIR)/gen_os_seed_execute.normalized)" -eq 1 \
	|| { printf '!!! test-tps-gen-os FAIL: seed execution differs from hand/TPS/fpc behavior\n'; exit 1; }
@printf '>>> test-tps-gen-os: green (TPS-on-OS emit -> both links -> on-OS execution; seed == TPS == fpc behavior)\n'
```

## test-tps-gen-os-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18550):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tps_gen_mut.img build/.makefile.stamp`
```make
@printf '>>> test-tps-gen-os-mutant: OFFBYONE must execute to a WRONG TINY value, without crash\n'
@$(call fixture-preflight,$(TPS_GEN_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_GEN_MUT_IMG)" \
	--name tps_gen_mut_execute --out "$(BUILD)" --timeout-ms 30000 \
	--keys "m,t,i,n,y,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/gen_os_mut_execute.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/gen_os_mut_execute.report"; then \
	printf '!!! test-tps-gen-os-mutant FAIL: OFFBYONE crashed instead of returning wrong values\n'; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_gen_mut_execute.serial > $(TPS_LEX_BUILD_DIR)/gen_os_mut_execute.normalized
@if grep -Fqx -f $(TPS_GEN_TINY_GOLDEN) $(TPS_LEX_BUILD_DIR)/gen_os_mut_execute.normalized; then \
	printf '!!! TPS_GEN_MUT_OFFBYONE printed the clean golden -- execution oracle is decoration\n'; exit 1; fi
@grep -q '^TINY=' $(TPS_LEX_BUILD_DIR)/gen_os_mut_execute.normalized \
	|| { printf '!!! TPS_GEN_MUT_OFFBYONE did not run to a wrong TINY value\n'; exit 1; }
@printf '>>> test-tps-gen-os-mutant: green (one-word parameter shift executes WRONG VALUE, no crash)\n'
```

## test-tps-lex-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17854):
Prerequisites: `build/tps_lex/tps_seed_bare.elf build/tps_lex/TPS.COM build/.makefile.stamp`
```make
@test -s $(TPS_LEX_SEED_BARE_ELF) && test -s $(TPS_LEX_COM) \
	|| { printf '!!! test-tps-lex-build FAIL: an output artifact is empty\n'; exit 1; }
@grep -q '^v_readchunk: resb 256$$' $(TPS_LEX_SEED_ASM) \
	&& grep -q '^v_nextchunk: resb 256$$' $(TPS_LEX_SEED_ASM) \
	&& grep -q '^pf_advancesourcewindow:' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-lex-build FAIL: source is not the fixed two-ShortString rolling window\n'; exit 1; }
@! grep -q '^v_sourcewords:' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-lex-build FAIL: retired B9.2 whole-source arena still consumes BSS\n'; exit 1; }
@grep -q 'call rtl_file_blockread$$' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-lex-build FAIL: tps.pas does not call the B8 BlockRead RTL\n'; exit 1; }
@$(NM) $(TPS_LEX_SEED_DOS_ELF) | grep -q ' __bss_end$$' \
	|| { printf '!!! test-tps-lex-build FAIL: DOS link lacks the crt0 BSS boundary\n'; exit 1; }
@# initech-6m52 BSS resolution: B9.3 retired B9.2's 80 KiB whole-source
@# arena. B9.4 spends 6 KiB of that returned budget on the 1024-row table
@# and packed generator state, under a documented 72 KiB soft reserve. The
@# load-bearing 0x6f000 arena-ceiling assertion below remains unchanged.
@bss_start_hex=$$($(NM) -n $(TPS_LEX_SEED_DOS_ELF) | awk '$$3 == "__bss_start" { print $$1 }'); \
	bss_end_hex=$$($(NM) -n $(TPS_LEX_SEED_DOS_ELF) | awk '$$3 == "__bss_end" { print $$1 }'); \
	test -n "$$bss_start_hex" && test -n "$$bss_end_hex" \
		|| { printf '!!! test-tps-lex-build FAIL: could not locate BSS boundaries\n'; exit 1; }; \
	bss_start=$$((0x$$bss_start_hex)); bss_end=$$((0x$$bss_end_hex)); \
	bss_size=$$((bss_end-bss_start)); \
	test "$$bss_size" -le $$((0x12000)) \
		|| { printf '!!! test-tps-lex-build FAIL: static BSS exceeds B9.4 soft reserve (size=%s, reserve=73728)\n' "$$bss_size"; exit 1; }; \
	test "$$bss_end" -lt $$((0x6f000)) \
		|| { printf '!!! test-tps-lex-build FAIL: static TPS image/BSS reaches env block (end=0x%s, ceiling=0x6f000)\n' "$$bss_end_hex"; exit 1; }; \
	printf '>>> test-tps-lex-build: DOS BSS=%s bytes <= B9.4 soft reserve 73728; end=0x%s below env 0x6f000\n' "$$bss_size" "$$bss_end_hex"
@printf '>>> test-tps-lex-build: green (seed compile + bare ELF link + DOS TPS.COM link)\n'
```

## test-tps-lex-fpc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17733):
Prerequisites: `build/tps_lex/lex_basic.out build/.makefile.stamp`
```make
@printf '>>> test-tps-lex-fpc: hand-computed full-surface golden\n'
@diff -u $(TPS_LEX_BASIC_GOLDEN) $(TPS_LEX_FPC_BASIC_OUT) \
	|| { printf '!!! test-tps-lex-fpc FAIL: lex_basic token dump differs\n'; exit 1; }
@printf '>>> test-tps-lex-fpc: bad-character + unterminated string/comment goldens\n'
@for name in $(TPS_LEX_ERROR_CASES); do \
	run=$(TPS_LEX_BUILD_DIR)/run_$$name; \
	rm -rf "$$run"; mkdir -p "$$run"; \
	cp -f $(TPS_LEX_FIXTURE_DIR)/$$name.pas "$$run/TPSIN.PAS"; \
	(cd "$$run" && ../tps_fpc > ../$$name.raw) \
		|| { printf '!!! test-tps-lex-fpc FAIL: lexer process failed on %s\n' "$$name"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$name.raw > $(TPS_LEX_BUILD_DIR)/$$name.normalized; \
	test "$$(grep -c '^TPS-LEX-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		&& test "$$(grep -c '^TPS-LEX-END$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		|| { printf '!!! test-tps-lex-fpc FAIL: %s lexer markers missing/non-singular\n' "$$name"; exit 1; }; \
	sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-LEX-END$$/p' $(TPS_LEX_BUILD_DIR)/$$name.normalized > $(TPS_LEX_BUILD_DIR)/$$name.out; \
	diff -u $(TPS_LEX_FIXTURE_DIR)/$$name.golden $(TPS_LEX_BUILD_DIR)/$$name.out \
		|| { printf '!!! test-tps-lex-fpc FAIL: %s error stream differs\n' "$$name"; exit 1; }; \
done
@printf '>>> test-tps-lex-fpc: deterministic non-empty self-lex (tps.pas reads itself)\n'
@for run in self_a self_b; do \
	dir=$(TPS_LEX_BUILD_DIR)/run_$$run; \
	rm -rf "$$dir"; mkdir -p "$$dir"; \
	cp -f $(TPS_LEX_SRC) "$$dir/TPSIN.PAS"; \
	(cd "$$dir" && ../tps_fpc > ../$$run.raw) \
		|| { printf '!!! test-tps-lex-fpc FAIL: self-lex process failed (%s)\n' "$$run"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$run.raw > $(TPS_LEX_BUILD_DIR)/$$run.normalized; \
	test "$$(grep -c '^TPS-LEX-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$$run.normalized)" -eq 1 \
		&& test "$$(grep -c '^TPS-LEX-END$$' $(TPS_LEX_BUILD_DIR)/$$run.normalized)" -eq 1 \
		|| { printf '!!! test-tps-lex-fpc FAIL: self-lex markers missing/non-singular (%s)\n' "$$run"; exit 1; }; \
	sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-LEX-END$$/p' $(TPS_LEX_BUILD_DIR)/$$run.normalized > $(TPS_LEX_BUILD_DIR)/$$run.out; \
done
@cmp -s $(TPS_LEX_BUILD_DIR)/self_a.out $(TPS_LEX_BUILD_DIR)/self_b.out \
	|| { printf '!!! test-tps-lex-fpc FAIL: two self-lex dumps differ (nondeterministic)\n'; exit 1; }
@test "$$(grep -c '^TPS-LEX-BEGIN$$' $(TPS_LEX_BUILD_DIR)/self_a.out)" -eq 1 \
	&& test "$$(grep -c '^TPS-LEX-END$$' $(TPS_LEX_BUILD_DIR)/self_a.out)" -eq 1 \
	&& test "$$(grep -c '^EOF$$' $(TPS_LEX_BUILD_DIR)/self_a.out)" -eq 1 \
	|| { printf '!!! test-tps-lex-fpc FAIL: self-lex is empty or markers/EOF are not singular\n'; exit 1; }
@! grep -q '^TPS-LEX-ERROR' $(TPS_LEX_BUILD_DIR)/self_a.out \
	|| { printf '!!! test-tps-lex-fpc FAIL: self-lex emitted TPS-LEX-ERROR\n'; exit 1; }
@printf '>>> test-tps-lex-fpc: green (independent golden + four error goldens + deterministic self-lex)\n'
```

## test-tps-lex-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17802):
Prerequisites: `build/tps_lex/lex_basic.out build/tps_lex/tps_mut_casefold build/tps_lex/tps_mut_stresc build/.makefile.stamp`
```make
@printf '>>> test-tps-lex-mutant: CASEFOLD must emit a mixed-case IDENT, not crash/error\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_mut_case
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_mut_case
@cp -f $(TPS_LEX_BASIC) $(TPS_LEX_BUILD_DIR)/run_mut_case/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_mut_case && ../tps_mut_casefold > ../mut_case.raw \
	|| { printf '!!! CASEFOLD mutant crashed/exited non-zero\n'; exit 1; }
@tr -d '\r' < $(TPS_LEX_BUILD_DIR)/mut_case.raw > $(TPS_LEX_BUILD_DIR)/mut_case.normalized
@sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-LEX-END$$/p' $(TPS_LEX_BUILD_DIR)/mut_case.normalized > $(TPS_LEX_BUILD_DIR)/mut_case.out
@! grep -q '^TPS-LEX-ERROR' $(TPS_LEX_BUILD_DIR)/mut_case.out \
	|| { printf '!!! CASEFOLD mutant errored instead of producing wrong values\n'; exit 1; }
@grep -q '^IDENT MiXeD_Id$$' $(TPS_LEX_BUILD_DIR)/mut_case.out \
	|| { printf '!!! CASEFOLD mutant did not fail for the named canonicalization reason\n'; exit 1; }
@if diff -q $(TPS_LEX_BASIC_GOLDEN) $(TPS_LEX_BUILD_DIR)/mut_case.out >/dev/null 2>&1; then \
	printf '!!! CASEFOLD mutant PASSED -- the canonical-identifier golden is decoration\n'; exit 1; fi
@printf '>>> test-tps-lex-mutant: STRESC must decode doubled quote as byte 33, not crash/error\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_mut_stresc
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_mut_stresc
@cp -f $(TPS_LEX_BASIC) $(TPS_LEX_BUILD_DIR)/run_mut_stresc/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_mut_stresc && ../tps_mut_stresc > ../mut_stresc.raw \
	|| { printf '!!! STRESC mutant crashed/exited non-zero\n'; exit 1; }
@tr -d '\r' < $(TPS_LEX_BUILD_DIR)/mut_stresc.raw > $(TPS_LEX_BUILD_DIR)/mut_stresc.normalized
@sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-LEX-END$$/p' $(TPS_LEX_BUILD_DIR)/mut_stresc.normalized > $(TPS_LEX_BUILD_DIR)/mut_stresc.out
@! grep -q '^TPS-LEX-ERROR' $(TPS_LEX_BUILD_DIR)/mut_stresc.out \
	|| { printf '!!! STRESC mutant errored instead of producing wrong values\n'; exit 1; }
@grep -q '^STRING 4 105 116 33 115$$' $(TPS_LEX_BUILD_DIR)/mut_stresc.out \
	|| { printf '!!! STRESC mutant did not fail for the named doubled-quote reason\n'; exit 1; }
@if diff -q $(TPS_LEX_BASIC_GOLDEN) $(TPS_LEX_BUILD_DIR)/mut_stresc.out >/dev/null 2>&1; then \
	printf '!!! STRESC mutant PASSED -- the doubled-quote golden is decoration\n'; exit 1; fi
@printf '>>> test-tps-lex-mutant: green (both source mutants compile and go RED for the right wrong value)\n'
```

## test-tps-lex-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 17895):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tps_lex.img build/tps_lex/lex_basic.out build/.makefile.stamp`
```make
@printf '>>> test-tps-lex-os: seed-built TPS.COM on InitechDOS == fpc-built TPS stdout\n'
@$(call fixture-preflight,$(TPS_LEX_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_LEX_IMG)" \
	--name tps_lex_os --out "$(BUILD)" --timeout-ms 120000 \
	--keys "$(TPS_LEX_OS_KEYS)" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/os.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/os.report"; then \
	printf '!!! test-tps-lex-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' $(BUILD)/tps_lex_os.serial \
	|| { printf '!!! test-tps-lex-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' $(BUILD)/tps_lex_os.serial; then \
	printf '!!! test-tps-lex-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' $(BUILD)/tps_lex_os.serial; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_lex_os.serial > $(TPS_LEX_BUILD_DIR)/os.normalized
@test "$$(grep -c '^TPS-LEX-BEGIN$$' $(TPS_LEX_BUILD_DIR)/os.normalized)" -eq 1 \
	&& test "$$(grep -c '^TPS-LEX-END$$' $(TPS_LEX_BUILD_DIR)/os.normalized)" -eq 1 \
	|| { printf '!!! test-tps-lex-os FAIL: anchored dump markers are missing/non-singular\n'; exit 1; }
@sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-LEX-END$$/p' $(TPS_LEX_BUILD_DIR)/os.normalized > $(TPS_LEX_BUILD_DIR)/os.dump
@! grep -q '^TPS-LEX-ERROR' $(TPS_LEX_BUILD_DIR)/os.dump \
	|| { printf '!!! test-tps-lex-os FAIL: TPS.COM emitted a lexer error\n'; exit 1; }
@diff -u $(TPS_LEX_FPC_BASIC_OUT) $(TPS_LEX_BUILD_DIR)/os.dump \
	|| { printf '!!! test-tps-lex-os FAIL: seed/InitechDOS dump differs from fpc stdout\n'; exit 1; }
@printf '>>> test-tps-lex-os: green (two-level differential byte-identical)\n'
```

## test-tps-parse-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18058):
Prerequisites: `test-tps-lex-build build/.makefile.stamp`
```make
@grep -q '^pf_parseprogram:' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-parse-build FAIL: seed output lacks the recursive-descent parser\n'; exit 1; }
@test -s $(TPS_LEX_SEED_BARE_ELF) && test -s $(TPS_LEX_COM) \
	|| { printf '!!! test-tps-parse-build FAIL: a linked parser artifact is empty\n'; exit 1; }
@printf '>>> test-tps-parse-build: green (seed compile + both links + BSS/arena checks)\n'
```

## test-tps-parse-fpc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 17944):
Prerequisites: `build/tps_lex/parse_rich.out build/.makefile.stamp`
```make
@printf '>>> test-tps-parse-fpc: hand-computed grammar-rich production trace\n'
@diff -u $(TPS_PARSE_RICH_GOLDEN) $(TPS_PARSE_FPC_RICH_OUT) \
	|| { printf '!!! test-tps-parse-fpc FAIL: grammar-rich parse trace differs\n'; exit 1; }
@printf '>>> test-tps-parse-fpc: four required located errors + explicit seed case divergence\n'
@for name in $(TPS_PARSE_ERROR_CASES); do \
	run=$(TPS_LEX_BUILD_DIR)/run_$$name; \
	rm -rf "$$run"; mkdir -p "$$run"; \
	cp -f $(TPS_LEX_FIXTURE_DIR)/$$name.pas "$$run/TPSIN.PAS"; \
	(cd "$$run" && ../tps_fpc > ../$$name.raw) \
		|| { printf '!!! test-tps-parse-fpc FAIL: TPS process failed on %s\n' "$$name"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$name.raw > $(TPS_LEX_BUILD_DIR)/$$name.normalized; \
	test "$$(grep -c '^TPS-PARSE-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		&& test "$$(grep -c '^TPS-PARSE-END$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		|| { printf '!!! test-tps-parse-fpc FAIL: %s parse markers missing/non-singular\n' "$$name"; exit 1; }; \
	grep '^TPS-PARSE-ERROR' $(TPS_LEX_BUILD_DIR)/$$name.normalized > $(TPS_LEX_BUILD_DIR)/$$name.error; \
	test "$$(wc -l < $(TPS_LEX_BUILD_DIR)/$$name.error)" -eq 1 \
		|| { printf '!!! test-tps-parse-fpc FAIL: %s did not emit exactly one parse error\n' "$$name"; exit 1; }; \
	diff -u $(TPS_LEX_FIXTURE_DIR)/$$name.golden $(TPS_LEX_BUILD_DIR)/$$name.error \
		|| { printf '!!! test-tps-parse-fpc FAIL: %s located error differs\n' "$$name"; exit 1; }; \
done
@printf '>>> test-tps-parse-fpc: TPS parses itself to EOF twice with one deterministic trace\n'
@for run in parse_self_a parse_self_b; do \
	dir=$(TPS_LEX_BUILD_DIR)/run_$$run; \
	rm -rf "$$dir"; mkdir -p "$$dir"; \
	cp -f $(TPS_LEX_SRC) "$$dir/TPSIN.PAS"; \
	(cd "$$dir" && ../tps_fpc > ../$$run.raw) \
		|| { printf '!!! test-tps-parse-fpc FAIL: TPS self-parse process failed (%s)\n' "$$run"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$run.raw > $(TPS_LEX_BUILD_DIR)/$$run.normalized; \
	test "$$(grep -c '^TPS-PARSE-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$$run.normalized)" -eq 1 \
		&& test "$$(grep -c '^TPS-PARSE-END$$' $(TPS_LEX_BUILD_DIR)/$$run.normalized)" -eq 1 \
		|| { printf '!!! test-tps-parse-fpc FAIL: self-parse markers missing/non-singular (%s)\n' "$$run"; exit 1; }; \
	sed -n '/^TPS-PARSE-BEGIN$$/,/^TPS-PARSE-END$$/p' $(TPS_LEX_BUILD_DIR)/$$run.normalized > $(TPS_LEX_BUILD_DIR)/$$run.out; \
	! grep -q '^TPS-PARSE-ERROR' $(TPS_LEX_BUILD_DIR)/$$run.out \
		|| { printf '!!! test-tps-parse-fpc FAIL: self-parse emitted TPS-PARSE-ERROR (%s)\n' "$$run"; exit 1; }; \
	test "$$(grep -c '^TPS-PARSE-OK$$' $(TPS_LEX_BUILD_DIR)/$$run.out)" -eq 1 \
		|| { printf '!!! test-tps-parse-fpc FAIL: self-parse did not reach EOF (%s)\n' "$$run"; exit 1; }; \
done
@cmp -s $(TPS_LEX_BUILD_DIR)/parse_self_a.out $(TPS_LEX_BUILD_DIR)/parse_self_b.out \
	|| { printf '!!! test-tps-parse-fpc FAIL: two TPS self-parse traces differ\n'; exit 1; }
@printf '>>> test-tps-parse-fpc: green (independent trace + five errors + deterministic self-parse tooth)\n'
```

## test-tps-parse-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18012):
Prerequisites: `build/tps_lex/tps_fpc build/tps_lex/tps_mut_precedence build/tps_lex/tps_mut_else build/.makefile.stamp`
```make
@printf '>>> test-tps-parse-mutant: PRECEDENCE moves plus into term -- valid input, wrong nesting trace\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_parse_prec_base $(TPS_LEX_BUILD_DIR)/run_parse_prec_mut
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_parse_prec_base $(TPS_LEX_BUILD_DIR)/run_parse_prec_mut
@cp -f $(TPS_PARSE_MUT_PREC_FIXTURE) $(TPS_LEX_BUILD_DIR)/run_parse_prec_base/TPSIN.PAS
@cp -f $(TPS_PARSE_MUT_PREC_FIXTURE) $(TPS_LEX_BUILD_DIR)/run_parse_prec_mut/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_parse_prec_base && ../tps_fpc > ../parse_prec_base.raw
@cd $(TPS_LEX_BUILD_DIR)/run_parse_prec_mut && ../tps_mut_precedence > ../parse_prec_mut.raw
@for who in base mut; do \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.raw > $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.normalized; \
	sed -n '/^TPS-PARSE-BEGIN$$/,/^TPS-PARSE-END$$/p' $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.normalized > $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.out; \
	! grep -q '^TPS-PARSE-ERROR' $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.out \
		&& test "$$(grep -c '^TPS-PARSE-OK$$' $(TPS_LEX_BUILD_DIR)/parse_prec_$$who.out)" -eq 1 \
		|| { printf '!!! PRECEDENCE %s run errored instead of producing a valid trace\n' "$$who"; exit 1; }; \
done
@test "$$(grep -c '^ENTER term L4$$' $(TPS_LEX_BUILD_DIR)/parse_prec_base.out)" -eq 2 \
	&& test "$$(grep -c '^ENTER term L4$$' $(TPS_LEX_BUILD_DIR)/parse_prec_mut.out)" -eq 1 \
	|| { printf '!!! PRECEDENCE mutant failed for the wrong reason (expected 2 -> 1 term entries on L4)\n'; exit 1; }
@! cmp -s $(TPS_LEX_BUILD_DIR)/parse_prec_base.out $(TPS_LEX_BUILD_DIR)/parse_prec_mut.out \
	|| { printf '!!! PRECEDENCE mutant PASSED -- the expression trace is decoration\n'; exit 1; }
@printf '>>> test-tps-parse-mutant: ELSE binds dangling else to outer if -- valid input, wrong event order\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_parse_else_base $(TPS_LEX_BUILD_DIR)/run_parse_else_mut
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_parse_else_base $(TPS_LEX_BUILD_DIR)/run_parse_else_mut
@cp -f $(TPS_PARSE_MUT_ELSE_FIXTURE) $(TPS_LEX_BUILD_DIR)/run_parse_else_base/TPSIN.PAS
@cp -f $(TPS_PARSE_MUT_ELSE_FIXTURE) $(TPS_LEX_BUILD_DIR)/run_parse_else_mut/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_parse_else_base && ../tps_fpc > ../parse_else_base.raw
@cd $(TPS_LEX_BUILD_DIR)/run_parse_else_mut && ../tps_mut_else > ../parse_else_mut.raw
@for who in base mut; do \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/parse_else_$$who.raw > $(TPS_LEX_BUILD_DIR)/parse_else_$$who.normalized; \
	sed -n '/^TPS-PARSE-BEGIN$$/,/^TPS-PARSE-END$$/p' $(TPS_LEX_BUILD_DIR)/parse_else_$$who.normalized > $(TPS_LEX_BUILD_DIR)/parse_else_$$who.out; \
	! grep -q '^TPS-PARSE-ERROR' $(TPS_LEX_BUILD_DIR)/parse_else_$$who.out \
		&& test "$$(grep -c '^TPS-PARSE-OK$$' $(TPS_LEX_BUILD_DIR)/parse_else_$$who.out)" -eq 1 \
		|| { printf '!!! ELSE %s run errored instead of producing a valid trace\n' "$$who"; exit 1; }; \
done
@base_else=$$(grep -n '^ENTER else-clause L6$$' $(TPS_LEX_BUILD_DIR)/parse_else_base.out | cut -d: -f1); \
	base_exit=$$(grep -n '^EXIT stmt-if L7$$' $(TPS_LEX_BUILD_DIR)/parse_else_base.out | head -1 | cut -d: -f1); \
	mut_else=$$(grep -n '^ENTER else-clause L6$$' $(TPS_LEX_BUILD_DIR)/parse_else_mut.out | cut -d: -f1); \
	mut_exit=$$(grep -n '^EXIT stmt-if L6$$' $(TPS_LEX_BUILD_DIR)/parse_else_mut.out | head -1 | cut -d: -f1); \
	test -n "$$base_else" && test -n "$$base_exit" && test "$$base_else" -lt "$$base_exit" \
		&& test -n "$$mut_else" && test -n "$$mut_exit" && test "$$mut_exit" -lt "$$mut_else" \
		|| { printf '!!! ELSE mutant failed for the wrong reason (nearest-if event order did not invert)\n'; exit 1; }
@! cmp -s $(TPS_LEX_BUILD_DIR)/parse_else_base.out $(TPS_LEX_BUILD_DIR)/parse_else_mut.out \
	|| { printf '!!! ELSE mutant PASSED -- the dangling-else trace is decoration\n'; exit 1; }
@printf '>>> test-tps-parse-mutant: green (both valid-input mutants emit the named wrong trace)\n'
```

## test-tps-parse-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18076):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tps_parse.img build/tps_lex/parse_rich.out build/.makefile.stamp`
```make
@printf '>>> test-tps-parse-os: seed-built TPS.COM parse trace on InitechDOS == fpc-built TPS trace\n'
@$(call fixture-preflight,$(TPS_PARSE_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_PARSE_IMG)" \
	--name tps_parse_os --out "$(BUILD)" --timeout-ms 120000 \
	--keys "$(TPS_PARSE_OS_KEYS)" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/parse_os.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/parse_os.report"; then \
	printf '!!! test-tps-parse-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' $(BUILD)/tps_parse_os.serial \
	|| { printf '!!! test-tps-parse-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' $(BUILD)/tps_parse_os.serial; then \
	printf '!!! test-tps-parse-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' $(BUILD)/tps_parse_os.serial; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_parse_os.serial > $(TPS_LEX_BUILD_DIR)/parse_os.normalized
@test "$$(grep -c '^TPS-PARSE-BEGIN$$' $(TPS_LEX_BUILD_DIR)/parse_os.normalized)" -eq 1 \
	&& test "$$(grep -c '^TPS-PARSE-END$$' $(TPS_LEX_BUILD_DIR)/parse_os.normalized)" -eq 1 \
	|| { printf '!!! test-tps-parse-os FAIL: anchored parse markers are missing/non-singular\n'; exit 1; }
@sed -n '/^TPS-PARSE-BEGIN$$/,/^TPS-PARSE-END$$/p' $(TPS_LEX_BUILD_DIR)/parse_os.normalized > $(TPS_LEX_BUILD_DIR)/parse_os.dump
@! grep -q '^TPS-PARSE-ERROR' $(TPS_LEX_BUILD_DIR)/parse_os.dump \
	|| { printf '!!! test-tps-parse-os FAIL: TPS.COM emitted a parser error\n'; exit 1; }
@diff -u $(TPS_PARSE_FPC_RICH_OUT) $(TPS_LEX_BUILD_DIR)/parse_os.dump \
	|| { printf '!!! test-tps-parse-os FAIL: seed/InitechDOS parse trace differs from fpc trace\n'; exit 1; }
@printf '>>> test-tps-parse-os: green (two-level parse differential byte-identical)\n'
```

## test-tps-report

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 230):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tpsrep_program.bin harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat harness/diff/tps/tps_a_expected.txt harness/diff/tps/tpsrept_expected.txt harness/diff/tps/tps_b_expected.txt harness/diff/tps/tps_c_expected.txt spec/tps_ritual.json build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-tps-report : TPS Report Generator (initech-8479.1)\n'
@printf '  typed at A:\\> ; every file byte through the FCB API ; goldens = real MS-DOS 6.22\n'
@printf '======================================================================\n'
@set -e; n=0; awk '{ line = $$0; while (1) { \
  if (!f) { if (match(line, /"(prompt|refuse|accept|refuse_missing_key)": *\[/)) { f = 1; line = substr(line, RSTART + RLENGTH) } else break } \
  e = index(line, "]"); q = match(line, /"[^"]*"/); \
  if (q && (e == 0 || RSTART < e)) { print substr(line, RSTART + 1, RLENGTH - 2); line = substr(line, RSTART + RLENGTH) } \
  else if (e) { f = 0; line = substr(line, e + 1) } else break } }' $(TPS_SPEC) > $(BUILD)/tps_spec_literals.txt; \
sed -n 's/.*"end_line": "\(.*\)".*/\1/p' $(TPS_SPEC) >> $(BUILD)/tps_spec_literals.txt; \
while IFS= read -r lit; do printf '%s\n' "$$lit" | sed 's/{n}/\n/g' | while IFS= read -r frag; do \
  [ -z "$$frag" ] && continue; grep -aqF -- "$$frag" $(TPSREP_BIN) || { printf '!!! test-tps-report FAIL: spec literal fragment "%s" not in TPSREP.COM\n' "$$frag"; exit 1; }; done; n=$$((n+1)); \
done < $(BUILD)/tps_spec_literals.txt; \
[ $$(wc -l < $(BUILD)/tps_spec_literals.txt) -ge 14 ] || { printf '!!! test-tps-report FAIL: spec literal extraction found too few rows\n'; exit 1; }; \
! grep -nE '^\s*mov\s+ah,\s*0x(3[C-F]|4[0-6])\b' $(TPSREP_ASM) || { printf '!!! test-tps-report FAIL: TPSREP issues a handle call\n'; exit 1; }; \
printf '>>> test-tps-report [spec]: %s locked literals present in TPSREP.COM; no handle call (AH=3Ch-46h) in the source\n' "$$(wc -l < $(BUILD)/tps_spec_literals.txt)"
@set -e; awk '/"id": *"R2"/ { r = 1; next } /"id": *"/ { r = 0 } r && /"prompt": *\[/ { p = 1; sub(/.*"prompt": *\[/, "") } p { print } p && /\]/ { p = 0 }' $(TPS_SPEC) | \
  grep -oE '"[^"]*"' > $(BUILD)/tps_spec_r2_prompt.txt; \
[ $$(wc -l < $(BUILD)/tps_spec_r2_prompt.txt) -eq 3 ] || { printf '!!! test-tps-report FAIL [tacit]: spec R2 prompt is not the three-line alignment prompt\n'; exit 1; }; \
! grep -nE '7|SPACE' $(BUILD)/tps_spec_r2_prompt.txt || { printf '!!! test-tps-report FAIL [tacit]: the locked R2 prompt states the count or the key (initech-5igf)\n'; exit 1; }; \
grep -qE '"tacit": *"[^"]*OP-7\.4' $(TPS_SPEC) || { printf '!!! test-tps-report FAIL [tacit]: spec R2 lost its tacit row (the count lives in OP-7.4)\n'; exit 1; }; \
printf '>>> test-tps-report [tacit/spec]: the locked R2 prompt (3 lines) names neither the count nor the key; the count is the manual'"'"'s (OP-7.4)\n'
$(call tps-run,$(TPS_IMG_A),$(SECONDARY_TRACER_IMG),tps_a,$(TPSREP_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
@grep -q '^SHELL-EXIT' $(BUILD)/tps_a.serial || { printf '!!! test-tps-report FAIL [A]: the prompt never came back (TPSREP hung?)\n'; exit 1; }
@grep -q 'tpsrep tps.dbf' $(BUILD)/tps_a.serial || { printf '!!! test-tps-report FAIL [A]: the command was not typed at the prompt\n'; exit 1; }
@diff $(TPS_DIR)/tps_a_expected.txt $(BUILD)/tps_a.got || { printf '!!! test-tps-report FAIL [A]: console transcript != real DOS\n'; exit 1; }
@cmp $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/tps_a.rept || { printf '!!! test-tps-report FAIL [A]: TPSREPT.TXT != real DOS (FCB sequential writes)\n'; exit 1; }
@cmp $(TPS_DIR)/old_tpsrept.txt $(BUILD)/tps_a.bak || { printf '!!! test-tps-report FAIL [A]: TPSREPT.BAK is not the previous report (FCB delete + rename)\n'; exit 1; }
@printf '>>> test-tps-report [A]: typed at A:\\>, ritual refused 6/8 presses, mismatched number, wrong memo, then ran; transcript (%s lines) + TPSREPT.TXT (%s B) == real DOS; previous report kept as .BAK\n' "$$(wc -l < $(BUILD)/tps_a.got)" "$$(wc -c < $(BUILD)/tps_a.rept)"
@$(call tps-check-tacit,tps_a) || { printf '!!! test-tps-report FAIL [tacit]: an R2 prompt the program printed states the count or the key, or was not reprinted per refusal (%s)\n' $(BUILD)/tps_a.r2span; exit 1; }
@printf '>>> test-tps-report [tacit/A]: %s R2 prompts printed (one per attempt), none states the count or the key\n' "$$(grep -c '^TOP OF FORM ALIGNMENT' $(BUILD)/tps_a.got)"
$(call tps-run,$(TPS_IMG_B),$(SECONDARY_TRACER_IMG),tps_b,$(TPSREP_BIN),$(TPS_EXIT_KEYS),$(TPS_DIR)/autoexec_b.bat=AUTOEXEC.BAT)
@diff $(TPS_DIR)/tps_b_expected.txt $(BUILD)/tps_b.got || { printf '!!! test-tps-report FAIL [B]: missing-parameter-file transcript/ERRORLEVEL != real DOS\n'; exit 1; }
@printf '>>> test-tps-report [B]: no TPSPARM.DAT -> TPS-001 / OP-7.3, ERRORLEVEL 1 (== real DOS)\n'
$(call tps-run,$(TPS_IMG_C),$(SECONDARY_TRACER_IMG),tps_c,$(TPSREP_BIN),$(TPS_C_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/autoexec_c.bat=AUTOEXEC.BAT)
@diff $(TPS_DIR)/tps_c_expected.txt $(BUILD)/tps_c.got || { printf '!!! test-tps-report FAIL [C]: cover-sheet-N transcript/ERRORLEVEL != real DOS\n'; exit 1; }
@! test -f $(BUILD)/tps_c.rept || { printf '!!! test-tps-report FAIL [C]: a report was written although the cover sheet was not attached\n'; exit 1; }
@printf '>>> test-tps-report [C]: cover sheet N -> TPS-005 memo reference, ERRORLEVEL 5, no TPSREPT.TXT (== real DOS)\n'
@printf 'VERDICT   : PASS -- the TPS Report Generator works from the prompt through the FCB API (QEMU only)\n'
@printf '======================================================================\n'
```

## test-tps-report-faults

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 338):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tpsrep_program.bin harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat harness/diff/tps/keys_ok.txt build/tps_badnum.dat build/tps_badpage.dbf build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'make test-tps-report-faults : TPS write status, LINES= and PAGES validation (initech-ycvk)\n'
@printf '======================================================================\n'
$(call tps-flt-d,$(TPSREP_BIN),ok)
@grep -q '^SHELL-EXIT' $(BUILD)/tps_fd_ok.serial || { printf '!!! test-tps-report-faults FAIL [D]: the prompt never came back\n'; exit 1; }
@$(call tps-flt-check-d,tps_fd_ok) || { printf '!!! test-tps-report-faults FAIL [D]: full volume: no TPS-009 / RETURN CODE 9, or a success claim, or a truncated TPSREPT.TXT left, or .BAK lost (audit L004)\n'; tail -n 6 $(BUILD)/tps_fd_ok.got; exit 1; }
@printf '>>> test-tps-report-faults [D]: full volume -> TPS-009, RETURN CODE 9, no success line, truncated report dropped, previous report kept as .BAK\n'
$(call tps-flt-ef,$(TPSREP_BIN),tps_fe,$(BUILD)/tps_badnum.dat=TPSPARM.DAT)
@$(call tps-flt-check-e,tps_fe) || { printf '!!! test-tps-report-faults FAIL [E]: LINES=banana not refused with TPS-010 / code 1 before the ritual (audit L028)\n'; head -n 12 $(BUILD)/tps_fe.got; exit 1; }
@printf '>>> test-tps-report-faults [E]: LINES=banana -> TPS-010 / OP-7.3, code 1, no ritual, previous TPSREPT.TXT untouched\n'
$(call tps-flt-ef,$(TPSREP_BIN),tps_ff,$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(BUILD)/tps_badpage.dbf=TPS.DBF)
@$(call tps-flt-check-f,tps_ff) || { printf '!!! test-tps-report-faults FAIL [F]: PAGES=XYZ not refused with TPS-011 / code 6 before the ritual (audit L029)\n'; head -n 12 $(BUILD)/tps_ff.got; exit 1; }
@printf '>>> test-tps-report-faults [F]: PAGES=XYZ in record 1 -> TPS-011 (RECORD 1), code 6, no ritual, previous TPSREPT.TXT untouched\n'
@printf 'VERDICT   : PASS -- TPSREP checks its write status and its numeric inputs (QEMU only)\n'
```

## test-tps-report-faults-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 354):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tpsrep_program_nowrchk.bin build/tpsrep_program_linesok.bin build/tpsrep_program_pagesok.bin harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat harness/diff/tps/keys_ok.txt build/tps_badnum.dat build/tps_badpage.dbf build/.makefile.stamp`
```make
$(call tps-flt-d,$(TPSREP_NOWR_BIN),mut)
@grep -q 'TPS REPORT WRITTEN TO' $(BUILD)/tps_fd_mut.got || { printf '!!! test-tps-report-faults-mutant FAIL: NOWRCHK did not run the report (RED for the wrong reason)\n'; exit 1; }
@if $(call tps-flt-check-d,tps_fd_mut); then printf '!!! test-tps-report-faults-mutant FAIL: NOWRCHK (write status ignored) passed [D]\n'; exit 1; fi
@printf '>>> test-tps-report-faults-mutant: NOWRCHK correctly RED on [D] (claims success on a full volume)\n'
$(call tps-flt-ef,$(TPSREP_LINESOK_BIN),tps_fem,$(BUILD)/tps_badnum.dat=TPSPARM.DAT)
@if $(call tps-flt-check-e,tps_fem); then printf '!!! test-tps-report-faults-mutant FAIL: LINESOK passed [E]\n'; exit 1; fi
@printf '>>> test-tps-report-faults-mutant: LINESOK correctly RED on [E]\n'
$(call tps-flt-ef,$(TPSREP_PAGESOK_BIN),tps_ffm,$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(BUILD)/tps_badpage.dbf=TPS.DBF)
@if $(call tps-flt-check-f,tps_ffm); then printf '!!! test-tps-report-faults-mutant FAIL: PAGESOK passed [F]\n'; exit 1; fi
@printf '>>> test-tps-report-faults-mutant: PAGESOK correctly RED on [F]\n'
@printf '>>> test-tps-report-faults-mutant: green (3/3 mutants RED)\n'
```

## test-tps-report-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 272):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fcb_seq.img build/tpsrep_program.bin build/tpsrep_program_count6.bin build/tpsrep_program_say7.bin harness/diff/tps/tps.dbf harness/diff/tps/tpsparm.dat harness/diff/tps/old_tpsrept.txt harness/diff/tps/old_tpsrept.bak harness/diff/tps/keys_a.txt harness/diff/tps/keys_c.txt harness/diff/tps/autoexec_b.bat harness/diff/tps/autoexec_c.bat harness/diff/tps/tps_a_expected.txt harness/diff/tps/tpsrept_expected.txt harness/diff/tps/tps_b_expected.txt harness/diff/tps/tps_c_expected.txt build/.makefile.stamp`
```make
$(call tps-run,$(BUILD)/tps_mut6.img,$(SECONDARY_TRACER_IMG),tps_mut6,$(TPSREP_MUT_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
@if $(call tps-check-a,tps_mut6); then printf '!!! test-tps-report-mutant FAIL: COUNT6 (six presses accepted) matched every golden\n'; exit 1; fi
@printf '>>> test-tps-report-mutant: COUNT6 correctly RED: '; diff $(TPS_DIR)/tps_a_expected.txt $(BUILD)/tps_mut6.got | sed -n '2p'
$(call tps-run,$(BUILD)/tps_mutseq.img,$(BUILD)/fcb_seq.img,tps_mutseq,$(TPSREP_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
@if $(call tps-check-a,tps_mutseq); then printf '!!! test-tps-report-mutant FAIL: FCB kernel mutant seq matched every golden\n'; exit 1; fi
@printf '>>> test-tps-report-mutant: FCB kernel mutant seq correctly RED (TPSREPT.TXT %s B vs golden %s B)\n' "$$(wc -c < $(BUILD)/tps_mutseq.rept 2>/dev/null || echo 0)" "$$(wc -c < $(TPS_DIR)/tpsrept_expected.txt)"
$(call tps-run,$(BUILD)/tps_say7.img,$(SECONDARY_TRACER_IMG),tps_say7,$(TPSREP_SAY7_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
@grep -q '^FORM ALIGNED\.$$' $(BUILD)/tps_say7.got || { printf '!!! test-tps-report-mutant FAIL: SAY7 did not run the ritual (RED for the wrong reason)\n'; exit 1; }
@if $(call tps-check-tacit,tps_say7); then printf '!!! test-tps-report-mutant FAIL: SAY7 (the count stated in the R2 prompt) passed [tacit] -- the tacit rule is decoration\n'; exit 1; fi
@printf '>>> test-tps-report-mutant: SAY7 correctly RED on [tacit]: %s\n' "$$(grep -m1 -E '7|SPACE' $(BUILD)/tps_say7.r2span)"
@printf '>>> test-tps-report-mutant: green (3/3 mutants RED)\n'
```

## test-tps-type-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18220):
Prerequisites: `test-tps-lex-build build/.makefile.stamp`
```make
@grep -q '^v_symnameoffset: resd 1024$$' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-type-build FAIL: seed output lacks the documented B9.4 1024-entry symbol arena\n'; exit 1; }
@grep -q '^pf_runtypechecker:' $(TPS_LEX_SEED_ASM) \
	|| { printf '!!! test-tps-type-build FAIL: seed output lacks the check-as-you-parse type driver\n'; exit 1; }
@test -s $(TPS_LEX_SEED_BARE_ELF) && test -s $(TPS_LEX_COM) \
	|| { printf '!!! test-tps-type-build FAIL: a linked typechecker artifact is empty\n'; exit 1; }
@printf '>>> test-tps-type-build: green (seed compile + both links + BSS/arena checks)\n'
```

## test-tps-type-fpc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18126):
Prerequisites: `build/tps_lex/type_rich.out build/.makefile.stamp`
```make
@printf '>>> test-tps-type-fpc: hand-computed declaration-rich symbol dump\n'
@diff -u $(TPS_TYPE_RICH_GOLDEN) $(TPS_TYPE_FPC_RICH_OUT) \
	|| { printf '!!! test-tps-type-fpc FAIL: declaration-rich symbol dump differs\n'; exit 1; }
@printf '>>> test-tps-type-fpc: six required located first-error goldens\n'
@for name in $(TPS_TYPE_ERROR_CASES); do \
	run=$(TPS_LEX_BUILD_DIR)/run_$$name; \
	rm -rf "$$run"; mkdir -p "$$run"; \
	cp -f $(TPS_LEX_FIXTURE_DIR)/$$name.pas "$$run/TPSIN.PAS"; \
	(cd "$$run" && ../tps_fpc > ../$$name.raw) \
		|| { printf '!!! test-tps-type-fpc FAIL: TPS process failed on %s\n' "$$name"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$name.raw > $(TPS_LEX_BUILD_DIR)/$$name.normalized; \
	test "$$(grep -c '^TPS-TYPE-BEGIN$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		&& test "$$(grep -c '^TPS-TYPE-END$$' $(TPS_LEX_BUILD_DIR)/$$name.normalized)" -eq 1 \
		|| { printf '!!! test-tps-type-fpc FAIL: %s type markers missing/non-singular\n' "$$name"; exit 1; }; \
	grep '^TPS-TYPE-ERROR' $(TPS_LEX_BUILD_DIR)/$$name.normalized > $(TPS_LEX_BUILD_DIR)/$$name.error; \
	test "$$(wc -l < $(TPS_LEX_BUILD_DIR)/$$name.error)" -eq 1 \
		|| { printf '!!! test-tps-type-fpc FAIL: %s did not emit exactly one type error\n' "$$name"; exit 1; }; \
	diff -u $(TPS_LEX_FIXTURE_DIR)/$$name.golden $(TPS_LEX_BUILD_DIR)/$$name.error \
		|| { printf '!!! test-tps-type-fpc FAIL: %s located error differs\n' "$$name"; exit 1; }; \
done
@printf '>>> test-tps-type-fpc: TPS typechecks itself twice with one deterministic dump\n'
@for run in type_self_a type_self_b; do \
	dir=$(TPS_LEX_BUILD_DIR)/run_$$run; \
	rm -rf "$$dir"; mkdir -p "$$dir"; \
	cp -f $(TPS_LEX_SRC) "$$dir/TPSIN.PAS"; \
	(cd "$$dir" && ../tps_fpc > ../$$run.raw) \
		|| { printf '!!! test-tps-type-fpc FAIL: TPS self-typecheck process failed (%s)\n' "$$run"; exit 1; }; \
	tr -d '\r' < $(TPS_LEX_BUILD_DIR)/$$run.raw > $(TPS_LEX_BUILD_DIR)/$$run.normalized; \
	sed -n '/^TPS-TYPE-BEGIN$$/,/^TPS-TYPE-END$$/p' $(TPS_LEX_BUILD_DIR)/$$run.normalized > $(TPS_LEX_BUILD_DIR)/$$run.out; \
	! grep -q '^TPS-TYPE-ERROR' $(TPS_LEX_BUILD_DIR)/$$run.out \
		&& test "$$(grep -c '^TPS-TYPE-OK$$' $(TPS_LEX_BUILD_DIR)/$$run.out)" -eq 1 \
		|| { printf '!!! test-tps-type-fpc FAIL: self-typecheck did not accept (%s)\n' "$$run"; exit 1; }; \
done
@cmp -s $(TPS_LEX_BUILD_DIR)/type_self_a.out $(TPS_LEX_BUILD_DIR)/type_self_b.out \
	|| { printf '!!! test-tps-type-fpc FAIL: two TPS self symbol dumps differ\n'; exit 1; }
@printf '>>> test-tps-type-fpc: green (independent dump + six errors + deterministic self-typecheck tooth)\n'
```

## test-tps-type-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 18190):
Prerequisites: `test-tps-type-fpc build/tps_lex/tps_mut_type_dupok build/tps_lex/tps_mut_type_assign build/.makefile.stamp`
```make
@printf '>>> test-tps-type-mutant: DUPOK accepts duplicate and emits an extra row (wrong dump, no crash)\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_type_mut_dup
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_type_mut_dup
@cp -f $(TPS_LEX_FIXTURE_DIR)/type_bad_duplicate.pas $(TPS_LEX_BUILD_DIR)/run_type_mut_dup/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_type_mut_dup && ../tps_mut_type_dupok > ../type_mut_dup.raw
@tr -d '\r' < $(TPS_LEX_BUILD_DIR)/type_mut_dup.raw > $(TPS_LEX_BUILD_DIR)/type_mut_dup.normalized
@sed -n '/^TPS-TYPE-BEGIN$$/,/^TPS-TYPE-END$$/p' $(TPS_LEX_BUILD_DIR)/type_mut_dup.normalized > $(TPS_LEX_BUILD_DIR)/type_mut_dup.out
@! grep -q '^TPS-TYPE-ERROR' $(TPS_LEX_BUILD_DIR)/type_mut_dup.out \
	&& test "$$(grep -c '^TPS-TYPE-OK$$' $(TPS_LEX_BUILD_DIR)/type_mut_dup.out)" -eq 1 \
	|| { printf '!!! DUPOK mutant errored/crashed instead of accepting the duplicate\n'; exit 1; }
@test "$$(grep -c '^SYM kind=var name=count ' $(TPS_LEX_BUILD_DIR)/type_mut_dup.out)" -eq 2 \
	|| { printf '!!! DUPOK mutant did not emit exactly two count rows\n'; exit 1; }
@printf '>>> test-tps-type-mutant: ASSIGN loses the required mismatch error (wrong acceptance, no crash)\n'
@rm -rf $(TPS_LEX_BUILD_DIR)/run_type_mut_assign
@mkdir -p $(TPS_LEX_BUILD_DIR)/run_type_mut_assign
@cp -f $(TPS_LEX_FIXTURE_DIR)/type_bad_assignment.pas $(TPS_LEX_BUILD_DIR)/run_type_mut_assign/TPSIN.PAS
@cd $(TPS_LEX_BUILD_DIR)/run_type_mut_assign && ../tps_mut_type_assign > ../type_mut_assign.raw
@tr -d '\r' < $(TPS_LEX_BUILD_DIR)/type_mut_assign.raw > $(TPS_LEX_BUILD_DIR)/type_mut_assign.normalized
@sed -n '/^TPS-TYPE-BEGIN$$/,/^TPS-TYPE-END$$/p' $(TPS_LEX_BUILD_DIR)/type_mut_assign.normalized > $(TPS_LEX_BUILD_DIR)/type_mut_assign.out
@grep '^TPS-TYPE-ERROR' $(TPS_LEX_BUILD_DIR)/type_mut_assign.out > $(TPS_LEX_BUILD_DIR)/type_mut_assign.error || true
@! grep -q '^TPS-TYPE-ERROR' $(TPS_LEX_BUILD_DIR)/type_mut_assign.out \
	&& test "$$(grep -c '^TPS-TYPE-OK$$' $(TPS_LEX_BUILD_DIR)/type_mut_assign.out)" -eq 1 \
	|| { printf '!!! ASSIGN mutant did not reach the named wrong acceptance\n'; exit 1; }
@test ! -s $(TPS_LEX_BUILD_DIR)/type_mut_assign.error
@if diff -q $(TPS_LEX_FIXTURE_DIR)/type_bad_assignment.golden $(TPS_LEX_BUILD_DIR)/type_mut_assign.error >/dev/null 2>&1; then \
	printf '!!! ASSIGN mutant matched the required error golden -- the mutation did not bite\n'; exit 1; fi
@printf '>>> test-tps-type-mutant: green (both mutants RED for the named wrong values)\n'
```

## test-tps-type-os

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18240):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/tps_type.img build/tps_lex/type_rich.full build/.makefile.stamp`
```make
@printf '>>> test-tps-type-os: seed-built TPS.COM on InitechDOS == fpc-built TPS over full triple bracket\n'
@$(call fixture-preflight,$(TPS_TYPE_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(TPS_TYPE_IMG)" \
	--name tps_type_os --out "$(BUILD)" --timeout-ms 120000 \
	--keys "$(TPS_TYPE_OS_KEYS)" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(TPS_LEX_BUILD_DIR)/type_os.report" || true
@if grep -q 'triple_fault=1' "$(TPS_LEX_BUILD_DIR)/type_os.report"; then \
	printf '!!! test-tps-type-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' $(BUILD)/tps_type_os.serial \
	|| { printf '!!! test-tps-type-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' $(BUILD)/tps_type_os.serial; then \
	printf '!!! test-tps-type-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' $(BUILD)/tps_type_os.serial; exit 1; fi
@tr -d '\r' < $(BUILD)/tps_type_os.serial > $(TPS_LEX_BUILD_DIR)/type_os.normalized
@for marker in TPS-LEX-BEGIN TPS-LEX-END TPS-PARSE-BEGIN TPS-PARSE-END TPS-TYPE-BEGIN TPS-TYPE-END; do \
	test "$$(grep -c "^$$marker$$" $(TPS_LEX_BUILD_DIR)/type_os.normalized)" -eq 1 \
		|| { printf '!!! test-tps-type-os FAIL: %s marker missing/non-singular\n' "$$marker"; exit 1; }; \
done
@sed -n '/^TPS-LEX-BEGIN$$/,/^TPS-TYPE-END$$/p' $(TPS_LEX_BUILD_DIR)/type_os.normalized > $(TPS_LEX_BUILD_DIR)/type_os.full
@diff -u $(TPS_TYPE_FPC_FULL_OUT) $(TPS_LEX_BUILD_DIR)/type_os.full \
	|| { printf '!!! test-tps-type-os FAIL: full seed/InitechDOS triple bracket differs from fpc\n'; exit 1; }
@printf '>>> test-tps-type-os: green (two-level full triple-bracket differential byte-identical)\n'
```

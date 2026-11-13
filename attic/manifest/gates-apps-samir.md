## test-dbase

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 16762):
Prerequisites: `test-dbase-roundtrip test-dbase-roundtrip-mutant test-dbase-diff test-dbase-diff-mutant test-dbase-diff-mutants build/.makefile.stamp`
```make
@printf '>>> test-dbase: M6 InitechBase differential GREEN -- round-trip + program diff (100%%)\n'
```

## test-dbase-corpus-pristine

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2721):
Prerequisites: `harness/diff/dbf_diff/corpus_pristine.sh build/.makefile.stamp | build`
```make
@printf ">>> test-dbase-corpus-pristine: the dBASE sample corpus == the pristine Ashton-Tate disk image, CNAMES.NDX authentic (initech-586.4.1)\n"
@sh $(CORPUS_PRISTINE_SH) "$(DBASE_PRISTINE_IMG)" "$(DBASE_PRISTINE_FILES)" $(BUILD)/corpus_pristine
@printf ">>> test-dbase-corpus-pristine: green\n"
```

## test-dbase-corpus-pristine-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2725):
Prerequisites: `harness/diff/dbf_diff/corpus_pristine.sh build/.makefile.stamp | build`
```make
@printf ">>> test-dbase-corpus-pristine-mutant: one flipped CNAMES.NDX key byte must go RED (Rule 6)\n"
@test -d "$(DBASE_PRISTINE_FILES)" || { sh harness/missing_reference.sh DBASE3_DECOMP "corpus dir absent ($(DBASE_PRISTINE_FILES))"; exit 0; }
@rm -rf $(BUILD)/corpus_pristine_mut.files && cp -r "$(DBASE_PRISTINE_FILES)" $(BUILD)/corpus_pristine_mut.files
@chmod u+w $(BUILD)/corpus_pristine_mut.files/CNAMES.NDX
@printf 'Z' | dd of=$(BUILD)/corpus_pristine_mut.files/CNAMES.NDX bs=1 seek=1036 conv=notrunc status=none
@if sh $(CORPUS_PRISTINE_SH) "$(DBASE_PRISTINE_IMG)" $(BUILD)/corpus_pristine_mut.files $(BUILD)/corpus_pristine_mut > $(BUILD)/corpus_pristine_mut.log 2>&1; then \
	printf '!!! test-dbase-corpus-pristine-mutant FAIL: a corrupted CNAMES.NDX PASSED -- the provenance gate is decoration\n'; exit 1; fi
@grep -q 'FAIL: CNAMES.NDX differs from the pristine disk image' $(BUILD)/corpus_pristine_mut.log \
	|| { cat $(BUILD)/corpus_pristine_mut.log; printf '!!! test-dbase-corpus-pristine-mutant FAIL: RED for the wrong reason\n'; exit 1; }
@printf '>>> test-dbase-corpus-pristine-mutant: green (%s)\n' "$$(grep -m1 'CNAMES.NDX differs' $(BUILD)/corpus_pristine_mut.log)"
```

## test-dbase-diff

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16716):
Prerequisites: `build/prog_diff samir-prog-goldens build/.makefile.stamp`
```make
@printf ">>> test-dbase-diff: xBase .prg program differential (stdout + result .dbf vs real III+ goldens; gate 100%%) (S6.4)\n"
@$(TEST_DBASE_DIFF) $(DBASE3_DECOMP) $(PROG_DIFF_DIR)
@printf ">>> test-dbase-diff: green\n"
```

## test-dbase-diff-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16722):
Prerequisites: `build/prog_diff_mut samir-prog-goldens build/.makefile.stamp`
```make
@printf ">>> test-dbase-diff-mutant: confirming the =-direction-swap mutant goes RED (Rule 6; initech-17n.2)\n"
@$(TEST_DBASE_DIFF_MUT) $(DBASE3_DECOMP) $(PROG_DIFF_DIR) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbase-diff-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBASE_DIFF_MUT) $(DBASE3_DECOMP) $(PROG_DIFF_DIR) >/dev/null 2>&1; then \
	printf '!!! test-dbase-diff-mutant FAIL: mutant PASSED -- the program differential is decoration\n'; exit 1; \
else \
	printf '>>> test-dbase-diff-mutant: green (=-direction swap correctly RED)\n'; \
fi
```

## test-dbase-diff-mutants

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16742):
Prerequisites: `build/prog_diff_mut_expr build/prog_diff_mut_flow build/prog_diff_mut_funcs build/prog_diff_mut_mutate build/prog_diff_mut_proc build/prog_diff_mut_query samir-prog-goldens build/.makefile.stamp`
```make
@printf '>>> test-dbase-diff-mutants: confirming all SIX xbase_prog_diff corpus mutants go RED (Rule 6; initech-0k2d)\n'
@for m in expr:$(TEST_DBASE_DIFF_MUT_EXPR) flow:$(TEST_DBASE_DIFF_MUT_FLOW) \
          funcs:$(TEST_DBASE_DIFF_MUT_FUNCS) mutate:$(TEST_DBASE_DIFF_MUT_MUTATE) \
          proc:$(TEST_DBASE_DIFF_MUT_PROC) query:$(TEST_DBASE_DIFF_MUT_QUERY); do \
	name=$${m%%:*}; bin=$${m#*:}; \
	"$$bin" "$(DBASE3_DECOMP)" "$(PROG_DIFF_DIR)" 2>/dev/null | grep -q 'checks,' \
		|| { printf '!!! test-dbase-diff-mutants FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if "$$bin" "$(DBASE3_DECOMP)" "$(PROG_DIFF_DIR)" >/dev/null 2>&1; then \
		printf '!!! test-dbase-diff-mutants FAIL: %s.out PASSED under mutation -- that golden is decoration\n' "$$name"; exit 1; \
	else \
		printf '>>> test-dbase-diff-mutants: green (%s.out correctly RED -- its corpus program bites)\n' "$$name"; \
	fi; \
done
@printf '>>> test-dbase-diff-mutants: green (ALL SIX corpus goldens mutation-proven)\n'
```

## test-dbase-roundtrip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3136):
Prerequisites: `build/test_dbase_roundtrip build/.makefile.stamp`
```make
@printf ">>> test-dbase-roundtrip: bidirectional .dbf+.dbt+.ndx round-trip (masked-memcmp + python read-back) (S6.3)\n"
@$(TEST_DBASE_ROUNDTRIP) $(DBASE3_DECOMP)
@printf ">>> test-dbase-roundtrip: green\n"
```

## test-dbase-roundtrip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3142):
Prerequisites: `build/test_dbase_roundtrip_mut build/.makefile.stamp`
```make
@printf ">>> test-dbase-roundtrip-mutant: confirming the un-masked-date mutant goes RED (Rule 6; bead 586.3)\n"
@$(TEST_DBASE_ROUNDTRIP_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbase-roundtrip-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBASE_ROUNDTRIP_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbase-roundtrip-mutant FAIL: mutant PASSED -- the normalization mask is decoration\n'; exit 1; \
else \
	printf '>>> test-dbase-roundtrip-mutant: green (un-masked date correctly RED)\n'; \
fi
```

## test-dbf-fields

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2461):
Prerequisites: `build/test_dbf_fields build/.makefile.stamp`
```make
@printf ">>> test-dbf-fields: .dbf III+ field-descriptor array (name/type/len/dec; name-to-NUL) (S1.2)\n"
@$(TEST_DBF_FIELDS) $(DBASE3_DECOMP)
@printf ">>> test-dbf-fields: green\n"
```

## test-dbf-fields-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2467):
Prerequisites: `build/test_dbf_fields_mut build/.makefile.stamp`
```make
@printf ">>> test-dbf-fields-mutant: confirming the 48-byte (dBASE-7) stride mutant goes RED (Rule 6; initech-aul.2)\n"
@$(TEST_DBF_FIELDS_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-fields-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_FIELDS_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-fields-mutant FAIL: mutant PASSED -- the descriptor stride is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-fields-mutant: green (48-byte stride correctly RED)\n'; \
fi
```

## test-dbf-header

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2432):
Prerequisites: `build/test_dbf_header build/.makefile.stamp`
```make
@printf ">>> test-dbf-header: .dbf III+ header parse + structural invariants 1/1b/2 (S1.1)\n"
@$(TEST_DBF_HEADER) $(DBASE3_DECOMP)
@printf ">>> test-dbf-header: green\n"
```

## test-dbf-header-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2438):
Prerequisites: `build/test_dbf_header_mut build/.makefile.stamp`
```make
@printf ">>> test-dbf-header-mutant: confirming the reclen off-by-one mutant goes RED (Rule 6; initech-aul.1)\n"
@$(TEST_DBF_HEADER_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-header-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_HEADER_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-header-mutant FAIL: mutant PASSED -- the invariant-1b reclen check is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-header-mutant: green (reclen off-by-one correctly RED)\n'; \
fi
```

## test-dbf-mutate

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2570):
Prerequisites: `build/test_dbf_mutate build/.makefile.stamp`
```make
@printf ">>> test-dbf-mutate: .dbf mutation verbs (append-blank/replace/delete/recall/pack/zap + assign-coerce) (S1.5)\n"
@$(TEST_DBF_MUTATE) $(DBASE3_DECOMP)
@printf ">>> test-dbf-mutate: green\n"
```

## test-dbf-mutate-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2576):
Prerequisites: `build/test_dbf_mutate_mut build/.makefile.stamp`
```make
@printf ">>> test-dbf-mutate-mutant: confirming the delete-flag (0x20-for-0x2A) mutant goes RED (Rule 6; initech-aul.5)\n"
@$(TEST_DBF_MUTATE_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-mutate-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_MUTATE_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-mutate-mutant FAIL: mutant PASSED -- the delete flag is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-mutate-mutant: green (delete-flag 0x20-for-0x2A correctly RED)\n'; \
fi
```

## test-dbf-read

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2498):
Prerequisites: `build/test_dbf_read build/.makefile.stamp`
```make
@printf ">>> test-dbf-read: .dbf III+ record read -> typed xb_val (C/N/D/L/M + delete flag) (S1.3)\n"
@$(TEST_DBF_READ) $(DBASE3_DECOMP)
@printf ">>> test-dbf-read: green\n"
```

## test-dbf-read-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2504):
Prerequisites: `build/test_dbf_read_mut build/.makefile.stamp`
```make
@printf ">>> test-dbf-read-mutant: confirming the record-offset +1 mutant goes RED (Rule 6; initech-aul.3)\n"
@$(TEST_DBF_READ_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-read-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_READ_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-read-mutant FAIL: mutant PASSED -- the record offset is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-read-mutant: green (record-offset +1 correctly RED)\n'; \
fi
```

## test-dbf-read-openrw-hdrlen-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2515):
Prerequisites: `build/test_dbf_read_mut_openrw_hdrlen build/.makefile.stamp`
```make
@printf ">>> test-dbf-read-openrw-hdrlen-mutant: confirming the eager +1-at-open-time mutant goes RED (Rule 6; initech-jf8p)\n"
@$(TEST_DBF_READ_MUT2) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-read-openrw-hdrlen-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_READ_MUT2) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-read-openrw-hdrlen-mutant FAIL: mutant PASSED -- the +2-form dbf_open_rw header_length fix is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-read-openrw-hdrlen-mutant: green (eager +1-at-open-time correctly RED)\n'; \
fi
```

## test-dbf-roundtrip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2540):
Prerequisites: `build/test_dbf_roundtrip build/.makefile.stamp`
```make
@printf ">>> test-dbf-roundtrip: .dbf deterministic write + bidirectional round-trip (C + dbf_ref.py + golden mask) (S1.4)\n"
@$(TEST_DBF_ROUNDTRIP) $(DBASE3_DECOMP)
@printf ">>> test-dbf-roundtrip: green\n"
```

## test-dbf-roundtrip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2546):
Prerequisites: `build/test_dbf_roundtrip_mut build/.makefile.stamp`
```make
@printf ">>> test-dbf-roundtrip-mutant: confirming the memo-version-bit mutant goes RED (Rule 6; initech-aul.4)\n"
@$(TEST_DBF_ROUNDTRIP_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbf-roundtrip-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBF_ROUNDTRIP_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbf-roundtrip-mutant FAIL: mutant PASSED -- the memo version bit is decoration\n'; exit 1; \
else \
	printf '>>> test-dbf-roundtrip-mutant: green (0x03-for-memo correctly RED)\n'; \
fi
```

## test-dbt-read

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2854):
Prerequisites: `build/test_dbt_read build/.makefile.stamp`
```make
@printf ">>> test-dbt-read: .dbt III+ memo open + read (512-byte blocks, LE ptr, 0x1A 0x1A term) (S2.1)\n"
@$(TEST_DBT_READ) $(DBASE3_DECOMP)
@printf ">>> test-dbt-read: green\n"
```

## test-dbt-read-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2860):
Prerequisites: `build/test_dbt_read_mut build/.makefile.stamp`
```make
@printf ">>> test-dbt-read-mutant: confirming the 511-byte-block mutant goes RED (Rule 6; initech-aul.6)\n"
@$(TEST_DBT_READ_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbt-read-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBT_READ_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbt-read-mutant FAIL: mutant PASSED -- the .dbt block geometry is decoration\n'; exit 1; \
else \
	printf '>>> test-dbt-read-mutant: green (511-byte block geometry correctly RED)\n'; \
fi
```

## test-dbt-roundtrip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2883):
Prerequisites: `build/test_dbt_roundtrip build/.makefile.stamp`
```make
@printf ">>> test-dbt-roundtrip: .dbt III+ write/append round-trip (dbt_create/dbt_append, ceil blocks, LE ptr) (S2.2)\n"
@$(TEST_DBT_ROUNDTRIP) $(DBASE3_DECOMP)
@printf ">>> test-dbt-roundtrip: green\n"
```

## test-dbt-roundtrip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2889):
Prerequisites: `build/test_dbt_roundtrip_mut build/.makefile.stamp`
```make
@printf ">>> test-dbt-roundtrip-mutant: confirming the BE next-free-ptr mutant goes RED (Rule 6; initech-aul.7)\n"
@$(TEST_DBT_ROUNDTRIP_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-dbt-roundtrip-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_DBT_ROUNDTRIP_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-dbt-roundtrip-mutant FAIL: mutant PASSED -- the LE next-free ptr is decoration\n'; exit 1; \
else \
	printf '>>> test-dbt-roundtrip-mutant: green (BE next-free ptr correctly RED)\n'; \
fi
```

## test-flair-samir-high-return

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 130):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/samir_list.img build/exec_high_legacy.exe build/.makefile.stamp`
```make
@cp -f $(SAMIR_LIST_IMG) $(BUILD)/samir_high_return_data.img
@mcopy -o -i $(BUILD)/samir_high_return_data.img $(BUILD)/exec_high_legacy.exe ::MZEXEC.EXE
@printf '@echo off\r\n' >$(BUILD)/samir_high_return_autoexec.bat
@mcopy -o -i $(BUILD)/samir_high_return_data.img $(BUILD)/samir_high_return_autoexec.bat ::AUTOEXEC.BAT
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(FLAIRTENANTS_IMG) --disk2 $(BUILD)/samir_high_return_data.img --name samir_high_return --out $(BUILD) --mouse '$(SAMIR_HIGH_RETURN_KEYS)' --keys-after FLAIR-LIVE-READY --quit-after MZEXEC-OK --timeout-ms 60000 2>$(BUILD)/samir_high_return.report || true
@# Real III+ at-enter/entered: Enter is CR only. The native prompt first
@# emits its 79 cleanup spaces; this oracle grades commands, not cell layout.
@tr $(if $(SAMIR_HIGH_RETURN_MUTATE_CR),-d '\r','\r' '\n') <$(BUILD)/samir_high_return.serial | sed 's/^ *//' >$(BUILD)/samir_high_return.repl
@! grep -q '^PANIC' $(BUILD)/samir_high_return.repl
@grep -q 'triple_fault=0' $(BUILD)/samir_high_return.report
@grep -qx 'FLAIR-RESUME' $(BUILD)/samir_high_return.repl
@grep -qx 'FLAIR-EXIT-TO-DOS' $(BUILD)/samir_high_return.repl
@n=$$(grep -c '^\. quit$$' $(BUILD)/samir_high_return.repl); \
 [ "$$n" -ge 3 ] || { printf 'RED test-flair-samir-high-return: CR-delimited quit count %s < 3\n' "$$n"; exit 1; }
@grep -qx 'MZEXEC-OK' $(BUILD)/samir_high_return.repl
@grep -q 'quit_after_found=1' $(BUILD)/samir_high_return.report
@printf '>>> test-flair-samir-high-return: green (hotkey/resume, Exit to InitechDOS, two high prompt launches/reclaims, legacy MZ return)\n'
```

## test-flair-samir-suspend

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25224):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/samir_list.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-samir-suspend : booted TEXT tenant (O-7)\n'
@printf '  Hotkey -> suspend desktop -> SAMIR full-screen (USE/LIST/QUIT) -> rebuild.\n'
@printf '  Ref: ADR-0013 Sec 3.2 (class-2 text tenant); spec/memory_map.h. Law 2/4.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (SAMIR.EXE + CLIENTS.DBF, primary slave)\n' "$(FLAIRTENANTS_IMG)" "$(SAMIR_LIST_IMG)"
@printf 'Expecting : FLAIR-SUSPEND + LIST rows {PESTON,WADDAMS,LUMBERGH} + FLAIR-RESUME + pre==post\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(SAMIR_LIST_IMG))
cp -f $(SAMIR_LIST_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(SAMIR_LIST_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(SAMIR_LIST_IMG))),:)
@# ---- PRE: the co-resident desktop before the hotkey (no keys). ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(SAMIR_SUSP_PRE_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--screendump --screendump-after "FLAIR-LIVE-READY" \
	2> "$(SAMIR_SUSP_PRE_REPORT)" || true
@# ---- POST: hotkey + SAMIR's USE/LIST/QUIT; dump after the rebuild. ----
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(SAMIR_SUSP_POST_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIR_SUSP_KEYS)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-RESUME" \
	2> "$(SAMIR_SUSP_POST_REPORT)" || true
@cat "$(SAMIR_SUSP_POST_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. No triple-fault either boot. ----
@if grep -q 'triple_fault=1' "$(SAMIR_SUSP_PRE_REPORT)" "$(SAMIR_SUSP_POST_REPORT)"; then \
	printf '!!! test-flair-samir-suspend FAIL: TRIPLE FAULT in a tenants boot\n'; exit 1; \
fi
@printf '>>> test-flair-samir-suspend [1/4]: no triple-fault (PRE + POST)\n'
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(SAMIR_SUSP_PRE_SERIAL)" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(SAMIR_SUSP_POST_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend FAIL: mount marker missing in PRE or POST\n'; exit 1; }
@# ---- 2. Suspend/resume markers + the SAMIR LIST rows on serial. ----
@grep -q '^FLAIR-SUSPEND$$' "$(SAMIR_SUSP_POST_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend FAIL: FLAIR-SUSPEND missing -- the hotkey did not launch SAMIR\n'; grep '^FLAIR-' "$(SAMIR_SUSP_POST_SERIAL)" || true; exit 1; }
@grep -q '^FLAIR-RESUME$$' "$(SAMIR_SUSP_POST_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend FAIL: FLAIR-RESUME missing -- the desktop never rebuilt after SAMIR\n'; exit 1; }
@# Scope the LIST-row assertions to AFTER FLAIR-SUSPEND (SAMIR's own output).
@tr -d '\r' < "$(SAMIR_SUSP_POST_SERIAL)" | sed -n '/FLAIR-SUSPEND/,$$p' > "$(BUILD)/$(SAMIR_SUSP_POST_NAME).samir"
@for tok in PESTON WADDAMS LUMBERGH; do \
	grep -qF "$$tok" "$(BUILD)/$(SAMIR_SUSP_POST_NAME).samir" \
	  || { printf '!!! test-flair-samir-suspend FAIL: LIST row token "%s" missing -- SAMIR USE/LIST did not render (root-cause the on-target dbf path, Rule 3)\n' "$$tok"; \
	       cat "$(BUILD)/$(SAMIR_SUSP_POST_NAME).samir"; exit 1; }; \
done
@printf '>>> test-flair-samir-suspend [2/4]: FLAIR-SUSPEND + LIST rows (PESTON/WADDAMS/LUMBERGH) + FLAIR-RESUME\n'
@# ---- 3. Both screendumps captured. ----
@if [ ! -s "$(SAMIR_SUSP_PRE_PPM)" ] || [ ! -s "$(SAMIR_SUSP_POST_PPM)" ]; then \
	printf '!!! test-flair-samir-suspend FAIL: a screendump is missing (PRE %s / POST %s)\n' "$(SAMIR_SUSP_PRE_PPM)" "$(SAMIR_SUSP_POST_PPM)"; exit 1; \
fi
@printf '>>> test-flair-samir-suspend [3/4]: PRE + POST screendumps captured\n'
@# ---- 4. BIT-IDENTITY: the desktop returned to its exact pre-suspend state. ----
@cmp -s "$(SAMIR_SUSP_PRE_PPM)" "$(SAMIR_SUSP_POST_PPM)" \
	|| { printf '!!! test-flair-samir-suspend FAIL: pre.ppm != post.ppm -- the desktop did NOT return BIT-IDENTICAL after SAMIR\n'; cmp "$(SAMIR_SUSP_PRE_PPM)" "$(SAMIR_SUSP_POST_PPM)" || true; exit 1; }
@printf '>>> test-flair-samir-suspend [4/4]: pre.ppm == post.ppm (desktop rebuilt BIT-IDENTICAL)\n'
@printf 'VERDICT   : PASS -- the system hotkey launched SAMIR full-screen (LISTed CLIENTS.DBF)\n'
@printf '            and the live FLAIR desktop returned BIT-IDENTICAL; O-7 holds (ADR-0013)\n'
@printf '            (QEMU; Bochs boot leg = make test-flair-samir-suspend-bochs)\n'
@printf '======================================================================\n'
@printf '>>> test-flair-samir-suspend: green\n'
```

## test-flair-samir-suspend-bochs

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 25337):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/samir_suspend_bochs_fallback.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-samir-suspend-bochs : BOCHS leg (O-7)\n'
@printf '  Bochs 2.7: VBE ENOMODE -> mode-0x13 (320x200); the 640x480 desktop cannot\n'
@printf '  fit, so kmain FAILS LOUD after MOUNT-OK but before SAMIR launch; the leg\n'
@printf '  proves the shared boot/heap/mount milestones + the guard. The\n'
@printf '  text-tenant suspend/rebuild is graded on QEMU by test-flair-samir-suspend.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@# DEC-03a.8 KEEP: every original fallback assertion remains. Modern
@# vgabios supports 0101h; the explicit negative-video fixture suppresses
@# that mode with the existing VBE_MUT_NO_101 hook. The same production
@# kernel/overlay run; test-exec-high-bochs separately proves native VBE/8 MiB.
@$(BOCHS_BIN) --disk "$(BUILD)/samir_suspend_bochs_fallback.img" --disk2 "$(FLAIR_GATE_DATA)" --expect HALTED \
	--name "$(SAMIR_SUSP_BOCHS_NAME)" --out "$(BUILD)" --timeout-ms 45000 \
	2> "$(SAMIR_SUSP_BOCHS_REPORT)" || true
@cat "$(SAMIR_SUSP_BOCHS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@grep -q 'rfb_unblocked=1' "$(SAMIR_SUSP_BOCHS_REPORT)" \
	|| { printf '!!! test-flair-samir-suspend-bochs FAIL: RFB unblock failed -- Bochs did not run the guest\n'; exit 1; }
@if grep -q 'triple_fault=1' "$(SAMIR_SUSP_BOCHS_REPORT)"; then \
	printf '!!! test-flair-samir-suspend-bochs FAIL: TRIPLE FAULT under Bochs\n'; exit 1; \
fi
@if [ ! -s "$(SAMIR_SUSP_BOCHS_SERIAL)" ]; then \
	printf '!!! test-flair-samir-suspend-bochs FAIL: no serial captured at %s\n' "$(SAMIR_SUSP_BOCHS_SERIAL)"; exit 1; \
fi
@for m in VBE-ENOMODE VGA13 S1 PM KERNEL CONSOLE BANNER FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK; do \
	grep -q "^$$m$$" "$(SAMIR_SUSP_BOCHS_SERIAL)" \
		|| { printf '!!! test-flair-samir-suspend-bochs FAIL: shared marker %s missing under Bochs\n' "$$m"; exit 1; }; \
done
@grep -q 'PANIC flair-desktop: LFB smaller than 640x480' "$(SAMIR_SUSP_BOCHS_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend-bochs FAIL: the 640x480-required guard did NOT fire under the 320x200 fallback\n'; exit 1; }
@grep -q '^HALTED$$' "$(SAMIR_SUSP_BOCHS_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend-bochs FAIL: HALTED terminal marker missing\n'; exit 1; }
@printf 'VERDICT   : PASS -- the flairtenants kernel booted under Bochs through the shared\n'
@printf '            kernel + FLAIR-heap milestones (== QEMU), and the 640x480 fail-loud\n'
@printf '            guard fired after MOUNT-OK but before SAMIR launch (no fault).\n'
@printf '======================================================================\n'
@printf '>>> test-flair-samir-suspend-bochs: green\n'
```

## test-flair-samir-suspend-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25295):
Prerequisites: `build/flair_tenants.img build/qemu_harness build/samir_list.img build/flair_tenants_mut_no_rebuild.img build/.makefile.stamp`
```make
@printf '>>> test-flair-samir-suspend-mutant: confirming the no-rebuild mutant goes RED (Rule 6)\n'
@$(call fixture-preflight,$(SAMIR_LIST_IMG))
cp -f $(SAMIR_LIST_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(SAMIR_LIST_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(SAMIR_LIST_IMG))),:)
@# Mint this gate's CLEAN pre baseline from the current real desktop.
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(SAMIR_SUSP_PRE_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--screendump --screendump-after "FLAIR-LIVE-READY" 2> "$(SAMIR_SUSP_PRE_REPORT)"
@if [ ! -s "$(SAMIR_SUSP_PRE_PPM)" ]; then printf '!!! test-flair-samir-suspend-mutant FAIL: clean PRE screendump missing (cannot establish the baseline)\n'; exit 1; fi
@# POST under the mutant image: same keys, dump after FLAIR-RESUME.
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_no_rebuild.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK \
	--name "$(SAMIR_SUSP_MUT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIR_SUSP_KEYS)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-RESUME" \
	2> "$(SAMIR_SUSP_MUT_REPORT)"
@sh harness/emu/require_trace.sh "$(SAMIR_SUSP_MUT_REPORT)" "$(SAMIR_SUSP_MUT_SERIAL)" "$$(printf '%s\n' '$(SAMIR_SUSP_KEYS)' | awk -F, '{print NF}')"
@if grep -q 'triple_fault=1' "$(SAMIR_SUSP_MUT_REPORT)"; then \
	printf '!!! test-flair-samir-suspend-mutant FAIL: the mutant TRIPLE-FAULTED -- a crash, not the wrong-pixels RED the gate asserts\n'; exit 1; \
fi
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(SAMIR_SUSP_MUT_SERIAL)" \
	|| { printf '!!! test-flair-samir-suspend-mutant FAIL: mount marker missing\n'; exit 1; }
@if [ ! -s "$(SAMIR_SUSP_MUT_PPM)" ]; then printf '!!! test-flair-samir-suspend-mutant FAIL: no mutant screendump captured (live guest required)\n'; exit 1; fi
@# The mutant skipped the rebuild, so its post MUST DIFFER from the clean pre.
@if cmp -s "$(SAMIR_SUSP_PRE_PPM)" "$(SAMIR_SUSP_MUT_PPM)"; then \
	printf '!!! test-flair-samir-suspend-mutant FAIL: pre.ppm == mutant post.ppm -- the bit-identity oracle is DECORATION (it did not bite the no-rebuild mutant)\n'; exit 1; \
fi
@printf '>>> test-flair-samir-suspend-mutant: green (no-rebuild mutant correctly RED -- pre != post, no crash)\n'
```

## test-interp-decimals

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3367):
Prerequisites: `build/test_interp_decimals build/.makefile.stamp`
```make
@printf ">>> test-interp-decimals: SET DECIMALS -> ?/?? computed-numeric display (division; 1/3->0.33) (7az.20)\n"
@$(TEST_INTERP_DECIMALS) $(DBASE3_DECOMP)
@printf ">>> test-interp-decimals: green\n"
```

## test-interp-decimals-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3373):
Prerequisites: `build/test_interp_decimals_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-decimals-mutant: confirming the ignore-SET-DECIMALS mutant goes RED (Rule 6; 7az.20)\n"
@$(TEST_INTERP_DECIMALS_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-decimals-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_DECIMALS_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-decimals-mutant FAIL: mutant PASSED -- the SET DECIMALS display wiring is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-decimals-mutant: green (ignore-SET-DECIMALS correctly RED)\n'; \
fi
```

## test-interp-do-file

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/do_file.mk', line 19):
Prerequisites: `build/test_interp_do_file build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/do_file_test.sh host $<
@printf '>>> test-interp-do-file: green\n'
```

## test-interp-do-file-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/do_file.mk', line 22):
Prerequisites: `build/SAMIR.EXE build/SAMIR.OVL build/tracer_secondary.img build/qemu_harness build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/do_file_test.sh emu
@printf '>>> test-interp-do-file-emu: green\n'
```

## test-interp-do-file-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/do_file.mk', line 25):
Prerequisites: `test-interp-do-file build/test_interp_do_file_skip build/test_interp_do_file_depth build/test_interp_do_file_master build/test_interp_do_file_close build/test_interp_do_file_active build/.makefile.stamp`
```make
@set -e; for mutant in skip depth master close active; do \
 log=$(BUILD)/do-file-test/$$mutant.log; \
 set +e; sh $(DBF_DIFF_DIR)/do_file_test.sh host $(BUILD)/test_interp_do_file_$$mutant > $$log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED DO-FILE' $$log || { cat $$log; exit 1; }; \
 printf '>>> DO-FILE mutant %s correctly RED: ' "$$mutant"; grep -m1 '^RED DO-FILE' $$log; \
 done
@printf '>>> test-interp-do-file-mutant: green\n'
```

## test-interp-flow

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3166):
Prerequisites: `build/test_interp_flow build/.makefile.stamp`
```make
@printf ">>> test-interp-flow: statement executor + DO WHILE/IF/DO CASE/LOOP/EXIT; guard-must-be-Logical (S5.3)\n"
@$(TEST_INTERP_FLOW) $(DBASE3_DECOMP)
@printf ">>> test-interp-flow: green\n"
```

## test-interp-flow-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3172):
Prerequisites: `build/test_interp_flow_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-flow-mutant: confirming the DO-CASE-runs-all mutant goes RED (Rule 6; initech-7az.4)\n"
@$(TEST_INTERP_FLOW_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-flow-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_FLOW_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-flow-mutant FAIL: mutant PASSED -- DO CASE first-match is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-flow-mutant: green (DO CASE runs-all correctly RED)\n'; \
fi
```

## test-interp-list

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3197):
Prerequisites: `build/test_interp_list build/.makefile.stamp`
```make
@printf ">>> test-interp-list: LIST/DISPLAY scope/FOR/WHILE/OFF; ?/??; LOCATE/CONTINUE; SEEK/FIND (S5.4)\n"
@$(TEST_INTERP_LIST) $(DBASE3_DECOMP)
@printf ">>> test-interp-list: green\n"
```

## test-interp-list-heading-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33907):
Prerequisites: `build/test_interp_screen_HEADING build/.makefile.stamp`
```make
@set +e; $< $(DBASE3_DECOMP) > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'screen:' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (implementation mutant RED: screen controls)\n' '$@'
```

## test-interp-list-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3203):
Prerequisites: `build/test_interp_list_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-list-mutant: confirming the CONTINUE-re-scope mutant goes RED (Rule 6; initech-7az.5)\n"
@$(TEST_INTERP_LIST_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-list-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_LIST_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-list-mutant FAIL: mutant PASSED -- CONTINUE FOR-only is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-list-mutant: green (CONTINUE re-scope correctly RED)\n'; \
fi
```

## test-interp-nav

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3105):
Prerequisites: `build/test_interp_nav build/.makefile.stamp`
```make
@printf ">>> test-interp-nav: GO/GOTO/SKIP/GO TOP-BOTTOM/EOF/BOF; physical + index order (S5.2)\n"
@$(TEST_INTERP_NAV) $(DBASE3_DECOMP)
@printf ">>> test-interp-nav: green\n"
```

## test-interp-nav-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3111):
Prerequisites: `build/test_interp_nav_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-nav-mutant: confirming the SKIP off-by-one mutant goes RED (Rule 6; initech-7az.3)\n"
@$(TEST_INTERP_NAV_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-nav-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_NAV_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-nav-mutant FAIL: mutant PASSED -- nav SKIP is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-nav-mutant: green (SKIP off-by-one correctly RED)\n'; \
fi
```

## test-interp-proc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3463):
Prerequisites: `build/test_interp_proc build/.makefile.stamp`
```make
@printf ">>> test-interp-proc: DO/PROC/PARAMS/RETURN/PUBLIC/PRIVATE/ACCEPT/INPUT/WAIT/ON ERROR + scope (S5.7)\n"
@$(TEST_INTERP_PROC) $(DBASE3_DECOMP)
@printf ">>> test-interp-proc: green\n"
```

## test-interp-proc-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3469):
Prerequisites: `build/test_interp_proc_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-proc-mutant: confirming the PRIVATE-no-restore mutant goes RED (Rule 6; initech-7az.8)\n"
@$(TEST_INTERP_PROC_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-proc-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_PROC_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-proc-mutant FAIL: mutant PASSED -- PRIVATE scope restore is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-proc-mutant: green (PRIVATE no-restore correctly RED)\n'; \
fi
```

## test-interp-replace

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3228):
Prerequisites: `build/test_interp_replace build/.makefile.stamp`
```make
@printf ">>> test-interp-replace: REPLACE/APPEND/DELETE/RECALL/PACK/ZAP; coercion + index drift (S5.5)\n"
@$(TEST_INTERP_REPLACE) $(DBASE3_DECOMP)
@printf ">>> test-interp-replace: green\n"
```

## test-interp-replace-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3234):
Prerequisites: `build/test_interp_replace_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-replace-mutant: confirming the REPLACE-ignores-scope mutant goes RED (Rule 6; initech-7az.6)\n"
@$(TEST_INTERP_REPLACE_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-replace-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_REPLACE_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-replace-mutant FAIL: mutant PASSED -- REPLACE scope is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-replace-mutant: green (REPLACE-ignores-scope correctly RED)\n'; \
fi
```

## test-interp-set

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3289):
Prerequisites: `build/test_interp_set build/.makefile.stamp`
```make
@printf ">>> test-interp-set: SET EXACT/DECIMALS/DATE/CENTURY/ORDER/... defaults + override (S5.6)\n"
@$(TEST_INTERP_SET) $(DBASE3_DECOMP)
@printf ">>> test-interp-set: green\n"
```

## test-interp-set-console-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33907):
Prerequisites: `build/test_interp_screen_CONSOLE build/.makefile.stamp`
```make
@set +e; $< $(DBASE3_DECOMP) > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'screen:' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (implementation mutant RED: screen controls)\n' '$@'
```

## test-interp-set-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3295):
Prerequisites: `build/test_interp_set_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-set-mutant: confirming the EXACT-default-ON mutant goes RED (Rule 6; initech-7az.7)\n"
@$(TEST_INTERP_SET_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-set-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_SET_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-set-mutant FAIL: mutant PASSED -- SET EXACT default is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-set-mutant: green (EXACT default-ON correctly RED)\n'; \
fi
```

## test-interp-setfmt

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3336):
Prerequisites: `build/test_interp_setfmt build/.makefile.stamp`
```make
@printf ">>> test-interp-setfmt: SET DATE/CENTURY -> DTOC/CTOD; STR ignores SET DECIMALS (verified) (S5.6/7az.15)\n"
@$(TEST_INTERP_SETFMT) $(DBASE3_DECOMP)
@printf ">>> test-interp-setfmt: green\n"
```

## test-interp-setfmt-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3342):
Prerequisites: `build/test_interp_setfmt_mut test-interp-setfmt-mutant-blankdtoc build/.makefile.stamp`
```make
@printf ">>> test-interp-setfmt-mutant: confirming the DTOC-ignores-SET-DATE/CENTURY mutant goes RED (Rule 6; 7az.15)\n"
@$(TEST_INTERP_SETFMT_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-setfmt-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_SETFMT_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-setfmt-mutant FAIL: mutant PASSED -- the SET DATE/CENTURY wiring is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-setfmt-mutant: green (DTOC-ignores-SET-DATE correctly RED)\n'; \
fi
```

## test-interp-setfmt-mutant-blankdtoc

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3933):
Prerequisites: `build/test_interp_setfmt_mut_blankdtoc build/.makefile.stamp`
```make
@log=$(BUILD)/blankdtoc-setfmt-mut.log; set +e; $< $(DBASE3_DECOMP) > $$log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'checks,' $$log && grep -q 'FAIL.*bracketed blank DTOC' $$log \
 || { cat $$log; printf '!!! space-only DTOC mutant did not fail its setfmt checks\n'; exit 1; }; \
 printf '>>> test-interp-setfmt-mutant-blankdtoc: green (space-only DTOC correctly RED): '; grep -m1 'FAIL.*bracketed blank DTOC' $$log
```

## test-interp-use

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3075):
Prerequisites: `build/test_interp_use build/.makefile.stamp`
```make
@printf ">>> test-interp-use: work-area model + USE/CLOSE/SELECT + field/memo resolve glue (S5.1)\n"
@$(TEST_INTERP_USE) $(DBASE3_DECOMP)
@printf ">>> test-interp-use: green\n"
```

## test-interp-use-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3081):
Prerequisites: `build/test_interp_use_mut build/.makefile.stamp`
```make
@printf ">>> test-interp-use-mutant: confirming the SELECT-misroute mutant goes RED (Rule 6; initech-7az.2)\n"
@$(TEST_INTERP_USE_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-interp-use-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_INTERP_USE_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-interp-use-mutant FAIL: mutant PASSED -- SELECT routing is decoration\n'; exit 1; \
else \
	printf '>>> test-interp-use-mutant: green (SELECT misroute correctly RED)\n'; \
fi
```

## test-merge-samir

Declared tier: outside active vector; #  recipe to execute (from 'harness/merge_samir.mk', line 19):
Prerequisites: `build/.makefile.stamp`
```make
@gates="$(MERGE_SAMIR_GATES)"; \
if $(MAKE) -n test-pubs-lockstep-src >/dev/null 2>&1; then gates="$$gates test-pubs-lockstep-src"; lock=yes; else lock=absent; fi; \
printf '>>> test-merge-samir: %s gates (test-pubs-lockstep-src: %s; no LaTeX)\n' "$$(echo $$gates | wc -w)" "$$lock"; \
printf '>>> declared omissions (Bochs per-session): %s\n' '$(MERGE_SAMIR_OMISSIONS)'; \
$(MAKE) -k $$gates && printf '>>> test-merge-samir: ALL GREEN\n'
```

## test-ndx-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2692):
Prerequisites: `build/test_ndx_build build/.makefile.stamp`
```make
@printf ">>> test-ndx-build: bulk INDEX ON build, byte-exact (pack 100%% L->R, remainder last; N keys/N+1 children) (S4.4)\n"
@$(TEST_NDX_BUILD) $(DBASE3_DECOMP)
@printf ">>> test-ndx-build: green\n"
```

## test-ndx-build-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2698):
Prerequisites: `build/test_ndx_build_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-build-mutant: 50/50 leaf-split mutant must go RED (Rule 6; initech-ahu.4)\n"
@$(TEST_NDX_BUILD_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-build-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_BUILD_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-build-mutant FAIL: mutant PASSED -- the leaf packing is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-build-mutant: green (50/50 split correctly RED)\n'; \
fi
```

## test-ndx-incremental

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2780):
Prerequisites: `build/test_ndx_incremental build/.makefile.stamp`
```make
@cd "$(DBASE3_DECOMP)/mint/work" && sha256sum --quiet -c "$(abspath $(DBF_DIFF_DIR)/ndx_incremental.sha256)"
@printf '>>> test-ndx-incremental: multi-level build/insert/REPLACE vs real III+ snapshot bytes (initech-7tj3)\n'
@$(TEST_NDX_INCR) "$(DBASE3_DECOMP)"
@printf '>>> test-ndx-incremental: green\n'
```

## test-ndx-incremental-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2785):
Prerequisites: `build/test_ndx_incremental_mut_LEAF_SPLIT_LOW build/test_ndx_incremental_mut_BULK_NO_ELIDE build/test_ndx_incremental_mut_DELETE_NO_SEP_UPDATE build/test_ndx_incremental_mut_ZERO_SEP_LEAF build/test_ndx_incremental_mut_DELETE_NOSPAN build/.makefile.stamp`
```make
@set -e; for pair in $(NDX_INCR_MUTANTS); do \
	m=$${pair%%:*}; why=$$(printf '%s' "$${pair#*:}" | tr '_' ' '); bin=$(BUILD)/test_ndx_incremental_mut_$$m; \
	set +e; $$bin "$(DBASE3_DECOMP)" > $$bin.log 2>&1; rc=$$?; set -e; \
	test $$rc -eq 1 && grep -Eq '^test-ndx-incremental: [0-9]+ checks, [1-9][0-9]* failures$$' $$bin.log \
	  || { tail -5 $$bin.log; printf '!!! incremental mutant %s did not fail with a test summary\n' "$$m"; exit 1; }; \
	grep -q "^FIRST RED $$why" $$bin.log \
	  || { grep 'FIRST RED' $$bin.log; printf '!!! incremental mutant %s: expected first RED %s\n' "$$m" "$$why"; exit 1; }; \
	printf '>>> NDX_MUTATE_%s RED: %s\n' "$$m" "$$(grep -m1 '^FIRST RED' $$bin.log)"; \
done
@printf '>>> test-ndx-incremental-mutant: green (five named mutants RED)\n'
```

## test-ndx-keys

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2629):
Prerequisites: `build/test_ndx_keys build/.makefile.stamp`
```make
@printf ">>> test-ndx-keys: .ndx key decode + collation (char CP437 byte-order; numeric arithmetic, NO sign-flip) (S4.2)\n"
@$(TEST_NDX_KEYS) $(DBASE3_DECOMP)
@printf ">>> test-ndx-keys: green\n"
```

## test-ndx-keys-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2635):
Prerequisites: `build/test_ndx_keys_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-keys-mutant: sign-flip transform (mint-001 disproved) must go RED (Rule 6; initech-ahu.2)\n"
@$(TEST_NDX_KEYS_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-keys-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_KEYS_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-keys-mutant FAIL: mutant PASSED -- numeric collation is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-keys-mutant: green (sign-flip correctly RED)\n'; \
fi
```

## test-ndx-maintain

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2814):
Prerequisites: `build/test_ndx_maintain build/.makefile.stamp`
```make
@printf ">>> test-ndx-maintain: .ndx incremental insert/update/delete keep SEEK + in-order correct (behavioral) (S4.5)\n"
@$(TEST_NDX_MAINTAIN) $(DBASE3_DECOMP)
@printf ">>> test-ndx-maintain: green\n"
```

## test-ndx-maintain-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2820):
Prerequisites: `build/test_ndx_maintain_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-maintain-mutant: confirming the no-sorted-insert mutant goes RED (Rule 6; initech-ahu.5)\n"
@$(TEST_NDX_MAINTAIN_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-maintain-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_MAINTAIN_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-maintain-mutant FAIL: mutant PASSED -- the sorted-insert invariant is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-maintain-mutant: green (no-sorted-insert correctly RED)\n'; \
fi
```

## test-ndx-maintain-span-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2831):
Prerequisites: `build/test_ndx_maintain_spanmut build/.makefile.stamp`
```make
@printf ">>> test-ndx-maintain-span-mutant: confirming the no-equal-key-run-walk mutant goes RED (Rule 6; initech-0g22)\n"
@$(TEST_NDX_MAINTAIN_SPANMUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-maintain-span-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_MAINTAIN_SPANMUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-maintain-span-mutant FAIL: mutant PASSED -- the equal-key-run delete walk is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-maintain-span-mutant: green (no-equal-key-run-walk correctly RED)\n'; \
fi
```

## test-ndx-multilevel

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2752):
Prerequisites: `build/test_ndx_multilevel build/.makefile.stamp`
```make
@printf ">>> test-ndx-multilevel: minted A00 shape, HIGH-key separators, SEEK, insert/delete (initech-h5vg/7tj3)\n"
@$(TEST_NDX_ML)
@printf ">>> test-ndx-multilevel: green\n"
```

## test-ndx-multilevel-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2758):
Prerequisites: `build/test_ndx_multilevel_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-multilevel-mutant: 2-level-ceiling revert (-DNDX_MUTATE_NO_ROOT_SPLIT) must go RED (Rule 6; initech-h5vg)\n"
@$(TEST_NDX_ML_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-multilevel-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_ML_MUT) >/dev/null 2>&1; then \
	printf '!!! test-ndx-multilevel-mutant FAIL: mutant PASSED -- multi-level build is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-multilevel-mutant: green (2-level ceiling revert correctly RED)\n'; \
fi
```

## test-ndx-pack

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3258):
Prerequisites: `build/test_ndx_pack build/.makefile.stamp`
```make
@printf ">>> test-ndx-pack: PACK full-reindexes every open .ndx; SEEK resolves renumbered survivors (S5.5)\n"
@$(TEST_NDX_PACK) $(DBASE3_DECOMP)
@printf ">>> test-ndx-pack: green\n"
```

## test-ndx-pack-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3264):
Prerequisites: `build/test_ndx_pack_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-pack-mutant: confirming the no-reindex-on-PACK mutant goes RED (Rule 6; initech-x87g)\n"
@$(TEST_NDX_PACK_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-pack-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_PACK_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-pack-mutant FAIL: mutant PASSED -- PACK reindex is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-pack-mutant: green (no-reindex-on-PACK correctly RED)\n'; \
fi
```

## test-ndx-parse

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2600):
Prerequisites: `build/test_ndx_parse build/.makefile.stamp`
```make
@printf ">>> test-ndx-parse: .ndx III+ header + node parse (10-field hdr, 2+2 node, verbatim key-expr) (S4.1)\n"
@$(TEST_NDX_PARSE) $(DBASE3_DECOMP)
@printf ">>> test-ndx-parse: green\n"
```

## test-ndx-parse-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2606):
Prerequisites: `build/test_ndx_parse_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-parse-mutant: confirming the group-sublayout-swap mutant goes RED (Rule 6; initech-ahu.1)\n"
@$(TEST_NDX_PARSE_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-parse-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_PARSE_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-parse-mutant FAIL: mutant PASSED -- the .ndx group sublayout is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-parse-mutant: green (group sublayout swap correctly RED)\n'; \
fi
```

## test-ndx-seek

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2659):
Prerequisites: `build/test_ndx_seek build/.makefile.stamp`
```make
@printf ">>> test-ndx-seek: .ndx B-tree in-order traverse + SEEK (first key >= target, rightmost child; SET EXACT begins-with) (S4.3)\n"
@$(TEST_NDX_SEEK) $(DBASE3_DECOMP)
@printf ">>> test-ndx-seek: green\n"
```

## test-ndx-seek-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2665):
Prerequisites: `build/test_ndx_seek_mut build/.makefile.stamp`
```make
@printf ">>> test-ndx-seek-mutant: wrong-child descent (off-by-one) must go RED (Rule 6; initech-ahu.3)\n"
@$(TEST_NDX_SEEK_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-ndx-seek-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_NDX_SEEK_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-ndx-seek-mutant FAIL: mutant PASSED -- the B-tree descent is decoration\n'; exit 1; \
else \
	printf '>>> test-ndx-seek-mutant: green (wrong-child descent correctly RED)\n'; \
fi
```

## test-pubs-samir-disk

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 383):
Prerequisites: `build/pubs/samir-disk/.minted build/.makefile.stamp`
```make
@set -e; while IFS='|' read -r name src; do case $$name in ''|'#'*) continue;; esac; \
	[ -f $(PUBS_SAMIR_DISK)/$$name ] || { printf '!!! test-pubs RED SAMPLE-DISK: %s missing\n' "$$name"; exit 1; }; \
done < $(PUBS_DIR)/examples/samir/disk/MANIFEST
@printf '>>> test-pubs-samir-disk: green (%s files on the Sample Programs and Utilities diskette)\n' "$$(ls $(PUBS_SAMIR_DISK) | wc -l)"
```

## test-pubs-samir-programs

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 519):
Prerequisites: `build/.makefile.stamp`
```make
@sh harness/pubs/samir_programs.sh $(PUBS_DIR)/examples/samir/disk $(CANON_DIR) $(PUBS_DIR)/manuals/initechbase-volume-two
```

## test-samir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3970):
Prerequisites: `test-samir-pal test-samir-rt test-samir-pal-host test-samir-spec test-samir-value test-samir-dbf-ref test-samir-ndx-ref test-samir-registries test-samir-registries-mutant test-samir-def-deps build/.makefile.stamp`
```make
@printf ">>> test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref)\n"
```

## test-samir-abbrev

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35056):
Prerequisites: `test-samir-dml-abbrev test-samir-dml-abbrscope build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(SAMIR_DML_TEST) abbrfull host
@for f in GOLD.OUT BASE.DBF NAMES.NDX VALUES.MEM; do cmp $(BUILD)/samirdml/golden/abbrev/$$f $(BUILD)/samirdml/golden/abbrfull/$$f || exit 1; done
@printf '>>> test-samir-abbrev: green (both spellings and replacement scopes vs real III+ output and files)\n'
```

## test-samir-abbrev-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35063):
Prerequisites: `test-samir-dml-abbrev-emu test-samir-dml-abbrscope-emu build/CLIENTS.DBF`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) abbrfull emu $(SAMIR_EXE)
@printf '>>> test-samir-abbrev-emu: green\n'
```

## test-samir-abbrev-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35060):
Prerequisites: `test-samir-abbrev test-samir-dml-abbrev-mutant test-samir-dml-abbrscope-mutant build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(BUILD)/test_samir_dml_abbrev abbrfull host
@printf '>>> test-samir-abbrev-mutant: green (DISP and replacement-scope mutants RED; full DISPLAY control green)\n'
```

## test-samir-append-safe

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3537):
Prerequisites: `build/test_samir_dbsafe build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@case "$@" in *append*) mode=append;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-append-safe-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3547):
Prerequisites: `build/test_samir_append_memory build/test_samir_append_count build/test_samir_append_rewrite build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@set -e; for kind in memory count rewrite; do \
  case $$kind in memory) reason='L001 in-memory committed count';; count) reason='L001 original header geometry and committed count';; *) reason='L001 prior record bytes untouched';; esac; \
  if $(BUILD)/test_samir_append_$$kind append > $(BUILD)/dbsafe/mut-$$kind.log 2>&1; then echo '!!! mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/dbsafe/mut-$$kind.log; grep -Fq "$$reason" $(BUILD)/dbsafe/mut-$$kind.log; \
  printf '>>> test-samir-append-safe-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-append-safe-mutant: green\n'
```

## test-samir-audit-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29181):
Prerequisites: `build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(TEST_SAMIR_AUDIT_EMU) view
@printf '>>> test-samir-audit-emu: green\n'
```

## test-samir-audit-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29198):
Prerequisites: `build/SAMIR_AUDIT_filter.EXE build/SAMIR_AUDIT_deleted.EXE build/SAMIR_AUDIT_marker.EXE build/SAMIR_AUDIT_extension.EXE build/SAMIR_AUDIT_useerror.EXE build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@set -e; for kind in filter deleted marker extension useerror; do \
  case $$kind in filter) reason='K05 active FILTER excludes negative row';; deleted) reason='K06 ON hides deleted row';; marker) reason='K06 audit DELETE+DISPLAY has asterisk';; extension) reason='K09 implicit extension succeeded before explicit control';; useerror) reason='K09 missing table is';; esac; \
  if $(TEST_SAMIR_AUDIT_EMU) view $(BUILD)/SAMIR_AUDIT_$$kind.EXE > $(BUILD)/samir-audit/mut-emu-$$kind.log 2>&1; then echo '!!! emu mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-emu-$$kind.log; \
  grep -Fq "$$reason" $(BUILD)/samir-audit/mut-emu-$$kind.log; \
  grep -q 'triple_fault=0' $(BUILD)/samir-audit/mut-emu-$$kind.log; \
  grep -q 'quit_after_found=1' $(BUILD)/samir-audit/mut-emu-$$kind.log; \
  printf '>>> test-samir-audit-emu-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-audit-emu-mutant: green\n'
```

## test-samir-bochs-keys

Declared tier: HOST; #  recipe to execute (from 'harness/samir_boot_bochs.mk', line 17):
Prerequisites: `build/test_samir_bochs_keys build/.makefile.stamp`
```make
@$<
```

## test-samir-bochs-keys-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/samir_boot_bochs.mk', line 19):
Prerequisites: `test-samir-bochs-keys build/test_samir_bochs_keys_mut build/.makefile.stamp`
```make
@if $(BUILD)/test_samir_bochs_keys_mut > $(BUILD)/samir-bochs-keys-mutant.log 2>&1; then printf '!!! %s FAIL: Return mutant survived\n' "$@"; exit 1; fi
@grep -qxF 'RED samir-bochs-keys: Return keysym must be ff0d' $(BUILD)/samir-bochs-keys-mutant.log || { cat $(BUILD)/samir-bochs-keys-mutant.log; exit 1; }
@cat $(BUILD)/samir-bochs-keys-mutant.log
@printf '>>> test-samir-bochs-keys-mutant: green (Return-to-BackSpace branch correctly RED)\n'
```

## test-samir-boot

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29337):
Prerequisites: `build/qemu_harness build/samir_boot_capture.img build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/samir_boot_stream samir-use-goldens build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-boot : SAMIR runs INSIDE InitechOS (S8.2)\n'
@printf '  Ref: bead initech-hdlb; ADR-0009 DEC-08. boot -> EXEC SAMIR.EXE -> USE -> LIST.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (SAMIR.EXE + CLIENTS.DBF, primary slave)\n' "$(SECONDARY_TRACER_IMG)" "$(SAMIR_LIST_IMG)"
@printf 'Expecting : SHELL-READY + dot prompt + LIST rows {PESTON,WADDAMS,LUMBERGH} + no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# Run 1 (serial): EXEC SAMIR, USE CLIENTS.DBF, LIST, QUIT, EXIT. Generous
@# timeout: SAMIR is 77 KiB soft-float -- slower to load + construct than a
@# baked .COM. No screendump here (Run 2 grabs it on its own clean keys run).
@$(call fixture-preflight,$(SAMIR_LIST_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_LIST_IMG)" \
	--name "$(SAMIRBOOT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRBOOT_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SAMIRBOOT_REPORT)" || true
@cat "$(SAMIRBOOT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SAMIRBOOT_REPORT)"; then \
	printf '!!! test-samir-boot FAIL: TRIPLE FAULT -- SAMIR crashed (arena panic / bad int 0x21)\n'; \
	exit 1; \
fi
@printf '>>> test-samir-boot [1/5]: no triple-fault (SAMIR ran without crashing)\n'
@if [ ! -s "$(SAMIRBOOT_SERIAL)" ]; then \
	printf '!!! test-samir-boot FAIL: no serial captured at %s\n' "$(SAMIRBOOT_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(SAMIRBOOT_SERIAL)" \
	|| { printf '!!! test-samir-boot FAIL: SHELL-READY missing -- COMMAND.COM never reached the prompt\n'; exit 1; }
@printf '>>> test-samir-boot [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to AFTER `A:\>samir` so the assertions bite SAMIR's OWN output, not any
@# earlier boot text (the FAT dir listing also prints CLIENTS.DBF).
@tr -d '\r' < "$(SAMIRBOOT_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(SAMIRBOOT_NAME).samir"
@grep -q '^\. ' "$(BUILD)/$(SAMIRBOOT_NAME).samir" \
	|| { printf '!!! test-samir-boot FAIL: the SAMIR dot prompt ". " never appeared (SAMIR REPL did not start)\n'; \
	     cat "$(BUILD)/$(SAMIRBOOT_NAME).samir"; exit 1; }
@printf '>>> test-samir-boot [3/5]: SAMIR EXEC ran and the dBASE dot prompt appeared\n'
@# ---- the LIST rows: every fixture record value reaches serial. ----
@for tok in PESTON HONOLULU 1234.50 WADDAMS AUSTIN LUMBERGH DALLAS 7777.77; do \
	grep -qF "$$tok" "$(BUILD)/$(SAMIRBOOT_NAME).samir" \
	  || { printf '!!! test-samir-boot FAIL: LIST row token "%s" missing -- USE/LIST did not render the .dbf record (root-cause the on-target dbf read path, Rule 3)\n' "$$tok"; \
	       cat "$(BUILD)/$(SAMIRBOOT_NAME).samir"; exit 1; }; \
done
@printf '>>> test-samir-boot [4/5]: LIST rendered all 3 records on serial (PESTON / WADDAMS / LUMBERGH)\n'
@$(BUILD)/samir_boot_stream $(DBASE3_DECOMP)/mint/work/M005ULOG.TXT "$(SAMIRBOOT_SERIAL)"
@# initech-31d1 re-key: M005U/M005N's 7+2 prefix and unpadded final
@# heading, projected onto the existing CLIENTS value fixture.
@grep -Fxq 'Record#  NAME       CITY              BAL' "$(BUILD)/$(SAMIRBOOT_NAME).samir"
@grep -Fxq '      1  PESTON     HONOLULU      1234.50' "$(BUILD)/$(SAMIRBOOT_NAME).samir"
@grep -Fxq '      2  WADDAMS    AUSTIN         -42.00' "$(BUILD)/$(SAMIRBOOT_NAME).samir"
@grep -Fxq '      3  LUMBERGH   DALLAS        7777.77' "$(BUILD)/$(SAMIRBOOT_NAME).samir"
@# ---- 5. SCREENDUMP (Run 2; Law 4): a SEPARATE keys+grab run leaves SAMIR's
@# dot prompt + LIST table on the LFB. screendump-after SHELL-READY syncs the
@# grab AFTER the harness injects the keys (qemu.c qmp_session: keys then dump),
@# so the LIST rows are on screen. ppm_text_check grades the exact real
@# table cells below, including blanks where a missing/wrong row must fail.
@$(call fixture-preflight,$(SAMIR_LIST_IMG))
@$(HARNESS_BIN) --disk "$(SAMIRBOOT_CAPTURE_IMG)" --disk2 "$(SAMIR_LIST_IMG)" \
	--name "$(SAMIRBOOT_SCRN_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRBOOT_SCRN_KEYS)" --keys-after "SHELL-READY" \
	--screendump --screendump-after "$$(printf '7777.77\r\n. ')" \
	2> "$(SAMIRBOOT_SCRN_REPORT)" || true
@if grep -q 'triple_fault=1' "$(SAMIRBOOT_SCRN_REPORT)"; then \
	printf '!!! test-samir-boot FAIL: TRIPLE FAULT on the screendump run\n'; exit 1; \
fi
@if [ ! -s "$(SAMIRBOOT_PPM)" ]; then \
	printf '!!! test-samir-boot FAIL: no screendump captured at %s (live guest required)\n' "$(SAMIRBOOT_PPM)"; exit 1; \
fi
@# Real III+ boot-list-real/list (initech-0spyf/fzsvo): heading row 19,
@# records 20..22. Project exact ink AND blank cells from the real capture.
@cmp $(CLIENTS_DBF) $(SCREEN_MINT_DIR)/boot-list-real/SETUP/CLIENTS.DBF
@sed -n '22,25p' $(SCREEN_MINT_DIR)/boot-list-real/list.screen | \
	awk '{printf "%d 0 %s\n", NR+18, $$0}' > $(BUILD)/samir-boot.cells
@$(PPM_TEXT_CHECK_BIN) --cells "$(SAMIRBOOT_PPM)" $(BUILD)/samir-boot.cells
@printf '>>> test-samir-boot [5/5]: screendump shows SAMIR'"'"'s dot prompt + LIST table on the seafoam desktop\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechBase (SAMIR) booted inside InitechOS, opened a .dbf and LISTed it\n'
@printf '            (QEMU only; tri-emulator agreement pending bead initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-samir-boot: green (production serial; steady capture; real LIST cells)\n'
```

## test-samir-boot-bochs

Declared tier: outside active vector; #  recipe to execute (from 'harness/samir_boot_bochs.mk', line 31):
Prerequisites: `test-samir-boot test-samir-bochs-keys build/bochs_harness build/ppm_bochs_check build/samir_boot_capture.img build/samir_list.img build/samir_list.img build/.makefile.stamp`
```make
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! %s FAIL: bochs not found (apt install bochs)\n' "$@"; exit 1; }
@# The shared host has one RFB port. Poll before EVERY invocation; the
@# driver also holds its existing host lock through reaping (WL-0170).
@tries=0; while pgrep -x bochs >/dev/null; do \
 if [ "$$tries" -ge 60 ]; then printf '!!! %s FAIL: Bochs busy after 30 minutes\n' "$@"; exit 1; fi; \
 printf 'Waiting for the shared Bochs process (30s poll)\n'; sleep 30; tries=$$((tries+1)); \
done
@$(call fixture-preflight,$(SAMIR_BOCHS_SOURCE))
cp -f $(SAMIR_BOCHS_SOURCE) $(SAMIR_BOCHS_DATA)
@$(BOCHS_BIN) --disk "$(SAMIRBOOT_CAPTURE_IMG)" --disk2 "$(SAMIR_BOCHS_DATA)" \
 --expect SHELL-READY --quit-after SHELL-READY \
 --keys "$(SAMIRBOOT_SCRN_KEYS)" --keys-after SHELL-READY \
 --screendump-after SHELL-READY \
 --name "$(SAMIR_BOCHS_NAME)" --out "$(BUILD)" --timeout-ms 120000 \
 2> "$(SAMIR_BOCHS_REPORT)" || { cat "$(SAMIR_BOCHS_REPORT)"; exit 1; }
@cat "$(SAMIR_BOCHS_REPORT)"
@sh harness/emu/bochs_leg_check.sh test-samir-boot-bochs "$(SAMIR_BOCHS_REPORT)" "$(SAMIR_BOCHS_SERIAL)" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
@rc=0; sh harness/emu/samir_boot_check.sh $@ "$(SAMIR_BOCHS_SERIAL)" || rc=1; \
 $(BUILD)/ppm_bochs_check "$(SAMIRBOOT_PPM)" "$(BUILD)/$(SAMIR_BOCHS_NAME).snap.ppm" "$(BUILD)/$(SAMIR_BOCHS_NAME).rfb.pgm" || rc=1; \
 exit $$rc
@printf '>>> test-samir-boot-bochs: green (USE/LIST serial + framebuffer == QEMU golden, pixel-exact + displayed)\n'
```

## test-samir-boot-bochs-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/samir_boot_bochs.mk', line 57):
Prerequisites: `test-samir-boot-bochs build/samir_list_mut.img build/.makefile.stamp`
```make
@$(MAKE) --no-print-directory test-samir-boot-bochs SAMIR_BOCHS_SOURCE=$(SAMIR_LIST_MUT_IMG) SAMIR_BOCHS_NAME=samir_boot_bochs_mut > $(BUILD)/samir-bochs-mutant.log 2>&1; \
 rc=$$?; test "$$rc" -ne 0 || { cat $(BUILD)/samir-bochs-mutant.log; printf '!!! %s FAIL: short-read mutant survived\n' "$@"; exit 1; }
@grep -q '^RED test-samir-boot-bochs serial: LIST row missing:' $(BUILD)/samir-bochs-mutant.log \
 && grep -q '^RED bochs-exact RGB ' $(BUILD)/samir-bochs-mutant.log \
 && grep -q 'triple_fault=0' $(BUILD)/samir_boot_bochs_mut.report \
 && grep -q 'snapshot_ok=1' $(BUILD)/samir_boot_bochs_mut.report \
 || { cat $(BUILD)/samir-bochs-mutant.log; printf '!!! %s FAIL: missing wrong-data serial/pixel RED or failed capture\n' "$@"; exit 1; }
@grep '^RED ' $(BUILD)/samir-bochs-mutant.log
@printf '>>> test-samir-boot-bochs-mutant: green (short-read mutant RED on exact LIST rows AND pixels; no crash)\n'
```

## test-samir-boot-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29418):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/samir_list_mut.img build/.makefile.stamp`
```make
@printf '>>> test-samir-boot-mutant: confirming the short-read pal_milton mutant goes RED (Rule 6)\n'
@$(call fixture-preflight,$(SAMIR_LIST_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_LIST_MUT_IMG)" \
	--name "$(SAMIRBOOT_MUT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRBOOT_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SAMIRBOOT_MUT_REPORT)"
@sh harness/emu/require_trace.sh "$(SAMIRBOOT_MUT_REPORT)" "$(SAMIRBOOT_MUT_SERIAL)" "$$(printf '%s\n' '$(SAMIRBOOT_KEYS)' | awk -F, '{print NF}')"
@grep -Fq '. use clients.dbf' "$(SAMIRBOOT_MUT_SERIAL)"
@grep -Eq '15  Not a dBASE database[.]' "$(SAMIRBOOT_MUT_SERIAL)"
@! grep -q 'Insert System Disk' "$(SAMIRBOOT_MUT_SERIAL)"
@# The mutant must NOT triple-fault (it is a DATA bug, not a crash) ...
@if grep -q 'triple_fault=1' "$(SAMIRBOOT_MUT_REPORT)"; then \
	printf '!!! test-samir-boot-mutant FAIL: the mutant TRIPLE-FAULTED -- a crash, not the wrong-data RED the gate asserts\n'; exit 1; \
fi
@# ... and the LIST rows MUST be absent (the short read mis-decodes the .dbf).
@if tr -d '\r' < "$(SAMIRBOOT_MUT_SERIAL)" 2>/dev/null | sed -n '/A:.>samir/,$$p' | grep -qF 'PESTON'; then \
	printf '!!! test-samir-boot-mutant FAIL: PESTON present under the short-read mutant -- the gate is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-samir-boot-mutant: green (short-read mutant correctly RED -- LIST rows garbled/absent, no crash)\n'
```

## test-samir-boot-stream-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29443):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/samir_list.img build/samir_boot_stream build/SAMIR_SCREEN_SAMIR_LIST_MUTATE_DOT_OPENING.EXE samir-use-goldens build/.makefile.stamp`
```make
@cp -f $(SAMIR_LIST_IMG) $(BUILD)/samir_boot_stream_mut.img
@mcopy -o -i $(BUILD)/samir_boot_stream_mut.img $(BUILD)/SAMIR_SCREEN_SAMIR_LIST_MUTATE_DOT_OPENING.EXE ::SAMIR.EXE
@mcopy -o -i $(BUILD)/samir_boot_stream_mut.img $(BUILD)/SAMIR_SCREEN_SAMIR_LIST_MUTATE_DOT_OPENING.OVL ::SAMIR.OVL
@$(HARNESS_BIN) --disk $(SECONDARY_TRACER_IMG) --disk2 $(BUILD)/samir_boot_stream_mut.img \
	--name samir_boot_stream_mut --out $(BUILD) --timeout-ms 60000 \
	--keys "$(SAMIRBOOT_KEYS)" --keys-after SHELL-READY $(SHELL_OS_COMPLETION) \
	> $(BUILD)/samir_boot_stream_mut.out 2> $(BUILD)/samir_boot_stream_mut.report
@sh harness/emu/require_trace.sh $(BUILD)/samir_boot_stream_mut.report $(BUILD)/samir_boot_stream_mut.serial "$$(printf '%s\n' '$(SAMIRBOOT_KEYS)' | awk -F, '{print NF}')"
@rc=0; $(BUILD)/samir_boot_stream $(DBASE3_DECOMP)/mint/work/M005ULOG.TXT $(BUILD)/samir_boot_stream_mut.serial > $(BUILD)/samir_boot_stream_mut.log || rc=$$?; \
	test $$rc -eq 1 && grep -q '^RED boot LIST opening after Enter: differs at byte 0' $(BUILD)/samir_boot_stream_mut.log || { cat $(BUILD)/samir_boot_stream_mut.log; exit 1; }
@cat $(BUILD)/samir_boot_stream_mut.log
@printf '>>> test-samir-boot-stream-mutant: green (dot LIST opening CR/LF removed, raw serial RED; clean guest return)\n'
```

## test-samir-canon-salami

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30049):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/INVOICE.DBF build/SALAMI.PRG build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-canon-salami : Bolton salami virus RUNS inside InitechOS\n'
@printf '  Ref: bead initech-4hte (canon pair, salami twin of 9a0f); 586.2 (the enforced skim). boot -> EXEC SAMIR -> USE INVOICE -> DO SALAMI.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (SAMIR.EXE + INVOICE.DBF + SALAMI.PRG)\n' "$(SECONDARY_TRACER_IMG)" "$(CSAL_IMG)"
@printf 'Expecting : the posting report on serial WITH the buggy A1004 0.4998 / BOLTON 0.38 (too much too fast, canon)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call csalami_mint,$(CSAL_IMG),$(SAMIR_EXE))
@printf '>>> samir_canon_salami: minted FRESH %s (SAMIR.EXE + INVOICE.DBF + SALAMI.PRG)\n' "$(CSAL_IMG)"
@# Run 1 (serial): EXEC SAMIR, USE INVOICE, DO SALAMI, QUIT, EXIT.
@$(call fixture-preflight,$(CSAL_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CSAL_IMG)" \
	--name "$(CSAL_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CSAL_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(CSAL_REPORT)" || true
@cat "$(CSAL_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(CSAL_REPORT)"; then \
	printf '!!! test-samir-canon-salami FAIL: TRIPLE FAULT -- the DO/posting path crashed (root-cause pal_milton DO-read or flow, Rule 3)\n'; \
	exit 1; \
fi
@printf '>>> test-samir-canon-salami [1/4]: no triple-fault (the salami routine ran without crashing)\n'
@if [ ! -s "$(CSAL_SERIAL)" ]; then \
	printf '!!! test-samir-canon-salami FAIL: no serial captured at %s\n' "$(CSAL_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(CSAL_SERIAL)" \
	|| { printf '!!! test-samir-canon-salami FAIL: SHELL-READY missing -- COMMAND.COM never reached the prompt\n'; exit 1; }
@printf '>>> test-samir-canon-salami [2/4]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to AFTER `A:\>samir` so the assertions bite SAMIR's OWN output.
@tr -d '\r' < "$(CSAL_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(CSAL_NAME).samir"
@grep -qF 'INITECH SYSTEMS CORP -- ACCOUNTS RECEIVABLE' "$(BUILD)/$(CSAL_NAME).samir" \
	|| { printf '!!! test-samir-canon-salami FAIL: the posting report banner is absent -- DO SALAMI did not run the .prg (root-cause repl_try_do_file / pal_milton DO-read)\n'; \
	     cat "$(BUILD)/$(CSAL_NAME).samir"; exit 1; }
@# ---- THE CANON TOO-MUCH-TOO-FAST SKIM VALUES (the SAME the host gate test-canon-salami asserts): ----
@grep -qF 'A1004      1.4998        1.00      0.4998' "$(BUILD)/$(CSAL_NAME).samir" \
	|| { printf '!!! test-samir-canon-salami FAIL: the buggy A1004 ... 0.4998 posting line is absent -- the dollars-scale skim did not run in-emu (Law 4)\n'; \
	     cat "$(BUILD)/$(CSAL_NAME).samir"; exit 1; }
@grep -qF 'BOLTON SUSPENSE ACCOUNT:         0.38' "$(BUILD)/$(CSAL_NAME).samir" \
	|| { printf '!!! test-samir-canon-salami FAIL: the headline BOLTON 0.38 balloon is absent -- the too-much-too-fast skim did not accumulate in-emu (Law 4)\n'; \
	     cat "$(BUILD)/$(CSAL_NAME).samir"; exit 1; }
@printf '>>> test-samir-canon-salami [3/4]: SERIAL shows the finance-charge posting report WITH the enforced too-much-too-fast skim (A1004 0.4998 / BOLTON 0.38)\n'
@# ---- 4. SCREENDUMP (Run 2; Law 4): the report on the LFB. The DO SALAMI run
@# prints ~10 report lines; they scroll the active band lower than the 3-row LIST
@# gate, so the report text lands in band [176,260). seafoam right margin checked
@# (immune to vertical scroll), same discriminator pattern as test-samir-canon-y2k. ----
@$(call csalami_mint,$(CSAL_SCRN_IMG),$(SAMIR_EXE))
@$(call fixture-preflight,$(CSAL_SCRN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CSAL_SCRN_IMG)" \
	--name "$(CSAL_SCRN_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CSAL_SCRN_KEYS)" --keys-after "SHELL-READY" \
	--screendump --screendump-after 'BOLTON SUSPENSE ACCOUNT:         0.38. ' \
	2> "$(CSAL_SCRN_REPORT)" || true
@if grep -q 'triple_fault=1' "$(CSAL_SCRN_REPORT)"; then \
	printf '!!! test-samir-canon-salami FAIL: TRIPLE FAULT on the screendump run\n'; exit 1; \
fi
@if [ ! -s "$(CSAL_PPM)" ]; then \
	printf '!!! test-samir-canon-salami FAIL: no screendump captured at %s (live guest required)\n' "$(CSAL_PPM)"; exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(CSAL_PPM)" 176 260 300 32 560 \
	|| { printf '!!! test-samir-canon-salami FAIL: the posting report did not render on the framebuffer (band [176,260) < 300 fg), or the right-margin desktop is not seafoam\n'; exit 1; }
@printf '>>> test-samir-canon-salami [4/4]: screendump shows the finance-charge posting report on the seafoam desktop\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- Bolton'\''s salami-slicing rounding virus RAN inside InitechOS via DO SALAMI,\n'
@printf '            and its ENFORCED misplaced-decimal skim (586.2) produced the canonical "too much\n'
@printf '            too fast" balloon (A1004 sweeps 0.4998 dollars-scale; BOLTON suspense 0.38).\n'
@printf '            (QEMU only; tri-emulator agreement pending bead initech-x0i)\n'
@printf '======================================================================\n'
```

## test-samir-canon-salami-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 30128):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR_DOTRUNC.EXE build/INVOICE.DBF build/SALAMI.PRG build/.makefile.stamp`
```make
@printf '>>> test-samir-canon-salami-mutant: confirming the DO-file half-read mutant breaks canon in-emu -> RED (Rule 6; initech-4hte)\n'
@$(call csalami_mint,$(CSAL_MUT_IMG),$(SAMIR_EXE_DOTRUNC))
@$(call fixture-preflight,$(CSAL_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CSAL_MUT_IMG)" \
	--name "$(CSAL_MUT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CSAL_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(CSAL_MUT_REPORT)"
@sh harness/emu/require_trace.sh "$(CSAL_MUT_REPORT)" "$(CSAL_MUT_SERIAL)" "$$(printf '%s\n' '$(CSAL_KEYS)' | awk -F, '{print NF}')"
@# The mutant must NOT triple-fault (it is a wrong-data bug, not a crash).
@if grep -q 'triple_fault=1' "$(CSAL_MUT_REPORT)"; then \
	printf '!!! test-samir-canon-salami-mutant FAIL: the mutant TRIPLE-FAULTED -- a crash, not the wrong-data RED the gate asserts\n'; exit 1; \
fi
@if [ ! -s "$(CSAL_MUT_SERIAL)" ]; then \
	printf '!!! test-samir-canon-salami-mutant FAIL: no serial captured (the mutant must still RUN, just read a partial .prg)\n'; exit 1; \
fi
@tr -d '\r' < "$(CSAL_MUT_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(CSAL_MUT_NAME).samir"
@# With the .prg read truncated, the canon report BODY is cut off: the buggy
@# A1004 0.4998 posting line and the BOLTON 0.38 total MUST be absent -> the canon
@# assertions would fail. If they are STILL present, the mutant did not bite (decoration).
@if grep -qF 'A1004      1.4998        1.00      0.4998' "$(BUILD)/$(CSAL_MUT_NAME).samir"; then \
	printf '!!! test-samir-canon-salami-mutant FAIL: the buggy A1004 0.4998 line is STILL present under the DO-truncation mutant -- it did not bite (gate is decoration)\n'; \
	cat "$(BUILD)/$(CSAL_MUT_NAME).samir"; exit 1; \
fi
@if grep -qF 'BOLTON SUSPENSE ACCOUNT:         0.38' "$(BUILD)/$(CSAL_MUT_NAME).samir"; then \
	printf '!!! test-samir-canon-salami-mutant FAIL: the headline BOLTON 0.38 balloon is STILL present under the DO-truncation mutant -- it did not bite\n'; \
	cat "$(BUILD)/$(CSAL_MUT_NAME).samir"; exit 1; \
fi
@printf '    --- the TRUNCATED (mutant) in-emu transcript (the .prg body was cut off) ---\n'
@sed 's/^/    /' "$(BUILD)/$(CSAL_MUT_NAME).samir"
@printf '>>> test-samir-canon-salami-mutant: green (the DO-file half-read mutant correctly RED -- the canon posting body is absent; no crash)\n'
```

## test-samir-canon-y2k

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29831):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/INVOICE.DBF build/Y2KACCT.PRG build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-canon-y2k : the Initech AR app + its Y2K bug RUN inside InitechOS\n'
@printf '  Ref: bead initech-9a0f (Law-4 capstone); 586.1 (the enforced Y2K bug). boot -> EXEC SAMIR -> DO Y2KACCT.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (SAMIR.EXE + INVOICE.DBF + Y2KACCT.PRG)\n' "$(SECONDARY_TRACER_IMG)" "$(CY2K_IMG)"
@printf 'Expecting : the aging report on serial WITH the buggy A1001 -36477 / A1003 -36462 / TOTAL 0.00 (canon)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call cy2k_mint,$(CY2K_IMG),$(SAMIR_EXE),$(INVOICE_DBF))
@printf '>>> samir_canon_y2k: minted FRESH %s (SAMIR.EXE + INVOICE.DBF + Y2KACCT.PRG)\n' "$(CY2K_IMG)"
@# Run 1 (serial): EXEC SAMIR, DO Y2KACCT, QUIT, EXIT.
@$(call fixture-preflight,$(CY2K_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CY2K_IMG)" \
	--name "$(CY2K_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CY2K_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(CY2K_REPORT)" || true
@cat "$(CY2K_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(CY2K_REPORT)"; then \
	printf '!!! test-samir-canon-y2k FAIL: TRIPLE FAULT -- the DO/aging path crashed (root-cause pal_milton DO-read or flow, Rule 3)\n'; \
	exit 1; \
fi
@printf '>>> test-samir-canon-y2k [1/4]: no triple-fault (the AR app ran without crashing)\n'
@if [ ! -s "$(CY2K_SERIAL)" ]; then \
	printf '!!! test-samir-canon-y2k FAIL: no serial captured at %s\n' "$(CY2K_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(CY2K_SERIAL)" \
	|| { printf '!!! test-samir-canon-y2k FAIL: SHELL-READY missing -- COMMAND.COM never reached the prompt\n'; exit 1; }
@printf '>>> test-samir-canon-y2k [2/4]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to AFTER `A:\>samir` so the assertions bite SAMIR's OWN output.
@tr -d '\r' < "$(CY2K_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(CY2K_NAME).samir"
@grep -qF 'INITECH SYSTEMS CORP -- ACCOUNTS RECEIVABLE' "$(BUILD)/$(CY2K_NAME).samir" \
	|| { printf '!!! test-samir-canon-y2k FAIL: the AR report banner is absent -- DO Y2KACCT did not run the .prg (root-cause repl_try_do_file / pal_milton DO-read)\n'; \
	     cat "$(BUILD)/$(CY2K_NAME).samir"; exit 1; }
@# ---- THE CANON Y2K-BUGGY VALUES (the SAME the host gate test-canon-y2k asserts): ----
@grep -qF 'A1001  12/15/99  -36477   CURRENT' "$(BUILD)/$(CY2K_NAME).samir" \
	|| { printf '!!! test-samir-canon-y2k FAIL: the buggy A1001 -36477 CURRENT line is absent -- the Y2K mis-aging did not run in-emu (Law 4)\n'; \
	     cat "$(BUILD)/$(CY2K_NAME).samir"; exit 1; }
@grep -qF 'A1003  11/30/99  -36462   CURRENT' "$(BUILD)/$(CY2K_NAME).samir" \
	|| { printf '!!! test-samir-canon-y2k FAIL: the buggy A1003 -36462 CURRENT line is absent (Law 4)\n'; \
	     cat "$(BUILD)/$(CY2K_NAME).samir"; exit 1; }
@grep -qF 'TOTAL UNPAID OVERDUE:       0.00' "$(BUILD)/$(CY2K_NAME).samir" \
	|| { printf '!!! test-samir-canon-y2k FAIL: the headline 0.00 overdue total is absent -- the Y2K under-reporting did not occur (Law 4)\n'; \
	     cat "$(BUILD)/$(CY2K_NAME).samir"; exit 1; }
@printf '>>> test-samir-canon-y2k [3/4]: SERIAL shows the AR aging report WITH the enforced Y2K-buggy values (A1001 -36477 / A1003 -36462 / TOTAL 0.00)\n'
@# ---- 4. SCREENDUMP (Run 2; Law 4): the report on the LFB. The DO Y2KACCT run
@# prints ~12 report lines; they scroll the active band lower than the 3-row LIST
@# gate, so the report text lands in band [176,260). seafoam right margin checked
@# (immune to vertical scroll), same discriminator pattern as test-samir-write. ----
@$(call cy2k_mint,$(CY2K_SCRN_IMG),$(SAMIR_EXE),$(INVOICE_DBF))
@$(call fixture-preflight,$(CY2K_SCRN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CY2K_SCRN_IMG)" \
	--name "$(CY2K_SCRN_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CY2K_SCRN_KEYS)" --keys-after "SHELL-READY" \
	--screendump --screendump-after 'TOTAL UNPAID OVERDUE:       0.00. ' \
	2> "$(CY2K_SCRN_REPORT)" || true
@if grep -q 'triple_fault=1' "$(CY2K_SCRN_REPORT)"; then \
	printf '!!! test-samir-canon-y2k FAIL: TRIPLE FAULT on the screendump run\n'; exit 1; \
fi
@if [ ! -s "$(CY2K_PPM)" ]; then \
	printf '!!! test-samir-canon-y2k FAIL: no screendump captured at %s (live guest required)\n' "$(CY2K_PPM)"; exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(CY2K_PPM)" 176 260 300 32 560 \
	|| { printf '!!! test-samir-canon-y2k FAIL: the aging report did not render on the framebuffer (band [176,260) < 300 fg), or the right-margin desktop is not seafoam\n'; exit 1; }
@printf '>>> test-samir-canon-y2k [4/4]: screendump shows the AR aging report on the seafoam desktop\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- the Initech AR accounting app RAN inside InitechOS via DO Y2KACCT,\n'
@printf '            and its ENFORCED Year-2000 bug (586.1) produced the canonical wrong aging\n'
@printf '            (A1001/A1003 mis-aged to ~ -100 years; TOTAL UNPAID OVERDUE wrongly 0.00).\n'
@printf '            (QEMU only; tri-emulator agreement pending bead initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-samir-canon-y2k: green\n'
```

## test-samir-canon-y2k-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29918):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR_DOTRUNC.EXE build/INVOICE.DBF build/Y2KACCT.PRG build/.makefile.stamp`
```make
@printf '>>> test-samir-canon-y2k-mutant: confirming the DO-file half-read mutant breaks canon in-emu -> RED (Rule 6; initech-9a0f)\n'
@$(call cy2k_mint,$(CY2K_MUT_IMG),$(SAMIR_EXE_DOTRUNC),$(INVOICE_DBF))
@$(call fixture-preflight,$(CY2K_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(CY2K_MUT_IMG)" \
	--name "$(CY2K_MUT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(CY2K_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(CY2K_MUT_REPORT)"
@sh harness/emu/require_trace.sh "$(CY2K_MUT_REPORT)" "$(CY2K_MUT_SERIAL)" "$$(printf '%s\n' '$(CY2K_KEYS)' | awk -F, '{print NF}')"
@# The mutant must NOT triple-fault (it is a wrong-data bug, not a crash).
@if grep -q 'triple_fault=1' "$(CY2K_MUT_REPORT)"; then \
	printf '!!! test-samir-canon-y2k-mutant FAIL: the mutant TRIPLE-FAULTED -- a crash, not the wrong-data RED the gate asserts\n'; exit 1; \
fi
@if [ ! -s "$(CY2K_MUT_SERIAL)" ]; then \
	printf '!!! test-samir-canon-y2k-mutant FAIL: no serial captured (the mutant must still RUN, just read a partial .prg)\n'; exit 1; \
fi
@tr -d '\r' < "$(CY2K_MUT_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(CY2K_MUT_NAME).samir"
@# With the .prg read truncated, the canon report BODY is cut off: the buggy
@# A1001 -36477 line and the 0.00 total MUST be absent -> the canon assertions
@# would fail. If they are STILL present, the mutant did not bite (decoration).
@if grep -qF 'A1001  12/15/99  -36477   CURRENT' "$(BUILD)/$(CY2K_MUT_NAME).samir"; then \
	printf '!!! test-samir-canon-y2k-mutant FAIL: the buggy A1001 -36477 line is STILL present under the DO-truncation mutant -- it did not bite (gate is decoration)\n'; \
	cat "$(BUILD)/$(CY2K_MUT_NAME).samir"; exit 1; \
fi
@if grep -qF 'TOTAL UNPAID OVERDUE:       0.00' "$(BUILD)/$(CY2K_MUT_NAME).samir"; then \
	printf '!!! test-samir-canon-y2k-mutant FAIL: the headline 0.00 total is STILL present under the DO-truncation mutant -- it did not bite\n'; \
	cat "$(BUILD)/$(CY2K_MUT_NAME).samir"; exit 1; \
fi
@printf '    --- the TRUNCATED (mutant) in-emu transcript (the .prg body was cut off) ---\n'
@sed 's/^/    /' "$(BUILD)/$(CY2K_MUT_NAME).samir"
@printf '>>> test-samir-canon-y2k-mutant: green (the DO-file half-read mutant correctly RED -- the canon report body is absent; no crash)\n'
```

## test-samir-console-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33939):
Prerequisites: `build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@mode=status; case '$@' in *off-emu) mode=off;; *numeric*) mode=numeric;; *console*) mode=console;; esac; $(BUILD)/test_samir_screen_emu $$mode && \
 cp -f $(BUILD)/samir_screen_$$mode.ppm $(BUILD)/samir-screen/accepted-$$mode.ppm && \
 cp -f $(BUILD)/samir_screen_$$mode.serial $(BUILD)/samir-screen/accepted-$$mode.serial
```

## test-samir-console-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33960):
Prerequisites: `build/SAMIR_SCREEN_SAMIR_MUTATE_CONSOLE.EXE build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_screen_emu console $(BUILD)/SAMIR_SCREEN_SAMIR_MUTATE_CONSOLE.EXE > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && ! grep -Eq 'FAIL.*(guest|trace|shell|artifact|QMP|settings)' $(BUILD)/$@.log && grep -q 'console: program output and prompt suppressed' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (same CONSOLE suppression mutant RED in guest)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-console-review-color

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 11):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/test_console_review build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/console_review_emu.sh color
@printf '>>> test-samir-console-review-color: green\n'
```

## test-samir-console-review-color-state

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 11):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/test_console_review build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/console_review_emu.sh color-state
@printf '>>> test-samir-console-review-color-state: green\n'
```

## test-samir-console-review-continuation

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 11):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/test_console_review build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/console_review_emu.sh continuation
@printf '>>> test-samir-console-review-continuation: green\n'
```

## test-samir-console-review-controls

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 11):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/test_console_review build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/console_review_emu.sh controls
@printf '>>> test-samir-console-review-controls: green\n'
```

## test-samir-console-review-keys

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 11):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/test_console_review build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/console_review_emu.sh keys
@printf '>>> test-samir-console-review-keys: green\n'
```

## test-samir-console-review-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 42):
Prerequisites: `build/test_samir_continuation_mutant build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/test_console_review build/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_CONTINUATION.EXE build/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_KEYS.EXE build/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_COLOR.EXE build/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_COLOR_RESET.EXE build/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_REDIRECT.EXE build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_continuation_mutant > $(BUILD)/console-review-host-bound.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED continuation draw bound: row=24 col=3 n=80' $(BUILD)/console-review-host-bound.log || { cat $(BUILD)/console-review-host-bound.log; exit 1; }
@cat $(BUILD)/console-review-host-bound.log
@set -e; for m in $(CONSOLE_REVIEW_MUTANTS); do mode=$${m%%:*}; flag=$${m#*:}; log=$(BUILD)/console-review-mut-$$mode.log; \
 if sh $(DBF_DIFF_DIR)/console_review_emu.sh $$mode $(BUILD)/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_$$flag.EXE > $$log 2>&1; then cat $$log; printf '!!! console-review mutant %s survived\n' $$flag; exit 1; fi; \
 case $$mode in continuation) want='RED console-review: continuation native draw bound HALT';; keys) want='RED console-review: Left pair is navigation, never text K';; color|color-state) want='RED console-review: native SAY selected attribute 1F';; controls) want='RED console-review: redirected SAY paints real Z 0D Q 0A R cells';; esac; \
 grep -m1 -F "$$want" $$log || { cat $$log; exit 1; }; done
@printf '>>> test-samir-console-review-mutant: green (continuation bound HALT, keys, color, redirected literals RED)\n'
```

## test-samir-core

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34934):
Prerequisites: `build/test_samir_core build/.makefile.stamp`
```make
@$<
```

## test-samir-core-diff

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34991):
Prerequisites: `build/test_samir_core build/samir_core_ref build/.makefile.stamp`
```make
$(call samir-core-reference-run,accepted)
```

## test-samir-core-diff-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34993):
Prerequisites: `build/test_samir_core build/samir_core_ref build/test_samir_core_NDX_DUP_DESCENT build/.makefile.stamp`
```make
$(call samir-core-reference-run,mutant)
```

## test-samir-core-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34953):
Prerequisites: `build/test_samir_core_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(BUILD)/test_samir_core_emu $(SAMIR_EXE)
```

## test-samir-core-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 245):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-core-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-core-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-core-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-core-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-core-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-core-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-core-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-core-emu-bochs "$(BUILD)/samir-core-emu_bochs.report.txt" "$(BUILD)/samir-core-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-core-emu-mutants

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34955):
Prerequisites: `build/SAMIR_CORE_CONCAT_PLUS.EXE build/SAMIR_CORE_CONCAT_MINUS.EXE build/SAMIR_CORE_MEMVAR_RECLAIM.EXE build/test_samir_core_emu build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@set -e; for rule in $(SAMIR_CORE_EMU_RULES); do \
  log=$(BUILD)/samir-core-emu-$$rule.log; set +e; INITECH_QMP_STDIO=1 $(BUILD)/test_samir_core_emu $(BUILD)/SAMIR_CORE_$$rule.EXE > $$log 2>&1; rc=$$?; set -e; \
  test $$rc = 1 && grep -q "FAIL .*$$rule:" $$log && grep -q 'checks,' $$log && grep -q 'triple_fault=0' $$log && grep -q 'quit_after_found=1' $$log || { cat $$log; exit 1; }; \
  printf 'test-samir-core-emu-mutants: %s correctly RED: ' $$rule; grep -m1 "FAIL .*$$rule:" $$log; \
done
@printf 'test-samir-core-emu-mutants: GREEN (all guest regressions caught)\n'
```

## test-samir-core-mutants

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34936):
Prerequisites: `build/test_samir_core_DBF_SIZE build/test_samir_core_DATE_WIDTH build/test_samir_core_MEMO_POINTER build/test_samir_core_DBT_SPAN build/test_samir_core_NDX_SIZE build/test_samir_core_NDX_BUILD_SIZE build/test_samir_core_NDX_DUP_DESCENT build/test_samir_core_MEMVAR_RECLAIM build/test_samir_core_CONCAT_PLUS build/test_samir_core_CONCAT_MINUS build/test_samir_core_FILE_NEEDS_USE build/.makefile.stamp`
```make
@set -e; for rule in $(SAMIR_CORE_RULES); do \
  log=$(BUILD)/samir-core-$$rule.log; set +e; $(BUILD)/test_samir_core_$$rule > $$log 2>&1; rc=$$?; set -e; \
  test $$rc = 1 && grep -q "FAIL .*$$rule:" $$log && grep -q 'checks,' $$log || { cat $$log; exit 1; }; \
  printf 'test-samir-core-mutants: %s correctly RED: ' $$rule; grep -m1 "FAIL .*$$rule:" $$log; \
done
@printf 'test-samir-core-mutants: GREEN (all named regressions caught)\n'
```

## test-samir-ctl

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3632):
Prerequisites: `build/test_samir_ctl samir-ctl-goldens build/.makefile.stamp`
```make
@printf '>>> test-samir-ctl: Esc interrupt / ON ESCAPE / 20-level DO cap / RETURN TO MASTER / QUIT / RETRY vs real dBASE III PLUS 1.1 (mint samirctl)\n'
@$(TEST_SAMIR_CTL) $(abspath $(SAMIR_CTL_MINT))
@printf '>>> test-samir-ctl: green\n'
```

## test-samir-ctl-emu

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 29139):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@printf '>>> test-samir-ctl-emu: Esc / 20-level DO cap / QUIT in the guest (audit L002 L003 L013; mint samirctl wording)\n'
$(call samir_ctl_emu_run,$(SAMIR_EXE),samir_ctl_emu)
@sh harness/emu/require_trace.sh $(BUILD)/samir_ctl_emu.report $(BUILD)/samir_ctl_emu.serial "$$(printf '%s\n' '$(SAMIR_CTL_EMU_KEYS)' | awk -F, '{print NF}')"
@S=$(BUILD)/samir_ctl_emu.samir; R=$(BUILD)/samir_ctl_emu.report; \
fail() { printf '!!! test-samir-ctl-emu FAIL: %s\n' "$$1"; cat "$$S"; exit 1; }; \
! grep -q 'triple_fault=1' "$$R" || fail 'TRIPLE FAULT'; \
[ -s "$$S" ] || fail 'no SAMIR session on serial'; \
grep -qxF '*** INTERRUPTED ***' "$$S" || fail 'L002 Esc did not interrupt DO WHILE .T. (no *** INTERRUPTED ***)'; \
grep -qxF 'Called from - A:LOOP.prg' "$$S" || fail 'L002 trace line != real III+ "Called from - <drive>:<NAME>.prg"'; \
grep -qxF 'Cancel, Ignore, or Suspend? (C, I, or S) Cancel' "$$S" || fail 'L002 the C/I/S prompt + Cancel echo'; \
grep -qxF 'Do cancelled' "$$S" || fail 'L002 Do cancelled'; \
! grep -q 'NOTREACHED' "$$S" || fail 'L002 Cancel did not abandon the program'; \
grep -qF 'DOs nested too deep.' "$$S" || fail 'L003 recursion did not stop with #103 DOs nested too deep.'; \
sed -n '/do bigrec/,/do after/p' "$$S" | grep -qE 'DOs nested too deep\.|Insufficient memory\.' || fail 'L003 500-line recursive BIGREC did not stop with #103 or #43'; \
grep -qE 'RECOVERED +[1-9][0-9]*' "$$S" || fail 'L003/L002 session lost after recursion (DO AFTER: RECOVERED with N > 0 absent)'; \
grep -qF 'BEFORE QUIT' "$$S" || fail 'L013 QUITPRG did not run'; \
! grep -q 'AFTER QUIT' "$$S" || fail 'L013 a command after QUIT ran'; \
! grep -q 'Unrecognized command verb' "$$S" || fail 'L013/any: Unrecognized command verb'; \
sed -n '/BEFORE QUIT/,$$p' "$$S" | grep -q 'A:.>' || fail 'L013 QUIT did not return to the DOS prompt'; \
grep -q 'quit_after_found=1' "$$R" || fail 'the session never reached SHELL-DONE (EXIT after QUIT)'
@printf '>>> test-samir-ctl-emu: green (QEMU)\n'
```

## test-samir-ctl-emu-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 29161):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR_CTL_FLOW_MUTATE_NO_ESCAPE.EXE build/SAMIR_CTL_PROC_MUTATE_NO_DO_CAP.EXE build/SAMIR_CTL_FLOW_MUTATE_STACK_LINES.EXE build/.makefile.stamp`
```make
$(call samir_ctl_emu_run,$(BUILD)/SAMIR_CTL_FLOW_MUTATE_NO_ESCAPE.EXE,samir_ctl_emu_mut_esc,fault)
@sh harness/emu/require_fault_trace.sh $(BUILD)/samir_ctl_emu_mut_esc.report $(BUILD)/samir_ctl_emu_mut_esc.serial "$$(printf '%s\n' '$(SAMIR_CTL_EMU_KEYS)' | awk -F, '{print NF}')" escape
@if grep -qxF '*** INTERRUPTED ***' $(BUILD)/samir_ctl_emu_mut_esc.samir; then printf '!!! test-samir-ctl-emu-mutant FAIL: no-Esc-poll mutant still interrupted\n'; exit 1; fi
@grep -q 'triple_fault=0' $(BUILD)/samir_ctl_emu_mut_esc.report || { printf '!!! no-Esc mutant run did not complete cleanly (harness)\n'; exit 1; }
@printf '>>> test-samir-ctl-emu-mutant: Esc-never-polled correctly RED (the loop cannot be stopped)\n'
$(call samir_ctl_emu_run,$(BUILD)/SAMIR_CTL_PROC_MUTATE_NO_DO_CAP.EXE,samir_ctl_emu_mut_cap,fault)
@sh harness/emu/require_fault_trace.sh $(BUILD)/samir_ctl_emu_mut_cap.report $(BUILD)/samir_ctl_emu_mut_cap.serial "$$(printf '%s\n' '$(SAMIR_CTL_EMU_KEYS)' | awk -F, '{print NF}')" cap
@if grep -qE 'RECOVERED +[1-9][0-9]*' $(BUILD)/samir_ctl_emu_mut_cap.samir && grep -qF 'DOs nested too deep.' $(BUILD)/samir_ctl_emu_mut_cap.samir && grep -q 'quit_after_found=1' $(BUILD)/samir_ctl_emu_mut_cap.report; then printf '!!! test-samir-ctl-emu-mutant FAIL: no-DO-cap mutant survived the recursion\n'; exit 1; fi
@printf '>>> test-samir-ctl-emu-mutant: no-20-level-cap correctly RED (recursion past the cap loses the session)\n'
$(call samir_ctl_emu_run,$(BUILD)/SAMIR_CTL_FLOW_MUTATE_STACK_LINES.EXE,samir_ctl_emu_mut_stk,fault)
@sh harness/emu/require_fault_trace.sh $(BUILD)/samir_ctl_emu_mut_stk.report $(BUILD)/samir_ctl_emu_mut_stk.serial "$$(printf '%s\n' '$(SAMIR_CTL_EMU_KEYS)' | awk -F, '{print NF}')" stack
@if grep -qE 'RECOVERED +[1-9][0-9]*' $(BUILD)/samir_ctl_emu_mut_stk.samir && grep -q 'quit_after_found=1' $(BUILD)/samir_ctl_emu_mut_stk.report; then printf '!!! test-samir-ctl-emu-mutant FAIL: stack-line-table mutant survived BIGREC\n'; exit 1; fi
@printf '>>> test-samir-ctl-emu-mutant: per-call stack line tables correctly RED (500-line BIGREC loses the session)\n'
@printf '>>> test-samir-ctl-emu-mutant: green\n'
```

## test-samir-ctl-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3636):
Prerequisites: `build/test_samir_ctl_FLOW_MUTATE_NO_ESCAPE build/test_samir_ctl_PROC_MUTATE_NO_DO_CAP build/test_samir_ctl_PROC_MUTATE_MASTER_ONE_LEVEL build/test_samir_ctl_PROC_MUTATE_NO_QUIT build/test_samir_ctl_FLOW_MUTATE_NO_RETRY build/test_samir_ctl_FLOW_MUTATE_STACK_LINES samir-ctl-goldens build/.makefile.stamp`
```make
@for m in $(SAMIR_CTL_MUTS); do \
	$$m $(abspath $(SAMIR_CTL_MINT)) 2>/dev/null | grep -q 'checks,' \
		|| { printf '!!! test-samir-ctl-mutant FAIL: %s printed no summary -- RED is meaningless\n' "$$m"; exit 1; }; \
	if $$m $(abspath $(SAMIR_CTL_MINT)) >/dev/null 2>&1; then \
		printf '!!! test-samir-ctl-mutant FAIL: %s PASSED -- the oracle does not bite\n' "$$m"; exit 1; \
	fi; \
	printf '>>> test-samir-ctl-mutant: %s correctly RED\n' "$$(basename $$m)"; \
done
@printf '>>> test-samir-ctl-mutant: green (all 6 mutants RED)\n'
```

## test-samir-cursor25

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 57):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@SCREEN_FORMS_SUITE=cursor25 SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_cursor25_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh
```

## test-samir-cursor25-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 67):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR_SCREEN_SAMIR_CURSOR25_MUTANT.EXE build/.makefile.stamp`
```make
@log=$(BUILD)/samir-cursor25-mut.log; \
 if SCREEN_FORMS_SUITE=cursor25 SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_cursor25_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_CURSOR25_MUTANT.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
 grep -q '^RED: at-to-b/TE: RED: first difference cursor-row golden=25 actual=24' $$log || { cat $$log; exit 1; }; \
 cat $$log
@printf '>>> test-samir-cursor25-mutant: green (row 25 clamped to 24 RED)\n'
```

## test-samir-datatype

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35087):
Prerequisites: `build/test_samir_datatype build/.makefile.stamp`
```make
@for c in $(SAMIR_DT_CASES); do $(SAMIR_DT_TEST) $$c || exit 1; done
@printf '>>> test-samir-datatype: green (L001/L006/L016 vs real III+)\n'
```

## test-samir-datatype-mutants

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35090):
Prerequisites: `build/test_samir_datatype_blankwrite build/test_samir_datatype_starread build/test_samir_datatype_blankread build/test_samir_datatype_blankkey build/test_samir_datatype_cutoff test-samir-datatype build/.makefile.stamp`
```make
@for m in $(SAMIR_DT_MUTANTS); do red=0; \
  for c in $(SAMIR_DT_CASES); do \
    set +e; $(BUILD)/test_samir_datatype_$$m $$c > $(BUILD)/dt-mut-$$m-$$c.log 2>&1; rc=$$?; set -e; \
    if [ $$rc -eq 1 ] && grep -q 'FAIL.*L0[0-9][0-9]' $(BUILD)/dt-mut-$$m-$$c.log; then red=1; \
    elif [ $$rc -ne 0 ] && [ $$rc -ne 1 ]; then cat $(BUILD)/dt-mut-$$m-$$c.log; echo "mutant $$m: infrastructure failure"; exit 1; fi; \
  done; \
  if [ $$red -ne 1 ]; then printf '!!! SAMIR_DT_MUTANT_%s survived -- the check is decoration\n' $$m; exit 1; fi; \
  printf '>>> SAMIR_DT_MUTANT_%s RED: ' $$m; cat $(BUILD)/dt-mut-$$m-*.log | grep 'FAIL.*L0[0-9][0-9]' | head -1; \
done
```

## test-samir-dbf-ref

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3946):
Prerequisites: `build/.makefile.stamp`
```make
$(call need_goldens,test-samir-dbf-ref)
@printf ">>> test-samir-dbf-ref: independent .dbf/.dbt reader vs corpus goldens\n"
@if ! command -v python3 >/dev/null 2>&1 || [ ! -f "$(DBF_REF_PY)" ]; then \
	sh harness/missing_reference.sh REFERENCE_TOOLS "test-samir-dbf-ref: python3/reader absent"; \
else python3 $(DBF_REF_PY) --selftest; fi
@printf ">>> test-samir-dbf-ref: green\n"
```

## test-samir-def-deps

Declared tier: outside active vector; #  recipe to execute (from 'harness/registries.mk', line 12):
Prerequisites: `build/.makefile.stamp`
```make
+@MAKE="$(MAKE)" sh harness/def_dependency_check.sh
```

## test-samir-deferred-errors

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35416):
Prerequisites: `build/test_samir_deferred_errors build/.makefile.stamp`
```make
@$<
@echo '>>> test-samir-deferred-errors: green'
```

## test-samir-dml

Declared tier: outside active vector; 
Prerequisites: `test-samir-dml-clear test-samir-dml-macro test-samir-dml-close test-samir-dml-index test-samir-dml-reindex test-samir-dml-sort test-samir-dml-total test-samir-dml-append test-samir-dml-save test-samir-dml-restore test-samir-dml-copyfile test-samir-dml-type test-samir-dml-rename test-samir-dml-erase test-samir-dml-label test-samir-dml-run test-samir-dml-abbrev test-samir-dml-abbrscope test-samir-dml-status test-samir-dml-memory test-samir-dml-statusinto build/.makefile.stamp`
```make
```

## test-samir-dml-abbrev

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< abbrev host
@printf '>>> test-samir-dml-abbrev: green\n'
```

## test-samir-dml-abbrev-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) abbrev emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-abbrev-emu: green\n'
```

## test-samir-dml-abbrev-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-abbrev-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-abbrev-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-abbrev-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-abbrev-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-abbrev-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-abbrev-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-abbrev-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-abbrev-emu-bochs "$(BUILD)/samir-dml-abbrev-emu_bochs.report.txt" "$(BUILD)/samir-dml-abbrev-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-abbrev-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-abbrev-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_abbrev.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) abbrev emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_abbrev.EXE > $(BUILD)/dml-emu-mut-abbrev.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-abbrev.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-abbrev.log || { cat $(BUILD)/dml-emu-mut-abbrev.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_abbrev emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-abbrev.log | head -1`"
```

## test-samir-dml-abbrev-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_abbrev test-samir-dml-abbrev build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< abbrev host > $(BUILD)/dml-mut-abbrev.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-abbrev.log || { cat $(BUILD)/dml-mut-abbrev.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_abbrev RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-abbrev.log | head -1`"
```

## test-samir-dml-abbrscope

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< abbrscope host
@printf '>>> test-samir-dml-abbrscope: green\n'
```

## test-samir-dml-abbrscope-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) abbrscope emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-abbrscope-emu: green\n'
```

## test-samir-dml-abbrscope-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-abbrscope-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-abbrscope-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-abbrscope-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-abbrscope-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-abbrscope-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-abbrscope-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-abbrscope-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-abbrscope-emu-bochs "$(BUILD)/samir-dml-abbrscope-emu_bochs.report.txt" "$(BUILD)/samir-dml-abbrscope-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-abbrscope-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-abbrscope-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_abbrscope.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) abbrscope emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_abbrscope.EXE > $(BUILD)/dml-emu-mut-abbrscope.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-abbrscope.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-abbrscope.log || { cat $(BUILD)/dml-emu-mut-abbrscope.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_abbrscope emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-abbrscope.log | head -1`"
```

## test-samir-dml-abbrscope-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_abbrscope test-samir-dml-abbrscope build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< abbrscope host > $(BUILD)/dml-mut-abbrscope.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-abbrscope.log || { cat $(BUILD)/dml-mut-abbrscope.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_abbrscope RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-abbrscope.log | head -1`"
```

## test-samir-dml-append

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< append host
@printf '>>> test-samir-dml-append: green\n'
```

## test-samir-dml-append-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) append emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-append-emu: green\n'
```

## test-samir-dml-append-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-append-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-append-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-append-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-append-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-append-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-append-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-append-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-append-emu-bochs "$(BUILD)/samir-dml-append-emu_bochs.report.txt" "$(BUILD)/samir-dml-append-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-append-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-append-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_append.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) append emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_append.EXE > $(BUILD)/dml-emu-mut-append.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-append.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-append.log || { cat $(BUILD)/dml-emu-mut-append.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_append emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-append.log | head -1`"
```

## test-samir-dml-append-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_append test-samir-dml-append build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< append host > $(BUILD)/dml-mut-append.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-append.log || { cat $(BUILD)/dml-mut-append.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_append RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-append.log | head -1`"
```

## test-samir-dml-clear

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< clear host
@printf '>>> test-samir-dml-clear: green\n'
```

## test-samir-dml-clear-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) clear emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-clear-emu: green\n'
```

## test-samir-dml-clear-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-clear-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-clear-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-clear-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-clear-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-clear-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-clear-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-clear-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-clear-emu-bochs "$(BUILD)/samir-dml-clear-emu_bochs.report.txt" "$(BUILD)/samir-dml-clear-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-clear-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-clear-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_clear.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) clear emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_clear.EXE > $(BUILD)/dml-emu-mut-clear.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-clear.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-clear.log || { cat $(BUILD)/dml-emu-mut-clear.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_clear emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-clear.log | head -1`"
```

## test-samir-dml-clear-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_clear test-samir-dml-clear build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< clear host > $(BUILD)/dml-mut-clear.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-clear.log || { cat $(BUILD)/dml-mut-clear.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_clear RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-clear.log | head -1`"
```

## test-samir-dml-clear-state

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml samir-dml-goldens build/CLIENTS.DBF build/.makefile.stamp`
```make
@$< clear state
@printf '>>> test-samir-dml-clear-state: green (state only; exact transcript still required)\n'
```

## test-samir-dml-clear-state-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml_clear test-samir-dml-clear-state build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< clear state > $(BUILD)/dml-state-mut-clear.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*DML state clear' $(BUILD)/dml-state-mut-clear.log || { cat $(BUILD)/dml-state-mut-clear.log; exit 1; }; printf '>>> SAMIR_DML_MUTANT_clear RED: %s\n' "`grep 'FAIL.*DML state clear' $(BUILD)/dml-state-mut-clear.log | head -1`"
```

## test-samir-dml-close

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< close host
@printf '>>> test-samir-dml-close: green\n'
```

## test-samir-dml-close-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) close emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-close-emu: green\n'
```

## test-samir-dml-close-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-close-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-close-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-close-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-close-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-close-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-close-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-close-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-close-emu-bochs "$(BUILD)/samir-dml-close-emu_bochs.report.txt" "$(BUILD)/samir-dml-close-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-close-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-close-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_close.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) close emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_close.EXE > $(BUILD)/dml-emu-mut-close.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-close.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-close.log || { cat $(BUILD)/dml-emu-mut-close.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_close emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-close.log | head -1`"
```

## test-samir-dml-close-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_close test-samir-dml-close build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< close host > $(BUILD)/dml-mut-close.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-close.log || { cat $(BUILD)/dml-mut-close.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_close RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-close.log | head -1`"
```

## test-samir-dml-close-state

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml samir-dml-goldens build/CLIENTS.DBF build/.makefile.stamp`
```make
@$< close state
@printf '>>> test-samir-dml-close-state: green (state only; exact transcript still required)\n'
```

## test-samir-dml-close-state-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml_close test-samir-dml-close-state build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< close state > $(BUILD)/dml-state-mut-close.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*DML state close' $(BUILD)/dml-state-mut-close.log || { cat $(BUILD)/dml-state-mut-close.log; exit 1; }; printf '>>> SAMIR_DML_MUTANT_close RED: %s\n' "`grep 'FAIL.*DML state close' $(BUILD)/dml-state-mut-close.log | head -1`"
```

## test-samir-dml-copyfile

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< copyfile host
@printf '>>> test-samir-dml-copyfile: green\n'
```

## test-samir-dml-copyfile-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) copyfile emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-copyfile-emu: green\n'
```

## test-samir-dml-copyfile-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-copyfile-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-copyfile-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-copyfile-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-copyfile-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-copyfile-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-copyfile-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-copyfile-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-copyfile-emu-bochs "$(BUILD)/samir-dml-copyfile-emu_bochs.report.txt" "$(BUILD)/samir-dml-copyfile-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-copyfile-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-copyfile-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_copyfile.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) copyfile emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_copyfile.EXE > $(BUILD)/dml-emu-mut-copyfile.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-copyfile.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-copyfile.log || { cat $(BUILD)/dml-emu-mut-copyfile.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_copyfile emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-copyfile.log | head -1`"
```

## test-samir-dml-copyfile-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_copyfile test-samir-dml-copyfile build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< copyfile host > $(BUILD)/dml-mut-copyfile.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-copyfile.log || { cat $(BUILD)/dml-mut-copyfile.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_copyfile RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-copyfile.log | head -1`"
```

## test-samir-dml-emu

Declared tier: outside active vector; 
Prerequisites: `test-samir-dml-clear-emu test-samir-dml-macro-emu test-samir-dml-close-emu test-samir-dml-index-emu test-samir-dml-reindex-emu test-samir-dml-sort-emu test-samir-dml-total-emu test-samir-dml-append-emu test-samir-dml-save-emu test-samir-dml-restore-emu test-samir-dml-copyfile-emu test-samir-dml-type-emu test-samir-dml-rename-emu test-samir-dml-erase-emu test-samir-dml-label-emu test-samir-dml-run-emu test-samir-dml-abbrev-emu test-samir-dml-abbrscope-emu test-samir-dml-status-emu test-samir-dml-memory-emu test-samir-dml-statusinto-emu build/.makefile.stamp`
```make
```

## test-samir-dml-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 246):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-emu-bochs "$(BUILD)/samir-dml-emu_bochs.report.txt" "$(BUILD)/samir-dml-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-emu-mutants

Declared tier: outside active vector; 
Prerequisites: `test-samir-dml-clear-emu-mutant test-samir-dml-macro-emu-mutant test-samir-dml-close-emu-mutant test-samir-dml-index-emu-mutant test-samir-dml-reindex-emu-mutant test-samir-dml-sort-emu-mutant test-samir-dml-total-emu-mutant test-samir-dml-append-emu-mutant test-samir-dml-save-emu-mutant test-samir-dml-restore-emu-mutant test-samir-dml-copyfile-emu-mutant test-samir-dml-type-emu-mutant test-samir-dml-rename-emu-mutant test-samir-dml-erase-emu-mutant test-samir-dml-label-emu-mutant test-samir-dml-run-emu-mutant test-samir-dml-abbrev-emu-mutant test-samir-dml-abbrscope-emu-mutant test-samir-dml-status-emu-mutant test-samir-dml-memory-emu-mutant test-samir-dml-statusinto-emu-mutant build/.makefile.stamp`
```make
```

## test-samir-dml-erase

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< erase host
@printf '>>> test-samir-dml-erase: green\n'
```

## test-samir-dml-erase-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) erase emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-erase-emu: green\n'
```

## test-samir-dml-erase-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-erase-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-erase-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-erase-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-erase-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-erase-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-erase-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-erase-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-erase-emu-bochs "$(BUILD)/samir-dml-erase-emu_bochs.report.txt" "$(BUILD)/samir-dml-erase-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-erase-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-erase-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_erase.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) erase emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_erase.EXE > $(BUILD)/dml-emu-mut-erase.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-erase.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-erase.log || { cat $(BUILD)/dml-emu-mut-erase.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_erase emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-erase.log | head -1`"
```

## test-samir-dml-erase-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_erase test-samir-dml-erase build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< erase host > $(BUILD)/dml-mut-erase.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-erase.log || { cat $(BUILD)/dml-mut-erase.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_erase RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-erase.log | head -1`"
```

## test-samir-dml-index

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< index host
@printf '>>> test-samir-dml-index: green\n'
```

## test-samir-dml-index-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) index emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-index-emu: green\n'
```

## test-samir-dml-index-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-index-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-index-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-index-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-index-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-index-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-index-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-index-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-index-emu-bochs "$(BUILD)/samir-dml-index-emu_bochs.report.txt" "$(BUILD)/samir-dml-index-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-index-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-index-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_index.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) index emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_index.EXE > $(BUILD)/dml-emu-mut-index.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-index.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-index.log || { cat $(BUILD)/dml-emu-mut-index.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_index emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-index.log | head -1`"
```

## test-samir-dml-index-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_index test-samir-dml-index build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< index host > $(BUILD)/dml-mut-index.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-index.log || { cat $(BUILD)/dml-mut-index.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_index RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-index.log | head -1`"
```

## test-samir-dml-label

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< label host
@printf '>>> test-samir-dml-label: green\n'
```

## test-samir-dml-label-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) label emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-label-emu: green\n'
```

## test-samir-dml-label-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-label-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-label-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-label-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-label-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-label-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-label-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-label-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-label-emu-bochs "$(BUILD)/samir-dml-label-emu_bochs.report.txt" "$(BUILD)/samir-dml-label-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-label-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-label-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_label.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) label emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_label.EXE > $(BUILD)/dml-emu-mut-label.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-label.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-label.log || { cat $(BUILD)/dml-emu-mut-label.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_label emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-label.log | head -1`"
```

## test-samir-dml-label-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_label test-samir-dml-label build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< label host > $(BUILD)/dml-mut-label.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-label.log || { cat $(BUILD)/dml-mut-label.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_label RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-label.log | head -1`"
```

## test-samir-dml-macro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< macro host
@printf '>>> test-samir-dml-macro: green\n'
```

## test-samir-dml-macro-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) macro emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-macro-emu: green\n'
```

## test-samir-dml-macro-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-macro-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-macro-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-macro-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-macro-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-macro-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-macro-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-macro-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-macro-emu-bochs "$(BUILD)/samir-dml-macro-emu_bochs.report.txt" "$(BUILD)/samir-dml-macro-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-macro-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-macro-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_macro.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) macro emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_macro.EXE > $(BUILD)/dml-emu-mut-macro.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-macro.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-macro.log || { cat $(BUILD)/dml-emu-mut-macro.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_macro emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-macro.log | head -1`"
```

## test-samir-dml-macro-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_macro test-samir-dml-macro build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< macro host > $(BUILD)/dml-mut-macro.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-macro.log || { cat $(BUILD)/dml-mut-macro.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_macro RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-macro.log | head -1`"
```

## test-samir-dml-macro-state

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml samir-dml-goldens build/CLIENTS.DBF build/.makefile.stamp`
```make
@$< macro state
@printf '>>> test-samir-dml-macro-state: green (state only; exact transcript still required)\n'
```

## test-samir-dml-macro-state-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35177):
Prerequisites: `build/test_samir_dml_macro test-samir-dml-macro-state build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< macro state > $(BUILD)/dml-state-mut-macro.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*DML state macro' $(BUILD)/dml-state-mut-macro.log || { cat $(BUILD)/dml-state-mut-macro.log; exit 1; }; printf '>>> SAMIR_DML_MUTANT_macro RED: %s\n' "`grep 'FAIL.*DML state macro' $(BUILD)/dml-state-mut-macro.log | head -1`"
```

## test-samir-dml-memory

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< memory host
@printf '>>> test-samir-dml-memory: green\n'
```

## test-samir-dml-memory-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) memory emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-memory-emu: green\n'
```

## test-samir-dml-memory-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-memory-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-memory-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-memory-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-memory-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-memory-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-memory-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-memory-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-memory-emu-bochs "$(BUILD)/samir-dml-memory-emu_bochs.report.txt" "$(BUILD)/samir-dml-memory-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-memory-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-memory-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_memory.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) memory emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_memory.EXE > $(BUILD)/dml-emu-mut-memory.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-memory.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-memory.log || { cat $(BUILD)/dml-emu-mut-memory.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_memory emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-memory.log | head -1`"
```

## test-samir-dml-memory-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_memory test-samir-dml-memory build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< memory host > $(BUILD)/dml-mut-memory.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-memory.log || { cat $(BUILD)/dml-mut-memory.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_memory RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-memory.log | head -1`"
```

## test-samir-dml-mutants

Declared tier: outside active vector; 
Prerequisites: `test-samir-dml-clear-mutant test-samir-dml-macro-mutant test-samir-dml-close-mutant test-samir-dml-index-mutant test-samir-dml-reindex-mutant test-samir-dml-sort-mutant test-samir-dml-total-mutant test-samir-dml-append-mutant test-samir-dml-save-mutant test-samir-dml-restore-mutant test-samir-dml-copyfile-mutant test-samir-dml-type-mutant test-samir-dml-rename-mutant test-samir-dml-erase-mutant test-samir-dml-label-mutant test-samir-dml-run-mutant test-samir-dml-abbrev-mutant test-samir-dml-abbrscope-mutant test-samir-dml-status-mutant test-samir-dml-memory-mutant test-samir-dml-statusinto-mutant build/.makefile.stamp`
```make
```

## test-samir-dml-reindex

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< reindex host
@printf '>>> test-samir-dml-reindex: green\n'
```

## test-samir-dml-reindex-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) reindex emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-reindex-emu: green\n'
```

## test-samir-dml-reindex-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-reindex-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-reindex-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-reindex-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-reindex-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-reindex-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-reindex-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-reindex-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-reindex-emu-bochs "$(BUILD)/samir-dml-reindex-emu_bochs.report.txt" "$(BUILD)/samir-dml-reindex-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-reindex-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-reindex-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_reindex.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) reindex emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_reindex.EXE > $(BUILD)/dml-emu-mut-reindex.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-reindex.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-reindex.log || { cat $(BUILD)/dml-emu-mut-reindex.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_reindex emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-reindex.log | head -1`"
```

## test-samir-dml-reindex-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_reindex test-samir-dml-reindex build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< reindex host > $(BUILD)/dml-mut-reindex.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-reindex.log || { cat $(BUILD)/dml-mut-reindex.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_reindex RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-reindex.log | head -1`"
```

## test-samir-dml-rename

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< rename host
@printf '>>> test-samir-dml-rename: green\n'
```

## test-samir-dml-rename-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) rename emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-rename-emu: green\n'
```

## test-samir-dml-rename-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-rename-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-rename-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-rename-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-rename-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-rename-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-rename-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-rename-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-rename-emu-bochs "$(BUILD)/samir-dml-rename-emu_bochs.report.txt" "$(BUILD)/samir-dml-rename-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-rename-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-rename-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_rename.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) rename emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_rename.EXE > $(BUILD)/dml-emu-mut-rename.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-rename.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-rename.log || { cat $(BUILD)/dml-emu-mut-rename.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_rename emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-rename.log | head -1`"
```

## test-samir-dml-rename-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_rename test-samir-dml-rename build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< rename host > $(BUILD)/dml-mut-rename.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-rename.log || { cat $(BUILD)/dml-mut-rename.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_rename RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-rename.log | head -1`"
```

## test-samir-dml-restore

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< restore host
@printf '>>> test-samir-dml-restore: green\n'
```

## test-samir-dml-restore-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) restore emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-restore-emu: green\n'
```

## test-samir-dml-restore-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-restore-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-restore-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-restore-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-restore-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-restore-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-restore-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-restore-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-restore-emu-bochs "$(BUILD)/samir-dml-restore-emu_bochs.report.txt" "$(BUILD)/samir-dml-restore-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-restore-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-restore-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_restore.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) restore emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_restore.EXE > $(BUILD)/dml-emu-mut-restore.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-restore.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-restore.log || { cat $(BUILD)/dml-emu-mut-restore.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_restore emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-restore.log | head -1`"
```

## test-samir-dml-restore-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_restore test-samir-dml-restore build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< restore host > $(BUILD)/dml-mut-restore.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-restore.log || { cat $(BUILD)/dml-mut-restore.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_restore RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-restore.log | head -1`"
```

## test-samir-dml-run

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< run host
@printf '>>> test-samir-dml-run: green\n'
```

## test-samir-dml-run-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) run emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-run-emu: green\n'
```

## test-samir-dml-run-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-run-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-run-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-run-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-run-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-run-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-run-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-run-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-run-emu-bochs "$(BUILD)/samir-dml-run-emu_bochs.report.txt" "$(BUILD)/samir-dml-run-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-run-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-run-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_run.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) run emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_run.EXE > $(BUILD)/dml-emu-mut-run.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-run.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-run.log || { cat $(BUILD)/dml-emu-mut-run.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_run emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-run.log | head -1`"
```

## test-samir-dml-run-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_run test-samir-dml-run build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< run host > $(BUILD)/dml-mut-run.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-run.log || { cat $(BUILD)/dml-mut-run.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_run RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-run.log | head -1`"
```

## test-samir-dml-save

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< save host
@printf '>>> test-samir-dml-save: green\n'
```

## test-samir-dml-save-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) save emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-save-emu: green\n'
```

## test-samir-dml-save-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-save-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-save-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-save-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-save-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-save-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-save-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-save-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-save-emu-bochs "$(BUILD)/samir-dml-save-emu_bochs.report.txt" "$(BUILD)/samir-dml-save-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-save-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-save-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_save.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) save emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_save.EXE > $(BUILD)/dml-emu-mut-save.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-save.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-save.log || { cat $(BUILD)/dml-emu-mut-save.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_save emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-save.log | head -1`"
```

## test-samir-dml-save-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_save test-samir-dml-save build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< save host > $(BUILD)/dml-mut-save.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-save.log || { cat $(BUILD)/dml-mut-save.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_save RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-save.log | head -1`"
```

## test-samir-dml-sort

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< sort host
@printf '>>> test-samir-dml-sort: green\n'
```

## test-samir-dml-sort-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) sort emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-sort-emu: green\n'
```

## test-samir-dml-sort-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-sort-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-sort-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-sort-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-sort-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-sort-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-sort-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-sort-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-sort-emu-bochs "$(BUILD)/samir-dml-sort-emu_bochs.report.txt" "$(BUILD)/samir-dml-sort-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-sort-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-sort-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_sort.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) sort emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_sort.EXE > $(BUILD)/dml-emu-mut-sort.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-sort.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-sort.log || { cat $(BUILD)/dml-emu-mut-sort.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_sort emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-sort.log | head -1`"
```

## test-samir-dml-sort-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_sort test-samir-dml-sort build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< sort host > $(BUILD)/dml-mut-sort.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-sort.log || { cat $(BUILD)/dml-mut-sort.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_sort RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-sort.log | head -1`"
```

## test-samir-dml-status

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< status host
@printf '>>> test-samir-dml-status: green\n'
```

## test-samir-dml-status-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-status-emu: green\n'
```

## test-samir-dml-status-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-status-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-status-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-status-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-status-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-status-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-status-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-status-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-status-emu-bochs "$(BUILD)/samir-dml-status-emu_bochs.report.txt" "$(BUILD)/samir-dml-status-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-status-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-status-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_status.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_status.EXE > $(BUILD)/dml-emu-mut-status.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-status.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-status.log || { cat $(BUILD)/dml-emu-mut-status.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_status emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-status.log | head -1`"
```

## test-samir-dml-status-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_status test-samir-dml-status build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< status host > $(BUILD)/dml-mut-status.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-status.log || { cat $(BUILD)/dml-mut-status.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_status RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-status.log | head -1`"
```

## test-samir-dml-statusinto

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< statusinto host
@printf '>>> test-samir-dml-statusinto: green\n'
```

## test-samir-dml-statusinto-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) statusinto emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-statusinto-emu: green\n'
```

## test-samir-dml-statusinto-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-statusinto-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-statusinto-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-statusinto-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-statusinto-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-statusinto-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-statusinto-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-statusinto-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-statusinto-emu-bochs "$(BUILD)/samir-dml-statusinto-emu_bochs.report.txt" "$(BUILD)/samir-dml-statusinto-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-statusinto-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-statusinto-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_statusinto.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) statusinto emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_statusinto.EXE > $(BUILD)/dml-emu-mut-statusinto.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-statusinto.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-statusinto.log || { cat $(BUILD)/dml-emu-mut-statusinto.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_statusinto emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-statusinto.log | head -1`"
```

## test-samir-dml-statusinto-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_statusinto test-samir-dml-statusinto build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< statusinto host > $(BUILD)/dml-mut-statusinto.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-statusinto.log || { cat $(BUILD)/dml-mut-statusinto.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_statusinto RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-statusinto.log | head -1`"
```

## test-samir-dml-total

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< total host
@printf '>>> test-samir-dml-total: green\n'
```

## test-samir-dml-total-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) total emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-total-emu: green\n'
```

## test-samir-dml-total-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-total-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-total-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-total-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-total-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-total-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-total-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-total-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-total-emu-bochs "$(BUILD)/samir-dml-total-emu_bochs.report.txt" "$(BUILD)/samir-dml-total-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-total-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-total-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_total.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) total emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_total.EXE > $(BUILD)/dml-emu-mut-total.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-total.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-total.log || { cat $(BUILD)/dml-emu-mut-total.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_total emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-total.log | head -1`"
```

## test-samir-dml-total-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_total test-samir-dml-total build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< total host > $(BUILD)/dml-mut-total.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-total.log || { cat $(BUILD)/dml-mut-total.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_total RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-total.log | head -1`"
```

## test-samir-dml-type

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@$< type host
@printf '>>> test-samir-dml-type: green\n'
```

## test-samir-dml-type-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) type emu $(SAMIR_EXE)
@printf '>>> test-samir-dml-type-emu: green\n'
```

## test-samir-dml-type-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 247):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-dml-type-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-dml-type-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-dml-type-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-dml-type-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-dml-type-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-dml-type-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-dml-type-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-dml-type-emu-bochs "$(BUILD)/samir-dml-type-emu_bochs.report.txt" "$(BUILD)/samir-dml-type-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-dml-type-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `test-samir-dml-type-emu build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_DML_MUTANT_type.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) type emu $(BUILD)/SAMIR_SCREEN_SAMIR_DML_MUTANT_type.EXE > $(BUILD)/dml-emu-mut-type.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/dml-emu-mut-type.log && grep -q 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-type.log || { cat $(BUILD)/dml-emu-mut-type.log; echo 'emu mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_type emu RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-emu-mut-type.log | head -1`"
```

## test-samir-dml-type-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35042):
Prerequisites: `build/test_samir_dml_type test-samir-dml-type build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $< type host > $(BUILD)/dml-mut-type.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'DML transcript byte-identical\|bytes match real III+' $(BUILD)/dml-mut-type.log || { cat $(BUILD)/dml-mut-type.log; echo 'mutant survived or failed infrastructure'; exit 1; }; printf '>>> SAMIR_DML_MUTANT_type RED: %s\n' "`grep 'FAIL.*DML transcript\|FAIL.*bytes match real III+' $(BUILD)/dml-mut-type.log | head -1`"
```

## test-samir-do-file

Declared tier: HOST; 
Prerequisites: `test-interp-do-file build/.makefile.stamp`
```make
```

## test-samir-do-file-emu

Declared tier: EMU; 
Prerequisites: `test-interp-do-file-emu build/.makefile.stamp`
```make
```

## test-samir-do-file-mutant

Declared tier: HOST; 
Prerequisites: `test-interp-do-file-mutant build/.makefile.stamp`
```make
```

## test-samir-dotprompt

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 55):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@SCREEN_FORMS_SUITE=dotprompt SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_dotprompt_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh
```

## test-samir-dotprompt-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 61):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_bottom.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_blank79.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_scoreboard.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_clear.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_enter.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_saywrap.EXE build/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_saycontrol.EXE build/.makefile.stamp`
```make
@set -e; for m in $(DOTPROMPT_MUTANTS); do k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-dotprompt-mut-$$k.log; \
  if SCREEN_FORMS_SUITE=dotprompt SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_dotprompt_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_$$k.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
  grep -q "^RED: $$want: RED: first difference" $$log || { cat $$log; exit 1; }; \
  printf 'SAMIR_DOTPROMPT_MUTANT_%s RED: ' $$k; grep -m1 "^RED: $$want" $$log; done
@printf '>>> test-samir-dotprompt-mutant: green (bottom, blank79, scoreboard, clear, enter, saywrap, saycontrol RED)\n'
```

## test-samir-fns

Declared tier: outside active vector; 
Prerequisites: `test-samir-fns-alias test-samir-fns-alternate test-samir-fns-argorder test-samir-fns-argpicture test-samir-fns-col test-samir-fns-console test-samir-fns-diskspace test-samir-fns-division test-samir-fns-errors test-samir-fns-exp test-samir-fns-exppicture test-samir-fns-exprcol test-samir-fns-fklabel test-samir-fns-getenv test-samir-fns-index test-samir-fns-inkey test-samir-fns-iscolor test-samir-fns-limitpic test-samir-fns-listoff test-samir-fns-log test-samir-fns-logdoarg test-samir-fns-logguards test-samir-fns-lupdate test-samir-fns-matrixb test-samir-fns-ndx test-samir-fns-ndxdisplay test-samir-fns-ndxdoarg test-samir-fns-numconvert test-samir-fns-numlist test-samir-fns-os test-samir-fns-pcol test-samir-fns-prow test-samir-fns-readkey test-samir-fns-recsize test-samir-fns-retryconfirm test-samir-fns-returnarg test-samir-fns-row test-samir-fns-sqrt test-samir-fns-sqrtdoarg test-samir-fns-sqrtpass test-samir-fns-struct test-samir-fns-time test-samir-fns-type test-samir-fns-unknown test-samir-fns-version test-samir-deferred-errors test-samir-ndx-names build/.makefile.stamp`
```make
```

## test-samir-fns-alias

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) alias host
@echo '>>> test-samir-fns-alias: green'
```

## test-samir-fns-alias-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) alias emu
@echo '>>> test-samir-fns-alias-emu: green'
```

## test-samir-fns-alias-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-alias-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_alias.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) alias emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_alias.EXE > $(BUILD)/samirfns/emu-mut-alias.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-alias.log || { cat $(BUILD)/samirfns/emu-mut-alias.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_alias emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-alias.log`"
```

## test-samir-fns-alias-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-alias build/test_samir_fns_alias build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_alias alias host > $(BUILD)/samirfns/mut-alias.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-alias.log || { cat $(BUILD)/samirfns/mut-alias.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_alias RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-alias.log`"
```

## test-samir-fns-alternate

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) alternate host
@echo '>>> test-samir-fns-alternate: green'
```

## test-samir-fns-alternate-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) alternate emu
@echo '>>> test-samir-fns-alternate-emu: green'
```

## test-samir-fns-alternate-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-alternate-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_alternate.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) alternate emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_alternate.EXE > $(BUILD)/samirfns/emu-mut-alternate.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-alternate.log || { cat $(BUILD)/samirfns/emu-mut-alternate.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_alternate emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-alternate.log`"
```

## test-samir-fns-alternate-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-alternate build/test_samir_fns_alternate build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_alternate alternate host > $(BUILD)/samirfns/mut-alternate.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-alternate.log || { cat $(BUILD)/samirfns/mut-alternate.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_alternate RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-alternate.log`"
```

## test-samir-fns-argorder

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) argorder host
@echo '>>> test-samir-fns-argorder: green'
```

## test-samir-fns-argorder-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) argorder emu
@echo '>>> test-samir-fns-argorder-emu: green'
```

## test-samir-fns-argorder-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-argorder-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_argorder.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) argorder emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_argorder.EXE > $(BUILD)/samirfns/emu-mut-argorder.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-argorder.log || { cat $(BUILD)/samirfns/emu-mut-argorder.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_argorder emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-argorder.log`"
```

## test-samir-fns-argorder-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-argorder build/test_samir_fns_argorder build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_argorder argorder host > $(BUILD)/samirfns/mut-argorder.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-argorder.log || { cat $(BUILD)/samirfns/mut-argorder.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_argorder RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-argorder.log`"
```

## test-samir-fns-argpicture

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) argpicture host
@echo '>>> test-samir-fns-argpicture: green'
```

## test-samir-fns-argpicture-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) argpicture emu
@echo '>>> test-samir-fns-argpicture-emu: green'
```

## test-samir-fns-argpicture-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-argpicture-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_argpicture.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) argpicture emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_argpicture.EXE > $(BUILD)/samirfns/emu-mut-argpicture.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-argpicture.log || { cat $(BUILD)/samirfns/emu-mut-argpicture.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_argpicture emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-argpicture.log`"
```

## test-samir-fns-argpicture-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-argpicture build/test_samir_fns_argpicture build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_argpicture argpicture host > $(BUILD)/samirfns/mut-argpicture.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-argpicture.log || { cat $(BUILD)/samirfns/mut-argpicture.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_argpicture RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-argpicture.log`"
```

## test-samir-fns-century

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) century host
@echo '>>> test-samir-fns-century: green'
```

## test-samir-fns-col

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) col host
@echo '>>> test-samir-fns-col: green'
```

## test-samir-fns-col-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) col emu
@echo '>>> test-samir-fns-col-emu: green'
```

## test-samir-fns-col-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-col-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_col.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) col emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_col.EXE > $(BUILD)/samirfns/emu-mut-col.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-col.log || { cat $(BUILD)/samirfns/emu-mut-col.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_col emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-col.log`"
```

## test-samir-fns-col-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-col build/test_samir_fns_col build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_col col host > $(BUILD)/samirfns/mut-col.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-col.log || { cat $(BUILD)/samirfns/mut-col.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_col RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-col.log`"
```

## test-samir-fns-console

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) console host
@echo '>>> test-samir-fns-console: green'
```

## test-samir-fns-console-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) console emu
@echo '>>> test-samir-fns-console-emu: green'
```

## test-samir-fns-console-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-console-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_console.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) console emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_console.EXE > $(BUILD)/samirfns/emu-mut-console.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-console.log || { cat $(BUILD)/samirfns/emu-mut-console.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_console emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-console.log`"
```

## test-samir-fns-console-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-console build/test_samir_fns_console build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_console console host > $(BUILD)/samirfns/mut-console.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-console.log || { cat $(BUILD)/samirfns/mut-console.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_console RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-console.log`"
```

## test-samir-fns-default

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) default host
@echo '>>> test-samir-fns-default: green'
```

## test-samir-fns-diskspace

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) diskspace host
@echo '>>> test-samir-fns-diskspace: green'
```

## test-samir-fns-diskspace-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) diskspace emu
@echo '>>> test-samir-fns-diskspace-emu: green'
```

## test-samir-fns-diskspace-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-diskspace-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_diskspace.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) diskspace emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_diskspace.EXE > $(BUILD)/samirfns/emu-mut-diskspace.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-diskspace.log || { cat $(BUILD)/samirfns/emu-mut-diskspace.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_diskspace emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-diskspace.log`"
```

## test-samir-fns-diskspace-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-diskspace build/test_samir_fns_diskspace build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_diskspace diskspace host > $(BUILD)/samirfns/mut-diskspace.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-diskspace.log || { cat $(BUILD)/samirfns/mut-diskspace.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_diskspace RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-diskspace.log`"
```

## test-samir-fns-division

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) division host
@echo '>>> test-samir-fns-division: green'
```

## test-samir-fns-division-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) division emu
@echo '>>> test-samir-fns-division-emu: green'
```

## test-samir-fns-division-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-division-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_division.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) division emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_division.EXE > $(BUILD)/samirfns/emu-mut-division.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-division.log || { cat $(BUILD)/samirfns/emu-mut-division.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_division emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-division.log`"
```

## test-samir-fns-division-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-division build/test_samir_fns_division build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_division division host > $(BUILD)/samirfns/mut-division.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-division.log || { cat $(BUILD)/samirfns/mut-division.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_division RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-division.log`"
```

## test-samir-fns-emu

Declared tier: outside active vector; 
Prerequisites: `test-samir-fns-alias-emu test-samir-fns-alternate-emu test-samir-fns-argorder-emu test-samir-fns-argpicture-emu test-samir-fns-col-emu test-samir-fns-console-emu test-samir-fns-diskspace-emu test-samir-fns-division-emu test-samir-fns-errors-emu test-samir-fns-exp-emu test-samir-fns-exppicture-emu test-samir-fns-exprcol-emu test-samir-fns-fklabel-emu test-samir-fns-getenv-emu test-samir-fns-index-emu test-samir-fns-inkey-emu test-samir-fns-iscolor-emu test-samir-fns-limitpic-emu test-samir-fns-listoff-emu test-samir-fns-log-emu test-samir-fns-logdoarg-emu test-samir-fns-logguards-emu test-samir-fns-lupdate-emu test-samir-fns-matrixb-emu test-samir-fns-ndx-emu test-samir-fns-ndxdisplay-emu test-samir-fns-ndxdoarg-emu test-samir-fns-numconvert-emu test-samir-fns-numlist-emu test-samir-fns-os-emu test-samir-fns-pcol-emu test-samir-fns-prow-emu test-samir-fns-readkey-emu test-samir-fns-recsize-emu test-samir-fns-retryconfirm-emu test-samir-fns-returnarg-emu test-samir-fns-row-emu test-samir-fns-sqrt-emu test-samir-fns-sqrtdoarg-emu test-samir-fns-sqrtpass-emu test-samir-fns-struct-emu test-samir-fns-time-emu test-samir-fns-type-emu test-samir-fns-unknown-emu test-samir-fns-version-emu build/.makefile.stamp`
```make
```

## test-samir-fns-emu-mutants

Declared tier: outside active vector; 
Prerequisites: `test-samir-fns-alias-emu-mutant test-samir-fns-alternate-emu-mutant test-samir-fns-argorder-emu-mutant test-samir-fns-argpicture-emu-mutant test-samir-fns-col-emu-mutant test-samir-fns-console-emu-mutant test-samir-fns-diskspace-emu-mutant test-samir-fns-division-emu-mutant test-samir-fns-errors-emu-mutant test-samir-fns-exp-emu-mutant test-samir-fns-exppicture-emu-mutant test-samir-fns-exprcol-emu-mutant test-samir-fns-fklabel-emu-mutant test-samir-fns-getenv-emu-mutant test-samir-fns-index-emu-mutant test-samir-fns-inkey-emu-mutant test-samir-fns-iscolor-emu-mutant test-samir-fns-limitpic-emu-mutant test-samir-fns-listoff-emu-mutant test-samir-fns-log-emu-mutant test-samir-fns-logdoarg-emu-mutant test-samir-fns-logguards-emu-mutant test-samir-fns-lupdate-emu-mutant test-samir-fns-matrixb-emu-mutant test-samir-fns-ndx-emu-mutant test-samir-fns-ndxdisplay-emu-mutant test-samir-fns-ndxdoarg-emu-mutant test-samir-fns-numconvert-emu-mutant test-samir-fns-numlist-emu-mutant test-samir-fns-os-emu-mutant test-samir-fns-pcol-emu-mutant test-samir-fns-prow-emu-mutant test-samir-fns-readkey-emu-mutant test-samir-fns-recsize-emu-mutant test-samir-fns-retryconfirm-emu-mutant test-samir-fns-returnarg-emu-mutant test-samir-fns-row-emu-mutant test-samir-fns-sqrt-emu-mutant test-samir-fns-sqrtdoarg-emu-mutant test-samir-fns-sqrtpass-emu-mutant test-samir-fns-struct-emu-mutant test-samir-fns-time-emu-mutant test-samir-fns-type-emu-mutant test-samir-fns-unknown-emu-mutant test-samir-fns-version-emu-mutant test-samir-fns-ndx-handler-name-emu-mutant test-samir-fns-ndx-handler-depth-emu-mutant build/.makefile.stamp`
```make
```

## test-samir-fns-errors

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) errors host
@echo '>>> test-samir-fns-errors: green'
```

## test-samir-fns-errors-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) errors emu
@echo '>>> test-samir-fns-errors-emu: green'
```

## test-samir-fns-errors-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-errors-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_errors.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) errors emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_errors.EXE > $(BUILD)/samirfns/emu-mut-errors.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-errors.log || { cat $(BUILD)/samirfns/emu-mut-errors.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_errors emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-errors.log`"
```

## test-samir-fns-errors-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-errors build/test_samir_fns_errors build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_errors errors host > $(BUILD)/samirfns/mut-errors.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-errors.log || { cat $(BUILD)/samirfns/mut-errors.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_errors RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-errors.log`"
```

## test-samir-fns-exp

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) exp host
@echo '>>> test-samir-fns-exp: green'
```

## test-samir-fns-exp-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exp emu
@echo '>>> test-samir-fns-exp-emu: green'
```

## test-samir-fns-exp-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exp-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exp.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exp emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exp.EXE > $(BUILD)/samirfns/emu-mut-exp.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exp.log || { cat $(BUILD)/samirfns/emu-mut-exp.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exp emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exp.log`"
```

## test-samir-fns-exp-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exp build/test_samir_fns_exp build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_exp exp host > $(BUILD)/samirfns/mut-exp.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exp.log || { cat $(BUILD)/samirfns/mut-exp.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exp RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exp.log`"
```

## test-samir-fns-exppicture

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) exppicture host
@echo '>>> test-samir-fns-exppicture: green'
```

## test-samir-fns-exppicture-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exppicture emu
@echo '>>> test-samir-fns-exppicture-emu: green'
```

## test-samir-fns-exppicture-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exppicture-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exppicture.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exppicture emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exppicture.EXE > $(BUILD)/samirfns/emu-mut-exppicture.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exppicture.log || { cat $(BUILD)/samirfns/emu-mut-exppicture.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exppicture emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exppicture.log`"
```

## test-samir-fns-exppicture-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exppicture build/test_samir_fns_exppicture build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_exppicture exppicture host > $(BUILD)/samirfns/mut-exppicture.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exppicture.log || { cat $(BUILD)/samirfns/mut-exppicture.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exppicture RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exppicture.log`"
```

## test-samir-fns-exprcol

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) exprcol host
@echo '>>> test-samir-fns-exprcol: green'
```

## test-samir-fns-exprcol-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exprcol emu
@echo '>>> test-samir-fns-exprcol-emu: green'
```

## test-samir-fns-exprcol-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exprcol-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exprcol.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) exprcol emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_exprcol.EXE > $(BUILD)/samirfns/emu-mut-exprcol.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exprcol.log || { cat $(BUILD)/samirfns/emu-mut-exprcol.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exprcol emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-exprcol.log`"
```

## test-samir-fns-exprcol-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-exprcol build/test_samir_fns_exprcol build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_exprcol exprcol host > $(BUILD)/samirfns/mut-exprcol.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exprcol.log || { cat $(BUILD)/samirfns/mut-exprcol.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_exprcol RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-exprcol.log`"
```

## test-samir-fns-fklabel

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) fklabel host
@echo '>>> test-samir-fns-fklabel: green'
```

## test-samir-fns-fklabel-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) fklabel emu
@echo '>>> test-samir-fns-fklabel-emu: green'
```

## test-samir-fns-fklabel-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-fklabel-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_fklabel.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) fklabel emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_fklabel.EXE > $(BUILD)/samirfns/emu-mut-fklabel.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-fklabel.log || { cat $(BUILD)/samirfns/emu-mut-fklabel.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_fklabel emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-fklabel.log`"
```

## test-samir-fns-fklabel-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-fklabel build/test_samir_fns_fklabel build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_fklabel fklabel host > $(BUILD)/samirfns/mut-fklabel.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-fklabel.log || { cat $(BUILD)/samirfns/mut-fklabel.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_fklabel RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-fklabel.log`"
```

## test-samir-fns-getenv

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) getenv host
@echo '>>> test-samir-fns-getenv: green'
```

## test-samir-fns-getenv-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) getenv emu
@echo '>>> test-samir-fns-getenv-emu: green'
```

## test-samir-fns-getenv-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-getenv-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_getenv.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) getenv emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_getenv.EXE > $(BUILD)/samirfns/emu-mut-getenv.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-getenv.log || { cat $(BUILD)/samirfns/emu-mut-getenv.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_getenv emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-getenv.log`"
```

## test-samir-fns-getenv-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-getenv build/test_samir_fns_getenv build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_getenv getenv host > $(BUILD)/samirfns/mut-getenv.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-getenv.log || { cat $(BUILD)/samirfns/mut-getenv.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_getenv RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-getenv.log`"
```

## test-samir-fns-index

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) index host
@echo '>>> test-samir-fns-index: green'
```

## test-samir-fns-index-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) index emu
@echo '>>> test-samir-fns-index-emu: green'
```

## test-samir-fns-index-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-index-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_index.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) index emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_index.EXE > $(BUILD)/samirfns/emu-mut-index.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-index.log || { cat $(BUILD)/samirfns/emu-mut-index.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_index emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-index.log`"
```

## test-samir-fns-index-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-index build/test_samir_fns_index build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_index index host > $(BUILD)/samirfns/mut-index.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-index.log || { cat $(BUILD)/samirfns/mut-index.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_index RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-index.log`"
```

## test-samir-fns-inkey

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) inkey host
@echo '>>> test-samir-fns-inkey: green'
```

## test-samir-fns-inkey-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) inkey emu
@echo '>>> test-samir-fns-inkey-emu: green'
```

## test-samir-fns-inkey-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-inkey-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_inkey.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) inkey emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_inkey.EXE > $(BUILD)/samirfns/emu-mut-inkey.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-inkey.log || { cat $(BUILD)/samirfns/emu-mut-inkey.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_inkey emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-inkey.log`"
```

## test-samir-fns-inkey-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-inkey build/test_samir_fns_inkey build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_inkey inkey host > $(BUILD)/samirfns/mut-inkey.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-inkey.log || { cat $(BUILD)/samirfns/mut-inkey.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_inkey RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-inkey.log`"
```

## test-samir-fns-iscolor

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) iscolor host
@echo '>>> test-samir-fns-iscolor: green'
```

## test-samir-fns-iscolor-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) iscolor emu
@echo '>>> test-samir-fns-iscolor-emu: green'
```

## test-samir-fns-iscolor-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-iscolor-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_iscolor.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) iscolor emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_iscolor.EXE > $(BUILD)/samirfns/emu-mut-iscolor.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-iscolor.log || { cat $(BUILD)/samirfns/emu-mut-iscolor.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_iscolor emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-iscolor.log`"
```

## test-samir-fns-iscolor-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-iscolor build/test_samir_fns_iscolor build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_iscolor iscolor host > $(BUILD)/samirfns/mut-iscolor.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-iscolor.log || { cat $(BUILD)/samirfns/mut-iscolor.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_iscolor RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-iscolor.log`"
```

## test-samir-fns-lastkey

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) lastkey host
@echo '>>> test-samir-fns-lastkey: green'
```

## test-samir-fns-limitpic

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) limitpic host
@echo '>>> test-samir-fns-limitpic: green'
```

## test-samir-fns-limitpic-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) limitpic emu
@echo '>>> test-samir-fns-limitpic-emu: green'
```

## test-samir-fns-limitpic-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-limitpic-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_limitpic.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) limitpic emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_limitpic.EXE > $(BUILD)/samirfns/emu-mut-limitpic.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-limitpic.log || { cat $(BUILD)/samirfns/emu-mut-limitpic.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_limitpic emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-limitpic.log`"
```

## test-samir-fns-limitpic-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-limitpic build/test_samir_fns_limitpic build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_limitpic limitpic host > $(BUILD)/samirfns/mut-limitpic.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-limitpic.log || { cat $(BUILD)/samirfns/mut-limitpic.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_limitpic RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-limitpic.log`"
```

## test-samir-fns-listoff

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) listoff host
@echo '>>> test-samir-fns-listoff: green'
```

## test-samir-fns-listoff-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) listoff emu
@echo '>>> test-samir-fns-listoff-emu: green'
```

## test-samir-fns-listoff-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-listoff-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_listoff.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) listoff emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_listoff.EXE > $(BUILD)/samirfns/emu-mut-listoff.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-listoff.log || { cat $(BUILD)/samirfns/emu-mut-listoff.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_listoff emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-listoff.log`"
```

## test-samir-fns-listoff-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-listoff build/test_samir_fns_listoff build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_listoff listoff host > $(BUILD)/samirfns/mut-listoff.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-listoff.log || { cat $(BUILD)/samirfns/mut-listoff.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_listoff RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-listoff.log`"
```

## test-samir-fns-log

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) log host
@echo '>>> test-samir-fns-log: green'
```

## test-samir-fns-log-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) log emu
@echo '>>> test-samir-fns-log-emu: green'
```

## test-samir-fns-log-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-log-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_log.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) log emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_log.EXE > $(BUILD)/samirfns/emu-mut-log.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-log.log || { cat $(BUILD)/samirfns/emu-mut-log.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_log emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-log.log`"
```

## test-samir-fns-log-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-log build/test_samir_fns_log build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_log log host > $(BUILD)/samirfns/mut-log.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-log.log || { cat $(BUILD)/samirfns/mut-log.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_log RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-log.log`"
```

## test-samir-fns-logdoarg

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) logdoarg host
@echo '>>> test-samir-fns-logdoarg: green'
```

## test-samir-fns-logdoarg-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) logdoarg emu
@echo '>>> test-samir-fns-logdoarg-emu: green'
```

## test-samir-fns-logdoarg-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-logdoarg-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_logdoarg.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) logdoarg emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_logdoarg.EXE > $(BUILD)/samirfns/emu-mut-logdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-logdoarg.log || { cat $(BUILD)/samirfns/emu-mut-logdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_logdoarg emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-logdoarg.log`"
```

## test-samir-fns-logdoarg-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-logdoarg build/test_samir_fns_logdoarg build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_logdoarg logdoarg host > $(BUILD)/samirfns/mut-logdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-logdoarg.log || { cat $(BUILD)/samirfns/mut-logdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_logdoarg RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-logdoarg.log`"
```

## test-samir-fns-logguards

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) logguards host
@echo '>>> test-samir-fns-logguards: green'
```

## test-samir-fns-logguards-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) logguards emu
@echo '>>> test-samir-fns-logguards-emu: green'
```

## test-samir-fns-logguards-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-logguards-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_logguards.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) logguards emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_logguards.EXE > $(BUILD)/samirfns/emu-mut-logguards.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-logguards.log || { cat $(BUILD)/samirfns/emu-mut-logguards.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_logguards emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-logguards.log`"
```

## test-samir-fns-logguards-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-logguards build/test_samir_fns_logguards build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_logguards logguards host > $(BUILD)/samirfns/mut-logguards.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-logguards.log || { cat $(BUILD)/samirfns/mut-logguards.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_logguards RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-logguards.log`"
```

## test-samir-fns-lupdate

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) lupdate host
@echo '>>> test-samir-fns-lupdate: green'
```

## test-samir-fns-lupdate-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) lupdate emu
@echo '>>> test-samir-fns-lupdate-emu: green'
```

## test-samir-fns-lupdate-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-lupdate-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_lupdate.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) lupdate emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_lupdate.EXE > $(BUILD)/samirfns/emu-mut-lupdate.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-lupdate.log || { cat $(BUILD)/samirfns/emu-mut-lupdate.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_lupdate emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-lupdate.log`"
```

## test-samir-fns-lupdate-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-lupdate build/test_samir_fns_lupdate build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_lupdate lupdate host > $(BUILD)/samirfns/mut-lupdate.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-lupdate.log || { cat $(BUILD)/samirfns/mut-lupdate.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_lupdate RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-lupdate.log`"
```

## test-samir-fns-matrixb

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) matrixb host
@echo '>>> test-samir-fns-matrixb: green'
```

## test-samir-fns-matrixb-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) matrixb emu
@echo '>>> test-samir-fns-matrixb-emu: green'
```

## test-samir-fns-matrixb-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-matrixb-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_matrixb.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) matrixb emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_matrixb.EXE > $(BUILD)/samirfns/emu-mut-matrixb.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-matrixb.log || { cat $(BUILD)/samirfns/emu-mut-matrixb.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_matrixb emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-matrixb.log`"
```

## test-samir-fns-matrixb-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-matrixb build/test_samir_fns_matrixb build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_matrixb matrixb host > $(BUILD)/samirfns/mut-matrixb.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-matrixb.log || { cat $(BUILD)/samirfns/mut-matrixb.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_matrixb RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-matrixb.log`"
```

## test-samir-fns-mutants

Declared tier: outside active vector; 
Prerequisites: `test-samir-fns-alias-mutant test-samir-fns-alternate-mutant test-samir-fns-argorder-mutant test-samir-fns-argpicture-mutant test-samir-fns-col-mutant test-samir-fns-console-mutant test-samir-fns-diskspace-mutant test-samir-fns-division-mutant test-samir-fns-errors-mutant test-samir-fns-exp-mutant test-samir-fns-exppicture-mutant test-samir-fns-exprcol-mutant test-samir-fns-fklabel-mutant test-samir-fns-getenv-mutant test-samir-fns-index-mutant test-samir-fns-inkey-mutant test-samir-fns-iscolor-mutant test-samir-fns-limitpic-mutant test-samir-fns-listoff-mutant test-samir-fns-log-mutant test-samir-fns-logdoarg-mutant test-samir-fns-logguards-mutant test-samir-fns-lupdate-mutant test-samir-fns-matrixb-mutant test-samir-fns-ndx-mutant test-samir-fns-ndxdisplay-mutant test-samir-fns-ndxdoarg-mutant test-samir-fns-numconvert-mutant test-samir-fns-numlist-mutant test-samir-fns-os-mutant test-samir-fns-pcol-mutant test-samir-fns-prow-mutant test-samir-fns-readkey-mutant test-samir-fns-recsize-mutant test-samir-fns-retryconfirm-mutant test-samir-fns-returnarg-mutant test-samir-fns-row-mutant test-samir-fns-sqrt-mutant test-samir-fns-sqrtdoarg-mutant test-samir-fns-sqrtpass-mutant test-samir-fns-struct-mutant test-samir-fns-time-mutant test-samir-fns-type-mutant test-samir-fns-unknown-mutant test-samir-fns-version-mutant test-samir-fns-ndx-deferred-mutant test-samir-fns-ndx-handler-name-mutant test-samir-fns-ndx-handler-depth-mutant build/.makefile.stamp`
```make
```

## test-samir-fns-ndx

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) ndx host
@echo '>>> test-samir-fns-ndx: green'
```

## test-samir-fns-ndx-deferred-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35410):
Prerequisites: `test-samir-fns-ndx build/test_samir_fns_ndx_deferred build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndx_deferred ndx host > $(BUILD)/samirfns/mut-ndx-deferred.log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-deferred.log || { cat $(BUILD)/samirfns/mut-ndx-deferred.log; exit 1; }; \
 printf '>>> SAMIR_FNS_MUTANT_ndx_deferred RED: '; grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-deferred.log
```

## test-samir-fns-ndx-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndx emu
@echo '>>> test-samir-fns-ndx-emu: green'
```

## test-samir-fns-ndx-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndx-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndx emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx.EXE > $(BUILD)/samirfns/emu-mut-ndx.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx.log || { cat $(BUILD)/samirfns/emu-mut-ndx.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx.log`"
```

## test-samir-fns-ndx-handler-depth-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35450):
Prerequisites: `test-samir-fns-ndx-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx_handler_depth.EXE build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndx emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx_handler_depth.EXE > $(BUILD)/samirfns/emu-mut-ndx-handler-depth.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx-handler-depth.log || { cat $(BUILD)/samirfns/emu-mut-ndx-handler-depth.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx_handler_depth emu RED: '; grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx-handler-depth.log
```

## test-samir-fns-ndx-handler-depth-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35450):
Prerequisites: `test-samir-fns-ndx build/test_samir_fns_ndx_handler_depth build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndx_handler_depth ndx host > $(BUILD)/samirfns/mut-ndx-handler-depth.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-handler-depth.log || { cat $(BUILD)/samirfns/mut-ndx-handler-depth.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx_handler_depth RED: '; grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-handler-depth.log
```

## test-samir-fns-ndx-handler-name-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35450):
Prerequisites: `test-samir-fns-ndx-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx_handler_name.EXE build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndx emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndx_handler_name.EXE > $(BUILD)/samirfns/emu-mut-ndx-handler-name.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx-handler-name.log || { cat $(BUILD)/samirfns/emu-mut-ndx-handler-name.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx_handler_name emu RED: '; grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndx-handler-name.log
```

## test-samir-fns-ndx-handler-name-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35450):
Prerequisites: `test-samir-fns-ndx build/test_samir_fns_ndx_handler_name build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndx_handler_name ndx host > $(BUILD)/samirfns/mut-ndx-handler-name.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-handler-name.log || { cat $(BUILD)/samirfns/mut-ndx-handler-name.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx_handler_name RED: '; grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx-handler-name.log
```

## test-samir-fns-ndx-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndx build/test_samir_fns_ndx build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndx ndx host > $(BUILD)/samirfns/mut-ndx.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx.log || { cat $(BUILD)/samirfns/mut-ndx.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndx RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndx.log`"
```

## test-samir-fns-ndxdisplay

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) ndxdisplay host
@echo '>>> test-samir-fns-ndxdisplay: green'
```

## test-samir-fns-ndxdisplay-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndxdisplay emu
@echo '>>> test-samir-fns-ndxdisplay-emu: green'
```

## test-samir-fns-ndxdisplay-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndxdisplay-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndxdisplay.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndxdisplay emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndxdisplay.EXE > $(BUILD)/samirfns/emu-mut-ndxdisplay.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndxdisplay.log || { cat $(BUILD)/samirfns/emu-mut-ndxdisplay.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndxdisplay emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndxdisplay.log`"
```

## test-samir-fns-ndxdisplay-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndxdisplay build/test_samir_fns_ndxdisplay build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndxdisplay ndxdisplay host > $(BUILD)/samirfns/mut-ndxdisplay.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndxdisplay.log || { cat $(BUILD)/samirfns/mut-ndxdisplay.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndxdisplay RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndxdisplay.log`"
```

## test-samir-fns-ndxdoarg

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) ndxdoarg host
@echo '>>> test-samir-fns-ndxdoarg: green'
```

## test-samir-fns-ndxdoarg-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndxdoarg emu
@echo '>>> test-samir-fns-ndxdoarg-emu: green'
```

## test-samir-fns-ndxdoarg-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndxdoarg-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndxdoarg.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) ndxdoarg emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_ndxdoarg.EXE > $(BUILD)/samirfns/emu-mut-ndxdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndxdoarg.log || { cat $(BUILD)/samirfns/emu-mut-ndxdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndxdoarg emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-ndxdoarg.log`"
```

## test-samir-fns-ndxdoarg-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-ndxdoarg build/test_samir_fns_ndxdoarg build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_ndxdoarg ndxdoarg host > $(BUILD)/samirfns/mut-ndxdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndxdoarg.log || { cat $(BUILD)/samirfns/mut-ndxdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_ndxdoarg RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-ndxdoarg.log`"
```

## test-samir-fns-numconvert

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numconvert host
@echo '>>> test-samir-fns-numconvert: green'
```

## test-samir-fns-numconvert-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) numconvert emu
@echo '>>> test-samir-fns-numconvert-emu: green'
```

## test-samir-fns-numconvert-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-numconvert-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_numconvert.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) numconvert emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_numconvert.EXE > $(BUILD)/samirfns/emu-mut-numconvert.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-numconvert.log || { cat $(BUILD)/samirfns/emu-mut-numconvert.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_numconvert emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-numconvert.log`"
```

## test-samir-fns-numconvert-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-numconvert build/test_samir_fns_numconvert build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_numconvert numconvert host > $(BUILD)/samirfns/mut-numconvert.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-numconvert.log || { cat $(BUILD)/samirfns/mut-numconvert.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_numconvert RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-numconvert.log`"
```

## test-samir-fns-numdisplay

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numdisplay host
@echo '>>> test-samir-fns-numdisplay: green'
```

## test-samir-fns-numfmt

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numfmt host
@echo '>>> test-samir-fns-numfmt: green'
```

## test-samir-fns-numfmtwide

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numfmtwide host
@echo '>>> test-samir-fns-numfmtwide: green'
```

## test-samir-fns-numlist

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numlist host
@echo '>>> test-samir-fns-numlist: green'
```

## test-samir-fns-numlist-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) numlist emu
@echo '>>> test-samir-fns-numlist-emu: green'
```

## test-samir-fns-numlist-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-numlist-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_numlist.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) numlist emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_numlist.EXE > $(BUILD)/samirfns/emu-mut-numlist.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-numlist.log || { cat $(BUILD)/samirfns/emu-mut-numlist.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_numlist emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-numlist.log`"
```

## test-samir-fns-numlist-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-numlist build/test_samir_fns_numlist build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_numlist numlist host > $(BUILD)/samirfns/mut-numlist.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-numlist.log || { cat $(BUILD)/samirfns/mut-numlist.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_numlist RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-numlist.log`"
```

## test-samir-fns-numround

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) numround host
@echo '>>> test-samir-fns-numround: green'
```

## test-samir-fns-os

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) os host
@echo '>>> test-samir-fns-os: green'
```

## test-samir-fns-os-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) os emu
@echo '>>> test-samir-fns-os-emu: green'
```

## test-samir-fns-os-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-os-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_os.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) os emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_os.EXE > $(BUILD)/samirfns/emu-mut-os.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-os.log || { cat $(BUILD)/samirfns/emu-mut-os.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_os emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-os.log`"
```

## test-samir-fns-os-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-os build/test_samir_fns_os build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_os os host > $(BUILD)/samirfns/mut-os.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-os.log || { cat $(BUILD)/samirfns/mut-os.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_os RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-os.log`"
```

## test-samir-fns-overflow

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) overflow host
@echo '>>> test-samir-fns-overflow: green'
```

## test-samir-fns-path

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) path host
@echo '>>> test-samir-fns-path: green'
```

## test-samir-fns-pcol

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) pcol host
@echo '>>> test-samir-fns-pcol: green'
```

## test-samir-fns-pcol-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) pcol emu
@echo '>>> test-samir-fns-pcol-emu: green'
```

## test-samir-fns-pcol-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-pcol-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_pcol.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) pcol emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_pcol.EXE > $(BUILD)/samirfns/emu-mut-pcol.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-pcol.log || { cat $(BUILD)/samirfns/emu-mut-pcol.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_pcol emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-pcol.log`"
```

## test-samir-fns-pcol-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-pcol build/test_samir_fns_pcol build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_pcol pcol host > $(BUILD)/samirfns/mut-pcol.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-pcol.log || { cat $(BUILD)/samirfns/mut-pcol.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_pcol RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-pcol.log`"
```

## test-samir-fns-pending

Declared tier: outside active vector; 
Prerequisites: `test-samir-fns-century test-samir-fns-default test-samir-fns-lastkey test-samir-fns-numdisplay test-samir-fns-numfmt test-samir-fns-numfmtwide test-samir-fns-numround test-samir-fns-overflow test-samir-fns-path test-samir-fns-procedure test-samir-fns-relation test-samir-fns-talk test-samir-fns-with build/.makefile.stamp`
```make
```

## test-samir-fns-procedure

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) procedure host
@echo '>>> test-samir-fns-procedure: green'
```

## test-samir-fns-prow

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) prow host
@echo '>>> test-samir-fns-prow: green'
```

## test-samir-fns-prow-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) prow emu
@echo '>>> test-samir-fns-prow-emu: green'
```

## test-samir-fns-prow-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-prow-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_prow.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) prow emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_prow.EXE > $(BUILD)/samirfns/emu-mut-prow.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-prow.log || { cat $(BUILD)/samirfns/emu-mut-prow.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_prow emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-prow.log`"
```

## test-samir-fns-prow-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-prow build/test_samir_fns_prow build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_prow prow host > $(BUILD)/samirfns/mut-prow.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-prow.log || { cat $(BUILD)/samirfns/mut-prow.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_prow RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-prow.log`"
```

## test-samir-fns-readkey

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) readkey host
@echo '>>> test-samir-fns-readkey: green'
```

## test-samir-fns-readkey-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) readkey emu
@echo '>>> test-samir-fns-readkey-emu: green'
```

## test-samir-fns-readkey-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-readkey-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_readkey.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) readkey emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_readkey.EXE > $(BUILD)/samirfns/emu-mut-readkey.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-readkey.log || { cat $(BUILD)/samirfns/emu-mut-readkey.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_readkey emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-readkey.log`"
```

## test-samir-fns-readkey-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-readkey build/test_samir_fns_readkey build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_readkey readkey host > $(BUILD)/samirfns/mut-readkey.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-readkey.log || { cat $(BUILD)/samirfns/mut-readkey.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_readkey RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-readkey.log`"
```

## test-samir-fns-recsize

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) recsize host
@echo '>>> test-samir-fns-recsize: green'
```

## test-samir-fns-recsize-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) recsize emu
@echo '>>> test-samir-fns-recsize-emu: green'
```

## test-samir-fns-recsize-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-recsize-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_recsize.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) recsize emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_recsize.EXE > $(BUILD)/samirfns/emu-mut-recsize.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-recsize.log || { cat $(BUILD)/samirfns/emu-mut-recsize.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_recsize emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-recsize.log`"
```

## test-samir-fns-recsize-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-recsize build/test_samir_fns_recsize build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_recsize recsize host > $(BUILD)/samirfns/mut-recsize.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-recsize.log || { cat $(BUILD)/samirfns/mut-recsize.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_recsize RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-recsize.log`"
```

## test-samir-fns-relation

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) relation host
@echo '>>> test-samir-fns-relation: green'
```

## test-samir-fns-retryconfirm

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) retryconfirm host
@echo '>>> test-samir-fns-retryconfirm: green'
```

## test-samir-fns-retryconfirm-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) retryconfirm emu
@echo '>>> test-samir-fns-retryconfirm-emu: green'
```

## test-samir-fns-retryconfirm-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-retryconfirm-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_retryconfirm.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) retryconfirm emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_retryconfirm.EXE > $(BUILD)/samirfns/emu-mut-retryconfirm.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-retryconfirm.log || { cat $(BUILD)/samirfns/emu-mut-retryconfirm.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_retryconfirm emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-retryconfirm.log`"
```

## test-samir-fns-retryconfirm-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-retryconfirm build/test_samir_fns_retryconfirm build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_retryconfirm retryconfirm host > $(BUILD)/samirfns/mut-retryconfirm.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-retryconfirm.log || { cat $(BUILD)/samirfns/mut-retryconfirm.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_retryconfirm RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-retryconfirm.log`"
```

## test-samir-fns-returnarg

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) returnarg host
@echo '>>> test-samir-fns-returnarg: green'
```

## test-samir-fns-returnarg-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) returnarg emu
@echo '>>> test-samir-fns-returnarg-emu: green'
```

## test-samir-fns-returnarg-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-returnarg-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_returnarg.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) returnarg emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_returnarg.EXE > $(BUILD)/samirfns/emu-mut-returnarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-returnarg.log || { cat $(BUILD)/samirfns/emu-mut-returnarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_returnarg emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-returnarg.log`"
```

## test-samir-fns-returnarg-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-returnarg build/test_samir_fns_returnarg build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_returnarg returnarg host > $(BUILD)/samirfns/mut-returnarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-returnarg.log || { cat $(BUILD)/samirfns/mut-returnarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_returnarg RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-returnarg.log`"
```

## test-samir-fns-row

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) row host
@echo '>>> test-samir-fns-row: green'
```

## test-samir-fns-row-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) row emu
@echo '>>> test-samir-fns-row-emu: green'
```

## test-samir-fns-row-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-row-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_row.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) row emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_row.EXE > $(BUILD)/samirfns/emu-mut-row.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-row.log || { cat $(BUILD)/samirfns/emu-mut-row.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_row emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-row.log`"
```

## test-samir-fns-row-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-row build/test_samir_fns_row build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_row row host > $(BUILD)/samirfns/mut-row.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-row.log || { cat $(BUILD)/samirfns/mut-row.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_row RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-row.log`"
```

## test-samir-fns-sqrt

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) sqrt host
@echo '>>> test-samir-fns-sqrt: green'
```

## test-samir-fns-sqrt-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrt emu
@echo '>>> test-samir-fns-sqrt-emu: green'
```

## test-samir-fns-sqrt-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrt-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrt.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrt emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrt.EXE > $(BUILD)/samirfns/emu-mut-sqrt.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrt.log || { cat $(BUILD)/samirfns/emu-mut-sqrt.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrt emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrt.log`"
```

## test-samir-fns-sqrt-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrt build/test_samir_fns_sqrt build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_sqrt sqrt host > $(BUILD)/samirfns/mut-sqrt.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrt.log || { cat $(BUILD)/samirfns/mut-sqrt.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrt RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrt.log`"
```

## test-samir-fns-sqrtdoarg

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) sqrtdoarg host
@echo '>>> test-samir-fns-sqrtdoarg: green'
```

## test-samir-fns-sqrtdoarg-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrtdoarg emu
@echo '>>> test-samir-fns-sqrtdoarg-emu: green'
```

## test-samir-fns-sqrtdoarg-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrtdoarg-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrtdoarg.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrtdoarg emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrtdoarg.EXE > $(BUILD)/samirfns/emu-mut-sqrtdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrtdoarg.log || { cat $(BUILD)/samirfns/emu-mut-sqrtdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrtdoarg emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrtdoarg.log`"
```

## test-samir-fns-sqrtdoarg-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrtdoarg build/test_samir_fns_sqrtdoarg build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_sqrtdoarg sqrtdoarg host > $(BUILD)/samirfns/mut-sqrtdoarg.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrtdoarg.log || { cat $(BUILD)/samirfns/mut-sqrtdoarg.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrtdoarg RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrtdoarg.log`"
```

## test-samir-fns-sqrtpass

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) sqrtpass host
@echo '>>> test-samir-fns-sqrtpass: green'
```

## test-samir-fns-sqrtpass-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrtpass emu
@echo '>>> test-samir-fns-sqrtpass-emu: green'
```

## test-samir-fns-sqrtpass-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrtpass-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrtpass.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) sqrtpass emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_sqrtpass.EXE > $(BUILD)/samirfns/emu-mut-sqrtpass.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrtpass.log || { cat $(BUILD)/samirfns/emu-mut-sqrtpass.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrtpass emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-sqrtpass.log`"
```

## test-samir-fns-sqrtpass-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-sqrtpass build/test_samir_fns_sqrtpass build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_sqrtpass sqrtpass host > $(BUILD)/samirfns/mut-sqrtpass.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrtpass.log || { cat $(BUILD)/samirfns/mut-sqrtpass.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_sqrtpass RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-sqrtpass.log`"
```

## test-samir-fns-struct

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) struct host
@echo '>>> test-samir-fns-struct: green'
```

## test-samir-fns-struct-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) struct emu
@echo '>>> test-samir-fns-struct-emu: green'
```

## test-samir-fns-struct-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-struct-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_struct.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) struct emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_struct.EXE > $(BUILD)/samirfns/emu-mut-struct.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-struct.log || { cat $(BUILD)/samirfns/emu-mut-struct.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_struct emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-struct.log`"
```

## test-samir-fns-struct-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-struct build/test_samir_fns_struct build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_struct struct host > $(BUILD)/samirfns/mut-struct.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-struct.log || { cat $(BUILD)/samirfns/mut-struct.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_struct RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-struct.log`"
```

## test-samir-fns-talk

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) talk host
@echo '>>> test-samir-fns-talk: green'
```

## test-samir-fns-time

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) time host
@echo '>>> test-samir-fns-time: green'
```

## test-samir-fns-time-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) time emu
@echo '>>> test-samir-fns-time-emu: green'
```

## test-samir-fns-time-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-time-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_time.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) time emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_time.EXE > $(BUILD)/samirfns/emu-mut-time.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-time.log || { cat $(BUILD)/samirfns/emu-mut-time.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_time emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-time.log`"
```

## test-samir-fns-time-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-time build/test_samir_fns_time build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_time time host > $(BUILD)/samirfns/mut-time.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-time.log || { cat $(BUILD)/samirfns/mut-time.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_time RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-time.log`"
```

## test-samir-fns-type

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) type host
@echo '>>> test-samir-fns-type: green'
```

## test-samir-fns-type-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) type emu
@echo '>>> test-samir-fns-type-emu: green'
```

## test-samir-fns-type-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-type-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_type.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) type emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_type.EXE > $(BUILD)/samirfns/emu-mut-type.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-type.log || { cat $(BUILD)/samirfns/emu-mut-type.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_type emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-type.log`"
```

## test-samir-fns-type-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-type build/test_samir_fns_type build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_type type host > $(BUILD)/samirfns/mut-type.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-type.log || { cat $(BUILD)/samirfns/mut-type.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_type RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-type.log`"
```

## test-samir-fns-unknown

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) unknown host
@echo '>>> test-samir-fns-unknown: green'
```

## test-samir-fns-unknown-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) unknown emu
@echo '>>> test-samir-fns-unknown-emu: green'
```

## test-samir-fns-unknown-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-unknown-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_unknown.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) unknown emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_unknown.EXE > $(BUILD)/samirfns/emu-mut-unknown.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-unknown.log || { cat $(BUILD)/samirfns/emu-mut-unknown.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_unknown emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-unknown.log`"
```

## test-samir-fns-unknown-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-unknown build/test_samir_fns_unknown build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_unknown unknown host > $(BUILD)/samirfns/mut-unknown.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-unknown.log || { cat $(BUILD)/samirfns/mut-unknown.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_unknown RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-unknown.log`"
```

## test-samir-fns-version

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) version host
@echo '>>> test-samir-fns-version: green'
```

## test-samir-fns-version-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/samirfns/.promoted build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) version emu
@echo '>>> test-samir-fns-version-emu: green'
```

## test-samir-fns-version-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-version-emu build/SAMIR_SCREEN_SAMIR_FNS_MUTANT_version.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) version emu $(BUILD)/SAMIR_SCREEN_SAMIR_FNS_MUTANT_version.EXE > $(BUILD)/samirfns/emu-mut-version.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-version.log || { cat $(BUILD)/samirfns/emu-mut-version.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_version emu RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/emu-mut-version.log`"
```

## test-samir-fns-version-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35303):
Prerequisites: `test-samir-fns-version build/test_samir_fns_version build/CLIENTS.DBF build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_fns_version version host > $(BUILD)/samirfns/mut-version.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-version.log || { cat $(BUILD)/samirfns/mut-version.log; exit 1; }; printf '>>> SAMIR_FNS_MUTANT_version RED: %s\n' "`grep 'FAIL.*FNS transcript' $(BUILD)/samirfns/mut-version.log`"
```

## test-samir-fns-with

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35314):
Prerequisites: `build/test_samir_fns build/CLIENTS.DBF build/samirfns/.promoted build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) with host
@echo '>>> test-samir-fns-with: green'
```

## test-samir-format-mutants

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35106):
Prerequisites: `build/test_samir_datatype_blankdtoc build/test_samir_datatype_numericfit build/test_samir_datatype_valstar test-xbase-transform-rounding-mutant build/.makefile.stamp`
```make
@for m in $(SAMIR_FMT_MUTANTS); do \
  case $$m in blankdtoc) tag=DTOC;; numericfit) tag=NFIT;; valstar) tag=VALSTAR;; esac; \
  log=$(BUILD)/dt-mut-$$m.log; \
  set +e; $(BUILD)/test_samir_datatype_$$m $$m > $$log 2>&1; rc=$$?; set -e; \
  if [ $$rc -ne 1 ] || ! grep -q 'checks,' $$log || ! grep -q "FAIL.*FMT-$$tag" $$log; then \
    cat $$log; printf '!!! %s mutant did not fail its named check\n' $$m; exit 1; fi; \
  printf '>>> %s correctly RED: ' $$m; grep -m1 "FAIL.*FMT-$$tag" $$log; \
done
```

## test-samir-forms-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 39):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE`
```make
@sh harness/diff/dbf_diff/screen_forms_emu.sh
```

## test-samir-forms-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 45):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_glyph.EXE build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_cursor.EXE build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_clear.EXE build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_wrap.EXE build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_narrow.EXE build/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_subcmd.EXE`
```make
@set -e; for m in $(FORMS_MUTANTS); do k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-forms-mut-$$k.log; \
  if sh harness/diff/dbf_diff/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_$$k.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
  grep -q "^RED: $$want" $$log || { cat $$log; exit 1; }; \
  printf 'SAMIR_FORMS_MUTANT_%s RED: ' $$k; grep -m1 "^RED: $$want" $$log; done
@printf '>>> test-samir-forms-emu-mutant: green (glyph, cursor, clear, wrap, narrow, subcmd RED)\n'
```

## test-samir-full-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29009):
Prerequisites: `build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@case "$@" in *full*) mode=full;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE_EMU) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-full-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29024):
Prerequisites: `build/SAMIR_DBSAFE_full.EXE build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe   # own scratch dir: FLAKY on a fresh parallel worker (initech-hteno)
@if $(TEST_SAMIR_DBSAFE_EMU) full $(BUILD)/SAMIR_DBSAFE_full.EXE > $(BUILD)/dbsafe/emu-mut-full.log 2>&1; then echo '!!! mutant passed'; exit 1; fi
@grep -Fq 'L001 table reopens in guest' $(BUILD)/dbsafe/emu-mut-full.log
@grep -q 'checks,.*failures' $(BUILD)/dbsafe/emu-mut-full.log
@printf '>>> test-samir-full-emu-mutant: green (header-first rewrite correctly RED)\n'
```

## test-samir-high-stack

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 220):
Prerequisites: `build/SAMIR.EXE build/SAMIR.OVL build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/emu/samir_high_stack.sh $(BUILD) $(SAMIR_EXE) clean
```

## test-samir-high-stack-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 222):
Prerequisites: `test-samir-high-stack build/SAMIR_STACK_ARGS.EXE build/.makefile.stamp`
```make
@sh harness/emu/samir_high_stack.sh $(BUILD) $(BUILD)/SAMIR_STACK_ARGS.EXE mutant
```

## test-samir-input

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3683):
Prerequisites: `build/test_samir_input build/.makefile.stamp`
```make
@$(TEST_SAMIR_INPUT)
@printf '>>> test-samir-input: green\n'
```

## test-samir-input-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29184):
Prerequisites: `build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(TEST_SAMIR_AUDIT_EMU) input
@printf '>>> test-samir-input-emu: green\n'
```

## test-samir-input-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29209):
Prerequisites: `build/SAMIR_AUDIT_input.EXE build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@if $(TEST_SAMIR_AUDIT_EMU) input $(BUILD)/SAMIR_AUDIT_input.EXE > $(BUILD)/samir-audit/mut-emu-input.log 2>&1; then echo '!!! input emu mutant passed'; exit 1; fi
@grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-emu-input.log
@grep -Fq 'K13 USE/LIST/QUIT rendered all three rows' $(BUILD)/samir-audit/mut-emu-input.log
@grep -q 'triple_fault=0' $(BUILD)/samir-audit/mut-emu-input.log
@grep -q 'quit_after_found=1' $(BUILD)/samir-audit/mut-emu-input.log
@printf '>>> test-samir-input-emu-mutant: green (multi-command read correctly RED; clean shell return)\n'
```

## test-samir-input-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3727):
Prerequisites: `build/test_samir_input_mut build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@if $(SAMIR_INPUT_MUT) > $(BUILD)/samir-audit/mut-input.log 2>&1; then echo '!!! input mutant passed'; exit 1; fi
@grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-input.log
@grep -Fq 'one logical command, no embedded newline' $(BUILD)/samir-audit/mut-input.log
@printf '>>> test-samir-input-mutant: green (multi-command read correctly RED)\n'
```

## test-samir-lifetime

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3537):
Prerequisites: `build/test_samir_dbsafe build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@case "$@" in *append*) mode=append;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-lifetime-desktop

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29044):
Prerequisites: `build/test_samir_dbsafe_emu build/qemu_harness build/flair_tenants_interactive.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(TEST_SAMIR_DBSAFE_EMU) lifetime-desktop
@printf '>>> test-samir-lifetime-desktop: green\n'
```

## test-samir-lifetime-desktop-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29047):
Prerequisites: `build/test_samir_dbsafe_emu build/qemu_harness build/flair_tenants_interactive.img build/SAMIR_DBSAFE_lifetime.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@if $(TEST_SAMIR_DBSAFE_EMU) lifetime-desktop $(BUILD)/SAMIR_DBSAFE_lifetime.EXE > $(BUILD)/dbsafe/emu-mut-desktop.log 2>&1; then echo '!!! mutant passed'; exit 1; fi
@grep -Fq 'L003 real catalog duplicate-open refusal' $(BUILD)/dbsafe/emu-mut-desktop.log
@grep -q 'checks,.*failures' $(BUILD)/dbsafe/emu-mut-desktop.log
@printf '>>> test-samir-lifetime-desktop-mutant: green (duplicate USE correctly RED)\n'
```

## test-samir-lifetime-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29009):
Prerequisites: `build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@case "$@" in *full*) mode=full;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE_EMU) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-lifetime-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29030):
Prerequisites: `build/SAMIR_DBSAFE_lifetime.EXE build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@if $(TEST_SAMIR_DBSAFE_EMU) lifetime $(BUILD)/SAMIR_DBSAFE_lifetime.EXE > $(BUILD)/dbsafe/emu-mut-lifetime.log 2>&1; then echo '!!! mutant passed'; exit 1; fi
@grep -Fq 'L003 real catalog duplicate-open refusal' $(BUILD)/dbsafe/emu-mut-lifetime.log
@grep -q 'checks,.*failures' $(BUILD)/dbsafe/emu-mut-lifetime.log
@printf '>>> test-samir-lifetime-emu-mutant: green (duplicate USE correctly RED)\n'
```

## test-samir-lifetime-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3562):
Prerequisites: `build/test_samir_lifetime_reset build/test_samir_lifetime_duplicate build/test_samir_lifetime_heap build/test_samir_lifetime_procedure build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@set -e; for kind in reset duplicate heap procedure; do \
  case $$kind in reset) reason='L003 reset cannot cross a live owner';; heap) reason='L003 reclaimed open';; procedure) reason='procedure persistent heap high-water stable';; *) reason='L003 exact duplicate refused';; esac; \
  if $(BUILD)/test_samir_lifetime_$$kind lifetime > $(BUILD)/dbsafe/mut-$$kind.log 2>&1; then echo '!!! mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/dbsafe/mut-$$kind.log; grep -Fq "$$reason" $(BUILD)/dbsafe/mut-$$kind.log; \
  printf '>>> test-samir-lifetime-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-lifetime-mutant: green\n'
```

## test-samir-link-reserve-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 138):
Prerequisites: `build/samir.ld build/samir_link_reserve.o build/samir_link_reserve_big.o build/.makefile.stamp`
```make
@$(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) --defsym=__samir_link_reserve=65536 -o $(BUILD)/samir_link_reserve.elf $(BUILD)/samir_link_reserve.o
@if $(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) --defsym=__samir_link_reserve=65552 -o $(BUILD)/samir_link_reserve_bad.elf $(BUILD)/samir_link_reserve.o >$(BUILD)/samir_link_reserve.log 2>&1; then echo '!!! DEC-03a link reserve above declaration survived'; exit 1; fi
@grep 'link reserve exceeds declared loader reserve/window' $(BUILD)/samir_link_reserve.log
@if $(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) -o $(BUILD)/samir_link_actual_bad.elf $(BUILD)/samir_link_reserve_big.o >$(BUILD)/samir_link_actual.log 2>&1; then echo '!!! DEC-03a actual reserve overflow survived'; exit 1; fi
@grep 'slot + BSS + PAL tail exceed declared loader reserve' $(BUILD)/samir_link_actual.log
@printf '>>> test-samir-link-reserve-mutant: green (link bound > declaration RED, actual reserve overflow RED, equal bound GREEN)\n'
```

## test-samir-list-layout

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34412):
Prerequisites: `build/test_samir_use samir-use-goldens build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir.audit $(BUILD)/samir-audit
@$(TEST_SAMIR_USE) $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-list-layout-host.log 2>&1 || { cat $(BUILD)/samir-list-layout-host.log; exit 1; }
@tail -1 $(BUILD)/samir-list-layout-host.log
@printf '>>> test-samir-list-layout: green (M005U/M005N bytes; OFF/scope order and numlist EOF capture)\n'
```

## test-samir-list-layout-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34438):
Prerequisites: `build/test_samir_list_layout_emu build/qemu_harness build/tracer_secondary.img build/ppm_text_check build/SAMIR.EXE build/SAMIR.OVL samir-use-goldens build/.makefile.stamp`
```make
@$(TEST_SAMIR_LIST_LAYOUT_EMU) files $(SAMIR_EXE) $(abspath $(DBASE3_DECOMP))
@$(TEST_SAMIR_LIST_LAYOUT_EMU) screen $(SAMIR_EXE) $(abspath $(DBASE3_DECOMP))
@printf '>>> test-samir-list-layout-emu: green (9 real tables on serial/disk; M005U console cells)\n'
```

## test-samir-list-layout-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34442):
Prerequisites: `build/test_samir_list_layout_emu build/qemu_harness build/tracer_secondary.img build/ppm_text_check build/SAMIR_SCREEN_SAMIR_LIST_MUTATE_COLUMNS.EXE build/SAMIR_SCREEN_SAMIR_LIST_MUTATE_HEADING.EXE build/SAMIR_SCREEN_SAMIR_LIST_MUTATE_OPENING.EXE build/SAMIR_SCREEN_SAMIR_STREAM_MUTATE_DROP_CRLF.EXE samir-use-goldens build/.makefile.stamp`
```make
@set -e; for kind in COLUMNS HEADING OPENING; do \
  if $(TEST_SAMIR_LIST_LAYOUT_EMU) files $(BUILD)/SAMIR_SCREEN_SAMIR_LIST_MUTATE_$$kind.EXE $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-list-layout-emu-mut-$$kind.log 2>&1; then echo '!!! LIST emu mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/samir-list-layout-emu-mut-$$kind.log; \
  grep -q 'FAIL .*M005U table 0 stdout file byte-identical' $(BUILD)/samir-list-layout-emu-mut-$$kind.log; \
  grep -q 'triple_fault=0' $(BUILD)/samir-list-layout-emu-mut-$$kind.log; \
  grep -q 'quit_after_found=1' $(BUILD)/samir-list-layout-emu-mut-$$kind.log; \
  grep '^\[harness\]' $(BUILD)/samir-list-layout-emu-mut-$$kind.log > $(BUILD)/samir-list-layout-emu-mut-$$kind.report; \
  printf '>>> test-samir-list-layout-emu-mutant: %s correctly RED: ' "$$kind"; \
  grep -m1 'LIST M005U table 0 stdout file differs' $(BUILD)/samir-list-layout-emu-mut-$$kind.log; \
done
@set +e; $(TEST_SAMIR_LIST_LAYOUT_EMU) screen $(BUILD)/SAMIR_SCREEN_SAMIR_STREAM_MUTATE_DROP_CRLF.EXE $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-list-layout-emu-mut-STREAM.log 2>&1; rc=$$?; set -e; \
  test $$rc -eq 1 && grep -q 'screen): .*checks, 1 failures' $(BUILD)/samir-list-layout-emu-mut-STREAM.log && \
  grep -q 'FAIL .*M005U table 0 screen serial byte-identical' $(BUILD)/samir-list-layout-emu-mut-STREAM.log && \
  ! grep -q 'CELL RED' $(BUILD)/samir-list-layout-emu-mut-STREAM.log || { cat $(BUILD)/samir-list-layout-emu-mut-STREAM.log; exit 1; }; \
  printf '>>> SAMIR_STREAM_MUTATE_DROP_CRLF RED: '; grep -m1 'LIST M005U table 0 screen serial differs' $(BUILD)/samir-list-layout-emu-mut-STREAM.log
@printf '>>> test-samir-list-layout-emu-mutant: green (three clean guest returns; wrong bytes and dropped screen CR/LF rejected)\n'
```

## test-samir-list-layout-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34417):
Prerequisites: `build/test_samir_list_layout_COLUMNS build/test_samir_list_layout_HEADING build/test_samir_list_layout_OPENING build/test_samir_list_layout_OFF build/test_samir_list_layout_SCOPE build/test_samir_list_layout_EOF samir-use-goldens build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir.audit $(BUILD)/samir-audit
@set -e; for kind in COLUMNS HEADING OPENING OFF SCOPE EOF; do \
  case $$kind in OFF) reason='LIST NEXT 1 OFF BAL * 2, NAME';; SCOPE|EOF) reason='tnvc numlist real-dBASE golden';; *) reason='M005U real-dBASE golden';; esac; \
  rc=0; $(BUILD)/test_samir_list_layout_$$kind $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-list-layout-mut-$$kind.log 2>&1 || rc=$$?; \
  test $$rc -eq 1 || { cat $(BUILD)/samir-list-layout-mut-$$kind.log; echo '!!! LIST layout mutant did not fail cleanly'; exit 1; }; \
  grep -q 'checks,.*failures' $(BUILD)/samir-list-layout-mut-$$kind.log; \
  grep 'FAIL ' $(BUILD)/samir-list-layout-mut-$$kind.log | grep -Fq "$$reason"; \
  printf '>>> test-samir-list-layout-mutant: %s correctly RED: ' "$$kind"; \
  grep 'FAIL ' $(BUILD)/samir-list-layout-mut-$$kind.log | grep -Fm1 "$$reason"; \
done
@printf '>>> test-samir-list-layout-mutant: green\n'
```

## test-samir-memocells

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35402):
Prerequisites: `build/test_samir_memocells build/w2_fixture build/.makefile.stamp`
```make
@d=$(BUILD)/memocells; rm -rf $$d; mkdir -p $$d/DRIVE $$d/DATA; $(SAMIR_W2_FIXTURE) $$d; cd $$d && $(abspath $(BUILD))/test_samir_memocells
@echo '>>> test-samir-memocells: green'
```

## test-samir-memwidth

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 91):
Prerequisites: `test-dbase-diff test-samir-dml-macro test-samir-repl samir-size-goldens build/test_samir_fns build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(SAMIR_FNS_TEST) memwidth host
@printf '>>> test-samir-memwidth: green (PROG-04/05, DML-02 numeric memvar columns)\n'
```

## test-samir-memwidth-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 100):
Prerequisites: `test-samir-dml-macro-emu samir-size-goldens build/test_samir_fns build/CLIENTS.DBF build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) memwidth emu
@printf '>>> test-samir-memwidth-emu: green (real III+ DML-02 in QEMU)\n'
```

## test-samir-memwidth-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 103):
Prerequisites: `test-samir-memwidth-emu build/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MEMVAR_WIDTH.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-size   # own scratch dir: FAIL on a fresh parallel worker (initech-27clv)
@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) macro emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MEMVAR_WIDTH.EXE > $(BUILD)/samir-size/memwidth-emu-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'FAIL.*DML transcript byte-identical' $(BUILD)/samir-size/memwidth-emu-red.log && \
 ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/samir-size/memwidth-emu-red.log || { cat $(BUILD)/samir-size/memwidth-emu-red.log; exit 1; }; \
 printf 'SAMIR_SIZE_MUTATE_MEMVAR_WIDTH emu RED: '; grep -m1 'FAIL.*DML transcript' $(BUILD)/samir-size/memwidth-emu-red.log
@printf '>>> test-samir-memwidth-emu-mutant: green\n'
```

## test-samir-memwidth-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 94):
Prerequisites: `test-samir-memwidth build/prog_diff_memwidth_mutant build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-size
@set +e; $(BUILD)/prog_diff_memwidth_mutant $(DBASE3_DECOMP) $(PROG_DIFF_DIR) > $(BUILD)/samir-size/memwidth-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^FAIL: flow: stdout matches real III+ golden' $(BUILD)/samir-size/memwidth-red.log || { cat $(BUILD)/samir-size/memwidth-red.log; exit 1; }; \
 printf 'SAMIR_SIZE_MUTATE_MEMVAR_WIDTH RED: '; grep -m1 '^FAIL:' $(BUILD)/samir-size/memwidth-red.log
@printf '>>> test-samir-memwidth-mutant: green\n'
```

## test-samir-messages

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 72):
Prerequisites: `build/test_samir_messages build/samir_msg_pack build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-size
@$(SAMIR_MSG_GEN) spec/samir/dbase_msg_codes.tsv > $(BUILD)/samir-size/messages-regenerated.h
@cmp $(SAMIR_MSG_HEADER) $(BUILD)/samir-size/messages-regenerated.h
@$(BUILD)/test_samir_messages spec/samir/dbase_msg_codes.tsv
@printf '>>> test-samir-messages: green (deterministic packing; independent catalogue decode)\n'
```

## test-samir-messages-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 116):
Prerequisites: `test-samir-core-emu build/.makefile.stamp`
```make
@printf '>>> test-samir-messages-emu: green (exact catalogue messages 76/77 in QEMU)\n'
```

## test-samir-messages-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 118):
Prerequisites: `test-samir-messages-emu build/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MSG_PAIR.EXE build/.makefile.stamp`
```make
@set +e; INITECH_QMP_STDIO=1 $(BUILD)/test_samir_core_emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MSG_PAIR.EXE > $(BUILD)/samir-size/msg-pair-emu-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'FAIL.*guest exact manual error' $(BUILD)/samir-size/msg-pair-emu-red.log && \
 ! grep -q 'FAIL.*guest did not panic' $(BUILD)/samir-size/msg-pair-emu-red.log || { cat $(BUILD)/samir-size/msg-pair-emu-red.log; exit 1; }; \
 printf 'SAMIR_SIZE_MUTATE_MSG_PAIR emu RED: '; grep -m1 'FAIL.*guest exact manual error' $(BUILD)/samir-size/msg-pair-emu-red.log
@printf '>>> test-samir-messages-emu-mutant: green\n'
```

## test-samir-messages-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 78):
Prerequisites: `test-samir-messages build/test_samir_messages_mutant build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_messages_mutant spec/samir/dbase_msg_codes.tsv > $(BUILD)/samir-size/msg-pair-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED SAMIR_MSG_PAIR:' $(BUILD)/samir-size/msg-pair-red.log || { cat $(BUILD)/samir-size/msg-pair-red.log; exit 1; }; \
 head -1 $(BUILD)/samir-size/msg-pair-red.log
@printf '>>> test-samir-messages-mutant: green (SAMIR_SIZE_MUTATE_MSG_PAIR RED)\n'
```

## test-samir-mzlink-io

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 156):
Prerequisites: `build/mzlink build/mzlink_close_fault build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/test_mzlink_io.sh $(MZLINK_BIN) $(BUILD)/mzlink_close_fault $(BUILD)/mzlink-io
```

## test-samir-ndx-names

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35424):
Prerequisites: `build/test_samir_ndx_names build/ndx_fn_builtins.o build/CLIENTS.DBF build/.makefile.stamp`
```make
@$(NM) -u $(BUILD)/ndx_fn_builtins.o > $(BUILD)/ndx-fn-undefined.log
@if grep -Eq '[[:space:]](wa_|ndx_)' $(BUILD)/ndx-fn-undefined.log; then cat $(BUILD)/ndx-fn-undefined.log; exit 1; fi
@echo '>>> test-samir-ndx-link: green (no undefined work-area/index symbols)'
@$(BUILD)/test_samir_ndx_names
@echo '>>> test-samir-ndx-names: green'
```

## test-samir-ndx-ref

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3960):
Prerequisites: `build/.makefile.stamp`
```make
$(call need_goldens,test-samir-ndx-ref)
@printf ">>> test-samir-ndx-ref: independent .ndx B-tree reader vs corpus goldens\n"
@if ! command -v python3 >/dev/null 2>&1 || [ ! -f "$(DBF_DIFF_DIR)/ndx_ref.py" ]; then \
	sh harness/missing_reference.sh REFERENCE_TOOLS "test-samir-ndx-ref: python3/reader absent"; \
else python3 $(DBF_DIFF_DIR)/ndx_ref.py --selftest; fi
@printf ">>> test-samir-ndx-ref: green\n"
```

## test-samir-numeric-format

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33889):
Prerequisites: `build/samir_numeric_format build/.makefile.stamp`
```make
@python3 $(DBF_DIFF_DIR)/numeric_format_ref.py $< $(DBASE3_DECOMP)
@printf '>>> test-samir-numeric-format: green\n'
```

## test-samir-numeric-format-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33939):
Prerequisites: `build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@mode=status; case '$@' in *off-emu) mode=off;; *numeric*) mode=numeric;; *console*) mode=console;; esac; $(BUILD)/test_samir_screen_emu $$mode && \
 cp -f $(BUILD)/samir_screen_$$mode.ppm $(BUILD)/samir-screen/accepted-$$mode.ppm && \
 cp -f $(BUILD)/samir_screen_$$mode.serial $(BUILD)/samir-screen/accepted-$$mode.serial
```

## test-samir-numeric-format-emu-fraction-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33950):
Prerequisites: `build/SAMIR_SCREEN_SAMIR_NUMERIC_MUTATE_FRACTION.EXE build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_screen_emu numeric $< > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && ! grep -Eq 'FAIL.*(guest|trace|shell|artifact|QMP|settings)' $(BUILD)/$@.log && grep -q 'numeric: integral fraction and right alignment' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (same numeric implementation mutant RED in guest)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-numeric-format-emu-left-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33950):
Prerequisites: `build/SAMIR_SCREEN_SAMIR_NUMERIC_MUTATE_LEFT.EXE build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_screen_emu numeric $< > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && ! grep -Eq 'FAIL.*(guest|trace|shell|artifact|QMP|settings)' $(BUILD)/$@.log && grep -q 'numeric: integral fraction and right alignment' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (same numeric implementation mutant RED in guest)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-numeric-format-fraction-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33894):
Prerequisites: `build/samir_numeric_FRACTION build/.makefile.stamp`
```make
@set +e; python3 $(DBF_DIFF_DIR)/numeric_format_ref.py $< $(DBASE3_DECOMP) > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'descriptor differential' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (implementation mutant RED: descriptor differential)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-numeric-format-left-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33894):
Prerequisites: `build/samir_numeric_LEFT build/.makefile.stamp`
```make
@set +e; python3 $(DBF_DIFF_DIR)/numeric_format_ref.py $< $(DBASE3_DECOMP) > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'descriptor differential' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (implementation mutant RED: descriptor differential)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-ovl

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 39):
Prerequisites: `build/test_samir_ovl build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@$(TEST_SAMIR_OVL) $(SAMIR_EXE)
@printf '>>> test-samir-ovl: green (manager pure parts on literal bytes; shipped SAMIR.EXE/SAMIR.OVL pair consistent)\n'
```

## test-samir-ovl-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 82):
Prerequisites: `build/test_samir_ovl_emu build/qemu_harness build/tracer_secondary.img build/SAMIR_REPORT_subtotal.EXE build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@set -e; for m in ok missing stale corrupt; do \
  INITECH_QMP_STDIO=1 $(TEST_SAMIR_OVL_EMU) $$m $(SAMIR_EXE) $(SAMIR_OVL_STALE); done
@printf '>>> test-samir-ovl-emu: green (right SAMIR.OVL runs; missing/stale/corrupt -> Insert System Disk, ENTER retries, Ctrl-C exits 1)\n'
```

## test-samir-ovl-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 88):
Prerequisites: `test-samir-ovl-emu build/test_samir_ovl_emu build/qemu_harness build/tracer_secondary.img build/SAMIR_REPORT_subtotal.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_SKIP_OPEN.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_STAMP.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_CRC.EXE build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-ovl
@set -e; for m in SKIP_OPEN:missing NO_STAMP:stale NO_CRC:corrupt; do k=$${m%%:*}; mode=$${m#*:}; \
  log=$(BUILD)/samir-ovl/emu-mut-$$k.log; \
  set +e; INITECH_QMP_STDIO=1 $(TEST_SAMIR_OVL_EMU) $$mode $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE $(SAMIR_OVL_STALE) > $$log 2>&1; rc=$$?; set -e; \
  test $$rc = 1 && grep -q "FAIL.*$$mode: Insert System Disk prompt shown" $$log && grep -q "\[harness\] OK=1" $$log && grep -q "quit_after_found=1" $$log || { cat $$log; exit 1; }; \
  printf 'SAMIR_OVL_MUTATE_%s (%s) emu RED: ' $$k $$mode; grep -m1 "FAIL.*$$mode: Insert System Disk" $$log; done
@printf '>>> test-samir-ovl-emu-mutant: green (SKIP_OPEN, NO_STAMP, NO_CRC RED)\n'
```

## test-samir-ovl-guard-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 120):
Prerequisites: `build/test_samir_ovl_guard_emu build/qemu_harness build/tracer_secondary.img build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_CALLBACK.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_RAW.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_LIVE.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_HOOK.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_PAL.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_CURSOR.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_ERROR.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_SUSPEND.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_SAY.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_ESCAPE_PRINT.EXE build/.makefile.stamp`
```make
@set -e; for pair in ESCAPE_CALLBACK:fn ESCAPE_RAW:'c->mirror_user' LIVE:'live swap' ESCAPE_HOOK:hook ESCAPE_PAL:'pal->conout' ESCAPE_CURSOR:'v->recno' ESCAPE_ERROR:error ESCAPE_SUSPEND:fn ESCAPE_SAY:'pal->say_screen' ESCAPE_PRINT:'pal->printout'; do \
 k=$${pair%%:*}; why=$${pair#*:}; $(BUILD)/test_samir_ovl_guard_emu $$k $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE "$$why"; done
```

## test-samir-ovl-lifetime

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 131):
Prerequisites: `build/test_samir_ovl_lifetime build/.makefile.stamp`
```make
@$<
@printf '>>> test-samir-ovl-lifetime: green\n'
```

## test-samir-ovl-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 42):
Prerequisites: `test-samir-ovl build/test_samir_ovl_NO_RELOC build/test_samir_ovl_NO_STAMP build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-ovl
@set -e; for m in NO_RELOC:'B2 site 0' NO_STAMP:'B3 STAMP'; do k=$${m%%:*}; want=$${m#*:}; \
  set +e; $(BUILD)/test_samir_ovl_$$k $(SAMIR_EXE) > $(BUILD)/samir-ovl/mut-$$k.log 2>&1; rc=$$?; set -e; \
  test $$rc -ne 0 && grep -q "FAIL.*$$want" $(BUILD)/samir-ovl/mut-$$k.log || { cat $(BUILD)/samir-ovl/mut-$$k.log; exit 1; }; \
  printf 'SAMIR_OVL_MUTATE_%s RED: ' $$k; grep -m1 "FAIL.*$$want" $(BUILD)/samir-ovl/mut-$$k.log; done
@printf '>>> test-samir-ovl-mutant: green (NO_RELOC, NO_STAMP RED)\n'
```

## test-samir-ovl-output

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 139):
Prerequisites: `build/.makefile.stamp`
```make
@set -e; root=$$(mktemp -d $(abspath $(BUILD))/ovl-output-XXXXXX); \
 $(MAKE) --no-print-directory samir-exe BUILD=$$root > $$root/build.log 2>&1; \
 cp -f $$root/SAMIR.EXE $$root/first.EXE; cp -f $$root/SAMIR.OVL $$root/first.OVL; \
 rm -f $$root/SAMIR.OVL; \
 $(MAKE) --no-print-directory $$root/SAMIR.OVL BUILD=$$root > $$root/rebuild.log 2>&1; \
 cmp $$root/SAMIR.EXE $$root/first.EXE; cmp $$root/SAMIR.OVL $$root/first.OVL; \
 printf '>>> test-samir-ovl-output: green (missing OVL rebuilt byte-identical pair)\n'
```

## test-samir-ovl-static

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 52):
Prerequisites: `build/SAMIR.EXE build/SAMIR.OVL build/samir_ovl_pack build/mzlink spec/samir/ovl_format.h spec/samir/memory_contract.h spec/initechmz_extended.h build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-ovl
@cp -f $(BUILD)/samir_exe/SAMIR.elf $(BUILD)/samir-ovl/SAMIR.elf
@$(call samir_pack,$(BUILD)/samir-ovl/SAMIR.elf,$(BUILD)/samir-ovl/REPACK.EXE)
@cmp $(SAMIR_EXE) $(BUILD)/samir-ovl/REPACK.EXE
@cmp $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-ovl/REPACK.OVL
@printf '>>> test-samir-ovl-static: green (relocation kinds, slot, headers, call-graph rule; re-pack byte-identical)\n'
```

## test-samir-ovl-static-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 65):
Prerequisites: `test-samir-ovl-static build/samir_ovl_pack build/mzlink build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-ovl
@set -e; for m in CROSS:'prohibited cross reference from .ovl_report to' \
                  RESREF:'prohibited cross reference from .text to' \
                  REENTER:'<- samir_do <- report_ovl_entry'; do \
  k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-ovl/static-$$k.log; \
  rm -f $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE; \
  if $(MAKE) --no-print-directory $(addprefix -o ,$(SAMIR_OVL_STATIC_FROZEN)) $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE > $$log 2>&1; then \
    printf '!!! test-samir-ovl-static-mutant: %s built -- the check is decoration\n' $$k; exit 1; fi; \
  grep -q -- "$$want" $$log || { cat $$log; exit 1; }; \
  printf 'SAMIR_OVL_MUTATE_%s RED: ' $$k; grep -m1 -- "$$want" $$log; done
@printf '>>> test-samir-ovl-static-mutant: green (CROSS, RESREF refused by the link; REENTER refused by the call-graph rule)\n'
```

## test-samir-ovl-swap-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 97):
Prerequisites: `build/test_samir_dml build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF samir-dml-goldens build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(SAMIR_EXE)
@printf '>>> test-samir-ovl-swap-emu: green (SET/STATUS/FILE overlays alternate; transcript byte-identical to real III+)\n'
```

## test-samir-ovl-swap-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_ovl.mk', line 101):
Prerequisites: `test-samir-ovl-swap-emu build/test_samir_ovl_guard_emu build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_SWAP.EXE build/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_RELOC.EXE build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-ovl
@set -e; for k in NO_SWAP; do log=$(BUILD)/samir-ovl/swap-mut-$$k.log; \
  set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE > $$log 2>&1; rc=$$?; set -e; \
  test $$rc = 1 && grep -q 'FAIL.*DML transcript byte-identical' $$log && grep -q 'quit_after_found=1' $$log && grep -q '\[harness\] OK=1' $$log || { cat $$log; exit 1; }; \
  printf 'SAMIR_OVL_MUTATE_%s swap RED: ' $$k; grep -m1 'FAIL.*DML' $$log; done
@$(BUILD)/test_samir_ovl_guard_emu NO_RELOC $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_RELOC.EXE 'entry outside slot'
@printf '>>> test-samir-ovl-swap-emu-mutant: green (NO_SWAP wrong output with completed shell; NO_RELOC diagnosed by completed kernel-panic trace)\n'
```

## test-samir-pal

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 2370):
Prerequisites: `build/test_samir_pal_contract build/.makefile.stamp`
```make
@printf ">>> test-samir-pal: samir_pal vtable contract complete + linkable (pal_null)\n"
@$(TEST_SAMIR_PAL)
@printf ">>> test-samir-pal: green\n"
```

## test-samir-pal-host

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 2396):
Prerequisites: `build/test_samir_pal_host build/.makefile.stamp`
```make
@printf ">>> test-samir-pal-host: host PAL binding (libc + injectable clock + arena)\n"
@$(TEST_SAMIR_PAL_HOST)
@printf ">>> test-samir-pal-host: green\n"
```

## test-samir-print

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35185):
Prerequisites: `test-samir-dml-clear test-samir-dml-macro test-samir-dml-close test-samir-use build/.makefile.stamp`
```make
@echo '>>> test-samir-print: green (exact ?/?? + mint-005 LIST/DISPLAY)'
```

## test-samir-print-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35191):
Prerequisites: `test-samir-dml-clear-emu test-samir-dml-macro-emu test-samir-dml-close-emu build/.makefile.stamp`
```make
@echo '>>> test-samir-print-emu: green'
```

## test-samir-print-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 248):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-print-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-samir-print-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/samir-print-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/samir-print-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "samir-print-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/samir-print-emu_bochs.report.txt" || true
@cat "$(BUILD)/samir-print-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-samir-print-emu-bochs "$(BUILD)/samir-print-emu_bochs.report.txt" "$(BUILD)/samir-print-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-samir-print-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35193):
Prerequisites: `test-samir-print-emu build/test_samir_dml build/SAMIR_SCREEN_SAMIR_PRINT_MUTATE_LF.EXE build/CLIENTS.DBF`
```make
@if INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) clear emu $(BUILD)/SAMIR_SCREEN_SAMIR_PRINT_MUTATE_LF.EXE > $(BUILD)/samir-print-lf-emu.log 2>&1; then echo 'guest LF mutant survived'; exit 1; fi
@grep -q 'got 0x0a, real 0x0d' $(BUILD)/samir-print-lf-emu.log
@printf '>>> SAMIR_PRINT_MUTATE_LF emu RED: '; grep 'DML transcript differs' $(BUILD)/samir-print-lf-emu.log
```

## test-samir-print-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35187):
Prerequisites: `build/test_samir_print_lf test-samir-print build/CLIENTS.DBF build/.makefile.stamp`
```make
@if $< clear host > $(BUILD)/samir-print-lf.log 2>&1; then echo 'LF mutant survived'; exit 1; fi
@grep -q 'got 0x0a, real 0x0d' $(BUILD)/samir-print-lf.log
@printf '>>> SAMIR_PRINT_MUTATE_LF RED: '; grep 'DML transcript differs' $(BUILD)/samir-print-lf.log
```

## test-samir-query

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3521):
Prerequisites: `build/test_samir_query build/.makefile.stamp`
```make
@printf ">>> test-samir-query: query.c logical echo (.T./.F. vs T/F) + SET DATE/CENTURY + LOCATE RECORD/NEXT\n"
@$(TEST_SAMIR_QUERY) $(DBASE3_DECOMP)
@printf ">>> test-samir-query: green\n"
```

## test-samir-registries

Declared tier: outside active vector; #  recipe to execute (from 'harness/registries.mk', line 8):
Prerequisites: `build/check_registries build/.makefile.stamp`
```make
@$(REGISTRY_CHECK) .
```

## test-samir-registries-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/registries.mk', line 10):
Prerequisites: `test-samir-registries build/.makefile.stamp`
```make
@sh harness/registry_mutants.sh $(REGISTRY_CHECK)
```

## test-samir-repl

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3495):
Prerequisites: `build/test_samir_repl build/.makefile.stamp`
```make
@printf ">>> test-samir-repl: dot-prompt REPL USE/LIST/LOCATE/REPLACE/STORE/SET/DO/QUIT + catalog errors (S5.8)\n"
@$(TEST_SAMIR_REPL) $(DBASE3_DECOMP)
@printf ">>> test-samir-repl: green\n"
```

## test-samir-repl-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3501):
Prerequisites: `build/test_samir_repl_mut build/.makefile.stamp`
```make
@printf ">>> test-samir-repl-mutant: confirming the no-mutate-module mutant goes RED (Rule 6; initech-7az.9)\n"
@$(TEST_SAMIR_REPL_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-samir-repl-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_SAMIR_REPL_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-samir-repl-mutant FAIL: mutant PASSED -- REPL module registration is decoration\n'; exit 1; \
else \
	printf '>>> test-samir-repl-mutant: green (REPLACE-unregistered correctly RED)\n'; \
fi
```

## test-samir-report

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3664):
Prerequisites: `build/test_samir_report samir-report-goldens build/.makefile.stamp`
```make
@$(TEST_SAMIR_REPORT) $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-report.log 2>&1 || { cat $(BUILD)/samir-report.log; exit 1; }
@grep -c 'byte-identical to real dBASE' $(BUILD)/samir-report.log | grep -qx 124
@tail -1 $(BUILD)/samir-report.log
@printf '>>> test-samir-report: green (111 real III+ files; 13 repeated with abbreviated commands)\n'
```

## test-samir-report-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29079):
Prerequisites: `build/test_samir_report_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/samir_report_data.img build/ppm_text_check build/.makefile.stamp`
```make
@$(TEST_SAMIR_REPORT_EMU) files $(SAMIR_EXE) $(abspath $(DBASE3_DECOMP))
@$(TEST_SAMIR_REPORT_EMU) screen $(SAMIR_EXE) $(abspath $(DBASE3_DECOMP))
@printf '>>> test-samir-report-emu: green (guest REPORT FORM files byte-identical to real dBASE; screendump cells match)\n'
```

## test-samir-report-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29083):
Prerequisites: `build/test_samir_report_emu build/qemu_harness build/tracer_secondary.img build/SAMIR_REPORT_subtotal.EXE build/samir_report_data.img build/.makefile.stamp`
```make
@if $(TEST_SAMIR_REPORT_EMU) files $(SAMIR_REPORT_MUT_EXE) $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-report-emu-mut.log 2>&1; then echo '!!! report emu mutant passed'; exit 1; fi
@grep -q 'checks,.*failures' $(BUILD)/samir-report-emu-mut.log
@grep -Fq 'REPORT G8TP.TXT: guest file differs' $(BUILD)/samir-report-emu-mut.log
@grep -q 'triple_fault=0' $(BUILD)/samir-report-emu-mut.log
@grep -Fq 'G1FLT.TXT  guest file byte-identical' $(BUILD)/samir-report-emu-mut.log
@printf '>>> test-samir-report-emu-mutant: green (dropped subtotal band correctly RED in the guest: G8TP.TXT differs, G1FLT.TXT still identical, no crash)\n'
```

## test-samir-report-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3669):
Prerequisites: `build/test_samir_report_subtotal build/test_samir_report_column build/test_samir_report_page samir-report-goldens build/.makefile.stamp`
```make
@set -e; for kind in subtotal column page; do \
  case $$kind in subtotal) reason='REPORT G8TP.TXT: differs';; column) reason='REPORT G1FLT.TXT: differs';; page) reason='REPORT G2TRV.TXT: differs';; esac; \
  if $(BUILD)/test_samir_report_$$kind $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-report-mut-$$kind.log 2>&1; then echo '!!! report mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/samir-report-mut-$$kind.log; \
  grep -Fq "$$reason" $(BUILD)/samir-report-mut-$$kind.log; \
  printf '>>> test-samir-report-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-report-mutant: green\n'
```

## test-samir-rt

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 2390):
Prerequisites: `build/test_samir_rt build/.makefile.stamp`
```make
@printf ">>> test-samir-rt: freestanding rt (JDN, dec_format ties->+inf, mem/str)\n"
@$(TEST_SAMIR_RT)
@printf ">>> test-samir-rt: green\n"
```

## test-samir-safety

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3537):
Prerequisites: `build/test_samir_dbsafe build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@case "$@" in *append*) mode=append;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-safety-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29009):
Prerequisites: `build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@case "$@" in *full*) mode=full;; *lifetime*) mode=lifetime;; *) mode=safety;; esac; $(TEST_SAMIR_DBSAFE_EMU) $$mode
@printf '>>> %s: green\n' '$@'
```

## test-samir-safety-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29035):
Prerequisites: `build/SAMIR_DBSAFE_safety.EXE build/test_samir_dbsafe_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@if $(TEST_SAMIR_DBSAFE_EMU) safety $(BUILD)/SAMIR_DBSAFE_safety.EXE > $(BUILD)/dbsafe/emu-mut-safety.log 2>&1; then echo '!!! mutant passed'; exit 1; fi
@grep -Fq 'L007 N preserves all records' $(BUILD)/dbsafe/emu-mut-safety.log
@grep -q 'checks,.*failures' $(BUILD)/dbsafe/emu-mut-safety.log
@printf '>>> test-samir-safety-emu-mutant: green (missing confirmation correctly RED)\n'
```

## test-samir-safety-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3575):
Prerequisites: `build/test_samir_safety_mut build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dbsafe
@if $< safety > $(BUILD)/dbsafe/mut-safety.log 2>&1; then echo '!!! mutant passed'; exit 1; fi
@grep -Fq 'L007 declined ZAP byte-identical' $(BUILD)/dbsafe/mut-safety.log
@grep -q 'checks,.*failures' $(BUILD)/dbsafe/mut-safety.log
@printf '>>> test-samir-safety-mutant: green (missing confirmation correctly RED)\n'
```

## test-samir-say

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35253):
Prerequisites: `build/test_samir_say build/.makefile.stamp`
```make
@$<
@printf ">>> test-samir-say: green\n"
```

## test-samir-scratch-lock

Declared tier: HOST; #  recipe to execute (from 'harness/merge_samir.mk', line 51):
Prerequisites: `build/test_samir_scratch_lock build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/test_scratch_lock.sh $<
```

## test-samir-set-color

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3311):
Prerequisites: `build/test_samir_set_color build/.makefile.stamp`
```make
@$<
@echo '>>> test-samir-set-color: green'
```

## test-samir-set-color-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3314):
Prerequisites: `test-samir-set-color build/test_samir_set_color_mutant build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_set_color_mutant > $(BUILD)/set-color-mutant.log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'FAIL.*SET COLOR standard attribute reaches PAL' $(BUILD)/set-color-mutant.log || { cat $(BUILD)/set-color-mutant.log; exit 1; }; \
 printf '>>> SET_COLOR_MUTANT_BG RED: '; grep -m1 'FAIL.*SET COLOR standard attribute reaches PAL' $(BUILD)/set-color-mutant.log
@printf '>>> test-samir-set-color-mutant: green (SET_COLOR_MUTANT_BG rejected)\n'
```

## test-samir-set-headings

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35364):
Prerequisites: `test-samir-w2-set-headings build/.makefile.stamp`
```make
@echo '>>> test-samir-set-headings: green (ACCEPTED vs real III+ capture)'
```

## test-samir-set-headings-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35366):
Prerequisites: `test-samir-set-headings build/test_samir_w2_setheadings_mut build/.makefile.stamp`
```make
@set +e; $(SAMIR_W2_SETHEADINGS_MUT) set-headings host > $(BUILD)/w2-set-headings-mut.log 2>&1; rc=$$?; set -e; \
 test $$rc -ne 0 && grep -q 'W2 transcript' $(BUILD)/w2-set-headings-mut.log || { cat $(BUILD)/w2-set-headings-mut.log; echo 'mutant survived'; exit 1; }; \
 echo '>>> test-samir-set-headings-mutant: RED as required'
```

## test-samir-size

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 12):
Prerequisites: `build/test_samir_size build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@$(SAMIR_SIZE_TEST) $(SAMIR_SIZE_IMAGE)
@printf '>>> test-samir-size: green (DEC-03a.8: module <=256 KiB, declared high footprint/heap; slot == largest overlay)\n'
```

## test-samir-size-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 56):
Prerequisites: `test-samir-boot test-samir-write test-samir-canon-y2k build/.makefile.stamp`
```make
@printf '>>> test-samir-size-emu: green (boot, persisted writes, canon replay)\n'
```

## test-samir-size-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 58):
Prerequisites: `test-samir-boot-mutant test-samir-write-mutant test-samir-canon-y2k-mutant build/.makefile.stamp`
```make
@printf '>>> test-samir-size-emu-mutant: green (PAL short-read, dropped write, DO-file truncation RED)\n'
```

## test-samir-size-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 19):
Prerequisites: `test-samir-size build/mzlink build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-size
@set -e; for v in red:262145 floor:262144 above-old:200000; do n=$${v%%:*}; bytes=$${v#*:}; \
   cp -f $(SAMIR_SIZE_RES) $(BUILD)/samir-size/$$n.res; \
   truncate -s $$bytes $(BUILD)/samir-size/$$n.res; \
   $(MZLINK_BIN) $(BUILD)/samir-size/$$n.res $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.EXE --header-relocs --samir 2>/dev/null; \
   cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.OVL; \
 done; \
 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_red.EXE > $(BUILD)/samir-size/pad-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED SAMIR_SIZE:' $(BUILD)/samir-size/pad-red.log || { cat $(BUILD)/samir-size/pad-red.log; exit 1; }; \
 cat $(BUILD)/samir-size/pad-red.log
@$(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_above-old.EXE
@$(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_floor.EXE
@set -e; cp -f $(SAMIR_EXE) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE; \
 cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL; \
 slot=$$(od -An -tu4 -j16 -N4 $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL | tr -d ' '); slot=$$(( slot + 16 )); \
 printf "$$(printf '\\%03o\\%03o\\%03o\\%03o' $$(( slot & 255 )) $$(( (slot >> 8) & 255 )) $$(( (slot >> 16) & 255 )) $$(( slot >> 24 )))" | \
   dd of=$(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL bs=1 seek=16 conv=notrunc status=none; \
 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE > $(BUILD)/samir-size/slot-red.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED SAMIR_SLOT:' $(BUILD)/samir-size/slot-red.log || { cat $(BUILD)/samir-size/slot-red.log; exit 1; }; \
 cat $(BUILD)/samir-size/slot-red.log
@printf '>>> test-samir-size-mutant: green (DEC-03a.8 ceiling+1 RED; ceiling and above-old GREEN; SAMIR_SIZE_SLOT RED)\n'
```

## test-samir-size-repro

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_size.mk', line 43):
Prerequisites: `build/SAMIR.EXE build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-size
@set -e; root=$$(mktemp -d $(abspath $(BUILD))/samir-size/repro-XXXXXX); \
 $(MAKE) --no-print-directory -B samir-exe BUILD=$$root/first > $$root/first.log 2>&1; \
 $(MAKE) --no-print-directory -B samir-exe BUILD=$$root/second > $$root/second.log 2>&1; \
 cmp $$root/first/SAMIR.EXE $$root/second/SAMIR.EXE; \
 cmp $$root/first/SAMIR.OVL $$root/second/SAMIR.OVL; \
 cmp $(SAMIR_EXE) $$root/first/SAMIR.EXE; \
 cmp $(basename $(SAMIR_EXE)).OVL $$root/first/SAMIR.OVL
@printf '>>> test-samir-size-repro: green (two clean builds byte-identical): '; sha256sum $(SAMIR_EXE) $(SAMIR_OVL)
```

## test-samir-softfp

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28853):
Prerequisites: `build/test_samir_softfp build/.makefile.stamp`
```make
@printf '>>> test-samir-softfp: softfp.c 18 helpers vs host double (DEC-02 / Law 2)\n'
@$(TEST_SAMIR_SOFTFP)
```

## test-samir-softfp-implmutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28869):
Prerequisites: `build/test_samir_softfp_implmutant build/.makefile.stamp`
```make
@if $(TEST_SAMIR_SOFTFP_IMPLMUT) >/dev/null 2>&1; then printf '!!! test-samir-softfp-implmutant FAIL: round-tie mutant PASSED -- oracle is decoration\n'; exit 1; \
else printf '>>> test-samir-softfp-implmutant: green (SOFTFP_MUT_ROUND correctly RED -- implementation-side mutant, oracle bites)\n'; fi
```

## test-samir-softfp-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28856):
Prerequisites: `build/test_samir_softfp_mut build/.makefile.stamp`
```make
@if $(TEST_SAMIR_SOFTFP_MUT) >/dev/null 2>&1; then printf '!!! test-samir-softfp-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; \
else printf '>>> test-samir-softfp-mutant: green (mutant correctly RED)\n'; fi
```

## test-samir-spec

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 2402):
Prerequisites: `build/test_samir_spec build/.makefile.stamp`
```make
@printf ">>> test-samir-spec: spec/samir locked data consistent (III+-only; no ==; 0x1C/0x1F NORMALIZE)\n"
@$(TEST_SAMIR_SPEC) $(SAMIR_SPEC_DIR)
@printf ">>> test-samir-spec: green\n"
```

## test-samir-status

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33919):
Prerequisites: `build/test_samir_status build/.makefile.stamp`
```make
@$< $(DBASE3_DECOMP)
```

## test-samir-status-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33939):
Prerequisites: `build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@mode=status; case '$@' in *off-emu) mode=off;; *numeric*) mode=numeric;; *console*) mode=console;; esac; $(BUILD)/test_samir_screen_emu $$mode && \
 cp -f $(BUILD)/samir_screen_$$mode.ppm $(BUILD)/samir-screen/accepted-$$mode.ppm && \
 cp -f $(BUILD)/samir_screen_$$mode.serial $(BUILD)/samir-screen/accepted-$$mode.serial
```

## test-samir-status-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33943):
Prerequisites: `build/SAMIR_SCREEN_SAMIR_MUTATE_STATUS.EXE build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_screen_emu status $< > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && ! grep -Eq 'FAIL.*(guest|trace|shell|artifact|QMP|settings|heading)' $(BUILD)/$@.log && grep -q 'status: serial Command Line' $(BUILD)/$@.log && grep -q 'status: reverse band' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (same status-band mutant RED on serial and video)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-status-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33921):
Prerequisites: `build/test_samir_status_mutant build/.makefile.stamp`
```make
@set +e; $< $(DBASE3_DECOMP) > $(BUILD)/$@.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'status: Command Line' $(BUILD)/$@.log && grep -q 'checks,' $(BUILD)/$@.log || { cat $(BUILD)/$@.log; exit 1; }; \
 printf '%s: PASS (status-band implementation mutant RED)\n' '$@'
@printf '>>> %s: green\n' '$@'
```

## test-samir-status-off-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33939):
Prerequisites: `build/test_samir_screen_emu build/qemu_harness build/tracer_secondary.img build/samir_list.img build/ppm_text_check build/.makefile.stamp`
```make
@mode=status; case '$@' in *off-emu) mode=off;; *numeric*) mode=numeric;; *console*) mode=console;; esac; $(BUILD)/test_samir_screen_emu $$mode && \
 cp -f $(BUILD)/samir_screen_$$mode.ppm $(BUILD)/samir-screen/accepted-$$mode.ppm && \
 cp -f $(BUILD)/samir_screen_$$mode.serial $(BUILD)/samir-screen/accepted-$$mode.serial
```

## test-samir-status-screen-pending

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 78):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/SAMIR.OVL build/.makefile.stamp`
```make
@sh $(DBF_DIFF_DIR)/screen_status_pending.sh
```

## test-samir-trace

Declared tier: HOST; #  recipe to execute (from 'harness/merge_samir.mk', line 72):
Prerequisites: `build/tracer_secondary.img build/samir_list_mut.img build/.makefile.stamp`
```make
+@MAKE="$(MAKE)" TRACE_ASSUME_OLD="$(TRACE_ASSUME_OLD)" sh tools/test-emu-trace.sh
```

## test-samir-undefined

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34468):
Prerequisites: `test-samir-use build/.makefile.stamp`
```make
@printf '>>> test-samir-undefined: green (manual #12 transcript; ON ERROR / ERROR())\n'
```

## test-samir-undefined-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34478):
Prerequisites: `build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(TEST_SAMIR_AUDIT_EMU) undefined
@printf '>>> test-samir-undefined-emu: green (typed dot prompt; ON ERROR / ERROR(); clean shell return)\n'
```

## test-samir-undefined-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34481):
Prerequisites: `build/SAMIR_AUDIT_undefined.EXE build/test_samir_audit_emu build/qemu_harness build/tracer_secondary.img build/CLIENTS.DBF build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit
@if INITECH_QMP_STDIO=1 $(TEST_SAMIR_AUDIT_EMU) undefined $(BUILD)/SAMIR_AUDIT_undefined.EXE > $(BUILD)/samir-audit/mut-emu-undefined.log 2>&1; then echo '!!! undefined emu error-1 mutant passed'; exit 1; fi
@grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-emu-undefined.log
@grep -Fq 'kdna exact guest dot-prompt error transcript' $(BUILD)/samir-audit/mut-emu-undefined.log
@grep -q 'triple_fault=0' $(BUILD)/samir-audit/mut-emu-undefined.log
@grep -q 'quit_after_found=1' $(BUILD)/samir-audit/mut-emu-undefined.log
@printf '>>> test-samir-undefined-emu-mutant: SAMIR_MUTATE_UNDEFINED_ERROR1 correctly RED: '
@grep -F 'FAIL' $(BUILD)/samir-audit/mut-emu-undefined.log | grep -F 'kdna exact guest dot-prompt error transcript'
@printf '>>> test-samir-undefined-emu-mutant: green (clean guest return)\n'
```

## test-samir-undefined-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34470):
Prerequisites: `build/test_samir_undefined_error1 samir-use-goldens build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@if $(SAMIR_UNDEFINED_MUT) $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-audit/mut-undefined.log 2>&1; then echo '!!! undefined error-1 mutant passed'; exit 1; fi
@grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-undefined.log
@grep -Fq 'kdna exact dot-prompt error transcript' $(BUILD)/samir-audit/mut-undefined.log
@printf '>>> test-samir-undefined-mutant: SAMIR_MUTATE_UNDEFINED_ERROR1 correctly RED: '
@grep -F 'FAIL' $(BUILD)/samir-audit/mut-undefined.log | grep -F 'kdna exact dot-prompt error transcript'
@printf '>>> test-samir-undefined-mutant: green\n'
```

## test-samir-use

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3602):
Prerequisites: `build/test_samir_use samir-use-goldens build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@$(TEST_SAMIR_USE) $(abspath $(DBASE3_DECOMP))
@printf '>>> test-samir-use: green (incl. mint 005 USE INDEX / NEXT n replays vs real dBASE III PLUS 1.1)\n'
```

## test-samir-use-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3714):
Prerequisites: `build/test_samir_use_extension build/test_samir_use_error build/test_samir_use_ndxext build/test_samir_use_indextop build/test_samir_use_nextpast build/test_samir_use_setstale build/test_samir_use_procstale samir-use-goldens build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@set -e; for kind in extension error ndxext indextop nextpast setstale procstale; do \
  case $$kind in extension) reason='K09 short USE defaults to .DBF';; error) reason='K09 missing file is';; \
    ndxext) reason='samir: 1  File does not exist.';; indextop) reason='samir: [U1] 1 CI10Buckman notEOF';; \
    nextpast) reason='samir: [N1] 32 notEOF';; setstale) reason='x5cc SET SAFETY does not leak';; \
    procstale) reason='x5cc ON ERROR trap does not leak';; esac; \
  if $(BUILD)/test_samir_use_$$kind $(abspath $(DBASE3_DECOMP)) > $(BUILD)/samir-audit/mut-$$kind.log 2>&1; then echo '!!! mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-$$kind.log; \
  grep -Fq "$$reason" $(BUILD)/samir-audit/mut-$$kind.log; \
  printf '>>> test-samir-use-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-use-mutant: green\n'
```

## test-samir-value

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 2412):
Prerequisites: `build/test_samir_value build/.makefile.stamp`
```make
@printf ">>> test-samir-value: xb_val typed value (C/N/D/L/M/U) + xb_typeof/xb_eq\n"
@$(TEST_SAMIR_VALUE)
@printf ">>> test-samir-value: green\n"
```

## test-samir-view

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3590):
Prerequisites: `build/test_samir_view build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@$(TEST_SAMIR_VIEW)
@printf '>>> test-samir-view: green\n'
```

## test-samir-view-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3704):
Prerequisites: `build/test_samir_view_filter build/test_samir_view_deleted build/test_samir_view_marker build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samir-audit $(BUILD)/samir.audit
@set -e; for kind in filter deleted marker; do \
  case $$kind in filter) reason='K05 hidden row absent';; deleted) reason='K06 deleted and filter compose';; marker) reason='K06 marker precedes deleted record';; esac; \
  if $(BUILD)/test_samir_view_$$kind > $(BUILD)/samir-audit/mut-$$kind.log 2>&1; then echo '!!! mutant passed'; exit 1; fi; \
  grep -q 'checks,.*failures' $(BUILD)/samir-audit/mut-$$kind.log; \
  grep -Fq "$$reason" $(BUILD)/samir-audit/mut-$$kind.log; \
  printf '>>> test-samir-view-mutant: %s correctly RED (%s)\n' "$$kind" "$$reason"; \
done
@printf '>>> test-samir-view-mutant: green\n'
```

## test-samir-w2-history

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_w2_history.mk', line 10):
Prerequisites: `test-samir-w2-set-history test-samir-w2-set-dohistory test-samir-w2-set-historyresize test-samir-w2-set-historyslots build/.makefile.stamp`
```make
@echo '>>> test-samir-w2-history: green (history, dohistory real III+ captures)'
```

## test-samir-w2-history-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/samir_w2_history.mk', line 12):
Prerequisites: `build/test_samir_w2 harness/diff/dbf_diff/test_samir_fns.c os/samir/cmd/set.c build/.makefile.stamp | build`
```make
@set -e; for m in $(SAMIR_W2_HISTORY_MUTANTS); do \
 $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_W2 -DSAMIR_W2_MUTANT_$$m -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $(BUILD)/test_samir_w2_mut_$$m \
  $(DBF_DIFF_DIR)/test_samir_fns.c $(DBF_DIFF_DIR)/w2_host.c $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) ; \
 red=0; for c in $(SAMIR_W2_HISTORY_CASES); do \
  set +e; $(BUILD)/test_samir_w2_mut_$$m $$c host > $(BUILD)/samirw2-mut-$$m-$$c.log 2>&1; rc=$$?; set -e; \
  if [ $$rc -ne 0 ] && grep -q 'FAIL.*transcript' $(BUILD)/samirw2-mut-$$m-$$c.log; then red=1; fi; \
 done; \
 test $$red -eq 1 || { echo "!!! SAMIR_W2_MUTANT_$$m stayed green: the history goldens are decoration"; exit 1; }; \
 echo ">>> SAMIR_W2_MUTANT_$$m RED"; \
done
```

## test-samir-w2-host-adapter

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35334):
Prerequisites: `build/test_w2_host build/.makefile.stamp`
```make
@$<
```

## test-samir-w2-pending

Declared tier: outside active vector; 
Prerequisites: `test-samir-w2-set-catalog test-samir-w2-set-default test-samir-w2-set-dohistory test-samir-w2-set-fields test-samir-w2-set-fixed test-samir-w2-set-historygrow test-samir-w2-set-historyresize test-samir-w2-set-historysame test-samir-w2-set-historyslots test-samir-w2-set-headings test-samir-w2-set-history test-samir-w2-set-margin test-samir-w2-set-memowidth test-samir-w2-set-path test-samir-w2-set-print test-samir-w2-set-procedure test-samir-w2-set-unique test-samir-w2-set-view test-samir-w2-verb-average test-samir-w2-verb-count test-samir-w2-verb-dir test-samir-w2-verb-eject test-samir-w2-verb-insert test-samir-w2-verb-join test-samir-w2-verb-sum test-samir-w2-verb-text test-samir-w2-verb-update build/.makefile.stamp`
```make
```

## test-samir-w2-set-catalog

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/catalog && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-catalog host
@echo '>>> test-samir-w2-set-catalog: green'
```

## test-samir-w2-set-default

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/default && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-default host
@echo '>>> test-samir-w2-set-default: green'
```

## test-samir-w2-set-dohistory

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/dohistory && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-dohistory host
@echo '>>> test-samir-w2-set-dohistory: green'
```

## test-samir-w2-set-fields

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/fields && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-fields host
@echo '>>> test-samir-w2-set-fields: green'
```

## test-samir-w2-set-fixed

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/fixed && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-fixed host
@echo '>>> test-samir-w2-set-fixed: green'
```

## test-samir-w2-set-fixed-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w2_set_fixed.mk', line 8):
Prerequisites: `test-samir-w2-set-fixed build/test_samir_w2_mutant_set_fixed build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_w2_mutant_set_fixed set-fixed host > $(BUILD)/samir-w2-mut-set-fixed.log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'FAIL.*W2 transcript' $(BUILD)/samir-w2-mut-set-fixed.log || { cat $(BUILD)/samir-w2-mut-set-fixed.log; exit 1; }; \
 printf '>>> SAMIR_W2_MUTANT_set_fixed RED: '; grep 'FAIL.*W2 transcript' $(BUILD)/samir-w2-mut-set-fixed.log
```

## test-samir-w2-set-headings

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/headings && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-headings host
@echo '>>> test-samir-w2-set-headings: green'
```

## test-samir-w2-set-history

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/history && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-history host
@echo '>>> test-samir-w2-set-history: green'
```

## test-samir-w2-set-historygrow

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/historygrow && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-historygrow host
@echo '>>> test-samir-w2-set-historygrow: green'
```

## test-samir-w2-set-historyresize

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/historyresize && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-historyresize host
@echo '>>> test-samir-w2-set-historyresize: green'
```

## test-samir-w2-set-historysame

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/historysame && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-historysame host
@echo '>>> test-samir-w2-set-historysame: green'
```

## test-samir-w2-set-historyslots

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/historyslots && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-historyslots host
@echo '>>> test-samir-w2-set-historyslots: green'
```

## test-samir-w2-set-margin

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/margin && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-margin host
@echo '>>> test-samir-w2-set-margin: green'
```

## test-samir-w2-set-margin-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35378):
Prerequisites: `test-samir-w2-set-margin build/test_samir_w2_set_margin_mutant build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/samirfns; set +e; $(BUILD)/test_samir_w2_set_margin_mutant set-margin host > $(BUILD)/samirfns/mut-w2-set-margin.log 2>&1; rc=$$?; set -e; \
 test $$rc -ne 0 && grep -q 'FAIL.*W2' $(BUILD)/samirfns/mut-w2-set-margin.log || { cat $(BUILD)/samirfns/mut-w2-set-margin.log; exit 1; }; \
 printf '>>> SAMIR_W2_MUTANT_set_margin RED: %s\n' "`grep 'FAIL.*W2' $(BUILD)/samirfns/mut-w2-set-margin.log | head -1`"
```

## test-samir-w2-set-memowidth

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/memowidth && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-memowidth host
@echo '>>> test-samir-w2-set-memowidth: green'
```

## test-samir-w2-set-memowidth-mutant-ignore

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35396):
Prerequisites: `test-samir-w2-set-memowidth build/test_samir_w2_mut_memowidth_ignore build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_w2_mut_memowidth_ignore set-memowidth host > $(BUILD)/test_samir_w2_mut_memowidth_ignore.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*W2 transcript' $(BUILD)/test_samir_w2_mut_memowidth_ignore.log || { cat $(BUILD)/test_samir_w2_mut_memowidth_ignore.log; exit 1; }; printf '>>> SAMIR_W2_MUTANT_memowidth_ignore RED: '; grep 'FAIL.*W2 transcript' $(BUILD)/test_samir_w2_mut_memowidth_ignore.log
```

## test-samir-w2-set-memowidth-mutant-nowrap256

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35396):
Prerequisites: `test-samir-w2-set-memowidth build/test_samir_w2_mut_memowidth_nowrap256 build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_samir_w2_mut_memowidth_nowrap256 set-memowidth host > $(BUILD)/test_samir_w2_mut_memowidth_nowrap256.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*W2 transcript' $(BUILD)/test_samir_w2_mut_memowidth_nowrap256.log || { cat $(BUILD)/test_samir_w2_mut_memowidth_nowrap256.log; exit 1; }; printf '>>> SAMIR_W2_MUTANT_memowidth_nowrap256 RED: '; grep 'FAIL.*W2 transcript' $(BUILD)/test_samir_w2_mut_memowidth_nowrap256.log
```

## test-samir-w2-set-memowidth-mutants

Declared tier: HOST; 
Prerequisites: `test-samir-w2-set-memowidth-mutant-ignore test-samir-w2-set-memowidth-mutant-nowrap256 build/.makefile.stamp`
```make
```

## test-samir-w2-set-path

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/path && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-path host
@echo '>>> test-samir-w2-set-path: green'
```

## test-samir-w2-set-print

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/print && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-print host
@echo '>>> test-samir-w2-set-print: green'
```

## test-samir-w2-set-procedure

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/procedure && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-procedure host
@echo '>>> test-samir-w2-set-procedure: green'
```

## test-samir-w2-set-unique

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/unique && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-unique host
@echo '>>> test-samir-w2-set-unique: green'
```

## test-samir-w2-set-uniquerebuild

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/w2_set_unique.mk', line 6):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/uniquerebuild && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-uniquerebuild host
@echo '>>> test-samir-w2-set-uniquerebuild: green'
```

## test-samir-w2-set-view

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/set-mint/view && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) set-view host
@echo '>>> test-samir-w2-set-view: green'
```

## test-samir-w2-unique

Declared tier: HOST; 
Prerequisites: `test-samir-w2-set-unique test-samir-w2-set-uniquerebuild samir-w2-unique-goldens build/.makefile.stamp`
```make
```

## test-samir-w2-unique-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w2_set_unique.mk', line 16):
Prerequisites: `test-samir-w2-unique build/.makefile.stamp`
```make
@set -e; mkdir -p $(BUILD)/samirw2mut; for m in $(SAMIR_W2_UNIQUE_MUTANTS); do \
 $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_W2 -DSAMIR_W2_MUTANT_unique_$$m -Iseed -I$(SAMIR_INC_DIR) -Ispec \
  -o $(BUILD)/samirw2mut/unique_$$m $(DBF_DIFF_DIR)/test_samir_fns.c $(DBF_DIFF_DIR)/w2_host.c \
  $(filter-out $(SAMIR_MAIN_SRC),$(filter %.c,$(SAMIR_MAIN_SRC) $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC))); \
 red=0; for c in set-unique set-uniquerebuild; do \
  if $(BUILD)/samirw2mut/unique_$$m $$c host > $(BUILD)/samirw2mut/unique_$$m.$$c.log 2>&1; then :; else red=1; fi; done; \
 test $$red -eq 1 || { printf '!!! SAMIR_W2_MUTANT_unique_%s stayed GREEN\n' $$m; exit 1; }; \
 printf '>>> SAMIR_W2_MUTANT_unique_%s RED\n' $$m; done
@echo '>>> test-samir-w2-unique-mutant: green (every mutant caught)'
```

## test-samir-w2-verb-average

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/average && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-average host
@echo '>>> test-samir-w2-verb-average: green'
```

## test-samir-w2-verb-count

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/count && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-count host
@echo '>>> test-samir-w2-verb-count: green'
```

## test-samir-w2-verb-dir

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/dir && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-dir host
@echo '>>> test-samir-w2-verb-dir: green'
```

## test-samir-w2-verb-eject

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/eject && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-eject host
@echo '>>> test-samir-w2-verb-eject: green'
```

## test-samir-w2-verb-insert

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/insert && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-insert host
@echo '>>> test-samir-w2-verb-insert: green'
```

## test-samir-w2-verb-join

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/join && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-join host
@echo '>>> test-samir-w2-verb-join: green'
```

## test-samir-w2-verb-sum

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/sum && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-sum host
@echo '>>> test-samir-w2-verb-sum: green'
```

## test-samir-w2-verb-text

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/text && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-text host
@echo '>>> test-samir-w2-verb-text: green'
```

## test-samir-w2-verb-update

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35354):
Prerequisites: `build/test_samir_w2 build/.makefile.stamp`
```make
@cd harness/diff/dbf_diff/xbase_prog_diff/verb-mint/update && sha256sum --quiet -c sha256.txt
@$(SAMIR_W2_TEST) verb-update host
@echo '>>> test-samir-w2-verb-update: green'
```

## test-samir-write

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29568):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR.EXE build/SAMIR.OVL build/CLIENTS.DBF build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-samir-write : SAMIR WRITES a .dbf that PERSISTS (S8.2 deepening)\n'
@printf '  Ref: bead initech-g6wx; ADR-0009 DEC-08. boot -> USE -> REPLACE + APPEND -> flush -> on-disk .dbf.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + FRESH WRITABLE data disk %s (SAMIR.EXE + CLIENTS.DBF, primary slave)\n' "$(SECONDARY_TRACER_IMG)" "$(SAMIR_WRITE_IMG)"
@printf 'Expecting : in-emu LIST shows BAL 9999.99 + rec4 5555.55; extracted .dbf has 4 records (independent reader)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# FRESH disk for the serial leg -- Rule 11: each run starts from the pristine
@# 3-record fixture so APPEND BLANK lands exactly one new record (3->4), never
@# accumulating across legs/invocations.
@$(call samir_write_mint,$(SAMIR_WRITE_IMG),$(SAMIR_EXE))
@printf '>>> samir_write: minted FRESH %s (pristine 3-record CLIENTS.DBF) for the serial leg\n' "$(SAMIR_WRITE_IMG)"
@# Run 1 (serial): EXEC SAMIR, USE rw, REPLACE + APPEND + REPLACE, LIST, QUIT, EXIT.
@$(call fixture-preflight,$(SAMIR_WRITE_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_WRITE_IMG)" \
	--name "$(SAMIRWR_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRWR_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SAMIRWR_REPORT)" || true
@cat "$(SAMIRWR_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SAMIRWR_REPORT)"; then \
	printf '!!! test-samir-write FAIL: TRIPLE FAULT -- the WRITE/SEEK/CLOSE path crashed (root-cause pal_milton, Rule 3)\n'; \
	exit 1; \
fi
@printf '>>> test-samir-write [1/5]: no triple-fault (SAMIR ran the mutations without crashing)\n'
@if [ ! -s "$(SAMIRWR_SERIAL)" ]; then \
	printf '!!! test-samir-write FAIL: no serial captured at %s\n' "$(SAMIRWR_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(SAMIRWR_SERIAL)" \
	|| { printf '!!! test-samir-write FAIL: SHELL-READY missing -- COMMAND.COM never reached the prompt\n'; exit 1; }
@printf '>>> test-samir-write [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to AFTER `A:\>samir` so the assertions bite SAMIR's OWN output.
@tr -d '\r' < "$(SAMIRWR_SERIAL)" | sed -n '/A:.>samir/,$$p' > "$(BUILD)/$(SAMIRWR_NAME).samir"
@grep -qF '9999.99' "$(BUILD)/$(SAMIRWR_NAME).samir" \
	|| { printf '!!! test-samir-write FAIL: the in-emu LIST did not show the REPLACEd BAL 9999.99 (the mutation did not apply, root-cause mutate.c / dbf_replace)\n'; \
	     cat "$(BUILD)/$(SAMIRWR_NAME).samir"; exit 1; }
@grep -qF '5555.55' "$(BUILD)/$(SAMIRWR_NAME).samir" \
	|| { printf '!!! test-samir-write FAIL: the in-emu LIST did not show the new rec-4 BAL 5555.55 (APPEND BLANK + REPLACE did not apply)\n'; \
	     cat "$(BUILD)/$(SAMIRWR_NAME).samir"; exit 1; }
@printf '>>> test-samir-write [3/5]: in-emu LIST shows the mutated rec-1 BAL (9999.99) + new rec-4 BAL (5555.55)\n'
@# ---- 4. SCREENDUMP (Run 2; Law 4): a SEPARATE keys+grab run leaves the
@# mutated LIST table (now 4 rows) on the LFB. The longer session (USE +
@# REPLACE + APPEND + REPLACE echoes) scrolls the LIST lower than the 3-row
@# boot gate: the 4 data rows land DETERMINISTICALLY in band [176,240) (fg
@# ~2400 every run; 0 if SAMIR did not USE+mutate+LIST). The ABSOLUTE bottom of
@# the dot-prompt scroll drifts run-to-run (the REPL keeps prompting during the
@# grab drain), but the session text is always confined to the LEFT columns
@# (x < ~320 below the banner). So the seafoam check (C) samples the RIGHT
@# margin x in [560,640), y in [32,480) -- a region the session NEVER inks and
@# that a solid-fill/garbage screen would still fail (ppm_text_check optional
@# bg_y0=32 bg_x0=560; immune to vertical scroll; weakens nothing). ----
@# FRESH disk for the screendump leg (its OWN image) so its appends never
@# disturb the serial leg's on-disk persistence proof (Rule 11).
@$(call samir_write_mint,$(SAMIR_WRITE_SCRN_IMG),$(SAMIR_EXE))
@$(call fixture-preflight,$(SAMIR_WRITE_SCRN_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_WRITE_SCRN_IMG)" \
	--name "$(SAMIRWR_SCRN_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRWR_SCRN_KEYS)" --keys-after "SHELL-READY" \
	--screendump --screendump-after "SHELL-READY" \
	2> "$(SAMIRWR_SCRN_REPORT)" || true
@if grep -q 'triple_fault=1' "$(SAMIRWR_SCRN_REPORT)"; then \
	printf '!!! test-samir-write FAIL: TRIPLE FAULT on the screendump run\n'; exit 1; \
fi
@if [ ! -s "$(SAMIRWR_PPM)" ]; then \
	printf '!!! test-samir-write FAIL: no screendump captured at %s (live guest required)\n' "$(SAMIRWR_PPM)"; exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(SAMIRWR_PPM)" 176 240 300 32 560 \
	|| { printf '!!! test-samir-write FAIL: the mutated LIST rows did not render on the framebuffer (band [176,240) < 300 fg), or the right-margin desktop is not seafoam\n'; exit 1; }
@printf '>>> test-samir-write [4/5]: screendump shows the mutated 4-row LIST table on the seafoam desktop\n'
@# ---- 5. THE KEY PROOF -- PERSISTENCE: extract CLIENTS.DBF off the (mutated)
@# data disk and verify with the INDEPENDENT python reader. mcopy + python3 are
@# REQUIRED here (this is the whole point of the gate, not a bonus). ----
@command -v mcopy >/dev/null 2>&1 \
	|| { printf '!!! test-samir-write FAIL: mcopy absent -- cannot extract the on-disk .dbf to PROVE persistence (this gate REQUIRES mtools)\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 \
	|| { printf '!!! test-samir-write FAIL: python3 absent -- the independent dbf_ref.py reader is REQUIRED\n'; exit 1; }
@rm -f "$(SAMIRWR_OUT_DBF)"
@mcopy -i "$(SAMIR_WRITE_IMG)" ::CLIENTS.DBF "$(SAMIRWR_OUT_DBF)" 2>/dev/null \
	|| { printf '!!! test-samir-write FAIL: mcopy could not extract CLIENTS.DBF off the post-run disk\n'; exit 1; }
@python3 harness/diff/dbf_diff/dbf_ref.py --records "$(SAMIRWR_OUT_DBF)" > "$(BUILD)/$(SAMIRWR_NAME).recs" 2> "$(BUILD)/$(SAMIRWR_NAME).recerr" \
	|| { printf '!!! test-samir-write FAIL: dbf_ref.py could not parse the extracted .dbf (structural corruption?):\n'; cat "$(BUILD)/$(SAMIRWR_NAME).recerr"; exit 1; }
@printf '    --- dbf_ref.py --records on the EXTRACTED (post-run) CLIENTS.DBF ---\n'
@sed 's/^/    /' "$(BUILD)/$(SAMIRWR_NAME).recs"
@nrec=$$(grep -c '^rec' "$(BUILD)/$(SAMIRWR_NAME).recs"); \
 if [ "$$nrec" -ne 4 ]; then \
	printf '!!! test-samir-write FAIL: extracted .dbf has %s records, expected 4 (APPEND BLANK did not persist to the FAT volume)\n' "$$nrec"; exit 1; \
 fi
@grep -q '^rec0 active NAME=PESTON CITY=HONOLULU BAL=9999.99$$' "$(BUILD)/$(SAMIRWR_NAME).recs" \
	|| { printf '!!! test-samir-write FAIL: rec0 BAL != 9999.99 on disk (the REPLACE did not persist through pal_milton -> FAT write)\n'; exit 1; }
@grep -q '^rec3 active NAME= CITY= BAL=5555.55$$' "$(BUILD)/$(SAMIRWR_NAME).recs" \
	|| { printf '!!! test-samir-write FAIL: rec3 BAL != 5555.55 on disk (the APPEND+REPLACE did not persist)\n'; exit 1; }
@printf '>>> test-samir-write [5/5]: PERSISTED -- independent reader confirms 4 records, rec0 BAL=9999.99, rec3 BAL=5555.55 ON DISK\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- SAMIR WROTE a .dbf inside InitechOS; the REPLACE + APPEND BLANK\n'
@printf '            round-tripped through pal_milton -> INT 21h -> the kernel FAT12 write path\n'
@printf '            (QEMU only; tri-emulator agreement pending bead initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-samir-write: green\n'
```

## test-samir-write-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 29669):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/SAMIR_DROPWRITE.EXE build/CLIENTS.DBF build/.makefile.stamp`
```make
@printf '>>> test-samir-write-mutant: confirming the drop-write pal_milton mutant goes RED (Rule 6)\n'
@# FRESH mutant disk from the pristine fixture (Rule 11).
@$(call samir_write_mint,$(SAMIR_WRITE_MUT_IMG),$(SAMIR_EXE_DROPWRITE))
@$(call fixture-preflight,$(SAMIR_WRITE_MUT_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_WRITE_MUT_IMG)" \
	--name "$(SAMIRWR_MUT_NAME)" --out "$(BUILD)" --timeout-ms 60000 \
	--keys "$(SAMIRWR_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(SAMIRWR_MUT_REPORT)"
@sh harness/emu/require_trace.sh "$(SAMIRWR_MUT_REPORT)" "$(SAMIRWR_MUT_SERIAL)" "$$(printf '%s\n' '$(SAMIRWR_KEYS)' | awk -F, '{print NF}')"
@# The mutant must NOT triple-fault (it is a data-not-persisted bug, not a crash).
@if grep -q 'triple_fault=1' "$(SAMIRWR_MUT_REPORT)"; then \
	printf '!!! test-samir-write-mutant FAIL: the mutant TRIPLE-FAULTED -- a crash, not the stale-disk RED the gate asserts\n'; exit 1; \
fi
@command -v mcopy >/dev/null 2>&1 \
	|| { printf '!!! test-samir-write-mutant FAIL: mcopy absent -- cannot extract the on-disk .dbf to confirm the bite\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 \
	|| { printf '!!! test-samir-write-mutant FAIL: python3 absent -- the independent reader is REQUIRED\n'; exit 1; }
@rm -f "$(SAMIRWR_MUT_OUT_DBF)"
@mcopy -i "$(SAMIR_WRITE_MUT_IMG)" ::CLIENTS.DBF "$(SAMIRWR_MUT_OUT_DBF)" 2>/dev/null \
	|| { printf '!!! test-samir-write-mutant FAIL: mcopy could not extract CLIENTS.DBF off the mutant disk\n'; exit 1; }
@python3 harness/diff/dbf_diff/dbf_ref.py --records "$(SAMIRWR_MUT_OUT_DBF)" > "$(BUILD)/$(SAMIRWR_MUT_NAME).recs" 2>/dev/null \
	|| { printf '!!! test-samir-write-mutant FAIL: dbf_ref.py could not parse the mutant .dbf -- expected a STALE-but-valid 3-record file, not corruption\n'; exit 1; }
@printf '    --- dbf_ref.py --records on the mutant extracted CLIENTS.DBF ---\n'
@sed 's/^/    /' "$(BUILD)/$(SAMIRWR_MUT_NAME).recs"
@# The mutant dropped the WRITE: the on-disk .dbf MUST be the original 3-record
@# fixture (unmutated). If it has 4 records or rec0 BAL=9999.99, the mutant did
@# NOT bite -> the gate is decoration.
@nrec=$$(grep -c '^rec' "$(BUILD)/$(SAMIRWR_MUT_NAME).recs"); \
 if [ "$$nrec" -ne 3 ]; then \
	printf '!!! test-samir-write-mutant FAIL: mutant disk has %s records (expected the original 3) -- the drop-write mutant did NOT bite\n' "$$nrec"; exit 1; \
 fi
@if grep -q 'BAL=9999.99' "$(BUILD)/$(SAMIRWR_MUT_NAME).recs"; then \
	printf '!!! test-samir-write-mutant FAIL: mutant on-disk .dbf has BAL=9999.99 -- the write was NOT dropped (gate decoration)\n'; exit 1; \
fi
@printf '>>> test-samir-write-mutant: green (drop-write mutant correctly RED -- on-disk .dbf STALE [3 records, original BALs], no crash)\n'
```

## test-screen-mint

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 19):
Prerequisites: `build/screen_tool build/SCREEN.COM build/.makefile.stamp`
```make
@sh harness/diff/dbf_diff/screen_mint_test.sh
@printf '>>> test-screen-mint: green\n'
```

## test-screen-mint-mutants

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 32):
Prerequisites: `build/test_qemu_screen_helper build/screen_tool build/SCREEN.COM build/screen_capture build/qemu_harness build/tracer_secondary.img build/samir_list.img build/.makefile.stamp`
```make
@$(BUILD)/test_qemu_screen_helper
@sh harness/diff/dbf_diff/screen_mint_mutants.sh
@printf '>>> test-screen-mint-mutants: green\n'
```

## test-screen-mint-repro

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 22):
Prerequisites: `build/screen_tool build/SCREEN.COM build/.makefile.stamp`
```make
@sh harness/diff/dbf_diff/screen_mint_repro.sh
```

## test-screen-mint-samir

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/dbf_diff/screen_mint.mk', line 26):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/.makefile.stamp`
```make
@sh harness/diff/dbf_diff/screen_mint_samir.sh
```

## test-use-rw

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3748):
Prerequisites: `build/test_use_rw build/.makefile.stamp`
```make
@printf ">>> test-use-rw: writable USE (dbf_open_rw + wa_set_open_rw); plain USE-then-REPLACE persists (7az.16)\n"
@$(TEST_USE_RW) $(DBASE3_DECOMP)
@printf ">>> test-use-rw: green\n"
```

## test-use-rw-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3754):
Prerequisites: `build/test_use_rw_mut build/.makefile.stamp`
```make
@printf ">>> test-use-rw-mutant: confirming the open-RW-but-read-only mutant goes RED (Rule 6; initech-7az.16)\n"
@$(TEST_USE_RW_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-use-rw-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_USE_RW_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-use-rw-mutant FAIL: mutant PASSED -- writable USE is decoration\n'; exit 1; \
else \
	printf '>>> test-use-rw-mutant: green (open-RW-but-read-only correctly RED)\n'; \
fi
```

## test-w4-files

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 22):
Prerequisites: `build/frm_diff build/lbl_diff build/qry_diff build/scr_diff build/fmt_diff build/config_diff build/prg_diff build/vue_diff build/exact_diff build/w4_mutate build/.makefile.stamp`
```make
@DBASE3_DECOMP="$(DBASE3_DECOMP)" sh $(W4_DIR)/w4_files_paths_test.sh
```

## test-w4-inventory

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 18):
Prerequisites: `build/w4_inventory build/frm_diff build/lbl_diff build/qry_diff build/scr_diff build/fmt_diff build/config_diff build/prg_diff build/vue_diff build/exact_diff build/.makefile.stamp`
```make
@$(BUILD)/w4_inventory $(W4_SPEC)
```

## test-w4-inventory-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 20):
Prerequisites: `build/w4_inventory build/frm_diff build/lbl_diff build/qry_diff build/scr_diff build/fmt_diff build/config_diff build/prg_diff build/vue_diff build/exact_diff build/w4_mutate build/.makefile.stamp`
```make
@sh $(W4_DIR)/w4_inventory_test.sh
```

## test-w4-retained

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 24):
Prerequisites: `build/SCREEN.COM build/screen_tool build/w4_screen_check build/.makefile.stamp`
```make
@DBASE3_DECOMP="$(DBASE3_DECOMP)" sh $(W4_DIR)/w4_retained_test.sh
```

## test-w4-structure-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 30):
Prerequisites: `test-w4-trace-smoke build/w4_mutate build/.makefile.stamp`
```make
@sh $(W4_DIR)/w4_smoke_mutants.sh
```

## test-w4-trace-smoke

Declared tier: EMU; #  recipe to execute (from 'harness/diff/dbf_diff/w4.mk', line 26):
Prerequisites: `build/screen_capture build/screen_tool build/qemu_harness build/tracer_secondary.img build/samir_list.img build/SAMIR.EXE build/w4_screen_check build/ppm_text_check build/.makefile.stamp`
```make
@sh $(W4_DIR)/w4_trace.sh smoke
@sh $(W4_DIR)/w4_trace.sh mixed-case
@sh $(W4_DIR)/w4_trace.sh setup-prelude
```

## test-xbase-coercion

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3899):
Prerequisites: `build/dbf_coerce_fuzz build/.makefile.stamp`
```make
@printf ">>> test-xbase-coercion: coercion fuzzer -- engine vs table-driven reference (directed 34 cells + 2000 seeds) (S3.4)\n"
@$(DBF_COERCE_FUZZ)
@printf ">>> test-xbase-coercion: green\n"
```

## test-xbase-coercion-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3905):
Prerequisites: `build/dbf_coerce_fuzz_mut test-xbase-coercion-mutant-blankdate build/.makefile.stamp`
```make
@printf ">>> test-xbase-coercion-mutant: confirming the engine C+N-succeeds mutant is caught by the fuzzer (Rule 6; initech-gmo.4)\n"
@$(DBF_COERCE_FUZZ_MUT) 2>/dev/null | grep -q 'checks,\|coverage:' \
	|| { printf '!!! test-xbase-coercion-mutant FAIL: no summary -- harness dead, RED is meaningless\n'; exit 1; }
@if $(DBF_COERCE_FUZZ_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-coercion-mutant FAIL: mutant PASSED -- the coercion fuzzer is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-coercion-mutant: green (fuzzer caught the C+N divergence)\n'; \
fi
```

## test-xbase-coercion-mutant-blankdate

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3928):
Prerequisites: `build/dbf_coerce_fuzz_mut_blankdate build/.makefile.stamp`
```make
@log=$(BUILD)/blankdate-coercion-mut.log; set +e; $< > $$log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'checks,' $$log && grep -q 'FAIL.*BLANKCMP' $$log \
 || { cat $$log; printf '!!! blank-HIGH mutant did not fail its coercion checks\n'; exit 1; }; \
 printf '>>> test-xbase-coercion-mutant-blankdate: green (blank-HIGH correctly RED): '; grep -m1 'FAIL.*BLANKCMP' $$log
```

## test-xbase-eval

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3868):
Prerequisites: `build/test_xbase_eval build/.makefile.stamp`
```make
@printf ">>> test-xbase-eval: xBase III+ evaluator + operator coercion (C+N=error#9; SET EXACT; D arith; \$$) (S3.3)\n"
@$(TEST_XBASE_EVAL)
@printf ">>> test-xbase-eval: green\n"
```

## test-xbase-eval-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3874):
Prerequisites: `build/test_xbase_eval_mut test-xbase-eval-mutant-blankdate build/.makefile.stamp`
```make
@printf ">>> test-xbase-eval-mutant: confirming the C+N-succeeds mutant goes RED (Rule 6; initech-gmo.3)\n"
@$(TEST_XBASE_EVAL_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-eval-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_EVAL_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-eval-mutant FAIL: mutant PASSED -- the C+N type-mismatch rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-eval-mutant: green (C+N auto-stringify correctly RED)\n'; \
fi
```

## test-xbase-eval-mutant-blankdate

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 3923):
Prerequisites: `build/test_xbase_eval_mut_blankdate build/.makefile.stamp`
```make
@log=$(BUILD)/blankdate-eval-mut.log; set +e; $< > $$log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'checks,' $$log && grep -q 'FAIL.*BLANKCMP' $$log \
 || { cat $$log; printf '!!! blank-HIGH mutant did not fail its eval checks\n'; exit 1; }; \
 printf '>>> test-xbase-eval-mutant-blankdate: green (blank-HIGH correctly RED): '; grep -m1 'FAIL.*BLANKCMP' $$log
```

## test-xbase-fn-a

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2917):
Prerequisites: `build/test_xbase_fn_a build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-a: xBase III+ built-in functions A (STR/SUBSTR/CTOD/IIF/TYPE/...) (S3.5)\n"
@$(TEST_XBASE_FN_A)
@printf ">>> test-xbase-fn-a: green\n"
```

## test-xbase-fn-a-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2923):
Prerequisites: `build/test_xbase_fn_a_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-a-mutant: confirming the SUBSTR-0-based mutant goes RED (Rule 6; initech-7az.1)\n"
@$(TEST_XBASE_FN_A_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-fn-a-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_FN_A_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-fn-a-mutant FAIL: mutant PASSED -- the SUBSTR 1-based rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-fn-a-mutant: green (SUBSTR 0-based correctly RED)\n'; \
fi
```

## test-xbase-fn-a-mutant-ctod

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2934):
Prerequisites: `build/test_xbase_fn_a_mut_ctod build/.makefile.stamp`
```make
@log=$(BUILD)/ctod-cap-mut.log; set +e; $< > $$log 2>&1; rc=$$?; set -e; \
 test $$rc -eq 1 && grep -q 'checks,' $$log && grep -q 'FAIL.*CALVALID' $$log \
 || { cat $$log; printf '!!! CTOD mutant did not fail its minted rollover checks\n'; exit 1; }; \
 printf '>>> test-xbase-fn-a-mutant-ctod: green (old calendar cap correctly RED): '; grep -m1 'FAIL.*CALVALID' $$log
```

## test-xbase-fn-b

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2958):
Prerequisites: `build/test_xbase_fn_b build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-b: xBase III+ functions B freestanding (ABS/INT/MOD/ROUND/MAX/MIN/DOW/CDOW/CMONTH) (S3.6a)\n"
@$(TEST_XBASE_FN_B)
@printf ">>> test-xbase-fn-b: green\n"
```

## test-xbase-fn-b-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2964):
Prerequisites: `build/test_xbase_fn_b_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-b-mutant: confirming the DOW-shift mutant goes RED (Rule 6; initech-7az.11)\n"
@$(TEST_XBASE_FN_B_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-fn-b-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_FN_B_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-fn-b-mutant FAIL: mutant PASSED -- the DOW numbering rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-fn-b-mutant: green (DOW shift correctly RED)\n'; \
fi
```

## test-xbase-fn-b-mutant-round

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2975):
Prerequisites: `build/test_xbase_fn_b_mut_round build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-b-mutant-round: confirming the ROUND floor-vs-truncate mutant goes RED (Rule 6; initech-eyig)\n"
@$(TEST_XBASE_FN_B_MUT_ROUND) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-fn-b-mutant-round FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_FN_B_MUT_ROUND) >/dev/null 2>&1; then \
	printf '!!! test-xbase-fn-b-mutant-round FAIL: mutant PASSED -- the ROUND floor rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-fn-b-mutant-round: green (negative-operand truncation correctly RED)\n'; \
fi
```

## test-xbase-fn-c

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2997):
Prerequisites: `build/test_xbase_fn_c build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-c: xBase III+ string fns C (LEFT/RIGHT/STUFF/REPLICATE/AT/ISALPHA/ISUPPER/ISLOWER; TRANSFORM GATED) (7az.12)\n"
@$(TEST_XBASE_FN_C)
@printf ">>> test-xbase-fn-c: green\n"
```

## test-xbase-fn-c-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3003):
Prerequisites: `build/test_xbase_fn_c_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-c-mutant: confirming the LEFT n+1 mutant goes RED (Rule 6; initech-7az.12)\n"
@$(TEST_XBASE_FN_C_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-fn-c-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_FN_C_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-fn-c-mutant FAIL: mutant PASSED -- the LEFT semantics is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-fn-c-mutant: green (LEFT n+1 correctly RED)\n'; \
fi
```

## test-xbase-fn-d

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3778):
Prerequisites: `build/test_xbase_fn_d build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-d: xBase III+ DB functions (RECNO/RECCOUNT/EOF/BOF/FOUND/DELETED/FIELD/DBF/FILE) (S3.6b)\n"
@$(TEST_XBASE_FN_D) $(DBASE3_DECOMP)
@printf ">>> test-xbase-fn-d: green\n"
```

## test-xbase-fn-d-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3784):
Prerequisites: `build/test_xbase_fn_d_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-fn-d-mutant: confirming the RECNO-at-EOF mutant goes RED (Rule 6; initech-7az.10)\n"
@$(TEST_XBASE_FN_D_MUT) $(DBASE3_DECOMP) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-fn-d-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_FN_D_MUT) $(DBASE3_DECOMP) >/dev/null 2>&1; then \
	printf '!!! test-xbase-fn-d-mutant FAIL: mutant PASSED -- the RECNO-at-EOF rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-fn-d-mutant: green (RECNO-at-EOF correctly RED)\n'; \
fi
```

## test-xbase-lex

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3807):
Prerequisites: `build/test_xbase_lex build/.makefile.stamp`
```make
@printf ">>> test-xbase-lex: xBase III+ expression lexer (literals/ops/dotted; rejects ==) (S3.1)\n"
@$(TEST_XBASE_LEX)
@printf ">>> test-xbase-lex: green\n"
```

## test-xbase-lex-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3813):
Prerequisites: `build/test_xbase_lex_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-lex-mutant: confirming the '== accepted' mutant goes RED (Rule 6; initech-gmo.1)\n"
@$(TEST_XBASE_LEX_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-lex-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_LEX_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-lex-mutant FAIL: mutant PASSED -- the == rejection is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-lex-mutant: green (== acceptance correctly RED)\n'; \
fi
```

## test-xbase-parse

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3837):
Prerequisites: `build/test_xbase_parse build/.makefile.stamp`
```make
@printf ">>> test-xbase-parse: xBase III+ precedence parser -> AST (^ left-assoc; unary>^) (S3.2)\n"
@$(TEST_XBASE_PARSE)
@printf ">>> test-xbase-parse: green\n"
```

## test-xbase-parse-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3843):
Prerequisites: `build/test_xbase_parse_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-parse-mutant: confirming the right-assoc-^ mutant goes RED (Rule 6; initech-gmo.2)\n"
@$(TEST_XBASE_PARSE_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-parse-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_PARSE_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-parse-mutant FAIL: mutant PASSED -- the ^ associativity is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-parse-mutant: green (right-assoc ^ correctly RED)\n'; \
fi
```

## test-xbase-transform

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3026):
Prerequisites: `build/test_xbase_transform build/.makefile.stamp`
```make
@printf ">>> test-xbase-transform: TRANSFORM full picture/function engine (numeric + @(/@X/@C/@B + char) (7az.14)\n"
@$(TEST_XBASE_TRANSFORM)
@printf ">>> test-xbase-transform: green\n"
```

## test-xbase-transform-dotwidth-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3051):
Prerequisites: `build/test_xbase_transform_dotwidth_mut build/.makefile.stamp`
```make
@$(TEST_XBASE_TRANSFORM_DOT_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-transform-dotwidth-mutant FAIL: no TEST_SUMMARY -- harness dead\n'; exit 1; }
@if $(TEST_XBASE_TRANSFORM_DOT_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-transform-dotwidth-mutant FAIL: mutant PASSED -- the decimal-point column is decoration\n'; exit 1; \
elif $(TEST_XBASE_TRANSFORM_DOT_MUT) 2>&1 | grep -q 'FAIL.*L007'; then \
	printf '>>> test-xbase-transform-dotwidth-mutant: green (L007 exact-fit cases correctly RED)\n'; \
else printf '!!! test-xbase-transform-dotwidth-mutant FAIL: RED for the wrong reason\n'; exit 1; fi
```

## test-xbase-transform-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3032):
Prerequisites: `build/test_xbase_transform_mut build/.makefile.stamp`
```make
@printf ">>> test-xbase-transform-mutant: confirming the @( paren mutant goes RED (Rule 6; initech-7az.14)\n"
@$(TEST_XBASE_TRANSFORM_MUT) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-xbase-transform-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_XBASE_TRANSFORM_MUT) >/dev/null 2>&1; then \
	printf '!!! test-xbase-transform-mutant FAIL: mutant PASSED -- the @( paren rule is decoration\n'; exit 1; \
else \
	printf '>>> test-xbase-transform-mutant: green (@( paren correctly RED)\n'; \
fi
```

## test-xbase-transform-rounding-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 35119):
Prerequisites: `build/test_xbase_transform_rounding_mut build/.makefile.stamp`
```make
@log=$(BUILD)/tr-rounding-mut.log; set +e; $< > $$log 2>&1; rc=$$?; set -e; \
 if [ $$rc -ne 1 ] || ! grep -q 'checks,' $$log || ! grep -q 'FAIL.*G10:' $$log; then \
   cat $$log; printf '!!! TRANSFORM rounding mutant did not fail G10\n'; exit 1; fi; \
 printf '>>> TRANSFORM rounding correctly RED: '; grep -m1 'FAIL.*G10:' $$log
```

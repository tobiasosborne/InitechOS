## test-calc-file-copy-routing

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 126):
Prerequisites: `build/test_calc_file_copy build/.makefile.stamp`
```make
@$(BUILD)/test_calc_file_copy
@printf '>>> test-calc-file-copy-routing: green (Save enabled, asks the FILE COPY box once, PRD caption, a real src -> new dst)\n'
```

## test-calc-file-copy-routing-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 134):
Prerequisites: `test-calc-file-copy-routing build/test_calc_file_copy_mut_caption build/test_calc_file_copy_mut_CALC_MUT_SAVE_STALE build/test_calc_file_copy_mut_CALC_MUT_REPLACE_SILENT build/test_calc_file_copy_mut_CALC_MUT_ONE_FLAG build/test_calc_file_copy_mut_CALC_MUT_STAGE_CLOBBER build/.makefile.stamp`
```make
@set -e; for pair in CALC_MUT_SAVE_STALE:SAVE_EDIT CALC_MUT_REPLACE_SILENT:REPLACE_ASKS CALC_MUT_ONE_FLAG:SAVE_REPEAT CALC_MUT_STAGE_CLOBBER:STAGE_CLOBBER; do m=$${pair%%:*}; why=$${pair#*:}; \
	if $(BUILD)/test_calc_file_copy_mut_$$m > $(BUILD)/test_calc_file_copy_mut_$$m.log 2>&1; then printf 'FAIL file-copy routing mutant %s survived\n' "$$m"; exit 1; fi; \
	grep -q "FAIL file-copy routing: $$why" $(BUILD)/test_calc_file_copy_mut_$$m.log || { printf 'FAIL %s RED for the wrong reason\n' "$$m"; exit 1; }; \
	printf '>>> test-calc-file-copy-routing-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL file-copy routing: $$why" $(BUILD)/test_calc_file_copy_mut_$$m.log)"; done
@if $(BUILD)/test_calc_file_copy_mut_caption > $(BUILD)/test_calc_file_copy_mut_caption.log 2>&1; then printf 'FAIL file-copy routing mutant CALC_MUT_CAPTION survived\n'; exit 1; fi
@grep -q 'FAIL file-copy routing: SAVE_CAPTION' $(BUILD)/test_calc_file_copy_mut_caption.log
@printf '>>> test-calc-file-copy-routing-mutant: CALC_MUT_CAPTION RED: %s\n' "$$(grep -m1 SAVE_CAPTION $(BUILD)/test_calc_file_copy_mut_caption.log)"
```

## test-calc-spec

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 23):
Prerequisites: `build/calc_ledger_gen build/.makefile.stamp`
```make
@$(BUILD)/calc_ledger_gen spec/assets/calc_ledger.json > $(BUILD)/calc_ledger.generated.h
@cmp spec/assets/calc_ledger.h $(BUILD)/calc_ledger.generated.h
@printf '>>> test-calc-spec: green (locked JSON -> checked-in C header, byte-identical)\n'
```

## test-calc-tenant

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 29):
Prerequisites: `test-calc-spec build/test_calc_tenant build/.makefile.stamp`
```make
@$(BUILD)/test_calc_tenant
@printf '>>> test-calc-tenant: green (PRD literals, two windows, i123 edits, menu/close routing)\n'
```

## test-calc-tenant-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 32):
Prerequisites: `test-calc-tenant build/test_calc_tenant_mut_CALC_MUT_PIE100 build/test_calc_tenant_mut_CALC_MUT_LEADING_MINUS build/test_calc_tenant_mut_CALC_MUT_NEVER_DIRTY build/test_calc_tenant_mut_CALC_MUT_CANCEL_QUITS build/.makefile.stamp`
```make
@set -e; for m in $(CALC_MUTANTS); do \
	if $(BUILD)/test_calc_tenant_mut_$$m > $(BUILD)/calc_host_$$m.log 2>&1; then printf 'FAIL calc mutant survived: %s\n' "$$m"; exit 1; fi; \
	case $$m in *PIE*) why=PIE_SUM;; *NEVER_DIRTY*) why=QUIT_ASKS;; *CANCEL_QUITS*) why=QUIT_CANCEL;; *) why=TRAILING_MINUS;; esac; \
	grep -q "FAIL $$why" $(BUILD)/calc_host_$$m.log; \
	printf '>>> test-calc-tenant-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL $$why" $(BUILD)/calc_host_$$m.log)"; \
done
@printf '>>> test-calc-tenant-mutant: green (four named mutants RED)\n'
```

## test-flair-calc

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 64):
Prerequisites: `test-calc-spec build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_calc_check build/.makefile.stamp`
```make
$(call appl-boot,flair_calc,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_calc.serial) && $(call CALC_TEXT_OK,$(BUILD)/flair_calc.serial) || { printf 'FAIL calc PRD text/window trace\n'; exit 1; }
@$(BUILD)/ppm_flair_calc_check $(BUILD)/flair_calc.ppm $(CALC_GOLDEN)
$(call appl-boot,flair_calc_edit,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_EDIT_SPEC,)
@grep -q 'ax=0x0030 win=1 .*s="73" fg=65537 bg=0 -> 0' $(BUILD)/flair_calc_edit.serial && grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_calc_edit.serial || { printf 'FAIL calc edit/quit\n'; exit 1; }
@printf '>>> test-flair-calc: green (disk launch, two windows, PRD text, historical chrome pixels, edit, quit)\n'
```

## test-flair-calc-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 121):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-calc-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-calc-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-calc_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-calc_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-calc_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-calc_bochs.report.txt" || true
@cat "$(BUILD)/flair-calc_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-calc-bochs "$(BUILD)/flair-calc_bochs.report.txt" "$(BUILD)/flair-calc_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-calc-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 88):
Prerequisites: `test-flair-calc build/flair_data_calc_CALC_MUT_PIE100.img build/flair_data_calc_CALC_MUT_LEADING_MINUS.img build/flair_data_calc_CALC_MUT_NEVER_DIRTY.img build/flair_data_calc_CALC_MUT_CANCEL_QUITS.img build/flair_data_calc_CALC_MUT_MERGE_SLICES.img build/flair_tenants_mut_calc_token_alias.img build/flair_tenants_mut_calc_geneva_ignored.img build/.makefile.stamp`
```make
$(call calc-emu-mutant-leg,CALC_MUT_PIE100,win=2 .*s="100%",PIE_SUM rendered 100%)
$(call calc-emu-mutant-leg,CALC_MUT_LEADING_MINUS,win=1 .*s="-570",TRAILING_MINUS rendered -570)
$(call calc-kernel-mutant-leg,token_alias,WINDOW_TOKENS,TBX_MUT_TOKEN_ALIAS)
$(call calc-kernel-mutant-leg,geneva_ignored,GENEVA9,TBX_MUT_GENEVA_IGNORED)
$(call appl-boot,flair_calc_merge,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_MERGE_SLICES_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_calc_merge.serial) && $(call CALC_TEXT_OK,$(BUILD)/flair_calc_merge.serial) || { printf 'FAIL merged-slices mutant did not draw PRD text\n'; exit 1; }
@$(BUILD)/ppm_flair_calc_check $(BUILD)/flair_calc_merge.ppm $(CALC_GOLDEN) > $(BUILD)/flair_calc_merge.pixels 2>&1; rc=$$?; \
[ "$$rc" = 1 ] && grep -q 'PIE_FIVE' $(BUILD)/flair_calc_merge.pixels || { cat $(BUILD)/flair_calc_merge.pixels; exit 1; }
@printf '>>> test-flair-calc-mutant: CALC_MUT_MERGE_SLICES RED: FAIL calc pixels: PIE_FIVE five visible sectors\n'
@printf '>>> test-flair-calc-mutant: green (five named mutants RED)\n'
```

## test-flair-calc-quit

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 346):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_calc_seeded_tables.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@command -v mdir >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-calc-quit FAIL: mtools / fsck.fat missing\n'; exit 1; }
$(call appl-boot,flair_calc_qg_alert,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_ALERT,--screendump --screendump-after "TENANT-ALERT up")
@f=$(BUILD)/flair_calc_qg_alert.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_alert_data.img) || { printf '!!! test-flair-calc-quit [1/11] FAIL: ALERT -- an edited Ctrl-Q must ask Save / Cancel / Don'"'"'t Save and never exit (L007)\n'; grep -E '^(TENANT-(GATE ax=0x0090|ALERT|EXIT|MENU|CRASH)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) tenant3 $(BUILD)/flair_calc_qg_alert.ppm > $(BUILD)/flair_calc_qg_alert.pixels || { cat $(BUILD)/flair_calc_qg_alert.pixels; printf '!!! test-flair-calc-quit [1/11] FAIL: ALERT -- the three-button alert is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-calc-quit [1/11] ALERT: Ctrl-Q over the edited A1 asks (3 lines, 3 buttons, pixel-graded), no exit, nothing written\n'
$(call appl-boot,flair_calc_qg_cancel,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CANCEL,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_calc_qg_cancel.serial; $(call APPL_ALIVE,$$f) && [ "$$(grep -c '^TENANT-ALERT up lines=3 buttons=3$$' $$f)" = 2 ] && [ "$$(grep -c '^TENANT-ALERT-HIT item=2$$' $$f)" = 2 ] && $(call CQ_NO_EXIT,$$f) && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_cancel_data.img) || { printf '!!! test-flair-calc-quit [2/11] FAIL: CANCEL -- Escape must keep Calc open and dirty (asked twice)\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [2/11] CANCEL: Escape twice -> two Cancel answers, no exit, still dirty (asked again), nothing written\n'
$(call appl-boot,flair_calc_qg_dontsave,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_DONTSAVE,)
@f=$(BUILD)/flair_calc_qg_dontsave.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=3' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_dontsave_data.img) && ! grep -q '^TENANT-GATE ax=0x0091' $$f || { printf '!!! test-flair-calc-quit [3/11] FAIL: DONTSAVE -- the third button must end Calc and write nothing\n'; grep -E '^(TENANT-(ALERT|EXIT|GATE ax=0x009)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [3/11] DONTSAVE: the third button -> EXIT(0); no FILE COPY, no TABLES.TXT on the volume\n'
$(call appl-boot,flair_calc_qg_savequit,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_SAVEQUIT,)
@f=$(BUILD)/flair_calc_qg_savequit.serial; d=$(BUILD)/flair_calc_qg_savequit_data.img; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-ALERT-HIT item=1' $$f && grep -q '^TENANT-FILECOPY-DONE' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [4/11] FAIL: SAVEQUIT -- Save from the alert must write the EDITED worksheet, then end Calc\n'; grep -E '^(TENANT-(ALERT|EXIT|FILECOPY|GATE ax=0x009)|PANIC)' $$f; $(call CQ_TABLES,$$d) | head -3; exit 1; }
@printf '>>> test-flair-calc-quit [4/11] SAVEQUIT: Save -> FILE COPY box -> TABLES.TXT line 2 is the edited 73 (mtools), no stage left, fsck clean, then EXIT(0)\n'
$(call appl-boot,flair_calc_qg_clean,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CLEAN,)
@f=$(BUILD)/flair_calc_qg_clean.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && ! grep -q '^TENANT-ALERT' $$f || { printf '!!! test-flair-calc-quit [5/11] FAIL: CLEAN -- an unedited worksheet must quit at once, never ask\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [5/11] CLEAN: no edit -> Ctrl-Q exits at once, no question\n'
$(call appl-boot,flair_calc_qg_pending,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_PENDING,)
@f=$(BUILD)/flair_calc_qg_pending.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) || { printf '!!! test-flair-calc-quit [6/11] FAIL: PENDING -- an entry being typed is unsaved work: Quit must ask\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [6/11] PENDING: "7" typed, not entered -> Ctrl-Q asks\n'
$(call appl-boot,flair_calc_qg_close,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CLOSE,)
@f=$(BUILD)/flair_calc_qg_close.serial; $(call APPL_ALIVE,$$f) && grep -q '^FLAIR-CLOSE-ASK win ' $$f && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) || { printf '!!! test-flair-calc-quit [7/11] FAIL: CLOSE -- the close box over an edited sheet must ask, not terminate\n'; grep -E '^(FLAIR-CLOSE|TENANT-(ALERT|EXIT|EVT)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [7/11] CLOSE: the close box over an edited sheet -> FLAIR-CLOSE-ASK, the alert, no exit\n'
$(call appl-boot,flair_calc_qg_save,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_SAVE,)
@f=$(BUILD)/flair_calc_qg_save.serial; d=$(BUILD)/flair_calc_qg_save_data.img; $(call APPL_ALIVE,$$f) && grep -q '^TENANT-FILECOPY-DONE' $$f && ! grep -q '^TENANT-ALERT' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && mcopy -n -i $$d ::APPS/LEDGER.TXT - | cmp -s - $(FLAIR_DATA_LEDGER) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [8/11] FAIL: SAVE -- Save must write the EDITED worksheet (L006: TABLES.TXT line 2 = 73, not Film Costs)\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
@printf '>>> test-flair-calc-quit [8/11] SAVE: A1 := 73, Ctrl-S -> TABLES.TXT holds 73 and no Film Costs (mtools), LEDGER.TXT untouched, fsck clean\n'
$(call appl-boot,flair_calc_qg_replaceask,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACEASK,--screendump --screendump-after "TENANT-ALERT up")
@f=$(BUILD)/flair_calc_qg_replaceask.serial; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && ! grep -q '^TENANT-GATE ax=0x0091' $$f && $(call CQ_SEED_KEPT,$(BUILD)/flair_calc_qg_replaceask_data.img) || { printf '!!! test-flair-calc-quit [9/11] FAIL: REPLACEASK -- a Save onto an existing TABLES.TXT must name it and ask (L009)\n'; grep -E '^(TENANT-(ALERT|FILECOPY-DONE|GATE ax=0x009)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-calc-quit [9/11] REPLACEASK: Save over a seeded TABLES.TXT -> "TABLES.TXT" already exists. ... Yes / No, nothing copied yet\n'
$(call appl-boot,flair_calc_qg_replacecancel,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACECANCEL,)
@f=$(BUILD)/flair_calc_qg_replacecancel.serial; d=$(BUILD)/flair_calc_qg_replacecancel_data.img; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=2' $$f && ! grep -q '^TENANT-GATE ax=0x0091' $$f && $(call CQ_SEED_KEPT,$$d) && $(call CQ_NO_STAGE,$$d) || { printf '!!! test-flair-calc-quit [10/11] FAIL: REPLACECANCEL -- Cancel must leave the existing TABLES.TXT alone\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
@printf '>>> test-flair-calc-quit [10/11] REPLACECANCEL: Cancel -> the seeded TABLES.TXT is byte-identical (mtools), nothing copied\n'
$(call appl-boot,flair_calc_qg_replaceyes,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACEYES,)
@f=$(BUILD)/flair_calc_qg_replaceyes.serial; d=$(BUILD)/flair_calc_qg_replaceyes_data.img; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=1' $$f && grep -q '^TENANT-FILECOPY-DONE' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [11/11] FAIL: REPLACEYES -- Replace must overwrite TABLES.TXT with the worksheet (73)\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
@printf '>>> test-flair-calc-quit [11/11] REPLACEYES: Replace -> TABLES.TXT now holds the 73 worksheet (mtools), no stage left, fsck clean\n'
@printf '>>> test-flair-calc-quit: green\n'
```

## test-flair-calc-quit-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 384):
Prerequisites: `test-flair-calc-quit build/flair_data_calc_CALC_MUT_NEVER_DIRTY.img build/flair_data_calc_CALC_MUT_CANCEL_QUITS.img build/flair_data_calc_CALC_MUT_SAVE_STALE.img build/flair_calc_seeded_tables_silent.img build/.makefile.stamp`
```make
$(call appl-boot,flair_calc_qgm_dirty,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_NEVER_DIRTY_IMG),FLAIR_CALC_QG_ALERT,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_calc_qgm_dirty.serial; if $(call CQ_ASKED,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_NEVER_DIRTY survived\n'; exit 1; fi; grep -qx 'TENANT-EXIT rc=0 via=exit' $$f || { printf '!!! test-flair-calc-quit-mutant: NEVER_DIRTY RED for the wrong reason (no silent exit)\n'; exit 1; }
@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_NEVER_DIRTY RED: an edited Ctrl-Q quit silently (L007)\n'
$(call appl-boot,flair_calc_qgm_cancel,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_CANCEL_QUITS_IMG),FLAIR_CALC_QG_CANCEL,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_calc_qgm_cancel.serial; if $(call CQ_NO_EXIT,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_CANCEL_QUITS survived\n'; exit 1; fi
@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_CANCEL_QUITS RED: Cancel ended the session\n'
$(call appl-boot,flair_calc_qgm_stale,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_SAVE_STALE_IMG),FLAIR_CALC_QG_SAVE,)
@f=$(BUILD)/flair_calc_qgm_stale.serial; d=$(BUILD)/flair_calc_qgm_stale_data.img; grep -q '^TENANT-FILECOPY-DONE' $$f || { printf '!!! test-flair-calc-quit-mutant: SAVE_STALE did not run the copy\n'; exit 1; }; if ( $(call CQ_HAS_EDIT,$$d,73) ); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_SAVE_STALE survived\n'; exit 1; fi
@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_SAVE_STALE RED: TABLES.TXT is the stale ledger, the 73 edit is lost (L006)\n'
$(call appl-boot,flair_calc_qgm_silent,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_SILENT_DATA),FLAIR_CALC_QG_REPLACEASK,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_calc_qgm_silent.serial; if $(call CQ_REPLACE_ASKED,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_REPLACE_SILENT survived\n'; exit 1; fi; grep -q '^TENANT-GATE ax=0x0091' $$f || { printf '!!! test-flair-calc-quit-mutant: REPLACE_SILENT RED for the wrong reason (the copy did not run)\n'; exit 1; }
@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_REPLACE_SILENT RED: the second Save replaced TABLES.TXT unasked (L009)\n'
@printf '>>> test-flair-calc-quit-mutant: green (four mutants RED for the named reason)\n'
```

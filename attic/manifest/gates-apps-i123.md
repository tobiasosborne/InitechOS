## test-flair-i123

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23808):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_region_count build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-i123 : Initech 123 opens from the Finder and takes entries\n'
@printf '  beads initech-9u8w / initech-w96l; Face A laid out from the real 1-2-3 R2.2 screen\n'
@printf '======================================================================\n'
$(call appl-boot,flair_i123_type,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_TYPE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_type.serial) || { printf '!!! test-flair-i123 [1/3] FAIL: the desktop did not survive\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED)' $(BUILD)/flair_i123_type.serial; exit 1; }
@$(call I123_TYPE_SERIAL,$(BUILD)/flair_i123_type.serial) || { printf '!!! test-flair-i123 [1/3] FAIL: TYPE -- 123 did not come up, or row 1 / line 1 / the mode / the pointer are not what the typed MINT01 row must give\n'; grep -E '^(TENANT-(LOAD|REGISTER|CRASH|EXIT)|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_i123_type.serial; grep -E '^TENANT-GATE ax=0x0033 win=1 x=(1|33|249|577) y=(0|64) ' $(BUILD)/flair_i123_type.serial | tail -4; exit 1; }
@printf '>>> test-flair-i123 [1/3] TYPE: %s; row 1 drawn exactly as the real 1-2-3 showed MINT01 ("Hello 123 1.5 -7"), line 1 "D1: -7", READY, pointer on D1\n' "$$(grep -m1 '^TENANT-LOAD name=123' $(BUILD)/flair_i123_type.serial)"
@f=$(BUILD)/flair_i123_type.ppm; \
	set -- $$($(call I123_PX,$$f,7 110 615 126,9 112)); bne=$$1; beq=$$2; \
	set -- $$($(call I123_PX,$$f,575 62 615 78,9 112)); mne=$$1; meq=$$2; \
	set -- $$($(call I123_PX,$$f,255 126 327 142,9 112)); pne=$$1; peq=$$2; \
	set -- $$($(call I123_PX,$$f,39 126 111 142,305 452)); ane=$$1; \
	set -- $$($(call I123_PX,$$f,255 142 327 158,9 112)); qeq=$$2; \
	[ -n "$$beq" ] && [ "$$beq" -ge 7800 ] && [ "$$meq" -ge 380 ] && [ "$$peq" -ge 900 ] && [ "$$ane" -ge 40 ] && [ "$$qeq" -le 0 ] || { \
		printf '!!! test-flair-i123 [1/3] FAIL: TYPE -- the screen is not the Face A layout (border reverse %s/9728 px, mode %s/640, pointer on D1 %s/1152, A1 ink %s px, pointer below D1 %s px)\n' "$$beq" "$$meq" "$$peq" "$$ane" "$$qeq"; exit 1; }; \
	printf '>>> test-flair-i123 [1/3] SCREEN: column border reverse %s/9728 px, mode indicator %s/640, pointer filled on D1 %s/1152, ink in A1 %s px, nothing reversed on D2\n' "$$beq" "$$meq" "$$peq" "$$ane"
$(call appl-boot,flair_i123_pre,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_PRE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_pre.serial) && ! grep -q '^TENANT-' $(BUILD)/flair_i123_pre.serial || { printf '!!! test-flair-i123 [3/3] FAIL: the never-launched baseline did not come up clean\n'; exit 1; }
$(call appl-boot,flair_i123_quit,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_QUIT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_quit.serial) && grep -qx 'TENANT-MENU menu=129 item=1 src=key' $(BUILD)/flair_i123_quit.serial && grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_i123_quit.serial || { printf '!!! test-flair-i123 [2/3] FAIL: QUIT -- Ctrl-Q must reach 123 as its File > Quit and end it\n'; grep -E '^(TENANT-(MENU|EVT what=81|EXIT|CRASH)|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_i123_quit.serial; exit 1; }
@cmp -s $(BUILD)/flair_i123_pre.ppm $(BUILD)/flair_i123_quit.ppm || { printf '!!! test-flair-i123 [2/3] FAIL: QUIT -- the post-quit frame differs from the never-launched baseline (%s bytes)\n' "$$(cmp -l $(BUILD)/flair_i123_pre.ppm $(BUILD)/flair_i123_quit.ppm | wc -l)"; exit 1; }
@printf '>>> test-flair-i123 [2/3] QUIT: Ctrl-Q -> TENANT-MENU 129/1 src=key -> EXIT(0); the frame is byte-identical to the never-launched baseline\n'
@printf '>>> test-flair-i123 [3/3] PRE: the baseline the quit leg was compared against\n'
@printf '>>> test-flair-i123: green\n'
```

## test-flair-i123-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23853):
Prerequisites: `build/bochs_harness build/qemu_harness build/flair_tenants_mut_tbx_smoke_i123.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-i123-bochs : Initech 123 on the CPU path, QEMU vs Bochs\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-i123-bochs FAIL: bochs not found (SKIP_BOCHS=1 to opt out loudly)\n'; exit 1; }
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_i123_smoke_qemu_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_i123_smoke_qemu_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/flair_i123_smoke_bochs_data.img
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(BUILD)/flair_i123_smoke_bochs_data.img,$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIR_I123_SMOKE_IMG)" --disk2 "$(BUILD)/flair_i123_smoke_qemu_data.img" --expect HALTED \
	--name flair_i123_smoke_qemu --out "$(BUILD)" --timeout-ms 30000 2> $(BUILD)/flair_i123_smoke_qemu.report || true
@$(BOCHS_BIN) --disk "$(FLAIR_I123_SMOKE_IMG)" --disk2 "$(BUILD)/flair_i123_smoke_bochs_data.img" --expect HALTED \
	--name flair_i123_smoke_bochs --out "$(BUILD)" --timeout-ms 240000 2> $(BUILD)/flair_i123_smoke_bochs.report || true
@if grep -q 'triple_fault=1' $(BUILD)/flair_i123_smoke_qemu.report $(BUILD)/flair_i123_smoke_bochs.report; then printf '!!! test-flair-i123-bochs FAIL: TRIPLE FAULT\n'; exit 1; fi
@for e in qemu bochs; do \
	f=$(BUILD)/flair_i123_smoke_$$e.serial; \
	grep -qx 'TBX-SMOKE-OK list-empty' $$f || { printf '!!! test-flair-i123-bochs FAIL: %s did not complete the 123 smoke with an empty process list\n' $$e; grep -E '^(TBX-|TENANT-(LOAD|REG|CRASH|EXIT|MENU)|PANIC|HALTED)' $$f; exit 1; }; \
	if grep -q '^PANIC' $$f; then printf '!!! test-flair-i123-bochs FAIL: %s PANIC\n' $$e; exit 1; fi; \
	grep -E '^(TENANT-|TBX-SMOKE-)' $$f > $$f.trace; \
done
@grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=33 y=64 s="$(I123_SMOKE_ROW1)" fg=1 bg=0 -> 0' $(BUILD)/flair_i123_smoke_qemu.serial.trace && grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_i123_smoke_qemu.serial.trace || { printf '!!! test-flair-i123-bochs FAIL: the QEMU reference smoke did not render row 1 as "123 1.5 -7 1.234567" or did not quit cleanly\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 ' $(BUILD)/flair_i123_smoke_qemu.serial.trace | tail -2; exit 1; }
@cmp -s $(BUILD)/flair_i123_smoke_qemu.serial.trace $(BUILD)/flair_i123_smoke_bochs.serial.trace || { printf '!!! test-flair-i123-bochs FAIL: QEMU and Bochs disagree on the 123 CPU path (stop condition: investigate, do not pin to QEMU)\n'; diff -u $(BUILD)/flair_i123_smoke_qemu.serial.trace $(BUILD)/flair_i123_smoke_bochs.serial.trace | head -30; exit 1; }
@printf '>>> test-flair-i123-bochs: %s TENANT-*/TBX-SMOKE-* lines byte-identical on QEMU and Bochs (load + BSS, Face A paint, x87 entry and display, Ctrl-Q)\n' "$$(wc -l < $(BUILD)/flair_i123_smoke_qemu.serial.trace)"
@printf '>>> test-flair-i123-bochs: green\n'
```

## test-flair-i123-copy

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35641):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data.img build/test_i123_copy_move build/ppm_text_check`
```make
$(call appl-boot,flair_i123_copy,$(FLAIR_I123_KEYS_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_COPY,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_copy.serial) && $(call I123_COPY_POINT_OK,$(BUILD)/flair_i123_copy.serial)
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_copy.serial
@mcopy -o -i $(BUILD)/flair_i123_copy_data.img ::SINGLE.WK1 $(BUILD)/i123-copy-guest.WK1
@LOTUS123_DECOMP=$(LOTUS123_DECOMP) $(BUILD)/test_i123_copy_move --saved fixtures/i123-copy/SINGLE.WK1 $(BUILD)/i123-copy-guest.WK1 $(BUILD)
$(PPM_TEXT_CHECK_BIN) --i123-copy $(BUILD)/flair_i123_copy.ppm
@printf '>>> test-flair-i123-copy: green (QEMU rectangle save vs real Lotus; POINT key replay and pixels)\n'
```

## test-flair-i123-copy-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35664):
Prerequisites: `test-flair-i123-copy-cpu build/bochs_harness build/.makefile.stamp`
```make
cp -f $(FLAIR_DATA_IMG) $(BUILD)/i123_copy_cpu_bochs_data.img
@$(BOCHS_BIN) --disk $(FLAIR_I123_COPY_SMOKE_IMG) --disk2 $(BUILD)/i123_copy_cpu_bochs_data.img --expect HALTED --name i123_copy_cpu_bochs --out $(BUILD) --timeout-ms 240000 2> $(BUILD)/i123_copy_cpu_bochs.report || true
@$(call I123_COPY_CPU_OK,bochs)
@printf '>>> test-flair-i123-copy-bochs: green (SINGLE/ROW/OVER/BLOCK saved cells match real R2.2 on both CPUs; POINT replay)\n'
```

## test-flair-i123-copy-cpu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35650):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_tbx_smoke_i123_copy.img build/flair_data.img build/test_i123_copy_move build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(BUILD)/i123_copy_cpu_qemu_data.img
@$(HARNESS_BIN) --disk $(FLAIR_I123_COPY_SMOKE_IMG) --disk2 $(BUILD)/i123_copy_cpu_qemu_data.img --expect HALTED --name i123_copy_cpu_qemu --out $(BUILD) --timeout-ms 30000 2> $(BUILD)/i123_copy_cpu_qemu.report || true
@$(call I123_COPY_CPU_OK,qemu)
@printf '>>> test-flair-i123-copy-cpu: green (QEMU SINGLE/ROW/OVER/BLOCK vs real R2.2; POINT replay)\n'
```

## test-flair-i123-copy-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35672):
Prerequisites: `test-flair-i123-copy build/flair_data_i123_period_text.img`
```make
$(call appl-boot,flair_i123_copy_mut,$(FLAIR_I123_KEYS_GATE_IMG),$(FLAIR_DATA_I123_PERIOD_TEXT_IMG),FLAIR_I123_COPY,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_copy_mut.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_copy_mut.serial
@mcopy -o -i $(BUILD)/flair_i123_copy_mut_data.img ::SINGLE.WK1 $(BUILD)/i123-copy-mut.WK1
@LOTUS123_DECOMP=$(LOTUS123_DECOMP) $(BUILD)/test_i123_copy_move --saved fixtures/i123-copy/SINGLE.WK1 $(BUILD)/i123-copy-mut.WK1 $(BUILD)
@if $(call I123_COPY_POINT_OK,$(BUILD)/flair_i123_copy_mut.serial); then printf '!!! period-as-text passed POINT serial oracle\n'; exit 1; fi
@if $(PPM_TEXT_CHECK_BIN) --i123-copy $(BUILD)/flair_i123_copy_mut.ppm; then printf '!!! period-as-text passed POINT pixel oracle\n'; exit 1; fi
@printf '>>> test-flair-i123-copy-mutant: green (rectangle save passes; period-as-text fails serial and pixels)\n'
```

## test-flair-i123-entry

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33229):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_entry_gate.img build/flair_data.img`
```make
$(call need_lotus_goldens,test-flair-i123-entry)
$(call appl-boot,flair_i123_entry,$(FLAIR_I123_ENTRY_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_ENTRY,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_entry.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_entry.serial || { printf '!!! test-flair-i123-entry FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_entry.serial || { printf '!!! test-flair-i123-entry FAIL: input ring overrun\n'; exit 1; }
@grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=1 y=16 s="$(I123_ENTRY_TAIL)" fg=1 bg=0 -> 0' $(BUILD)/flair_i123_entry.serial || { printf '!!! test-flair-i123-entry FAIL: 104-character entry tail\n'; exit 1; }
@mcopy -o -i $(BUILD)/flair_i123_entry_data.img ::ENTRY.WK1 $(BUILD)/i123-entry.WK1 || { printf '!!! test-flair-i123-entry FAIL: ENTRY.WK1 not saved\n'; exit 1; }
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-entry.WK1 > $(BUILD)/i123-entry.ref
@grep -Eq "LABEL +len=111 +P:Default c0 r0 label=\"'$(I123_ENTRY_LABEL)\"$$" $(BUILD)/i123-entry.ref && grep -q 'consumed=.* OK$$' $(BUILD)/i123-entry.ref || { printf '!!! test-flair-i123-entry FAIL: independent WK1 reader must find all 104 characters in A1\n'; cat $(BUILD)/i123-entry.ref; exit 1; }
@printf '>>> test-flair-i123-entry: green (QEMU typed 104 characters, tail shown, A1 committed and saved; wk1_ref.py reads full label)\n'
```

## test-flair-i123-entry-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33251):
Prerequisites: `test-flair-i123-entry build/qemu_harness build/flair_tenants_mut_i123_entry_gate.img build/flair_data_i123_entry75.img`
```make
$(call appl-boot,flair_i123_entry_mut,$(FLAIR_I123_ENTRY_GATE_IMG),$(FLAIR_DATA_I123_ENTRY75_IMG),FLAIR_I123_ENTRY,)
@f=$(BUILD)/flair_i123_entry_mut.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && \
 grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=1 y=16 s="$(I123_ENTRY_AT75)" fg=1 bg=0 -> 0' $$f || \
 { printf '!!! test-flair-i123-entry-mutant FAIL: ENTRY_75 went RED for the wrong reason (want: 123 up, the line at 75 characters)\n'; exit 1; }
@if grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=1 y=16 s="$(I123_ENTRY_TAIL)" fg=1 bg=0 -> 0' $(BUILD)/flair_i123_entry_mut.serial; then \
 printf '!!! test-flair-i123-entry-mutant FAIL: ENTRY_75 PASSED the tail check -- the QEMU leg is decoration\n'; exit 1; fi
@printf '>>> test-flair-i123-entry-mutant: I123_MUT_ENTRY_75 correctly RED -- the entry line stopped at "...%s", the 104-character tail never drawn\n' "$$(printf %s '$(I123_ENTRY_AT75)' | tail -c 12)"
@printf '>>> test-flair-i123-entry-mutant: green\n'
```

## test-flair-i123-file-ui

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32066):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_i123_file_data.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_file_ui,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_FILE_UI,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call I123_UI_OK,$(BUILD)/flair_i123_file_ui.serial) || { printf '!!! test-flair-i123-file-ui FAIL: menu/prompt/visible refusal\n'; exit 1; }
$(call appl-boot,flair_i123_file_error,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_FILE_ERROR,)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_file_error.serial) && $(call I123_FILE_ROWS,$(BUILD)/flair_i123_file_error.serial) && grep -q 's="File not found' $(BUILD)/flair_i123_file_error.serial || { printf '!!! test-flair-i123-file-ui FAIL: error visible and old sheet retained\n'; exit 1; }
$(call appl-boot,flair_i123_file_bad,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_FILE_BAD,)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_file_bad.serial) && $(call I123_FILE_ROWS,$(BUILD)/flair_i123_file_bad.serial) && grep -q 's="Not a valid WK1 file' $(BUILD)/flair_i123_file_bad.serial || { printf '!!! test-flair-i123-file-ui FAIL: malformed Retrieve preserved old sheet after repaint\n'; exit 1; }
$(call appl-boot,flair_i123_cancel,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_CANCEL,)
@mcopy -o -i $(BUILD)/flair_i123_cancel_data.img ::MINT01.WK1 $(BUILD)/i123-cancel.WK1
@$(call APPL_ALIVE,$(BUILD)/flair_i123_cancel.serial) && grep -q 's="Cancel  Replace  Backup' $(BUILD)/flair_i123_cancel.serial && cmp $(BUILD)/i123-corpus-original.WK1 $(BUILD)/i123-cancel.WK1 || { printf '!!! test-flair-i123-file-ui FAIL: overwrite Cancel\n'; exit 1; }
$(call appl-boot,flair_i123_replace,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_REPLACE,)
@mcopy -o -i $(BUILD)/flair_i123_replace_data.img ::MINT01.WK1 $(BUILD)/i123-replace.WK1
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-replace.WK1 > $(BUILD)/i123-replace.ref
@$(call APPL_ALIVE,$(BUILD)/flair_i123_replace.serial) && grep -q "LABEL.*c0 r0 label=.*Changed" $(BUILD)/i123-replace.ref || { printf '!!! test-flair-i123-file-ui FAIL: overwrite Replace\n'; exit 1; }
fsck.fat -n $(BUILD)/flair_i123_replace_data.img
@printf '>>> test-flair-i123-file-ui: green (reference menus/prompts, visible refusal/error, Cancel/Replace judged by mtools)\n'
```

## test-flair-i123-file-ui-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32106):
Prerequisites: `test-flair-i123-file-ui build/flair_i123_file_mut_command_silent.img build/flair_i123_file_mut_get_clobber.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_ui_mut,$(FLAIR_I123_FILE_GATE_IMG),$(BUILD)/flair_i123_file_mut_command_silent.img,FLAIR_I123_FILE_UI,)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_ui_mut.serial) && grep -qF 's="Name of file to retrieve: A:\*.wk1' $(BUILD)/flair_i123_ui_mut.serial
@if $(call I123_UI_OK,$(BUILD)/flair_i123_ui_mut.serial); then printf '!!! test-flair-i123-file-ui-mutant FAIL: MENU_SPELLING survived\n'; exit 1; fi
@grep -qF 's="Vorksheet  Range' $(BUILD)/flair_i123_ui_mut.serial || { printf '!!! test-flair-i123-file-ui-mutant FAIL: wrong RED reason (MENU_SPELLING did not misspell the top line)\n'; exit 1; }
@printf '>>> test-flair-i123-file-ui-mutant: green (MENU_SPELLING correctly RED: the 123.RI top line is defended; file prompt and desktop intact)\n'
$(call appl-boot,flair_i123_get_mut,$(FLAIR_I123_FILE_GATE_IMG),$(BUILD)/flair_i123_file_mut_get_clobber.img,FLAIR_I123_FILE_BAD,)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_get_mut.serial) && grep -q 's="Not a valid WK1 file' $(BUILD)/flair_i123_get_mut.serial
@if $(call I123_FILE_ROWS,$(BUILD)/flair_i123_get_mut.serial); then printf '!!! test-flair-i123-file-ui-mutant FAIL: GET_CLOBBER survived\n'; exit 1; fi
@printf '>>> test-flair-i123-file-ui-mutant: green (GET_CLOBBER correctly RED: malformed file refused, old cells lost on repaint, desktop alive)\n'
```

## test-flair-i123-files

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32042):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_i123_file_data.img build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_files,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_FILES,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_files.serial) && [ "$$(grep -c '^TENANT-REGISTER ok app=123$$' $(BUILD)/flair_i123_files.serial)" = 2 ] && grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_i123_files.serial || { printf '!!! test-flair-i123-files FAIL: launch/quit/relaunch\n'; exit 1; }
@mcopy -o -i $(BUILD)/flair_i123_files_data.img ::X.WK1 $(BUILD)/i123-saved.WK1 || { printf '!!! test-flair-i123-files FAIL: Save did not create X.WK1\n'; exit 1; }
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-saved.WK1 > $(BUILD)/i123-saved.ref
@$(call I123_DISK_CELLS,$(BUILD)/i123-saved.ref) || { printf '!!! test-flair-i123-files FAIL: independent WK1 cells\n'; cat $(BUILD)/i123-saved.ref; exit 1; }
@$(call I123_FILE_ROWS,$(BUILD)/flair_i123_files.serial) || { printf '!!! test-flair-i123-files FAIL: retrieved cells\n'; exit 1; }
@set -- $$($(call I123_PX,$(BUILD)/flair_i123_files.ppm,39 126 111 142,9 112)); [ "$$1" -ge 40 ] || { printf '!!! test-flair-i123-files FAIL: cells not presented\n'; exit 1; }
fsck.fat -n $(BUILD)/flair_i123_files_data.img
$(call appl-boot,flair_i123_corpus,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_FILE_DATA),FLAIR_I123_CORPUS,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_corpus.serial) && $(call I123_FILE_ROWS,$(BUILD)/flair_i123_corpus.serial) && grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=80 .* fg=1 bg=0 -> 0$$' $(BUILD)/flair_i123_corpus.serial | tail -1 | grep -q 's="     246    117\.5 39\.16666 big *"' || { printf '!!! test-flair-i123-files FAIL: real MINT01 rows vs screenshot\n'; exit 1; }
@set -- $$($(call I123_PX,$(BUILD)/flair_i123_corpus.ppm,255 142 327 158,9 112)); [ "$$2" -ge 900 ] || { printf '!!! test-flair-i123-files FAIL: corpus pointer/cells not presented\n'; exit 1; }
@mcopy -o -i $(BUILD)/flair_i123_corpus_data.img ::MINT01.WK1 $(BUILD)/i123-corpus-after.WK1
@cmp $(BUILD)/i123-corpus-original.WK1 $(BUILD)/i123-corpus-after.WK1
@printf '>>> test-flair-i123-files: green (Save/Quit/Retrieve + wk1_ref.py + real MINT01 screenshot rows/pixels)\n'
```

## test-flair-i123-files-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32096):
Prerequisites: `test-flair-i123-files build/flair_i123_file_mut_save_empty.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_files_mut,$(FLAIR_I123_FILE_GATE_IMG),$(BUILD)/flair_i123_file_mut_save_empty.img,FLAIR_I123_FILES,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_files_mut.serial) && [ "$$(grep -c '^TENANT-REGISTER ok app=123$$' $(BUILD)/flair_i123_files_mut.serial)" = 2 ] && grep -q 's="Retrieved x.wk1' $(BUILD)/flair_i123_files_mut.serial
@mcopy -o -i $(BUILD)/flair_i123_files_mut_data.img ::X.WK1 $(BUILD)/i123-saved-mutant.WK1
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-saved-mutant.WK1 > $(BUILD)/i123-saved-mutant.ref
@grep -q 'consumed=.* OK$$' $(BUILD)/i123-saved-mutant.ref
@if $(call I123_DISK_CELLS,$(BUILD)/i123-saved-mutant.ref) || $(call I123_FILE_ROWS,$(BUILD)/flair_i123_files_mut.serial); then printf '!!! test-flair-i123-files-mutant FAIL: SAVE_EMPTY survived\n'; exit 1; fi
@if grep -Eq '^ +(LABEL|INTEGER|NUMBER|FORMULA) +len=' $(BUILD)/i123-saved-mutant.ref; then printf '!!! test-flair-i123-files-mutant FAIL: wrong RED reason (want no cells in valid file)\n'; exit 1; fi
@printf '>>> test-flair-i123-files-mutant: green (SAVE_EMPTY correctly RED: independent saved cells and retrieved row absent; valid WK1, desktop alive)\n'
```

## test-flair-i123-formula

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32275):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_data.img build/ppm_region_count build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-i123-formula : formulas typed into Initech 123 recalculate in the OS (initech-94ah)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_i123_formula,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_FORMULA_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_formula.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_formula.serial || { printf '!!! test-flair-i123-formula FAIL: 123 did not come up or the desktop died\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED)' $(BUILD)/flair_i123_formula.serial; exit 1; }
@$(call I123_FORMULA_ROWS,$(BUILD)/flair_i123_formula.serial) || { printf '!!! test-flair-i123-formula FAIL [1/3] VALUES: row 1 was not drawn as "123 1.5 124.5 -249" and, after A1 = 200, "200 1.5 201.5 -403"\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 ' $(BUILD)/flair_i123_formula.serial | tail -4; exit 1; }
@printf '>>> test-flair-i123-formula [1/3] VALUES: C1 @SUM(A1..B1) = 124.5, D1 -C1*2 = -249 drawn; A1 retyped 200 -> recalculated to 201.5 / -403\n'
@$(call I123_FORMULA_TEXT,$(BUILD)/flair_i123_formula.serial) || { printf '!!! test-flair-i123-formula FAIL [2/3] TEXT: line 1 does not show the formula text\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=1 y=0 ' $(BUILD)/flair_i123_formula.serial | tail -3; exit 1; }
@printf '>>> test-flair-i123-formula [2/3] TEXT: line 1 "D1: -C1*2", finally "B1: @SUM(A1..C1)" (the decompiled, uppercased formula)\n'
@$(call I123_FORMULA_CIRC,$(BUILD)/flair_i123_formula.serial) || { printf '!!! test-flair-i123-formula FAIL [3/3] CIRC: the indicator is missing, or lit before the circular entry\n'; grep 'y=368 ' $(BUILD)/flair_i123_formula.serial | tail -3; exit 1; }
@f=$(BUILD)/flair_i123_formula.ppm; \
	set -- $$($(call I123_PX,$$f,183 126 255 142,305 452)); cne=$$1; \
	set -- $$($(call I123_PX,$$f,583 430 615 446,9 112)); ieq=$$2; \
	if [ "$$cne" -lt 20 ] || [ "$$ieq" -lt 200 ]; then printf '!!! test-flair-i123-formula FAIL [3/3] SCREEN: ink in C1 %s px (want >= 20), CIRC highlight %s/512 px (want >= 200)\n' "$$cne" "$$ieq"; exit 1; fi; \
	printf '>>> test-flair-i123-formula [3/3] CIRC: drawn highlighted at the status line end only after B1 @SUM(A1..C1); screen: ink in C1 %s px, CIRC highlight %s/512 px\n' "$$cne" "$$ieq"
@printf '>>> test-flair-i123-formula: green\n'
```

## test-flair-i123-formula-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32293):
Prerequisites: `test-flair-i123-formula build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_data_i123_norecalc.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_formula_mut,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_DATA_I123_NORECALC_IMG),FLAIR_I123_FORMULA_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_formula_mut.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_formula_mut.serial && grep -qxF 'TENANT-GATE ax=0x0033 win=1 x=33 y=64 s="$(I123_FROW_A)" fg=1 bg=0 -> 0' $(BUILD)/flair_i123_formula_mut.serial || { printf '!!! test-flair-i123-formula-mutant FAIL: NO_RECALC went RED for the wrong reason (want: 123 up, the first formula row drawn)\n'; exit 1; }
@if $(call I123_FORMULA_ROWS,$(BUILD)/flair_i123_formula_mut.serial); then printf '!!! test-flair-i123-formula-mutant FAIL: NO_RECALC PASSED -- the recalculation leg is decoration\n'; exit 1; fi
@printf '>>> test-flair-i123-formula-mutant: NO_RECALC correctly RED -- A1 retyped 200 left row 1 at "%s"\n' "$$(grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 ' $(BUILD)/flair_i123_formula_mut.serial | grep '     200 ' | head -1 | cut -c45-80)"
@printf '>>> test-flair-i123-formula-mutant: green\n'
```

## test-flair-i123-hit

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33368):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_hit,$(FLAIR_I123_HIT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_HIT,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_hit.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-hit FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_hit.serial || { printf '!!! test-flair-i123-hit FAIL: input ring overrun\n'; exit 1; }
@$(call I123_HIT_TAB_OK,$(BUILD)/flair_i123_hit.serial) || { printf '!!! test-flair-i123-hit FAIL: H5 after Tab the click did not select M4 (address + pointer)\n'; exit 1; }
@$(call I123_HIT_RESIZE_OK,$(BUILD)/flair_i123_hit.serial) || { printf '!!! test-flair-i123-hit FAIL: H9 grow (640,440)->(440,340) then the click did not select E4 (address + pointer + READY)\n'; exit 1; }
$(PPM_TEXT_CHECK_BIN) --i123-hit $(BUILD)/flair_i123_hit.ppm
@printf '>>> test-flair-i123-hit: green (QEMU: Tab + click M4, Shift-Tab, grow 640x440->440x340 + click E4; TENANT-GATE draws + screendump)\n'
```

## test-flair-i123-hit-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33384):
Prerequisites: `test-flair-i123-hit build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data_i123_hitscroll.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_hit_mut,$(FLAIR_I123_HIT_GATE_IMG),$(FLAIR_DATA_I123_HITSCROLL_IMG),FLAIR_I123_HIT,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_hit_mut.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && \
 grep -qE '$(I123_HIT_GROW)' $$f && \
 [ "$$(grep -cE '^$(call I123_HIT_G,1,0,E4: +,1,0)$$' $$f)" -ge 2 ] || \
 { printf '!!! test-flair-i123-hit-mutant FAIL: HIT_IGNORES_SCROLL went RED for the wrong reason (want: 123 up, grow done, E4 selected by both clicks)\n'; exit 1; }
@if $(call I123_HIT_TAB_OK,$(BUILD)/flair_i123_hit_mut.serial); then \
 printf '!!! test-flair-i123-hit-mutant FAIL: HIT_IGNORES_SCROLL PASSED the after-Tab check -- the QEMU leg is decoration\n'; exit 1; fi
@if $(call I123_HIT_RESIZE_OK,$(BUILD)/flair_i123_hit_mut.serial); then \
 printf '!!! test-flair-i123-hit-mutant FAIL: HIT_IGNORES_SCROLL PASSED the resize check (E4 before the grow must fail it)\n'; exit 1; fi
@printf '>>> test-flair-i123-hit-mutant: I123_MUT_HIT_IGNORES_SCROLL correctly RED -- the click after Tab drew "E4:", never "M4:"\n'
@printf '>>> test-flair-i123-hit-mutant: green\n'
```

## test-flair-i123-keys

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33302):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_keys,$(FLAIR_I123_KEYS_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_KEYS,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_keys.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-keys FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_keys.serial || { printf '!!! test-flair-i123-keys FAIL: input ring overrun\n'; exit 1; }
@$(call I123_KEYS_F2_OK,$(BUILD)/flair_i123_keys.serial) || { printf '!!! test-flair-i123-keys FAIL: K1 F2 EDIT "'"'"'Hello", Esc erase, Esc READY with A1 unchanged\n'; exit 1; }
@$(call I123_KEYS_GOTO_OK,$(BUILD)/flair_i123_keys.serial) || { printf '!!! test-flair-i123-keys FAIL: K8 F5 prompt/POINT or Z100 at the upper-left\n'; exit 1; }
@$(call I123_KEYS_PGDN_OK,$(BUILD)/flair_i123_keys.serial) || { printf '!!! test-flair-i123-keys FAIL: K13 PgDn from A1 to A20 with row 20 first\n'; exit 1; }
@$(call I123_KEYS_TAB_OK,$(BUILD)/flair_i123_keys.serial) || { printf '!!! test-flair-i123-keys FAIL: K18 Tab from A1 to I1 with column I first\n'; exit 1; }
$(PPM_TEXT_CHECK_BIN) --i123-keys $(BUILD)/flair_i123_keys.ppm
@printf '>>> test-flair-i123-keys: green (QEMU: F2 EDIT + Esc/Esc, F5 Z100 upper-left, PgDn A1->A20, Tab A1->I1; TENANT-GATE draws + screendump)\n'
```

## test-flair-i123-keys-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33321):
Prerequisites: `test-flair-i123-keys build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data_i123_tab1.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_keys_mut,$(FLAIR_I123_KEYS_GATE_IMG),$(FLAIR_DATA_I123_TAB1_IMG),FLAIR_I123_KEYS,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_keys_mut.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && \
 $(call I123_KEYS_PGDN_OK,$$f) && grep -qE '^$(call I123_KEYS_G,1,0,B1: +,1,0)$$' $$f || \
 { printf '!!! test-flair-i123-keys-mutant FAIL: TAB_ONE_CELL went RED for the wrong reason (want: 123 up, PgDn green, Tab on B1)\n'; exit 1; }
@if $(call I123_KEYS_TAB_OK,$(BUILD)/flair_i123_keys_mut.serial); then \
 printf '!!! test-flair-i123-keys-mutant FAIL: TAB_ONE_CELL PASSED the Tab check -- the QEMU leg is decoration\n'; exit 1; fi
@if $(PPM_TEXT_CHECK_BIN) --i123-keys $(BUILD)/flair_i123_keys_mut.ppm; then \
 printf '!!! test-flair-i123-keys-mutant FAIL: TAB_ONE_CELL PASSED the screendump check\n'; exit 1; fi
@printf '>>> test-flair-i123-keys-mutant: I123_MUT_TAB_ONE_CELL correctly RED -- Tab drew "B1:", never "I1:"; screendump pointer off column one\n'
@printf '>>> test-flair-i123-keys-mutant: green\n'
```

## test-flair-i123-menu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33560):
Prerequisites: `test-i123-menu-tree build/check_i123_menu_levels build/qemu_harness build/flair_tenants_mut_i123_menu_gate.img build/flair_data.img build/ppm_text_check build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_menu_ops,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_OPS,$(FLAIR_I123_MENU_ACK) --screendump --screendump-after FLAIR-LIVE-OK)
@! grep -q '^FLAIR-EVT-DROP' $(BUILD)/flair_i123_menu_ops.serial
@test "$$(grep -c '^FLAIR-EVT what=3 ' $(BUILD)/flair_i123_menu_ops.serial)" -eq 50
@$(call I123_MENU_OPS_OK,$(BUILD)/flair_i123_menu_ops.serial) || { printf '!!! test-flair-i123-menu FAIL: copy/move row, menu text or MENU\n'; exit 1; }
$(PPM_TEXT_CHECK_BIN) --i123-menu $(BUILD)/flair_i123_menu_ops.ppm
$(call appl-boot,flair_i123_menu_tree,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_TREE,$(FLAIR_I123_MENU_ACK))
@f=$(BUILD)/flair_i123_menu_tree.serial; $(call APPL_ALIVE,$$f) && \
 grep -qF 's="Attach  Detach  Invoke  Clear  Quit' $$f && grep -qF 's="No  Yes' $$f && \
 grep -qF 's="Command not implemented' $$f || { printf '!!! test-flair-i123-menu FAIL: unavailable leaf / Quit navigation\n'; exit 1; }
$(call appl-boot,flair_i123_menu_level_1,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_1,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_1.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_1.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 1
$(call appl-boot,flair_i123_menu_level_2,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_2,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_2.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_2.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 2
$(call appl-boot,flair_i123_menu_level_3,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_3,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_3.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_3.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 3
$(call appl-boot,flair_i123_menu_level_4,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_4,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_4.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_4.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 4
$(call appl-boot,flair_i123_menu_level_5,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_5,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_5.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_5.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 5
$(call appl-boot,flair_i123_menu_level_6,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_6,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_6.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_6.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 6
$(call appl-boot,flair_i123_menu_level_7,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_7,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_7.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_7.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 7
$(call appl-boot,flair_i123_menu_level_8,$(FLAIR_I123_MENU_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_MENU_LEVEL_8,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_level_8.serial)
@$(BUILD)/check_i123_menu_levels $(BUILD)/flair_i123_menu_level_8.serial spec/i123/menu_tree.golden spec/i123/menu_level_visits.tsv 8
@printf '>>> test-flair-i123-menu: green (tree, RI descriptions, Copy/Move row and MENU pixel bands)\n'
```

## test-flair-i123-menu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33595):
Prerequisites: `test-flair-i123-menu build/flair_i123_menu_stale.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_menu_mut,$(FLAIR_I123_MENU_GATE_IMG),$(BUILD)/flair_i123_menu_stale.img,FLAIR_I123_MENU_OPS,$(FLAIR_I123_MENU_ACK))
@$(call APPL_ALIVE,$(BUILD)/flair_i123_menu_mut.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_menu_mut.serial
@! grep -q '^FLAIR-EVT-DROP' $(BUILD)/flair_i123_menu_mut.serial
@grep '^FLAIR-EVT what=3 ' $(BUILD)/flair_i123_menu_ops.serial > $(BUILD)/flair_i123_menu_ops.keys
@grep '^FLAIR-EVT what=3 ' $(BUILD)/flair_i123_menu_mut.serial > $(BUILD)/flair_i123_menu_mut.keys
diff -u $(BUILD)/flair_i123_menu_ops.keys $(BUILD)/flair_i123_menu_mut.keys
@if $(call I123_MENU_OPS_OK,$(BUILD)/flair_i123_menu_mut.serial); then printf '!!! Move stale mutant PASSED\n'; exit 1; fi
@grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 ' $(BUILD)/flair_i123_menu_mut.serial | tail -1 | grep -q '     -80      160        0 '
@printf '>>> test-flair-i123-menu-mutant: green (Move left H1 stale: 0 instead of 80, correctly RED)\n'
```

## test-flair-i123-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23835):
Prerequisites: `test-flair-i123 build/qemu_harness build/flair_tenants.img build/flair_data_i123_arrows.img build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_mut,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_I123_ARROWS_IMG),FLAIR_I123_TYPE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_mut.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_mut.serial || { printf '!!! test-flair-i123-mutant FAIL: ARROWS_DEAD went RED for the wrong reason (want: 123 up and typed into, desktop alive)\n'; exit 1; }
@if $(call I123_TYPE_SERIAL,$(BUILD)/flair_i123_mut.serial); then printf '!!! test-flair-i123-mutant FAIL: ARROWS_DEAD PASSED -- the TYPE leg is decoration\n'; exit 1; fi
@set -- $$($(call I123_PX,$(BUILD)/flair_i123_mut.ppm,255 126 327 142,9 112)); \
	if [ "$$2" -ge 900 ]; then printf '!!! test-flair-i123-mutant FAIL: ARROWS_DEAD still shows the pointer on D1\n'; exit 1; fi; \
	printf '>>> test-flair-i123-mutant: ARROWS_DEAD correctly RED -- row 1 is not the MINT01 row (last row-1 draw: %s) and the pointer is not on D1 (%s px)\n' "$$(grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 .* fg=1 bg=0 -> 0$$' $(BUILD)/flair_i123_mut.serial | tail -1 | cut -c38-80)" "$$2"
@printf '>>> test-flair-i123-mutant: green\n'
```

## test-flair-i123-numeric

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wk1_diff/i123_numeric.mk', line 20):
Prerequisites: `build/qemu_harness build/flair_tenants_recorddbl.img build/flair_data.img build/check_i123_numeric`
```make
$(call need_lotus_goldens,test-flair-i123-numeric)
$(call appl-boot,flair_i123_numeric,$(FLAIRTENANTS_RECORDDBL_IMG),$(FLAIR_DATA_IMG),I123_NUMERIC_TRACE,--rtc-base 1992-03-09T12:00:00 --rtc-deterministic --screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_i123_numeric.serial) && grep -qx 'TENANT-REGISTER ok app=123' $(BUILD)/flair_i123_numeric.serial
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_numeric.serial
@mcopy -o -i $(BUILD)/flair_i123_numeric_data.img ::NOW1.WK1 $(BUILD)/i123-NOW1.WK1
@mcopy -o -i $(BUILD)/flair_i123_numeric_data.img ::NOW2.WK1 $(BUILD)/i123-NOW2.WK1
@$(BUILD)/check_i123_numeric $(BUILD)/i123-NOW1.WK1 $(BUILD)/i123-NOW2.WK1
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-NOW2.WK1 > $(BUILD)/i123-NOW2.ref
@grep -q 'consumed=.* OK$$' $(BUILD)/i123-NOW2.ref
@printf '>>> test-flair-i123-numeric: green\n'
```

## test-flair-i123-quitguard

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32175):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_i123_quit_fixture.img build/ppm_region_count build/ppm_flair_trash_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-i123-quitguard : 123 asks before discarding work (initech-fvnj, L008)\n'
@printf '  Ref: R2.2 Reference Manual p. 9-1; 123.RI 0x300-0x370; spec/toolbox_gate.h Sec 11.\n'
@printf '======================================================================\n'
$(call appl-boot,flair_i123_qg_alert,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_ALERT,--screendump --screendump-after "TENANT-ALERT up")
@f=$(BUILD)/flair_i123_qg_alert.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-LINE Save changes to the worksheet' $$f && grep -qx 'TENANT-ALERT-LINE before quitting?' $$f && $(call QG_NO_EXIT,$$f) || { printf '!!! test-flair-i123-quitguard [1/9] FAIL: ALERT -- a dirty Ctrl-Q must ask (and never exit)\n'; grep -E '^(TENANT-(GATE ax=0x0090|ALERT|EXIT|MENU|CRASH)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) tenant3 $(BUILD)/flair_i123_qg_alert.ppm > $(BUILD)/flair_i123_qg_alert.pixels || { cat $(BUILD)/flair_i123_qg_alert.pixels; printf '!!! test-flair-i123-quitguard [1/9] FAIL: ALERT -- the three-button alert is not drawn as graded\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard [1/9] ALERT: Ctrl-Q over the typed MINT01 row asks "WORKSHEET CHANGES NOT SAVED!" with Save / Cancel / Don'"'"'t Save (pixel-graded), no exit\n'
$(call appl-boot,flair_i123_qg_cancel,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_CANCEL,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_qg_cancel.serial; $(call APPL_ALIVE,$$f) && [ "$$(grep -c '^TENANT-ALERT up lines=3 buttons=3$$' $$f)" = 2 ] && $(call QG_CANCELLED,$$f) || { printf '!!! test-flair-i123-quitguard [2/9] FAIL: CANCEL -- Escape must keep 123 and its row, and the sheet must stay dirty (asked twice)\n'; grep -E '^(TENANT-(ALERT|EXIT|EVT what=144)|PANIC)' $$f; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) tenant3-gone $(BUILD)/flair_i123_qg_cancel.ppm > $(BUILD)/flair_i123_qg_cancel.pixels || { cat $(BUILD)/flair_i123_qg_cancel.pixels; printf '!!! test-flair-i123-quitguard [2/9] FAIL: CANCEL -- the alert footprint was not repainted\n'; exit 1; }
@set -- $$($(call I123_PX,$(BUILD)/flair_i123_qg_cancel.ppm,39 126 111 142,9 112)); [ "$$1" -ge 40 ] || { printf '!!! test-flair-i123-quitguard [2/9] FAIL: CANCEL -- row 1 is not on screen after Cancel\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard [2/9] CANCEL: Escape twice -> two Cancel answers, no exit, row 1 drawn from the model, the alert footprint repainted\n'
$(call appl-boot,flair_i123_qg_dontsave,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_DONTSAVE,)
@f=$(BUILD)/flair_i123_qg_dontsave.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=3' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f || { printf '!!! test-flair-i123-quitguard [3/9] FAIL: DONTSAVE -- the Don'"'"'t Save button must end 123\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@[ "$$(mdir -b -i $(BUILD)/flair_i123_qg_dontsave_data.img :: | grep -i '\.WK1$$' | sort)" = "$$(mdir -b -i $(FLAIR_I123_QUIT_DATA) :: | grep -i '\.WK1$$' | sort)" ] && ! grep -q '^TENANT-GATE ax=008[03] ' $(BUILD)/flair_i123_qg_dontsave.serial || { printf '!!! test-flair-i123-quitguard [3/9] FAIL: DONTSAVE -- a worksheet was written (mtools / the file-verb trace)\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard [3/9] DONTSAVE: the third button -> EXIT(0); no CREATE/WRITE, mtools: the .WK1 set is unchanged\n'
$(call appl-boot,flair_i123_qg_pending,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_PENDING,)
@f=$(BUILD)/flair_i123_qg_pending.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=2' $$f && $(call QG_NO_EXIT,$$f) && \
	awk '/^TENANT-ALERT up /{a=1} a && /^TENANT-GATE ax=0x0033 win=1 x=1 y=16 s="Hel_ /{b=1} END{exit !b}' $$f && \
	grep '^TENANT-GATE ax=0x0033 win=1 x=33 y=64 ' $$f | tail -1 | grep -q ' s="Help  ' || { printf '!!! test-flair-i123-quitguard [4/9] FAIL: PENDING -- an entry being typed must ask, survive Cancel ("Hel_") and then commit (A1 Help)\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; grep -E 'x=(1 y=16|33 y=64) ' $$f | tail -4; exit 1; }
@printf '>>> test-flair-i123-quitguard [4/9] PENDING: "Hel" (not entered) asks; Cancel keeps "Hel_"; p + Return commits A1 Help\n'
$(call appl-boot,flair_i123_qg_save,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_SAVE,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_qg_save.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=1' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && [ "$$(grep -c '^TENANT-REGISTER ok app=123$$' $$f)" = 2 ] && $(call I123_FILE_ROWS,$$f) || { printf '!!! test-flair-i123-quitguard [5/9] FAIL: SAVE -- Save, q must write, end 123, and the relaunch must retrieve the row\n'; grep -E '^(TENANT-(ALERT|EXIT|REGISTER|GATE ax=008)|PANIC)' $$f; exit 1; }
@mcopy -o -i $(BUILD)/flair_i123_qg_save_data.img ::Q.WK1 $(BUILD)/i123-qg-saved.WK1 || { printf '!!! test-flair-i123-quitguard [5/9] FAIL: SAVE -- no Q.WK1 on the volume (mtools)\n'; exit 1; }
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-qg-saved.WK1 > $(BUILD)/i123-qg-saved.ref
@$(call I123_DISK_CELLS,$(BUILD)/i123-qg-saved.ref) || { printf '!!! test-flair-i123-quitguard [5/9] FAIL: SAVE -- Q.WK1 does not hold the row per wk1_ref.py\n'; cat $(BUILD)/i123-qg-saved.ref; exit 1; }
@set -- $$($(call I123_PX,$(BUILD)/flair_i123_qg_save.ppm,39 126 111 142,9 112)); [ "$$1" -ge 40 ] || { printf '!!! test-flair-i123-quitguard [5/9] FAIL: SAVE -- the retrieved row is not on screen\n'; exit 1; }
@fsck.fat -n $(BUILD)/flair_i123_qg_save_data.img > /dev/null || { printf '!!! test-flair-i123-quitguard [5/9] FAIL: SAVE -- fsck.fat\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard [5/9] SAVE: Return (Save), q -> Q.WK1 (wk1_ref.py: Hello 123 1.5 -7), EXIT(0); relaunch + /FR q shows the row; fsck clean\n'
$(call appl-boot,flair_i123_qg_savefail,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_SAVEFAIL,)
@f=$(BUILD)/flair_i123_qg_savefail.serial; $(call APPL_ALIVE,$$f) && [ "$$(grep -c '^TENANT-ALERT up lines=3 buttons=3$$' $$f)" = 2 ] && grep -qF 's="Cancel  Replace  Backup' $$f && grep -q '^TENANT-GATE ax=0x0086 path="ro.wk1" -> -261$$' $$f && grep -q 's="File I/O error' $$f && $(call QG_NO_EXIT,$$f) || { printf '!!! test-flair-i123-quitguard [6/9] FAIL: SAVEFAIL -- a refused Save must keep 123 open and dirty (asked again)\n'; grep -E '^(TENANT-(ALERT|EXIT|GATE ax=008)|PANIC)' $$f; exit 1; }
@mcopy -o -i $(BUILD)/flair_i123_qg_savefail_data.img ::RO.WK1 $(BUILD)/i123-qg-ro.WK1 && cmp -s $(BUILD)/i123-corpus-original.WK1 $(BUILD)/i123-qg-ro.WK1 && mattrib -i $(BUILD)/flair_i123_qg_savefail_data.img ::RO.WK1 | grep -q ' R .*RO.WK1' || { printf '!!! test-flair-i123-quitguard [6/9] FAIL: SAVEFAIL -- RO.WK1 changed or lost its read-only bit (mtools)\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard [6/9] SAVEFAIL: Save + Replace onto read-only RO.WK1 -> DOS 5, "File I/O error", no exit, asked again; RO.WK1 byte-identical\n'
$(call appl-boot,flair_i123_qg_close,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_CLOSE,)
@f=$(BUILD)/flair_i123_qg_close.serial; $(call APPL_ALIVE,$$f) && $(call QG_CLOSE_ASKED,$$f) || { printf '!!! test-flair-i123-quitguard [7/9] FAIL: CLOSE -- a dirty close box must ask, not terminate\n'; grep -E '^(FLAIR-CLOSE|TENANT-(ALERT|EXIT|EVT what=32)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-i123-quitguard [7/9] CLOSE: the close box over a dirty sheet -> FLAIR-CLOSE-ASK, TBX_EVT_GOAWAY, the alert; Escape keeps 123\n'
$(call appl-boot,flair_i123_qg_closeclean,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_CLOSECLEAN,)
@f=$(BUILD)/flair_i123_qg_closeclean.serial; $(call APPL_ALIVE,$$f) && grep -q '^FLAIR-CLOSE-ASK win ' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && ! grep -q '^TENANT-ALERT' $$f || { printf '!!! test-flair-i123-quitguard [8/9] FAIL: CLOSECLEAN -- a clean close box must end 123 at once\n'; grep -E '^(FLAIR-CLOSE|TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-i123-quitguard [8/9] CLOSECLEAN: the close box over a clean sheet -> EXIT(0), no question\n'
$(call appl-boot,flair_i123_qg_slash,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_SLASH,)
@f=$(BUILD)/flair_i123_qg_slash.serial; $(call APPL_ALIVE,$$f) && grep -qF 's="No  Yes' $$f && grep -qF 's="Do not end 1-2-3 session; return to READY mode' $$f && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=2' $$f && $(call QG_NO_EXIT,$$f) && grep '^TENANT-GATE ax=0x0033 win=1 x=569 y=0 ' $$f | tail -1 | grep -q 's="READY"' || { printf '!!! test-flair-i123-quitguard [9/9] FAIL: SLASH -- /Quit must offer No / Yes and Yes must ask\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
@printf '>>> test-flair-i123-quitguard [9/9] SLASH: /Quit shows "No  Yes" (123.RI 0x300/0x332), Yes asks, Escape -> READY\n'
@printf '>>> test-flair-i123-quitguard: green\n'
```

## test-flair-i123-quitguard-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32221):
Prerequisites: `test-flair-i123-quitguard build/qemu_harness build/flair_tenants_mut_i123_file_gate.img build/flair_tenants_mut_i123_close_noask.img build/flair_i123_file_mut_never_dirty.img build/flair_i123_file_mut_cancel_quits.img build/flair_i123_quit_fixture.img build/.makefile.stamp`
```make
$(call appl-boot,flair_i123_qg_mut_dirty,$(FLAIR_I123_FILE_GATE_IMG),$(BUILD)/flair_i123_file_mut_never_dirty.img,FLAIR_I123_QG_ALERT,)
@f=$(BUILD)/flair_i123_qg_mut_dirty.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && grep -qx 'TENANT-MENU menu=129 item=1 src=key' $$f || { printf '!!! test-flair-i123-quitguard-mutant FAIL: NEVER_DIRTY went RED for the wrong reason (want: 123 up, Ctrl-Q heard, desktop alive)\n'; exit 1; }
@f=$(BUILD)/flair_i123_qg_mut_dirty.serial; if $(call QG_ASKED,$$f) && $(call QG_NO_EXIT,$$f); then printf '!!! test-flair-i123-quitguard-mutant FAIL: NEVER_DIRTY PASSED leg 1 -- the ALERT leg is decoration\n'; exit 1; fi
@grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_i123_qg_mut_dirty.serial || { printf '!!! test-flair-i123-quitguard-mutant FAIL: NEVER_DIRTY: wrong RED reason (want the silent exit)\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard-mutant: NEVER_DIRTY correctly RED (leg 1: Ctrl-Q over the typed row exited silently -- L008)\n'
$(call appl-boot,flair_i123_qg_mut_cancel,$(FLAIR_I123_FILE_GATE_IMG),$(BUILD)/flair_i123_file_mut_cancel_quits.img,FLAIR_I123_QG_CANCEL,)
@f=$(BUILD)/flair_i123_qg_mut_cancel.serial; $(call APPL_ALIVE,$$f) && $(call QG_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=2' $$f || { printf '!!! test-flair-i123-quitguard-mutant FAIL: CANCEL_QUITS went RED for the wrong reason (want: asked, Cancel answered)\n'; exit 1; }
@f=$(BUILD)/flair_i123_qg_mut_cancel.serial; if $(call QG_CANCELLED,$$f); then printf '!!! test-flair-i123-quitguard-mutant FAIL: CANCEL_QUITS PASSED leg 2 -- the CANCEL leg is decoration\n'; exit 1; fi
@grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_i123_qg_mut_cancel.serial || { printf '!!! test-flair-i123-quitguard-mutant FAIL: CANCEL_QUITS: wrong RED reason (want the exit after Cancel)\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard-mutant: CANCEL_QUITS correctly RED (leg 2: Cancel ended 123)\n'
$(call appl-boot,flair_i123_qg_mut_close,$(FLAIR_I123_CLOSE_NOASK_IMG),$(FLAIR_I123_QUIT_DATA),FLAIR_I123_QG_CLOSE,)
@f=$(BUILD)/flair_i123_qg_mut_close.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-quitguard-mutant FAIL: CLOSE_NO_ASK went RED for the wrong reason (want: 123 up, desktop alive)\n'; exit 1; }
@f=$(BUILD)/flair_i123_qg_mut_close.serial; if $(call QG_CLOSE_ASKED,$$f); then printf '!!! test-flair-i123-quitguard-mutant FAIL: CLOSE_NO_ASK PASSED leg 7 -- the CLOSE leg is decoration\n'; exit 1; fi
@grep -qx 'TENANT-EXIT rc=0 via=close' $(BUILD)/flair_i123_qg_mut_close.serial || { printf '!!! test-flair-i123-quitguard-mutant FAIL: CLOSE_NO_ASK: wrong RED reason (want the terminate via=close)\n'; exit 1; }
@printf '>>> test-flair-i123-quitguard-mutant: CLOSE_NO_ASK correctly RED (leg 7: the close box terminated the dirty sheet)\n'
@# the pixel legs bite (Rule 6): each must refuse the OTHER leg's dump.
@if $(PPM_FLAIR_TRASH_CHECK_BIN) tenant3 $(BUILD)/flair_i123_qg_cancel.ppm > /dev/null 2>&1; then printf '!!! test-flair-i123-quitguard-mutant FAIL: leg tenant3 PASSED a dump with no alert\n'; exit 1; fi
@if $(PPM_FLAIR_TRASH_CHECK_BIN) tenant3-gone $(BUILD)/flair_i123_qg_alert.ppm > /dev/null 2>&1; then printf '!!! test-flair-i123-quitguard-mutant FAIL: leg tenant3-gone PASSED a dump with the alert up\n'; exit 1; fi
@printf '>>> test-flair-i123-quitguard-mutant: the pixel legs bite (tenant3 refuses the alert-gone dump, tenant3-gone the alert-up dump)\n'
@printf '>>> test-flair-i123-quitguard-mutant: green (all three mutants RED for the named reason)\n'
```

## test-flair-i123-resize

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33448):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_resize,$(FLAIR_I123_RESIZE_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_RESIZE,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_resize.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-resize FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_resize.serial || { printf '!!! test-flair-i123-resize FAIL: input ring overrun\n'; exit 1; }
@$(call I123_RS1_OK,$(BUILD)/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS1 shrunk to 352x246, READY not at the content edge (x=281)\n'; exit 1; }
@$(call I123_RS2_OK,$(BUILD)/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS2 (L018) Right to E1 at 352x246 -- border B..E, pointer x=249, "E1:", READY\n'; exit 1; }
@$(call I123_RS3_OK,$(BUILD)/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS3 grown to 552x346 -- border B..G, pointer x=249, READY x=481\n'; exit 1; }
@$(call I123_RS4_OK,$(BUILD)/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS4 shrunk to 280x246 -- border C..E, pointer x=177, READY x=209\n'; exit 1; }
$(PPM_TEXT_CHECK_BIN) --i123-resize $(BUILD)/flair_i123_resize.ppm
@printf '>>> test-flair-i123-resize: green (QEMU: shrink 640x440->352x246, Right to E1 scrolls to B..E; grow 552x346 keeps B..G; shrink 280x246 follows to C..E; TENANT-GATE draws + screendump)\n'
```

## test-flair-i123-resize-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33467):
Prerequisites: `test-flair-i123-resize build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data_i123_fixedgeo.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_resize_mut,$(FLAIR_I123_RESIZE_GATE_IMG),$(FLAIR_DATA_I123_FIXEDGEO_IMG),FLAIR_I123_RESIZE,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_resize_mut.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && \
 grep -qE '$(call I123_RS_GROW,552,346,280,246)' $$f && \
 grep -qE '$(call I123_RS_G,321,64, {9},0,3)' $$f && grep -qE '$(call I123_RS_G,1,0,E1: {68},1,0)' $$f || \
 { printf '!!! test-flair-i123-resize-mutant FAIL: FIXED_GEOMETRY went RED for the wrong reason (want: 123 up, the grows done, E1 drawn on the 76-character face)\n'; exit 1; }
@for leg in 1 2 3 4; do \
 case $$leg in 1) $(call I123_RS1_OK,$(BUILD)/flair_i123_resize_mut.serial) ;; 2) $(call I123_RS2_OK,$(BUILD)/flair_i123_resize_mut.serial) ;; \
  3) $(call I123_RS3_OK,$(BUILD)/flair_i123_resize_mut.serial) ;; 4) $(call I123_RS4_OK,$(BUILD)/flair_i123_resize_mut.serial) ;; esac && \
 { printf '!!! test-flair-i123-resize-mutant FAIL: FIXED_GEOMETRY PASSED leg RS%s -- the QEMU leg is decoration\n' $$leg; exit 1; }; done; true
@if $(PPM_TEXT_CHECK_BIN) --i123-resize $(BUILD)/flair_i123_resize_mut.ppm; then \
 printf '!!! test-flair-i123-resize-mutant FAIL: FIXED_GEOMETRY PASSED the screendump check\n'; exit 1; fi
@printf '>>> test-flair-i123-resize-mutant: I123_MUT_FIXED_GEOMETRY correctly RED -- E1 drawn at x=321 on the 76-character face, never reflowed (RS1..RS4 and the screendump all RED)\n'
@printf '>>> test-flair-i123-resize-mutant: green\n'
```

## test-flair-i123-wide

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33514):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_wide,$(FLAIR_I123_WIDE_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_I123_WIDE,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_wide.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-wide FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/flair_i123_wide.serial || { printf '!!! test-flair-i123-wide FAIL: input ring overrun\n'; exit 1; }
@$(call I123_WD1_OK,$(BUILD)/flair_i123_wide.serial) || { printf '!!! test-flair-i123-wide FAIL: WD1 (L008) width 240 -- the border A at char 39 and the 72-wide pointer with 1 at its right were not drawn\n'; exit 1; }
@$(call I123_WD2_OK,$(BUILD)/flair_i123_wide.serial) || { printf '!!! test-flair-i123-wide FAIL: WD2 Right -- border B..I, "B1:", the pointer on B\n'; exit 1; }
@$(call I123_WD3_OK,$(BUILD)/flair_i123_wide.serial) || { printf '!!! test-flair-i123-wide FAIL: WD3 Left -- the clipped A and its 72-wide pointer were not drawn again\n'; exit 1; }
@$(call I123_WD4_OK,$(BUILD)/flair_i123_wide.serial) || { printf '!!! test-flair-i123-wide FAIL: WD4 the click inside the clipped A -- "A2:" and the 72-wide pointer on row 2\n'; exit 1; }
$(PPM_TEXT_CHECK_BIN) --i123-wide $(BUILD)/flair_i123_wide.ppm
@printf '>>> test-flair-i123-wide: green (QEMU: A set to 240 shows clipped to the 72-character area as the real R2.2 does -- A centred, the 72-wide pointer, 1 right-aligned; Right B..I; Left back; a click at area char 60 selects A2; TENANT-GATE draws + screendump)\n'
```

## test-flair-i123-wide-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 33532):
Prerequisites: `test-flair-i123-wide build/qemu_harness build/flair_tenants_mut_i123_keys_gate.img build/flair_data_i123_widehidden.img build/ppm_text_check`
```make
$(call appl-boot,flair_i123_wide_mut,$(FLAIR_I123_WIDE_GATE_IMG),$(FLAIR_DATA_I123_WIDEHIDDEN_IMG),FLAIR_I123_WIDE,--screendump --screendump-after FLAIR-LIVE-OK)
@f=$(BUILD)/flair_i123_wide_mut.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f && \
 grep -qE '$(call I123_RS_G,1,0,B1: {68},1,0)' $$f || \
 { printf '!!! test-flair-i123-wide-mutant FAIL: WIDE_HIDDEN went RED for the wrong reason (want: 123 up, Right reached B1)\n'; exit 1; }
@for leg in 1 3 4; do \
 case $$leg in 1) $(call I123_WD1_OK,$(BUILD)/flair_i123_wide_mut.serial) ;; 3) $(call I123_WD3_OK,$(BUILD)/flair_i123_wide_mut.serial) ;; \
  4) $(call I123_WD4_OK,$(BUILD)/flair_i123_wide_mut.serial) ;; esac && \
 { printf '!!! test-flair-i123-wide-mutant FAIL: WIDE_HIDDEN PASSED leg WD%s -- the QEMU leg is decoration\n' $$leg; exit 1; }; done; true
@if $(PPM_TEXT_CHECK_BIN) --i123-wide $(BUILD)/flair_i123_wide_mut.ppm; then \
 printf '!!! test-flair-i123-wide-mutant FAIL: WIDE_HIDDEN PASSED the screendump check\n'; exit 1; fi
@printf '>>> test-flair-i123-wide-mutant: I123_MUT_WIDE_HIDDEN correctly RED -- B1 reached, but the 240-wide A never drawn (WD1, WD3, WD4 and the screendump all RED)\n'
@printf '>>> test-flair-i123-wide-mutant: green\n'
```

## test-i123-cells

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33190):
Prerequisites: `build/test_i123_cells build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-cells-fs
@$< $(BUILD)/i123-cells-fs
@printf '>>> test-i123-cells: green (L010 erase frees a full store slot, A2049 enters; Fixed-2/unprotected erases stay BLANKs; L032 equal/shorter/grow-after-reclaim replacements, genuine overflow keeps the old bytes)\n'
```

## test-i123-cells-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33194):
Prerequisites: `test-i123-cells build/test_i123_cells_mutant_I123_MUT_DEFAULT_BLANK build/test_i123_cells_mutant_I123_MUT_ALLOC_BEFORE_RECLAIM build/test_i123_cells_mutant_I123_MUT_ABSENT_UNPROTECTED build/.makefile.stamp`
```make
@for p in $(I123_CELLS_MUTANTS); do m=$${p%%:*}; id=$${p##*:}; log=$(BUILD)/test_i123_cells_mutant_$$m.log; \
 set +e; $(BUILD)/test_i123_cells_mutant_$$m $(BUILD)/i123-cells-fs > $$log 2>&1; rc=$$?; set -e; \
 [ $$rc -eq 1 ] && grep -q '^i123-cells: .* checks,' $$log && grep -q "FAIL $$id " $$log || \
 { cat $$log; printf '!!! test-i123-cells-mutant: %s wrong RED (exit %s, want FAIL %s)\n' "$$m" "$$rc" "$$id"; exit 1; }; \
 printf '>>> test-i123-cells-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "FAIL $$id " $$log | sed 's/^ *//')"; done
@printf '>>> test-i123-cells-mutant: green (all %s mutants caught)\n' "$(words $(I123_CELLS_MUTANTS))"
```

## test-i123-clock

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33078):
Prerequisites: `build/test_i123_clock build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-clock-fs
@$< $(BUILD)/i123-clock-fs
```

## test-i123-clock-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33087):
Prerequisites: `test-i123-clock build/test_i123_clock_mutant_I123_MUT_CLOCK_MISSING build/test_i123_clock_mutant_I123_MUT_CLOCK_FROZEN build/.makefile.stamp`
```make
@for pair in I123_MUT_CLOCK_MISSING:N1 I123_MUT_CLOCK_FROZEN:N2; do \
 m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_i123_clock_mutant_$$m; \
 if $$bin $(BUILD)/i123-clock-fs > $$bin.log 2>&1; then printf '!!! %s survived\n' $$m; exit 1; fi; \
 grep -q '^i123-clock: .* checks,' $$bin.log && grep -q "FAIL $$why NOW cache" $$bin.log || { cat $$bin.log; exit 1; }; \
 printf '>>> test-i123-clock-mutant: %s correctly RED (%s)\n' $$m $$why; done
```

## test-i123-copy-move

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32979):
Prerequisites: `test-i123-copy-move-model harness/diff/wk1_diff/wk1_formula_ref.py test-i123-copy-tile test-i123-copy-move-baseline build/.makefile.stamp`
```make
@$(BUILD)/test_i123_copy_move --corpus $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF)
@printf '>>> test-i123-copy-move: green (8 real 1-2-3 R2.2 /Copy and /Move before/after pairs, independent readers)\n'
```

## test-i123-copy-move-baseline

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33610):
Prerequisites: `build/test_i123_transfer_baseline harness/diff/wk1_diff/wk1_formula_ref.py build/.makefile.stamp`
```make
@$< $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF)
@printf '>>> test-i123-copy-move-baseline: green (12 real-sheet self-transfers, independent FORMULA bodies)\n'
```

## test-i123-copy-move-corpus-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33012):
Prerequisites: `test-i123-copy-move build/test_i123_copy_move_mutant_I123_MUT_ERR_WORD build/test_i123_copy_move_mutant_I123_MUT_MOVE_COL14 build/test_i123_copy_move_mutant_I123_MUT_ERR_LOAD build/test_i123_copy_move_mutant_I123_MUT_ROW_ERR build/test_i123_copy_move_mutant_I123_MUT_COPY_ABSOLUTE build/.makefile.stamp`
```make
@for pair in $(I123_MOVE_CORPUS_MUTANTS); do \
	m=$${pair%%:*}; g=$${pair#*:}; b=$(BUILD)/test_i123_copy_move_mutant_$$m; \
	mkdir -p $(BUILD)/i123-cm-mut-$$m; rc=0; \
	$$b --corpus $(LOTUS123_DECOMP) $(BUILD)/i123-cm-mut-$$m $(I123_FORMULA_REF) > $$b.corpus.log 2>&1 || rc=$$?; \
	if [ $$rc -ne 1 ]; then cat $$b.corpus.log; printf '!!! %s corpus exit %s, want 1 (RED)\n' "$$m" "$$rc"; exit 1; fi; \
	grep -q "RED-REASON corpus $$g FORMULA bodies" $$b.corpus.log && grep -q "^corpus $$g .*: RED$$" $$b.corpus.log || { cat $$b.corpus.log; exit 1; }; \
	printf '>>> test-i123-copy-move-corpus-mutant: %s correctly RED (%s FORMULA bodies vs real 1-2-3)\n' "$$m" "$$g"; \
done
@printf '>>> test-i123-copy-move-corpus-mutant: green (ERR corner word, 8-bit /Move column, loaded ERR reference, row wrap, /Copy byte identity each RED against the real goldens)\n'
```

## test-i123-copy-move-model

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32976):
Prerequisites: `build/test_i123_copy_move build/.makefile.stamp`
```make
@$<
@printf '>>> test-i123-copy-move-model: green (4000 random sheets, reference model corrected to the real R2.2 bytes)\n'
```

## test-i123-copy-move-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32993):
Prerequisites: `test-i123-copy-move-model build/test_i123_copy_move_mutant_I123_MUT_COPY_ABSOLUTE build/test_i123_copy_move_mutant_I123_MUT_MOVE_STALE build/test_i123_copy_move_mutant_I123_MUT_MOVE_COL14 build/test_i123_copy_move_mutant_I123_MUT_ERR_WORD build/.makefile.stamp`
```make
@for pair in $(I123_MOVE_MODEL_MUTANTS); do \
	m=$${pair%%:*}; why=$${pair#*:}; b=$(BUILD)/test_i123_copy_move_mutant_$$m; \
	if $$b > $$b.log 2>&1; then printf '!!! %s PASSED\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$b.log && grep -q "RED-REASON $$why" $$b.log || { cat $$b.log; exit 1; }; \
	printf '>>> test-i123-copy-move-mutant: %s correctly RED (%s)\n' "$$m" "$$why"; \
done
@printf '>>> test-i123-copy-move-mutant: green (absolute adjustment, stale referrers, 14-bit /Move column, non-0xFFFF ERR corner correctly RED)\n'
```

## test-i123-copy-tile

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32984):
Prerequisites: `build/test_i123_copy_move fixtures/i123-copy/BLOCK.WK1 fixtures/i123-copy/OVER.WK1 fixtures/i123-copy/ROW.WK1 fixtures/i123-copy/SINGLE.WK1 build/.makefile.stamp`
```make
@LOTUS123_DECOMP=$(LOTUS123_DECOMP) $< --tile fixtures/i123-copy $(BUILD) $(I123_FORMULA_REF)
```

## test-i123-copy-tile-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32986):
Prerequisites: `test-i123-copy-tile build/test_i123_copy_move_mutant_I123_MUT_COPY_RECT_REFUSED build/.makefile.stamp`
```make
@rc=0; LOTUS123_DECOMP=$(LOTUS123_DECOMP) $(BUILD)/test_i123_copy_move_mutant_I123_MUT_COPY_RECT_REFUSED --tile fixtures/i123-copy $(BUILD) $(I123_FORMULA_REF) > $(BUILD)/i123-copy-tile-mutant.log 2>&1 || rc=$$?; \
[ $$rc -eq 1 ] && grep -q 'RED-REASON tile SINGLE refused' $(BUILD)/i123-copy-tile-mutant.log
@printf '>>> test-i123-copy-tile-mutant: green (destination rectangle refusal caught)\n'
```

## test-i123-entry

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33059):
Prerequisites: `build/test_i123_entry build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-entry)
@mkdir -p $(BUILD)/i123-entry-fs
@$< $(BUILD)/i123-entry-fs
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(BUILD)/i123-entry-fs/CURRENCY.WK1 > $(BUILD)/i123-entry-fs/CURRENCY.ref
@grep -q 'NUMBER.*Currency2 c0 r0 num=12.34' $(BUILD)/i123-entry-fs/CURRENCY.ref && grep -q 'FORMULA.*c1 r0 cached=12.34' $(BUILD)/i123-entry-fs/CURRENCY.ref && grep -q 'consumed=.* OK$$' $(BUILD)/i123-entry-fs/CURRENCY.ref
@printf '>>> test-i123-entry: green (75/104/240/241, tail viewport, beep/refusal, Backspace, commit, VALUE/EDIT)\n'
```

## test-i123-entry-currency-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33093):
Prerequisites: `test-i123-entry build/test_i123_entry_currency_mutant build/.makefile.stamp`
```make
@if $(BUILD)/test_i123_entry_currency_mutant $(BUILD)/i123-entry-fs > $(BUILD)/test_i123_entry_currency_mutant.log 2>&1; then printf '!!! currency mutant survived\n'; exit 1; fi
@grep -q '^i123-entry: .* checks,' $(BUILD)/test_i123_entry_currency_mutant.log && grep -q 'FAIL E10 leading dollar' $(BUILD)/test_i123_entry_currency_mutant.log
```

## test-i123-entry-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33066):
Prerequisites: `test-i123-entry build/test_i123_entry_mutant build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_i123_entry_mutant $(BUILD)/i123-entry-fs > $(BUILD)/test_i123_entry_mutant.log 2>&1; rc=$$?; set -e; \
[ $$rc -eq 1 ] && grep -q '^i123-entry: .* checks,' $(BUILD)/test_i123_entry_mutant.log && \
grep -q 'FAIL E1 accept through 240' $(BUILD)/test_i123_entry_mutant.log || { cat $(BUILD)/test_i123_entry_mutant.log; printf '!!! test-i123-entry-mutant: wrong RED (exit %s)\n' "$$rc"; exit 1; }; \
printf '>>> test-i123-entry-mutant: I123_MUT_ENTRY_75 correctly RED (%s)\n' "$$(grep -m1 'FAIL E1 ' $(BUILD)/test_i123_entry_mutant.log)"
@printf '>>> test-i123-entry-mutant: green (old 75-character cap caught)\n'
```

## test-i123-formula

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15198):
Prerequisites: `build/test_i123_formula build/test_i123_formula_x87_037F build/test_i123_formula_x87_027F harness/diff/wk1_diff/wk1_formula_ref.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-formula)
@printf ">>> test-i123-formula: the formula engine vs 1090 real 1-2-3 R2.2 formulas, their typed text and cached values, via wk1_ref.py (initech-94ah)\n"
@$(TEST_I123_FORMULA) $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF)
@printf ">>> test-i123-formula: x87 build, control word 0x037F (64-bit precision: the tenant's, = the kernel boot word)\n"
@$(BUILD)/test_i123_formula_x87_037F $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF) > $(BUILD)/test_i123_formula_x87_037F.log 2>&1 || { cat $(BUILD)/test_i123_formula_x87_037F.log; printf '!!! test-i123-formula FAIL: the x87 64-bit build (the tenant numerics) is RED\n'; exit 1; }
@grep -E '^  part [A-F]:|checks,' $(BUILD)/test_i123_formula_x87_037F.log
@if $(BUILD)/test_i123_formula_x87_027F $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF) > $(BUILD)/test_i123_formula_x87_027F.log 2>&1; then \
	printf '!!! test-i123-formula FAIL: the 53-bit x87 build is green -- the recorded control-word ruling (0x037F) rests on its being RED; re-examine\n'; exit 1; \
else \
	printf '>>> test-i123-formula: x87 build, control word 0x027F (53-bit precision) is RED as recorded (%s): the literal parser needs the 64-bit significand; the 1090 cached values do not discriminate (%s)\n' "$$(grep -c 'FAIL [AB]:' $(BUILD)/test_i123_formula_x87_027F.log) typed/round-trip failures, 1E99" "$$(grep -E '^  part C:' $(BUILD)/test_i123_formula_x87_027F.log | sed 's/^  part C: //; s/ (.*//')"; \
fi
@printf ">>> test-i123-formula: green\n"
```

## test-i123-formula-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15212):
Prerequisites: `build/test_i123_formula_mutant_I123_MUT_PREC_FLAT build/test_i123_formula_mutant_I123_MUT_ROUND_EVEN build/test_i123_formula_mutant_I123_MUT_CACHE_ROUND harness/diff/wk1_diff/wk1_formula_ref.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-formula-mutant)
@for pair in 'I123_MUT_PREC_FLAT:RED-REASON round-trip' 'I123_MUT_ROUND_EVEN:RED-REASON values' 'I123_MUT_CACHE_ROUND:RED-REASON recalc-file'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_i123_formula_mutant_$$m; \
	if $$bin $(LOTUS123_DECOMP) $(BUILD) $(I123_FORMULA_REF) > $$bin.log 2>&1; then printf '!!! test-i123-formula-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-i123-formula-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-i123-formula-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-i123-formula-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -F "$$why" $$bin.log)"; \
done
@printf '>>> test-i123-formula-mutant: green (all three mutants RED for the named reason)\n'
```

## test-i123-hit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33136):
Prerequisites: `build/test_i123_hit build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-hit-fs
@$< $(BUILD)/i123-hit-fs
@printf '>>> test-i123-hit: green (first/last visible cell, borders/panel/status ignored, partial edge, after Tab and PgDn, unequal widths, entry completed first)\n'
```

## test-i123-hit-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33140):
Prerequisites: `test-i123-hit build/test_i123_hit_mutant_I123_MUT_HIT_IGNORES_SCROLL build/.makefile.stamp`
```make
@for p in $(I123_HIT_MUTANTS); do m=$${p%%:*}; id=$${p##*:}; log=$(BUILD)/test_i123_hit_mutant_$$m.log; \
 set +e; $(BUILD)/test_i123_hit_mutant_$$m $(BUILD)/i123-hit-fs > $$log 2>&1; rc=$$?; set -e; \
 [ $$rc -eq 1 ] && grep -q '^i123-hit: .* checks,' $$log && grep -q "FAIL $$id " $$log || \
 { cat $$log; printf '!!! test-i123-hit-mutant: %s wrong RED (exit %s, want FAIL %s)\n' "$$m" "$$rc" "$$id"; exit 1; }; \
 printf '>>> test-i123-hit-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "FAIL $$id " $$log | sed 's/^ *//')"; done
@printf '>>> test-i123-hit-mutant: green (all %s mutants caught)\n' "$(words $(I123_HIT_MUTANTS))"
```

## test-i123-keys

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33110):
Prerequisites: `build/test_i123_keys build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-keys-fs
@$< $(BUILD)/i123-keys-fs
@printf '>>> test-i123-keys: green (F2 EDIT/Esc/Enter/240, F5 GOTO prompt/top-left/ERROR, PgDn/PgUp one screen, Tab/Shift-Tab one screen; real-R2.2 captures)\n'
```

## test-i123-keys-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33114):
Prerequisites: `test-i123-keys build/test_i123_keys_mutant_I123_MUT_TAB_ONE_CELL build/test_i123_keys_mutant_I123_MUT_GOTO_FOLLOW build/test_i123_keys_mutant_I123_MUT_EDIT_ESC_CANCELS build/.makefile.stamp`
```make
@for p in $(I123_KEYS_MUTANTS); do m=$${p%%:*}; id=$${p##*:}; log=$(BUILD)/test_i123_keys_mutant_$$m.log; \
 set +e; $(BUILD)/test_i123_keys_mutant_$$m $(BUILD)/i123-keys-fs > $$log 2>&1; rc=$$?; set -e; \
 [ $$rc -eq 1 ] && grep -q '^i123-keys: .* checks,' $$log && grep -q "FAIL $$id " $$log || \
 { cat $$log; printf '!!! test-i123-keys-mutant: %s wrong RED (exit %s, want FAIL %s)\n' "$$m" "$$rc" "$$id"; exit 1; }; \
 printf '>>> test-i123-keys-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "FAIL $$id " $$log | sed 's/^ *//')"; done
@printf '>>> test-i123-keys-mutant: green (all %s mutants caught)\n' "$(words $(I123_KEYS_MUTANTS))"
```

## test-i123-menu-tree

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32953):
Prerequisites: `build/test_i123_menu_tree build/i123-menu.mnu build/.makefile.stamp`
```make
@$< $(I123_RI) spec/i123/menu_ri_offsets.tsv > $(BUILD)/i123-menu-ri.txt
diff -u spec/i123/menu_tree.golden $(BUILD)/i123-menu-ri.txt
@$< > $(BUILD)/i123-menu-app.txt
diff -u $(BUILD)/i123-menu-ri.txt $(BUILD)/i123-menu-app.txt
@$(BUILD)/test_i123_menu_tree --load $(BUILD)/i123-menu.mnu > $(BUILD)/i123-menu-loaded.txt
diff -u $(BUILD)/i123-menu-ri.txt $(BUILD)/i123-menu-loaded.txt
@printf '>>> test-i123-menu-tree: green (82 levels, 381 items; independent 123.RI bytes and child links)\n'
```

## test-i123-menu-tree-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32961):
Prerequisites: `test-i123-menu-tree build/test_i123_menu_tree_mutant build/.makefile.stamp`
```make
@$(BUILD)/test_i123_menu_tree_mutant > $(BUILD)/i123-menu-mutant.txt || test $$? = 1
@if cmp -s $(BUILD)/i123-menu-ri.txt $(BUILD)/i123-menu-mutant.txt; then printf '!!! menu spelling mutant PASSED\n'; exit 1; fi
@grep -q '^/.*Vorksheet' $(BUILD)/i123-menu-mutant.txt
@$(BUILD)/test_i123_menu_tree --resource-mutant > $(BUILD)/i123-menu-bad.mnu
@if $(BUILD)/test_i123_menu_tree --load $(BUILD)/i123-menu-bad.mnu > $(BUILD)/i123-menu-bad.log 2>&1; then printf '!!! corrupt menu resource PASSED\n'; exit 1; fi
@grep -qx 'menu resource refused' $(BUILD)/i123-menu-bad.log
@printf '>>> test-i123-menu-tree-mutant: green (Worksheet misspelled Vorksheet correctly RED)\n'
```

## test-i123-quit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15286):
Prerequisites: `build/test_i123_quit build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-quit)
@printf '>>> test-i123-quit: the 123.EXE exit guard over a recording Toolbox mock (initech-fvnj, audit L008)\n'
@rm -rf $(I123_QUIT_FS) && mkdir -p $(I123_QUIT_FS)
@$(TEST_I123_QUIT) $(I123_QUIT_FS)
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(I123_QUIT_FS)/Q.WK1 > $(BUILD)/i123_quit_q.ref
@$(call I123_DISK_CELLS,$(BUILD)/i123_quit_q.ref) || { printf '!!! test-i123-quit FAIL: Q.WK1 (Save from the alert) does not hold the typed row per wk1_ref.py\n'; cat $(BUILD)/i123_quit_q.ref; exit 1; }
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(I123_QUIT_FS)/PE.WK1 > $(BUILD)/i123_quit_pe.ref
@grep -q "LABEL.*c0 r0 label=.*Pend" $(BUILD)/i123_quit_pe.ref && grep -q 'consumed=.* OK$$' $(BUILD)/i123_quit_pe.ref || { printf '!!! test-i123-quit FAIL: PE.WK1 does not hold the committed pending entry per wk1_ref.py\n'; cat $(BUILD)/i123_quit_pe.ref; exit 1; }
@python3 $(LOTUS123_DECOMP)/tools/wk1_ref.py $(I123_QUIT_FS)/X.WK1 > $(BUILD)/i123_quit_x.ref
@grep -q "LABEL.*c0 r0 label=.*Again" $(BUILD)/i123_quit_x.ref && grep -q 'consumed=.* OK$$' $(BUILD)/i123_quit_x.ref || { printf '!!! test-i123-quit FAIL: X.WK1 (Save + Replace from the alert) does not hold Again per wk1_ref.py\n'; cat $(BUILD)/i123_quit_x.ref; exit 1; }
@[ ! -e $(I123_QUIT_FS)/Z.WK1 ] && [ ! -e $(I123_QUIT_FS)/W.WK1 ] || { printf '!!! test-i123-quit FAIL: a failed Save left a file behind\n'; exit 1; }
@printf '>>> test-i123-quit: green (model + recorded gate calls; Q.WK1 / PE.WK1 / X.WK1 judged by wk1_ref.py)\n'
```

## test-i123-quit-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15300):
Prerequisites: `build/test_i123_quit_mutant_I123_MUT_NEVER_DIRTY build/test_i123_quit_mutant_I123_MUT_CANCEL_QUITS build/test_i123_quit_mutant_I123_MUT_PENDING_IGNORED build/test_i123_quit_mutant_I123_MUT_SAVE_FAIL_QUITS build/.makefile.stamp`
```make
@for pair in I123_MUT_NEVER_DIRTY:Q2 I123_MUT_CANCEL_QUITS:Q3 I123_MUT_PENDING_IGNORED:Q5 I123_MUT_SAVE_FAIL_QUITS:Q8; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_i123_quit_mutant_$$m; dir=$(BUILD)/i123_quit_fs_$$m; \
	rm -rf $$dir && mkdir -p $$dir; \
	if $$bin $$dir > $$bin.log 2>&1; then printf '!!! test-i123-quit-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; cat $$bin.log; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-i123-quit-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -q "FAIL $$why " $$bin.log || { printf '!!! test-i123-quit-mutant FAIL: %s went RED for the wrong reason (want %s)\n' "$$m" "$$why"; cat $$bin.log; exit 1; }; \
	printf '>>> test-i123-quit-mutant: %s correctly RED (%s: %s)\n' "$$m" "$$why" "$$(grep -m1 "FAIL $$why " $$bin.log)"; \
done
@printf '>>> test-i123-quit-mutant: green (all four mutants RED for the named reason)\n'
```

## test-i123-range-menu

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33039):
Prerequisites: `build/test_i123_range_menu build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-range-menu-fs
@$< $(BUILD)/i123-range-menu-fs
@printf '>>> test-i123-range-menu: green (Copy, Move, names, formats, protection, Value, Trans, Esc/Break)\n'
```

## test-i123-range-menu-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33046):
Prerequisites: `test-i123-range-menu build/test_i123_range_menu_mutant build/.makefile.stamp`
```make
@rc=0; $(BUILD)/test_i123_range_menu_mutant $(BUILD)/i123-range-menu-fs > $(BUILD)/i123-point-mutant.log 2>&1 || rc=$$?; \
[ $$rc -eq 1 ] && grep -q 'FAIL P3 period and arrows extend POINT range' $(BUILD)/i123-point-mutant.log
@printf '>>> test-i123-range-menu-mutant: green (period-as-text caught)\n'
```

## test-i123-recalc-order

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15243):
Prerequisites: `build/test_i123_recalc_order harness/diff/wk1_diff/wk1_formula_ref.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-recalc-order)
@printf ">>> test-i123-recalc-order: natural/columnwise/rowwise, iteration, manual, CIRC vs the R2.2 manual and the minted MINT01/CIRC01/MINT04 typing (initech-94ah)\n"
@$(TEST_I123_RECALC) $(LOTUS123_DECOMP) $(I123_FORMULA_REF)
@printf ">>> test-i123-recalc-order: green\n"
```

## test-i123-recalc-order-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15249):
Prerequisites: `build/test_i123_recalc_mutant_I123_MUT_NO_PRECEDENTS build/test_i123_recalc_mutant_I123_MUT_CIRC_BLIND harness/diff/wk1_diff/wk1_formula_ref.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-recalc-order-mutant)
@for pair in 'I123_MUT_NO_PRECEDENTS:RED-REASON order' 'I123_MUT_CIRC_BLIND:RED-REASON circ'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_i123_recalc_mutant_$$m; \
	if $$bin $(LOTUS123_DECOMP) $(I123_FORMULA_REF) > $$bin.log 2>&1; then printf '!!! test-i123-recalc-order-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-i123-recalc-order-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-i123-recalc-order-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-i123-recalc-order-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -F "$$why" $$bin.log)"; \
done
@printf '>>> test-i123-recalc-order-mutant: green (both mutants RED for the named reason)\n'
```

## test-i123-resize

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33163):
Prerequisites: `build/test_i123_resize build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/i123-resize-fs
@$< $(BUILD)/i123-resize-fs
@printf '>>> test-i123-resize: green (640x440 / 352x246 / 440x340 / 200x150 / 96x64 / collapsed content: reflowed border, rows, READY + status at the edges; L018 Right to E1 follows; shrink/grow keep the pointer visible; click, Tab, PgDn, GOTO by the window now; bars never faked; B06 a 240-wide A clipped to the area as the real R2.2 shows it)\n'
```

## test-i123-resize-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33167):
Prerequisites: `test-i123-resize build/test_i123_resize_mutant_I123_MUT_FIXED_GEOMETRY build/test_i123_resize_mutant_I123_MUT_WIDE_HIDDEN build/.makefile.stamp`
```make
@for p in $(I123_RESIZE_MUTANTS); do m=$${p%%:*}; id=$${p##*:}; log=$(BUILD)/test_i123_resize_mutant_$$m.log; \
 set +e; $(BUILD)/test_i123_resize_mutant_$$m $(BUILD)/i123-resize-fs > $$log 2>&1; rc=$$?; set -e; \
 [ $$rc -eq 1 ] && grep -q '^i123-resize: .* checks,' $$log && grep -q "FAIL $$id " $$log || \
 { cat $$log; printf '!!! test-i123-resize-mutant: %s wrong RED (exit %s, want FAIL %s)\n' "$$m" "$$rc" "$$id"; exit 1; }; \
 printf '>>> test-i123-resize-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "FAIL $$id " $$log | sed 's/^ *//')"; done
@printf '>>> test-i123-resize-mutant: green (all %s mutants caught)\n' "$(words $(I123_RESIZE_MUTANTS))"
```

## test-i123-wk1-roundtrip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15146):
Prerequisites: `build/test_i123_wk1_roundtrip harness/diff/wk1_diff/wk1_canon.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-wk1-roundtrip)
@printf ">>> test-i123-wk1-roundtrip: the 1-2-3 R2.2 .WK1 codec vs 55 real goldens, wk1_ref.py and the minted screenshots (initech-9u8w)\n"
@$(TEST_I123_WK1) $(LOTUS123_DECOMP) $(BUILD) $(I123_WK1_CANON)
@printf ">>> test-i123-wk1-roundtrip: green\n"
```

## test-i123-wk1-roundtrip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15152):
Prerequisites: `build/test_i123_wk1_mutant_I123_MUT_LABEL_OPCODE build/test_i123_wk1_mutant_I123_MUT_GENERAL_ROUNDS build/test_i123_wk1_mutant_I123_MUT_WIDE_HIDDEN harness/diff/wk1_diff/wk1_canon.py build/.makefile.stamp`
```make
$(call need_lotus_goldens,test-i123-wk1-roundtrip-mutant)
@for pair in 'I123_MUT_LABEL_OPCODE:RED-REASON golden-rewrite' 'I123_MUT_GENERAL_ROUNDS:RED-REASON display' 'I123_MUT_WIDE_HIDDEN:RED-REASON display'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_i123_wk1_mutant_$$m; \
	if $$bin $(LOTUS123_DECOMP) $(BUILD) $(I123_WK1_CANON) > $$bin.log 2>&1; then printf '!!! test-i123-wk1-roundtrip-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-i123-wk1-roundtrip-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-i123-wk1-roundtrip-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-i123-wk1-roundtrip-mutant: %s correctly RED (%s: %s)\n' "$$m" "$$why" "$$(grep -F "$$why" $$bin.log)"; \
done
@printf '>>> test-i123-wk1-roundtrip-mutant: green (all three mutants RED for the named reason)\n'
```

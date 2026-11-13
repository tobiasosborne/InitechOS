## test-flair-notepad

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 18):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_notepad_check build/da_store_check build/.makefile.stamp`
```make
$(call appl-boot,flair_notepad,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_NOTEPAD_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call DA_UP,$(BUILD)/flair_notepad.serial,NOTEPAD)
@grep -qx 'TENANT-SCRAP put n=5 count=1' $(BUILD)/flair_notepad.serial && grep -qx 'TENANT-SCRAP get n=5' $(BUILD)/flair_notepad.serial
@$(call DA_NOTE_DISK,flair_notepad)
@grep -q 's="2 of 8"' $(BUILD)/flair_notepad.serial && grep -q 's="alpha2"' $(BUILD)/flair_notepad.serial
@$(call DA_PIXEL_CHECK,pad,flair_notepad)
@fsck.fat -n $(BUILD)/flair_notepad_data.img
@printf '>>> test-flair-notepad: green (Apple launch; dog-ear; TEXT Scrap; turn/close/reopen; independent FAT + PNG)\n'
```

## test-flair-notepad-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 207):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-notepad-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-notepad-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-notepad_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-notepad_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-notepad_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-notepad_bochs.report.txt" || true
@cat "$(BUILD)/flair-notepad_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-notepad-bochs "$(BUILD)/flair-notepad_bochs.report.txt" "$(BUILD)/flair-notepad_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-notepad-corrupt

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 96):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/da_corrupt_source.img build/ppm_flair_notepad_check build/.makefile.stamp`
```make
$(call appl-boot,flair_note_corrupt,$(FLAIR_ITEXT_GATE_IMG),$(DA_CORRUPT_DATA),FLAIR_NOTEPAD_OPEN_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call DA_UP,$(BUILD)/flair_note_corrupt.serial,NOTEPAD) && grep -q '^DA-CORRUPT NOTEPAD.DAT empty pages' $(BUILD)/flair_note_corrupt.serial
@grep -q 's="1 of 8"' $(BUILD)/flair_note_corrupt.serial
@$(call DA_PIXEL_CHECK,pad,flair_note_corrupt)
@printf '>>> test-flair-notepad-corrupt: green (malformed FAT file -> empty pages + DA-CORRUPT serial line)\n'
```

## test-flair-notepad-corrupt-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 208):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/da_corrupt_source.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-notepad-corrupt-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-notepad-corrupt-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/da_corrupt_source.img $(BUILD)/flair-notepad-corrupt_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-notepad-corrupt_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-notepad-corrupt_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-notepad-corrupt_bochs.report.txt" || true
@cat "$(BUILD)/flair-notepad-corrupt_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-notepad-corrupt-bochs "$(BUILD)/flair-notepad-corrupt_bochs.report.txt" "$(BUILD)/flair-notepad-corrupt_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-notepad-keep

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 131):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/da_badsum_source.img build/.makefile.stamp`
```make
$(call appl-boot,flair_note_keep,$(FLAIR_ITEXT_GATE_IMG),$(DA_BADSUM_DATA),FLAIR_NOTEPAD_KEEP_SPEC,)
@$(call DA_KEEP_UP,$(BUILD)/flair_note_keep.serial) || { printf '!!! test-flair-notepad-keep FAIL: UP -- Note Pad up, the store refused (DA-CORRUPT), closed (EXIT 0)\n'; grep -E '^(DA-|TENANT-(REG|EXIT|ALERT|CRASH)|PANIC)' $(BUILD)/flair_note_keep.serial; exit 1; }
@$(call DA_KEEP_KEPT,$(BUILD)/flair_note_keep.serial,$(BUILD)/flair_note_keep_data.img) || { printf '!!! test-flair-notepad-keep FAIL: KEPT -- the damaged store was written over / no warning\n'; grep -E '^(DA-|TENANT-ALERT)' $(BUILD)/flair_note_keep.serial; exit 1; }
@fsck.fat -n $(BUILD)/flair_note_keep_data.img > /dev/null
@printf '>>> test-flair-notepad-keep: green (bad-checksum store: empty pages, Close warns "damaged", NOTEPAD.DAT byte-identical (mtools); fsck clean)\n'
```

## test-flair-notepad-keep-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 137):
Prerequisites: `test-flair-notepad-keep build/da_badsum_mut_source.img build/.makefile.stamp`
```make
$(call appl-boot,flair_note_keep_mut,$(FLAIR_ITEXT_GATE_IMG),$(DA_BADSUM_MUT_DATA),FLAIR_NOTEPAD_KEEP_SPEC,)
@$(call DA_KEEP_UP,$(BUILD)/flair_note_keep_mut.serial) || { printf '!!! test-flair-notepad-keep-mutant FAIL: OVERWRITE_DAMAGED went RED for the wrong reason (want: Note Pad up, DA-CORRUPT, EXIT 0)\n'; exit 1; }
@if $(call DA_KEEP_KEPT,$(BUILD)/flair_note_keep_mut.serial,$(BUILD)/flair_note_keep_mut_data.img); then printf '!!! test-flair-notepad-keep-mutant FAIL: OVERWRITE_DAMAGED kept the store -- the leg is decoration\n'; exit 1; fi
@mcopy -o -i $(BUILD)/flair_note_keep_mut_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_keep_mut.dat && ! cmp -s $(DA_BADSUM_DAT) $(BUILD)/flair_note_keep_mut.dat || { printf '!!! test-flair-notepad-keep-mutant FAIL: wrong RED reason (the store survived)\n'; exit 1; }
@printf '>>> test-flair-notepad-keep-mutant: DA_MUT_OVERWRITE_DAMAGED correctly RED (the bad-checksum store was written over)\n'
```

## test-flair-notepad-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 65):
Prerequisites: `test-flair-notepad build/flair_data_da_TURN.img build/flair_data_da_CLOSE.img build/flair_data_da_PASTE.img build/.makefile.stamp`
```make
$(call da-note-mut-leg,TURN,DA_MUT_TURN_OFFBYONE)
$(call da-note-mut-leg,CLOSE,DA_MUT_SKIP_CLOSE_SAVE)
$(call da-note-mut-leg,PASTE,DA_MUT_PASTE_WRONG_PAGE)
```

## test-flair-notepad-restart

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 77):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_notepad_check build/da_store_check build/.makefile.stamp`
```make
$(call pw-reboot,$(FLAIR_ITEXT_GATE_IMG),flair_note_restart,$(BUILD)/flair_note_restart_data.img,$(FLAIR_NOTEPAD_RESTART_SPEC))
@grep -qx 'FLAIR-RESTART' $(BUILD)/flair_note_restart.serial && [ "$$(grep -c '^FLAIR-LIVE-READY' $(BUILD)/flair_note_restart.serial)" = 2 ]
@mcopy -o -i $(BUILD)/flair_note_restart_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_restart.dat
@$(DA_DISK) pad $(BUILD)/flair_note_restart.dat 1 alpha beta
$(call appl-boot,flair_note_reopen,$(FLAIR_ITEXT_GATE_IMG),$(BUILD)/flair_note_restart_data.img,FLAIR_NOTEPAD_OPEN_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call DA_UP,$(BUILD)/flair_note_reopen.serial,NOTEPAD) && grep -q '^DA-LOADED NOTEPAD.DAT' $(BUILD)/flair_note_reopen.serial
@grep -q 's="beta"' $(BUILD)/flair_note_reopen.serial && grep -q 's="2 of 8"' $(BUILD)/flair_note_reopen.serial
@$(call DA_PIXEL_CHECK,pad,flair_note_reopen)
@fsck.fat -n $(BUILD)/flair_note_reopen_data.img
@printf '>>> test-flair-notepad-restart: green (Special Restart -> second LIVE-READY; independent disk alpha/beta; fresh-boot reopen beta page2)\n'
```

## test-flair-notepad-restart-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 207):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-notepad-restart-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-notepad-restart-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-notepad-restart_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-notepad-restart_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-notepad-restart_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-notepad-restart_bochs.report.txt" || true
@cat "$(BUILD)/flair-notepad-restart_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-notepad-restart-bochs "$(BUILD)/flair-notepad-restart_bochs.report.txt" "$(BUILD)/flair-notepad-restart_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-scrapbook

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 27):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_notepad_check build/da_store_check test-scrollbar-look build/.makefile.stamp`
```make
$(call appl-boot,flair_scrapbook,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_SCRAPBOOK_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call DA_UP,$(BUILD)/flair_scrapbook.serial,SCRAPBK)
@$(call DA_SB_DISK,flair_scrapbook)
@grep -q 's="1 of 2"' $(BUILD)/flair_scrapbook.serial && grep -q '^DA-LOADED SCRAPBK.DAT' $(BUILD)/flair_scrapbook.serial
@$(call DA_PIXEL_CHECK,scrapbook,flair_scrapbook)
@fsck.fat -n $(BUILD)/flair_scrapbook_data.img
@printf '>>> test-flair-scrapbook: green (TEXT-only append; page bar; Copy; close/reopen; independent FAT + PNG)\n'
```

## test-flair-scrapbook-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 207):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-scrapbook-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-scrapbook-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-scrapbook_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-scrapbook_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-scrapbook_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-scrapbook_bochs.report.txt" || true
@cat "$(BUILD)/flair-scrapbook_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-scrapbook-bochs "$(BUILD)/flair-scrapbook_bochs.report.txt" "$(BUILD)/flair-scrapbook_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-scrapbook-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 69):
Prerequisites: `test-flair-scrapbook build/flair_data_da_SBPASTE.img build/.makefile.stamp`
```make
$(call appl-boot,flair_sb_mut,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_DA_SBPASTE_IMG),FLAIR_SCRAPBOOK_SPEC,)
@$(call DA_UP,$(BUILD)/flair_sb_mut.serial,SCRAPBK)
@if $(call DA_SB_DISK,flair_sb_mut) > $(BUILD)/flair_sb_mut.red 2>&1; then printf 'FAIL DA_MUT_PASTE_WRONG_PAGE survived\n'; exit 1; fi
@grep -q '^RED-REASON disk:' $(BUILD)/flair_sb_mut.red
@printf '>>> test-flair-scrapbook-mutant: DA_MUT_PASTE_WRONG_PAGE correctly RED: '; cat $(BUILD)/flair_sb_mut.red
```

## test-notepad-route

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34524):
Prerequisites: `build/test_notepad_route build/.makefile.stamp`
```make
@$<
@printf '>>> test-notepad-route: green\n'
```

## test-notepad-route-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34538):
Prerequisites: `build/test_notepad_mut_DA_MUT_TURN_OFFBYONE build/test_notepad_mut_DA_MUT_SKIP_CLOSE_SAVE build/test_notepad_mut_DA_MUT_PASTE_WRONG_PAGE build/test_notepad_mut_DA_MUT_ACCEPT_CORRUPT build/test_notepad_mut_DA_MUT_OVERWRITE_DAMAGED build/.makefile.stamp`
```make
@for p in $(DA_HOST_MUTANTS); do m=$${p%%:*}; why=$${p#*:}; b=$(BUILD)/test_notepad_mut_$$m; \
 if $$b > $$b.log 2>&1; then printf 'FAIL %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks,' $$b.log && grep -q "^RED-REASON $$why:" $$b.log || { cat $$b.log; exit 1; }; \
 printf '>>> test-notepad-route-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^RED-REASON $$why:" $$b.log)"; done
```

## test-scrapbook-route

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34527):
Prerequisites: `build/test_scrapbook_route build/.makefile.stamp`
```make
@$<
@printf '>>> test-scrapbook-route: green\n'
```

## test-scrapbook-route-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34543):
Prerequisites: `build/test_scrapbook_mut_DA_MUT_PASTE_WRONG_PAGE build/test_scrapbook_mut_DA_MUT_SKIP_CLOSE_SAVE build/test_scrapbook_mut_DA_MUT_ACCEPT_CORRUPT build/.makefile.stamp`
```make
@for p in DA_MUT_PASTE_WRONG_PAGE:paste-page DA_MUT_SKIP_CLOSE_SAVE:save-close DA_MUT_ACCEPT_CORRUPT:corrupt; do \
 m=$${p%%:*}; why=$${p#*:}; b=$(BUILD)/test_scrapbook_mut_$$m; \
 if $$b > $$b.log 2>&1; then printf 'FAIL %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks,' $$b.log && grep -q "^RED-REASON $$why:" $$b.log || { cat $$b.log; exit 1; }; \
 printf '>>> test-scrapbook-route-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^RED-REASON $$why:" $$b.log)"; done
```

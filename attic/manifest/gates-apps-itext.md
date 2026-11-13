## test-flair-itext

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34124):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_itext_check build/itext_memo.expect build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-itext : InitechText edits, copies, pastes and saves through Standard File (initech-tdnl.16)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_itext,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_itext.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext.serial) || { printf '!!! test-flair-itext FAIL [1/5] UP: the scrap / TEXT did not come up, or the desktop died\n'; grep -E '^(PANIC|FLAIR-SCRAP|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED|FLAIR-DISPATCH)' $(BUILD)/flair_itext.serial; exit 1; }
@printf '>>> test-flair-itext [1/5] UP: %s; %s; registered, dispatched\n' "$$(grep -m1 '^FLAIR-SCRAP-READY' $(BUILD)/flair_itext.serial)" "$$(grep -m1 '^TENANT-LOAD name=TEXT' $(BUILD)/flair_itext.serial)"
@$(call ITEXT_EDIT,$(BUILD)/flair_itext.serial) || { printf '!!! test-flair-itext FAIL [2/5] EDIT: the tracked drag / the scrap put+get / the last paint are not as locked\n'; grep -E '^TENANT-(TRACK|SCRAP)' $(BUILD)/flair_itext.serial; grep '^TENANT-GATE ax=0x0030 win=1 ' $(BUILD)/flair_itext.serial | tail -6; exit 1; }
@printf '>>> test-flair-itext [2/5] EDIT: %s; Copy -> "%s"; paste -> "%s"; last paint Geneva 12: line 0 "tables" white on black, "Saving " / " to disk", "PC LOAD LETTER", "tables" on 16-px lines\n' "$$(grep -m1 '^TENANT-TRACK' $(BUILD)/flair_itext.serial)" "$$(grep -m1 '^TENANT-SCRAP put' $(BUILD)/flair_itext.serial)" "$$(grep -m1 '^TENANT-SCRAP get n=6' $(BUILD)/flair_itext.serial)"
@$(call ITEXT_SAVEAS,$(BUILD)/flair_itext.serial) || { printf '!!! test-flair-itext FAIL [3/5] SAVEAS: the Save As box / its reply are not as locked\n'; grep '^TENANT-SF' $(BUILD)/flair_itext.serial; exit 1; }
@printf '>>> test-flair-itext [3/5] SAVEAS: %s -> %s\n' "$$(grep -m1 '^TENANT-SF put up' $(BUILD)/flair_itext.serial)" "$$(grep -m1 '^TENANT-SF-REPLY' $(BUILD)/flair_itext.serial)"
@$(call ITEXT_DISK,$(BUILD)/flair_itext_data.img) || { printf '!!! test-flair-itext FAIL [4/5] DISK: MEMO.TXT is missing or is not the typed + pasted bytes\n'; xxd $(BUILD)/flair_itext_data.img.memo 2>/dev/null | head -5; exit 1; }
fsck.fat -n $(BUILD)/flair_itext_data.img
@printf '>>> test-flair-itext [4/5] DISK: MEMO.TXT (%s bytes) == "Saving tables to disk" CR "PC LOAD LETTER" CR "tables" (mtools, independent of the app); fsck.fat clean\n' "$$(wc -c < $(BUILD)/flair_itext_data.img.memo)"
@$(call ITEXT_PIXEL,$(BUILD)/flair_itext.ppm) > $(BUILD)/flair_itext.pixel 2>&1 || { cat $(BUILD)/flair_itext.pixel; printf '!!! test-flair-itext FAIL [5/5] PIXEL: the screendump is not the Mac OS 8.1 SimpleText capture (s8_simpletext_doc.png)\n'; exit 1; }
@printf '>>> test-flair-itext [5/5] PIXEL: %s\n' "$$(cat $(BUILD)/flair_itext.pixel)"
@printf '>>> test-flair-itext: green\n'
```

## test-flair-itext-badname

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34190):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@printf 'InitechOS (STAPLER) -- make test-flair-itext-badname : an invalid 8.3 name in Save As alerts (initech-oobx L017)\n'
$(call appl-boot,flair_itext_badname,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_BADNAME_SPEC,)
$(call appl-boot,flair_itext_badname_up,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_BADNAME_UP_SPEC,--screendump --screendump-after "TENANT-SF badname")
@$(call APPL_ALIVE,$(BUILD)/flair_itext_badname.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_badname.serial) || { printf '!!! test-flair-itext-badname FAIL: TEXT did not come up\n'; exit 1; }
@$(call ITEXT_BADNAME,$(BUILD)/flair_itext_badname.serial) || { printf '!!! test-flair-itext-badname FAIL: no bad-name alert, or the box did not recover to a valid reply\n'; grep '^TENANT-SF' $(BUILD)/flair_itext_badname.serial; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) oknotice $(BUILD)/flair_itext_badname_up.ppm > $(BUILD)/flair_itext_badname.pixels 2>&1 || { cat $(BUILD)/flair_itext_badname.pixels; printf '!!! test-flair-itext-badname FAIL: the bad-name alert is not DRAWN as graded\n'; exit 1; }
@printf '>>> test-flair-itext-badname: %s; %s; alert pixel-graded (oknotice)\n' "$$(grep -m1 '^TENANT-SF badname' $(BUILD)/flair_itext_badname.serial)" "$$(grep -m1 '^TENANT-SF-REPLY' $(BUILD)/flair_itext_badname.serial)"
@printf '>>> test-flair-itext-badname: green\n'
```

## test-flair-itext-badname-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34200):
Prerequisites: `test-flair-itext-badname build/qemu_harness build/flair_tenants_mut_itext_badname.img build/flair_tenants_mut_alert_nodraw.img build/flair_data.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_badname_mut,$(BUILD)/flair_tenants_mut_itext_badname.img,$(FLAIR_DATA_IMG),FLAIR_ITEXT_BADNAME_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_badname_mut.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_badname_mut.serial) || { printf '!!! test-flair-itext-badname-mutant FAIL: wrong-reason RED (TEXT did not come up)\n'; exit 1; }
@if grep -q '^TENANT-SF badname' $(BUILD)/flair_itext_badname_mut.serial; then printf '!!! test-flair-itext-badname-mutant FAIL: the mutant still alerted\n'; exit 1; fi
@if $(call ITEXT_BADNAME,$(BUILD)/flair_itext_badname_mut.serial); then printf '!!! test-flair-itext-badname-mutant FAIL: the silent refusal PASSED the marker leg -- decoration\n'; exit 1; fi
@printf '>>> test-flair-itext-badname-mutant: SF_MUT_BADNAME_SILENT correctly RED (the alert marker is absent from the log)\n'
$(call appl-boot,flair_itext_badname_nodraw,$(BUILD)/flair_tenants_mut_alert_nodraw.img,$(FLAIR_DATA_IMG),FLAIR_ITEXT_BADNAME_SPEC,)
$(call appl-boot,flair_itext_badname_nodraw_up,$(BUILD)/flair_tenants_mut_alert_nodraw.img,$(FLAIR_DATA_IMG),FLAIR_ITEXT_BADNAME_UP_SPEC,--screendump --screendump-after "TENANT-SF badname")
@$(call APPL_ALIVE,$(BUILD)/flair_itext_badname_nodraw.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_badname_nodraw.serial) && $(call ITEXT_BADNAME,$(BUILD)/flair_itext_badname_nodraw.serial) || { printf '!!! test-flair-itext-badname-mutant FAIL: ALERT_NODRAW must keep the marker AND the reply (wrong-reason RED)\n'; exit 1; }
@if $(PPM_FLAIR_TRASH_CHECK_BIN) oknotice $(BUILD)/flair_itext_badname_nodraw_up.ppm > $(BUILD)/flair_itext_badname_nodraw.pixels 2>&1; then printf '!!! test-flair-itext-badname-mutant FAIL: the undrawn alert PASSED the pixel leg -- decoration\n'; exit 1; fi
@printf '>>> test-flair-itext-badname-mutant: KMAIN_MUT_ALERT_NODRAW correctly RED -- markers + reply present, pixel leg: %s\n' "$$(grep -m1 'FAIL' $(BUILD)/flair_itext_badname_nodraw.pixels | cut -c1-120)"
```

## test-flair-itext-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 119):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-itext-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-itext-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-itext_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-itext_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-itext_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-itext_bochs.report.txt" || true
@cat "$(BUILD)/flair-itext_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-itext-bochs "$(BUILD)/flair-itext_bochs.report.txt" "$(BUILD)/flair-itext_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-itext-full

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34212):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_itext_full.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
@printf 'InitechOS (STAPLER) -- make test-flair-itext-full : typing into a full document raises the notice (initech-oobx L014)\n'
$(call appl-boot,flair_itext_full,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_FULL_IMG),FLAIR_ITEXT_FULL_SPEC,--screendump --screendump-after "TENANT-ALERT up")
@$(call APPL_ALIVE,$(BUILD)/flair_itext_full.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_full.serial) || { printf '!!! test-flair-itext-full FAIL: TEXT did not come up\n'; exit 1; }
@$(call ITEXT_FULL,$(BUILD)/flair_itext_full.serial) || { printf '!!! test-flair-itext-full FAIL: no full-document notice\n'; grep -E '^(TENANT-(ALERT|SF)|PANIC)' $(BUILD)/flair_itext_full.serial; exit 1; }
@$(PPM_FLAIR_TRASH_CHECK_BIN) oknotice $(BUILD)/flair_itext_full.ppm > $(BUILD)/flair_itext_full.pixels 2>&1 || { cat $(BUILD)/flair_itext_full.pixels; printf '!!! test-flair-itext-full FAIL: the notice is not DRAWN as graded\n'; exit 1; }
@printf '>>> test-flair-itext-full: %s; notice pixel-graded (oknotice)\n>>> test-flair-itext-full: green\n' "$$(grep -m1 '^TENANT-ALERT up' $(BUILD)/flair_itext_full.serial)"
```

## test-flair-itext-full-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34220):
Prerequisites: `test-flair-itext-full build/qemu_harness build/flair_tenants_mut_alert_nodraw.img build/flair_data_itext_full.img build/ppm_flair_trash_check build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_full_nodraw,$(BUILD)/flair_tenants_mut_alert_nodraw.img,$(FLAIR_DATA_ITEXT_FULL_IMG),FLAIR_ITEXT_FULL_SPEC,--screendump --screendump-after "TENANT-ALERT up")
@$(call APPL_ALIVE,$(BUILD)/flair_itext_full_nodraw.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_full_nodraw.serial) && $(call ITEXT_FULL,$(BUILD)/flair_itext_full_nodraw.serial) || { printf '!!! test-flair-itext-full-mutant FAIL: ALERT_NODRAW must keep the markers and the reply (wrong-reason RED)\n'; exit 1; }
@if $(PPM_FLAIR_TRASH_CHECK_BIN) oknotice $(BUILD)/flair_itext_full_nodraw.ppm > $(BUILD)/flair_itext_full_nodraw.pixels 2>&1; then printf '!!! test-flair-itext-full-mutant FAIL: the undrawn notice PASSED the pixel leg -- decoration\n'; exit 1; fi
@printf '>>> test-flair-itext-full-mutant: KMAIN_MUT_ALERT_NODRAW correctly RED -- markers + reply present, pixel leg: %s\n' "$$(grep -m1 'FAIL' $(BUILD)/flair_itext_full_nodraw.pixels | cut -c1-120)"
```

## test-flair-itext-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34142):
Prerequisites: `test-flair-itext build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_tenants_mut_itext_g12row.img build/flair_data_itext_nocopy.img build/flair_data_itext_nocr.img build/flair_data_itext_plain.img build/ppm_flair_itext_check build/itext_memo.expect build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_nocopy,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_NOCOPY_IMG),FLAIR_ITEXT_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_nocopy.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_nocopy.serial) && $(call ITEXT_SAVEAS,$(BUILD)/flair_itext_nocopy.serial) || { printf '!!! test-flair-itext-mutant FAIL: COPY_NOTHING went RED for the wrong reason (want: TEXT up, Save As answered)\n'; exit 1; }
@if $(call ITEXT_EDIT,$(BUILD)/flair_itext_nocopy.serial); then printf '!!! test-flair-itext-mutant FAIL: COPY_NOTHING PASSED the EDIT leg -- it is decoration\n'; exit 1; fi
@if $(call ITEXT_DISK,$(BUILD)/flair_itext_nocopy_data.img); then printf '!!! test-flair-itext-mutant FAIL: COPY_NOTHING PASSED the DISK leg\n'; exit 1; fi
@printf '>>> test-flair-itext-mutant: COPY_NOTHING correctly RED -- %s; MEMO.TXT is %s bytes, not the locked 43\n' "$$(grep -m1 '^TENANT-SCRAP put' $(BUILD)/flair_itext_nocopy.serial)" "$$(wc -c < $(BUILD)/flair_itext_nocopy_data.img.memo)"
$(call appl-boot,flair_itext_nocr,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_NOCR_IMG),FLAIR_ITEXT_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_nocr.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_nocr.serial) && $(call ITEXT_EDIT,$(BUILD)/flair_itext_nocr.serial) && $(call ITEXT_SAVEAS,$(BUILD)/flair_itext_nocr.serial) || { printf '!!! test-flair-itext-mutant FAIL: SAVE_NO_CR went RED for the wrong reason (want: edit and Save As as locked)\n'; exit 1; }
@if $(call ITEXT_DISK,$(BUILD)/flair_itext_nocr_data.img); then printf '!!! test-flair-itext-mutant FAIL: SAVE_NO_CR PASSED the DISK leg -- it is decoration\n'; exit 1; fi
@printf '>>> test-flair-itext-mutant: SAVE_NO_CR correctly RED -- MEMO.TXT is %s bytes with %s CRs (the differential wants 43 with 2)\n' "$$(wc -c < $(BUILD)/flair_itext_nocr_data.img.memo)" "$$(tr -cd '\r' < $(BUILD)/flair_itext_nocr_data.img.memo | wc -c)"
$(call appl-boot,flair_itext_plain,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_PLAIN_IMG),FLAIR_ITEXT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_plain.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_plain.serial) && $(call ITEXT_DISK,$(BUILD)/flair_itext_plain_data.img) || { printf '!!! test-flair-itext-mutant FAIL: SEL_NOT_INVERSE went RED for the wrong reason (want: TEXT up, MEMO.TXT as locked)\n'; exit 1; }
@if $(call ITEXT_PIXEL,$(BUILD)/flair_itext_plain.ppm) > $(BUILD)/flair_itext_plain.pixel 2>&1; then printf '!!! test-flair-itext-mutant FAIL: SEL_NOT_INVERSE PASSED the PIXEL leg -- it is decoration\n'; exit 1; fi
@grep -q '^FAIL itext-pixel band top row: dump black pen -1..-1, capture 45..83' $(BUILD)/flair_itext_plain.pixel || { cat $(BUILD)/flair_itext_plain.pixel; printf '!!! test-flair-itext-mutant FAIL: SEL_NOT_INVERSE went RED for the wrong reason (want: no band)\n'; exit 1; }
@printf '>>> test-flair-itext-mutant: SEL_NOT_INVERSE correctly RED -- %s\n' "$$(grep -m1 'band top row' $(BUILD)/flair_itext_plain.pixel)"
$(call appl-boot,flair_itext_g12row,$(FLAIR_ITEXT_G12ROW_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_g12row.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_g12row.serial) && $(call ITEXT_EDIT,$(BUILD)/flair_itext_g12row.serial) && $(call ITEXT_DISK,$(BUILD)/flair_itext_g12row_data.img) || { printf '!!! test-flair-itext-mutant FAIL: G12ROW went RED for the wrong reason (want: TEXT up, edit as locked, MEMO.TXT as locked)\n'; exit 1; }
@if $(call ITEXT_PIXEL,$(BUILD)/flair_itext_g12row.ppm) > $(BUILD)/flair_itext_g12row.pixel 2>&1; then printf '!!! test-flair-itext-mutant FAIL: a flipped row of the packed Geneva 12 strike PASSED the PIXEL leg -- it is decoration\n'; exit 1; fi
@grep -q "^FAIL itext-pixel L0 cell 0 'S': dump ink x 1..6 y 2..11, capture x 1..6 y 3..11" $(BUILD)/flair_itext_g12row.pixel || { cat $(BUILD)/flair_itext_g12row.pixel; printf '!!! test-flair-itext-mutant FAIL: G12ROW went RED for the wrong reason (want: the S ink box)\n'; exit 1; }
@printf '>>> test-flair-itext-mutant: GENEVA12_PACK_MUT_ROW correctly RED -- %s\n' "$$(grep -m1 "L0 cell 0 'S'" $(BUILD)/flair_itext_g12row.pixel)"
@printf '>>> test-flair-itext-mutant: green (all four mutants RED for the named reason)\n'
```

## test-flair-itext-scroll

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34239):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_flair_scroll_look_check test-itext-route test-scrollbar-look build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_scroll,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_SCROLL_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_scroll.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_scroll.serial)
@$(call ITEXT_SCROLL_INPUT,$(BUILD)/flair_itext_scroll.serial) || { printf 'FAIL itext-scroll: expected exactly forty key events\n'; exit 1; }
@$(call ITEXT_SCROLL_SERIAL,$(BUILD)/flair_itext_scroll.serial) || { printf 'FAIL itext-scroll: forty Returns did not set range/value275 (Geneva 12, initech-uelk)\n'; exit 1; }
@$(SCROLL_LOOK_CHECK) itext $(BUILD)/flair_itext_scroll.ppm
@printf '>>> test-flair-itext-scroll: green (40 Returns; Geneva 12 doc664 range/value275; whole enabled bar; 16-px caret in view)\n'
```

## test-flair-itext-scroll-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 120):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-itext-scroll-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-itext-scroll-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-itext-scroll_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-itext-scroll_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-itext-scroll_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-itext-scroll_bochs.report.txt" || true
@cat "$(BUILD)/flair-itext-scroll_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-itext-scroll-bochs "$(BUILD)/flair-itext-scroll_bochs.report.txt" "$(BUILD)/flair-itext-scroll_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-itext-scroll-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34246):
Prerequisites: `test-flair-itext-scroll build/flair_data_itext_noscroll.img build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_scroll_mut,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_NOSCROLL_IMG),FLAIR_ITEXT_SCROLL_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_scroll_mut.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_scroll_mut.serial)
@$(call ITEXT_SCROLL_INPUT,$(BUILD)/flair_itext_scroll_mut.serial)
@grep '^TENANT-GATE ax=0x00c1 ' $(BUILD)/flair_itext_scroll_mut.serial | tail -1 | grep -qx 'TENANT-GATE ax=0x00c1 win=1 len=389 value=275 -> 0'
@if $(call ITEXT_SCROLL_SERIAL,$(BUILD)/flair_itext_scroll_mut.serial); then printf 'FAIL scroll-never-enabled passed serial leg\n'; exit 1; fi
@if $(SCROLL_LOOK_CHECK) itext $(BUILD)/flair_itext_scroll_mut.ppm > $(BUILD)/flair_itext_scroll_mut.pixels 2>&1; then printf 'FAIL scroll-never-enabled passed pixel leg\n'; exit 1; fi
@grep -q 'FAIL scroll-look crop thumb-v' $(BUILD)/flair_itext_scroll_mut.pixels
@printf '>>> test-flair-itext-scroll-mutant: ITEXT_MUT_SCROLL_NEVER_ENABLED correctly RED (range0; independent thumb-v crop)\n'
```

## test-flair-itext-undo

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34280):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/itext_undo.expect build/.makefile.stamp`
```make
@printf 'InitechOS (STAPLER) -- make test-flair-itext-undo : Ctrl-A, Ctrl-X, Ctrl-Z brings the cut text back; saved bytes exact (initech-4hkk L011)\n'
$(call appl-boot,flair_itext_undo,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_ITEXT_UNDO_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_undo.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_undo.serial) || { printf '!!! test-flair-itext-undo FAIL: TEXT did not come up\n'; exit 1; }
@$(call ITEXT_UNDO_PAINT,$(BUILD)/flair_itext_undo.serial) || { printf '!!! test-flair-itext-undo FAIL [1/3] PAINT: the restored lines are not the last paint\n'; grep '^TENANT-GATE ax=0x0030 win=1 ' $(BUILD)/flair_itext_undo.serial | tail -6; exit 1; }
@printf '>>> test-flair-itext-undo [1/3] PAINT: after Ctrl-Z the last paint is "Saving tables to disk" / "PC LOAD LETTER", selected (white ink on black)\n'
@$(call ITEXT_UNDO_SAVE,$(BUILD)/flair_itext_undo.serial) || { printf '!!! test-flair-itext-undo FAIL [2/3] SAVE: no Save As reply UNDO.TXT\n'; grep '^TENANT-SF' $(BUILD)/flair_itext_undo.serial; exit 1; }
@printf '>>> test-flair-itext-undo [2/3] SAVE: %s\n' "$$(grep -m1 '^TENANT-SF-REPLY' $(BUILD)/flair_itext_undo.serial)"
@$(call ITEXT_UNDO_DISK,$(BUILD)/flair_itext_undo_data.img) || { printf '!!! test-flair-itext-undo FAIL [3/3] DISK: UNDO.TXT is not the restored 36 bytes\n'; xxd $(BUILD)/flair_itext_undo_data.img.undo 2>/dev/null | head -4; exit 1; }
@printf '>>> test-flair-itext-undo [3/3] DISK: UNDO.TXT (%s bytes) == "Saving tables to disk" CR "PC LOAD LETTER" (mtools); fsck.fat clean\n>>> test-flair-itext-undo: green\n' "$$(wc -c < $(BUILD)/flair_itext_undo_data.img.undo)"
```

## test-flair-itext-undo-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34290):
Prerequisites: `test-flair-itext-undo build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_itext_undonone.img build/itext_undo.expect build/.makefile.stamp`
```make
$(call appl-boot,flair_itext_undonone,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_ITEXT_UNDONONE_IMG),FLAIR_ITEXT_UNDO_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_itext_undonone.serial) && $(call ITEXT_UP,$(BUILD)/flair_itext_undonone.serial) && $(call ITEXT_UNDO_SAVE,$(BUILD)/flair_itext_undonone.serial) || { printf '!!! test-flair-itext-undo-mutant FAIL: UNDO_NO_INVERSE went RED for the wrong reason (want: TEXT up, Save As answered)\n'; exit 1; }
@if $(call ITEXT_UNDO_PAINT,$(BUILD)/flair_itext_undonone.serial); then printf '!!! test-flair-itext-undo-mutant FAIL: UNDO_NO_INVERSE PASSED the PAINT leg -- decoration\n'; exit 1; fi
@if $(call ITEXT_UNDO_DISK,$(BUILD)/flair_itext_undonone_data.img); then printf '!!! test-flair-itext-undo-mutant FAIL: UNDO_NO_INVERSE PASSED the DISK leg -- decoration\n'; exit 1; fi
@printf '>>> test-flair-itext-undo-mutant: ITEXT_MUT_UNDO_NO_INVERSE correctly RED -- no restored paint; UNDO.TXT is %s bytes, not the locked 36\n' "$$(wc -c < $(BUILD)/flair_itext_undonone_data.img.undo 2>/dev/null || echo 0)"
```

## test-flair-notepad-itext

Declared tier: EMU; #  recipe to execute (from 'harness/emu/notepad.mk', line 35):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/da_store_check build/.makefile.stamp`
```make
$(call appl-boot,flair_note_itext,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_NOTEPAD_ITEXT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call DA_UP,$(BUILD)/flair_note_itext.serial,NOTEPAD) && $(call ITEXT_UP,$(BUILD)/flair_note_itext.serial)
@[ "$$(grep -c '^TENANT-SCRAP put n=5 ' $(BUILD)/flair_note_itext.serial)" = 2 ] && grep -qx 'TENANT-SCRAP get n=5' $(BUILD)/flair_note_itext.serial
@grep -q 's="alpha" fg=65536 bg=1' $(BUILD)/flair_note_itext.serial
@grep -qx 'TENANT-ALERT-HIT item=3' $(BUILD)/flair_note_itext.serial && [ "$$(grep -c '^TENANT-REGISTER ok app=NOTEPAD' $(BUILD)/flair_note_itext.serial)" = 2 ]
@mcopy -o -i $(BUILD)/flair_note_itext_data.img ::NOTEPAD.DAT $(BUILD)/flair_note_itext.dat
@$(DA_DISK) pad $(BUILD)/flair_note_itext.dat 1 alpha alphar
@printf '>>> test-flair-notepad-itext: green (Note Pad -> InitechText -> Note Pad; two Copy puts; independent FAT alpha/alphar)\n'
```

## test-flair-notepad-itext-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 207):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-notepad-itext-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-notepad-itext-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-notepad-itext_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-notepad-itext_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-notepad-itext_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-notepad-itext_bochs.report.txt" || true
@cat "$(BUILD)/flair-notepad-itext_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-notepad-itext-bochs "$(BUILD)/flair-notepad-itext_bochs.report.txt" "$(BUILD)/flair-notepad-itext_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-itext-route

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14600):
Prerequisites: `build/test_itext_route build/.makefile.stamp`
```make
@printf '>>> test-itext-route: InitechText verbs over the real TextEdit, gate mocked (initech-tdnl.16)\n'
@$(TEST_ITEXT_ROUTE)
@printf '>>> test-itext-route: green\n'
```

## test-itext-route-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14606):
Prerequisites: `build/test_itext_route_mutant_ITEXT_MUT_COPY_NOTHING build/test_itext_route_mutant_ITEXT_MUT_SAVE_NO_CR build/test_itext_route_mutant_ITEXT_MUT_QUIT_NO_ASK build/test_itext_route_mutant_ITEXT_MUT_SEL_NOT_INVERSE build/test_itext_route_mutant_ITEXT_MUT_SCROLL_NEVER_ENABLED build/test_itext_route_mutant_ITEXT_MUT_LF_PAIR_OUTPUT build/test_itext_route_mutant_ITEXT_MUT_FULL_SILENT build/test_itext_route_mutant_ITEXT_MUT_UNDO_NO_INVERSE build/test_itext_route_mutant_ITEXT_MUT_UNDO_PER_KEY build/test_itext_route_mutant_ITEXT_MUT_UNDO_KEEPS_HISTORY build/.makefile.stamp`
```make
@for pair in $(ITEXT_ROUTE_MUTANTS); do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_itext_route_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-itext-route-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-itext-route-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; tail -5 $$bin.log; exit 1; }; \
	grep -q "^RED-REASON $$why:" $$bin.log || { printf '!!! test-itext-route-mutant FAIL: %s went RED for the wrong reason (want %s)\n' "$$m" "$$why"; grep '^RED-REASON' $$bin.log; exit 1; }; \
	printf '>>> test-itext-route-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^RED-REASON $$why:" $$bin.log)"; \
done
@printf '>>> test-itext-route-mutant: green (all ten mutants RED for the named reason)\n'
```

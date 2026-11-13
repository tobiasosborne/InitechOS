## test-flair-iword

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32358):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_region_count build/.makefile.stamp`
```make
$(call need_wp_goldens,test-flair-iword)
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword : InitechWord types, wraps, saves and retrieves a document (initech-l4tj)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_iword,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword.serial) && $(call IWORD_UP,$(BUILD)/flair_iword.serial) || { printf '!!! test-flair-iword FAIL [1/4] UP: WORD did not come up or the desktop died\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED|FLAIR-DISPATCH)' $(BUILD)/flair_iword.serial; exit 1; }
@printf '>>> test-flair-iword [1/4] UP: %s; registered, dispatched\n' "$$(grep -m1 '^TENANT-LOAD name=WORD' $(BUILD)/flair_iword.serial)"
@$(call IWORD_TYPED,$(BUILD)/flair_iword.serial) || { printf '!!! test-flair-iword FAIL [2/4] TYPED: the wrapped paragraph / the save status line / the WP prompts / the clear screen were not all drawn\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=1 y=\(0\|16\|368\) ' $(BUILD)/flair_iword.serial | tail -12; exit 1; }
@printf '>>> test-flair-iword [2/4] TYPED: line 1 "...jumps over", line 2 "the lazy dog." (the WP 5.1 wrap); saved "A:\\MEMO ... Ln 1.17\" Pos 2.3\""; Document to be Saved: / Save Document? Yes (No) / Exit WP? No (Yes) / Document to be Retrieved:; screen cleared\n'
@mcopy -o -i $(BUILD)/flair_iword_data.img ::MEMO $(BUILD)/iword-memo.WP5 || { printf '!!! test-flair-iword FAIL [3/4] DISK: F10 did not create MEMO on the data volume\n'; exit 1; }
@head -c 327 $(WORDPERFECT51_DECOMP)/goldens/minted/MINT02.WP5 > $(IWORD_MEMO_EXPECT)
@printf '%s\r%s' '$(IWORD_MEMO_TEXT)' '$(IWORD_MEMO_TAIL)' >> $(IWORD_MEMO_EXPECT)
@cmp $(IWORD_MEMO_EXPECT) $(BUILD)/iword-memo.WP5 || { printf '!!! test-flair-iword FAIL [3/4] DISK: MEMO is not the MINT02 prefix + the wrapped paragraph\n'; exit 1; }
@$(call iword_ref_text,$(BUILD)/iword-memo.WP5,$(IWORD_MEMO_TEXT)$(IWORD_MEMO_TAIL)) && grep -qF "T'$(IWORD_MEMO_TEXT)' [SRt] T'$(IWORD_MEMO_TAIL)'" $(BUILD)/iword-memo.WP5.ref || { printf '!!! test-flair-iword FAIL [3/4] DISK: wp5_ref.py does not read MEMO as the wrapped paragraph\n'; cat $(BUILD)/iword-memo.WP5.ref; exit 1; }
fsck.fat -n $(BUILD)/flair_iword_data.img
@printf '>>> test-flair-iword [3/4] DISK: MEMO (%s bytes) == MINT02.WP5 prefix + paragraph with the SRt; wp5_ref.py: %s\n' "$$(wc -c < $(BUILD)/iword-memo.WP5)" "$$(grep -F 'END offset' $(BUILD)/iword-memo.WP5.ref | sed 's/^ *//')"
@$(call IWORD_BACK,$(BUILD)/flair_iword.serial) || { printf '!!! test-flair-iword FAIL [4/4] BACK: after Shift-F10 the paragraph / the status line are not the last draws\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=1 y=\(0\|16\|368\) ' $(BUILD)/flair_iword.serial | tail -6; exit 1; }
@f=$(BUILD)/flair_iword.ppm; \
	set -- $$($(call IWORD_PX,$$f,15 62 511 78)); l1=$$1; \
	set -- $$($(call IWORD_PX,$$f,511 62 527 78)); gap=$$1; \
	set -- $$($(call IWORD_PX,$$f,7 78 111 94)); l2=$$1; \
	set -- $$($(call IWORD_PX,$$f,7 62 15 78)); cur=$$1; \
	set -- $$($(call IWORD_PX,$$f,7 94 615 430)); rest=$$1; \
	if [ "$$l1" -lt 500 ] || [ "$$gap" -ne 0 ] || [ "$$l2" -lt 100 ] || [ "$$cur" -lt 48 ] || [ "$$rest" -ne 0 ]; then \
		printf '!!! test-flair-iword FAIL [4/4] SCREEN: line 1 ink %s px (>= 500), columns 63-64 ink %s px (== 0), line 2 ink %s px (>= 100), cursor cell %s px (>= 48), lines 3-23 ink %s px (== 0)\n' "$$l1" "$$gap" "$$l2" "$$cur" "$$rest"; exit 1; fi; \
	printf '>>> test-flair-iword [4/4] BACK: Shift-F10 brought the paragraph back (last draws); screen: line 1 ink %s px, columns 63-64 empty, line 2 ink %s px, cursor cell %s px, lines 3-23 empty\n' "$$l1" "$$l2" "$$cur"
@printf '>>> test-flair-iword: green\n'
```

## test-flair-iword-block

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32621):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_region_count build/.makefile.stamp`
```make
$(call need_wp_goldens,test-flair-iword-block)
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-block : Block, Move, Delete [code]?, Tab and hard page (initech-jojk)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_iword_block,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_BLOCK_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_block.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_block.serial) || { printf '!!! test-flair-iword-block FAIL [1/5] UP: WORD did not come up or the desktop died\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED|FLAIR-DISPATCH)' $(BUILD)/flair_iword_block.serial; exit 1; }
@printf '>>> test-flair-iword-block [1/5] UP: %s; registered, dispatched\n' "$$(grep -m1 '^TENANT-LOAD name=WORD' $(BUILD)/flair_iword_block.serial)"
@$(call IWORD_B_KEYS,$(BUILD)/flair_iword_block.serial) || { printf '!!! test-flair-iword-block FAIL [2/5] KEYS: Block on / the band / the Move menus / the Move prompt / Delete [BOLD]? were not all drawn\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(16\|368\) ' $(BUILD)/flair_iword_block.serial | tail -20; exit 1; }
@printf '>>> test-flair-iword-block [2/5] KEYS: "Block on", "Beta " in the band (fg=1 bg=2), the two Move menus, "Move cursor; press Enter to retrieve.", "Delete [BOLD]? No (Yes)" -- as WP 5.1 draws them\n'
@$(call IWORD_B_DISK,flair_iword_block) || { printf '!!! test-flair-iword-block FAIL [3/5] DISK: BLOCK is not byte for byte the MINT20 WP.EXE saved for the same keys\n'; cmp -l $(IWORD_MINT20) $(BUILD)/flair_iword_block.WP5 | head; exit 1; }
@python3 -I $(WORDPERFECT51_DECOMP)/tools/wp5_ref.py $(BUILD)/flair_iword_block.WP5 > $(BUILD)/flair_iword_block.WP5.ref && grep -qF "T'x' [Tab 02080708070f00] T'tab' [var0xd4/0x00" $(BUILD)/flair_iword_block.WP5.ref && grep -qF "[HPg] T'Page two.'" $(BUILD)/flair_iword_block.WP5.ref && grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$$' $(BUILD)/flair_iword_block.WP5.ref || { printf '!!! test-flair-iword-block FAIL [3/5] DISK: wp5_ref.py does not read BLOCK clean\n'; cat $(BUILD)/flair_iword_block.WP5.ref; exit 1; }
fsck.fat -n $(BUILD)/flair_iword_block_data.img
@printf '>>> test-flair-iword-block [3/5] DISK: BLOCK (%s bytes) == MINT20 byte for byte; wp5_ref.py: %s\n' "$$(wc -c < $(BUILD)/flair_iword_block.WP5)" "$$(grep -F 'END offset' $(BUILD)/flair_iword_block.WP5.ref | sed 's/^ *//')"
@$(call IWORD_B_REVEAL,$(BUILD)/flair_iword_block.serial) || { printf '!!! test-flair-iword-block FAIL [4/5] REVEAL: the rows / status / codes pane / cursor / message are not the last draws\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(16\|32\|48\|64\|160\|192\|208\|224\|240\|368\) ' $(BUILD)/flair_iword_block.serial | tail -14; exit 1; }
@printf '>>> test-flair-iword-block [4/5] REVEAL: "Alpha Gamma.Beta", "x    tab", the bar, "Page two."; "Doc 1 Pg 2 Ln 1\" Pos 1.9\""; pane "[HRt]" / "Alpha Gamma.Beta [HRt]" / "x[Tab]tab[HPg]" / "Page two." + cursor (MINT20 04_typed, 06_reveal)\n'
@f=$(BUILD)/flair_iword_block.ppm; $(call IWORD_B_PIXELS,$$f) || { set -- $$($(call IWORD_PX,$$f,7 110 615 126)); printf '!!! test-flair-iword-block FAIL [5/5] PIXELS: bar ink %s (>= 600), tab gap %s (== 0), "tab" %s (>= 20)\n' "$$1" "$$($(call IWORD_PX,$$f,15 94 47 110) | cut -d' ' -f1)" "$$($(call IWORD_PX,$$f,47 94 71 110) | cut -d' ' -f1)"; exit 1; }
@f=$(BUILD)/flair_iword_block.ppm; printf '>>> test-flair-iword-block [5/5] PIXELS: bar row ink %s px, tab gap 0 px, "tab" %s px\n' "$$($(call IWORD_PX,$$f,7 110 615 126) | cut -d' ' -f1)" "$$($(call IWORD_PX,$$f,47 94 71 110) | cut -d' ' -f1)"
@printf '>>> test-flair-iword-block: green\n'
```

## test-flair-iword-block-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-block-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-block-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-block_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-block_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-block_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-block_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-block_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-block-bochs "$(BUILD)/flair-iword-block_bochs.report.txt" "$(BUILD)/flair-iword-block_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-block-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32641):
Prerequisites: `test-flair-iword-block build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_no_d4.img build/flair_data_iword_no_block_on.img build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_block_nod4,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOD4_IMG),FLAIR_IWORD_BLOCK_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_block_nod4.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_block_nod4.serial) && $(call IWORD_B_KEYS,$(BUILD)/flair_iword_block_nod4.serial) || { printf '!!! test-flair-iword-block-mutant FAIL: NO_D4 went RED for the wrong reason (want: WORD up, every prompt drawn)\n'; exit 1; }
@if $(call IWORD_B_DISK,flair_iword_block_nod4); then printf '!!! test-flair-iword-block-mutant FAIL: NO_D4 PASSED the DISK leg -- it is decoration\n'; exit 1; fi
@printf '>>> test-flair-iword-block-mutant: NO_D4 correctly RED -- prompts drawn, BLOCK (%s bytes) != MINT20 (%s bytes): the hard page lost its D4/00 group\n' "$$(wc -c < $(BUILD)/flair_iword_block_nod4.WP5)" "$$(wc -c < $(IWORD_MINT20))"
$(call appl-boot,flair_iword_block_noblk,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOBLK_IMG),FLAIR_IWORD_BLOCK_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_block_noblk.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_block_noblk.serial) && $(call IWORD_B_DISK,flair_iword_block_noblk) || { printf '!!! test-flair-iword-block-mutant FAIL: NO_BLOCK_ON went RED for the wrong reason (want: WORD up, BLOCK == MINT20)\n'; exit 1; }
@if $(call IWORD_B_KEYS,$(BUILD)/flair_iword_block_noblk.serial); then printf '!!! test-flair-iword-block-mutant FAIL: NO_BLOCK_ON PASSED the KEYS leg -- it is decoration\n'; exit 1; fi
@printf '>>> test-flair-iword-block-mutant: NO_BLOCK_ON correctly RED -- BLOCK == MINT20, but "Block on" was never drawn\n'
@printf '>>> test-flair-iword-block-mutant: green (both mutants RED for the named reason)\n'
```

## test-flair-iword-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 110):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-bochs "$(BUILD)/flair-iword_bochs.report.txt" "$(BUILD)/flair-iword_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-codes

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32502):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_iword_codes_check build/.makefile.stamp`
```make
$(call need_wp_goldens,test-flair-iword-codes)
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-codes : WordPerfect attribute codes, drawn and revealed (initech-jojk)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_iword_codes,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_CODES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_codes.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_codes.serial) || { printf '!!! test-flair-iword-codes FAIL [1/5] UP: WORD did not come up or the desktop died\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED|FLAIR-DISPATCH)' $(BUILD)/flair_iword_codes.serial; exit 1; }
@printf '>>> test-flair-iword-codes [1/5] UP: %s; registered, dispatched\n' "$$(grep -m1 '^TENANT-LOAD name=WORD' $(BUILD)/flair_iword_codes.serial)"
@$(call IWORD_C_KEYS,$(BUILD)/flair_iword_codes.serial) || { printf '!!! test-flair-iword-codes FAIL [2/5] KEYS: the plain line / the styled draws / the Font and Appearance menus were not all drawn\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(0\|16\|368\) ' $(BUILD)/flair_iword_codes.serial | tail -16; exit 1; }
@printf '>>> test-flair-iword-codes [2/5] KEYS: "bold" fg=256 (bold), "under" fg=1024 (underline), "slant" fg=512 (italic) drawn on line 2, accepted; Font menu + Appearance menu shown as WP 5.1 shows them\n'
@mcopy -o -i $(BUILD)/flair_iword_codes_data.img ::CODES $(BUILD)/iword-codes.WP5 || { printf '!!! test-flair-iword-codes FAIL [3/5] DISK: F10 did not create CODES on the data volume\n'; exit 1; }
@head -c 327 $(IWORD_MINT02) > $(IWORD_CODES_EXPECT)
@printf 'bold under slant\n\303\014\303bold\304\014\304 \303\016\303under\304\016\304 \303\010\303slant\304\010\304' >> $(IWORD_CODES_EXPECT)
@cmp $(IWORD_CODES_EXPECT) $(BUILD)/iword-codes.WP5 || { printf '!!! test-flair-iword-codes FAIL [3/5] DISK: CODES is not the cursor-at-end prefix of MINT02 + the three attribute pairs\n'; exit 1; }
@$(call iword_ref_text,$(BUILD)/iword-codes.WP5,bold under slantbold under slant) && grep -qF "T'bold under slant' [HRt] [AttrOn Bold] T'bold' [AttrOff Bold] T' ' [AttrOn Underline] T'under' [AttrOff Underline] T' ' [AttrOn Italic] T'slant' [AttrOff Italic]" $(BUILD)/iword-codes.WP5.ref || { printf '!!! test-flair-iword-codes FAIL [3/5] DISK: wp5_ref.py does not read the three pairs in CODES\n'; cat $(BUILD)/iword-codes.WP5.ref; exit 1; }
fsck.fat -n $(BUILD)/flair_iword_codes_data.img
@printf '>>> test-flair-iword-codes [3/5] DISK: CODES (%s bytes) == MINT02 prefix (0x00 at 0x103: saved at the end) + the body with the pairs; wp5_ref.py: %s\n' "$$(wc -c < $(BUILD)/iword-codes.WP5)" "$$(grep -F 'END offset' $(BUILD)/iword-codes.WP5.ref | sed 's/^ *//')"
@$(call IWORD_C_REVEAL,$(BUILD)/flair_iword_codes.serial) || { printf '!!! test-flair-iword-codes FAIL [4/5] REVEAL: the status line / ruler / codes pane / cursor / message are not the last draws\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(160\|176\|192\|208\|368\) ' $(BUILD)/flair_iword_codes.serial | tail -8; exit 1; }
@printf '>>> test-flair-iword-codes [4/5] REVEAL: status above the ruler, "[BOLD]bold[bold] [UND]under[und] [ITALC]slant[italc]" in the pane, cursor after [italc], "Press Reveal Codes to restore screen"\n'
@$(call IWORD_CODES_CHECK,$(BUILD)/flair_iword_codes.ppm) > $(BUILD)/flair_iword_codes.style || { printf '!!! test-flair-iword-codes FAIL [5/5] STYLE:\n'; cat $(BUILD)/flair_iword_codes.style; exit 1; }
@sed 's/^/>>> test-flair-iword-codes [5\/5] /' $(BUILD)/flair_iword_codes.style
@printf '>>> test-flair-iword-codes: green\n'
```

## test-flair-iword-codes-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-codes-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-codes-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-codes_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-codes_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-codes_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-codes_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-codes_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-codes-bochs "$(BUILD)/flair-iword-codes_bochs.report.txt" "$(BUILD)/flair-iword-codes_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-codes-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32525):
Prerequisites: `test-flair-iword-codes build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_tenants_mut_iword_ullow.img build/flair_data.img build/flair_data_iword_noface.img build/ppm_iword_codes_check build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_codes_noface,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOFACE_IMG),FLAIR_IWORD_CODES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_codes_noface.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_codes_noface.serial) && $(call IWORD_DREW,$(BUILD)/flair_iword_codes_noface.serial,16,$(IWORD_C_ROW)) && $(call IWORD_C_REVEAL,$(BUILD)/flair_iword_codes_noface.serial) || { printf '!!! test-flair-iword-codes-mutant FAIL: NO_FACE went RED for the wrong reason (want: WORD up, both lines typed, Reveal Codes shown)\n'; exit 1; }
@if $(call IWORD_STYLED,$(BUILD)/flair_iword_codes_noface.serial); then printf '!!! test-flair-iword-codes-mutant FAIL: NO_FACE still drew styled runs -- the KEYS leg is decoration\n'; exit 1; fi
@if $(call IWORD_CODES_CHECK,$(BUILD)/flair_iword_codes_noface.ppm) > $(BUILD)/flair_iword_codes_noface.style; then printf '!!! test-flair-iword-codes-mutant FAIL: NO_FACE PASSED the STYLE leg -- it is decoration\n'; exit 1; fi
@grep -q '^STYLE RED: columns 0..3 face 1' $(BUILD)/flair_iword_codes_noface.style || { printf '!!! test-flair-iword-codes-mutant FAIL: NO_FACE went RED for the wrong reason:\n'; cat $(BUILD)/flair_iword_codes_noface.style; exit 1; }
@printf '>>> test-flair-iword-codes-mutant: NO_FACE correctly RED -- no styled draw on line 2; %s\n' "$$(grep -m1 '^STYLE RED' $(BUILD)/flair_iword_codes_noface.style)"
$(call appl-boot,flair_iword_codes_ullow,$(FLAIR_IWORD_ULLOW_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_CODES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_codes_ullow.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_codes_ullow.serial) && $(call IWORD_C_KEYS,$(BUILD)/flair_iword_codes_ullow.serial) && $(call IWORD_C_REVEAL,$(BUILD)/flair_iword_codes_ullow.serial) || { printf '!!! test-flair-iword-codes-mutant FAIL: UL_LOW went RED for the wrong reason (want: every serial leg green)\n'; exit 1; }
@if $(call IWORD_CODES_CHECK,$(BUILD)/flair_iword_codes_ullow.ppm) > $(BUILD)/flair_iword_codes_ullow.style; then printf '!!! test-flair-iword-codes-mutant FAIL: UL_LOW PASSED the STYLE leg -- the pixel model is decoration\n'; exit 1; fi
@grep -q '^STYLE green: columns 0..3 face 1' $(BUILD)/flair_iword_codes_ullow.style && grep -q '^STYLE RED: columns 5..9 face 4' $(BUILD)/flair_iword_codes_ullow.style || { printf '!!! test-flair-iword-codes-mutant FAIL: UL_LOW went RED for the wrong reason:\n'; cat $(BUILD)/flair_iword_codes_ullow.style; exit 1; }
@printf '>>> test-flair-iword-codes-mutant: UL_LOW correctly RED -- serial legs green, bold green, %s\n' "$$(grep -m1 '^STYLE RED' $(BUILD)/flair_iword_codes_ullow.style)"
@printf '>>> test-flair-iword-codes-mutant: green (both mutants RED for the named reason)\n'
```

## test-flair-iword-fkeys

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 28):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/test_iword_p3 build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_fkeys,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys.serial)
@$(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys.serial
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_fkeys.ppm,7 430 79 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON fkeys: missing disabled-text pixels\n'; exit 1; }
@printf '>>> test-flair-iword-fkeys: green\n'
```

## test-flair-iword-fkeys-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-fkeys-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-fkeys-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-fkeys_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-fkeys_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-fkeys_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-fkeys_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-fkeys_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-fkeys-bochs "$(BUILD)/flair-iword-fkeys_bochs.report.txt" "$(BUILD)/flair-iword-fkeys_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-fkeys-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 40):
Prerequisites: `test-flair-iword-fkeys build/flair_data_iword_fkey_drop.img build/flair_data_iword_fkey_unmapped.img build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_fkeys_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_FKEY_DROP_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys_mut.serial)
@if $(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys_mut.serial > $(BUILD)/iword_fkeys_mut.log 2>&1; then printf 'FAIL: FKEY_DROP survived emulator gate\n'; exit 1; fi
@grep -F 'RED-REASON fkeys-serial:' $(BUILD)/iword_fkeys_mut.log
@printf '>>> test-flair-iword-fkeys-mutant: FKEY_DROP correctly RED: End Field missing\n'
$(call appl-boot,flair_iword_fkeys_unmapped,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_FKEY_UNMAPPED_IMG),FLAIR_IWORD_FKEYS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_fkeys_unmapped.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_fkeys_unmapped.serial)
@if $(BUILD)/test_iword_p3 fkeys-serial $(BUILD)/flair_iword_fkeys_unmapped.serial > $(BUILD)/iword_fkeys_unmapped_mut.log 2>&1; then printf 'FAIL: FKEY_UNMAPPED survived emulator gate\n'; exit 1; fi
@grep -F 'FAIL template unmapped keys do not clear' $(BUILD)/iword_fkeys_unmapped_mut.log
@printf '>>> test-flair-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)\n'
```

## test-flair-iword-format

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32728):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/ppm_region_count build/.makefile.stamp`
```make
$(call need_wp_goldens,test-flair-iword-format)
@test -f $(IWORD_MINT32) || { printf '!!! test-flair-iword-format FAIL: MINT32 (the golden) is missing from $(WORDPERFECT51_DECOMP)\n'; exit 1; }
@printf 'InitechOS (STAPLER) -- make test-flair-iword-format : Format screens, spacing, Center, Home, a block made bold (initech-jojk)\n'
$(call appl-boot,flair_iword_format,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_FORMAT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_format.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_format.serial) || { printf '!!! test-flair-iword-format FAIL [1/5] UP: WORD did not come up or the desktop died\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED|FLAIR-DISPATCH)' $(BUILD)/flair_iword_format.serial; exit 1; }
@printf '>>> test-flair-iword-format [1/5] UP: %s; registered, dispatched\n' "$$(grep -m1 '^TENANT-LOAD name=WORD' $(BUILD)/flair_iword_format.serial)"
@$(call IWORD_F_KEYS,$(BUILD)/flair_iword_format.serial) || { printf '!!! test-flair-iword-format FAIL [2/5] KEYS: the Format: Line field / the Justification menu / Block on / the band were not all drawn\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(48\|112\|208\|368\) ' $(BUILD)/flair_iword_format.serial | tail -20; exit 1; }
@printf '>>> test-flair-iword-format [2/5] KEYS: "Selection: 6", "6 - Line Spacing ... 2", the Justification menu, "3 - Justification ... Center", "Block on ... Pos 3.85\"", "old me." in the band (fg=1 bg=2) -- as WP 5.1 draws them\n'
@$(call IWORD_F_DISK,flair_iword_format) || { printf '!!! test-flair-iword-format FAIL [3/5] DISK: FMT is not byte for byte the MINT32 WP.EXE saved for the same keys\n'; cmp -l $(IWORD_MINT32) $(BUILD)/flair_iword_format.WP5 | head; exit 1; }
@python3 -I $(WORDPERFECT51_DECOMP)/tools/wp5_ref.py $(BUILD)/flair_iword_format.WP5 > $(BUILD)/flair_iword_format.WP5.ref && grep -qF "[var0xd0/0x02 size=8 conv=size=after-hdr 00010002] [Justification size=6 conv=size=after-hdr 0102]" $(BUILD)/flair_iword_format.WP5.ref && grep -qF "[AttrOn Bold] T'Bold me.' [AttrOff Bold]" $(BUILD)/flair_iword_format.WP5.ref && grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$$' $(BUILD)/flair_iword_format.WP5.ref || { printf '!!! test-flair-iword-format FAIL [3/5] DISK: wp5_ref.py does not read FMT clean\n'; cat $(BUILD)/flair_iword_format.WP5.ref; exit 1; }
fsck.fat -n $(BUILD)/flair_iword_format_data.img
@printf '>>> test-flair-iword-format [3/5] DISK: FMT (%s bytes) == MINT32 byte for byte; wp5_ref.py: %s\n' "$$(wc -c < $(BUILD)/flair_iword_format.WP5)" "$$(grep -F 'END offset' $(BUILD)/flair_iword_format.WP5.ref | sed 's/^ *//')"
@$(call IWORD_F_REVEAL,$(BUILD)/flair_iword_format.serial) || { printf '!!! test-flair-iword-format FAIL [4/5] REVEAL: the rows / status / codes pane / cursor / message are not the last draws\n'; grep '^TENANT-GATE ax=0x0033 win=1 x=[0-9]* y=\(16\|32\|48\|160\|192\|208\|224\|368\) ' $(BUILD)/flair_iword_format.serial | tail -14; exit 1; }
@printf '>>> test-flair-iword-format [4/5] REVEAL: "Centered." / "Bold me." at column 28, row 2 blank; "A:\\FMT ... Ln 1.5\" Pos 3.85\""; pane "[HRt]" / "[Ln Spacing:2][Just:Center]Centered.[HRt]" / "[BOLD]Bold me.[bold]" + the cursor on B (MINT32 03_typed, 05_reveal)\n'
@f=$(BUILD)/flair_iword_format.ppm; $(call IWORD_F_PIXELS,$$f) || { printf '!!! test-flair-iword-format FAIL [5/5] PIXELS: blank row ink %s (== 0), left of the centered text %s (== 0), "Centered." %s (>= 20)\n' "$$($(call IWORD_PX,$$f,7 94 615 110) | cut -d' ' -f1)" "$$($(call IWORD_PX,$$f,7 78 231 94) | cut -d' ' -f1)" "$$($(call IWORD_PX,$$f,231 78 303 94) | cut -d' ' -f1)"; exit 1; }
@f=$(BUILD)/flair_iword_format.ppm; printf '>>> test-flair-iword-format [5/5] PIXELS: blank row 0 px, left of the text 0 px, "Centered." %s px\n' "$$($(call IWORD_PX,$$f,231 78 303 94) | cut -d' ' -f1)"
@printf '>>> test-flair-iword-format: green\n'
```

## test-flair-iword-format-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-format-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-format-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-format_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-format_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-format_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-format_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-format_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-format-bochs "$(BUILD)/flair-iword-format_bochs.report.txt" "$(BUILD)/flair-iword-format_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-format-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32747):
Prerequisites: `test-flair-iword-format build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_no_center.img build/flair_data_iword_blk_cur_start.img build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_format_noctr,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOCTR_IMG),FLAIR_IWORD_FORMAT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_format_noctr.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_format_noctr.serial) && $(call IWORD_F_DISK,flair_iword_format_noctr) || { printf '!!! test-flair-iword-format-mutant FAIL: NO_CENTER went RED for the wrong reason (want: WORD up, FMT == MINT32)\n'; exit 1; }
@if $(call IWORD_F_REVEAL,$(BUILD)/flair_iword_format_noctr.serial); then printf '!!! test-flair-iword-format-mutant FAIL: NO_CENTER PASSED the REVEAL leg -- it is decoration\n'; exit 1; fi
@printf '>>> test-flair-iword-format-mutant: NO_CENTER correctly RED -- FMT == MINT32, but "Centered." was drawn at the left margin: %s\n' "$$(grep '^TENANT-GATE ax=0x0033 win=1 x=1 y=16 ' $(BUILD)/flair_iword_format_noctr.serial | tail -1 | cut -c1-80)"
$(call appl-boot,flair_iword_format_blkcur,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_BLKCUR_IMG),FLAIR_IWORD_FORMAT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_format_blkcur.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_format_blkcur.serial) && $(call IWORD_F_KEYS,$(BUILD)/flair_iword_format_blkcur.serial) || { printf '!!! test-flair-iword-format-mutant FAIL: BLK_CUR_START went RED for the wrong reason (want: WORD up, every prompt drawn)\n'; exit 1; }
@if $(call IWORD_F_DISK,flair_iword_format_blkcur); then printf '!!! test-flair-iword-format-mutant FAIL: BLK_CUR_START PASSED the DISK leg -- it is decoration\n'; exit 1; fi
@printf '>>> test-flair-iword-format-mutant: BLK_CUR_START correctly RED -- prompts drawn, FMT != MINT32 (%s)\n' "$$(cmp $(IWORD_MINT32) $(BUILD)/flair_iword_format_blkcur.WP5 2>&1 | head -1)"
@printf '>>> test-flair-iword-format-mutant: green (both mutants RED for the named reason)\n'
```

## test-flair-iword-listfiles

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 71):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_iword_lf.img build/test_iword_listfiles build/iword_lf_host/.stamp build/ppm_region_count build/.makefile.stamp`
```make
@$(call iword-lf-goldens,test-flair-iword-listfiles)
$(call appl-boot,flair_iword_lf,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_LF_DATA),FLAIR_IWORD_LISTFILES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_lf.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_lf.serial)
@free=$$($(call iword-lf-free,$(BUILD)/flair_iword_lf_data.img)); [ -n "$$free" ] || { printf 'RED-REASON listfiles-serial: no mdir free figure\n'; exit 1; }; \
 $(BUILD)/test_iword_listfiles "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) $(BUILD)/flair_iword_lf.serial $$free
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_lf.ppm,7 430 103 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON listfiles: no status-line pixels\n'; exit 1; }
@fsck.fat -n $(BUILD)/flair_iword_lf_data.img > /dev/null
@printf '>>> test-flair-iword-listfiles: green\n'
```

## test-flair-iword-listfiles-alias

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 191):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_iword_ops.img test-iword-listfiles-ops-goldens build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_alias,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_DATA),FLAIR_IWORD_LISTFILES_ALIAS_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_alias.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_alias.serial)
@grep -qF 's="New name: a:\ops.\alpha.txt' $(BUILD)/flair_iword_alias.serial
@$(call iword-ops-alias-kept,$(BUILD)/flair_iword_alias_data.img,ops-alias) || { printf 'RED-REASON listfiles-alias: same-file Move/Yes lost source bytes or changed active document\n'; exit 1; }
@! grep -E '^TENANT-GATE ax=0x008(0|6|9|a) path="OPS' $(BUILD)/flair_iword_alias.serial
@fsck.fat -n $(BUILD)/flair_iword_alias_data.img > /dev/null
@printf '>>> test-flair-iword-listfiles-alias: green (FAT12 trailing-dot alias keeps source bytes and active document)\n'
```

## test-flair-iword-listfiles-alias-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 209):
Prerequisites: `test-flair-iword-listfiles-alias build/flair_iword_ops_alias.img build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_alias_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_ALIAS_DATA),FLAIR_IWORD_LISTFILES_ALIAS_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_alias_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_alias_mut.serial)
@grep -qF 's="New name: a:\ops.\alpha.txt' $(BUILD)/flair_iword_alias_mut.serial
@if { $(call iword-ops-alias-kept,$(BUILD)/flair_iword_alias_mut_data.img,ops-alias-mut); } > $(BUILD)/iword-alias-mutant.log 2>&1; then printf 'FAIL: LF_ALIAS survived FAT12 emulator\n'; exit 1; fi
@grep -qF '::OPS/ALPHA.TXT' $(BUILD)/iword-alias-mutant.log
@printf '>>> test-flair-iword-listfiles-alias-mutant: green (LF_ALIAS loses the only copy)\n'
```

## test-flair-iword-listfiles-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 80):
Prerequisites: `test-flair-iword-listfiles build/flair_iword_lf_column.img build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_lf_column,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_LF_COLUMN_DATA),FLAIR_IWORD_LISTFILES_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_lf_column.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_lf_column.serial)
@free=$$($(call iword-lf-free,$(BUILD)/flair_iword_lf_column_data.img)); \
 if $(BUILD)/test_iword_listfiles "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) $(BUILD)/flair_iword_lf_column.serial $$free > $(BUILD)/iword_lf_column_mut.log 2>&1; then printf 'FAIL: LF_COLUMN survived the emulator gate\n'; exit 1; fi
@grep -F 'RED-REASON listfiles-serial:' $(BUILD)/iword_lf_column_mut.log
@printf '>>> test-flair-iword-listfiles-mutant: LF_COLUMN correctly RED: '; grep -m1 'FAIL ' $(BUILD)/iword_lf_column_mut.log
@printf '>>> test-flair-iword-listfiles-mutant: green\n'
```

## test-flair-iword-listfiles-ops

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 163):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_iword_ops.img build/flair_iword_ops_full.img build/test_iword_listfiles_ops build/ppm_region_count test-iword-listfiles-ops-goldens test-flair-iword-listfiles-alias build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_ops,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_DATA),FLAIR_IWORD_LISTFILES_OPS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_ops.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_ops.serial)
@$(BUILD)/test_iword_listfiles_ops --serial $(BUILD)/flair_iword_ops.serial $(IWORD_OPS_FIX)/grids
@$(call iword-ops-success,$(BUILD)/flair_iword_ops_data.img,ops-good) && $(call iword-ops-document,$(BUILD)/flair_iword_ops_data.img,ops-good) || { printf 'RED-REASON listfiles-ops: mtools success/document bytes\n'; exit 1; }
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_ops.ppm,7 62 103 78)); [ "$$1" -gt 10 ] || { printf 'RED-REASON listfiles-ops: active document pixels absent\n'; exit 1; }
@fsck.fat -n $(BUILD)/flair_iword_ops_data.img > /dev/null
$(call appl-boot,flair_iword_ops_full,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_FULL_DATA),FLAIR_IWORD_LISTFILES_OPS_FULL_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_ops_full.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_ops_full.serial)
@$(BUILD)/test_iword_listfiles_ops --full-serial $(BUILD)/flair_iword_ops_full.serial $(IWORD_OPS_FIX)/grids
@$(call iword-ops-kept,$(BUILD)/flair_iword_ops_full_data.img,ops-full) && $(call iword-ops-document,$(BUILD)/flair_iword_ops_full_data.img,ops-full) || { printf 'RED-REASON listfiles-ops: full-disk source/destination/document bytes\n'; exit 1; }
@fsck.fat -n $(BUILD)/flair_iword_ops_full_data.img > /dev/null
@printf '>>> test-flair-iword-listfiles-ops: green (mint prompts, pixels, mtools hashes, protected attributes, fsck, active document unchanged)\n'
```

## test-flair-iword-listfiles-ops-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 176):
Prerequisites: `test-flair-iword-listfiles-ops build/flair_iword_ops_truncate.img build/flair_iword_ops_wrong.img test-flair-iword-listfiles-alias-mutant build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_ops_wrong,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_WRONG_DATA),FLAIR_IWORD_LISTFILES_OPS_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_ops_wrong.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_ops_wrong.serial)
@if $(BUILD)/test_iword_listfiles_ops --serial $(BUILD)/flair_iword_ops_wrong.serial $(IWORD_OPS_FIX)/grids > $(BUILD)/ops-wrong.log 2>&1; then printf 'FAIL: LF_WRONG_PATH survived emulator\n'; exit 1; fi
@grep -qF 'RED-REASON listfiles-ops:' $(BUILD)/ops-wrong.log
$(call appl-boot,flair_iword_ops_truncate,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_IWORD_OPS_TRUNCATE_DATA),FLAIR_IWORD_LISTFILES_OPS_FULL_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_ops_truncate.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_ops_truncate.serial)
@if $(call iword-ops-kept,$(BUILD)/flair_iword_ops_truncate_data.img,ops-truncate); then printf 'FAIL: LF_TRUNCATE kept destination\n'; exit 1; fi
@printf '>>> test-flair-iword-listfiles-ops-mutant: green (wrong selected path and truncate-before-copy both RED)\n'
```

## test-flair-iword-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32387):
Prerequisites: `test-flair-iword build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_nowrap.img build/flair_data_iword_nosave.img build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_nowrap,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOWRAP_IMG),FLAIR_IWORD_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_nowrap.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_nowrap.serial) && mcopy -o -i $(BUILD)/flair_iword_nowrap_data.img ::MEMO $(BUILD)/iword-memo-nowrap.WP5 || { printf '!!! test-flair-iword-mutant FAIL: NO_WRAP went RED for the wrong reason (want: WORD up, MEMO saved, desktop alive)\n'; exit 1; }
@if $(call IWORD_TYPED,$(BUILD)/flair_iword_nowrap.serial); then printf '!!! test-flair-iword-mutant FAIL: NO_WRAP PASSED the TYPED leg -- it is decoration\n'; exit 1; fi
@python3 -I $(WORDPERFECT51_DECOMP)/tools/wp5_ref.py $(BUILD)/iword-memo-nowrap.WP5 > $(BUILD)/iword-memo-nowrap.ref
@if grep -qF '[SRt]' $(BUILD)/iword-memo-nowrap.ref; then printf '!!! test-flair-iword-mutant FAIL: NO_WRAP still saved a soft return\n'; exit 1; fi
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_nowrap.ppm,511 62 527 78)); [ "$$1" -gt 0 ] || { printf '!!! test-flair-iword-mutant FAIL: NO_WRAP left columns 63-64 of line 1 empty\n'; exit 1; }
@printf '>>> test-flair-iword-mutant: NO_WRAP correctly RED -- line 1 drawn "%s", MEMO has no [SRt] (wp5_ref.py), ink in columns 63-64\n' "$$(grep '^TENANT-GATE ax=0x0033 win=1 x=1 y=0 .* fg=0 bg=3 -> 0$$' $(BUILD)/flair_iword_nowrap.serial | tail -1 | cut -c40-110)"
$(call appl-boot,flair_iword_nosave,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_NOSAVE_IMG),FLAIR_IWORD_SPEC,)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_nosave.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_nosave.serial) && $(call IWORD_DREW,$(BUILD)/flair_iword_nosave.serial,0,$(IWORD_ROW0)) || { printf '!!! test-flair-iword-mutant FAIL: SAVE_NOTHING went RED for the wrong reason (want: WORD up, the paragraph typed and wrapped)\n'; exit 1; }
@mcopy -o -i $(BUILD)/flair_iword_nosave_data.img ::MEMO $(BUILD)/iword-memo-nosave.WP5 && [ "$$(wc -c < $(BUILD)/iword-memo-nosave.WP5)" = 0 ] || { printf '!!! test-flair-iword-mutant FAIL: SAVE_NOTHING did not leave an empty MEMO\n'; exit 1; }
@if $(call iword_ref_text,$(BUILD)/iword-memo-nosave.WP5,$(IWORD_MEMO_TEXT)$(IWORD_MEMO_TAIL)) 2>/dev/null; then printf '!!! test-flair-iword-mutant FAIL: SAVE_NOTHING PASSED the DISK leg -- it is decoration\n'; exit 1; fi
@if $(call IWORD_BACK,$(BUILD)/flair_iword_nosave.serial); then printf '!!! test-flair-iword-mutant FAIL: SAVE_NOTHING PASSED the BACK leg\n'; exit 1; fi
@$(call IWORD_DREW,$(BUILD)/flair_iword_nosave.serial,368,ERROR: Incompatible File Format) || { printf '!!! test-flair-iword-mutant FAIL: SAVE_NOTHING: the retrieve of the empty MEMO was not refused visibly\n'; exit 1; }
@printf '>>> test-flair-iword-mutant: SAVE_NOTHING correctly RED -- MEMO is 0 bytes (wp5_ref.py refuses it), the retrieve says ERROR: Incompatible File Format\n'
@printf '>>> test-flair-iword-mutant: green (both mutants RED for the named reason)\n'
```

## test-flair-iword-replace

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 24):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/test_iword_p32 build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_replace,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_REPLACE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_replace.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_replace.serial)
@$(BUILD)/test_iword_p32 replace-serial $(BUILD)/flair_iword_replace.serial
@mcopy -o -i $(BUILD)/flair_iword_replace_data.img ::REPLACE $(BUILD)/iword_replace_saved.WP5
@$(call iword_ref_text,$(BUILD)/iword_replace_saved.WP5,Cat fox)
@tail -c +328 $(BUILD)/iword_replace_saved.WP5 > $(BUILD)/iword_replace_saved.body
@printf 'Cat fox' > $(BUILD)/iword_replace_expected.body
@cmp $(BUILD)/iword_replace_expected.body $(BUILD)/iword_replace_saved.body
@fsck.fat -n $(BUILD)/flair_iword_replace_data.img
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_replace.ppm,7 62 63 78)); [ "$$1" -gt 10 ]
@printf '>>> test-flair-iword-replace: green\n'
```

## test-flair-iword-replace-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-replace-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-replace-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-replace_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-replace_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-replace_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-replace_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-replace_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-replace-bochs "$(BUILD)/flair-iword-replace_bochs.report.txt" "$(BUILD)/flair-iword-replace_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-replace-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 39):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_replace_confirm.img build/test_iword_p32 build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_replace_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_REPLACE_CONFIRM_IMG),FLAIR_IWORD_REPLACE_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_replace_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_replace_mut.serial)
@if $(BUILD)/test_iword_p32 replace-serial $(BUILD)/flair_iword_replace_mut.serial > $(BUILD)/iword_replace_mut.log 2>&1; then printf 'FAIL: REPLACE_CONFIRM survived\n'; exit 1; fi
@grep -F 'RED-REASON replace-serial:' $(BUILD)/iword_replace_mut.log
@printf '>>> test-flair-iword-replace-mutant: green (REPLACE_CONFIRM correctly RED)\n'
```

## test-flair-iword-search

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 79):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/test_iword_p3 build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_search,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_SEARCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_search.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_search.serial)
@$(BUILD)/test_iword_p3 search-serial $(BUILD)/flair_iword_search.serial
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_search.ppm,7 430 111 446)); [ "$$1" -gt 10 ] || { printf 'RED-REASON search: missing message pixels\n'; exit 1; }
@fsck.fat -n $(BUILD)/flair_iword_search_data.img
@printf '>>> test-flair-iword-search: green\n'
```

## test-flair-iword-search-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-search-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-search-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-search_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-search_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-search_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-search_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-search_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-search-bochs "$(BUILD)/flair-iword-search_bochs.report.txt" "$(BUILD)/flair-iword-search_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-search-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 101):
Prerequisites: `test-flair-iword-search test-flair-iword-search_back-mutant test-flair-iword-search_notfound-mutant build/.makefile.stamp`
```make
@printf '>>> test-flair-iword-search-mutant: green (both named mutants RED)\n'
```

## test-flair-iword-search_back-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 102):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/test_iword_p3 build/flair_data_iword_search_back.img build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_search_back,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_SEARCH_BACK_IMG),FLAIR_IWORD_SEARCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_search_back.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_search_back.serial)
@if $(BUILD)/test_iword_p3 search-serial $(BUILD)/flair_iword_search_back.serial > $(BUILD)/iword_search_back_mut.log 2>&1; then printf 'FAIL: SEARCH_BACK survived emulator gate\n'; exit 1; fi
@grep -F 'RED-REASON search-serial:' $(BUILD)/iword_search_back_mut.log
@printf '>>> test-flair-iword-search-mutant: SEARCH_BACK correctly RED: '; grep -m1 'FAIL search' $(BUILD)/iword_search_back_mut.log
@printf '>>> test-flair-iword-search_back-mutant: green (SEARCH_BACK RED)\n'
```

## test-flair-iword-search_notfound-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 103):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/test_iword_p3 build/flair_data_iword_search_notfound.img build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_search_notfound,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_SEARCH_NOTFOUND_IMG),FLAIR_IWORD_SEARCH_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_search_notfound.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_search_notfound.serial)
@if $(BUILD)/test_iword_p3 search-serial $(BUILD)/flair_iword_search_notfound.serial > $(BUILD)/iword_search_notfound_mut.log 2>&1; then printf 'FAIL: SEARCH_NOTFOUND survived emulator gate\n'; exit 1; fi
@grep -F 'RED-REASON search-serial:' $(BUILD)/iword_search_notfound_mut.log
@printf '>>> test-flair-iword-search-mutant: SEARCH_NOTFOUND correctly RED: '; grep -m1 'FAIL search' $(BUILD)/iword_search_notfound_mut.log
@printf '>>> test-flair-iword-search_notfound-mutant: green (SEARCH_NOTFOUND RED)\n'
```

## test-flair-iword-setup

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 111):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/test_iword_p32 build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_setup,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_SETUP_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_setup.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_setup.serial)
@$(BUILD)/test_iword_p32 setup-serial $(BUILD)/flair_iword_setup.serial
@mcopy -o -i $(BUILD)/flair_iword_setup_data.img ::IWORD.SET $(BUILD)/iword_setup_saved.SET
@printf 'IWS1cp\001\000' > $(BUILD)/iword_setup_expected.SET
@cmp $(BUILD)/iword_setup_expected.SET $(BUILD)/iword_setup_saved.SET
@fsck.fat -n $(BUILD)/flair_iword_setup_data.img
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_setup.ppm,7 62 175 78)); [ "$$1" -gt 10 ]
@printf '>>> test-flair-iword-setup: green\n'
```

## test-flair-iword-setup-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-setup-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-setup-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-setup_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-setup_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-setup_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-setup_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-setup_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-setup-bochs "$(BUILD)/flair-iword-setup_bochs.report.txt" "$(BUILD)/flair-iword-setup_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-setup-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 124):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_setup_load.img build/test_iword_p32 build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_setup_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_SETUP_LOAD_IMG),FLAIR_IWORD_SETUP_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_setup_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_setup_mut.serial)
@if $(BUILD)/test_iword_p32 setup-serial $(BUILD)/flair_iword_setup_mut.serial > $(BUILD)/iword_setup_mut.log 2>&1; then printf 'FAIL: SETUP_LOAD survived\n'; exit 1; fi
@grep -F 'RED-REASON setup-serial:' $(BUILD)/iword_setup_mut.log
@printf '>>> test-flair-iword-setup-mutant: green (SETUP_LOAD correctly RED)\n'
```

## test-flair-iword-undo

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 65):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/test_iword_p32 build/ppm_region_count build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_undo,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo.serial)
@$(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo.serial
@mcopy -o -i $(BUILD)/flair_iword_undo_data.img ::UNDO $(BUILD)/iword_undo_saved.WP5
@$(call iword_ref_text,$(BUILD)/iword_undo_saved.WP5,c)
@mcopy -o -i $(BUILD)/flair_iword_undo_data.img ::MOTION $(BUILD)/iword_motion_saved.WP5
@$(call iword_ref_text,$(BUILD)/iword_motion_saved.WP5,abc)
@fsck.fat -n $(BUILD)/flair_iword_undo_data.img
@set -- $$($(call IWORD_PX,$(BUILD)/flair_iword_undo.ppm,7 62 23 78)); [ "$$1" -gt 2 ]
@printf '>>> test-flair-iword-undo: green\n'
```

## test-flair-iword-undo-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 255):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-iword-undo-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_iword_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-iword-undo-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-iword-undo_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_iword_gate.img" --disk2 "$(BUILD)/flair-iword-undo_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-iword-undo_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-iword-undo_bochs.report.txt" || true
@cat "$(BUILD)/flair-iword-undo_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-iword-undo-bochs "$(BUILD)/flair-iword-undo_bochs.report.txt" "$(BUILD)/flair-iword-undo_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-iword-undo-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 82):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data_iword_undo_level.img build/flair_data_iword_undo_movement.img build/test_iword_p32 build/.makefile.stamp`
```make
$(call appl-boot,flair_iword_undo_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_UNDO_LEVEL_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo_mut.serial)
@if $(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo_mut.serial > $(BUILD)/iword_undo_mut.log 2>&1; then printf 'FAIL: UNDO_LEVEL survived\n'; exit 1; fi
@grep -F 'RED-REASON undo-serial:' $(BUILD)/iword_undo_mut.log
@printf '>>> test-flair-iword-undo-mutant: UNDO_LEVEL correctly RED\n'
$(call appl-boot,flair_iword_undo_movement_mut,$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IWORD_UNDO_MOVEMENT_IMG),FLAIR_IWORD_UNDO_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_iword_undo_movement_mut.serial) && $(call IWORD_UP,$(BUILD)/flair_iword_undo_movement_mut.serial)
@if $(BUILD)/test_iword_p32 undo-serial $(BUILD)/flair_iword_undo_movement_mut.serial > $(BUILD)/iword_undo_movement_mut.log 2>&1; then printf 'FAIL: UNDO_MOVEMENT survived\n'; exit 1; fi
@grep -F 'FAIL undo emulator Search cursor motion' $(BUILD)/iword_undo_movement_mut.log
@printf '>>> test-flair-iword-undo-mutant: green (UNDO_LEVEL and UNDO_MOVEMENT correctly RED)\n'
```

## test-iword-codes

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15464):
Prerequisites: `build/test_iword_codes build/.makefile.stamp`
```make
@printf '>>> test-iword-codes: InitechWord attribute codes vs the WordPerfect 5.1 rules -- model, units, scopes, wrap, state (initech-jojk)\n'
@$(TEST_IWORD_CODES)
@printf '>>> test-iword-codes: green\n'
```

## test-iword-codes-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15470):
Prerequisites: `build/test_iword_codes_mutant_IWORD_MUT_TOGGLE_DUP build/test_iword_codes_mutant_IWORD_MUT_PAIR_HALF build/test_iword_codes_mutant_IWORD_MUT_NO_ASK build/test_iword_codes_mutant_IWORD_MUT_ASK_STAY build/test_iword_codes_mutant_IWORD_MUT_TAB_FIXED build/test_iword_codes_mutant_IWORD_MUT_COPY_BARE build/test_iword_codes_mutant_IWORD_MUT_CUT_NO_MEND build/test_iword_codes_mutant_IWORD_MUT_RETRIEVE_DUP build/test_iword_codes_mutant_IWORD_MUT_TAB_ABS build/test_iword_codes_mutant_IWORD_MUT_NO_LEAD build/test_iword_codes_mutant_IWORD_MUT_OLD_STALE build/test_iword_codes_mutant_IWORD_MUT_WRAP_65 build/.makefile.stamp`
```make
@for p in $(IWORD_CODES_MUTANTS); do pair="IWORD_MUT_$${p%%:*}:RED-REASON $${p#*:}"; \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_codes_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-iword-codes-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-iword-codes-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-iword-codes-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-iword-codes-mutant: %s correctly RED (%s; %s)\n' "$$m" "$$(grep -m1 'FAIL' $$bin.log | sed 's/^ *//')" "$$(grep -m1 '      C' $$bin.log | sed 's/^ *//')"; \
done
@printf '>>> test-iword-codes-mutant: green (all thirteen mutants RED for the named reason)\n'
```

## test-iword-core

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15365):
Prerequisites: `build/test_iword_core build/.makefile.stamp`
```make
@printf '>>> test-iword-core: the InitechWord piece table, wrap, typing and cursor properties (initech-l4tj)\n'
@$(TEST_IWORD_CORE)
@printf '>>> test-iword-core: green\n'
```

## test-iword-core-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15370):
Prerequisites: `build/test_iword_core_mutant_IWORD_MUT_SPLIT_OFF build/test_iword_core_mutant_IWORD_MUT_REFLOW_STALE build/test_iword_core_mutant_IWORD_MUT_PREV_VAR build/.makefile.stamp`
```make
@for pair in 'IWORD_MUT_SPLIT_OFF:RED-REASON model' 'IWORD_MUT_REFLOW_STALE:RED-REASON reflow' 'IWORD_MUT_PREV_VAR:RED-REASON units'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_core_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-iword-core-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-iword-core-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-iword-core-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-iword-core-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -F "$$why" $$bin.log)"; \
done
@printf '>>> test-iword-core-mutant: green (all three mutants RED for the named reason)\n'
```

## test-iword-face

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15565):
Prerequisites: `build/test_iword_face build/test_iword_face_small build/.makefile.stamp`
```make
$(call need_wp_goldens,test-iword-face)
@printf '>>> test-iword-face: WORD.EXE Face A over a recording Toolbox mock -- screen, prompts, exit guard, files (initech-l4tj)\n'
@rm -rf $(IWORD_FACE_FS) $(IWORD_FACE_FS)_small && mkdir -p $(IWORD_FACE_FS) $(IWORD_FACE_FS)_small
@$(TEST_IWORD_FACE) $(IWORD_FACE_FS)
@$(BUILD)/test_iword_face_small $(IWORD_FACE_FS)_small
@head -c 327 $(WORDPERFECT51_DECOMP)/goldens/minted/MINT02.WP5 > $(BUILD)/iword_face_memo.expect
@printf '%s\r%s' '$(IWORD_MEMO_TEXT)' '$(IWORD_MEMO_TAIL)' >> $(BUILD)/iword_face_memo.expect
@cmp $(BUILD)/iword_face_memo.expect $(IWORD_FACE_FS)/MEMO || { printf '!!! test-iword-face FAIL: MEMO is not the real MINT02 prefix + the wrapped paragraph\n'; exit 1; }
@$(call iword_ref_text,$(IWORD_FACE_FS)/MEMO,$(IWORD_MEMO_TEXT)$(IWORD_MEMO_TAIL)) && grep -qF "T'$(IWORD_MEMO_TEXT)' [SRt] T'$(IWORD_MEMO_TAIL)'" $(IWORD_FACE_FS)/MEMO.ref || { printf '!!! test-iword-face FAIL: wp5_ref.py does not read MEMO as the wrapped paragraph\n'; cat $(IWORD_FACE_FS)/MEMO.ref; exit 1; }
@$(call iword_ref_text,$(IWORD_FACE_FS)/COPY,X$(IWORD_MEMO_TEXT)$(IWORD_MEMO_TAIL)) || { printf '!!! test-iword-face FAIL: COPY (Save, edit, Replace) per wp5_ref.py\n'; cat $(IWORD_FACE_FS)/COPY.ref; exit 1; }
@$(call iword_ref_text,$(IWORD_FACE_FS)/GUARD,Keep me) || { printf '!!! test-iword-face FAIL: GUARD (saved from the exit guard) per wp5_ref.py\n'; cat $(IWORD_FACE_FS)/GUARD.ref; exit 1; }
@python3 -I $(WORDPERFECT51_DECOMP)/tools/wp5_ref.py $(IWORD_FACE_FS)_small/BIG | grep -q 'END offset=4096 filesize=4096 trailer-errors=0$$' || { printf '!!! test-iword-face FAIL: BIG (the compaction leg) does not parse clean\n'; exit 1; }
@$(call iword_ref_text,$(IWORD_FACE_FS)/CODES,Plain bold under slant xy) && grep -qF "$(IWORD_CODES_REF)" $(IWORD_FACE_FS)/CODES.ref || { printf '!!! test-iword-face FAIL: wp5_ref.py does not read CODES (leg F7) as the attribute pairs typed\n'; cat $(IWORD_FACE_FS)/CODES.ref; exit 1; }
@tail -c +328 $(IWORD_FACE_FS)/TABS > $(BUILD)/iword_face_tabs.body && tail -c +328 $(IWORD_MINT09) > $(BUILD)/iword_face_tabs.want && cmp $(BUILD)/iword_face_tabs.want $(BUILD)/iword_face_tabs.body || { printf '!!! test-iword-face FAIL: the body of TABS (leg F10, the keys of mint/wpMINT09.steps) is not the body WP.EXE saved in MINT09\n'; exit 1; }
@python3 -I $(WORDPERFECT51_DECOMP)/tools/wp5_ref.py $(IWORD_FACE_FS)/TABS | grep -q 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$$' || { printf '!!! test-iword-face FAIL: TABS does not parse clean\n'; exit 1; }
@printf '>>> test-iword-face: green (MEMO byte-identical to the expected file; MEMO/COPY/GUARD/BIG/CODES/TABS read clean by wp5_ref.py; CODES carries bold/underline/italic pairs; the body of TABS == MINT09 (%s bytes))\n' "$$(wc -c < $(BUILD)/iword_face_tabs.body)"
```

## test-iword-face-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15583):
Prerequisites: `build/test_iword_face_mutant_IWORD_MUT_NO_WRAP build/test_iword_face_mutant_IWORD_MUT_SAVE_NOTHING build/test_iword_face_mutant_IWORD_MUT_NO_FACE build/test_iword_face_mutant_IWORD_MUT_NO_BLOCK_ON build/test_iword_face_mutant_IWORD_MUT_NO_CENTER build/test_iword_face_mutant_IWORD_MUT_SP_CEIL build/test_iword_face_mutant_IWORD_MUT_LINE_SCROLL build/test_iword_face_mutant_IWORD_MUT_PGDN_IGNORED build/test_iword_face_mutant_IWORD_MUT_GOTO_TYPES build/test_iword_face_mutant_IWORD_MUT_NAME_SPACE_DROP build/.makefile.stamp`
```make
@for pair in 'IWORD_MUT_NO_WRAP:FAIL F1 the paragraph wraps' 'IWORD_MUT_SAVE_NOTHING:FAIL F1 MEMO holds' 'IWORD_MUT_NO_FACE:FAIL F7 faces' 'IWORD_MUT_NO_BLOCK_ON:FAIL F8 Alt-F4: Block on' 'IWORD_MUT_NO_CENTER:FAIL F11 a centered line' 'IWORD_MUT_SP_CEIL:FAIL F12 1.33 takes one row' 'IWORD_MUT_LINE_SCROLL:FAIL F12 triple spacing scrolled by rows' 'IWORD_MUT_PGDN_IGNORED:FAIL F16 PgDn: the first line of page 2' 'IWORD_MUT_GOTO_TYPES:FAIL F16 Go To digits' 'IWORD_MUT_NAME_SPACE_DROP:FAIL F17 the space shows as typed'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_face_mutant_$$m; dir=$(BUILD)/iword_face_fs_$$m; \
	rm -rf $$dir && mkdir -p $$dir; \
	if $$bin $$dir > $$bin.log 2>&1; then printf '!!! test-iword-face-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-iword-face-mutant FAIL: %s died without a summary\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-iword-face-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-iword-face-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 -F "$$why" $$bin.log)"; \
done
@printf '>>> test-iword-face-mutant: green (all ten mutants RED for the named reason)\n'
```

## test-iword-fkeys

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 17):
Prerequisites: `build/test_iword_p3 build/.makefile.stamp`
```make
@$< fkeys
@$(BUILD)/iword_fkeys_gen spec/iword/fkeys.json > $(BUILD)/iword_fkeys.expected.h
@cmp spec/iword/fkeys.h $(BUILD)/iword_fkeys.expected.h
@printf '>>> test-iword-fkeys: green\n'
```

## test-iword-fkeys-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 22):
Prerequisites: `build/test_iword_p3_mut_FKEY_DROP build/test_iword_p3_mut_FKEY_UNMAPPED build/.makefile.stamp`
```make
@if $< fkeys > $<.log 2>&1; then printf 'FAIL: FKEY_DROP survived\n'; exit 1; fi
@grep -F 'RED-REASON fkeys:' $<.log
@if $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED fkeys > $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED.log 2>&1; then printf 'FAIL: FKEY_UNMAPPED survived\n'; exit 1; fi
@grep -F 'RED-REASON fkeys:' $(BUILD)/test_iword_p3_mut_FKEY_UNMAPPED.log
@printf '>>> test-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)\n'
```

## test-iword-listfiles

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 24):
Prerequisites: `build/test_iword_listfiles build/iword_lf_host/.stamp test-iword-listfiles-ops build/.makefile.stamp`
```make
@$(call iword-lf-goldens,test-iword-listfiles)
@$< "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST)
@printf '>>> test-iword-listfiles: green\n'
```

## test-iword-listfiles-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 29):
Prerequisites: `test-iword-listfiles build/test_iword_listfiles_mut_LF_COLUMN build/test_iword_listfiles_mut_LF_ZERO_DATE test-iword-listfiles-ops-mutant build/.makefile.stamp`
```make
@for m in $(IWORD_LF_MUTANTS); do b=$(BUILD)/test_iword_listfiles_mut_$$m; \
 if $$b "$(IWORD_LF_MINT)/grids" $(IWORD_LF_HOST) > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON listfiles:' $$b.log || exit 1; \
 printf '>>> test-iword-listfiles-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL ' $$b.log)"; done
@printf '>>> test-iword-listfiles-mutant: green\n'
```

## test-iword-listfiles-ops

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 99):
Prerequisites: `build/test_iword_listfiles_ops test-iword-listfiles-ops-goldens build/.makefile.stamp`
```make
@d=$$(mktemp -d $(BUILD)/iword_ops.XXXXXX); $< $$d fixtures/iword-listfiles-ops/grids
```

## test-iword-listfiles-ops-goldens

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 97):
Prerequisites: `build/.makefile.stamp`
```make
@cd fixtures/iword-listfiles-ops && sha256sum -c --quiet SHA256SUMS
```

## test-iword-listfiles-ops-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/wp5_diff/p3_listfiles.mk', line 106):
Prerequisites: `test-iword-listfiles-ops build/test_iword_listfiles_ops_mut_LF_WRONG_PATH build/test_iword_listfiles_ops_mut_LF_PROTECT build/test_iword_listfiles_ops_mut_LF_TRUNCATE build/test_iword_listfiles_ops_mut_LF_SELECTION build/test_iword_listfiles_ops_mut_LF_ALIAS build/.makefile.stamp`
```make
@set -e; for m in $(IWORD_LF_OPS_MUTANTS); do d=$$(mktemp -d $(BUILD)/iword_ops_mut.XXXXXX); b=$(BUILD)/test_iword_listfiles_ops_mut_$$m; \
 if $$b $$d fixtures/iword-listfiles-ops/grids > $$b.log 2>&1; then printf 'FAIL: %s survived\n' $$m; exit 1; fi; \
 grep -qF 'RED-REASON listfiles-ops:' $$b.log; printf '>>> listfiles-ops mutant %s correctly RED: ' $$m; grep -m1 'FAIL ' $$b.log; done
```

## test-iword-replace

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 9):
Prerequisites: `build/test_iword_p32 build/fat_dump build/.makefile.stamp`
```make
@$(BUILD)/test_iword_p32 replace
@sh harness/diff/wp5_diff/replace.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_replace $(FAT_DUMP_BIN) $(BUILD)/test_iword_p32
@printf '>>> test-iword-replace: green\n'
```

## test-iword-replace-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 14):
Prerequisites: `test-iword-replace build/test_iword_p32_mut_REPLACE_CASE build/test_iword_p32_mut_REPLACE_SKIP build/test_iword_p32_mut_REPLACE_CODE build/test_iword_p32_mut_REPLACE_CONFIRM build/test_iword_p32_mut_REPLACE_PROMPT build/.makefile.stamp`
```make
@for m in $(IWORD_REPLACE_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; \
 if $$b replace > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON replace:' $$b.log || exit 1; \
 printf '>>> test-iword-replace-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL replace' $$b.log)"; done
@$(FAT_DUMP_BIN) $(BUILD)/iword_replace/search.img --cat M06.WP5 > $(BUILD)/iword_replace/M06.WP5
@if $(BUILD)/test_iword_p32_mut_REPLACE_CODE replace-corpus $(BUILD)/iword_replace/M06.WP5 > $(BUILD)/iword_replace_code_corpus_mut.log 2>&1; then printf 'FAIL: corpus let REPLACE_CODE survive\n'; exit 1; fi
@grep -F 'RED-REASON replace-corpus:' $(BUILD)/iword_replace_code_corpus_mut.log
@printf '>>> test-iword-replace-mutant: real MINT06 REPLACE_CODE correctly RED: '; grep -m1 'FAIL replace-corpus' $(BUILD)/iword_replace_code_corpus_mut.log
@printf '>>> test-iword-replace-mutant: green\n'
```

## test-iword-reveal-codes

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15492):
Prerequisites: `build/test_iword_reveal_codes build/.makefile.stamp`
```make
$(call need_wp_goldens,test-iword-reveal-codes)
@printf '>>> test-iword-reveal-codes: the Reveal Codes pane vs 69 real WordPerfect 5.1 Reveal Codes screens (initech-jojk)\n'
@$(TEST_IWORD_REVEAL) $(WORDPERFECT51_DECOMP)
@printf '>>> test-iword-reveal-codes: green\n'
```

## test-iword-reveal-codes-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15498):
Prerequisites: `build/test_iword_reveal_mutant_IWORD_MUT_LABEL_ITALC build/test_iword_reveal_mutant_IWORD_MUT_PANE_ANCHOR build/test_iword_reveal_mutant_IWORD_MUT_BLOCK_LABEL build/test_iword_reveal_mutant_IWORD_MUT_LABEL_JUST build/test_iword_reveal_mutant_IWORD_MUT_HHH_AFTER build/test_iword_reveal_mutant_IWORD_MUT_BLK_CUR_START build/test_iword_reveal_mutant_IWORD_MUT_LABEL_SM_CAP build/.makefile.stamp`
```make
$(call need_wp_goldens,test-iword-reveal-codes-mutant)
@for pair in 'IWORD_MUT_LABEL_ITALC:RED-REASON label' 'IWORD_MUT_PANE_ANCHOR:RED-REASON pane' 'IWORD_MUT_BLOCK_LABEL:RED-REASON label' 'IWORD_MUT_LABEL_JUST:RED-REASON label' 'IWORD_MUT_HHH_AFTER:RED-REASON pane' 'IWORD_MUT_BLK_CUR_START:RED-REASON pane' 'IWORD_MUT_LABEL_SM_CAP:RED-REASON label'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_reveal_mutant_$$m; \
	if $$bin $(WORDPERFECT51_DECOMP) > $$bin.log 2>&1; then printf '!!! test-iword-reveal-codes-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-iword-reveal-codes-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-iword-reveal-codes-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-iword-reveal-codes-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 'FAIL' $$bin.log | sed 's/^ *//')"; \
done
@printf '>>> test-iword-reveal-codes-mutant: green (all seven mutants RED for the named reason)\n'
```

## test-iword-search

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 55):
Prerequisites: `build/test_iword_p3 build/test_iword_search_corpus build/fat_dump build/.makefile.stamp`
```make
@$< search
@sh harness/diff/wp5_diff/search.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_search $(FAT_DUMP_BIN) $(BUILD)/test_iword_search_corpus
@printf '>>> test-iword-search: green\n'
```

## test-iword-search-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 67):
Prerequisites: `test-iword-search build/test_iword_p3_mut_SEARCH_BACK build/test_iword_p3_mut_SEARCH_CASE build/test_iword_p3_mut_SEARCH_CODE build/test_iword_p3_mut_SEARCH_WRITE build/test_iword_p3_mut_SEARCH_PROMPT build/test_iword_p3_mut_SEARCH_CANCEL build/test_iword_p3_mut_SEARCH_NOTFOUND build/test_iword_p3_mut_SEARCH_CURSOR build/test_iword_search_corpus_mut_SEARCH_BACK build/test_iword_search_corpus_mut_SEARCH_CASE build/test_iword_search_corpus_mut_SEARCH_CODE build/test_iword_search_corpus_mut_SEARCH_WRITE build/.makefile.stamp`
```make
@for m in $(IWORD_SEARCH_MUTANTS); do b=$(BUILD)/test_iword_p3_mut_$$m; \
	if $$b search > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
	grep -F 'RED-REASON search:' $$b.log || exit 1; \
	printf '>>> test-iword-search-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL search' $$b.log)"; \
done
@for m in $(IWORD_SEARCH_CORPUS_MUTANTS); do b=$(BUILD)/test_iword_search_corpus_mut_$$m; \
	if $$b $(BUILD)/iword_search/milton.wp5 > $$b.log 2>&1; then printf 'FAIL: corpus oracle let %s survive\n' "$$m"; exit 1; fi; \
	grep -F 'RED-REASON search-corpus:' $$b.log || exit 1; \
	printf '>>> test-iword-search-mutant: corpus %s correctly RED\n' "$$m"; \
done
@printf '>>> test-iword-search-mutant: green\n'
```

## test-iword-setup

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 97):
Prerequisites: `build/test_iword_p32 build/fat_dump build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/iword_setup_fs
@rm -f $(BUILD)/iword_setup_fs/IWORD.SET
@$< setup $(BUILD)/iword_setup_fs
@sh harness/diff/wp5_diff/settings.sh $(BUILD)/iword_setup_fs $(FAT_DUMP_BIN)
@printf '>>> test-iword-setup: green\n'
```

## test-iword-setup-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 104):
Prerequisites: `build/test_iword_p32_mut_SETUP_SAVE build/test_iword_p32_mut_SETUP_LOAD build/test_iword_p32_mut_SETUP_VALIDATE build/test_iword_p32_mut_SETUP_SHORT build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/iword_setup_mut_fs
@for m in $(IWORD_SETUP_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; rm -f $(BUILD)/iword_setup_mut_fs/IWORD.SET; \
 if $$b setup $(BUILD)/iword_setup_mut_fs > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON setup:' $$b.log || exit 1; \
 printf '>>> test-iword-setup-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL setup' $$b.log)"; done
@printf '>>> test-iword-setup-mutant: green\n'
```

## test-iword-undo

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 49):
Prerequisites: `build/test_iword_p32 build/fat_dump build/.makefile.stamp`
```make
@$< undo
@sh harness/diff/wp5_diff/replace.sh $(WORDPERFECT51_DECOMP) $(BUILD)/iword_undo $(FAT_DUMP_BIN) $(BUILD)/test_iword_p32 undo-corpus
@printf '>>> test-iword-undo: green\n'
```

## test-iword-undo-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/wp5_diff/p3_part2.mk', line 55):
Prerequisites: `test-iword-undo build/test_iword_p32_mut_UNDO_FIXUP build/test_iword_p32_mut_UNDO_LEVEL build/test_iword_p32_mut_UNDO_ORDER build/test_iword_p32_mut_UNDO_CANCEL build/test_iword_p32_mut_UNDO_PREVIEW build/test_iword_p32_mut_UNDO_BLOCK build/test_iword_p32_mut_UNDO_MOVEMENT build/.makefile.stamp`
```make
@for m in $(IWORD_UNDO_MUTANTS); do b=$(BUILD)/test_iword_p32_mut_$$m; \
 if $$b undo > $$b.log 2>&1; then printf 'FAIL: %s survived\n' "$$m"; exit 1; fi; \
 grep -q 'checks, .* failures' $$b.log && grep -F 'RED-REASON undo:' $$b.log || exit 1; \
 printf '>>> test-iword-undo-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 'FAIL undo' $$b.log)"; done
@$(FAT_DUMP_BIN) $(BUILD)/iword_undo/search.img --cat M09.WP5 > $(BUILD)/iword_undo/M09.WP5
@if $(BUILD)/test_iword_p32_mut_UNDO_FIXUP undo-corpus $(BUILD)/iword_undo/M09.WP5 > $(BUILD)/iword_undo_fixup_mut.log 2>&1; then printf 'FAIL: UNDO_FIXUP survived\n'; exit 1; fi
@grep -F 'RED-REASON undo-corpus:' $(BUILD)/iword_undo_fixup_mut.log
@printf '>>> test-iword-undo-mutant: UNDO_FIXUP correctly RED: '; grep -m1 'undo-corpus byte diff:' $(BUILD)/iword_undo_fixup_mut.log
@printf '>>> test-iword-undo-mutant: green\n'
```

## test-iword-wp5-roundtrip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15398):
Prerequisites: `build/test_iword_wp5_roundtrip build/.makefile.stamp`
```make
$(call need_wp_goldens,test-iword-wp5-roundtrip)
@printf '>>> test-iword-wp5-roundtrip: the .WP5 codec + core vs the 31 real WordPerfect 5.1 goldens and wp5_ref.py (initech-l4tj, initech-jojk)\n'
@mkdir -p $(BUILD)/iword_wp5
@$(TEST_IWORD_WP5) $(WORDPERFECT51_DECOMP) $(BUILD)/iword_wp5
@printf '>>> test-iword-wp5-roundtrip: green\n'
```

## test-iword-wp5-roundtrip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15406):
Prerequisites: `build/test_iword_wp5_mutant_IWORD_MUT_DROP_CODE build/test_iword_wp5_mutant_IWORD_MUT_WRAP_EARLY build/test_iword_wp5_mutant_IWORD_MUT_CURSOR_FLAG build/test_iword_wp5_mutant_IWORD_MUT_NO_D4 build/test_iword_wp5_mutant_IWORD_MUT_WRAP_65 build/test_iword_wp5_mutant_IWORD_MUT_TAB_ABS build/test_iword_wp5_mutant_IWORD_MUT_MAR_NO_HRT build/test_iword_wp5_mutant_IWORD_MUT_NO_LEAD build/test_iword_wp5_mutant_IWORD_MUT_OLD_STALE build/test_iword_wp5_mutant_IWORD_MUT_SP_HRT build/test_iword_wp5_mutant_IWORD_MUT_SP_NEAR build/test_iword_wp5_mutant_IWORD_MUT_SENT_BARE build/.makefile.stamp`
```make
$(call need_wp_goldens,test-iword-wp5-roundtrip-mutant)
@mkdir -p $(BUILD)/iword_wp5_mut
@for pair in 'IWORD_MUT_DROP_CODE:RED-REASON golden-rewrite' 'IWORD_MUT_WRAP_EARLY:RED-REASON typed' 'IWORD_MUT_CURSOR_FLAG:RED-REASON typed' 'IWORD_MUT_NO_D4:RED-REASON typed' $(foreach m,$(IWORD_WP5_S3_MUTANTS),'IWORD_MUT_$(m):RED-REASON typed'); do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_iword_wp5_mutant_$$m; \
	if $$bin $(WORDPERFECT51_DECOMP) $(BUILD)/iword_wp5_mut > $$bin.log 2>&1; then printf '!!! test-iword-wp5-roundtrip-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-iword-wp5-roundtrip-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; cat $$bin.log; exit 1; }; \
	grep -qF "$$why" $$bin.log || { printf '!!! test-iword-wp5-roundtrip-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-iword-wp5-roundtrip-mutant: %s correctly RED (%s; %s)\n' "$$m" "$$(grep -F "$$why" $$bin.log)" "$$(grep -m1 'FAIL typed' $$bin.log | sed 's/^ *//')"; \
done
@printf '>>> test-iword-wp5-roundtrip-mutant: green (all thirteen mutants RED for the named reason)\n'
```

## test-pubs-iword

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 606):
Prerequisites: `test-pubs-build build/pubs_iword_probe build/pubs_gate build/.makefile.stamp`
```make
@sh harness/pubs/iword_check.sh check $(PUBS_IWORD_PROBE) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_IWORD_ALLOW)
```

## test-pubs-iword-flair-screens

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 610):
Prerequisites: `test-pubs-build build/qemu_harness build/flair_tenants_mut_iword_gate.img build/flair_data.img build/pubs_ppm_grey_png build/pubs_gate build/.makefile.stamp`
```make
@set -e; n=0; for m in $(PUBS_IWORD_BOOKS); do d=$(PUBS_DIR)/manuals/$$m/flair; f=fixtures/pubs/$$m; [ -f $$d/FLAIRSHOTS ] || continue; \
	sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(FLAIR_IWORD_GATE_IMG) $(FLAIR_DATA_IMG) $(PUBS_PPM_GREY) \
		$$d/FLAIRSHOTS $(PUBS_BUILD)/flair-check/$$m spec/flair_iword_traces.mk FLAIR_IWORD_TYPE_SPEC; \
	for p in $(PUBS_BUILD)/flair-check/$$m/*.png; do \
		$(PUBS_GATE) same SCREEN-DRIFT $$f/$$(basename $$p) $$p; n=$$((n + 1)); done; \
	for c in $$f/*.png; do [ -f $(PUBS_BUILD)/flair-check/$$m/$$(basename $$c) ] || { \
		printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s has no FLAIRSHOTS row\n' "$$c"; exit 1; }; \
		grep -qxF "$$(basename $$c .png)" $(PUBS_BUILD)/gate/$$m/main.flairshots || { \
		printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$$c"; exit 1; }; done; \
	while IFS= read -r x; do [ -f $$f/$$x.png ] || { printf '!!! test-pubs RED SCREEN-PAIR: %s/%s.png is printed but not committed\n' "$$f" "$$x"; exit 1; }; \
		done < $(PUBS_BUILD)/gate/$$m/main.flairshots; done; \
printf '>>> test-pubs-iword-flair-screens: green (%s FLAIR figures in the InitechWord manuals == the booted desktop, pixel for pixel)\n' "$$n"
```

## test-pubs-iword-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 608):
Prerequisites: `test-pubs-iword build/pubs_iword_probe build/pubs_iword_probe_noblk build/pubs_iword_probe_fkdrop build/pubs_gate build/.makefile.stamp`
```make
@sh harness/pubs/iword_mutants.sh $(PUBS_IWORD_PROBE) $(BUILD)/pubs_iword_probe_noblk $(BUILD)/pubs_iword_probe_fkdrop $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_IWORD_ALLOW)
```

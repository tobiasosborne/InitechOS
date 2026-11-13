## test-cpanels

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34630):
Prerequisites: `build/test_cpanels build/.makefile.stamp`
```make
@$< "$(SYSTEM7_DECOMP)"
```

## test-cpanels-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34632):
Prerequisites: `test-cpanels build/test_cpanels_mut_CP_MUT_OK_NOTHING build/test_cpanels_mut_CP_MUT_CANCEL_WRITES build/test_cpanels_mut_CP_MUT_CLOCK_NOT_APPLIED build/test_cpanels_mut_CP_TEST_MUT_NO_RELEASE build/.makefile.stamp`
```make
@set -e; for pair in CP_MUT_OK_NOTHING:OK_WRITES CP_MUT_CANCEL_WRITES:CANCEL_WRITES CP_MUT_CLOCK_NOT_APPLIED:CLOCK_APPLIED CP_TEST_MUT_NO_RELEASE:CLICK_RELEASE; do \
 m=$${pair%:*}; rule=$${pair#*:}; if $(BUILD)/test_cpanels_mut_$$m "$(SYSTEM7_DECOMP)" > $(BUILD)/cp_$$m.log; then printf 'FAIL mutant survived %s\n' $$m; exit 1; fi; \
 grep -q "^FAIL $$rule" $(BUILD)/cp_$$m.log; printf '>>> test-cpanels-mutant: %s correctly RED: %s\n' $$m "$$(grep -m1 "^FAIL $$rule" $(BUILD)/cp_$$m.log)"; done
```

## test-flair-cpanels

Declared tier: outside active vector; 
Prerequisites: `test-flair-cpanels-date test-flair-cpanels-patterns test-flair-cpanels-mouse test-flair-cpanels-sound test-flair-cpanels-memory build/.makefile.stamp`
```make
```

## test-flair-cpanels-bars-hide

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 58):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_hide,$(CP_BARS_IMG),CP_BARS_HIDE)
@:
@:
@build/ppm_flair_cpanels_bars_check build/cp_bars_hide.ppm fixtures/flair-cpanels-bars/cpanels.png hidden
@:
@grep -qx 'CPANELS-RTC hour=0 minute=56' build/cp_bars_hide.serial
@:
@:
@:
@printf '>>> test-flair-cpanels-bars-hide: green\n'
```

## test-flair-cpanels-bars-hide-exit

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 59):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_hide-exit,$(CP_BARS_IMG),CP_BARS_HIDE_EXIT)
@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' build/cp_bars_hide-exit.serial
@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' build/cp_bars_hide-exit.serial && grep -qx 'APPLE-ABOUT closed' build/cp_bars_hide-exit.serial
@build/ppm_flair_cpanels_bars_check build/cp_bars_hide-exit.ppm fixtures/flair-cpanels-bars/hello.png hidden
@:
@grep -qx 'CPANELS-RTC hour=0 minute=56' build/cp_bars_hide-exit.serial
@:
@:
@:
@printf '>>> test-flair-cpanels-bars-hide-exit: green\n'
```

## test-flair-cpanels-bars-mouse-exit

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 61):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_mouse-exit,$(CP_BARS_IMG),CP_BARS_MOUSE_EXIT)
@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' build/cp_bars_mouse-exit.serial
@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' build/cp_bars_mouse-exit.serial && grep -qx 'APPLE-ABOUT closed' build/cp_bars_mouse-exit.serial
@build/ppm_flair_cpanels_bars_check build/cp_bars_mouse-exit.ppm fixtures/flair-cpanels-bars/hello.png shown
@:
@:
@:
@grep -qx 'CPANELS-APPLY pattern=0 tracking=2 double=2 ticks=10' build/cp_bars_mouse-exit.serial
@:
@printf '>>> test-flair-cpanels-bars-mouse-exit: green\n'
```

## test-flair-cpanels-bars-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 69):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_no_system_bar.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_no_system_bar,$(BUILD)/flair_tenants_mut_cp_no_system_bar.img,CP_BARS_PATTERN)
@if $(CP_BARS_CHECK) $(BUILD)/cp_bars_no_system_bar.ppm $(CP_BARS_GOLDEN)/cpanels.png shown > $(BUILD)/cp_bars_no_system_bar.pixels 2>&1; then printf 'FAIL system-bar repaint mutant survived\n'; exit 1; fi
@grep -E '^FAIL CP_BARS system=[1-9][0-9]*/12800 foreground=0/12800$$' $(BUILD)/cp_bars_no_system_bar.pixels
@printf '>>> test-flair-cpanels-bars-mutant: green (omitted system bar correctly RED)\n'
```

## test-flair-cpanels-bars-pattern

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 56):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_pattern,$(CP_BARS_IMG),CP_BARS_PATTERN)
@:
@:
@build/ppm_flair_cpanels_bars_check build/cp_bars_pattern.ppm fixtures/flair-cpanels-bars/cpanels.png shown
@grep -qx 'CPANELS-APPLY pattern=3 tracking=1 double=1 ticks=20' build/cp_bars_pattern.serial && build/ppm_flair_cpanels_check pattern build/cp_bars_pattern.ppm "../system7-decomp/goldens/resources/PAT#_0.bin" 3
@:
@:
@:
@:
@printf '>>> test-flair-cpanels-bars-pattern: green\n'
```

## test-flair-cpanels-bars-pattern-exit

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 57):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_pattern-exit,$(CP_BARS_IMG),CP_BARS_PATTERN_EXIT)
@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' build/cp_bars_pattern-exit.serial
@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' build/cp_bars_pattern-exit.serial && grep -qx 'APPLE-ABOUT closed' build/cp_bars_pattern-exit.serial
@build/ppm_flair_cpanels_bars_check build/cp_bars_pattern-exit.ppm fixtures/flair-cpanels-bars/hello.png shown
@grep -qx 'CPANELS-APPLY pattern=3 tracking=1 double=1 ticks=20' build/cp_bars_pattern-exit.serial && build/ppm_flair_cpanels_check pattern build/cp_bars_pattern-exit.ppm "../system7-decomp/goldens/resources/PAT#_0.bin" 3
@:
@:
@:
@:
@printf '>>> test-flair-cpanels-bars-pattern-exit: green\n'
```

## test-flair-cpanels-bars-show-exit

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 60):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_show-exit,$(CP_BARS_IMG),CP_BARS_SHOW_EXIT)
@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' build/cp_bars_show-exit.serial
@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' build/cp_bars_show-exit.serial && grep -qx 'APPLE-ABOUT closed' build/cp_bars_show-exit.serial
@build/ppm_flair_cpanels_bars_check build/cp_bars_show-exit.ppm fixtures/flair-cpanels-bars/hello.png shown
@:
@:
@test "$$(grep -cx 'CPANELS-RTC hour=0 minute=56' build/cp_bars_show-exit.serial)" = 2
@:
@:
@printf '>>> test-flair-cpanels-bars-show-exit: green\n'
```

## test-flair-cpanels-bars-sound-exit

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/cpanels_bars.mk', line 62):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_bars_gate.img build/flair_data.img build/ppm_flair_cpanels_bars_check build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call cp-bars-boot,cp_bars_sound-exit,$(CP_BARS_IMG),CP_BARS_SOUND_EXIT)
@grep -qx 'TENANT-MENU menu=129 item=1 src=mouse' build/cp_bars_sound-exit.serial
@grep -qx 'FLAIR-MENU menu=1 item=1 (sel=0x00010001)' build/cp_bars_sound-exit.serial && grep -qx 'APPLE-ABOUT closed' build/cp_bars_sound-exit.serial
@build/ppm_flair_cpanels_bars_check build/cp_bars_sound-exit.ppm fixtures/flair-cpanels-bars/hello.png shown
@:
@:
@:
@:
@grep -qx 'CPANELS-BEEP hz=660' build/cp_bars_sound-exit.serial && grep -qx 'CPANELS-BEEP off' build/cp_bars_sound-exit.serial
@printf '>>> test-flair-cpanels-bars-sound-exit: green\n'
```

## test-flair-cpanels-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-bochs "$(BUILD)/flair-cpanels_bochs.report.txt" "$(BUILD)/flair-cpanels_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-corrupt

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34751):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/cp_corrupt_data.img build/.makefile.stamp`
```make
@$(HARNESS_BIN) --disk $(CP_GATE_IMG) --disk2 $(BUILD)/cp_corrupt_data.img --name cp_corrupt --out $(BUILD) --quit-after FLAIR-LIVE-OK --timeout-ms 25000 --screendump --screendump-after FLAIR-LIVE-OK 2>$(BUILD)/cp_corrupt.report
@grep -qx 'CPANELS-CORRUPT defaults' $(BUILD)/cp_corrupt.serial
@$(BUILD)/ppm_flair_cpanels_check pattern $(BUILD)/cp_corrupt.ppm "$(SYSTEM7_DECOMP)/goldens/resources/PAT#_0.bin" 0
@printf '>>> test-flair-cpanels-corrupt: green (fail-loud serial + locked teal defaults)\n'
```

## test-flair-cpanels-corrupt-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 211):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/cp_corrupt_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-corrupt-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-corrupt-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/cp_corrupt_data.img $(BUILD)/flair-cpanels-corrupt_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-corrupt_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-corrupt_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-corrupt_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-corrupt_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-corrupt-bochs "$(BUILD)/flair-cpanels-corrupt_bochs.report.txt" "$(BUILD)/flair-cpanels-corrupt_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-date

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34652):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_date,$(CP_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CP_DATE_SPEC,FLAIR-LIVE-OK)
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_date.serial
@grep -qx 'CPANEL title=Date & Time' build/cp_date.serial
@build/ppm_flair_cpanels_check build/cp_date.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" 
@printf '>>> test-flair-cpanels-date: green (locked trace, dialog routing, independent button/ring chrome)\n'
```

## test-flair-cpanels-date-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-date-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-date-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-date_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-date_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-date_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-date_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-date_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-date-bochs "$(BUILD)/flair-cpanels-date_bochs.report.txt" "$(BUILD)/flair-cpanels-date_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-date-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34675):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_ring.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_date_ring,build/flair_tenants_mut_cp_ring.img,build/flair_data.img,FLAIR_CP_DATE_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Date & Time' build/cp_date_ring.serial
@if build/ppm_flair_cpanels_check build/cp_date_ring.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" > build/cp_date_ring.pixels; then printf 'FAIL ring mutant survived\n'; exit 1; fi
@grep '^FAIL CP_CHROME' build/cp_date_ring.pixels
@printf '>>> test-flair-cpanels-date-mutant: CTRL_MUT_RING_OLD_CORNERS correctly RED (independent chrome)\n'
```

## test-flair-cpanels-memory

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34656):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_memory,$(CP_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CP_MEMORY_SPEC,FLAIR-LIVE-OK)
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_memory.serial
@grep -qx 'CPANEL title=Memory' build/cp_memory.serial
@build/ppm_flair_cpanels_check build/cp_memory.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" 
@printf '>>> test-flair-cpanels-memory: green (locked trace, dialog routing, independent button/ring chrome)\n'
```

## test-flair-cpanels-memory-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-memory-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-memory-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-memory_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-memory_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-memory_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-memory_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-memory_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-memory-bochs "$(BUILD)/flair-cpanels-memory_bochs.report.txt" "$(BUILD)/flair-cpanels-memory_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-memory-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34679):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_ring.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_memory_ring,build/flair_tenants_mut_cp_ring.img,build/flair_data.img,FLAIR_CP_MEMORY_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Memory' build/cp_memory_ring.serial
@if build/ppm_flair_cpanels_check build/cp_memory_ring.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" > build/cp_memory_ring.pixels; then printf 'FAIL ring mutant survived\n'; exit 1; fi
@grep '^FAIL CP_CHROME' build/cp_memory_ring.pixels
@printf '>>> test-flair-cpanels-memory-mutant: CTRL_MUT_RING_OLD_CORNERS correctly RED (independent chrome)\n'
```

## test-flair-cpanels-mouse

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34654):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_mouse,$(CP_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CP_MOUSE_SPEC,FLAIR-LIVE-OK)
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_mouse.serial
@grep -qx 'CPANEL title=Mouse' build/cp_mouse.serial
@build/ppm_flair_cpanels_check build/cp_mouse.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
@printf '>>> test-flair-cpanels-mouse: green (locked trace, dialog routing, independent button/ring chrome)\n'
```

## test-flair-cpanels-mouse-apply-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34740):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_mouse.img build/flair_data.img build/.makefile.stamp`
```make
$(call calcda-boot,cp_mouse_inert,$(BUILD)/flair_tenants_mut_cp_mouse.img,$(FLAIR_DATA_IMG),CP_MOUSE_APPLY_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANELS-APPLY pattern=0 tracking=2 double=2 ticks=20' $(BUILD)/cp_mouse_inert.serial
@grep -qx 'FLAIR-CURSOR x=421 y=220' $(BUILD)/cp_mouse_inert.serial && ! grep -qx 'FLAIR-CURSOR x=422 y=220' $(BUILD)/cp_mouse_inert.serial
@printf '>>> CP_MUT_MOUSE_INERT correctly RED: FAIL CP_MOUSE_APPLY cursor421 expected422\n'
```

## test-flair-cpanels-mouse-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-mouse-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-mouse-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-mouse_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-mouse_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-mouse_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-mouse_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-mouse_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-mouse-bochs "$(BUILD)/flair-cpanels-mouse_bochs.report.txt" "$(BUILD)/flair-cpanels-mouse_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-mouse-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34677):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_ring.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_mouse_ring,build/flair_tenants_mut_cp_ring.img,build/flair_data.img,FLAIR_CP_MOUSE_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Mouse' build/cp_mouse_ring.serial
@if build/ppm_flair_cpanels_check build/cp_mouse_ring.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" > build/cp_mouse_ring.pixels; then printf 'FAIL ring mutant survived\n'; exit 1; fi
@grep '^FAIL CP_CHROME' build/cp_mouse_ring.pixels
@printf '>>> test-flair-cpanels-mouse-mutant: CTRL_MUT_RING_OLD_CORNERS correctly RED (independent chrome)\n'
```

## test-flair-cpanels-mutant

Declared tier: outside active vector; 
Prerequisites: `test-flair-cpanels-date-mutant test-flair-cpanels-patterns-mutant test-flair-cpanels-mouse-mutant test-flair-cpanels-sound-mutant test-flair-cpanels-memory-mutant test-flair-cpanels-settings-mutant test-flair-cpanels-mouse-apply-mutant test-flair-cpanels-radio-mutant build/.makefile.stamp`
```make
```

## test-flair-cpanels-patterns

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34653):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_patterns,$(CP_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CP_PATTERNS_SPEC,FLAIR-LIVE-OK)
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_patterns.serial
@grep -qx 'CPANEL title=Desktop Patterns' build/cp_patterns.serial
@build/ppm_flair_cpanels_check build/cp_patterns.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
@printf '>>> test-flair-cpanels-patterns: green (locked trace, dialog routing, independent button/ring chrome)\n'
```

## test-flair-cpanels-patterns-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-patterns-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-patterns-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-patterns_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-patterns_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-patterns_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-patterns_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-patterns_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-patterns-bochs "$(BUILD)/flair-cpanels-patterns_bochs.report.txt" "$(BUILD)/flair-cpanels-patterns_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-patterns-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34676):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_ring.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_patterns_ring,build/flair_tenants_mut_cp_ring.img,build/flair_data.img,FLAIR_CP_PATTERNS_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Desktop Patterns' build/cp_patterns_ring.serial
@if build/ppm_flair_cpanels_check build/cp_patterns_ring.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" > build/cp_patterns_ring.pixels; then printf 'FAIL ring mutant survived\n'; exit 1; fi
@grep '^FAIL CP_CHROME' build/cp_patterns_ring.pixels
@printf '>>> test-flair-cpanels-patterns-mutant: CTRL_MUT_RING_OLD_CORNERS correctly RED (independent chrome)\n'
```

## test-flair-cpanels-persist

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34710):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check test-flair-cpanels-bars-pattern-exit build/.makefile.stamp`
```make
$(call pw-reboot,$(CP_GATE_IMG),cp_persist,$(BUILD)/cp_persist_data.img,$(CP_SAVE_SPEC))
@[ "$$(grep -cx 'FLAIR-LIVE-READY' $(BUILD)/cp_persist.serial)" = 2 ]
@$(call cp-disk-saved,$(BUILD)/cp_persist_data.img) || { printf 'FAIL CP_OK_WRITE independent16-byte record\n'; exit 1; }
@grep -qx 'CPANELS-LOAD pattern=3 tracking=1 double=1' $(BUILD)/cp_persist.serial
@$(HARNESS_BIN) --disk $(CP_GATE_IMG) --disk2 $(BUILD)/cp_persist_data.img --name cp_reload --out $(BUILD) --quit-after FLAIR-LIVE-OK --timeout-ms 25000 --screendump --screendump-after FLAIR-LIVE-OK 2>$(BUILD)/cp_reload.report
@$(BUILD)/ppm_flair_cpanels_check pattern $(BUILD)/cp_reload.ppm "$(SYSTEM7_DECOMP)/goldens/resources/PAT#_0.bin" 3
$(call cp-transaction-boot,cp_cancel,$(FLAIR_DATA_IMG),CP_CANCEL_SPEC)
@if mdir -i $(BUILD)/cp_cancel_data.img ::CPANELS.DAT >/dev/null 2>&1; then printf 'FAIL CP_CANCEL_WRITE created file\n'; exit 1; fi
@printf '>>> test-flair-cpanels-persist: green (OK disk bytes, actual Restart, boot reapply vs PAT# golden, Cancel no file)\n'
```

## test-flair-cpanels-persist-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-persist-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-persist-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-persist_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-persist_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-persist_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-persist_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-persist_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-persist-bochs "$(BUILD)/flair-cpanels-persist_bochs.report.txt" "$(BUILD)/flair-cpanels-persist_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-radio-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34764):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_radio.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_radio,$(BUILD)/flair_tenants_mut_cp_radio.img,$(FLAIR_DATA_IMG),FLAIR_CP_MOUSE_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Mouse' $(BUILD)/cp_radio.serial
@if $(BUILD)/ppm_flair_cpanels_check $(BUILD)/cp_radio.ppm "$(SYSTEM7_DECOMP)/goldens/captures/s8_alert_modal.png" "$(SYSTEM7_DECOMP)/goldens/resources/NFNT_5478.bin" radio > $(BUILD)/cp_radio.pixels; then printf 'FAIL radio mutant survived\n'; exit 1; fi
@grep '^FAIL CP_RADIO_FACE' $(BUILD)/cp_radio.pixels
@printf '>>> CTRL_MUT_RADIO_DESKTOP_BG correctly RED: FAIL CP_RADIO_FACE inherited desktop instead of dialog face\n'
```

## test-flair-cpanels-settings

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34702):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img test-flair-cpanels-bars-pattern test-flair-cpanels-bars-hide test-flair-cpanels-bars-hide-exit test-flair-cpanels-bars-show-exit test-flair-cpanels-bars-mouse-exit test-flair-cpanels-bars-sound-exit build/.makefile.stamp`
```make
$(call cp-transaction-boot,cp_clock,$(FLAIR_DATA_IMG),CP_CLOCK_SAVE_SPEC)
@grep -qx 'CPANELS-RTC hour=10 minute=0' $(BUILD)/cp_clock.serial && grep -qx 'FLAIR-CLOCK 10:00 am' $(BUILD)/cp_clock.serial
$(call cp-transaction-boot,cp_mouse_apply,$(FLAIR_DATA_IMG),CP_MOUSE_APPLY_SPEC)
@grep -qx 'CPANELS-APPLY pattern=0 tracking=2 double=2 ticks=10' $(BUILD)/cp_mouse_apply.serial && grep -qx 'FLAIR-CURSOR x=422 y=220' $(BUILD)/cp_mouse_apply.serial
$(call cp-transaction-boot,cp_sound_apply,$(FLAIR_DATA_IMG),CP_SOUND_APPLY_SPEC)
@grep -qx 'CPANELS-BEEP hz=660' $(BUILD)/cp_sound_apply.serial && grep -qx 'CPANELS-BEEP off' $(BUILD)/cp_sound_apply.serial
@printf '>>> test-flair-cpanels-settings: green (AH2D -> menu clock, real2x mouse pump, PIT660Hz on/off)\n'
```

## test-flair-cpanels-settings-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-settings-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-settings-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-settings_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-settings_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-settings_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-settings_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-settings_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-settings-bochs "$(BUILD)/flair-cpanels-settings_bochs.report.txt" "$(BUILD)/flair-cpanels-settings_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-settings-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34720):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/cp_CP_MUT_OK_NOTHING_data.img build/cp_CP_MUT_CANCEL_WRITES_data.img build/cp_CP_MUT_CLOCK_NOT_APPLIED_data.img test-flair-cpanels-bars-mutant build/.makefile.stamp`
```make
$(call cp-transaction-boot,cp_mut_ok,$(BUILD)/cp_CP_MUT_OK_NOTHING_data.img,CP_SAVE_ONLY_SPEC)
@grep -qx 'CPANELS-APPLY pattern=3 tracking=1 double=1 ticks=20' $(BUILD)/cp_mut_ok.serial
@if $(call cp-disk-saved,$(BUILD)/cp_mut_ok_data.img); then printf 'FAIL OK_NOTHING survived\n'; exit 1; fi
@printf '>>> CP_MUT_OK_NOTHING correctly RED: FAIL CP_OK_WRITE independent16-byte record\n'
$(call cp-transaction-boot,cp_mut_cancel,$(BUILD)/cp_CP_MUT_CANCEL_WRITES_data.img,CP_CANCEL_SPEC)
@mdir -i $(BUILD)/cp_mut_cancel_data.img ::CPANELS.DAT >/dev/null
@printf '>>> CP_MUT_CANCEL_WRITES correctly RED: FAIL CP_CANCEL_WRITE created file\n'
$(call cp-transaction-boot,cp_mut_clock,$(BUILD)/cp_CP_MUT_CLOCK_NOT_APPLIED_data.img,CP_CLOCK_SAVE_SPEC)
@grep -qx 'CPANEL title=Date & Time' $(BUILD)/cp_mut_clock.serial
@mdir -i $(BUILD)/cp_mut_clock_data.img ::CPANELS.DAT >/dev/null
@! grep -qx 'CPANELS-RTC hour=10 minute=0' $(BUILD)/cp_mut_clock.serial && ! grep -qx 'FLAIR-CLOCK 10:00 am' $(BUILD)/cp_mut_clock.serial
@printf '>>> CP_MUT_CLOCK_NOT_APPLIED correctly RED: FAIL CP_CLOCK_APPLY no10am clock\n'
```

## test-flair-cpanels-sound

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34655):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_sound,$(CP_GATE_IMG),$(FLAIR_DATA_IMG),FLAIR_CP_SOUND_SPEC,FLAIR-LIVE-OK)
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_sound.serial
@grep -qx 'CPANEL title=Sound' build/cp_sound.serial
@build/ppm_flair_cpanels_check build/cp_sound.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
@printf '>>> test-flair-cpanels-sound: green (locked trace, dialog routing, independent button/ring chrome)\n'
```

## test-flair-cpanels-sound-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 210):
Prerequisites: `build/bochs_harness build/flair_tenants_mut_cpanels_gate.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-cpanels-sound-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants_mut_cpanels_gate.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-cpanels-sound-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-cpanels-sound_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants_mut_cpanels_gate.img" --disk2 "$(BUILD)/flair-cpanels-sound_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-cpanels-sound_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-cpanels-sound_bochs.report.txt" || true
@cat "$(BUILD)/flair-cpanels-sound_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-cpanels-sound-bochs "$(BUILD)/flair-cpanels-sound_bochs.report.txt" "$(BUILD)/flair-cpanels-sound_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-cpanels-sound-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 34678):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_cp_ring.img build/flair_data.img build/ppm_flair_cpanels_check build/.makefile.stamp`
```make
$(call calcda-boot,cp_sound_ring,build/flair_tenants_mut_cp_ring.img,build/flair_data.img,FLAIR_CP_SOUND_SPEC,FLAIR-LIVE-OK)
@grep -qx 'CPANEL title=Sound' build/cp_sound_ring.serial
@if build/ppm_flair_cpanels_check build/cp_sound_ring.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" > build/cp_sound_ring.pixels; then printf 'FAIL ring mutant survived\n'; exit 1; fi
@grep '^FAIL CP_CHROME' build/cp_sound_ring.pixels
@printf '>>> test-flair-cpanels-sound-mutant: CTRL_MUT_RING_OLD_CORNERS correctly RED (independent chrome)\n'
```

## test-flair-paint

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 260):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_app_launch_check build/ppm_flair_paint_check build/.makefile.stamp`
```make
$(call appl-boot,flair_paint,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_PAINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_paint.serial) && $(call PAINT_UP,$(BUILD)/flair_paint.serial) || { printf '!!! test-flair-paint FAIL: PAINT did not register / install its bar / title its canvas\n'; grep -E '^(TENANT|FLAIR-DISPATCH)' $(BUILD)/flair_paint.serial | head -20; exit 1; }
@$(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: band 2 is not the canon File Edit Image Layer Select View Window Help bar\n'; exit 1; }
@$(PPM_FLAIR_PAINT_CHECK_BIN) canvas $(BUILD)/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: the canvas\n'; exit 1; }
@printf '>>> test-flair-paint [1/2]: BAR -- PAINT.EXE launched from disk; band 2 is pixel-exactly the canon Photoshop bar (Chicago 12, NFNT 5478); blank white canvas\n'
$(call appl-boot,flair_paint_menu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_PAINT_MENU_SPEC,--screendump --screendump-after FLAIR-MENU-XDROP)
@$(call APPL_ALIVE,$(BUILD)/flair_paint_menu.serial) && $(call PAINT_GRAYED,$(BUILD)/flair_paint_menu.serial) || { printf '!!! test-flair-paint FAIL: the grayed release delivered something, or Quit did not end PAINT\n'; grep -E '^(TENANT-EVT|TENANT-EXIT|FLAIR-MENU)' $(BUILD)/flair_paint_menu.serial; exit 1; }
@$(PPM_FLAIR_PAINT_CHECK_BIN) menu $(BUILD)/flair_paint_menu.ppm || { printf '!!! test-flair-paint FAIL: the Edit panel is not all period-grayed\n'; exit 1; }
@printf '>>> test-flair-paint [2/2]: MENUS -- Edit drops with only disabled ink; releasing on a grayed item delivers nothing; File > Quit exits\n'
@printf '>>> test-flair-paint: green\n'
```

## test-flair-paint-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 118):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-paint-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-paint-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-paint_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-paint_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-paint_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-paint_bochs.report.txt" || true
@cat "$(BUILD)/flair-paint_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-paint-bochs "$(BUILD)/flair-paint_bochs.report.txt" "$(BUILD)/flair-paint_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-paint-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/frameapps.mk', line 271):
Prerequisites: `test-flair-paint build/flair_data_paint_PAINT_MUT_BAR_TEXT.img build/flair_data_paint_PAINT_MUT_ITEMS_ENABLED.img build/.makefile.stamp`
```make
$(call appl-boot,flair_paint_bartext,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_PAINT_PAINT_MUT_BAR_TEXT_IMG),FLAIR_PAINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call PAINT_UP,$(BUILD)/flair_paint_bartext.serial) || { printf '!!! bar-text mutant did not launch\n'; exit 1; }
@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_paint_bartext.ppm > $(BUILD)/flair_paint_bartext.pixels 2>&1; then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_BAR_TEXT survived\n'; exit 1; fi
@printf '>>> test-flair-paint-mutant: PAINT_MUT_BAR_TEXT RED: %s\n' "$$(grep -m1 FAIL $(BUILD)/flair_paint_bartext.pixels | cut -c1-160)"
$(call appl-boot,flair_paint_enabled,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_PAINT_PAINT_MUT_ITEMS_ENABLED_IMG),FLAIR_PAINT_MENU_SPEC,--screendump --screendump-after FLAIR-MENU-XDROP)
@grep -qx 'FLAIR-MENU-XDROP menu=129' $(BUILD)/flair_paint_enabled.serial || { printf '!!! items-enabled mutant did not drop Edit\n'; exit 1; }
@if $(call PAINT_GRAYED,$(BUILD)/flair_paint_enabled.serial); then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_ITEMS_ENABLED survived the serial leg\n'; exit 1; fi
@if $(PPM_FLAIR_PAINT_CHECK_BIN) menu $(BUILD)/flair_paint_enabled.ppm > $(BUILD)/flair_paint_enabled.pixels 2>&1; then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_ITEMS_ENABLED survived the pixel leg\n'; exit 1; fi
@printf '>>> test-flair-paint-mutant: PAINT_MUT_ITEMS_ENABLED RED: %s; serial %s TENANT-EVT what=81\n' "$$(grep -m1 FAIL $(BUILD)/flair_paint_enabled.pixels)" "$$(grep -c '^TENANT-EVT what=81$$' $(BUILD)/flair_paint_enabled.serial)"
@printf '>>> test-flair-paint-mutant: green (four named mutants RED)\n'
```

## test-paint-tenant

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 290):
Prerequisites: `build/test_paint_tenant build/.makefile.stamp`
```make
@$(BUILD)/test_paint_tenant
@printf '>>> test-paint-tenant: green (the canon bar, no Apple slot, all items grayed but File > Quit, blank canvas)\n'
```

## test-paint-tenant-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/frameapps.mk', line 293):
Prerequisites: `test-paint-tenant build/test_paint_tenant_mut_PAINT_MUT_BAR_TEXT build/test_paint_tenant_mut_PAINT_MUT_ITEMS_ENABLED build/.makefile.stamp`
```make
@set -e; for m in $(PAINT_MUTANTS); do \
	if $(BUILD)/test_paint_tenant_mut_$$m > $(BUILD)/paint_host_$$m.log 2>&1; then printf 'FAIL paint mutant survived: %s\n' "$$m"; exit 1; fi; \
	case $$m in *BAR*) why=BAR_TEXT;; *) why=GRAYED;; esac; \
	grep -q "FAIL paint routing: $$why" $(BUILD)/paint_host_$$m.log; \
	printf '>>> test-paint-tenant-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL paint routing: $$why" $(BUILD)/paint_host_$$m.log)"; \
done
@printf '>>> test-paint-tenant-mutant: green (four named mutants RED)\n'
```

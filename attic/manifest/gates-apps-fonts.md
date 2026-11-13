## test-flair-fonts

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32844):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data_fonts.img build/ppm_flair_fonts_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-fonts : styled, clipped text on the real desktop (initech-tdnl.68)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_fonts,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_FONTS_IMG),FLAIR_FONTS_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_fonts.serial) && $(call FONTS_DREW,$(BUILD)/flair_fonts.serial) || { printf '!!! test-flair-fonts FAIL [1/4] UP: FONTS did not load, register, come forward and issue its seven runs\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_fonts.serial; exit 1; }
@printf '>>> test-flair-fonts [1/4] UP: %s; seven TEXTDRAW runs accepted (fg 1, 257, 513, 1, 1025, 1793, 257)\n' "$$(grep -m1 '^TENANT-LOAD name=' $(BUILD)/flair_fonts.serial)"
@[ -s $(BUILD)/flair_fonts.ppm ] || { printf '!!! test-flair-fonts FAIL: no screendump\n'; exit 1; }
@out=$$($(PPM_FLAIR_FONTS_CHECK_BIN) window $(BUILD)/flair_fonts.ppm); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^>>> />>> test-flair-fonts [2-3\/4] /'; \
[ $$rc -eq 0 ] || { printf '!!! test-flair-fonts FAIL [2-3/4]: the window does not match the model (above)\n'; exit 1; }
$(call appl-boot,flair_fonts_menu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_FONTS_IMG),FLAIR_FONTS_MENU_SPEC,--screendump --screendump-after "FLAIR-MENU-XDROP menu=130")
@$(call APPL_ALIVE,$(BUILD)/flair_fonts_menu.serial) && grep -qx 'FLAIR-MENU-XDROP menu=130' $(BUILD)/flair_fonts_menu.serial && [ -s $(BUILD)/flair_fonts_menu.ppm ] || { printf '!!! test-flair-fonts FAIL [4/4] MENU: the Style menu (130) was not dropped by the held drag, or no dump\n'; grep -E '^(FLAIR-MENU|TENANT-REG|PANIC)' $(BUILD)/flair_fonts_menu.serial; exit 1; }
@out=$$($(PPM_FLAIR_FONTS_CHECK_BIN) menu $(BUILD)/flair_fonts_menu.ppm); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^>>> />>> test-flair-fonts [4\/4] /'; \
[ $$rc -eq 0 ] || { printf '!!! test-flair-fonts FAIL [4/4] MENU: the Style menu does not match the model (above)\n'; exit 1; }
@printf '>>> test-flair-fonts: green\n'
```

## test-flair-fonts-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 32870):
Prerequisites: `test-flair-fonts build/qemu_harness build/flair_data_fonts.img build/ppm_flair_fonts_check build/flair_tenants_mut_fonts_smear_off.img build/flair_tenants_mut_fonts_clip_ignore.img build/flair_tenants_mut_fonts_trunc_period.img build/.makefile.stamp`
```make
@$(call fonts-mutant-leg,smear_off,!!! W1 STYLES: line "Hamburger" bold at)
@$(call fonts-mutant-leg,clip_ignore,!!! W2 CLIP:)
@$(call fonts-mutant-leg,trunc_period,!!! W3 TITLE:)
@printf '>>> test-flair-fonts-mutant: green (three kernel mutants RED for the named reason)\n'
```

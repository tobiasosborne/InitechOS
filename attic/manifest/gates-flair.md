## test-apple-menu

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33734):
Prerequisites: `build/test_apple_menu build/.makefile.stamp`
```make
@$< "$(SYSTEM7_DECOMP)"
@printf '>>> test-apple-menu: green (rows, order, actions, the Apple slot, geometry vs s8_apple_menu.png, tracking)\n'
```

## test-apple-menu-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33737):
Prerequisites: `test-apple-menu build/test_apple_menu_mut_MENU_MUT_APPLE_DEAD build/test_apple_menu_mut_APPLE_MENU_MUT_LIST_ALL build/.makefile.stamp`
```make
@for p in $(APPLE_HOST_MUTANTS); do m=$${p%%:*}; leg=$${p##*:}; \
 if $(BUILD)/test_apple_menu_mut_$$m "$(SYSTEM7_DECOMP)" > $(BUILD)/test_apple_menu_mut_$$m.log 2>&1; then \
   printf '!!! test-apple-menu-mutant: %s survived -- decoration\n' "$$m"; exit 1; fi; \
 grep -q "^FAIL $$leg" $(BUILD)/test_apple_menu_mut_$$m.log || { cat $(BUILD)/test_apple_menu_mut_$$m.log; exit 1; }; \
 printf '>>> test-apple-menu-mutant: green (%s correctly RED: %s)\n' "$$m" "$$(grep -m1 "^FAIL $$leg" $(BUILD)/test_apple_menu_mut_$$m.log | cut -c1-100)"; done
```

## test-asset-lz

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r5.mk', line 30):
Prerequisites: `build/asset_lz build/.makefile.stamp | build/ksize5`
```make
@$(ASSET_LZ_TOOL) >$(KERNEL_R5_DIR)/asset_lz_data.h 2>$(KERNEL_R5_DIR)/asset_lz.sizes
@cmp -s $(KERNEL_R5_DIR)/asset_lz_data.h $(ASSET_LZ_DATA) || { \
	printf '!!! asset-lz FAIL: %s is stale against the locked strikes -- run make gen-asset-lz\n' '$(ASSET_LZ_DATA)'; exit 1; }
@cat $(KERNEL_R5_DIR)/asset_lz.sizes
$(CC) $(CFLAGS) -Ispec/assets -Ios/flair harness/proptest/test_asset_lz.c -o $(KERNEL_R5_DIR)/asset-lz
@$(KERNEL_R5_DIR)/asset-lz
```

## test-asset-lz-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r5.mk', line 38):
Prerequisites: `test-asset-lz build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DASSET_LZ_MUT_BYTE -Ispec/assets -Ios/flair harness/proptest/test_asset_lz.c -o $(KERNEL_R5_DIR)/asset-lz-mut
@if $(KERNEL_R5_DIR)/asset-lz-mut >$(KERNEL_R5_DIR)/asset-lz-mut.log 2>&1; then \
	printf '!!! test-asset-lz-mutant FAIL: asset_lz_byte survived\n'; exit 1; fi
@grep -m1 '^!!! asset-lz FAIL:' $(KERNEL_R5_DIR)/asset-lz-mut.log
@printf '>>> test-asset-lz-mutant: green (asset_lz_byte RED)\n'
```

## test-assets

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28267):
Prerequisites: `build/asset_check spec/assets/palette.h spec/assets/palette.json build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-assets : palette honesty + strike check\n'
@printf '  Ref: PRD Sec 10 (asset pipeline) / Sec 6.4 (Chicago) / Sec 12 (IP).\n'
@printf '  beads initech-vcq. The frame is REFERENCE ONLY -- we measure it.\n'
@printf '======================================================================\n'
@if [ -f $(PREVIEW_WEBP) ]; then \
	$(MAKE) --no-print-directory $(PREVIEW_PPM) \
	&& $(ASSET_CHECK_BIN) $(PALETTE_JSON) $(PREVIEW_PPM) \
	&& printf '%s\n' '----------------------------------------------------------------------' \
	&& printf 'VERDICT   : PASS -- palette re-samples match the fixture, strike well-formed\n' \
	&& printf '======================================================================\n'; \
else \
	printf '[SKIP] test-assets: local-only fixture %s absent (gitignored copyrighted frame); palette-honesty check requires it\n' '$(PREVIEW_WEBP)'; \
	printf '%s\n' '----------------------------------------------------------------------'; \
	printf 'VERDICT   : SKIP -- fixture absent; full check runs only where the local frame is present\n'; \
	printf '======================================================================\n'; \
fi
```

## test-blitter

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13231):
Prerequisites: `build/test_blitter build/.makefile.stamp`
```make
@printf ">>> test-blitter: region-clipped blit/fill -- IFF (in rect AND in clip), 8bpp+32bpp, full/empty/non-rect/partial\n"
@$(TEST_BLITTER)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_BLITTER_OPT) $(BLITTER_INC) -c os/flair/blitter.c -o $(BUILD)/blitter_freestanding.o \
	|| { printf '!!! test-blitter FAIL: blitter.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-blitter: green\n"
```

## test-blitter-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13238):
Prerequisites: `build/test_blitter_mutant_ignore build/test_blitter_mutant_off1 build/.makefile.stamp`
```make
@printf ">>> test-blitter-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_BLITTER_MUT_IGNORE) >/dev/null 2>&1; then printf '!!! test-blitter-mutant FAIL: IGNORE_CLIP PASSED -- the clip oracle is decoration\n'; exit 1; else printf '>>> test-blitter-mutant: green (IGNORE_CLIP correctly RED)\n'; fi
@if $(TEST_BLITTER_MUT_OFF1) >/dev/null 2>&1; then printf '!!! test-blitter-mutant FAIL: OFF_BY_ONE PASSED -- the clip-edge oracle is decoration\n'; exit 1; else printf '>>> test-blitter-mutant: green (OFF_BY_ONE correctly RED)\n'; fi
```

## test-boot-logo

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34823):
Prerequisites: `build/test_boot_logo build/boot_logo_gen build/.makefile.stamp`
```make
@$(TEST_BOOT_LOGO)
@$(BOOT_LOGO_GEN_BIN) > $(BUILD)/boot_logo.regen.h
@cmp -s $(BUILD)/boot_logo.regen.h $(BOOT_LOGO_H) \
	|| { printf '!!! test-boot-logo FAIL: $(BOOT_LOGO_H) is not the generator output (hand edit? run make gen-boot-logo)\n'; exit 1; }
@printf '>>> test-boot-logo: green (strike == geometry pixel for pixel; header == tools/boot_logo_gen.c output; %s RLE bytes)\n' "$$(sed -n 's/^#define BOOT_LOGO_RLE_BYTES *//p' $(BOOT_LOGO_H))"
```

## test-boot-logo-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34829):
Prerequisites: `build/test_boot_logo_mut_notch_row build/.makefile.stamp`
```make
@if $(TEST_BOOT_LOGO_MUT) > $(BUILD)/test_boot_logo_mut.log 2>&1; then printf '!!! test-boot-logo-mutant FAIL: BOOT_LOGO_MUT_NOTCH_ROW survived -- the geometry oracle is decoration\n'; exit 1; fi
@grep -q 'FAIL.*strike == geometry, pixel for pixel' $(BUILD)/test_boot_logo_mut.log || { cat $(BUILD)/test_boot_logo_mut.log; exit 1; }
@printf '>>> test-boot-logo-mutant: green BOOT_LOGO_MUT_NOTCH_ROW correctly RED: %s\n' "$$(grep -m1 'DIFF' $(BUILD)/test_boot_logo_mut.log | sed 's/^ *//')"
```

## test-boot-screen

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34833):
Prerequisites: `build/test_boot_screen build/ppm_flair_boot_screen_check build/.makefile.stamp`
```make
@test -s "$(BOOT_SCREEN_PNG)" || { printf '!!! test-boot-screen FAIL: golden %s absent (system7-decomp mint-009)\n' "$(BOOT_SCREEN_PNG)"; exit 1; }
@$(TEST_BOOT_SCREEN) $(BUILD)/boot_screen_host.ppm
@$(BOOT_SCREEN_CHECK) $(BUILD)/boot_screen_host.ppm "$(BOOT_SCREEN_PNG)"
@printf '>>> test-boot-screen: green (host render == s8_startup_welcome.png furniture; logo cell == strike)\n'
```

## test-boot-screen-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34838):
Prerequisites: `build/test_boot_screen_mut_no_logo build/ppm_flair_boot_screen_check build/.makefile.stamp`
```make
@$(TEST_BOOT_SCREEN_NL) $(BUILD)/boot_screen_host_nologo.ppm >/dev/null
@if $(BOOT_SCREEN_CHECK) $(BUILD)/boot_screen_host_nologo.ppm "$(BOOT_SCREEN_PNG)" > $(BUILD)/boot_screen_host_nologo.log 2>&1; then printf '!!! test-boot-screen-mutant FAIL: FLAIR_BOOT_MUT_NO_LOGO survived\n'; exit 1; fi
@grep -q 'FAIL LOGO' $(BUILD)/boot_screen_host_nologo.log || { cat $(BUILD)/boot_screen_host_nologo.log; exit 1; }
@printf '>>> test-boot-screen-mutant: green FLAIR_BOOT_MUT_NO_LOGO correctly RED: %s\n' "$$(grep -m1 'FAIL LOGO' $(BUILD)/boot_screen_host_nologo.log | sed 's/^ *//')"
```

## test-boot-vesa-images

Declared tier: HOST; #  recipe to execute (from 'harness/emu/vesa8.mk', line 70):
Prerequisites: `build/vesa8/scene.img build/vesa8/flagship.img build/boot_system build/.makefile.stamp`
```make
@set -eu; for pair in scene:$(KERNEL_FLAIRSHELL_BIN) flagship:$(INITDOS_SYS_BIN); do \
 tag=$${pair%%:*}; kernel=$${pair#*:}; img=$(VESA8_DIR)/$$tag.img; \
 $(BUILD)/boot_system $$img check; \
 mdir -a -i $$img@@32256 :: > $(VESA8_DIR)/$$tag.mdir; \
 mattrib -i $$img@@32256 ::IO.SYS ::INITDOS.SYS > $(VESA8_DIR)/$$tag.attrs; \
 mcopy -i $$img@@32256 ::IO.SYS - > $(VESA8_DIR)/$$tag.IO.SYS; \
 mcopy -i $$img@@32256 ::INITDOS.SYS - > $(VESA8_DIR)/$$tag.INITDOS.SYS; \
 cmp $(VESA8_DIR)/IO.SYS $(VESA8_DIR)/$$tag.IO.SYS; \
 cmp $$kernel $(VESA8_DIR)/$$tag.INITDOS.SYS; \
 dd if=$$img of=$(VESA8_DIR)/$$tag.volume.img bs=512 skip=63 count=2880 status=none; \
 fsck.fat -n $(VESA8_DIR)/$$tag.volume.img > $(VESA8_DIR)/$$tag.fsck 2>&1; \
 echo "GREEN test-boot-vesa-images $$tag: mtools bytes/order/HSR + dosfstools on identical forced image"; done
```

## test-boot-vesa-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/vesa8.mk', line 130):
Prerequisites: `test-flair-desktop-8bpp build/vesa8/boot_no101.img build/vesa8/boot_priority32.img build/.makefile.stamp`
```make
@set -eu; for m in no101 priority32; do \
 if sh harness/emu/vesa8.sh $(BUILD) mode $(VESA8_DIR)/boot_$$m.img $$m > $(VESA8_DIR)/$$m.red 2>&1; then echo "FAIL boot mutant $$m survived"; exit 1; fi; \
 why=expected-8bpp; [ $$m != priority32 ] || why=expected-32bpp; \
 grep "^RED vesa8 $$m $$why$$" $(VESA8_DIR)/$$m.red; done
@echo 'GREEN test-boot-vesa-mutant: missing 0101 fallback and inverted depth preference each RED'
```

## test-canon

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13426):
Prerequisites: `build/test_canon build/.makefile.stamp`
```make
@printf ">>> test-canon: FLAIR canon (Law 4) -- hourglass-not-watch, Photoshop menu, PC LOAD LETTER, pie==116, 570-\n"
@$(TEST_CANON)
@printf ">>> test-canon: green\n"
```

## test-canon-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13431):
Prerequisites: `build/test_canon_mutant_watch build/test_canon_mutant_menu build/.makefile.stamp`
```make
@printf ">>> test-canon-mutant: confirming WATCH + FIX_MENU mutants go RED (Rule 6; Law 4)\n"
@if $(TEST_CANON_MUT_WATCH) >/dev/null 2>&1; then printf '!!! test-canon-mutant FAIL: WATCH PASSED -- canon is decoration (wristwatch would pass)\n'; exit 1; else printf '>>> test-canon-mutant: green (WATCH correctly RED -- the hourglass canon bites)\n'; fi
@if $(TEST_CANON_MUT_MENU) >/dev/null 2>&1; then printf '!!! test-canon-mutant FAIL: FIX_MENU PASSED -- canon is decoration (a corrected menu would pass)\n'; exit 1; else printf '>>> test-canon-mutant: green (FIX_MENU correctly RED -- the Photoshop-menu canon bites)\n'; fi
```

## test-canon-salami

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3432):
Prerequisites: `build/test_canon_salami build/.makefile.stamp`
```make
@printf ">>> test-canon-salami: Bolton's salami rounding-error routine with the enforced too-much-too-fast bug (S7.2 canon, Law 4)\n"
@$(TEST_CANON_SALAMI) $(DBASE3_DECOMP) $(CANON_DIR)
@printf ">>> test-canon-salami: green (the skim bug is present and matches canon)\n"
```

## test-canon-salami-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3438):
Prerequisites: `build/test_canon_salami_mut build/.makefile.stamp`
```make
@printf ">>> test-canon-salami-mutant: confirming a rounding 'fix' breaks canon -> RED (Rule 6 + Law 4; initech-586.2)\n"
@$(TEST_CANON_SALAMI_MUT) $(DBASE3_DECOMP) $(CANON_DIR) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-canon-salami-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_CANON_SALAMI_MUT) $(DBASE3_DECOMP) $(CANON_DIR) >/dev/null 2>&1; then \
	printf '!!! test-canon-salami-mutant FAIL: mutant PASSED -- the skim bug is not enforced (canon decoration)\n'; exit 1; \
else \
	printf '>>> test-canon-salami-mutant: green (a rounding fix correctly breaks canon)\n'; \
fi
```

## test-canon-y2k

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3400):
Prerequisites: `build/test_canon_y2k build/.makefile.stamp`
```make
@printf ">>> test-canon-y2k: Initech AR accounting app with the enforced Y2K bug (S7.1 canon, Law 4)\n"
@$(TEST_CANON_Y2K) $(DBASE3_DECOMP) $(CANON_DIR)
@printf ">>> test-canon-y2k: green (the Y2K bug is present and matches canon)\n"
```

## test-canon-y2k-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3406):
Prerequisites: `build/test_canon_y2k_mut build/.makefile.stamp`
```make
@printf ">>> test-canon-y2k-mutant: confirming a Y2K 'fix' breaks canon -> RED (Rule 6 + Law 4; initech-586.1)\n"
@$(TEST_CANON_Y2K_MUT) $(DBASE3_DECOMP) $(CANON_DIR) 2>/dev/null | grep -q 'checks,' \
	|| { printf '!!! test-canon-y2k-mutant FAIL: no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }
@if $(TEST_CANON_Y2K_MUT) $(DBASE3_DECOMP) $(CANON_DIR) >/dev/null 2>&1; then \
	printf '!!! test-canon-y2k-mutant FAIL: mutant PASSED -- the Y2K bug is not enforced (canon decoration)\n'; exit 1; \
else \
	printf '>>> test-canon-y2k-mutant: green (a Y2K fix correctly breaks canon)\n'; \
fi
```

## test-chicago-art

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13270):
Prerequisites: `build/font_art2h spec/assets/chicago12.art spec/assets/chicago12.h build/.makefile.stamp`
```make
@$(FONT_ART2H_BIN) $(CHICAGO_ART) > $(BUILD)/chicago12.regen.h
@cmp -s $(BUILD)/chicago12.regen.h $(CHICAGO_H) \
	|| { printf '!!! test-chicago-art FAIL: $(CHICAGO_H) is not the transcription of $(CHICAGO_ART) (run make gen-chicago12)\n'; exit 1; }
@printf '>>> test-chicago-art: green -- $(CHICAGO_H) == transcribe($(CHICAGO_ART))\n'
```

## test-chicago-art-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13275):
Prerequisites: `build/font_art2h spec/assets/chicago12.art spec/assets/chicago12.h build/.makefile.stamp`
```make
@awk 'BEGIN{g=0} /^glyph 0x41 /{g=1;n=0;print;next} g==1{n++; if(n==6){sub(/#/,".")} if(n==16)g=0} {print}' $(CHICAGO_ART) > $(BUILD)/chicago12.mut1.art
@cmp -s $(BUILD)/chicago12.mut1.art $(CHICAGO_ART) && { printf '!!! test-chicago-art-mutant FAIL: the pixel flip did not apply\n'; exit 1; } || true
@$(FONT_ART2H_BIN) $(BUILD)/chicago12.mut1.art > $(BUILD)/chicago12.mut1.h
@if cmp -s $(BUILD)/chicago12.mut1.h $(CHICAGO_H); then printf '!!! test-chicago-art-mutant FAIL: a flipped pixel left the header unchanged -- the sync check is decoration\n'; exit 1; fi
@printf '>>> test-chicago-art-mutant: (a) green -- one flipped pixel in A makes the header differ\n'
@sed '/^glyph 0x41 /s/lb 1/lb 2/' $(CHICAGO_ART) > $(BUILD)/chicago12.mut2.art
@if $(FONT_ART2H_BIN) $(BUILD)/chicago12.mut2.art > /dev/null 2> $(BUILD)/chicago12.mut2.err; then printf '!!! test-chicago-art-mutant FAIL: art disagreeing with its lb was ACCEPTED\n'; exit 1; fi
@grep -q 'leftmost ink column != lb' $(BUILD)/chicago12.mut2.err || { cat $(BUILD)/chicago12.mut2.err; printf '!!! test-chicago-art-mutant FAIL: refused for the wrong reason\n'; exit 1; }
@printf '>>> test-chicago-art-mutant: (b) green -- art whose ink disagrees with the NFNT lb is refused\n'
```

## test-chicago-metrics

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13299):
Prerequisites: `build/test_chicago_metrics build/.makefile.stamp`
```make
@$(TEST_CHIMET)
```

## test-chicago-metrics-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13301):
Prerequisites: `build/test_chicago_metrics_mutant build/test_chicago_check_mutant build/.makefile.stamp`
```make
@out=$$($(TEST_CHIMET_MUT) 2>&1); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-chicago-metrics-mutant FAIL: ONE_ADVANCE mutant PASSED -- the metrics oracle is decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q "advance 'i' (0x69): text_measure 5, NFNT aw 4" \
	|| { printf '%s\n' "$$out" | tail -5; printf '!!! test-chicago-metrics-mutant FAIL: RED, but not for the perturbed advance\n'; exit 1; }; \
printf '>>> test-chicago-metrics-mutant: green (ONE_ADVANCE correctly RED on the perturbed glyph -- the oracle bites)\n'
@if $(BUILD)/test_chicago_check_mutant > $(BUILD)/chicago_check_mutant.log 2>&1; then printf 'FAIL check advance mutant survived\n'; exit 1; fi
@grep -q 'FAIL: H check0x12.*measure12' $(BUILD)/chicago_check_mutant.log
@printf '>>> test-chicago-metrics-mutant: CHECK_ADVANCE correctly RED (0x12 advance12 vs11)\n'
```

## test-chicago-pack

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 125):
Prerequisites: `build/chicago_packed_data.h build/.makefile.stamp | build/ksize4`
```make
$(CC) $(CFLAGS) -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_chicago_pack.c -o $(KERNEL_R4_DIR)/chicago-pack
@$(KERNEL_R4_DIR)/chicago-pack
```

## test-chicago-pack-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 128):
Prerequisites: `test-chicago-pack build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DCHICAGO_PACK_MUT_ROW -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_chicago_pack.c -o $(KERNEL_R4_DIR)/chicago-pack-mut
@if $(KERNEL_R4_DIR)/chicago-pack-mut >$(KERNEL_R4_DIR)/chicago-mut.log 2>&1; then \
	printf '!!! test-chicago-pack-mutant FAIL: packed_row survived\n'; exit 1; fi
@grep '^!!! chicago-pack FAIL:' $(KERNEL_R4_DIR)/chicago-mut.log
@printf '>>> test-chicago-pack-mutant: green (packed_row RED)\n'
```

## test-chrome

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12783):
Prerequisites: `build/test_chrome spec/chrome_metrics.json spec/chrome_metrics.h build/.makefile.stamp`
```make
@printf '>>> test-chrome [1/3]: CONSISTENCY -- spec/chrome_metrics.h == spec/chrome_metrics.json (native)\n'
@python3 -c "import json,re; \
```

## test-chrome-fidelity

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13073):
Prerequisites: `build/test_chrome_fidelity build/.makefile.stamp`
```make
@printf '>>> test-chrome-fidelity [callers]: STRUCTURAL -- live os/ default skin reaches the total resolver\n'
@if ! grep -Eq 'return[[:space:]]+flair_skin_default\(\);' os/flair/flair_look.c; then \
	printf '!!! test-chrome-fidelity FAIL: flair_skin_default has no live os/ policy caller (DEC-10 Sec 5.3 Law-2 smell)\n'; \
	exit 1; \
fi
@awk '/flair_skin_default\(void\)/,/^}/' spec/flair_skins.h | \
	grep -q 'flair_skin_resolve(FLAIR_DEFAULT_ERA, FLAIR_DEFAULT_HERITAGE)' \
	|| { printf '!!! test-chrome-fidelity FAIL: the live default no longer routes through flair_skin_resolve\n'; exit 1; }
@printf '    flair_look_default_skin -> flair_skin_default -> flair_skin_resolve is live under os/\n'
@printf '>>> test-chrome-fidelity: Platinum window-chrome fidelity vs the INDEPENDENT ../system7-decomp golden (Law 2, NOT by-construction)\n'
@$(TEST_CHROME_FID)
@printf '>>> test-chrome-fidelity: green\n'
```

## test-chrome-fidelity-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13087):
Prerequisites: `build/test_chrome_fidelity_mutant_phase build/test_chrome_fidelity_mutant_notitle build/test_chrome_fidelity_mutant_centeroff build/test_chrome_fidelity_mutant_noshadow build/test_chrome_fidelity_mutant_boxgeom build/test_chrome_fidelity_mutant_scrollflat build/test_chrome_fidelity_mutant_nobevel build/test_chrome_fidelity_mutant_noinactive build/test_chrome_fidelity_mutant_inactiveblackframe build/test_chrome_fidelity_mutant_inactivebrighttitle build/test_chrome_fidelity_mutant_keepgadgets build/test_chrome_fidelity_mutant_collapse build/test_chrome_fidelity_mutant_ramp build/test_chrome_fidelity_mutant_notch build/test_chrome_fidelity_mutant_bodybar build/test_chrome_fidelity_mutant_wrongera build/test_chrome_fidelity_mutant_stripe_unshifted build/test_chrome_fidelity_mutant_notch_double build/test_chrome_fidelity_mutant_dark_ring_black build/test_chrome_fidelity_mutant_cbox_corner build/test_chrome_fidelity_mutant_zoom_centered build/test_chrome_fidelity_mutant_sb_flat_well build/test_chrome_fidelity_mutant_sb_tile_flat build/test_chrome_fidelity_mutant_sb_sep_gray build/test_chrome_fidelity_mutant_font_advance build/test_chrome_fidelity_mutant_scroll_always_enabled build/test_chrome_fidelity_mutant_scroll_pressed_idle build/test_chrome_fidelity_mutant_content_left_thin build/test_chrome_fidelity_mutant_widget_corners_edge build/.makefile.stamp`
```make
@if $(TEST_CHROME_FID_MUT_CLT) > $(BUILD)/test_chrome_fid_mut_clt.log 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_CONTENT_LEFT_THIN PASSED -- the CAPTURE-BODY legs are decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_chrome_fid_mut_clt.log | grep -q 'leg CAPTURE-BODY:' && grep 'FAIL' $(BUILD)/test_chrome_fid_mut_clt.log | grep -q 'leg CAPTURE-BODY-INACTIVE:' && ! grep 'FAIL' $(BUILD)/test_chrome_fid_mut_clt.log | grep -q 'CAPTURE-LOAD' || { cat $(BUILD)/test_chrome_fid_mut_clt.log; printf '!!! test-chrome-fidelity-mutant FAIL: CONTENT_LEFT_THIN red for the wrong reason (want CAPTURE-BODY + CAPTURE-BODY-INACTIVE, captures loaded)\n'; exit 1; }
@printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_CONTENT_LEFT_THIN correctly RED -- an owner fill over the left raised bar is caught against both captures)\n'
@if $(TEST_CHROME_FID_MUT_WCE) > $(BUILD)/test_chrome_fid_mut_wce.log 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_WIDGET_CORNERS_EDGE PASSED -- the CAPTURE-WIDGETS leg is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_chrome_fid_mut_wce.log | grep -q 'leg CAPTURE-WIDGETS:' && ! grep 'FAIL' $(BUILD)/test_chrome_fid_mut_wce.log | grep -q 'CAPTURE-LOAD' || { cat $(BUILD)/test_chrome_fid_mut_wce.log; printf '!!! test-chrome-fidelity-mutant FAIL: WIDGET_CORNERS_EDGE red for the wrong reason\n'; exit 1; }
@printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_WIDGET_CORNERS_EDGE correctly RED -- the #A5A5A5 off-diagonal widget corners are caught against the capture)\n'
@printf '>>> test-chrome-fidelity-mutant: confirming all twenty-eight Platinum chrome mutants go RED (Rule 6)\n'
@if $(TEST_CHROME_FID_MUT_SAE) > $(BUILD)/test_chrome_fid_mut_sae.log 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_MUT_SCROLL_ALWAYS_ENABLED PASSED -- the SCROLL-DISABLED leg is decoration\n'; exit 1; fi
@grep -q 'SCROLL-DISABLED' $(BUILD)/test_chrome_fid_mut_sae.log && grep 'FAIL' $(BUILD)/test_chrome_fid_mut_sae.log | grep -q 'SCROLL-DISABLED' || { printf '!!! test-chrome-fidelity-mutant FAIL: SCROLL_ALWAYS_ENABLED red for the wrong reason\n'; exit 1; }
@printf '>>> test-chrome-fidelity-mutant: green (CHROME_MUT_SCROLL_ALWAYS_ENABLED correctly RED -- an active window with nothing to scroll must draw the DISABLED bar)\n'
@if $(TEST_CHROME_FID_MUT_SPI) > $(BUILD)/test_chrome_fid_mut_spi.log 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_MUT_SCROLL_PRESSED_IDLE PASSED -- the SCROLL-PRESSED leg is decoration\n'; exit 1; fi
@grep 'FAIL' $(BUILD)/test_chrome_fid_mut_spi.log | grep -q 'SCROLL-PRESSED' || { printf '!!! test-chrome-fidelity-mutant FAIL: SCROLL_PRESSED_IDLE red for the wrong reason\n'; exit 1; }
@printf '>>> test-chrome-fidelity-mutant: green (CHROME_MUT_SCROLL_PRESSED_IDLE correctly RED -- a held arrow must draw pressed)\n'
@if $(TEST_CHROME_FID_MUT) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_PHASE PASSED -- the phase oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_PHASE correctly RED -- the dark-first 12-row stripe field is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_TTL) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_TITLE_BLANK PASSED -- the title bitmap oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_TITLE_BLANK correctly RED -- skipped DrawString is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_CTR) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_CENTER_OFF PASSED -- the centered-x oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_CENTER_OFF correctly RED -- the +7px title displacement is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_SHA) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_NO_SHADOW PASSED -- the shadow leg is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_NO_SHADOW correctly RED -- the missing shadow is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_BOX) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_BOX_GEOM PASSED -- the close/zoom box legs are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_BOX_GEOM correctly RED -- the flat two-widget geometry is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_SBF) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_SCROLL_FLAT PASSED -- the scrollbar oracle legs are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_SCROLL_FLAT correctly RED -- the flat/btnface/no-glyph scrollbar is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_BVL) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_NO_BEVEL PASSED -- the title-bar bevel + 15-row leg is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_NO_BEVEL correctly RED -- the missing 22-row face/highlight/shadow profile is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_INA) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_NO_INACTIVE PASSED -- the inactive-title-bar leg is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_NO_INACTIVE correctly RED -- the always-active title bar is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_IBF) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_INACTIVE_BLACK_FRAME PASSED -- the inactive frame-recolor legs are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_INACTIVE_BLACK_FRAME correctly RED -- the black-frame-on-inactive bug is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_IBT) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_INACTIVE_BRIGHT_TITLE PASSED -- the inactive title-dim legs are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_INACTIVE_BRIGHT_TITLE correctly RED -- the bright-title-on-inactive bug is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_IKG) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_KEEP_GADGETS PASSED -- the inactive gadget-absence leg is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_KEEP_GADGETS correctly RED -- the still-present gadgets on inactive are caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_COL) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_COLLAPSE PASSED -- the third-widget oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_COLLAPSE correctly RED -- the missing rightmost collapse widget is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_RMP) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_RAMP PASSED -- the widget-ramp oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_RAMP correctly RED -- the flattened seven-rung diagonal ramp is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_NTC) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_NOTCH PASSED -- the shadow-notch oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_NOTCH correctly RED -- the one-pixel near-corner shadow start is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_BDB) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_BODYBAR PASSED -- the raised-body-rail oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_BODYBAR correctly RED -- the single-line body frame is caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_SKIN) >/dev/null 2>&1; then \
	printf '!!! test-chrome-fidelity-mutant FAIL: FLAIR_MUT_SKIN_WRONG_ERA PASSED -- the era pointer is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-fidelity-mutant: green (FLAIR_MUT_SKIN_WRONG_ERA correctly RED -- retained SYS7 row values are caught)\n'; \
fi
@if $(TEST_CHROME_FID_MUT_SOF) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_STRIPE_UNSHIFTED PASSED -- the row-parity endpoint oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_STRIPE_UNSHIFTED correctly RED)\n'; fi
@if $(TEST_CHROME_FID_MUT_NDB) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_NOTCH_DOUBLE PASSED -- the shared separator oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_NOTCH_DOUBLE correctly RED)\n'; fi
@if $(TEST_CHROME_FID_MUT_DRB) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_DARK_RING_BLACK PASSED -- the sampled idx63 bevel oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_DARK_RING_BLACK correctly RED)\n'; fi
@if $(TEST_CHROME_FID_MUT_CBC) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_CBOX_CORNER PASSED -- the closed-ring corner oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_CBOX_CORNER correctly RED)\n'; fi
@if $(TEST_CHROME_FID_MUT_ZCT) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: CHROME_FID_MUT_ZOOM_CENTERED PASSED -- the nested-square glyph oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (CHROME_FID_MUT_ZOOM_CENTERED correctly RED)\n'; fi
@if $(TEST_CHROME_FID_MUT_SFW) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: SB_MUT_FLAT_WELL PASSED -- the five-value well oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (SB_MUT_FLAT_WELL correctly RED -- the flat C0 page well is caught)\n'; fi
@if $(TEST_CHROME_FID_MUT_STF) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: SB_MUT_TILE_FLAT PASSED -- the arrow-tile bevel oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (SB_MUT_TILE_FLAT correctly RED -- the flat E7 arrow tile is caught)\n'; fi
@if $(TEST_CHROME_FID_MUT_SSG) >/dev/null 2>&1; then printf '!!! test-chrome-fidelity-mutant FAIL: SB_MUT_SEP_GRAY PASSED -- the active separator oracle is decoration\n'; exit 1; else printf '>>> test-chrome-fidelity-mutant: green (SB_MUT_SEP_GRAY correctly RED -- inactive-gray active separators are caught)\n'; fi
@out=$$($(TEST_CHROME_FID_MUT_FONT) 2>&1); if [ $$? -eq 0 ]; then printf '!!! test-chrome-fidelity-mutant FAIL: CHICAGO_MUT_ONE_ADVANCE(T) PASSED -- the title glyph run is decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'leg TITLE:' || { printf '!!! test-chrome-fidelity-mutant FAIL: font mutant RED, but not on the TITLE leg\n'; exit 1; }; \
printf '>>> test-chrome-fidelity-mutant: green (CHICAGO_MUT_ONE_ADVANCE(T) correctly RED on the TITLE leg -- the run is graded against the NFNT advances)\n'
```

## test-chrome-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12830):
Prerequisites: `build/test_chrome_mutant_titlebar build/test_chrome_mutant_noframe build/test_chrome_mutant_scrollbar build/test_chrome_mutant_pressed_idle build/.makefile.stamp`
```make
@printf '>>> test-chrome-mutant: confirming all four mutants go RED (Rule 6; FO-2/AM-3)\n'
@if $(TEST_CHROME_MUT_PRESSED) >/dev/null 2>&1; then \
	printf '!!! test-chrome-mutant FAIL: PRESSED_IDLE mutant PASSED -- the pressed-widget oracle (initech-tdnl.60) is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-mutant: green (CHROME_MUT_PRESSED_IDLE correctly RED -- no press feedback is caught)\n'; \
fi
@if $(TEST_CHROME_MUT_TITLE) >/dev/null 2>&1; then \
	printf '!!! test-chrome-mutant FAIL: TITLEBAR_H mutant PASSED -- the title-bar-height oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-mutant: green (CHROME_MUTATE_TITLEBAR_H correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_CHROME_MUT_FRAME) >/dev/null 2>&1; then \
	printf '!!! test-chrome-mutant FAIL: NO_FRAME mutant PASSED -- the window-frame oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-mutant: green (CHROME_MUTATE_NO_FRAME correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_CHROME_MUT_SBW) >/dev/null 2>&1; then \
	printf '!!! test-chrome-mutant FAIL: SCROLLBAR_W mutant PASSED -- the scrollbar-width oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-chrome-mutant: green (CHROME_MUTATE_SCROLLBAR_W correctly RED -- the oracle bites)\n'; \
fi
```

## test-clut

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15999):
Prerequisites: `build/test_clut build/.makefile.stamp`
```make
@printf ">>> test-clut: 256-entry indexed-8 CLUT (ROM clut_8 cube/ramps + RGB->index) (spec/assets/clut.h)\n"
@$(TEST_CLUT)
@printf ">>> test-clut: green\n"
```

## test-clut-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16003):
Prerequisites: `build/test_clut_mutant test-clut build/.makefile.stamp`
```make
@if $(TEST_CLUT_MUT) >/dev/null 2>&1; then printf '!!! test-clut-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; else printf '>>> test-clut-mutant: green (mutant correctly RED)\n'; fi
```

## test-color-canon

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16046):
Prerequisites: `build/test_color_canon build/.makefile.stamp`
```make
@printf ">>> test-color-canon: FLAIR color VALUE oracle -- 6 legs vs INDEPENDENT decomp goldens (A wctb / B win31 / C pinstripe / D authored-teal / E title-bar inactive-hilite / F Platinum); NOT by construction (ADR-0010 + DEC-10)\n"
@$(TEST_COLOR_CANON)
@printf ">>> test-color-canon: green\n"
```

## test-color-canon-gen

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12096):
Prerequisites: `build/color_canon_extract spec/assets/color_canon.json spec/assets/color_canon.h build/.makefile.stamp`
```make
@$(COLOR_CANON_TOOL_BIN) $(COLOR_CANON_JSON) > $(BUILD)/color_canon_regen.h
@if diff -q $(COLOR_CANON_H) $(BUILD)/color_canon_regen.h >/dev/null 2>&1; then \
	printf ">>> test-color-canon-gen: green (committed color_canon.h == generator output)\n"; \
else \
	printf "!!! test-color-canon-gen FAIL: spec/assets/color_canon.h diverges from the generator (hand-edited? -- regenerate with make gen-color-canon)\n"; \
	diff $(COLOR_CANON_H) $(BUILD)/color_canon_regen.h | head -20; exit 1; \
fi
```

## test-color-canon-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16057):
Prerequisites: `test-color-canon build/test_color_canon_mut_teal build/test_color_canon_mut_navy build/test_color_canon_mut_white build/test_color_canon_mut_pin build/test_color_canon_mut_hilite build/.makefile.stamp`
```make
@printf ">>> test-color-canon-mutant: confirming all 5 VALUE mutants go RED (Rule 6; the INDEPENDENT golden catches a wrong canon value)\n"
@if $(TEST_COLOR_CANON_MUT_TEAL) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant FAIL: CANON_MUTATE_TEAL PASSED -- the authored-teal datum is not gated (seafoam relapse slips past)\n'; exit 1; else printf '>>> test-color-canon-mutant: green (CANON_MUTATE_TEAL correctly RED -- LEG D seafoam-relapse caught)\n'; fi
@if $(TEST_COLOR_CANON_MUT_NAVY) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant FAIL: CANON_MUTATE_NAVY PASSED -- the win31 depth-trap is swamped (#0000AA accepted)\n'; exit 1; else printf '>>> test-color-canon-mutant: green (CANON_MUTATE_NAVY correctly RED -- LEG B depth-trap tight)\n'; fi
@if $(TEST_COLOR_CANON_MUT_WHITE) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant FAIL: CANON_MUTATE_WHITE PASSED -- the wctb binary golden is decoration\n'; exit 1; else printf '>>> test-color-canon-mutant: green (CANON_MUTATE_WHITE correctly RED -- LEG A wctb bites)\n'; fi
@if $(TEST_COLOR_CANON_MUT_PIN) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant FAIL: CANON_MUTATE_PIN PASSED -- the pinstripe.md golden is decoration\n'; exit 1; else printf '>>> test-color-canon-mutant: green (CANON_MUTATE_PIN correctly RED -- LEG C pinstripe bites)\n'; fi
@if $(TEST_COLOR_CANON_MUT_HIL) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant FAIL: CANON_MUTATE_HILITE PASSED -- the title-bar.md inactive-hilite golden is decoration\n'; exit 1; else printf '>>> test-color-canon-mutant: green (CANON_MUTATE_HILITE correctly RED -- LEG E title-bar bites)\n'; fi
```

## test-color-canon-mutant-plat

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16065):
Prerequisites: `test-color-canon build/test_color_canon_mut_plat build/.makefile.stamp`
```make
@printf ">>> test-color-canon-mutant-plat: confirming the sampled-vs-nominal Platinum gamma-trap mutant goes RED (Rule 6)\n"
@if $(TEST_COLOR_CANON_MUT_PLAT) >/dev/null 2>&1; then printf '!!! test-color-canon-mutant-plat FAIL: CANON_MUTATE_PLAT PASSED -- LEG F accepts nominal idx119 #777777 for sampled #969696\n'; exit 1; else printf '>>> test-color-canon-mutant-plat: green (CANON_MUTATE_PLAT correctly RED -- LEG F sampled-domain golden bites)\n'; fi
```

## test-control

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15706):
Prerequisites: `build/test_control build/.makefile.stamp`
```make
@printf ">>> test-control: sampled checked box + unpressed push button + 16px Platinum scrollbar + FILE COPY progress bar + TestControl/TrackControl\n"
@$(TEST_CONTROL)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_CONTROL_OPT) -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/control.c -o $(BUILD)/control_freestanding.o \
	|| { printf '!!! test-control FAIL: control.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-control: green\n"
```

## test-control-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15713):
Prerequisites: `build/test_control_mutant_label_opaque build/test_control_mutant_btn_fringe build/test_control_mutant_progress_flat build/test_control_mutant_thumb build/test_control_mutant_clamp build/test_control_mutant_sb_thumb_16 build/test_control_mutant_sb_disabled_enabled build/test_control_mutant_check_fill build/test_control_mutant_btn_flat build/.makefile.stamp`
```make
@printf ">>> test-control-mutant: confirming all eight mutants go RED (Rule 6)\n"
@if $(TEST_CONTROL_MUT_THUMB) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: THUMB_OFF PASSED -- the scrollbar-math oracle is decoration\n'; exit 1; else printf '>>> test-control-mutant: green (THUMB_OFF correctly RED)\n'; fi
@if $(TEST_CONTROL_MUT_CLAMP) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: NO_CLAMP PASSED -- the value-clamp oracle is decoration\n'; exit 1; else printf '>>> test-control-mutant: green (NO_CLAMP correctly RED)\n'; fi
@if $(TEST_CONTROL_MUT_SB16) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: SB_MUT_THUMB_16 PASSED -- the sampled 15px thumb oracle is decoration\n'; exit 1; else printf '>>> test-control-mutant: green (SB_MUT_THUMB_16 correctly RED -- the heritage 16px thumb is caught)\n'; fi
@if $(TEST_CONTROL_MUT_SB_DISABLED) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: SB_MUT_DISABLED_ENABLED_LOOK PASSED -- the disabled-face oracle is decoration\n'; exit 1; else printf '>>> test-control-mutant: green (SB_MUT_DISABLED_ENABLED_LOOK correctly RED -- enabled art in the disabled state is caught)\n'; fi
@if $(TEST_CONTROL_MUT_CHECK_FILL) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: CTRL_MUT_CHECK_FILL PASSED -- accent-fill checkbox relapse escaped the exact tile/X oracle\n'; exit 1; else printf '>>> test-control-mutant: green (CTRL_MUT_CHECK_FILL correctly RED -- accent-fill relapse caught)\n'; fi
@if $(TEST_CONTROL_MUT_BTN_FLAT) >/dev/null 2>&1; then printf '!!! test-control-mutant FAIL: CTRL_MUT_BTN_FLAT PASSED -- flat button escaped the bevel/corner oracle\n'; exit 1; else printf '>>> test-control-mutant: green (CTRL_MUT_BTN_FLAT correctly RED -- bevel/corner relapse caught)\n'; fi
@$(TEST_CONTROL_MUT_LABEL) > $(TEST_CONTROL_MUT_LABEL).log 2>&1; \
if grep -q 'FAIL.*BUTTON-LABEL-BEVEL' $(TEST_CONTROL_MUT_LABEL).log; then printf '>>> test-control-mutant: green (CTRL_MUT_LABEL_OPAQUE correctly RED: BUTTON-LABEL-BEVEL)\n'; else cat $(TEST_CONTROL_MUT_LABEL).log; exit 1; fi
@$(TEST_CONTROL_MUT_FRINGE) > $(TEST_CONTROL_MUT_FRINGE).log 2>&1; \
if grep -q 'FAIL.*BUTTON-CORNERS' $(TEST_CONTROL_MUT_FRINGE).log; then printf '>>> test-control-mutant: green (CTRL_MUT_BTN_FRINGE correctly RED: BUTTON-CORNERS)\n'; else cat $(TEST_CONTROL_MUT_FRINGE).log; exit 1; fi
@$(TEST_CONTROL_MUT_PROGRESS_FLAT) > $(TEST_CONTROL_MUT_PROGRESS_FLAT).log 2>&1; \
if grep -q 'FAIL.*PROGRESS_GOLDEN capture E=274' $(TEST_CONTROL_MUT_PROGRESS_FLAT).log; then printf '>>> test-control-mutant: green (CTRL_MUT_PROGRESS_FLAT correctly RED: %s)\n' "$$(grep -m1 'FAIL.*PROGRESS_GOLDEN capture E=274' $(TEST_CONTROL_MUT_PROGRESS_FLAT).log)"; else cat $(TEST_CONTROL_MUT_PROGRESS_FLAT).log; exit 1; fi
```

## test-control-press-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 65):
Prerequisites: `test-control build/test_control_mut_press_accent build/.makefile.stamp`
```make
@if $(BUILD)/test_control_mut_press_accent > $(BUILD)/test_control_mut_press_accent.log 2>&1; then printf '!!! test-control-press-mutant: CTRL_MUT_PRESS_ACCENT survived\n'; exit 1; fi
@grep -m1 'FAIL.*PRESSED-CAPTURE' $(BUILD)/test_control_mut_press_accent.log
@printf '>>> test-control-press-mutant: CTRL_MUT_PRESS_ACCENT RED\n'
```

## test-control-record

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15947):
Prerequisites: `build/test_control_record build/.makefile.stamp`
```make
@printf ">>> test-control-record: ControlRecord + part-codes + CDEF proc IDs + Win flat-button accent (spec/control_record.h)\n"
@$(TEST_CTRL_RECORD)
@printf ">>> test-control-record: green\n"
```

## test-control-record-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15951):
Prerequisites: `build/test_control_record_mutant build/.makefile.stamp`
```make
@if $(TEST_CTRL_RECORD_MUT) >/dev/null 2>&1; then printf '!!! test-control-record-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; else printf '>>> test-control-record-mutant: green (mutant correctly RED)\n'; fi
```

## test-cursor

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13487):
Prerequisites: `build/test_cursor build/.makefile.stamp`
```make
@printf ">>> test-cursor: 500 clipped save-under restores + hotspot/golden + shield nesting + move erase\n"
@$(TEST_CURSOR) $(CURSOR_CAPTURES)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_CURSOR_OPT) $(CURSOR_INC) -c os/flair/cursor.c -o $(BUILD)/cursor_freestanding.o \
	|| { printf '!!! test-cursor FAIL: cursor.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-cursor: green\n"
```

## test-cursor-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13494):
Prerequisites: `build/test_cursor_mut_no_erase build/test_cursor_mut_hotspot build/test_cursor_mut_squared_stem build/.makefile.stamp`
```make
@printf ">>> test-cursor-mutant: confirming CursorMgr mutants go RED (Rule 6)\n"
@if $(TEST_CURSOR_MUT_NO_ERASE) $(CURSOR_CAPTURES) >/dev/null 2>&1; then printf '!!! test-cursor-mutant FAIL: CURSOR_MUT_NO_ERASE PASSED -- the trail oracle is decoration\n'; exit 1; else printf '>>> test-cursor-mutant: green (CURSOR_MUT_NO_ERASE correctly RED)\n'; fi
@if $(TEST_CURSOR_MUT_HOTSPOT) $(CURSOR_CAPTURES) >/dev/null 2>&1; then printf '!!! test-cursor-mutant FAIL: CURSOR_MUT_HOTSPOT PASSED -- the independent position golden is decoration\n'; exit 1; else printf '>>> test-cursor-mutant: green (CURSOR_MUT_HOTSPOT correctly RED)\n'; fi
@rc=0; $(TEST_CURSOR_MUT_SQUARED) $(CURSOR_CAPTURES) >$(BUILD)/cursor_mut_squared_stem.log 2>&1 || rc=$$?; \
[ "$$rc" -eq 1 ] && grep -qxF 'cursor M009 RED: 48/256 differing cells (zero tolerance)' $(BUILD)/cursor_mut_squared_stem.log \
	|| { cat $(BUILD)/cursor_mut_squared_stem.log; printf '!!! test-cursor-mutant FAIL: squared stem did not fail on the independent crop\n'; exit 1; }; \
printf '>>> CURSOR_MUT_SQUARED_STEM: RED -- 48/256 differing cells against Mac capture\n'
```

## test-desk-icons

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13547):
Prerequisites: `build/test_desk_icons build/.makefile.stamp`
```make
@printf ">>> test-desk-icons: blitter mechanics (transparency, clip, edges, hit test, 8bpp + 32bpp, selected) vs the mint-008 captures\n"
@$(TEST_DESK_ICONS) "$(ICON_CAPTURES_DIR)"
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(DESK_ICONS_INC) -c os/flair/finder_icon.c -o $(BUILD)/finder_icon_freestanding.o \
	|| { printf '!!! test-desk-icons FAIL: finder_icon.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-desk-icons: green\n"
```

## test-desk-icons-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13554):
Prerequisites: `build/test_desk_icons_mutant_mask_ignored build/test_desk_icons_mutant_row_off1 build/test_desk_icons_mutant_full_row build/.makefile.stamp`
```make
@printf ">>> test-desk-icons-mutant: confirming the desktop-icon mutants go RED (Rule 6)\n"
@$(TEST_DESK_ICONS_MUT_FULLROW) "$(ICON_CAPTURES_DIR)" > $(TEST_DESK_ICONS_MUT_FULLROW).log 2>&1; \
if grep -q 'TRASH_FULL draw: 21/3072 differing pixels' $(TEST_DESK_ICONS_MUT_FULLROW).log && ! $(TEST_DESK_ICONS_MUT_FULLROW) "$(ICON_CAPTURES_DIR)" >/dev/null 2>&1; then printf '>>> test-desk-icons-mutant: green (DESK_ICONS_MUT_FULL_ROW correctly RED -- the FULL strike lost its paper-tops row: 21/3072)\n'; \
else printf '!!! test-desk-icons-mutant FAIL: DESK_ICONS_MUT_FULL_ROW did not go RED on the FULL strike\n'; cat $(TEST_DESK_ICONS_MUT_FULLROW).log; exit 1; fi
@if $(TEST_DESK_ICONS_MUT_MASK) "$(ICON_CAPTURES_DIR)" >/dev/null 2>&1; then printf '!!! test-desk-icons-mutant FAIL: DESK_ICON_MUT_MASK_IGNORED PASSED -- the transparency oracle is decoration\n'; exit 1; else printf '>>> test-desk-icons-mutant: green (DESK_ICON_MUT_MASK_IGNORED correctly RED -- painted clear nibbles overwrite the sentinel)\n'; fi
@if $(TEST_DESK_ICONS_MUT_ROW) "$(ICON_CAPTURES_DIR)" >/dev/null 2>&1; then printf '!!! test-desk-icons-mutant FAIL: DESK_ICON_MUT_ROW_OFF1 PASSED -- the capture cell grade is decoration\n'; exit 1; else printf '>>> test-desk-icons-mutant: green (DESK_ICON_MUT_ROW_OFF1 correctly RED -- shifted rows miss the capture)\n'; fi
```

## test-dialog

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15859):
Prerequisites: `build/test_dialog build/.makefile.stamp`
```make
@printf ">>> test-dialog: FILE COPY E7/inset content + generic dBox + default ring/moat + canon layout + MODALDIALOG routing\n"
@$(TEST_DIALOG)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_DIALOG_OPT) -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets \
	-c os/flair/dialog.c -o $(BUILD)/dialog_freestanding.o \
	|| { printf '!!! test-dialog FAIL: dialog.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-dialog: green\n"
```

## test-dialog-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15867):
Prerequisites: `build/test_dialog_mutant_alert_slab build/test_dialog_mutant_border build/test_dialog_mutant_hit_static build/test_dialog_mutant_filecopy_msg build/test_dialog_mutant_titleless_modal build/test_dialog_mutant_progress_zero build/test_dialog_mutant_no_default_ring build/test_dialog_mutant_modal_passthrough build/test_dialog_mutant_local_hit build/.makefile.stamp`
```make
@printf ">>> test-dialog-mutant: confirming all nine mutants go RED (Rule 6; Law 4 for FILECOPY_MSG/TITLELESS_MODAL/PROGRESS_ZERO)\n"
@$(TEST_DIALOG_MUT_LOCAL_HIT) > $(TEST_DIALOG_MUT_LOCAL_HIT).log 2>&1; \
if grep -q 'FAIL.*release inside enabled ctrlItem' $(TEST_DIALOG_MUT_LOCAL_HIT).log; then printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_LOCAL_HIT correctly RED -- the click on the drawn OK button misses)\n'; \
else printf '!!! test-dialog-mutant FAIL: LOCAL_HIT mutant did not go RED on the click leg\n'; cat $(TEST_DIALOG_MUT_LOCAL_HIT).log; exit 1; fi
@if $(TEST_DIALOG_MUT_BORDER) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: BORDER mutant PASSED -- the border-width oracle is decoration\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_BORDER correctly RED)\n'; fi
@if $(TEST_DIALOG_MUT_STATIC) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: HIT_STATIC mutant PASSED -- the statText-exclusion oracle is decoration\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_HIT_STATIC correctly RED)\n'; fi
@if $(TEST_DIALOG_MUT_FILECOPY) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: FILECOPY_MSG mutant PASSED -- the canon-string oracle is decoration (Law 4 violated)\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_FILECOPY_MSG correctly RED -- Law 4 oracle bites)\n'; fi
@if $(TEST_DIALOG_MUT_TITLELESS) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: TITLELESS_MODAL mutant PASSED -- the moveable-titled-modal oracle is decoration (initech-zvo6)\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_TITLELESS_MODAL correctly RED -- Law 4 oracle bites)\n'; fi
@if $(TEST_DIALOG_MUT_PROGZERO) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: PROGRESS_ZERO mutant PASSED -- the progress-bar-fill oracle is decoration (initech-a90f)\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (DIALOG_MUTATE_PROGRESS_ZERO correctly RED -- Law 4 oracle bites)\n'; fi
@if $(TEST_DIALOG_MUT_NO_DEFAULT_RING) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: CTRL_MUT_NO_DEFAULT_RING PASSED -- Dialog Manager defaultItem is not proven to reach ring pixels\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (CTRL_MUT_NO_DEFAULT_RING correctly RED -- ring + moat wiring caught)\n'; fi
@if $(TEST_DIALOG_MUT_MODAL_PASS) >/dev/null 2>&1; then \
	printf '!!! test-dialog-mutant FAIL: MODAL_PASSTHROUGH PASSED -- dispatch-level true-modality oracle is decoration\n'; exit 1; \
else printf '>>> test-dialog-mutant: green (MODAL_PASSTHROUGH correctly RED -- outside click reached FindWindow)\n'; fi
@$(TEST_DIALOG_MUT_SLAB) > $(TEST_DIALOG_MUT_SLAB).log 2>&1; \
if grep -q 'FAIL.*ALERT-BANDS' $(TEST_DIALOG_MUT_SLAB).log; then printf '>>> test-dialog-mutant: green (DIALOG_MUT_ALERT_SLAB correctly RED: ALERT-BANDS)\n'; else cat $(TEST_DIALOG_MUT_SLAB).log; exit 1; fi
```

## test-dialog-record

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15973):
Prerequisites: `build/test_dialog_record build/.makefile.stamp`
```make
@printf ">>> test-dialog-record: DialogRecord + DITL item-type bytes + alert stages + ok/cancel=1/2 (spec/dialog_record.h)\n"
@$(TEST_DIALOG_RECORD)
@printf ">>> test-dialog-record: green\n"
```

## test-dialog-record-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15977):
Prerequisites: `build/test_dialog_record_mutant build/.makefile.stamp`
```make
@if $(TEST_DIALOG_RECORD_MUT) >/dev/null 2>&1; then printf '!!! test-dialog-record-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; else printf '>>> test-dialog-record-mutant: green (mutant correctly RED)\n'; fi
```

## test-dialog-tracking

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 45):
Prerequisites: `build/test_dialog_tracking build/.makefile.stamp`
```make
@$<
```

## test-dialog-tracking-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/finder_alerts.mk', line 55):
Prerequisites: `build/test_dialog_tracking_mutant build/.makefile.stamp`
```make
@if $< > $(BUILD)/dialog_tracking_mutant.log 2>&1; then printf '!!! ON_PRESS survived\n'; exit 1; fi
@grep 'FAIL.*RELEASE_INSIDE press' $(BUILD)/dialog_tracking_mutant.log
@printf '>>> test-dialog-tracking-mutant: DIALOG_MUT_ON_PRESS RED\n'
```

## test-drag

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14915):
Prerequisites: `build/test_drag build/.makefile.stamp`
```make
@printf ">>> test-drag: window drags w/ correct DiffRgn update regions, no over-repaint, chrome unchanged outside damage (D-5/AM-8)\n"
@$(TEST_DRAG) $(BUILD)/drag_before.ppm $(BUILD)/drag_after.ppm
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_DESKTOP_OPT) $(DRAG_INC) -c $(DESKTOP_C) -o $(BUILD)/desktop_freestanding.o \
	|| { printf '!!! test-drag FAIL: desktop.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-drag: green (wrote $(BUILD)/drag_before.ppm + $(BUILD)/drag_after.ppm)\n"
```

## test-drag-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14922):
Prerequisites: `build/test_drag_mutant_skip build/test_drag_mutant_noclip build/test_drag_mutant_nopaintallclear build/.makefile.stamp`
```make
@printf ">>> test-drag-mutant: confirming all three mutants go RED (Rule 6)\n"
@if $(TEST_DRAG_MUT_SKIP) >/dev/null 2>&1; then printf '!!! test-drag-mutant FAIL: SKIP_EXPOSED PASSED -- the no-over-repaint oracle is decoration\n'; exit 1; else printf '>>> test-drag-mutant: green (SKIP_EXPOSED correctly RED)\n'; fi
@if $(TEST_DRAG_MUT_NOCLIP) >/dev/null 2>&1; then printf '!!! test-drag-mutant FAIL: NO_CLIP PASSED -- the clip/over-repaint oracle is decoration\n'; exit 1; else printf '>>> test-drag-mutant: green (NO_CLIP correctly RED)\n'; fi
@if $(TEST_DRAG_MUT_NOPAINTALLCLEAR) >/dev/null 2>&1; then printf '!!! test-drag-mutant FAIL: NO_PAINTALL_CLEAR PASSED -- the stale-desktop_update oracle (initech-jmc5/-qi8v leg (e)) is decoration\n'; exit 1; else printf '>>> test-drag-mutant: green (NO_PAINTALL_CLEAR correctly RED -- the stale-stomp host proof bites, initech-jmc5/-qi8v)\n'; fi
```

## test-drawing-ops

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15986):
Prerequisites: `build/test_drawing_ops build/.makefile.stamp`
```make
@printf ">>> test-drawing-ops: QuickDraw verb/coord/CopyBits/pattern-phase op semantics (spec/drawing_ops.h)\n"
@$(TEST_DRAWING_OPS)
@printf ">>> test-drawing-ops: green\n"
```

## test-drawing-ops-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15990):
Prerequisites: `build/test_drawing_ops_mutant build/.makefile.stamp`
```make
@if $(TEST_DRAWING_OPS_MUT) >/dev/null 2>&1; then printf '!!! test-drawing-ops-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; else printf '>>> test-drawing-ops-mutant: green (mutant correctly RED)\n'; fi
```

## test-egg-jtc

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 18):
Prerequisites: `build/test_egg_jtc build/.makefile.stamp`
```make
@$<
```

## test-egg-jtc-cell-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 30):
Prerequisites: `test-egg-jtc build/test_egg_jtc_cell_mutant build/.makefile.stamp`
```make
@if $(BUILD)/test_egg_jtc_cell_mutant > $(BUILD)/egg_jtc_cell_mutant.log 2>&1; then printf '!!! EGG_MUT_CELL_WALK survived\n'; exit 1; fi
@grep -x 'FAIL JTC title' $(BUILD)/egg_jtc_cell_mutant.log
@printf '>>> test-egg-jtc-cell-mutant: EGG_MUT_CELL_WALK RED (title)\n'
```

## test-egg-jtc-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 20):
Prerequisites: `test-egg-jtc build/test_egg_jtc_mutant build/.makefile.stamp`
```make
@if $(BUILD)/test_egg_jtc_mutant > $(BUILD)/egg_jtc_mutant.log 2>&1; then printf '!!! EGG_MUT_PAD_MISSING survived\n'; exit 1; fi
@grep -x 'FAIL JTC pad count (expected 10)' $(BUILD)/egg_jtc_mutant.log
@printf '>>> test-egg-jtc-mutant: EGG_MUT_PAD_MISSING RED (pad count)\n'
```

## test-event

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14818):
Prerequisites: `build/test_event build/test_event_record build/.makefile.stamp`
```make
@printf ">>> test-event: ISR-enqueue SPSC ring + WaitNextEvent deterministic replay (raw trace -> EventRecord sequence, D-4/D-8)\n"
@$(TEST_EVENT)
@$(TEST_EVENT_RECORD)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_EVENT_OPT) $(EVENT_INC) -c os/flair/event.c -o $(BUILD)/event_freestanding.o \
	|| { printf '!!! test-event FAIL: event.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-event: green\n"
```

## test-event-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14826):
Prerequisites: `build/test_event_mutant_drop build/test_event_mutant_where build/test_event_mutant_set1 build/test_event_mutant_record build/.makefile.stamp`
```make
@printf ">>> test-event-mutant: confirming all four mutants go RED (Rule 6)\n"
@if $(TEST_EVENT_MUT_DROP) >/dev/null 2>&1; then printf '!!! test-event-mutant FAIL: DROP_SYNTH PASSED -- the synthesis oracle is decoration\n'; exit 1; else printf '>>> test-event-mutant: green (DROP_SYNTH correctly RED)\n'; fi
@if $(TEST_EVENT_MUT_WHERE) >/dev/null 2>&1; then printf '!!! test-event-mutant FAIL: STALE_WHERE PASSED -- the cursor-tracking oracle is decoration\n'; exit 1; else printf '>>> test-event-mutant: green (STALE_WHERE correctly RED)\n'; fi
@if $(TEST_EVENT_MUT_SET1) >/dev/null 2>&1; then printf '!!! test-event-mutant FAIL: SET1_BREAK_BLIND PASSED -- the modifier-release oracle (G11, initech-tdnl.58) is decoration\n'; exit 1; else printf '>>> test-event-mutant: green (SET1_BREAK_BLIND correctly RED -- released modifiers latch)\n'; fi
@if $(TEST_EVENT_MUT_RECORD) > $(BUILD)/test_event_mutant_record.log 2>&1; then printf '!!! test-event-mutant FAIL: RECORD_MUT_IGNORE_DBLCLICK_DEFINE survived\n'; exit 1; fi
@grep -q 'FAIL.*record double-click interval seeds 3000 ticks at init' $(BUILD)/test_event_mutant_record.log
@printf '>>> test-event-mutant: green (RECORD_MUT_IGNORE_DBLCLICK_DEFINE correctly RED -- record seed ignored)\n'
```

## test-event-owner-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/copybreak.mk', line 91):
Prerequisites: `build/test_event_owner_mut build/.makefile.stamp`
```make
@if $< >$(BUILD)/event_owner_mut.log 2>&1; then echo 'FAIL missing observer survived'; exit 1; fi
@grep 'FAIL.*ownership: observer' $(BUILD)/event_owner_mut.log
```

## test-film-scene

Declared tier: HOST; #  recipe to execute (from 'harness/film_scene.mk', line 7):
Prerequisites: `test-calc-spec build/test_film_scene build/.makefile.stamp`
```make
@$(BUILD)/test_film_scene
@printf '>>> test-film-scene: green (Calc content and real-window arrangement routing)\n'
```

## test-film-scene-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/film_scene.mk', line 28):
Prerequisites: `test-film-scene build/test_calc_tenant_mut_CALC_MUT_NO_PIE build/test_calc_tenant_mut_CALC_MUT_NO_LABELS build/test_calc_tenant_mut_CALC_MUT_MERGE_SLICES build/test_film_busy_mut build/.makefile.stamp`
```make
@set -e; for m in $(FILM_CALC_MUTANTS); do \
	case $$m in *NO_PIE) why=PIE_DRAW;; *NO_LABELS) why=PIE_LABELS;; *) why=PIE_FIVE;; esac; \
	if $(BUILD)/test_calc_tenant_mut_$$m > $(BUILD)/film_host_$$m.log 2>&1; then printf 'FAIL film mutant survived: %s\n' "$$m"; exit 1; fi; \
	grep -q "FAIL $$why" $(BUILD)/film_host_$$m.log; \
	printf '>>> test-film-scene-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL $$why" $(BUILD)/film_host_$$m.log)"; \
done
@if $(BUILD)/test_film_busy_mut > $(BUILD)/film_host_busy.log 2>&1; then printf 'FAIL busy mutant survived\n'; exit 1; fi
@grep -q 'FAIL BUSY_HOURGLASS' $(BUILD)/film_host_busy.log
@printf '>>> test-film-scene-mutant: CURSOR_MUT_BUSY_ARROW RED: %s\n' "$$(grep -m1 'FAIL BUSY_HOURGLASS' $(BUILD)/film_host_busy.log)"
@printf '>>> test-film-scene-mutant: green (four named mutants RED)\n'
```

## test-film-startup-screen

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/panic_reasons.mk', line 21):
Prerequisites: `test-panic-screen build/qemu_harness build/flair_live_interactive.img build/flair_data.img build/ppm_panic_check build/.makefile.stamp`
```make
@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) nodata nodata
@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) noapps noapps
@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) nocalc nocalc
@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) badcalc badcalc
```

## test-flair-close-terminate

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22831):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_solid_check build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name flair_close_terminate_pre --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-READY" --timeout-ms 15000 \
	2> "$(BUILD)/flair_close_terminate_pre.report" || true
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name flair_close_terminate_post --out "$(BUILD)" \
	--mouse "$(FLAIR_CLOSE_TERMINATE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-TENANT-EXIT name=HELLO" --timeout-ms 15000 \
	2> "$(BUILD)/flair_close_terminate_post.report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/flair_close_terminate_pre.report" "$(BUILD)/flair_close_terminate_post.report"; then printf '!!! test-flair-close-terminate FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-TENANT-EXIT name=HELLO$$' "$(BUILD)/flair_close_terminate_post.serial" \
	|| { printf '!!! test-flair-close-terminate FAIL: tenant-exit marker missing\n'; exit 1; }
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_close_terminate_pre.serial" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_close_terminate_post.serial" \
	|| { printf '!!! test-flair-close-terminate FAIL: mount marker missing\n'; exit 1; }
@$(PPM_FLAIR_SOLID_CHECK_BIN) F "$(BUILD)/flair_close_terminate_pre.ppm" "$(BUILD)/flair_close_terminate_post.ppm"
@printf '>>> test-flair-close-terminate: green -- HELLO terminated, NOTES active/intact, band 2 swapped\n'
```

## test-flair-close-terminate-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22852):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_solid_check build/flair_tenants_mut_close_hide_only.img build/.makefile.stamp`
```make
@$(call fixture-preflight,$(FLAIR_DATA_IMG))
cp -f $(FLAIR_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FLAIR_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FLAIR_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(FLAIRTENANTS_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name flair_close_terminate_pre --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-READY" --timeout-ms 15000 >/dev/null 2>&1 || true
@$(HARNESS_BIN) --disk "$(BUILD)/flair_tenants_mut_close_hide_only.img" --disk2 "$(FLAIR_GATE_DATA)" --expect FLAIR-FAT-MOUNT-OK --name flair_close_hide_only_post --out "$(BUILD)" \
	--mouse "$(FLAIR_CLOSE_TERMINATE_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-CLOSE win" --timeout-ms 15000 >/dev/null 2>&1 || true
@if grep -q '^FLAIR-TENANT-EXIT name=HELLO$$' "$(BUILD)/flair_close_hide_only_post.serial"; then printf '!!! CLOSE_HIDE_ONLY mutant unexpectedly emitted tenant exit\n'; exit 1; fi
@grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_close_terminate_pre.serial" \
	&& grep -qxF 'FLAIR-FAT-MOUNT-OK' "$(BUILD)/flair_close_hide_only_post.serial" \
	|| { printf '!!! test-flair-close-terminate-mutant: mount marker missing\n'; exit 1; }
@if $(PPM_FLAIR_SOLID_CHECK_BIN) F "$(BUILD)/flair_close_terminate_pre.ppm" "$(BUILD)/flair_close_hide_only_post.ppm" >/dev/null 2>&1; then printf '!!! CLOSE_HIDE_ONLY passed close-terminate pixels -- oracle decoration\n'; exit 1; fi
@printf '>>> test-flair-close-terminate-mutant: CLOSE_HIDE_ONLY correctly RED\n'
```

## test-flair-execguard

Declared tier: EMU; #  recipe to execute (from 'harness/emu/execguard.mk', line 121):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data_execguard.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-execguard : EXEC COMMAND.COM on the flagship (initech-13ig)\n'
@printf '  COMMAND /C runs the resident transient; the COMMAND3 overlay is never run as a .COM\n'
@printf '======================================================================\n'
$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXECGUARD_NAME),$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
@( $(call execguard-check,$(EXECGUARD_NAME)) ) || { printf '!!! test-flair-execguard FAIL: (above)\n'; grep -E '^(FLAIR-EXIT|COMMAND-|SHELL-|EXECGUARD-|LOADER-REFUSE|PANIC|HALTED)' $(BUILD)/$(EXECGUARD_NAME).serial | head -20; exit 1; }
@printf '>>> test-flair-execguard: EXEC COMMAND.COM " /C ECHO" -> the transient secondary processor echoed, rc 0; EXEC CMD2.COM (the COMMAND3 overlay) -> %s, ax=000B; no PANIC\n' "$$(grep -m1 '^LOADER-REFUSE-TRANSIENT' $(BUILD)/$(EXECGUARD_NAME).sx)"
@printf '>>> test-flair-execguard: green (QEMU; Bochs leg owed)\n'
```

## test-flair-execguard-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/execguard.mk', line 131):
Prerequisites: `build/qemu_harness build/flair_execguard_mut_no_comspec.img build/flair_execguard_mut_noguard.img build/flair_execguard_mut_skew.img build/flair_data_execguard.img build/.makefile.stamp`
```make
$(call exitdos-boot,$(EXECGUARD_MUT_NOCOMSPEC_IMG),flair_execguard_mut_nocomspec,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
@out=$$( ( $(call execguard-check,flair_execguard_mut_nocomspec) ) 2>&1 ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC PASSED -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -qF 'the COMMAND /C child never echoed' || { printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
grep -qxF 'EXECGUARD-COMSPEC-ERR ax=0002' $(BUILD)/flair_execguard_mut_nocomspec.sx || { printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC did not fail the EXEC with 0002h (COMMAND.COM absent on A:)\n'; exit 1; }; \
printf '>>> test-flair-execguard-mutant: NO_COMSPEC correctly RED (%s)\n' "$$out"
$(call exitdos-boot,$(EXECGUARD_MUT_NOGUARD_IMG),flair_execguard_mut_noguard,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
@out=$$( ( $(call execguard-check,flair_execguard_mut_noguard) ) 2>&1 ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD PASSED -- decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -qE 'did not recognise the COMMAND3 overlay|PANIC|HALTED' || { printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
! grep -qxF 'LOADER-REFUSE-TRANSIENT name=CMD2.COM' $(BUILD)/flair_execguard_mut_noguard.sx || { printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD still refused the overlay\n'; exit 1; }; \
printf '>>> test-flair-execguard-mutant: NO_GUARD correctly RED (%s)\n' "$$out"
$(call exitdos-boot,$(EXECGUARD_MUT_SKEW_IMG),flair_execguard_mut_skew,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),HALTED,0,40000)
@out=$$( ( $(call execguard-check,flair_execguard_mut_skew) ) 2>&1 ); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW PASSED -- the import check is decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -qF 'never handed over to the prompt' || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
s=$(BUILD)/flair_execguard_mut_skew.sx; k=$$(sed -n 's/^PANIC exit-to-dos: transient linked against another kernel build v=//p' $$s | head -1); \
[ -n "$$k" ] || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW was not refused as linked against another kernel build: %s\n' "$$(grep -m1 '^PANIC' $$s)"; exit 1; }; \
last=$$(( $$(wc -l < $(TRANSIENT_IMPORTS_LD).keep) - 1 )); \
[ "$$(printf '%d' "0x$$k")" -eq $$last ] && [ $$last -ge 5 ] || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW refused on import %s, expected the last (%s, beyond the old five)\n' "$$k" "$$last"; exit 1; }; \
grep -qxF 'Bad or missing Command Interpreter' $$s && ! grep -qxF 'SHELL-READY' $$s || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW did not halt with MSG-DOS-0014 before the prompt\n'; exit 1; }; \
printf '>>> test-flair-execguard-mutant: IMPORT_SKEW correctly RED (refused on import index %s of %s: %s)\n' "$$k" "$$(( last + 1 ))" "$$(sed -n 's/^\(PANIC exit-to-dos: transient linked[^v]*\).*/\1/p' $$s | head -1)"
@printf '>>> test-flair-execguard-mutant: green (3/3 RED for the named reason)\n'
```

## test-flair-heap

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12693):
Prerequisites: `build/test_flair_heap build/.makefile.stamp`
```make
@printf ">>> test-flair-heap: FLAIR heap allocator -- bump monotonicity/align + typed free-list reuse + class isolation + fail-loud exhaustion + determinism + data-integrity fuzz\n"
@$(TEST_FLAIR_HEAP)
@printf ">>> test-flair-heap: green\n"
```

## test-flair-heap-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12698):
Prerequisites: `build/test_flair_heap_mutant_bounds build/test_flair_heap_mutant_reuse build/.makefile.stamp`
```make
@printf ">>> test-flair-heap-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_FLAIR_HEAP_MUT_BOUNDS) >/dev/null 2>&1; then \
	printf '!!! test-flair-heap-mutant FAIL: NO_BOUNDS mutant PASSED -- the exhaustion oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-flair-heap-mutant: green (NO_BOUNDS mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_FLAIR_HEAP_MUT_REUSE) >/dev/null 2>&1; then \
	printf '!!! test-flair-heap-mutant FAIL: NO_REUSE mutant PASSED -- the free-list-reuse oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-flair-heap-mutant: green (NO_REUSE mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-flair-heap-ram

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28785):
Prerequisites: `build/test_flair_heap_ram build/.makefile.stamp`
```make
@printf '>>> test-flair-heap-ram: flair_heap_ram_ok() FAIL<min / PASS>=min (ADR-0004 DEC-03 / FO-G)\n'
@$(TEST_FLAIR_RAM)
```

## test-flair-heap-ram-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28788):
Prerequisites: `build/test_flair_heap_ram_mut build/.makefile.stamp`
```make
@if $(TEST_FLAIR_RAM_MUT) >/dev/null 2>&1; then printf '!!! test-flair-heap-ram-mutant FAIL: boundary mutant PASSED -- oracle is decoration\n'; exit 1; \
else printf '>>> test-flair-heap-ram-mutant: green (boundary mutant correctly RED)\n'; fi
```

## test-flair-modal-block

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 22873):
Prerequisites: `build/qemu_harness build/flair_live.img build/.makefile.stamp`
```make
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" --name flair_modal_block_pre --out "$(BUILD)" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
	2> "$(BUILD)/flair_modal_block_pre.report" || true
@$(HARNESS_BIN) --disk "$(FLAIRLIVE_IMG)" --name flair_modal_block_post --out "$(BUILD)" \
	--mouse "$(FLAIR_MODAL_BLOCK_SPEC)" --keys-after "FLAIR-LIVE-READY" \
	--screendump --screendump-after "FLAIR-LIVE-OK" --timeout-ms 15000 \
	2> "$(BUILD)/flair_modal_block_post.report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/flair_modal_block_pre.report" "$(BUILD)/flair_modal_block_post.report"; then printf '!!! test-flair-modal-block FAIL: TRIPLE FAULT\n'; exit 1; fi
@grep -q '^FLAIR-MODAL-BLOCK where=100,100$$' "$(BUILD)/flair_modal_block_post.serial" \
	|| { printf '!!! test-flair-modal-block FAIL: block marker missing\n'; exit 1; }
@if grep -q '^FLAIR-DRAG ' "$(BUILD)/flair_modal_block_post.serial"; then printf '!!! test-flair-modal-block FAIL: click behind modal dispatched a drag\n'; exit 1; fi
@cmp -s "$(BUILD)/flair_modal_block_pre.ppm" "$(BUILD)/flair_modal_block_post.ppm" \
	|| { printf '!!! test-flair-modal-block FAIL: blocked click changed/raised the back-window scene\n'; exit 1; }
@printf '>>> test-flair-modal-block: green -- outside click swallowed before FindWindow; scene unchanged\n'
```

## test-flair-toolbox-hardening

Declared tier: EMU; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 72):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_flair_scroll_look_check build/.makefile.stamp`
```make
$(call scroll-look-boot,$(FLAIRTENANTS_IMG),toolbox_hardening,$(FLAIR_TOOLBOX_HARDENING_SPEC))
@grep -qx 'FLAIR-SCROLL win 0 h part=21 value=16 max=171' $(BUILD)/toolbox_hardening.serial
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/toolbox_hardening.serial
@$(SCROLL_LOOK_CHECK) click $(BUILD)/toolbox_hardening.ppm
@printf '>>> test-flair-toolbox-hardening: green (locked trace; independent Mac OS 8.1 crops)\n'
```

## test-flair-toolbox-hardening-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 223):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-toolbox-hardening-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-flair-toolbox-hardening-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/flair-toolbox-hardening_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/flair-toolbox-hardening_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "flair-toolbox-hardening_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/flair-toolbox-hardening_bochs.report.txt" || true
@cat "$(BUILD)/flair-toolbox-hardening_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-flair-toolbox-hardening-bochs "$(BUILD)/flair-toolbox-hardening_bochs.report.txt" "$(BUILD)/flair-toolbox-hardening_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-flair-toolbox-hardening-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 78):
Prerequisites: `test-flair-scroll-look-mutant build/.makefile.stamp`
```make
@printf '>>> test-flair-toolbox-hardening-mutant: green (shared independent crop mutants RED)\n'
```

## test-font-styles

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13367):
Prerequisites: `build/test_font_styles build/.makefile.stamp`
```make
@$(TEST_FONT_STYLES)
```

## test-font-styles-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13374):
Prerequisites: `build/test_font_styles_mut_SMEAR_OFF build/test_font_styles_mut_BOLD_ADVANCE_PLAIN build/test_font_styles_mut_UL_OFFSET build/test_font_styles_mut_ITALIC_RUN4 build/.makefile.stamp`
```make
@$(call font-styles-bites,SMEAR_OFF,FAIL: G1 'Kind:' (s7_get_info.png pen_x 57 top 105))
@$(call font-styles-bites,BOLD_ADVANCE_PLAIN,FAIL: G1 'Kind:' (s7_get_info.png): bold StringWidth 23)
@$(call font-styles-bites,UL_OFFSET,FAIL: R underline (font 0): adds)
@$(call font-styles-bites,ITALIC_RUN4,FAIL: I row d=4 above the baseline: artifact shear 1)
```

## test-geneva12-art

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 28):
Prerequisites: `build/geneva12_art2h spec/assets/geneva12.art spec/assets/geneva12.h build/.makefile.stamp | build/geneva12`
```make
@$(GENEVA12_ART2H) $(GENEVA12_ART) > $(GENEVA12_DIR)/regen.h
@cmp -s $(GENEVA12_DIR)/regen.h $(GENEVA12_H) \
	|| { printf '!!! test-geneva12-art FAIL: $(GENEVA12_H) is not the transcription of $(GENEVA12_ART) (run make gen-geneva12)\n'; exit 1; }
@printf '>>> test-geneva12-art: green -- $(GENEVA12_H) == transcribe($(GENEVA12_ART))\n'
```

## test-geneva12-art-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 37):
Prerequisites: `build/geneva12_art2h spec/assets/geneva12.art spec/assets/geneva12.h build/.makefile.stamp | build/geneva12`
```make
@awk '/^glyph 0x53 /{g=1;r=0;print;next} /^glyph /{g=0} g&&!/^;/{if(r==2){$$0="." "#" substr($$0,3)} r++} {print}' $(GENEVA12_ART) > $(GENEVA12_DIR)/mut1.art
@cmp -s $(GENEVA12_DIR)/mut1.art $(GENEVA12_ART) && { printf '!!! test-geneva12-art-mutant FAIL: the pixel flip did not apply\n'; exit 1; } || true
@$(GENEVA12_ART2H) $(GENEVA12_DIR)/mut1.art > $(GENEVA12_DIR)/mut1.h
@if cmp -s $(GENEVA12_DIR)/mut1.h $(GENEVA12_H); then printf '!!! test-geneva12-art-mutant FAIL: a flipped pixel left the header unchanged -- the sync check is decoration\n'; exit 1; fi
@printf '>>> test-geneva12-art-mutant: (a) green -- one flipped pixel in S makes the header differ\n'
@awk '/^glyph 0x53 /{g=1;r=0;print;next} /^glyph /{g=0} g&&!/^;/{$$0="." substr($$0,1,length($$0)-1); r++} {print}' $(GENEVA12_ART) > $(GENEVA12_DIR)/mut2.art
@if $(GENEVA12_ART2H) $(GENEVA12_DIR)/mut2.art > $(GENEVA12_DIR)/mut2.h 2> $(GENEVA12_DIR)/mut2.log; then printf '!!! test-geneva12-art-mutant FAIL: art shifted off its NFNT bearing was accepted\n'; exit 1; fi
@grep -q 'leftmost ink column != lb' $(GENEVA12_DIR)/mut2.log || { cat $(GENEVA12_DIR)/mut2.log; printf '!!! test-geneva12-art-mutant FAIL: refused for the wrong reason\n'; exit 1; }
@printf '>>> test-geneva12-art-mutant: (b) green -- art whose ink disagrees with the NFNT lb is refused (%s)\n' "$$(cat $(GENEVA12_DIR)/mut2.log)"
```

## test-geneva12-metrics

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 71):
Prerequisites: `build/geneva12/metrics build/.makefile.stamp`
```make
@$<
```

## test-geneva12-metrics-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 73):
Prerequisites: `test-geneva12-metrics test-geneva12-art-mutant harness/proptest/test_geneva12_metrics.c os/flair/text.c os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/surface.c os/flair/surface.h harness/oracle_policy.h build/.corpus-config build/.makefile.stamp`
```make
@mkdir -p $(GENEVA12_DIR)/mutinc && cp -f $(GENEVA12_DIR)/mut1.h $(GENEVA12_DIR)/mutinc/geneva12.h
$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -I$(GENEVA12_DIR)/mutinc $(TEXT_INC) $(G12MET_DEF) -o $(GENEVA12_DIR)/metrics-mut harness/proptest/test_geneva12_metrics.c os/flair/text.c os/flair/surface.c
@if $(GENEVA12_DIR)/metrics-mut > $(GENEVA12_DIR)/metrics-mut.log 2>&1; then printf '!!! test-geneva12-metrics-mutant FAIL: a flipped S pixel PASSED -- the ink-box oracle is decoration\n'; exit 1; fi
@grep -q "C ink box 'S' (0x53): drawn x 1..6 y 2..11, NFNT 12112 x 1..6 y 3..11" $(GENEVA12_DIR)/metrics-mut.log \
	|| { tail -5 $(GENEVA12_DIR)/metrics-mut.log; printf '!!! test-geneva12-metrics-mutant FAIL: RED, but not for the flipped S pixel\n'; exit 1; }
@printf '>>> test-geneva12-metrics-mutant: green (S row-2 pixel RED: %s)\n' "$$(grep -m1 "ink box 'S'" $(GENEVA12_DIR)/metrics-mut.log | sed 's/^ *//')"
```

## test-geneva12-pack

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 54):
Prerequisites: `build/geneva12_packed_data.h build/.makefile.stamp | build/geneva12`
```make
$(CC) $(CFLAGS) -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_geneva12_pack.c -o $(GENEVA12_DIR)/pack
@$(GENEVA12_DIR)/pack
```

## test-geneva12-pack-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 57):
Prerequisites: `test-geneva12-pack build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DGENEVA12_PACK_MUT_ROW -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_geneva12_pack.c -o $(GENEVA12_DIR)/pack-mut
@if $(GENEVA12_DIR)/pack-mut >$(GENEVA12_DIR)/pack-mut.log 2>&1; then \
	printf '!!! test-geneva12-pack-mutant FAIL: packed_row survived\n'; exit 1; fi
@grep '^!!! geneva12-pack FAIL:' $(GENEVA12_DIR)/pack-mut.log
@printf '>>> test-geneva12-pack-mutant: green (GENEVA12_PACK_MUT_ROW RED)\n'
```

## test-grow-box

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34567):
Prerequisites: `test-chrome-fidelity test-window test-finder-ops build/.makefile.stamp`
```make
@printf '>>> test-grow-box: green (324/324 active/inactive PNG pixels, both depths; noGrow absence; routing, bounds, spatial Finder content)\n'
```

## test-grow-box-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34569):
Prerequisites: `test-grow-box build/test_grow_box_mut_CHROME_MUT_GROW_FIGURE_SWAP build/test_grow_box_mut_CHROME_MUT_GROW_NO_REGION build/test_grow_box_mut_CHROME_MUT_GROW_OMIT build/test_grow_box_mut_CHROME_MUT_GROW_INACTIVE_ACTIVE build/test_grow_box_no_commit build/test_window_mutant_grow_no_min build/.makefile.stamp`
```make
@for m in $(GROW_BOX_CHROME_MUTANTS); do \
	if $(BUILD)/test_grow_box_mut_$$m > $(BUILD)/grow_box_$$m.log 2>&1; then printf 'FAIL grow-box mutant survived: %s\n' "$$m"; exit 1; fi; \
	case "$$m" in *NO_REGION) leg='GROW-NO-REGION';; *INACTIVE_ACTIVE) leg='CAPTURE-GROW-INACTIVE';; *) leg='CAPTURE-GROW-ACTIVE';; esac; \
	grep -Eq "$$leg +RED" $(BUILD)/grow_box_$$m.log || { cat $(BUILD)/grow_box_$$m.log; exit 1; }; \
	printf '>>> test-grow-box-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 -E "$$leg +RED" $(BUILD)/grow_box_$$m.log)"; \
done
@for m in no_commit no_min; do \
	case "$$m" in no_commit) bin=$(BUILD)/test_grow_box_no_commit; knob=WINDOW_MUT_GROW_NO_COMMIT;; no_min) bin=$(TEST_WINDOW_MUT_GROW_NO_MIN); knob=WINDOW_MUTATE_GROW_NO_MIN;; esac; \
	if $$bin > $(BUILD)/grow_box_$$m.log 2>&1; then printf 'FAIL grow-box mutant survived: %s\n' "$$knob"; exit 1; fi; \
	grep -q 'FAIL.*SizeWindow: 1x1 request clamps' $(BUILD)/grow_box_$$m.log || { cat $(BUILD)/grow_box_$$m.log; exit 1; }; \
	printf '>>> test-grow-box-mutant: %s correctly RED: %s\n' "$$knob" "$$(grep -m1 'FAIL.*SizeWindow: 1x1 request clamps' $(BUILD)/grow_box_$$m.log)"; \
done
@printf '>>> test-grow-box-mutant: green (six named mutants RED)\n'
```

## test-input-handoff

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/copybreak.mk', line 125):
Prerequisites: `build/qemu_harness build/flair_tenants_mut_input_handoff.img build/launchkey_data.img build/.makefile.stamp`
```make
cp -f $(LAUNCHKEY_DATA) $(BUILD)/$(HANDOFF_TAG)_data.img
@$(HARNESS_BIN) --disk $(HANDOFF_IMAGE) --disk2 $(BUILD)/$(HANDOFF_TAG)_data.img --name $(HANDOFF_TAG) --out $(BUILD) --mouse '$(FLAIR_LAUNCHKEY_RESUME_SPEC)' --keys-after FLAIR-LIVE-READY --quit-after '$(HANDOFF_QUIT)' --timeout-ms 40000 2>$(BUILD)/$(HANDOFF_TAG).report || true
@grep -qx 'FLAIR-HANDOFF-QUEUED' $(BUILD)/$(HANDOFF_TAG).serial
@$(call LAUNCHKEY_RECOVERY,$(BUILD)/$(HANDOFF_TAG).serial) || { echo 'RED input-handoff: queued QUIT did not resume the sheet'; exit 1; }
@echo 'GREEN input-handoff: queued QUIT resumes the sheet'
```

## test-input-handoff-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/copybreak.mk', line 131):
Prerequisites: `build/flair_tenants_mut_no_input_replay.img build/.makefile.stamp`
```make
@if $(MAKE) --no-print-directory test-input-handoff HANDOFF_IMAGE=$(BUILD)/flair_tenants_mut_no_input_replay.img HANDOFF_TAG=handoff_mut HANDOFF_QUIT=FLAIR-HANDOFF-QUEUED >$(BUILD)/input_handoff_mut.log 2>&1; then echo 'FAIL no-replay mutant survived'; exit 1; fi
@grep -Fx 'RED input-handoff: queued QUIT did not resume the sheet' $(BUILD)/input_handoff_mut.log
@grep -qx 'FLAIR-HANDOFF-QUEUED' $(BUILD)/handoff_mut.serial
@! grep -q '^FLAIR-RESUME' $(BUILD)/handoff_mut.serial
```

## test-input-owner-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/copybreak.mk', line 56):
Prerequisites: `build/test_kbd_dual_ring build/test_conin_dual_ring build/test_conin_owner_pb build/.makefile.stamp`
```make
@set -eu; for t in kbd conin; do \
 if $(BUILD)/test_$${t}_dual_ring >$(BUILD)/$${t}_dual_ring.log 2>&1; then echo 'FAIL dual-ring mutant survived'; exit 1; fi; \
 grep 'FAIL.*ownership: GUI' $(BUILD)/$${t}_dual_ring.log; done
@if $(BUILD)/test_conin_owner_pb >$(BUILD)/conin_owner_pb.log 2>&1; then echo 'FAIL old lookahead survived handoff'; exit 1; fi
@grep 'FAIL.*ownership: GUI entry clears DOS lookahead' $(BUILD)/conin_owner_pb.log
```

## test-interact

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14244):
Prerequisites: `build/test_interact build/.makefile.stamp`
```make
@printf ">>> test-interact: FindWindow part-codes + DragWindow geometry + z-order visible regions (HOST, independent-by-recomputation, FO-9)\n"
@$(TEST_INTERACT)
@printf ">>> test-interact: green\n"
```

## test-interact-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14249):
Prerequisites: `build/test_interact_mutant_dragnoop build/test_interact_mutant_findoffbyone build/test_interact_mutant_visignorefront build/.makefile.stamp`
```make
@printf ">>> test-interact-mutant: confirming all three self-mutants go RED (Rule 6)\n"
@if $(TEST_INTERACT_MUT_DRAG) >/dev/null 2>&1; then printf '!!! test-interact-mutant FAIL: DRAG_NOOP PASSED -- the drag-geometry oracle is decoration\n'; exit 1; else printf '>>> test-interact-mutant: green (DRAG_NOOP correctly RED)\n'; fi
@if $(TEST_INTERACT_MUT_FIND) >/dev/null 2>&1; then printf '!!! test-interact-mutant FAIL: FINDWINDOW_OFFBYONE PASSED -- the hit-test oracle is decoration\n'; exit 1; else printf '>>> test-interact-mutant: green (FINDWINDOW_OFFBYONE correctly RED)\n'; fi
@if $(TEST_INTERACT_MUT_VIS) >/dev/null 2>&1; then printf '!!! test-interact-mutant FAIL: VISIBLE_IGNORE_FRONT PASSED -- the visible-region oracle is decoration\n'; exit 1; else printf '>>> test-interact-mutant: green (VISIBLE_IGNORE_FRONT correctly RED)\n'; fi
```

## test-kernel-abi-struct-return

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r7.mk', line 29):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/ksize7`
```make
@set -eu; $(call kernel-abi-struct-audit,,real)
@n=$$(objdump -d -M intel $(KERNEL_FLAIRTENANTS_ELF) | grep -c 'and    esp,0xfffffff8' || true); \
if [ "$$n" -ne 0 ]; then printf '!!! kernel-abi-struct-return FAIL: %s dynamic stack realignments in the flagship\n' "$$n"; exit 1; fi
@printf '>>> test-kernel-abi-struct-return: green (one struct-return convention on every kernel object + the link; 0 stack realignments)\n'
```

## test-kernel-abi-struct-return-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r7.mk', line 35):
Prerequisites: `test-kernel-abi-struct-return build/.makefile.stamp`
```make
@if ( set -eu; $(call kernel-abi-struct-audit,FLAIR_REGION_OPT='-Oz -fomit-frame-pointer -fpcc-struct-return',mutant) ) > $(KERNEL_R7_DIR)/mutant.log 2>&1; then \
	printf '!!! test-kernel-abi-struct-return-mutant FAIL: region_pcc survived\n'; exit 1; fi
@grep -x '!!! kernel-abi-struct-return FAIL: not -freg-struct-return: os/flair/atkinson/region.c' $(KERNEL_R7_DIR)/mutant.log
@printf '>>> test-kernel-abi-struct-return-mutant: green (region_pcc RED)\n'
```

## test-kernel-cold-about

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 168):
Prerequisites: `build/.makefile.stamp | build/ksize4`
```make
$(CC) $(CFLAGS) -Ios/milton harness/proptest/test_kmain_cold.c os/milton/kmain_cold.c -o $(KERNEL_R4_DIR)/cold-about
@$(KERNEL_R4_DIR)/cold-about
```

## test-kernel-cold-about-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 171):
Prerequisites: `test-kernel-cold-about build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DKMAIN_ABOUT_MUT_NONE -Ios/milton harness/proptest/test_kmain_cold.c os/milton/kmain_cold.c -o $(KERNEL_R4_DIR)/cold-about-mut
@if $(KERNEL_R4_DIR)/cold-about-mut >$(KERNEL_R4_DIR)/cold-about-mut.log 2>&1; then \
	printf '!!! test-kernel-cold-about-mutant FAIL: about_none survived\n'; exit 1; fi
@grep '^!!! cold-about FAIL:' $(KERNEL_R4_DIR)/cold-about-mut.log
@printf '>>> test-kernel-cold-about-mutant: green (about_none RED)\n'
```

## test-kernel-cold-lto

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 124):
Prerequisites: `build/.makefile.stamp | build/kernel_gc`
```make
@set -eu; for unit in entry answer; do \
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_COLD_LTO_FLAGS) -c harness/proptest/kernel_cold_$$unit.c -o $(KGC_DIR)/cold_$$unit.o; \
done
$(KERNEL_LINK) -o $(KGC_DIR)/cold.elf $(KGC_DIR)/cold_entry.o $(KGC_DIR)/cold_answer.o
@$(call kernel-cold-grade,$(KGC_DIR)/cold.elf)
@printf '>>> test-kernel-cold-lto: green (cross-TU call folded to 22; entry fixed)\n'
```

## test-kernel-cold-lto-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 131):
Prerequisites: `test-kernel-cold-lto build/.makefile.stamp`
```make
@set -eu; for unit in entry answer; do \
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Oz -c harness/proptest/kernel_cold_$$unit.c -o $(KGC_DIR)/cold_native_$$unit.o; \
done
$(KERNEL_LINK) -o $(KGC_DIR)/cold_no_lto.elf $(KGC_DIR)/cold_native_entry.o $(KGC_DIR)/cold_native_answer.o
@if ( $(call kernel-cold-grade,$(KGC_DIR)/cold_no_lto.elf) ) >$(KGC_DIR)/cold_no_lto.log 2>&1; then \
	printf '!!! test-kernel-cold-lto-mutant FAIL: no_lto survived\n'; exit 1; fi
@grep '!!! kernel-cold-lto FAIL: cross-object callee retained' $(KGC_DIR)/cold_no_lto.log
@printf '>>> test-kernel-cold-lto-mutant: green (no_lto RED)\n'
```

## test-kernel-df

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 61):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/ksize4`
```make
@sh harness/proptest/kernel_df_check.sh $< $(KERNEL_R4_DIR)/df.dis
@printf '>>> test-kernel-df: green (linked entry/resume CLD contract; static proof only)\n'
```

## test-kernel-df-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 64):
Prerequisites: `test-kernel-df build/.makefile.stamp`
```make
@cp -f $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/df-mutant.elf
@set -eu; addr=$$(awk '/<irq1_entry>:/ {active=1; next} active && /cld/ {sub(/:$$/,"",$$1); print $$1; exit}' $(KERNEL_R4_DIR)/df.dis); \
	set -- $$(objdump -h $(KERNEL_R4_DIR)/df-mutant.elf | awk '$$2==".text" {print $$4, $$6}'); \
	test -n "$$addr"; test "$$#" -eq 2; \
	printf '\220' | dd of=$(KERNEL_R4_DIR)/df-mutant.elf bs=1 seek=$$(( 0x$$addr - 0x$$1 + 0x$$2 )) conv=notrunc status=none
@if sh harness/proptest/kernel_df_check.sh $(KERNEL_R4_DIR)/df-mutant.elf $(KERNEL_R4_DIR)/df-mutant.dis >$(KERNEL_R4_DIR)/df-mutant.log 2>&1; then \
	printf '!!! test-kernel-df-mutant FAIL: missing_irq1_cld survived\n'; exit 1; fi
@grep '^!!! kernel-df FAIL: irq1_entry calls C before cld' $(KERNEL_R4_DIR)/df-mutant.log
@printf '>>> test-kernel-df-mutant: green (real ELF missing_irq1_cld RED)\n'
```

## test-kernel-gc

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 29):
Prerequisites: `build/kernel_gc/probe.elf test-kernel-gc-lto build/.makefile.stamp`
```make
@$(call kernel-gc-grade,$<)
@printf '>>> test-kernel-gc: green (entry/live code/data retained; dead code/data removed)\n'
```

## test-kernel-gc-emu

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 57):
Prerequisites: `test-boot test-flair-appswitch test-fcb-api build/.makefile.stamp`
```make
@printf '>>> test-kernel-gc-emu: green (boot/desktop pixels/FCB transcript and FAT differential)\n'
```

## test-kernel-gc-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 75):
Prerequisites: `test-flair-appswitch-mutant test-fcb-api-mutant build/.makefile.stamp`
```make
@printf '>>> test-kernel-gc-emu-mutant: green (desktop and FCB behaviour mutants rejected)\n'
```

## test-kernel-gc-lto

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 154):
Prerequisites: `build/kernel_gc/lto-probe.elf build/.makefile.stamp`
```make
@$(call kernel-gc-grade,$<)
@printf '>>> test-kernel-gc-lto: green (dead native reference discarded after LTO; entry fixed)\n'
```

## test-kernel-gc-lto-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 158):
Prerequisites: `test-kernel-gc-lto build/.makefile.stamp`
```make
$(filter-out $(KERNEL_LTO_SECTION_FLAGS),$(KERNEL_LINK)) -o $(KGC_DIR)/lto-no-sections.elf $(KGC_DIR)/lto-probe.o $(KGC_DIR)/native-dead.o
@if ( $(call kernel-gc-grade,$(KGC_DIR)/lto-no-sections.elf) ) >$(KGC_DIR)/lto-no-sections.log 2>&1; then \
	printf '!!! test-kernel-gc-lto-mutant FAIL: no_lto_sections survived\n'; exit 1; fi
@grep '!!! kernel-gc FAIL: dead gc_dead_function retained' $(KGC_DIR)/lto-no-sections.log
@printf '>>> test-kernel-gc-lto-mutant: green (no_lto_sections RED)\n'
```

## test-kernel-gc-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 33):
Prerequisites: `test-kernel-gc build/.makefile.stamp`
```make
@set -eu; for mutant in no_gc no_sections; do \
	case $$mutant in \
		no_gc) flags='$(KERNEL_SECTION_FLAGS)'; ldflags=;; \
		no_sections) flags=; ldflags='$(KERNEL_GC_LDFLAGS)';; \
	esac; \
	$(KERNEL_CC) $(KERNEL_CFLAGS) $$flags -Os -c $(KGC_SRC) -o $(KGC_DIR)/$$mutant.o; \
	$(LD) $$ldflags -m elf_i386 -T $(KERNEL_LD) -o $(KGC_DIR)/$$mutant.elf $(KGC_DIR)/$$mutant.o 2>$(KGC_DIR)/$$mutant.link; \
	if ( $(call kernel-gc-grade,$(KGC_DIR)/$$mutant.elf) ) >$(KGC_DIR)/$$mutant.log 2>&1; then \
		printf '!!! test-kernel-gc-mutant FAIL: %s survived\n' "$$mutant"; exit 1; \
	fi; \
	grep -q 'kernel-gc FAIL: dead gc_dead_function retained' $(KGC_DIR)/$$mutant.log; \
	printf '    %s: ' "$$mutant"; cat $(KGC_DIR)/$$mutant.log; \
done
@printf '>>> test-kernel-gc-mutant: green (no_gc/no_sections both RED for dead code)\n'
```

## test-kernel-gc-roots

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 64):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/kernel_gc`
```make
@nm --defined-only $< > $(KGC_DIR)/roots
@set -eu; for s in _start kernel_main g_idt g_idt_ptr g_exc_stubs \
	int20_entry int21_entry int22_entry int23_entry int24_entry int25_entry int26_entry \
	irq0_entry irq1_entry irq12_entry isr_spurious isr_irq7_spurious isr_irq15_spurious \
	isr_selftest tbx_gate_entry $$(seq 0 31 | sed 's/^/isr/'); do \
	grep -q " $$s$$" $(KGC_DIR)/roots || { printf '!!! kernel-gc-roots FAIL: %s removed\n' "$$s"; exit 1; }; \
done
@grep -q '^00500000 .* _start$$' $(KGC_DIR)/roots
@printf '>>> test-kernel-gc-roots: green (entry, IDT, all exception/IRQ/DOS/Toolbox stubs retained)\n'
```

## test-kernel-headroom

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 465):
Prerequisites: `build/.makefile.stamp`
```make
@$(kernel-headroom-enum); \
for b in $$bins; do case ' $(KERNEL_HEADROOM_BINS) ' in *" $$b "*) ;; \
	*) printf '!!! test-kernel-headroom: %s enumerated at run time but not a prerequisite (KERNEL_HEADROOM_BINS)\n' "$$b"; exit 1;; esac; done; \
elfs=$$(for b in $$bins; do printf '%s ' "$${b%.bin}.elf"; done); \
$(call kernel-headroom-measure,test-kernel-headroom)
@printf '>>> test-kernel-headroom: green\n'
```

## test-kernel-headroom-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 485):
Prerequisites: `test-kernel-headroom build/.makefile.stamp`
```make
@$(kernel-headroom-enum); \
elfs0=$$(for b in $$bins; do printf '%s ' "$${b%.bin}.elf"; done); \
win=$$(( $(KERNEL_SECTORS) * 512 )); \
$(OBJCOPY) -O binary $(KERNEL_FLAIRTENANTS_ELF) $(BUILD)/.khr_flag.tmp; \
flag=$$(wc -c < $(BUILD)/.khr_flag.tmp); rm -f $(BUILD)/.khr_flag.tmp; \
for want in $$(( $(KERNEL_HEADROOM_FLOOR) - 1 )) $(KERNEL_HEADROOM_FLOOR); do \
	pad=$$(( win - flag - want )); \
	[ "$$pad" -gt 0 ] || { printf '!!! test-kernel-headroom-mutant: flagship already within the floor (pad=%s)\n' "$$pad"; exit 1; }; \
	printf 'section .data align=1\nglobal khr_headroom_pad\nkhr_headroom_pad:\ntimes %d db 0x5A\n' "$$pad" > $(KHR_MUT_ASM); \
	$(NASM) -f elf32 $(KHR_MUT_ASM) -o $(KHR_MUT_OBJ) || exit 1; \
	$(KERNEL_LINK) -u khr_headroom_pad -o $(KHR_MUT_ELF) $(KERNEL_FLAIRTENANTS_OBJS) $(KHR_MUT_OBJ) @$(TRANSIENT_IMPORTS_LD) 2>/dev/null || exit 1; \
	$(OBJCOPY) -O binary $(KHR_MUT_ELF) $(BUILD)/.khr_mut.tmp; \
	got=$$(( win - $$(wc -c < $(BUILD)/.khr_mut.tmp) )); rm -f $(BUILD)/.khr_mut.tmp; \
	[ "$$got" -eq "$$want" ] || { printf '!!! test-kernel-headroom-mutant: pad kernel headroom %s != intended %s (layout assumption broke)\n' "$$got" "$$want"; exit 1; }; \
	out=$$( elfs="$$elfs0 $(KHR_MUT_ELF)"; $(call kernel-headroom-measure,khr-mutant) ); rc=$$?; \
	printf '%s\n' "$$out" | sed 's/^/    | /'; \
	if [ "$$want" -lt "$(KERNEL_HEADROOM_FLOOR)" ]; then \
		[ "$$rc" -ne 0 ] || { printf '!!! test-kernel-headroom-mutant FAIL: a kernel at floor-1 (%s B) PASSED -- the gate is decoration\n' "$$want"; exit 1; }; \
		printf '%s\n' "$$out" | grep -q 'LARGEST  khr_mut_pad.elf' || { printf '!!! test-kernel-headroom-mutant FAIL: RED, but not for the pad kernel\n'; exit 1; }; \
		printf '%s\n' "$$out" | grep -q 'khr-mutant FAIL: largest kernel khr_mut_pad.elf' || { printf '!!! test-kernel-headroom-mutant FAIL: RED for the wrong reason\n'; exit 1; }; \
		printf '    floor-1 (%s B, non-flagship kernel): RED for the named reason -- bites\n' "$$want"; \
	else \
		[ "$$rc" -eq 0 ] || { printf '!!! test-kernel-headroom-mutant FAIL: a kernel at exactly the floor (%s B) went RED -- off-by-one\n' "$$want"; exit 1; }; \
		printf '    floor   (%s B): GREEN -- the floor is inclusive\n' "$$want"; \
	fi; \
done
@rm -f $(KHR_MUT_ASM) $(KHR_MUT_OBJ) $(KHR_MUT_ELF)
@printf '>>> test-kernel-headroom-mutant: green (floor-1 pad kernel RED, floor pad kernel GREEN)\n'
```

## test-kernel-native-boundaries

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 140):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/ksize4`
```make
@set -eu; for src in $(TEST_LOADER_SRC) $(TEST_MZLOAD_SRC) $(TEST_TENANT_LOAD_SRC); do \
	$(CC) $(CFLAGS) -Ispec -Ios/milton -Iseed $$src $(KERNEL_LOADER_C) $(KERNEL_PSP_C) -o $(KERNEL_R4_DIR)/validation-model; \
	$(KERNEL_R4_DIR)/validation-model; \
done
$(CC) $(CFLAGS) $(KERNEL_EVENT_OPT) $(EVENT_INC) $(TEST_EVENT_SRC) os/flair/event.c -o $(KERNEL_R4_DIR)/event-model
@$(KERNEL_R4_DIR)/event-model
$(CC) $(CFLAGS) $(KERNEL_KBD_OPT) $(KERNEL_PURE_DECODE_FLAGS) -Ios/milton -Iseed $(TEST_KBD_SRC) $(TEST_KBD_DEPS) -o $(KERNEL_R4_DIR)/kbd-model
@$(KERNEL_R4_DIR)/kbd-model
@sh harness/proptest/kernel_transfer_check.sh $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/transfer.dis
@printf '>>> test-kernel-native-boundaries: green (linked loader transfer; host models; ring barriers review-proven only)\n'
```

## test-kernel-native-boundaries-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 151):
Prerequisites: `test-kernel-native-boundaries build/.makefile.stamp`
```make
@cp -f $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/bad-restore.elf
@set -eu; pos=$$(LC_ALL=C grep -abo "$$(printf '\213\130\010\213\150\014\213\160\020\213\170\024\213\040\377\140\004')" $(KERNEL_R4_DIR)/bad-restore.elf | cut -d: -f1); \
	test "$$(printf '%s\n' "$$pos" | wc -l)" -eq 1; test -n "$$pos"; \
	printf '\020' | dd of=$(KERNEL_R4_DIR)/bad-restore.elf bs=1 seek=$$(( pos + 11 )) conv=notrunc status=none
@if sh harness/proptest/kernel_transfer_check.sh $(KERNEL_R4_DIR)/bad-restore.elf $(KERNEL_R4_DIR)/bad-restore.dis >$(KERNEL_R4_DIR)/bad-restore.log 2>&1; then \
	printf '!!! test-kernel-native-boundaries-mutant FAIL: wrong_edi_restore survived\n'; exit 1; fi
@grep '^!!! kernel-transfer FAIL: restore step 4' $(KERNEL_R4_DIR)/bad-restore.log
@printf '>>> test-kernel-native-boundaries-mutant: green (real ELF wrong_edi_restore RED)\n'
```

## test-kernel-size-ovl

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 237):
Prerequisites: `build/.makefile.stamp | build/ovl-gates`
```make
@$(kernel-headroom-enum); \
prod=$$(for b in $$bins; do case "$$b" in *mut*|*_record*) ;; *) printf '%s ' "$$b";; esac; done); \
for b in $$prod; do case ' $(KERNEL_SIZE_OVL_BINS) ' in *" $$b "*) ;; \
  *) printf '!!! test-kernel-size-ovl: %s not a declared production prerequisite\n' "$$b"; exit 1;; esac; done; \
n=0; bad=0; for b in $$prod; do e=$${b%.bin}.elf; n=$$((n+1)); ( $(call kernel-ovl-grade,$$e) ) || bad=$$((bad+1)); done; \
$(OBJCOPY) -O binary $(KERNEL_FLAIRTENANTS_ELF) $(OVL_GATE_DIR)/resident.bin; \
printf '    flagship %s loaded %s B; cap %s B; %s of %s production kernels over\n' kernel_flairtenants.elf "$$(wc -c < $(OVL_GATE_DIR)/resident.bin)" '$(KERNEL_OVL_RESIDENT_MAX)' "$$bad" "$$n"; \
[ "$$bad" -eq 0 ] || exit 1; \
printf '>>> test-kernel-size-ovl: green (%s production kernels <= %s B)\n' "$$n" '$(KERNEL_OVL_RESIDENT_MAX)'
```

## test-kernel-size-ovl-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 250):
Prerequisites: `build/kernel_gc/probe.o build/.makefile.stamp | build/ovl-gates`
```make
@set -eu; $(LD) -m elf_i386 -T $(KERNEL_LD) -o $(OVL_GATE_DIR)/probe0.elf $(KGC_DIR)/probe.o; \
 base=$$($(OBJCOPY) -O binary $(OVL_GATE_DIR)/probe0.elf /dev/stdout | wc -c); \
 for extra in 0 1; do pad=$$(( $(KERNEL_OVL_RESIDENT_MAX) + extra - base )); \
	printf 'section .data align=1\nglobal ovl_size_pad\novl_size_pad: times %s db 0xA5\n' "$$pad" > $(OVL_GATE_DIR)/szpad.asm; \
	$(NASM) -f elf32 $(OVL_GATE_DIR)/szpad.asm -o $(OVL_GATE_DIR)/szpad.o; \
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $(OVL_GATE_DIR)/szpad$$extra.elf $(KGC_DIR)/probe.o $(OVL_GATE_DIR)/szpad.o; \
	got=$$($(OBJCOPY) -O binary $(OVL_GATE_DIR)/szpad$$extra.elf /dev/stdout | wc -c); \
	[ "$$got" -eq $$(( $(KERNEL_OVL_RESIDENT_MAX) + extra )) ] || { printf '!!! test-kernel-size-ovl-mutant: pad layout %s != %s\n' "$$got" "$$(( $(KERNEL_OVL_RESIDENT_MAX) + extra ))"; exit 1; }; \
 done
@( $(call kernel-ovl-grade,$(OVL_GATE_DIR)/szpad0.elf) ) || { printf '!!! test-kernel-size-ovl-mutant FAIL: exactly the cap went RED (off by one)\n'; exit 1; }
@if ( $(call kernel-ovl-grade,$(OVL_GATE_DIR)/szpad1.elf) ) > $(OVL_GATE_DIR)/szpad1.log 2>&1; then printf '!!! test-kernel-size-ovl-mutant FAIL: cap+1 survived\n'; exit 1; fi
@grep '!!! kernel-size-ovl FAIL: szpad1.elf loaded 169985 B' $(OVL_GATE_DIR)/szpad1.log
@printf '>>> test-kernel-size-ovl-mutant: green (cap passes, cap+1 RED)\n'
```

## test-kernel-size-r3-emu

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 108):
Prerequisites: `test-kernel-gc-emu test-flair-itext build/.makefile.stamp`
```make
@printf '>>> test-kernel-size-r3-emu: green (desktop pixels, text trace, DOS/FCB/FAT differential)\n'
```

## test-kernel-size-r3-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 110):
Prerequisites: `test-kernel-gc-emu-mutant build/.makefile.stamp`
```make
@printf '>>> test-kernel-size-r3-emu-mutant: green (desktop/FCB behaviour mutants RED)\n'
```

## test-kernel-size-r4

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 25):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/ksize4`
```make
@$(call kernel-r4-grade,$<)
@printf '>>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window)\n'
```

## test-kernel-size-r4-emu

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 51):
Prerequisites: `test-fcb-api test-flair-appswitch test-flair-itext build/.makefile.stamp`
```make
@printf '>>> test-kernel-size-r4-emu: green (DOS/FAT differential; injected desktop/text traces and pixels)\n'
```

## test-kernel-size-r4-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 256):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-kernel-size-r4-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-kernel-size-r4-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/kernel-size-r4-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/kernel-size-r4-emu_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "kernel-size-r4-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/kernel-size-r4-emu_bochs.report.txt" || true
@cat "$(BUILD)/kernel-size-r4-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-kernel-size-r4-emu-bochs "$(BUILD)/kernel-size-r4-emu_bochs.report.txt" "$(BUILD)/kernel-size-r4-emu_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-kernel-size-r4-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 53):
Prerequisites: `test-fcb-api-mutant test-flair-appswitch-mutant build/.makefile.stamp`
```make
@printf '>>> test-kernel-size-r4-emu-mutant: green (FCB and desktop behaviour mutants RED)\n'
```

## test-kernel-size-r4-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 31):
Prerequisites: `test-kernel-size-r4 build/.makefile.stamp`
```make
@set -eu; $(call kernel-r4-grade,$(KERNEL_FLAIRTENANTS_ELF)); \
pad=$$(( $(KERNEL_R4_LOADED_MAX) + 1 - loaded )); \
printf 'bits 32\nsection .data align=1\nglobal r4_size_pad\nr4_size_pad: times %s db 0xA5\n' "$$pad" >$(KERNEL_R4_DIR)/pad.asm; \
$(NASM) -f elf32 $(KERNEL_R4_DIR)/pad.asm -o $(KERNEL_R4_DIR)/pad.o
$(KERNEL_LINK) -u r4_size_pad -o $(KERNEL_R4_DIR)/pad.elf $(KERNEL_FLAIRTENANTS_OBJS) $(KERNEL_R4_DIR)/pad.o @$(TRANSIENT_IMPORTS_LD)
@$(OBJCOPY) -O binary $(KERNEL_R4_DIR)/pad.elf $(KERNEL_R4_DIR)/pad.bin
@test "$$(wc -c < $(KERNEL_R4_DIR)/pad.bin)" -eq $$(( $(KERNEL_R4_LOADED_MAX) + 1 ))
@if ( $(call kernel-r4-grade,$(KERNEL_R4_DIR)/pad.elf) ) >$(KERNEL_R4_DIR)/pad.log 2>&1; then \
	printf '!!! test-kernel-size-r4-mutant FAIL: size_pad_one survived\n'; exit 1; fi
@grep '!!! kernel-size-r4 FAIL:' $(KERNEL_R4_DIR)/pad.log
@printf '>>> test-kernel-size-r4-mutant: green (size_pad_one RED)\n'
```

## test-kernel-transfer

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 77):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp | build/ksize4`
```make
@sh harness/proptest/kernel_transfer_check.sh $< $(KERNEL_R4_DIR)/transfer.dis
@printf '>>> test-kernel-transfer: green (unique linked transfer; ESP/EBP-based stack operand)\n'
```

## test-kernel-transfer-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 80):
Prerequisites: `test-kernel-transfer build/.makefile.stamp`
```make
@sh harness/proptest/kernel_transfer_mutants.sh $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)
```

## test-list

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14645):
Prerequisites: `build/test_list build/.makefile.stamp`
```make
@printf ">>> test-list: ADR-0012 D-2b / initech-77dj List Manager (ListRec) -- cell store + LClick hit-test + lOnlyOne selection (round-trip vs independent hand-authored golden)\n"
@$(TEST_LIST)
@printf ">>> test-list: green\n"
```

## test-list-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14650):
Prerequisites: `build/test_list_mutant_indexswap build/test_list_mutant_hitoffbyone build/test_list_mutant_nodeselect build/.makefile.stamp`
```make
@printf ">>> test-list-mutant: confirming CELL_INDEX_SWAP + HIT_OFFBYONE + NO_DESELECT go RED (Rule 6)\n"
@if $(TEST_LIST_MUT_SWAP) >/dev/null 2>&1; then printf '!!! test-list-mutant FAIL: CELL_INDEX_SWAP PASSED -- the (col,row) store oracle is decoration\n'; exit 1; else printf '>>> test-list-mutant: green (CELL_INDEX_SWAP correctly RED)\n'; fi
@if $(TEST_LIST_MUT_HIT) >/dev/null 2>&1; then printf '!!! test-list-mutant FAIL: HIT_OFFBYONE PASSED -- the hit-test oracle is decoration\n'; exit 1; else printf '>>> test-list-mutant: green (HIT_OFFBYONE correctly RED)\n'; fi
@if $(TEST_LIST_MUT_DESEL) >/dev/null 2>&1; then printf '!!! test-list-mutant FAIL: NO_DESELECT PASSED -- the lOnlyOne oracle is decoration\n'; exit 1; else printf '>>> test-list-mutant: green (NO_DESELECT correctly RED)\n'; fi
```

## test-mech-policy

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16093):
Prerequisites: `build/test_mech_policy build/.makefile.stamp`
```make
@printf ">>> test-mech-policy: C-8 SOURCE scanner -- ZERO color literal/switch in the mechanism file set (allowlist clut.h + flair_look)\n"
@$(TEST_MECH_POLICY)
@printf ">>> test-mech-policy: green\n"
```

## test-mech-policy-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16097):
Prerequisites: `build/test_mech_policy_mutant build/.makefile.stamp`
```make
@if $(TEST_MECH_POLICY_MUT) >/dev/null 2>&1; then printf '!!! test-mech-policy-mutant FAIL: the planted 0xRRGGBB literal was NOT detected -- the scanner is decoration\n'; exit 1; else printf '>>> test-mech-policy-mutant: green (planted literal correctly RED -- the scanner bites)\n'; fi
```

## test-menu

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14982):
Prerequisites: `build/test_menu build/.makefile.stamp`
```make
@printf ">>> test-menu: behavior + sampled Platinum profile/corners/title/panel/separator/cmd/disabled fidelity + retained Apple strike\n"
@$(TEST_MENU) $(BUILD)/menu_window.ppm
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_MENU_OPT) -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/menu.c -o $(BUILD)/menu_freestanding.o \
	|| { printf '!!! test-menu FAIL: menu.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-menu: green\n"
```

## test-menu-handlers

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14124):
Prerequisites: `build/test_menu_handlers test-finder-demo build/.makefile.stamp`
```make
@printf ">>> test-menu-handlers: every enabled Finder / band-1 menu row has a real handler (tdnl.36)\n"
@$(TEST_MENU_HANDLERS)
@printf ">>> test-menu-handlers: green\n"
```

## test-menu-handlers-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14129):
Prerequisites: `build/test_menu_handlers_mutant_FINDER_MENU_MUT_STUB_ENABLED build/test_menu_handlers_mutant_SHELL_MENUS_MUT_ABOUT_LIVE build/test_menu_handlers_mutant_FINDER_CMD_MUT_EXEC_SWALLOWS build/test_menu_handlers_mutant_FINDER_WIN_MUT_EXIT_DOS_UNWIRED build/.makefile.stamp`
```make
@for pair in 'FINDER_MENU_MUT_STUB_ENABLED:H1 [all-live] enabled row menu=512 item=6' 'SHELL_MENUS_MUT_ABOUT_LIVE:H2 band-1 enabled row "About"' 'FINDER_CMD_MUT_EXEC_SWALLOWS:H3 an unimplemented command' 'FINDER_WIN_MUT_EXIT_DOS_UNWIRED:H5 Special > Exit to InitechDOS is implemented'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_menu_handlers_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-menu-handlers-mutant FAIL: %s PASSED -- the guard is decoration\n' "$$m"; exit 1; fi; \
	grep -F "$$why" $$bin.log | grep -q FAIL || { printf '!!! test-menu-handlers-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-menu-handlers-mutant: %s correctly RED (%s)\n' "$$m" "$$why"; \
done
@printf '>>> test-menu-handlers-mutant: green (all four mutants RED for the named reason)\n'
```

## test-menu-look

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33631):
Prerequisites: `build/test_menu_look build/.makefile.stamp`
```make
@$< "$(MENU_LOOK_PNG)" "$(MENU_LOOK_NFNT)"
@printf '>>> test-menu-look: green (independent title band, item20/4, shortcut14, NFNT check18, whole divider rows; both depths; clipping)\n'
```

## test-menu-look-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 33634):
Prerequisites: `test-menu-look build/test_menu_look_mut_MENU_LOOK_MUT_FLAT_HILITE build/test_menu_look_mut_MENU_LOOK_MUT_WRONG_RPAD build/test_menu_look_mut_MENU_LOOK_MUT_V_MARK build/test_menu_look_mut_MENU_LOOK_MUT_OLD_DIVIDER_INSET build/.makefile.stamp`
```make
@for m in $(MENU_LOOK_MUTANTS); do \
 if $(BUILD)/test_menu_look_mut_$$m "$(MENU_LOOK_PNG)" "$(MENU_LOOK_NFNT)" > $(BUILD)/menu_look_$$m.log 2>&1; then printf 'FAIL mutant %s survived\n' "$$m"; exit 1; fi; \
 case "$$m" in *FLAT*) leg=ML1;; *RPAD*) leg=ML4;; *MARK*) leg=ML5;; *DIVIDER*) leg=ML10;; esac; \
 grep -q "FAIL.*$$leg" $(BUILD)/menu_look_$$m.log || { cat $(BUILD)/menu_look_$$m.log; exit 1; }; \
 printf '>>> test-menu-look-mutant: %s correctly RED (%s)\n' "$$m" "$$leg"; done
```

## test-menu-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14989):
Prerequisites: `build/test_menu_mutant_fixedwidth build/test_menu_mutant_selectdisabled build/test_menu_mutant_norehit build/test_menu_mutant_applesquare build/test_menu_mutant_bar_flat build/test_menu_mutant_no_panel_bevel build/test_menu_mutant_sep_plain build/test_menu_mutant_disabled_normal_ink build/test_menu_mutant_title_no_hilite build/test_menu_mutant_track_cap build/.makefile.stamp`
```make
@printf ">>> test-menu-mutant: confirming all ten behavior/fidelity mutants go RED (Rule 6)\n"
@if $(TEST_MENU_MUT_FW) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: FIXED_WIDTH PASSED -- the proportional-layout oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (FIXED_WIDTH correctly RED)\n'; fi
@if $(TEST_MENU_MUT_SD) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: SELECT_DISABLED PASSED -- the selectability oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (SELECT_DISABLED correctly RED)\n'; fi
@if $(TEST_MENU_MUT_NR) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: NO_REHIT PASSED -- the cross-menu-drag oracle is decoration (initech-rl4v)\n'; exit 1; else printf '>>> test-menu-mutant: green (NO_REHIT correctly RED, initech-rl4v)\n'; fi
@if $(TEST_MENU_MUT_AS) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: APPLE_SQUARE PASSED -- the Apple-glyph oracle is decoration (initech-yx4v)\n'; exit 1; else printf '>>> test-menu-mutant: green (APPLE_SQUARE correctly RED, initech-yx4v)\n'; fi
@if $(TEST_MENU_MUT_BAR) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_BAR_FLAT PASSED -- the sampled bar profile/corner oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_BAR_FLAT correctly RED for profile/corners)\n'; fi
@if $(TEST_MENU_MUT_BEVEL) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_NO_PANEL_BEVEL PASSED -- the panel white/B3 bevel oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_NO_PANEL_BEVEL correctly RED for panel bevel)\n'; fi
@if $(TEST_MENU_MUT_SEP) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_SEP_PLAIN PASSED -- the etched-separator oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_SEP_PLAIN correctly RED for etched separator)\n'; fi
@if $(TEST_MENU_MUT_DISABLED) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_DISABLED_NORMAL_INK PASSED -- the disabled A5 ink oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_DISABLED_NORMAL_INK correctly RED for disabled ink)\n'; fi
@if $(TEST_MENU_MUT_TITLE) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_TITLE_NO_HILITE PASSED -- the pulled-title accent oracle is decoration\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_TITLE_NO_HILITE correctly RED for pulled-title state)\n'; fi
@if $(TEST_MENU_MUT_TC) >/dev/null 2>&1; then printf '!!! test-menu-mutant FAIL: MENU_MUT_TRACK_CAP PASSED -- the long-gesture oracle is decoration (tdnl.71)\n'; exit 1; else printf '>>> test-menu-mutant: green (MENU_MUT_TRACK_CAP correctly RED, tdnl.71)\n'; fi
```

## test-menu-record

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15960):
Prerequisites: `build/test_menu_record build/.makefile.stamp`
```make
@printf ">>> test-menu-record: MenuInfo + enableFlags + MenuSelect result-word (spec/menu_record.h)\n"
@$(TEST_MENU_RECORD)
@printf ">>> test-menu-record: green\n"
```

## test-menu-record-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15964):
Prerequisites: `build/test_menu_record_mutant build/.makefile.stamp`
```make
@if $(TEST_MENU_RECORD_MUT) >/dev/null 2>&1; then printf '!!! test-menu-record-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; else printf '>>> test-menu-record-mutant: green (mutant correctly RED)\n'; fi
```

## test-menubar-clock

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34321):
Prerequisites: `build/test_menubar_clock build/.makefile.stamp`
```make
@$< "$(MENUBAR_CLOCK_PNG)" "$(MENUBAR_CLOCK_NFNT)"
@printf '>>> test-menubar-clock: green (1440 readings, minute transitions, independent Chicago NFNT + Mac OS 8 bar)\n'
```

## test-menubar-clock-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34324):
Prerequisites: `build/test_menubar_clock_mut_CLOCK_WRONG_HOUR build/test_menubar_clock_mut_CLOCK_STUCK build/.makefile.stamp`
```make
@for m in CLOCK_WRONG_HOUR CLOCK_STUCK; do \
 if $(BUILD)/test_menubar_clock_mut_$$m "$(MENUBAR_CLOCK_PNG)" "$(MENUBAR_CLOCK_NFNT)" > $(BUILD)/menubar_clock_$$m.log 2>&1; then printf 'FAIL clock mutant %s survived\n' "$$m"; exit 1; fi; \
 case "$$m" in CLOCK_WRONG_HOUR) leg=MC1;; CLOCK_STUCK) leg=MC4;; esac; \
 grep -q "FAIL.*$$leg" $(BUILD)/menubar_clock_$$m.log || { cat $(BUILD)/menubar_clock_$$m.log; exit 1; }; \
 printf '>>> test-menubar-clock-mutant: %s correctly RED: %s\n' "$$m" "$$(grep -m1 "FAIL.*$$leg" $(BUILD)/menubar_clock_$$m.log)"; done
@printf '>>> test-menubar-clock-mutant: green (both named regressions rejected)\n'
```

## test-mouse-producer

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14866):
Prerequisites: `build/test_mouse_producer build/.makefile.stamp`
```make
@printf ">>> test-mouse-producer: INDEPENDENT physical-direction oracle (raw PS/2 wire -> real producer pack -> real ring -> real cook_raw; physical-up must DECREASE screen v; initech-8f5p)\n"
@$(TEST_MOUSE_PRODUCER)
@printf ">>> test-mouse-producer: green\n"
```

## test-mouse-producer-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14871):
Prerequisites: `build/test_mouse_producer_mutant_flipsign build/test_event build/.makefile.stamp`
```make
@printf ">>> test-mouse-producer-mutant: confirming EVENT_MUTATE_FLIP_SIGN goes RED (Rule 6) AND confirming the independence contrast (old test-event oracle stays GREEN under the SAME flag -- it never touches the producer)\n"
@if $(TEST_MOUSE_PRODUCER_MUT_FLIPSIGN) >/dev/null 2>&1; then printf '!!! test-mouse-producer-mutant FAIL: FLIP_SIGN PASSED -- the physical-direction oracle is decoration\n'; exit 1; else printf '>>> test-mouse-producer-mutant: green (FLIP_SIGN correctly RED)\n'; fi
@if $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DEVENT_MUTATE_FLIP_SIGN -Ios/flair -Ispec -Iseed -o $(BUILD)/test_event_flipsign_noop harness/proptest/test_event.c os/flair/event.c && $(BUILD)/test_event_flipsign_noop >/dev/null 2>&1; then \
	printf '>>> test-mouse-producer-mutant: green (independence contrast confirmed: test-event stays GREEN under -DEVENT_MUTATE_FLIP_SIGN -- it is blind to the producer, exactly the HER-02 gap this ticket closes)\n'; \
else \
	printf '!!! test-mouse-producer-mutant FAIL: test-event went RED (or failed to build) under -DEVENT_MUTATE_FLIP_SIGN -- the independence contrast is broken (test-event should be UNAFFECTED by a producer-only mutant)\n'; exit 1; \
fi
```

## test-ovl-absence

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 111):
Prerequisites: `build/kernel_flairtenants.elf build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o build/transient_imports.rsp build/.makefile.stamp | build/ovl-gates`
```make
@$(call ovl-absence-check,$(KERNEL_FLAIRTENANTS_ELF))
@printf '>>> test-ovl-absence: green (COLD families, including Finder [%s], absent from the resident ELF; exports are stubs only)\n' "$$(tr '\n' ' ' < $(OVL_GATE_DIR)/family.txt)"
```

## test-ovl-absence-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 118):
Prerequisites: `test-ovl-absence build/ovl-gates/egg_jtc_kept.o test-ovl-absence-finder-mutant test-ovl-absence-power-mutant build/.makefile.stamp`
```make
$(KERNEL_LINK) -u egg_jtc_open_kept -u egg_jtc_event_kept -o $(OVL_GATE_DIR)/kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/egg_jtc_kept.o @$(TRANSIENT_IMPORTS_LD)
@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/kept.elf) ) > $(OVL_GATE_DIR)/kept.log 2>&1; then printf '!!! test-ovl-absence-mutant FAIL: a resident copy survived\n'; exit 1; fi
@grep -E '!!! ovl-absence FAIL: (moved symbol|export .* resident 2 times)' $(OVL_GATE_DIR)/kept.log
@printf '>>> test-ovl-absence-mutant: green (resident copy of the moved family RED)\n'
```

## test-ovl-absence-power-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/power_overlay.mk', line 33):
Prerequisites: `test-ovl-absence test-ovl-absence-finder-mutant build/ovl-gates/power_cold_kept.o build/.makefile.stamp`
```make
$(KERNEL_LINK) -u finder_power_kept -u finder_exit_dos_kept -o $(OVL_GATE_DIR)/power_kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/power_cold_kept.o @$(TRANSIENT_IMPORTS_LD)
@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/power_kept.elf) ) > $(OVL_GATE_DIR)/power_kept.log 2>&1; then printf '!!! test-ovl-absence-power-mutant FAIL: a resident power/exit body survived\n'; exit 1; fi
@grep -E '!!! ovl-absence FAIL: (export finder_(power|exit_dos) resident [12] times|moved symbol (finder_shutdown_screen|finder_power_ask_apps|flair_exit_to_dos|exitdos_fail) still resident)' $(OVL_GATE_DIR)/power_kept.log
@printf '>>> test-ovl-absence-power-mutant: green (resident power/exit body RED)\n'
```

## test-ovl-behaviour

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 479):
Prerequisites: `test-flair-egg-jtc test-ovl-absence test-flair-file-ops test-flair-trash-edges test-flair-finder-alerts test-flair-file-copy build/.makefile.stamp`
```make
@grep -qx 'OVL-OK' $(BUILD)/egg_jtc_mat.serial
@printf '>>> test-ovl-behaviour: green (the JTC mat ran from C:\\FLAIR.OVL against its authored goldens; resident copy absent)\n'
```

## test-ovl-behaviour-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 482):
Prerequisites: `test-ovl-behaviour test-flair-file-ops-mutant test-flair-trash-edges-mutant test-flair-file-copy-mutant build/ovl-gates/nolabel.img build/ovl-gates/swap.img build/ppm_flair_egg_jtc_check build/ovl-gates/data.img build/.makefile.stamp`
```make
$(call jtc-boot,ovl_nolabel,$(OVL_GATE_DIR)/nolabel.img,FLAIR_EGG_JTC_OPEN_SPEC,EGG-JTC-OPEN click=3)
@if $(JTC_CHECK) $(BUILD)/ovl_nolabel.ppm fixtures/easter/jtc_mat.png > $(OVL_GATE_DIR)/nolabel.red 2>&1; then printf '!!! mutant overlay (one label) survived the authored still\n'; exit 1; fi
@cat $(OVL_GATE_DIR)/nolabel.red | head -n 2
@cp -f $(OVL_DATA) $(OVL_GATE_DIR)/swap_data.img
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(OVL_GATE_DIR)/swap.img --disk2 $(OVL_GATE_DIR)/swap_data.img --name ovl_swap --out $(OVL_GATE_DIR) \
 --mouse "$(FLAIR_EGG_JTC_OPEN_SPEC)" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK --timeout-ms 40000 \
 --screendump --screendump-after 'EGG-JTC-OPEN click=3' 2> $(OVL_GATE_DIR)/ovl_swap.report || true
@if tr -d '\r' < $(OVL_GATE_DIR)/ovl_swap.serial | grep -qx 'PANIC JTC launch'; then printf 'RED wrong-export stubs: PANIC JTC launch (open ran another entry)\n'; \
 elif ! tr -d '\r' < $(OVL_GATE_DIR)/ovl_swap.serial | grep -qx 'EGG-JTC-OPEN click=3'; then printf 'RED wrong-export stubs: the mat never opened\n'; \
 elif [ -f $(OVL_GATE_DIR)/ovl_swap.ppm ] && ! $(JTC_CHECK) $(OVL_GATE_DIR)/ovl_swap.ppm fixtures/easter/jtc_mat.png > $(OVL_GATE_DIR)/swap.red 2>&1; then head -n 1 $(OVL_GATE_DIR)/swap.red; \
 else printf '!!! wrong-export stubs survived\n'; exit 1; fi
@printf '>>> test-ovl-behaviour-mutant: green (mutant overlay on a real kernel and wrong-export stubs both RED)\n'
```

## test-ovl-context

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 432):
Prerequisites: `test-ovl-context-static test-panic-screen build/qemu_harness build/ppm_panic_check build/ovl-gates/data.img build/flair_tenants_mut_ovl_ctx_irq.img build/flair_tenants_mut_ovl_ctx_panic.img build/flair_tenants_mut_ovl_ctx_busy.img build/ovl-gates/fault.img build/.makefile.stamp`
```make
@sh harness/emu/ovl_panic.sh $(BUILD) $(BUILD)/flair_tenants_mut_ovl_ctx_irq.img $(OVL_DATA) ctx_irq CONTEXT 00000001 HALTED 1
@$(call ovl-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_panic.img,$(OVL_DATA),ctx_panic,CONTEXT,00000100)
@$(call ovl-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_busy.img,$(OVL_DATA),ctx_busy,CONTEXT,00010000)
$(call ovl-fault-leg,$(OVL_GATE_DIR)/fault.img,fault)
@grep -qx 'PANIC FLAIR.OVL section=COLD' $(OVL_GATE_DIR)/fault.norm && grep -q '^PANIC vec=06 ' $(OVL_GATE_DIR)/fault.norm && grep -qx HALTED $(OVL_GATE_DIR)/fault.norm \
 && ! grep -q '^TENANT-CRASH' $(OVL_GATE_DIR)/fault.norm || { cat $(OVL_GATE_DIR)/fault.norm | tail -5; printf '!!! test-ovl-context FAIL: an overlay #UD was not a kernel panic naming section COLD\n'; exit 1; }
@printf '>>> test-ovl-context: green (loads from IRQ0, the panic path and under a busy ATA channel refused CONTEXT; an overlay #UD panics naming section COLD, never a tenant crash)\n'
```

## test-ovl-context-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 440):
Prerequisites: `test-ovl-context build/flair_tenants_mut_ovl_ctx_irq_mut.img build/flair_tenants_mut_ovl_ctx_panic_mut.img build/flair_tenants_mut_ovl_ctx_busy_mut.img build/ovl-gates/fault_mut.img build/.makefile.stamp`
```make
$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_irq_mut.img,$(OVL_DATA),ctx_irq_mut,CONTEXT,00000001,HALTED,no PANIC FLAIR.OVL line,1)
@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_irq_mut.norm
$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_panic_mut.img,$(OVL_DATA),ctx_panic_mut,CONTEXT,00000100,HALTED,no PANIC FLAIR.OVL line)
@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_panic_mut.norm
$(call ovl-mutant-leg,$(BUILD)/flair_tenants_mut_ovl_ctx_busy_mut.img,$(OVL_DATA),ctx_busy_mut,CONTEXT,00010000,HALTED,no PANIC FLAIR.OVL line)
@grep -qx OVL-TEST-SURVIVED $(OVL_GATE_DIR)/ctx_busy_mut.norm
$(call ovl-fault-leg,$(OVL_GATE_DIR)/fault_mut.img,fault_mut)
@if grep -qx 'PANIC FLAIR.OVL section=COLD' $(OVL_GATE_DIR)/fault_mut.norm; then printf '!!! OVL_MUT_NO_FAULT_NAME survived\n'; exit 1; fi
@grep -q '^PANIC vec=06 ' $(OVL_GATE_DIR)/fault_mut.norm
@printf '>>> test-ovl-context-mutant: green (each guard disabled: the load proceeds -- OVL-TEST-SURVIVED; unnamed section fault RED)\n'
```

## test-ovl-context-static

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 178):
Prerequisites: `build/kernel_flairtenants.elf build/isr.o build/irq.o build/pit.o build/kbd.o build/mouse.o build/event.o build/int21.o build/fat12.o build/fileio_fat.o build/block_device.o build/ata.o build/transient_imports.rsp build/.makefile.stamp | build/ovl-gates`
```make
@$(call ovl-nostub-check,$(OVL_NOSTUB_OBJS))
@if grep -Eq '^(pit_set_tick_hook|kbd_set_scancode_hook|mouse_set_event_hook|panic_set_fault_hook|idt_.*)$$' $(BUILD)/ovl_imports.txt; then printf '!!! ovl-context-static FAIL: overlay imports a hook setter\n'; exit 1; fi
$(KERNEL_LINK) -Wl,--emit-relocs -o $(OVL_RELOC_ELF) $(KERNEL_FLAIRTENANTS_OBJS) @$(TRANSIENT_IMPORTS_LD)
@sh harness/proptest/ovl_ptr_check.sh resident $(OVL_RELOC_ELF) $(OVL_GATE_DIR)/ptr
@sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_SECTION_OBJS)
@printf '>>> test-ovl-context-static: green (no IRQ/driver/INT21 object references a stub; no hook-setter import; no resident data pointer into the window)\n'
```

## test-ovl-context-static-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 185):
Prerequisites: `test-ovl-context-static build/.makefile.stamp`
```make
@printf 'bits 32\nextern egg_jtc_event\nsection .text\nglobal pit_ovl_ref\npit_ovl_ref: call egg_jtc_event\n' > $(OVL_GATE_DIR)/pitref.asm
@$(NASM) -f elf32 $(OVL_GATE_DIR)/pitref.asm -o $(OVL_GATE_DIR)/pit.o
@if ( $(call ovl-nostub-check,$(OVL_GATE_DIR)/pit.o) ) > $(OVL_GATE_DIR)/nostub.log 2>&1; then printf '!!! FAIL: stub reference from pit.o survived\n'; exit 1; fi
@grep 'references an overlay stub' $(OVL_GATE_DIR)/nostub.log
@printf 'bits 32\nextern pit_set_tick_hook\nsection .text\nglobal egg_jtc_hookme\negg_jtc_hookme: call pit_set_tick_hook\n' > $(OVL_GATE_DIR)/hook.asm
@$(NASM) -f elf32 $(OVL_GATE_DIR)/hook.asm -o $(OVL_GATE_DIR)/hook.o
@mkdir -p $(OVL_GATE_DIR)/hookgen
@if sh tools/mk_ovl_tables.sh $(OVL_GATE_DIR)/hookgen $(NASM) $(TRANSIENT_IMPORTS_LD).kdef $(TRANSIENT_IMPORTS_LD).kund $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/hook.o > $(OVL_GATE_DIR)/hook.log 2>&1; then printf '!!! FAIL: hook-setter import survived\n'; exit 1; fi
@grep 'DEC-14-6.5 overlay imports an IRQ/IDT hook setter: pit_set_tick_hook' $(OVL_GATE_DIR)/hook.log
@printf 'extern ovl_window_sym\nsection .rodata align=4\nglobal ovl_bad_ptr\novl_bad_ptr: dd ovl_window_sym\n' > $(OVL_GATE_DIR)/badptr.asm
@$(NASM) -f elf32 $(OVL_GATE_DIR)/badptr.asm -o $(OVL_GATE_DIR)/badptr.o
$(KERNEL_LINK) -Wl,--emit-relocs -Wl,--defsym=ovl_window_sym=0x005C0100 -u ovl_bad_ptr -o $(OVL_GATE_DIR)/badptr.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/badptr.o @$(TRANSIENT_IMPORTS_LD)
@if sh harness/proptest/ovl_ptr_check.sh resident $(OVL_GATE_DIR)/badptr.elf $(OVL_GATE_DIR)/ptrmut > $(OVL_GATE_DIR)/badptr.log 2>&1; then printf '!!! FAIL: resident pointer into the window survived\n'; exit 1; fi
@grep 'resident relocated pointer(s) into the overlay window' $(OVL_GATE_DIR)/badptr.log
@printf 'section .text\nglobal egg_jtc_addr_taken\negg_jtc_addr_taken: mov eax, here\nhere: ret\n' > $(OVL_GATE_DIR)/addr.asm
@$(NASM) -f elf32 $(OVL_GATE_DIR)/addr.asm -o $(OVL_GATE_DIR)/addr.o
@if sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/addr.o > $(OVL_GATE_DIR)/addr.log 2>&1; then printf '!!! FAIL: overlay address-taken code survived\n'; exit 1; fi
@grep 'takes the address of overlay code' $(OVL_GATE_DIR)/addr.log
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(JTC_INC) -DEGG_MUT_OVL_LEAK -c os/flair/egg_jtc.c -o $(OVL_GATE_DIR)/egg_jtc_mut_leak.o
@if sh harness/proptest/ovl_ptr_check.sh overlay $(OVL_GATE_DIR)/egg_jtc_mut_leak.o $(filter-out $(BUILD)/egg_jtc.o,$(OVL_SECTION_OBJS)) > $(OVL_GATE_DIR)/leak.log 2>&1; then printf '!!! FAIL: a global overlay function address (reproducer B) survived\n'; exit 1; fi
@grep 'takes the address of overlay code' $(OVL_GATE_DIR)/leak.log
@mkdir -p $(OVL_GATE_DIR)/heapmap
@sed -e 's/^#define OVERLAY_BASE        0x[0-9A-F]*u$$/#define OVERLAY_BASE        0x004C0000u/' -e 's/^#define OVERLAY_CEIL        0x[0-9A-F]*u$$/#define OVERLAY_CEIL        0x004E0000u/' spec/memory_map.h > $(OVL_GATE_DIR)/heapmap/memory_map.h
@test "$$(grep -cE '^#define OVERLAY_(BASE|CEIL) +0x004[CE]0000u$$' $(OVL_GATE_DIR)/heapmap/memory_map.h)" -eq 2
@if $(CC) $(CFLAGS) -I$(OVL_GATE_DIR)/heapmap -I$(KERNEL_DIR) -o $(OVL_GATE_DIR)/heapmap/t $(TEST_MEMMAP_SRC) > $(OVL_GATE_DIR)/heapmap/cc.log 2>&1; then printf '!!! FAIL: overlay window inside the FLAIR heap compiled\n'; exit 1; fi
@grep -q 'memory map windows overlap: OVERLAY vs FLAIR_HEAP' $(OVL_GATE_DIR)/heapmap/cc.log
@printf '>>> test-ovl-context-static-mutant: green (stub ref from pit.o, hook-setter import, resident window pointer, overlay address-taken code (local and global), window-in-heap: all RED)\n'
```

## test-ovl-lazy

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 539):
Prerequisites: `test-flair-apple-menu build/flair_tenants_mut_ovl_lazy.img build/flair_data.img build/ppm_flair_egg_jtc_check build/.makefile.stamp`
```make
$(call ovl-lazy-mat,$(OVL_LAZY_IMG),ovl_lazy)
$(call ovl-lazy-about,$(OVL_LAZY_IMG),ovl_lazy)
$(call ovl-lazy-finder,$(OVL_LAZY_IMG),ovl_lazy)
@printf '>>> test-ovl-lazy: green (no preload: JTC, About and Finder move load once after READY; Finder bytes match through mtools)\n'
```

## test-ovl-lazy-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 544):
Prerequisites: `test-ovl-lazy build/ovl-gates/nosave.img build/ovl-gates/noload.img build/.makefile.stamp`
```make
@set -e; for m in nosave noload; do for leg in mat about finder; do \
 if ( $(MAKE) --no-print-directory ovl-lazy-leg-$$leg OVL_LAZY_LEG_IMG=$(OVL_GATE_DIR)/$$m.img OVL_LAZY_LEG_TAG=ovl_lazy_$$m ) > $(OVL_GATE_DIR)/lazy_$${m}_$$leg.log 2>&1; then \
	printf '!!! test-ovl-lazy-mutant FAIL: stub mutant %s survived the %s trace\n' $$m $$leg; exit 1; fi; \
 printf '    %s %s: %s\n' $$m $$leg "$$(grep -E '^(RED|!!!)' $(OVL_GATE_DIR)/lazy_$${m}_$$leg.log | head -n 1)"; done; done
@printf '>>> test-ovl-lazy-mutant: green (nosave/noload RED on JTC, About and Finder traces)\n'
```

## test-ovl-panic

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 397):
Prerequisites: `test-panic-screen build/qemu_harness build/ppm_panic_check build/ovl-gates/missing.img build/ovl-gates/skew.img build/ovl-gates/crcflip.img build/ovl-gates/magicflip.img build/ovl-gates/data.img build/ovl-gates/data_with_ovl.img build/.makefile.stamp`
```make
@$(call ovl-leg,$(OVL_GATE_DIR)/missing.img,$(OVL_DATA),missing,MISSING,-)
@$(call ovl-leg,$(OVL_GATE_DIR)/skew.img,$(OVL_DATA),skew,FOREIGN,$(OVL_N_IMPORTS_LAST))
@$(call ovl-leg,$(OVL_GATE_DIR)/crcflip.img,$(OVL_DATA),crcflip,CORRUPT,-)
@$(call ovl-leg,$(OVL_GATE_DIR)/magicflip.img,$(OVL_DATA),magicflip,FOREIGN,00000007)
@$(call ovl-leg,$(OVL_GATE_DIR)/missing.img,$(OVL_DATA_OVL),wrongvol,MISSING,-)
@printf '>>> test-ovl-panic: green (missing, foreign import skew, CRC flip, magic flip, wrong volume: each refused by name + PC LOAD LETTER, never READY)\n'
```

## test-ovl-panic-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/proptest/overlay.mk', line 404):
Prerequisites: `test-ovl-panic build/ovl-gates/missing_mut.img build/ovl-gates/skew_mut.img build/ovl-gates/crcflip_mut.img build/ovl-gates/magicflip_mut.img build/ovl-gates/wrongvol_mut.img build/ovl-gates/data_with_mut_ovl.img build/.makefile.stamp`
```make
$(call ovl-mutant-leg,$(OVL_GATE_DIR)/missing_mut.img,$(OVL_DATA),missing_mut,MISSING,-,HALTED,RED ovl-panic missing_mut: wrong refusal)
$(call ovl-mutant-leg,$(OVL_GATE_DIR)/skew_mut.img,$(OVL_DATA),skew_mut,FOREIGN,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
$(call ovl-mutant-leg,$(OVL_GATE_DIR)/crcflip_mut.img,$(OVL_DATA),crcflip_mut,CORRUPT,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
$(call ovl-mutant-leg,$(OVL_GATE_DIR)/magicflip_mut.img,$(OVL_DATA),magicflip_mut,FOREIGN,00000007,HALTED,RED ovl-panic magicflip_mut: wrong refusal)
$(call ovl-mutant-leg,$(OVL_GATE_DIR)/wrongvol_mut.img,$(OVL_DATA_OVL_MUT),wrongvol_mut,MISSING,-,FLAIR-TENANTS-READY,reached FLAIR-TENANTS-READY)
@printf '>>> test-ovl-panic-mutant: green (each check disabled in turn: all five legs RED)\n'
```

## test-ovl-stub-prune

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 153):
Prerequisites: `build/kernel_flairtenants.elf build/transient_imports.rsp build/.makefile.stamp | build/ovl-gates`
```make
@$(call ovl-stub-prune-check,$(KERNEL_FLAIRTENANTS_ELF))
@printf '>>> test-ovl-stub-prune: green (the flagship carries only the stubs it references)\n'
```

## test-ovl-stub-prune-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 157):
Prerequisites: `test-ovl-stub-prune build/kernel_mut_ovlonesec.elf build/.makefile.stamp`
```make
@if ( $(call ovl-stub-prune-check,$(BUILD)/kernel_mut_ovlonesec.elf) ) > $(OVL_GATE_DIR)/onesec.log 2>&1; then printf '!!! test-ovl-stub-prune-mutant FAIL: one-section stubs survived\n'; exit 1; fi
@grep '!!! ovl-stub-prune FAIL: kernel_mut_ovlonesec.elf carries stubs' $(OVL_GATE_DIR)/onesec.log
@printf '>>> test-ovl-stub-prune-mutant: green (one-section stubs RED)\n'
```

## test-ovl-window

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 68):
Prerequisites: `build/flairovl_kernel_flairtenants.elf build/flairovl_kernel_flairtenants.ovl build/.makefile.stamp | build/ovl-gates`
```make
@set -eu; s=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_start"{print "0x"$$1}'); e=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
 used=$$(( e - s )); sz=$$(wc -c < $(OVL_FLAG).ovl); win=$$(( 0x5E0000 - 0x5C0000 )); \
 [ "$$used" -le $$(( $(OVL_COLD_BUDGET) )) ] || { printf '!!! test-ovl-window FAIL: section COLD %s B > budget %s B\n' "$$used" "$$(( $(OVL_COLD_BUDGET) ))"; exit 1; }; \
 [ "$$sz" -le "$$win" ] || { printf '!!! test-ovl-window FAIL: FLAIR.OVL %s B > window %s B\n' "$$sz" "$$win"; exit 1; }; \
 printf '>>> test-ovl-window: green (section COLD %s of %s B budget; FLAIR.OVL %s of %s B window)\n' "$$used" "$$(( $(OVL_COLD_BUDGET) ))" "$$sz" "$$win"
```

## test-ovl-window-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 78):
Prerequisites: `test-ovl-window build/.makefile.stamp`
```make
@set -eu; s=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_start"{print "0x"$$1}'); e=$$(nm $(OVL_FLAG).elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
 for extra in 0 1; do pad=$$(( $(OVL_COLD_BUDGET) - (e - s) + extra )); \
  printf 'section .bss nobits alloc noexec write align=1\nglobal ovl_budget_pad\novl_budget_pad: resb %s\nsection .note.GNU-stack noalloc noexec nowrite progbits\n' "$$pad" > $(OVL_GATE_DIR)/pad.asm; \
  $(NASM) -f elf32 $(OVL_GATE_DIR)/pad.asm -o $(OVL_GATE_DIR)/pad.o; \
  if $(LD) -m elf_i386 -T $(OVL_LD) --just-symbols=$(OVL_FLAG).elf.syms -o $(OVL_GATE_DIR)/pad.elf $(OVL_HDR_OBJ) $(OVL_SECTION_OBJS) $(OVL_GATE_DIR)/pad.o > $(OVL_GATE_DIR)/pad$$extra.log 2>&1; then \
	[ "$$extra" -eq 0 ] || { printf '!!! test-ovl-window-mutant FAIL: budget+1 linked\n'; exit 1; }; \
	u=$$(nm $(OVL_GATE_DIR)/pad.elf | awk '$$3=="_ovl_cold_bss_end"{print "0x"$$1}'); \
	[ $$(( u - s )) -eq $$(( $(OVL_COLD_BUDGET) )) ] || { printf '!!! test-ovl-window-mutant: pad layout %s != budget\n' "$$(( u - s ))"; exit 1; }; \
  else [ "$$extra" -eq 1 ] || { cat $(OVL_GATE_DIR)/pad0.log; printf '!!! test-ovl-window-mutant FAIL: exactly the budget was refused (off by one)\n'; exit 1; }; fi; \
 done; grep 'FLAIR.OVL section COLD busts its budget' $(OVL_GATE_DIR)/pad1.log
@printf '>>> test-ovl-window-mutant: green (exactly the budget links; budget+1 refused by the link ASSERT)\n'
```

## test-palette-seafoam

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13450):
Prerequisites: `build/test_palette_seafoam build/.makefile.stamp`
```make
@printf ">>> test-palette-seafoam: palette.h desktop_bg == SEAFOAM (0x6F,0xA0,0x8E) == stage2 boot value (exact)\n"
@$(TEST_PALETTE_SEAFOAM)
@printf ">>> test-palette-seafoam: green\n"
```

## test-palette-seafoam-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13455):
Prerequisites: `build/test_palette_seafoam_mutant build/.makefile.stamp`
```make
@printf ">>> test-palette-seafoam-mutant: confirming GRAY_DESKTOP mutant goes RED (Rule 6)\n"
@if $(TEST_PALETTE_SEAFOAM_MUT) >/dev/null 2>&1; then printf '!!! test-palette-seafoam-mutant FAIL: GRAY_DESKTOP PASSED -- the seafoam oracle is decoration\n'; exit 1; else printf '>>> test-palette-seafoam-mutant: green (GRAY_DESKTOP correctly RED -- the oracle bites)\n'; fi
```

## test-process

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14297):
Prerequisites: `build/test_process build/.makefile.stamp`
```make
@printf ">>> test-process: ADR-0013 O-1 host routing/dispatch oracle (refCon demux + click-to-activate + activateEvt pair)\n"
@$(TEST_PROCESS)
@printf ">>> test-process: green\n"
```

## test-process-activate

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14730):
Prerequisites: `build/test_process_activate build/.makefile.stamp`
```make
@printf ">>> test-process-activate: ADR-0013 O-2 host activation/z-order oracle (app-group raise: whole group to front run, hilite-on-front, refCon round-trip)\n"
@$(TEST_PROC_ACTIVATE)
@printf ">>> test-process-activate: green\n"
```

## test-process-activate-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14735):
Prerequisites: `build/test_process_activate_mutant_nohilite build/test_process_activate_mutant_raisefrontonly build/.makefile.stamp`
```make
@printf ">>> test-process-activate-mutant: confirming both self-mutants go RED (Rule 6)\n"
@if $(TEST_PROC_ACTIVATE_MUT_HILITE) >/dev/null 2>&1; then printf '!!! test-process-activate-mutant FAIL: NO_HILITE PASSED -- the hilite-on-front oracle is decoration\n'; exit 1; else printf '>>> test-process-activate-mutant: green (NO_HILITE correctly RED)\n'; fi
@if $(TEST_PROC_ACTIVATE_MUT_FRONT) >/dev/null 2>&1; then printf '!!! test-process-activate-mutant FAIL: RAISE_FRONT_ONLY PASSED -- the whole-group-raise oracle is decoration\n'; exit 1; else printf '>>> test-process-activate-mutant: green (RAISE_FRONT_ONLY correctly RED)\n'; fi
```

## test-process-budget

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14396):
Prerequisites: `build/test_process_budget build/.makefile.stamp`
```make
@printf ">>> test-process-budget: ADR-0013 O-4 host fail-loud launch-budget oracle (over-budget + open-fail -> NULL, no install, reclaimed)\n"
@$(TEST_PROC_BUDGET)
@printf ">>> test-process-budget: green\n"
```

## test-process-budget-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14401):
Prerequisites: `build/test_process_budget_mutant_overcommit build/.makefile.stamp`
```make
@printf ">>> test-process-budget-mutant: confirming BUDGET_MUT_OVERCOMMIT goes RED (Rule 6)\n"
@if $(TEST_PROC_BUDGET_MUT) >/dev/null 2>&1; then printf '!!! test-process-budget-mutant FAIL: OVERCOMMIT PASSED -- the budget oracle is decoration\n'; exit 1; else printf '>>> test-process-budget-mutant: green (OVERCOMMIT correctly RED -- partial install detected)\n'; fi
```

## test-process-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14310):
Prerequisites: `build/test_process_mutant_ignorerefcon build/test_process_mutant_skipactivate build/test_process_mutant_keyundercursor build/test_process_mutant_nosametenantraise build/test_process_mutant_frontstaysactive build/.makefile.stamp`
```make
@printf ">>> test-process-mutant: confirming all five self-mutants go RED (Rule 6; Wave-2 gate)\n"
@$(TEST_PROCESS_MUT_FRONT) > $(TEST_PROCESS_MUT_FRONT).log 2>&1 && { printf '!!! test-process-mutant FAIL: FRONT_STAYS_ACTIVE PASSED -- the leg(g) active-window oracle is decoration\n'; exit 1; } || true
@grep -q 'FAIL.*leg(g2): after the close NO window is active' $(TEST_PROCESS_MUT_FRONT).log || { printf '!!! test-process-mutant FAIL: FRONT_STAYS_ACTIVE went RED for the wrong reason\n'; cat $(TEST_PROCESS_MUT_FRONT).log; exit 1; }
@printf '>>> test-process-mutant: green (FRONT_STAYS_ACTIVE correctly RED -- a background window would stay active under the foreground bar)\n'
@if $(TEST_PROCESS_MUT_REFCON) >/dev/null 2>&1; then printf '!!! test-process-mutant FAIL: IGNORE_REFCON PASSED -- the routing oracle is decoration\n'; exit 1; else printf '>>> test-process-mutant: green (IGNORE_REFCON correctly RED)\n'; fi
@if $(TEST_PROCESS_MUT_ACTIVATE) >/dev/null 2>&1; then printf '!!! test-process-mutant FAIL: SKIP_ACTIVATE_PAIR PASSED -- the activate-pair oracle is decoration\n'; exit 1; else printf '>>> test-process-mutant: green (SKIP_ACTIVATE_PAIR correctly RED)\n'; fi
@if $(TEST_PROCESS_MUT_KEY) >/dev/null 2>&1; then printf '!!! test-process-mutant FAIL: KEY_TO_UNDER_CURSOR PASSED -- the key-routing oracle is decoration\n'; exit 1; else printf '>>> test-process-mutant: green (KEY_TO_UNDER_CURSOR correctly RED)\n'; fi
@if $(TEST_PROCESS_MUT_RAISE) >/dev/null 2>&1; then printf '!!! test-process-mutant FAIL: NO_SAMETENANT_RAISE PASSED -- the leg(f) raise oracle is decoration (bug initech-tpzf)\n'; exit 1; else printf '>>> test-process-mutant: green (NO_SAMETENANT_RAISE correctly RED -- the clicked window really does come forward)\n'; fi
```

## test-process-mutant-build

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14304):
Prerequisites: `build/test_process_mutant_ignorerefcon build/test_process_mutant_skipactivate build/test_process_mutant_keyundercursor build/test_process_mutant_nosametenantraise build/.makefile.stamp`
```make
@printf ">>> test-process-mutant-build: all three self-mutants COMPILE (IGNORE_REFCON / SKIP_ACTIVATE_PAIR / KEY_TO_UNDER_CURSOR)\n"
```

## test-process-teardown

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14355):
Prerequisites: `build/test_process_teardown build/.makefile.stamp`
```make
@printf ">>> test-process-teardown: ADR-0013 O-3 host teardown/leak oracle (bump+free-list avail: cycle-1 drop, cycles>=2 stable; ADR-0013 AC-2 split-arena death-survival sub-test)\n"
@$(TEST_PROC_TEARDOWN)
@printf ">>> test-process-teardown: green\n"
```

## test-process-teardown-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14366):
Prerequisites: `build/test_process_teardown_mutant_leakblock build/test_process_teardown_mutant_leakrecords build/test_process_teardown_mutant_ubd0 build/test_process_teardown_mutant_close_hide_only build/.makefile.stamp`
```make
@printf ">>> test-process-teardown-mutant: confirming LEAK_BLOCK + LEAK_RECORDS + UBD0_RECORDS_IN_DATA_ARENA + CLOSE_HIDE_ONLY all go RED (Rule 6)\n"
@if $(TEST_PROC_TEARDOWN_MUT) >/dev/null 2>&1; then printf '!!! test-process-teardown-mutant FAIL: LEAK_BLOCK PASSED -- the data-leak oracle is decoration\n'; exit 1; else printf '>>> test-process-teardown-mutant: green (LEAK_BLOCK correctly RED -- data-block avail drifts down)\n'; fi
@if $(TEST_PROC_TEARDOWN_MUT_RECS) >/dev/null 2>&1; then printf '!!! test-process-teardown-mutant FAIL: LEAK_RECORDS PASSED -- the records-leak oracle is decoration\n'; exit 1; else printf '>>> test-process-teardown-mutant: green (LEAK_RECORDS correctly RED -- records-block avail drifts down)\n'; fi
@if $(TEST_PROC_TEARDOWN_MUT_UBD0) >/dev/null 2>&1; then printf '!!! test-process-teardown-mutant FAIL: UBD0_RECORDS_IN_DATA_ARENA PASSED -- AC-2 death-survival is decoration\n'; exit 1; else printf '>>> test-process-teardown-mutant: green (UBD0 records-in-data correctly RED -- kill reads the scribbled DATA arena)\n'; fi
@if $(TEST_PROC_TEARDOWN_MUT_CLOSE) >/dev/null 2>&1; then printf '!!! test-process-teardown-mutant FAIL: CLOSE_HIDE_ONLY PASSED -- terminate-on-close/key-routing oracle is decoration\n'; exit 1; else printf '>>> test-process-teardown-mutant: green (CLOSE_HIDE_ONLY correctly RED -- post-close key targets the hidden dead-designate tenant)\n'; fi
```

## test-process-update

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14777):
Prerequisites: `build/test_process_update build/.makefile.stamp`
```make
@printf ">>> test-process-update: ADR-0013 updateEvt-spine host oracle (flair_route_updates: damage -> owning tenant, validate, unowned tolerated)\n"
@$(TEST_PROC_UPDATE)
@printf ">>> test-process-update: green\n"
```

## test-process-update-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14782):
Prerequisites: `build/test_process_update_mutant_wrongowner build/test_process_update_mutant_skipvalidate build/test_process_update_mutant_no_visclip build/.makefile.stamp`
```make
@printf ">>> test-process-update-mutant: confirming WRONG_OWNER + SKIP_VALIDATE + UPDATE_NO_VISCLIP go RED (Rule 6)\n"
@if $(TEST_PROC_UPDATE_MUT_OWNER) >/dev/null 2>&1; then printf '!!! test-process-update-mutant FAIL: WRONG_OWNER PASSED -- the update-routing oracle is decoration\n'; exit 1; else printf '>>> test-process-update-mutant: green (WRONG_OWNER correctly RED)\n'; fi
@if $(TEST_PROC_UPDATE_MUT_VALID) >/dev/null 2>&1; then printf '!!! test-process-update-mutant FAIL: SKIP_VALIDATE PASSED -- the validate-clears oracle is decoration\n'; exit 1; else printf '>>> test-process-update-mutant: green (SKIP_VALIDATE correctly RED)\n'; fi
@if $(TEST_PROC_UPDATE_MUT_VIS) >/dev/null 2>&1; then printf '!!! test-process-update-mutant FAIL: UPDATE_NO_VISCLIP PASSED -- the stale-seed stomp oracle is decoration\n'; exit 1; else printf '>>> test-process-update-mutant: green (UPDATE_NO_VISCLIP correctly RED -- BACK stomped the FRONT overlap)\n'; fi
```

## test-resource

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14434):
Prerequisites: `build/test_resource build/.makefile.stamp`
```make
@printf ">>> test-resource: ADR-0012 D-2b / initech-0w45 Resource Manager -- real BE Mac resource-fork parse + type+ID lookup (round-trip vs independent hand-authored fork+expect golden)\n"
@$(TEST_RESOURCE)
@printf ">>> test-resource: green\n"
```

## test-resource-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14439):
Prerequisites: `build/test_resource_mutant_ignoretype build/test_resource_mutant_countoffbyone build/.makefile.stamp`
```make
@printf ">>> test-resource-mutant: confirming RES_MUT_IGNORE_TYPE + RES_MUT_COUNT_OFF_BY_ONE go RED (Rule 6)\n"
@if $(TEST_RESOURCE_MUT_TYPE) >/dev/null 2>&1; then printf '!!! test-resource-mutant FAIL: IGNORE_TYPE PASSED -- the type-key oracle is decoration\n'; exit 1; else printf '>>> test-resource-mutant: green (IGNORE_TYPE correctly RED -- wrong-type lookups leak)\n'; fi
@if $(TEST_RESOURCE_MUT_COUNT) >/dev/null 2>&1; then printf '!!! test-resource-mutant FAIL: COUNT_OFF_BY_ONE PASSED -- the count-1 decode oracle is decoration\n'; exit 1; else printf '>>> test-resource-mutant: green (COUNT_OFF_BY_ONE correctly RED -- last type missed)\n'; fi
```

## test-scrap

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14479):
Prerequisites: `build/test_scrap build/.makefile.stamp`
```make
@printf ">>> test-scrap: ADR-0012 D-2b / initech-b2vk Scrap (Clipboard) -- shell-owned cross-tenant copy/paste, multi-flavor TEXT+PICT (round-trip vs independent hand-authored op/byte golden)\n"
@$(TEST_SCRAP)
@printf ">>> test-scrap: green\n"
```

## test-scrap-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14484):
Prerequisites: `build/test_scrap_mutant_ignoreflavor build/test_scrap_mutant_noincrement build/test_scrap_mutant_noclear build/.makefile.stamp`
```make
@printf ">>> test-scrap-mutant: confirming IGNORE_FLAVOR + NO_INCREMENT + NO_CLEAR go RED (Rule 6)\n"
@if $(TEST_SCRAP_MUT_FLAVOR) >/dev/null 2>&1; then printf '!!! test-scrap-mutant FAIL: IGNORE_FLAVOR PASSED -- the flavor-key oracle is decoration\n'; exit 1; else printf '>>> test-scrap-mutant: green (IGNORE_FLAVOR correctly RED -- flavors collapse)\n'; fi
@if $(TEST_SCRAP_MUT_NOINC) >/dev/null 2>&1; then printf '!!! test-scrap-mutant FAIL: NO_INCREMENT PASSED -- the scrapCount oracle is decoration\n'; exit 1; else printf '>>> test-scrap-mutant: green (NO_INCREMENT correctly RED -- scrapCount frozen)\n'; fi
@if $(TEST_SCRAP_MUT_NOCLEAR) >/dev/null 2>&1; then printf '!!! test-scrap-mutant FAIL: NO_CLEAR PASSED -- the ZeroScrap oracle is decoration\n'; exit 1; else printf '>>> test-scrap-mutant: green (NO_CLEAR correctly RED -- stale flavors survive)\n'; fi
```

## test-scrollbar-look

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32885):
Prerequisites: `build/test_scrollbar_look build/.makefile.stamp`
```make
@$< "$(SCROLLBAR_LOOK_PNG)"
@printf '>>> test-scrollbar-look: green (both depths, both drawers; six independent crops)\n'
```

## test-scrollbar-look-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 32888):
Prerequisites: `test-scrollbar-look build/test_scrollbar_look_mut_SB_LOOK_MUT_RAMP_FLAT build/test_scrollbar_look_mut_SB_LOOK_MUT_GRIPS_MISSING build/test_scrollbar_look_mut_SB_LOOK_MUT_CORNERS_BEVELLED build/.makefile.stamp`
```make
@for m in $(SCROLLBAR_LOOK_MUTANTS); do \
	if $(BUILD)/test_scrollbar_look_mut_$$m "$(SCROLLBAR_LOOK_PNG)" > $(BUILD)/scrollbar_look_$$m.log 2>&1; then \
		printf 'FAIL test-scrollbar-look-mutant: %s survived\n' "$$m"; exit 1; fi; \
	grep -q 'FAIL crop' $(BUILD)/scrollbar_look_$$m.log || { cat $(BUILD)/scrollbar_look_$$m.log; exit 1; }; \
	grep -q 'FAIL.*SL4 Control' $(BUILD)/scrollbar_look_$$m.log || { printf 'FAIL control drawer mutant escaped\n'; exit 1; }; \
	case "$$m" in *CORNERS*) leg='SL3 all four';; *) leg='SL2 chrome thumb';; esac; \
	grep -q "FAIL.*$$leg" $(BUILD)/scrollbar_look_$$m.log || { printf 'FAIL chrome drawer mutant escaped\n'; exit 1; }; \
	printf '>>> test-scrollbar-look-mutant: %s correctly RED\n' "$$m"; \
done
```

## test-skin-era-frozen

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16165):
Prerequisites: `build/test_skin_era_frozen build/.makefile.stamp`
```make
@printf ">>> test-skin-era-frozen: accretion digest over the LOCKED base rows (append, never mutate)\n"
@$(TEST_SKIN_FROZEN)
@printf ">>> test-skin-era-frozen: green\n"
```

## test-skin-era-frozen-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16169):
Prerequisites: `build/test_skin_era_frozen_mutant build/.makefile.stamp`
```make
@if $(TEST_SKIN_FROZEN_MUT) >/dev/null 2>&1; then printf '!!! test-skin-era-frozen-mutant FAIL: a mutated base-row field did NOT change the digest\n'; exit 1; else printf '>>> test-skin-era-frozen-mutant: green (a mutated base row correctly RED)\n'; fi
```

## test-skin-era-frozen-mutant-baserow

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16171):
Prerequisites: `build/test_skin_era_frozen_mutant_baserow build/.makefile.stamp`
```make
@if $(TEST_SKIN_FROZEN_MUT2) >/dev/null 2>&1; then printf '!!! test-skin-era-frozen-mutant-baserow FAIL: a second mutated base-row field did NOT change the digest\n'; exit 1; else printf '>>> test-skin-era-frozen-mutant-baserow: green (a second mutated base row correctly RED)\n'; fi
```

## test-skin-teal

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16157):
Prerequisites: `build/test_skin_teal build/.makefile.stamp`
```make
@printf ">>> test-skin-teal: era registry authored-grade -- teal/bevel/win31 slots == the canon (by-inclusion)\n"
@$(TEST_SKIN_TEAL)
@printf ">>> test-skin-teal: green\n"
```

## test-skin-teal-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16161):
Prerequisites: `build/test_skin_teal_mutant build/.makefile.stamp`
```make
@if $(TEST_SKIN_TEAL_MUT) >/dev/null 2>&1; then printf '!!! test-skin-teal-mutant FAIL: a slot drifting from the canon was NOT caught\n'; exit 1; else printf '>>> test-skin-teal-mutant: green (a canon-drifted slot correctly RED)\n'; fi
```

## test-skin-teal-mutant-plat

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16163):
Prerequisites: `build/test_skin_teal_mutant_plat build/.makefile.stamp`
```make
@if $(TEST_SKIN_TEAL_MUT2) >/dev/null 2>&1; then printf '!!! test-skin-teal-mutant-plat FAIL: a Platinum teal slot drifting from the canon was NOT caught\n'; exit 1; else printf '>>> test-skin-teal-mutant-plat: green (a Platinum canon-drifted slot correctly RED)\n'; fi
```

## test-ssim

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34908):
Prerequisites: `build/test_ssim build/ssim build/.makefile.stamp`
```make
@$(SSIM_TEST_BIN) $(SSIM_BIN) $(BUILD)
```

## test-ssim-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 34910):
Prerequisites: `test-ssim build/ssim_mut_WINDOW_9 build/ssim_mut_K1 build/ssim_mut_K2 build/ssim_mut_SIGMA build/ssim_mut_LUMA build/.makefile.stamp`
```make
@for m in $(SSIM_MUTANTS); do \
 rc=0; $(SSIM_TEST_BIN) $(BUILD)/ssim_mut_$$m $(BUILD) > $(BUILD)/ssim_mut_$$m.log 2>&1 || rc=$$?; \
 [ "$$rc" = 1 ] || { cat $(BUILD)/ssim_mut_$$m.log; printf 'FAIL SSIM_MUT_%s wrong exit %s\n' "$$m" "$$rc"; exit 1; }; \
 case "$$m" in WINDOW_9|SIGMA) rule='GAUSSIAN one-pixel';; K1) rule='K1 constant-patch';; K2) rule='K2 impulse255';; LUMA) rule='LUMA RGB red';; esac; \
 grep -q "^RED $$rule" $(BUILD)/ssim_mut_$$m.log || { cat $(BUILD)/ssim_mut_$$m.log; exit 1; }; \
 printf '>>> SSIM_MUT_%s correctly RED: %s\n' "$$m" "$$(grep -m1 "^RED $$rule" $(BUILD)/ssim_mut_$$m.log)"; \
 done
@printf '>>> test-ssim-mutant: green (five named math mutants RED)\n'
```

## test-stdfile

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14688):
Prerequisites: `build/test_stdfile build/.makefile.stamp`
```make
@printf ">>> test-stdfile: FLAIR Phase 4.5 / initech-gymo Standard File (SFGetFile) -- navigate/select/return over the real List Manager (round-trip vs independent hand-authored golden)\n"
@$(TEST_STDFILE)
@printf ">>> test-stdfile: green\n"
```

## test-stdfile-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14693):
Prerequisites: `build/test_stdfile_mutant_wrongcell build/test_stdfile_mutant_nonavigate build/test_stdfile_mutant_cancelgood build/.makefile.stamp`
```make
@printf ">>> test-stdfile-mutant: confirming RETURN_WRONG_CELL + NO_NAVIGATE + CANCEL_GOOD go RED (Rule 6)\n"
@if $(TEST_STDFILE_MUT_CELL) >/dev/null 2>&1; then printf '!!! test-stdfile-mutant FAIL: SF_MUT_RETURN_WRONG_CELL PASSED -- the doOpen selection oracle is decoration\n'; exit 1; else printf '>>> test-stdfile-mutant: green (SF_MUT_RETURN_WRONG_CELL correctly RED)\n'; fi
@if $(TEST_STDFILE_MUT_NAV) >/dev/null 2>&1; then printf '!!! test-stdfile-mutant FAIL: SF_MUT_NO_NAVIGATE PASSED -- the folder re-enumeration oracle is decoration\n'; exit 1; else printf '>>> test-stdfile-mutant: green (SF_MUT_NO_NAVIGATE correctly RED)\n'; fi
@if $(TEST_STDFILE_MUT_CANCEL) >/dev/null 2>&1; then printf '!!! test-stdfile-mutant FAIL: SF_MUT_CANCEL_GOOD PASSED -- the Cancel-never-confirms oracle is decoration\n'; exit 1; else printf '>>> test-stdfile-mutant: green (SF_MUT_CANCEL_GOOD correctly RED)\n'; fi
```

## test-tbx-file

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 31955):
Prerequisites: `build/test_tbx_file build/.makefile.stamp`
```make
@$(BUILD)/test_tbx_file
@printf '>>> test-tbx-file: green\n'
```

## test-tbx-file-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 31958):
Prerequisites: `build/test_tbx_file_mutant_TBX_MUT_FILE_BOUNDS build/test_tbx_file_mutant_TBX_MUT_FILE_OWNER build/test_tbx_file_mutant_TBX_MUT_FILE_LEAK build/test_tbx_file_mutant_TBX_MUT_FILE_RENAME_EDI build/test_tbx_file_mutant_TBX_MUT_FILE_CREATE_NEW_TRUNC build/.makefile.stamp`
```make
@for pair in TBX_MUT_FILE_BOUNDS:F2 TBX_MUT_FILE_OWNER:F3 TBX_MUT_FILE_LEAK:F4 TBX_MUT_FILE_RENAME_EDI:F5 TBX_MUT_FILE_CREATE_NEW_TRUNC:F6; do \
 m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_tbx_file_mutant_$$m; \
 if $$bin > $$bin.log 2>&1; then printf '!!! test-tbx-file-mutant: %s survived\n' $$m; exit 1; fi; \
 grep -q "FAIL.*$$why" $$bin.log || { cat $$bin.log; exit 1; }; \
 printf '>>> test-tbx-file-mutant: %s correctly RED (%s)\n' $$m $$why; done
@printf '>>> test-tbx-file-mutant: green\n'
```

## test-tbx-menu

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15022):
Prerequisites: `build/test_tbx_menu build/.makefile.stamp`
```make
@printf ">>> test-tbx-menu: a disk tenant hears its own menu choices -- key route, mouse/key convergence, bar identity (tdnl.31)\n"
@$(TEST_TBX_MENU)
@printf ">>> test-tbx-menu: green\n"
```

## test-tbx-menu-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 15027):
Prerequisites: `build/test_tbx_menu_mutant_TBX_MUT_MENU_KEY_PLAIN build/test_tbx_menu_mutant_TBX_MUT_MENU_ANY_BAR build/.makefile.stamp`
```make
@for pair in 'TBX_MUT_MENU_KEY_PLAIN:M1 Ctrl-q on the tenant bar is File > Quit' 'TBX_MUT_MENU_ANY_BAR:M3 a choice in another bar'; do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_tbx_menu_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-tbx-menu-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -F "$$why" $$bin.log | grep -q FAIL || { printf '!!! test-tbx-menu-mutant FAIL: %s went RED for the wrong reason\n' "$$m"; cat $$bin.log; exit 1; }; \
	printf '>>> test-tbx-menu-mutant: %s correctly RED (%s)\n' "$$m" "$$why"; \
done
@printf '>>> test-tbx-menu-mutant: green (both mutants RED for the named reason)\n'
```

## test-tbx-mutant-BLITTER_MUT_COPY32_ZERO

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 37):
Prerequisites: `build/tbx_mut_BLITTER_MUT_COPY32_ZERO build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL BLITTER_MUT_COPY32_ZERO: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*asci' $<.log || { printf 'FAIL BLITTER_MUT_COPY32_ZERO: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> BLITTER_MUT_COPY32_ZERO: RED: '; grep -Em1 'FAIL.*asci' $<.log
```

## test-tbx-mutant-CTRL_MUT_DISABLED_HIT

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 31):
Prerequisites: `build/tbx_mut_CTRL_MUT_DISABLED_HIT build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL CTRL_MUT_DISABLED_HIT: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*ct5n' $<.log || { printf 'FAIL CTRL_MUT_DISABLED_HIT: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> CTRL_MUT_DISABLED_HIT: RED: '; grep -Em1 'FAIL.*ct5n' $<.log
```

## test-tbx-mutant-CTRL_MUT_NULL_SILENT

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 30):
Prerequisites: `build/tbx_mut_CTRL_MUT_NULL_SILENT build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL CTRL_MUT_NULL_SILENT: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*gxo2' $<.log || { printf 'FAIL CTRL_MUT_NULL_SILENT: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> CTRL_MUT_NULL_SILENT: RED: '; grep -Em1 'FAIL.*gxo2' $<.log
```

## test-tbx-mutant-CTRL_MUT_SHORT_BUTTON

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 32):
Prerequisites: `build/tbx_mut_CTRL_MUT_SHORT_BUTTON build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL CTRL_MUT_SHORT_BUTTON: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*44la' $<.log || { printf 'FAIL CTRL_MUT_SHORT_BUTTON: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> CTRL_MUT_SHORT_BUTTON: RED: '; grep -Em1 'FAIL.*44la' $<.log
```

## test-tbx-mutant-CTRL_MUT_SHORT_LIVE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 33):
Prerequisites: `build/tbx_mut_CTRL_MUT_SHORT_LIVE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL CTRL_MUT_SHORT_LIVE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*mktj' $<.log || { printf 'FAIL CTRL_MUT_SHORT_LIVE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> CTRL_MUT_SHORT_LIVE: RED: '; grep -Em1 'FAIL.*mktj' $<.log
```

## test-tbx-mutant-EVENT_MUT_SILENT_DROP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 41):
Prerequisites: `build/tbx_mut_EVENT_MUT_SILENT_DROP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL EVENT_MUT_SILENT_DROP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*owpg' $<.log || { printf 'FAIL EVENT_MUT_SILENT_DROP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> EVENT_MUT_SILENT_DROP: RED: '; grep -Em1 'FAIL.*owpg' $<.log
```

## test-tbx-mutant-EVENT_MUT_TICK_WRAP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 40):
Prerequisites: `build/tbx_mut_EVENT_MUT_TICK_WRAP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL EVENT_MUT_TICK_WRAP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*gwts' $<.log || { printf 'FAIL EVENT_MUT_TICK_WRAP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> EVENT_MUT_TICK_WRAP: RED: '; grep -Em1 'FAIL.*gwts' $<.log
```

## test-tbx-mutant-FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 25):
Prerequisites: `build/tbx_mut_FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*r081' $<.log || { printf 'FAIL FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW: RED: '; grep -Em1 'FAIL.*r081' $<.log
```

## test-tbx-mutant-FLAIR_HEAP_MUTATE_SPAN_OVERFLOW

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 26):
Prerequisites: `build/tbx_mut_FLAIR_HEAP_MUTATE_SPAN_OVERFLOW build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL FLAIR_HEAP_MUTATE_SPAN_OVERFLOW: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*5l5n' $<.log || { printf 'FAIL FLAIR_HEAP_MUTATE_SPAN_OVERFLOW: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> FLAIR_HEAP_MUTATE_SPAN_OVERFLOW: RED: '; grep -Em1 'FAIL.*5l5n' $<.log
```

## test-tbx-mutant-FLAIR_HEAP_MUTATE_WALK_REUSE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 27):
Prerequisites: `build/tbx_mut_FLAIR_HEAP_MUTATE_WALK_REUSE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL FLAIR_HEAP_MUTATE_WALK_REUSE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*zhjo' $<.log || { printf 'FAIL FLAIR_HEAP_MUTATE_WALK_REUSE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> FLAIR_HEAP_MUTATE_WALK_REUSE: RED: '; grep -Em1 'FAIL.*zhjo' $<.log
```

## test-tbx-mutant-KBD_MUT_SILENT_DROP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 42):
Prerequisites: `build/tbx_mut_KBD_MUT_SILENT_DROP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL KBD_MUT_SILENT_DROP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*owpg' $<.log || { printf 'FAIL KBD_MUT_SILENT_DROP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> KBD_MUT_SILENT_DROP: RED: '; grep -Em1 'FAIL.*owpg' $<.log
```

## test-tbx-mutant-LIST_MUT_COLUMN_HEADROOM

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 18):
Prerequisites: `build/tbx_mut_LIST_MUT_COLUMN_HEADROOM build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL LIST_MUT_COLUMN_HEADROOM: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*42dq' $<.log || { printf 'FAIL LIST_MUT_COLUMN_HEADROOM: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> LIST_MUT_COLUMN_HEADROOM: RED: '; grep -Em1 'FAIL.*42dq' $<.log
```

## test-tbx-mutant-LIST_MUT_NO_OVERFLOW_CHECK

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 17):
Prerequisites: `build/tbx_mut_LIST_MUT_NO_OVERFLOW_CHECK build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL LIST_MUT_NO_OVERFLOW_CHECK: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*nbnx' $<.log || { printf 'FAIL LIST_MUT_NO_OVERFLOW_CHECK: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> LIST_MUT_NO_OVERFLOW_CHECK: RED: '; grep -Em1 'FAIL.*nbnx' $<.log
```

## test-tbx-mutant-LIST_MUT_NULL_CLICK_MISS

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 15):
Prerequisites: `build/tbx_mut_LIST_MUT_NULL_CLICK_MISS build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL LIST_MUT_NULL_CLICK_MISS: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*6vvw' $<.log || { printf 'FAIL LIST_MUT_NULL_CLICK_MISS: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> LIST_MUT_NULL_CLICK_MISS: RED: '; grep -Em1 'FAIL.*6vvw' $<.log
```

## test-tbx-mutant-LIST_MUT_ROW_WRAP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 16):
Prerequisites: `build/tbx_mut_LIST_MUT_ROW_WRAP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL LIST_MUT_ROW_WRAP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*8mip' $<.log || { printf 'FAIL LIST_MUT_ROW_WRAP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> LIST_MUT_ROW_WRAP: RED: '; grep -Em1 'FAIL.*8mip' $<.log
```

## test-tbx-mutant-MOUSE_MUT_EARLY_BUTTON

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 44):
Prerequisites: `build/tbx_mut_MOUSE_MUT_EARLY_BUTTON build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL MOUSE_MUT_EARLY_BUTTON: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*qupy' $<.log || { printf 'FAIL MOUSE_MUT_EARLY_BUTTON: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> MOUSE_MUT_EARLY_BUTTON: RED: '; grep -Em1 'FAIL.*qupy' $<.log
```

## test-tbx-mutant-MOUSE_MUT_SIGN_BYTE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 43):
Prerequisites: `build/tbx_mut_MOUSE_MUT_SIGN_BYTE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL MOUSE_MUT_SIGN_BYTE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*qupy' $<.log || { printf 'FAIL MOUSE_MUT_SIGN_BYTE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> MOUSE_MUT_SIGN_BYTE: RED: '; grep -Em1 'FAIL.*qupy' $<.log
```

## test-tbx-mutant-PROC_MUT_REGISTERED_FREE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 28):
Prerequisites: `build/tbx_mut_PROC_MUT_REGISTERED_FREE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL PROC_MUT_REGISTERED_FREE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*k0n8' $<.log || { printf 'FAIL PROC_MUT_REGISTERED_FREE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> PROC_MUT_REGISTERED_FREE: RED: '; grep -Em1 'FAIL.*k0n8' $<.log
```

## test-tbx-mutant-PROC_MUT_STALE_MAGIC

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 29):
Prerequisites: `build/tbx_mut_PROC_MUT_STALE_MAGIC build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL PROC_MUT_STALE_MAGIC: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*vibs' $<.log || { printf 'FAIL PROC_MUT_STALE_MAGIC: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> PROC_MUT_STALE_MAGIC: RED: '; grep -Em1 'FAIL.*vibs' $<.log
```

## test-tbx-mutant-RES_MUT_DATA_ESCAPE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 22):
Prerequisites: `build/tbx_mut_RES_MUT_DATA_ESCAPE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RES_MUT_DATA_ESCAPE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*u37e' $<.log || { printf 'FAIL RES_MUT_DATA_ESCAPE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RES_MUT_DATA_ESCAPE: RED: '; grep -Em1 'FAIL.*u37e' $<.log
```

## test-tbx-mutant-RES_MUT_DATA_WRAP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 21):
Prerequisites: `build/tbx_mut_RES_MUT_DATA_WRAP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RES_MUT_DATA_WRAP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*nc0z' $<.log || { printf 'FAIL RES_MUT_DATA_WRAP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RES_MUT_DATA_WRAP: RED: '; grep -Em1 'FAIL.*nc0z' $<.log
```

## test-tbx-mutant-RES_MUT_MAP_PREAMBLE

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 19):
Prerequisites: `build/tbx_mut_RES_MUT_MAP_PREAMBLE build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RES_MUT_MAP_PREAMBLE: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*6mfi' $<.log || { printf 'FAIL RES_MUT_MAP_PREAMBLE: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RES_MUT_MAP_PREAMBLE: RED: '; grep -Em1 'FAIL.*6mfi' $<.log
```

## test-tbx-mutant-RES_MUT_TYPE_WRAP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 20):
Prerequisites: `build/tbx_mut_RES_MUT_TYPE_WRAP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RES_MUT_TYPE_WRAP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*s3bh' $<.log || { printf 'FAIL RES_MUT_TYPE_WRAP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RES_MUT_TYPE_WRAP: RED: '; grep -Em1 'FAIL.*s3bh' $<.log
```

## test-tbx-mutant-RGN_MUT_ALIAS_RESET

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 49):
Prerequisites: `build/tbx_mut_RGN_MUT_ALIAS_RESET build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RGN_MUT_ALIAS_RESET: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*bz8f' $<.log || { printf 'FAIL RGN_MUT_ALIAS_RESET: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RGN_MUT_ALIAS_RESET: RED: '; grep -Em1 'FAIL.*bz8f' $<.log
```

## test-tbx-mutant-RGN_MUT_REPEATED_UNION

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 50):
Prerequisites: `build/tbx_mut_RGN_MUT_REPEATED_UNION build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL RGN_MUT_REPEATED_UNION: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*0i1m' $<.log || { printf 'FAIL RGN_MUT_REPEATED_UNION: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> RGN_MUT_REPEATED_UNION: RED: '; grep -Em1 'FAIL.*0i1m' $<.log
```

## test-tbx-mutant-SCRAP_MUT_FULL_FLAVOR

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 23):
Prerequisites: `build/tbx_mut_SCRAP_MUT_FULL_FLAVOR build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SCRAP_MUT_FULL_FLAVOR: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*iuwq' $<.log || { printf 'FAIL SCRAP_MUT_FULL_FLAVOR: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SCRAP_MUT_FULL_FLAVOR: RED: '; grep -Em1 'FAIL.*iuwq' $<.log
```

## test-tbx-mutant-SCRAP_MUT_NULL_DATA

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 24):
Prerequisites: `build/tbx_mut_SCRAP_MUT_NULL_DATA build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SCRAP_MUT_NULL_DATA: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*m1ry' $<.log || { printf 'FAIL SCRAP_MUT_NULL_DATA: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SCRAP_MUT_NULL_DATA: RED: '; grep -Em1 'FAIL.*m1ry' $<.log
```

## test-tbx-mutant-SF_MUT_SWALLOW_NAV_ERROR

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 45):
Prerequisites: `build/tbx_mut_SF_MUT_SWALLOW_NAV_ERROR build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SF_MUT_SWALLOW_NAV_ERROR: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*q9cr' $<.log || { printf 'FAIL SF_MUT_SWALLOW_NAV_ERROR: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SF_MUT_SWALLOW_NAV_ERROR: RED: '; grep -Em1 'FAIL.*q9cr' $<.log
```

## test-tbx-mutant-SURFACE_MUT_BGR_SWAP

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 36):
Prerequisites: `build/tbx_mut_SURFACE_MUT_BGR_SWAP build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SURFACE_MUT_BGR_SWAP: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*asci' $<.log || { printf 'FAIL SURFACE_MUT_BGR_SWAP: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SURFACE_MUT_BGR_SWAP: RED: '; grep -Em1 'FAIL.*asci' $<.log
```

## test-tbx-mutant-SURFACE_MUT_CLEAR_FLAT

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 39):
Prerequisites: `build/tbx_mut_SURFACE_MUT_CLEAR_FLAT build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SURFACE_MUT_CLEAR_FLAT: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*gv2x' $<.log || { printf 'FAIL SURFACE_MUT_CLEAR_FLAT: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SURFACE_MUT_CLEAR_FLAT: RED: '; grep -Em1 'FAIL.*gv2x' $<.log
```

## test-tbx-mutant-SURFACE_MUT_INDEX_ZERO

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 38):
Prerequisites: `build/tbx_mut_SURFACE_MUT_INDEX_ZERO build/.makefile.stamp`
```make
@ulimit -c 0; if $< > $<.log 2>&1; then printf 'FAIL SURFACE_MUT_INDEX_ZERO: mutant survived\n'; exit 1; fi; grep -Eq 'FAIL.*5le3' $<.log || { printf 'FAIL SURFACE_MUT_INDEX_ZERO: RED for the wrong reason\n'; cat $<.log; exit 1; }; printf '>>> SURFACE_MUT_INDEX_ZERO: RED: '; grep -Em1 'FAIL.*5le3' $<.log
```

## test-te-surface

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14560):
Prerequisites: `build/test_te_surface build/.makefile.stamp`
```make
@printf '>>> test-te-surface: TextEdit editing surface vs an independent reference model (initech-tdnl.16)\n'
@$(TEST_TE_SURFACE)
@printf '>>> test-te-surface: green\n'
```

## test-te-surface-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14566):
Prerequisites: `build/test_te_surface_mutant_TE_MUT_HIT_FLOOR build/test_te_surface_mutant_TE_MUT_HIT_PAST_CR build/test_te_surface_mutant_TE_MUT_ARROW_NO_COLLAPSE build/test_te_surface_mutant_TE_MUT_EXTEND_ANCHOR build/test_te_surface_mutant_TE_MUT_PROBE_NO_LINES build/.makefile.stamp`
```make
@for pair in $(TE_SURFACE_MUTANTS); do \
	m=$${pair%%:*}; why=$${pair#*:}; bin=$(BUILD)/test_te_surface_mutant_$$m; \
	if $$bin > $$bin.log 2>&1; then printf '!!! test-te-surface-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	grep -q 'checks,' $$bin.log || { printf '!!! test-te-surface-mutant FAIL: %s died without a summary -- RED is meaningless\n' "$$m"; tail -5 $$bin.log; exit 1; }; \
	grep -q "^RED-REASON $$why:" $$bin.log || { printf '!!! test-te-surface-mutant FAIL: %s went RED for the wrong reason (want %s)\n' "$$m" "$$why"; grep '^RED-REASON' $$bin.log; exit 1; }; \
	printf '>>> test-te-surface-mutant: %s correctly RED (%s)\n' "$$m" "$$(grep -m1 "^RED-REASON $$why:" $$bin.log)"; \
done
@printf '>>> test-te-surface-mutant: green (all five mutants RED for the named reason)\n'
```

## test-tenant-load

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 7591):
Prerequisites: `build/test_tenant_load build/.makefile.stamp`
```make
@printf ">>> test-tenant-load: parameterized-base reloc + PSP@block + BSS zero + refusals + psp_build address-independence + EXEC path untouched\n"
@$(TEST_TENANT_LOAD)
@printf ">>> test-tenant-load: green\n"
```

## test-tenant-load-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 7596):
Prerequisites: `build/test_tenant_load_mutant_progimage build/.makefile.stamp`
```make
@printf ">>> test-tenant-load-mutant: the PROGRAM_IMAGE-base mutant must go RED (Rule 6)\n"
@if $(TEST_TENANT_LOAD_MUT) > $(BUILD)/test_tenant_load_mutant.log 2>&1; then \
	printf '!!! test-tenant-load-mutant FAIL: the mutant PASSED -- the fixture is decoration\n'; exit 1; \
fi
@grep -q 'T1 every site relocated to link + block base' $(BUILD)/test_tenant_load_mutant.log \
	|| { printf '!!! test-tenant-load-mutant FAIL: RED, but not for the relocation-base reason\n'; cat $(BUILD)/test_tenant_load_mutant.log; exit 1; }
@printf ">>> test-tenant-load-mutant: green (RED for the named reason: relocated against PROGRAM_IMAGE, not the block)\n"
```

## test-text

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13328):
Prerequisites: `build/test_text build/.makefile.stamp`
```make
@printf ">>> test-text: proportional Chicago/Geneva text -- measure (sum of advances) + draw + center\n"
@$(TEST_TEXT)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_TEXT_OPT) $(TEXT_INC) -c os/flair/text.c -o $(BUILD)/text_freestanding.o \
	|| { printf '!!! test-text FAIL: text.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-text: green\n"
```

## test-text-clip

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13398):
Prerequisites: `build/test_text_clip build/.makefile.stamp`
```make
@$(TEST_TEXT_CLIP)
```

## test-text-clip-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13400):
Prerequisites: `build/test_text_clip_mut_ignore build/.makefile.stamp`
```make
@out=$$($(TEST_TEXT_CLIP_MUT) 2>&1); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-text-clip-mutant FAIL: CLIP_IGNORE PASSED -- the clip oracle is decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'px outside the rule' || { printf '%s\n' "$$out" | tail -4; printf '!!! test-text-clip-mutant FAIL: RED for the wrong reason\n'; exit 1; }; \
printf '>>> test-text-clip-mutant: green (CLIP_IGNORE correctly RED: %s)\n' "$$(printf '%s\n' "$$out" | grep -m1 'checks,' | sed 's/^>>> //')"
```

## test-text-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13335):
Prerequisites: `build/test_text_mutant build/.makefile.stamp`
```make
@printf ">>> test-text-mutant: confirming FIXED_PITCH mutant goes RED (Rule 6)\n"
@if $(TEST_TEXT_MUT) >/dev/null 2>&1; then printf '!!! test-text-mutant FAIL: FIXED_PITCH PASSED -- the proportional oracle is decoration\n'; exit 1; else printf '>>> test-text-mutant: green (FIXED_PITCH correctly RED -- the oracle bites)\n'; fi
```

## test-textedit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14524):
Prerequisites: `build/test_textedit build/.makefile.stamp`
```make
@printf ">>> test-textedit: ADR-0012 D-2b / initech-77dj TextEdit (TERec) -- reduced edit record, half-open selection, CR+wrap line-breaking, cut/copy/paste via the Scrap (round-trip vs independent hand-authored golden)\n"
@$(TEST_TEXTEDIT)
@printf ">>> test-textedit: green\n"
```

## test-textedit-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14529):
Prerequisites: `build/test_textedit_mutant_selinclusive build/test_textedit_mutant_pastenodelete build/test_textedit_mutant_linebreakoff build/test_textedit_mutant_wrapchars build/.makefile.stamp`
```make
@printf ">>> test-textedit-mutant: confirming SEL_INCLUSIVE + PASTE_NO_DELETE + LINEBREAK_OFF go RED (Rule 6)\n"
@if $(TEST_TEXTEDIT_MUT_SEL) >/dev/null 2>&1; then printf '!!! test-textedit-mutant FAIL: SEL_INCLUSIVE PASSED -- the half-open-selection oracle is decoration\n'; exit 1; else printf '>>> test-textedit-mutant: green (SEL_INCLUSIVE correctly RED)\n'; fi
@if $(TEST_TEXTEDIT_MUT_PASTE) >/dev/null 2>&1; then printf '!!! test-textedit-mutant FAIL: PASTE_NO_DELETE PASSED -- the replace-range oracle is decoration\n'; exit 1; else printf '>>> test-textedit-mutant: green (PASTE_NO_DELETE correctly RED)\n'; fi
@if $(TEST_TEXTEDIT_MUT_LINE) >/dev/null 2>&1; then printf '!!! test-textedit-mutant FAIL: LINEBREAK_OFF PASSED -- the line-break oracle is decoration\n'; exit 1; else printf '>>> test-textedit-mutant: green (LINEBREAK_OFF correctly RED)\n'; fi
@out=$$($(TEST_TEXTEDIT_MUT_WRAP) 2>&1); rc=$$?; \
if [ $$rc -eq 0 ]; then printf '!!! test-textedit-mutant FAIL: WRAP_CHARS PASSED -- the pixel-wrap oracle is decoration\n'; exit 1; fi; \
printf '%s\n' "$$out" | grep -q 'step-1[012]' || { printf '%s\n' "$$out" | tail -5; printf '!!! test-textedit-mutant FAIL: WRAP_CHARS went RED, but not on the pixel-wrap steps\n'; exit 1; }; \
printf '>>> test-textedit-mutant: green (WRAP_CHARS correctly RED on %s pixel-wrap check(s))\n' "$$(printf '%s\n' "$$out" | grep -c 'step-1[012]')"
```

## test-window

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13660):
Prerequisites: `build/test_window build/.makefile.stamp`
```make
@printf ">>> test-window: visible region (strucRgn DIFF fronts) + DiffRgn damage (no over-repaint, D-5) + z-order + FindWindow\n"
@$(TEST_WINDOW)
@$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(FLAIR_WINDOW_OPT) $(WINDOW_R1_CFLAGS) $(WINDOW_INC) -c os/flair/window.c -o $(BUILD)/window_freestanding.o \
	|| { printf '!!! test-window FAIL: window.c does NOT compile freestanding (Law 3)\n'; exit 1; }
@printf ">>> test-window: green\n"
```

## test-window-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13667):
Prerequisites: `build/test_window_mutant_zorder build/test_window_mutant_overpaint build/test_window_mutant_no_deact_inval build/test_window_mutant_no_activate_inval build/test_window_mutant_shadow_clipped build/test_window_mutant_content_top_stale build/test_window_mutant_content_over_scroll build/test_window_mutant_zone_off build/test_window_mutant_zoom_no_restore build/test_window_mutant_collapse_leak build/test_window_mutant_grow_no_min build/test_window_mutant_content_over_hscroll build/.makefile.stamp`
```make
@printf ">>> test-window-mutant: confirming all twelve mutants go RED (Rule 6)\n"
@if $(TEST_WINDOW_MUT_CONTENT_OVER_HSCROLL) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: CONTENT_OVER_HSCROLL PASSED -- the contRgn-excludes-the-horizontal-bar oracle (tdnl.35) is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (CONTENT_OVER_HSCROLL correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_ZORDER) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: ZORDER PASSED -- the visible-region oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (ZORDER correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_OVERPAINT) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: OVERPAINT PASSED -- the no-over-repaint oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (OVERPAINT correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_NO_DEACT_INVAL) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: NO_DEACT_INVAL PASSED -- the deactivation-repaint oracle (initech-v6t2) is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (NO_DEACT_INVAL correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_NO_ACTIVATE_INVAL) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: NO_ACTIVATE_INVAL PASSED -- the activation-repaint oracle (initech-rqz5) is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (NO_ACTIVATE_INVAL correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_SHADOW_CLIPPED) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: CHROME_FID_MUT_SHADOW_CLIPPED PASSED -- the live shadow-region oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (CHROME_FID_MUT_SHADOW_CLIPPED correctly RED -- frame-only strucRgn loses old-shadow damage)\n'; fi
@if $(TEST_WINDOW_MUT_CONTENT_TOP_STALE) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: CHROME_FID_MUT_CONTENT_TOP_STALE PASSED -- the CalcDoc top oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (CHROME_FID_MUT_CONTENT_TOP_STALE correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_CONTENT_OVER_SCROLL) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: CHROME_FID_MUT_CONTENT_OVER_SCROLL PASSED -- the CalcDoc scrollbar exclusion oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (CHROME_FID_MUT_CONTENT_OVER_SCROLL correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_ZONE_OFF) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: ZONE_OFF PASSED -- the full title-band hit-zone oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (ZONE_OFF correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_ZOOM_NO_RESTORE) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: ZOOM_NO_RESTORE PASSED -- the userState toggle oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (ZOOM_NO_RESTORE correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_COLLAPSE_LEAK) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: COLLAPSE_LEAK PASSED -- the empty collapsed contRgn oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (COLLAPSE_LEAK correctly RED)\n'; fi
@if $(TEST_WINDOW_MUT_GROW_NO_MIN) >/dev/null 2>&1; then printf '!!! test-window-mutant FAIL: GROW_NO_MIN PASSED -- the 96x88 minimum-size oracle is decoration\n'; exit 1; else printf '>>> test-window-mutant: green (GROW_NO_MIN correctly RED)\n'; fi
```

## test-winscroll

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13701):
Prerequisites: `build/test_winscroll build/.makefile.stamp`
```make
@printf ">>> test-winscroll: window scroll bars -- geometry, thumb math, part codes, steps, FindWindow inContent (never inDrag), FindControl (tdnl.35)\n"
@$(TEST_WINSCROLL)
@printf ">>> test-winscroll: green\n"
```

## test-winscroll-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 13706):
Prerequisites: `build/test_winscroll_mutant_WINDOW_MUTATE_SCROLL_DRAG build/test_winscroll_mutant_WSCROLL_MUT_THUMB_UNSCALED build/test_winscroll_mutant_WSCROLL_MUT_PAGE_WRONG_DIR build/test_winscroll_mutant_WINDOW_MUT_SCROLL_STALE_RESIZE build/test_winscroll_mutant_WINDOW_MUTATE_GROW_NO_MIN build/.makefile.stamp`
```make
@for m in $(WINSCROLL_MUTANTS); do \
	if $(BUILD)/test_winscroll_mutant_$$m > $(BUILD)/test_winscroll_mutant_$$m.log 2>&1; then \
		printf '!!! test-winscroll-mutant FAIL: %s PASSED -- the oracle is decoration\n' "$$m"; exit 1; fi; \
	printf '>>> test-winscroll-mutant: %s correctly RED: ' "$$m"; grep -m1 'FAIL' $(BUILD)/test_winscroll_mutant_$$m.log; \
done
@grep -q 'W5 every pixel' $(BUILD)/test_winscroll_mutant_WINDOW_MUTATE_SCROLL_DRAG.log || { printf '!!! test-winscroll-mutant FAIL: SCROLL_DRAG red for the wrong reason\n'; exit 1; }
@grep -q 'W2 thumb' $(BUILD)/test_winscroll_mutant_WSCROLL_MUT_THUMB_UNSCALED.log || { printf '!!! test-winscroll-mutant FAIL: THUMB_UNSCALED red for the wrong reason\n'; exit 1; }
@grep -q 'W4 page' $(BUILD)/test_winscroll_mutant_WSCROLL_MUT_PAGE_WRONG_DIR.log || { printf '!!! test-winscroll-mutant FAIL: PAGE_WRONG_DIR red for the wrong reason\n'; exit 1; }
@grep -q 'FAIL.*W7 resize' $(BUILD)/test_winscroll_mutant_WINDOW_MUT_SCROLL_STALE_RESIZE.log || { printf 'FAIL stale resize mutant wrong reason\n'; exit 1; }
@grep -q 'FAIL.*W8 minimum' $(BUILD)/test_winscroll_mutant_WINDOW_MUTATE_GROW_NO_MIN.log || { printf 'FAIL minimum size mutant wrong reason\n'; exit 1; }
@printf ">>> test-winscroll-mutant: green (five mutants RED for the named reason)\n"
```

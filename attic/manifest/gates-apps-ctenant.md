## test-flair-ctenant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23649):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/ppm_region_count build/flair_tenants_mut_hold.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-flair-ctenant : a C application from disk; its key/click draws reach the screen and survive repaints\n'
@printf '  beads initech-w96l (the C tenant path) + initech-8zii (present after a plain mouseDown/keyDown) + initech-6qsk (retained readouts)\n'
@printf '======================================================================\n'
$(call appl-boot,flair_ctenant,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CTENANT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_ctenant.serial) || { printf '!!! test-flair-ctenant [1/4] FAIL: the desktop did not survive\n'; grep -E '^(PANIC|TENANT-|HALTED)' $(BUILD)/flair_ctenant.serial; exit 1; }
@$(call CTEN_SERIAL,$(BUILD)/flair_ctenant.serial) || { printf '!!! test-flair-ctenant [1/4] FAIL: CTENANT.EXE did not load, register, come forward and draw exactly its keyDown/mouseDown runs\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_ctenant.serial; exit 1; }
@printf '>>> test-flair-ctenant [1/4] C PATH: %s; the keyDown/mouseDown draws are the TENANT-GATE lines ctenant.c issues\n' "$$(grep -m1 '^TENANT-LOAD name=CTENANT' $(BUILD)/flair_ctenant.serial)"
@k=$$($(call CTEN_NE,$(BUILD)/flair_ctenant.ppm,194 232 234 248)); c=$$($(call CTEN_NE,$(BUILD)/flair_ctenant.ppm,194 256 234 272)); m=$$($(call CTEN_NE,$(BUILD)/flair_ctenant.ppm,194 288 234 304)); \
	[ -n "$$k" ] && [ "$$k" -ge 15 ] && [ "$$c" -ge 400 ] && [ "$$m" -ge 400 ] || { printf '!!! test-flair-ctenant [2/4] FAIL: PRESENT -- the key/click draws are not on the screen (ink: TEXTDRAW %s, cells %s, click %s; want >=15, >=400, >=400)\n' "$$k" "$$c" "$$m"; exit 1; }; \
	printf '>>> test-flair-ctenant [2/4] PRESENT: on screen after the budget -- TEXTDRAW "Key q" %s ink px, DRAWCELLS "Key q" %s px, "Click" %s px\n' "$$k" "$$c" "$$m"
$(call cten-hold-boot,flair_ctenant_drag,$(FLAIR_DATA_IMG),FLAIR_CTENANT_DRAG_SPEC)
@f=$(BUILD)/flair_ctenant_drag; w=300,180; $(call APPL_ALIVE,$$f.serial) && $(call CTEN_SERIAL,$$f.serial) && $(call CTEN_DRAG_MOVED,$$f.serial) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG -- the tenant did not run, draw its readouts and get its title dragged to (230,220)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-DRAG|PANIC)' $$f.serial; exit 1; }; \
	s=$$($(call CTEN_REPAINT_SERIAL,$$f.serial,$$w)) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG serial -- an updateEvt after the input did not repaint all three readouts (%s)\n' "$$s"; grep -E '^(TENANT-|FLAIR-DRAG)' $$f.serial; exit 1; }; \
	p=$$($(call CTEN_REPAINT_PIXELS,$$f.ppm)) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG pixels -- the readouts are not on the moved window (%s; want >=15, >=400, >=400)\n' "$$p"; exit 1; }; \
	printf '>>> test-flair-ctenant [3/4] DRAG: %s; %s; on screen at the moved window: %s\n' "$$(grep -m1 '^FLAIR-DRAG win' $$f.serial)" "$$s" "$$p"
$(call cten-hold-boot,flair_ctenant_reveal,$(FLAIR_DATA_IMG),FLAIR_CTENANT_REVEAL_SPEC)
@f=$(BUILD)/flair_ctenant_reveal; w=430,230; $(call APPL_ALIVE,$$f.serial) && $(call CTEN_SERIAL,$$f.serial) && $(call CTEN_DRAG_MOVED,$$f.serial) && $(call CTEN_APPS_OVER,$$f.serial) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL -- the tenant did not run, get dragged, and get covered by the APPS window at (40,180)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-DRAG|PANIC)' $$f.serial; exit 1; }; \
	s=$$($(call CTEN_REPAINT_SERIAL,$$f.serial,$$w)) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL serial -- an updateEvt after the input did not repaint all three readouts (%s)\n' "$$s"; grep -E '^(TENANT-|FLAIR-DRAG|FLAIR-DISPATCH)' $$f.serial; exit 1; }; \
	p=$$($(call CTEN_REPAINT_PIXELS,$$f.ppm)) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL pixels -- the readouts are not on the revealed window (%s; want >=15, >=400, >=400)\n' "$$p"; exit 1; }; \
	printf '>>> test-flair-ctenant [4/4] REVEAL: APPS dragged over, CTENANT raised; %s; on screen: %s\n' "$$s" "$$p"
@printf '>>> test-flair-ctenant: green\n'
```

## test-flair-ctenant-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 23673):
Prerequisites: `test-flair-ctenant build/qemu_harness build/flair_tenants_mut_tbx_draw_not_presented.img build/flair_data.img build/ppm_region_count build/flair_tenants_mut_hold.img build/flair_data_ctenant_noretain.img build/.makefile.stamp`
```make
$(call appl-boot,flair_ctenant_mut,$(BUILD)/flair_tenants_mut_tbx_draw_not_presented.img,$(FLAIR_DATA_IMG),FLAIR_CTENANT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
@$(call APPL_ALIVE,$(BUILD)/flair_ctenant_mut.serial) && $(call CTEN_SERIAL,$(BUILD)/flair_ctenant_mut.serial) || { printf '!!! test-flair-ctenant-mutant FAIL: DRAW_NOT_PRESENTED went RED for the wrong reason (want: the tenant ran and drew exactly as in the clean leg)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' $(BUILD)/flair_ctenant_mut.serial; exit 1; }
@m=$$($(call CTEN_NE,$(BUILD)/flair_ctenant_mut.ppm,194 288 234 304)); k=$$($(call CTEN_NE,$(BUILD)/flair_ctenant_mut.ppm,194 256 234 272)); \
	if [ "$$m" -ge 400 ]; then printf '!!! test-flair-ctenant-mutant FAIL: DRAW_NOT_PRESENTED PASSED -- the "Click" cells reached the screen anyway (%s px): the PRESENT leg is decoration\n' "$$m"; exit 1; fi; \
	printf '>>> test-flair-ctenant-mutant: DRAW_NOT_PRESENTED correctly RED -- drawn (TENANT-GATE lines present) but not presented: "Click" %s px, "Key q" cells %s px on screen\n' "$$m" "$$k"
$(call cten-hold-boot,flair_ctenant_drag_noretain,$(FLAIR_DATA_CTENANT_NORETAIN_IMG),FLAIR_CTENANT_DRAG_SPEC)
$(call cten-hold-boot,flair_ctenant_reveal_noretain,$(FLAIR_DATA_CTENANT_NORETAIN_IMG),FLAIR_CTENANT_REVEAL_SPEC)
@for leg in drag:300,180 reveal:430,230; do n=$${leg%%:*}; w=$${leg#*:}; f=$(BUILD)/flair_ctenant_$${n}_noretain; \
	$(call APPL_ALIVE,$$f.serial) && $(call CTEN_SERIAL,$$f.serial) && $(call CTEN_DRAG_MOVED,$$f.serial) || { printf '!!! test-flair-ctenant-mutant FAIL: NO_RETAIN %s went RED for the wrong reason (want: the tenant ran, drew its readouts once and was dragged)\n' $$n; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-DRAG|PANIC)' $$f.serial; exit 1; }; \
	if s=$$($(call CTEN_REPAINT_SERIAL,$$f.serial,$$w)); then printf '!!! test-flair-ctenant-mutant FAIL: NO_RETAIN %s serial PASSED (%s): the repaint leg is decoration\n' $$n "$$s"; exit 1; fi; \
	if p=$$($(call CTEN_REPAINT_PIXELS,$$f.ppm)); then printf '!!! test-flair-ctenant-mutant FAIL: NO_RETAIN %s pixels PASSED (%s): the repaint dump is decoration\n' $$n "$$p"; exit 1; fi; \
	printf '>>> test-flair-ctenant-mutant: NO_RETAIN %s correctly RED -- serial: %s; screen: %s\n' $$n "$$s" "$$p"; \
done
@printf '>>> test-flair-ctenant-mutant: green\n'
```

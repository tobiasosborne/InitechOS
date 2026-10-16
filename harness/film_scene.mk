# Film scene M024-M026; reuse Calc's disk bytes, launch host and trace runner.
include spec/flair_film_scene_traces.mk
$(BUILD)/test_film_scene: harness/proptest/test_film_scene.c harness/proptest/test_calc_tenant.c os/flair/film_scene.h $(CALC_SRC) $(CALC_DEP) os/flair/cursor.c os/flair/cursor.h os/flair/surface.c | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -Ios/flair -Ios/flair/atkinson -Wl,--gc-sections -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC)) os/flair/cursor.c os/flair/surface.c
.PHONY: test-film-scene
test-film-scene: test-calc-spec $(BUILD)/test_film_scene
	@$(BUILD)/test_film_scene
	@printf '>>> test-film-scene: green (Calc content and real-window arrangement routing)\n'
TEST_UNIT_GATES += test-film-scene

.PHONY: test-flair-live-scene
test-flair-live-scene: $(HARNESS_BIN) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/ppm_flair_calc_check
	$(call appl-boot,film_scene_busy,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),FLAIR_FILM_BUSY_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=470 y=228" --quit-after "FLAIR-CURSOR x=470 y=228")
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_scene_busy.ppm $(CALC_GOLDEN) busy
	@$(call CALC_DRAW_OK,$(BUILD)/film_scene_busy.serial) && grep -qx 'FLAIR-FILM-SCENE CALC ledger+chart' $(BUILD)/film_scene_busy.serial || { printf 'FAIL film real-tenant content trace\n'; exit 1; }
	$(call appl-boot,film_scene_drag,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),FLAIR_FILM_DRAG_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=620 y=440" --quit-after "FLAIR-CURSOR x=620 y=440")
	@grep -qx 'FLAIR-DRAG win 0 (300,120)->(360,160)' $(BUILD)/film_scene_drag.serial && grep -qx 'FLAIR-FILM-MODAL dismissed' $(BUILD)/film_scene_drag.serial || { printf 'FAIL film real-window drag trace\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_scene_drag.ppm $(CALC_GOLDEN) drag
	@printf '>>> test-flair-live-scene: green (real Calc, authored arrangement, Saving hourglass, five labelled slices after drag)\n'
TEST_EMU_GATES += test-flair-live-scene

# Rule6: same disk-tenant recipe and volume replacement as Calc's own family.
FILM_CALC_MUTANTS := CALC_MUT_NO_PIE CALC_MUT_NO_LABELS CALC_MUT_MERGE_SLICES
$(foreach m,$(filter-out $(CALC_PIXEL_MUTANT),$(FILM_CALC_MUTANTS)),$(eval $(call tenant-exe-rules,calc_$(m),$(CALC_SRC),$(CALC_INC) -D$(m),$(CALC_DEP),--gc-sections)))
$(foreach m,$(filter-out $(CALC_PIXEL_MUTANT),$(FILM_CALC_MUTANTS)),$(eval $(call calc-mut-volume,$(m))))
.PHONY: test-film-scene-mutant
test-film-scene-mutant: test-film-scene $(foreach m,$(FILM_CALC_MUTANTS),$(BUILD)/test_calc_tenant_mut_$(m)) $(BUILD)/test_film_busy_mut
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
$(BUILD)/test_film_busy_mut: harness/proptest/test_film_scene.c harness/proptest/test_calc_tenant.c os/flair/film_scene.h $(CALC_SRC) $(CALC_DEP) os/flair/cursor.c os/flair/cursor.h os/flair/surface.c | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -DCURSOR_MUT_BUSY_ARROW -Ios/flair -Ios/flair/atkinson -Wl,--gc-sections -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC)) os/flair/cursor.c os/flair/surface.c
TEST_UNIT_GATES += test-film-scene-mutant

# Reuse the existing kernel template for the cursor twin, then swap ONLY its
# CursorMgr object. Its main is the same staged scene + bounded pump.
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_FILM_SCENE,film_scene))
$(BUILD)/kmain_flairtenants_mut_film_scene.o: os/flair/film_scene.h os/flair/cursor.h
$(BUILD)/cursor_mut_busy_arrow.o: os/flair/cursor.c os/flair/cursor.h spec/assets/cursors.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -DCURSOR_MUT_BUSY_ARROW -Ios/flair -Ispec -Ispec/assets -c $< -o $@
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_CURSOR_OBJ),$(BUILD)/kmain_flairtenants_mut_film_scene.o $(BUILD)/cursor_mut_busy_arrow.o,film_busy_arrow))

define film-pixel-mutant-leg
	$(call appl-boot,film_$(1),$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_CALC_$(1)_IMG),FLAIR_FILM_DRAG_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=620 y=440" --quit-after "FLAIR-CURSOR x=620 y=440")
	@grep -qx 'FLAIR-DRAG win 0 (300,120)->(360,160)' $(BUILD)/film_$(1).serial && grep -qx 'TENANT-REGISTER ok app=CALC' $(BUILD)/film_$(1).serial || { printf 'FAIL film mutant did not launch/drag\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_$(1).ppm $(CALC_GOLDEN) drag > $(BUILD)/film_$(1).pixels 2>&1; rc=$$?; \
	[ "$$rc" = 1 ] && grep -q '$(2)' $(BUILD)/film_$(1).pixels || { cat $(BUILD)/film_$(1).pixels; printf 'FAIL film mutant survived or wrong reason\n'; exit 1; }
	@printf '>>> test-flair-live-scene-mutant: $(1) RED: %s\n' "$$(grep -m1 '$(2)' $(BUILD)/film_$(1).pixels)"
endef
.PHONY: test-flair-live-scene-mutant
test-flair-live-scene-mutant: test-flair-live-scene $(foreach m,$(FILM_CALC_MUTANTS),$(FLAIR_DATA_CALC_$(m)_IMG)) $(BUILD)/flair_tenants_mut_film_busy_arrow.img
	$(call film-pixel-mutant-leg,CALC_MUT_NO_PIE,PIE_DRAW)
	$(call film-pixel-mutant-leg,CALC_MUT_NO_LABELS,PIE_LABELS)
	$(call film-pixel-mutant-leg,CALC_MUT_MERGE_SLICES,PIE_FIVE)
	$(call appl-boot,film_busy_arrow,$(BUILD)/flair_tenants_mut_film_busy_arrow.img,$(FLAIR_DATA_IMG),FLAIR_FILM_BUSY_SPEC,--screendump --screendump-after "FLAIR-CURSOR x=470 y=228" --quit-after "FLAIR-CURSOR x=470 y=228")
	@grep -qx 'FLAIR-FILM-SCENE CALC ledger+chart' $(BUILD)/film_busy_arrow.serial || { printf 'FAIL cursor mutant did not launch Calc\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/film_busy_arrow.ppm $(CALC_GOLDEN) busy > $(BUILD)/film_busy_arrow.pixels 2>&1; rc=$$?; \
	[ "$$rc" = 1 ] && grep -q 'BUSY_HOURGLASS' $(BUILD)/film_busy_arrow.pixels || { cat $(BUILD)/film_busy_arrow.pixels; exit 1; }
	@printf '>>> test-flair-live-scene-mutant: CURSOR_MUT_BUSY_ARROW RED: %s\n' "$$(grep -m1 'BUSY_HOURGLASS' $(BUILD)/film_busy_arrow.pixels)"
	@printf '>>> test-flair-live-scene-mutant: green (four named mutants RED on scene pixels)\n'
TEST_EMU_GATES += test-flair-live-scene-mutant

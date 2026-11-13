# Typed terminal diagnostics: initech-480p / initech-dbp1. No new CPU vector.
PANIC_REASON_DEPS := os/milton/panic.h os/milton/panic_screen.h os/flair/film_startup.h
$(KERNEL_PANIC_OBJ) $(KERNEL_LOADER_OBJ): $(PANIC_REASON_DEPS)
$(BUILD)/kmain%.o: .EXTRA_PREREQS += $(PANIC_REASON_DEPS)
$(BUILD)/panic_screen_%.o $(BUILD)/loader%.o: .EXTRA_PREREQS += os/milton/panic.h
$(BUILD)/test_panic_reasons: harness/proptest/test_panic_reasons.c $(PANIC_REASON_DEPS) os/milton/fat12.h spec/toolbox_gate.h | $(BUILD)
	$(CC) $(CFLAGS) -Ios/flair -Ios/milton -Ispec $< -o $@
$(BUILD)/test_panic_reasons_mut: harness/proptest/test_panic_reasons.c $(PANIC_REASON_DEPS) os/milton/fat12.h spec/toolbox_gate.h | $(BUILD)
	$(CC) $(CFLAGS) -DPANIC_MUTATE_STARTUP_OOM -Ios/flair -Ios/milton -Ispec $< -o $@
.PHONY: test-panic-reason-map test-panic-reason-map-mutant
test-panic-reason-map: $(BUILD)/test_panic_reasons
	@$<
test-panic-reason-map-mutant: $(BUILD)/test_panic_reasons_mut
	@if $< > $(BUILD)/panic_reason_map.red 2>&1; then echo 'FAIL PANIC_MUTATE_STARTUP_OOM survived'; exit 1; fi
	@grep '^RED panic-reason-map:' $(BUILD)/panic_reason_map.red
	@echo 'GREEN test-panic-reason-map-mutant: all-OOM mapping rejected'
.PHONY: test-panic-foreign-screen test-film-startup-screen test-panic-reasons
test-panic-foreign-screen: test-panic-screen $(HARNESS_BIN) $(TRACER_IMG) $(BUILD)/ppm_panic_check
	@sh harness/emu/panic_reasons.sh $(BUILD) $(TRACER_IMG) - foreign foreign
test-film-startup-screen: test-panic-screen $(HARNESS_BIN) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) $(BUILD)/ppm_panic_check
	@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) nodata nodata
	@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) noapps noapps
	@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) nocalc nocalc
	@sh harness/emu/panic_reasons.sh $(BUILD) $(FLAIRLIVE_INTERACTIVE_IMG) $(FLAIR_DATA_IMG) badcalc badcalc
test-panic-reasons: test-panic-reason-map test-panic-foreign-screen test-film-startup-screen test-flair-live-scene
TEST_UNIT_GATES += test-panic-reason-map test-panic-reason-map-mutant
TEST_EMU_GATES += test-panic-reasons

# Same shell/film kernels; change only the physical paint or cause mapping.
$(eval $(call panic-screen-kernel,skipreason,-DPANIC_MUTATE_SKIP_REASON))
$(eval $(call panic-screen-image,skipreason,$(STAGE2_BIN),$(BUILD)/kernel_panic_screen_skipreason.bin,$(IO_SYS_BIN)))
$(eval $(call flair-tenants-kmain-mutant-rules,FLAIR_FILM_SCENE -DPANIC_MUTATE_STARTUP_OOM,film_startup_oom))
$(BUILD)/kmain_flairtenants_mut_film_startup_oom.o: $(PANIC_REASON_DEPS)

.PHONY: test-panic-reasons-mutant
test-panic-reasons-mutant: test-panic-screen $(PANIC_SCREEN_DIR)/skipreason.img $(BUILD)/flair_tenants_mut_film_startup_oom.img $(FLAIR_DATA_IMG) $(BUILD)/ppm_panic_check
	@mkdir -p $(BUILD)/panic-reasons
	@set -e; if sh harness/emu/panic_reasons.sh $(BUILD) $(PANIC_SCREEN_DIR)/skipreason.img - skipreason foreign > $(BUILD)/panic-reasons/skipreason.red 2>&1; then echo 'FAIL PANIC_MUTATE_SKIP_REASON survived'; exit 1; else rc=$$?; test $$rc -eq 1; fi
	@grep '^GREEN panic-reason-serial skipreason:' $(BUILD)/panic-reasons/skipreason.red
	@grep '^RED panic-pixels:' $(BUILD)/panic-reasons/skipreason.red
	@set -e; for kind in nodata noapps nocalc badcalc; do \
		if sh harness/emu/panic_reasons.sh $(BUILD) $(BUILD)/flair_tenants_mut_film_startup_oom.img $(FLAIR_DATA_IMG) oom_$$kind $$kind > $(BUILD)/panic-reasons/oom_$$kind.red 2>&1; then echo 'FAIL PANIC_MUTATE_STARTUP_OOM survived'; exit 1; else rc=$$?; test $$rc -eq 1; fi; \
		grep "^GREEN panic-reason-serial oom_$$kind:" $(BUILD)/panic-reasons/oom_$$kind.red; \
		grep '^RED panic-pixels:' $(BUILD)/panic-reasons/oom_$$kind.red; \
	done
	@echo 'GREEN test-panic-reasons-mutant: serial retained; skipped painter and all-OOM mapping rejected by screen pixels'
TEST_EMU_GATES += test-panic-reasons-mutant

# Immutable input for record-flair SCRIPT=film_panic. The recorder copies it.
$(eval $(call fixture-contract,$(BUILD)/panic-reasons/film-panic-data.img,$(filter-out APPS/CALC.EXE,$(FLAIR_FIXTURES))))
$(BUILD)/panic-reasons/film-panic-data.img: $(FLAIR_DATA_IMG)
	@mkdir -p $(BUILD)/panic-reasons
	cp -f $< $@
	mdel -i $@ ::APPS/CALC.EXE
	@$(call fixture-check,recipe,$@)

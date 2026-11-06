# Review regression vector; PRD 6.6, initech-0spyf/fzsvo.
$(BUILD)/test_console_review: $(DBF_DIFF_DIR)/test_console_review.c $(DBF_DIFF_DIR)/screen_file.h | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
CONSOLE_REVIEW_CASES := continuation keys color color-state controls
.PHONY: $(addprefix test-samir-console-review-,$(CONSOLE_REVIEW_CASES))
define console-review-case
test-samir-console-review-$(1): $$(PROMPT_SCREEN_DEPS) $$(SAMIR_EXE) $$(SAMIR_OVL) $$(BUILD)/test_console_review
	@sh $$(DBF_DIFF_DIR)/console_review_emu.sh $(1)
	@printf '>>> test-samir-console-review-$(1): green\n'
endef
$(foreach c,$(CONSOLE_REVIEW_CASES),$(eval $(call console-review-case,$(c))))
TEST_EMU_GATES += $(addprefix test-samir-console-review-,$(CONSOLE_REVIEW_CASES))

ANSI_BRIDGE_DEPS := os/milton/test_ansi_bridge.c os/milton/ansi_cursor.h os/milton/console.h os/milton/ansi.h os/milton/boot_info.h os/flair/surface.h
ANSI_BRIDGE_SRC := os/milton/test_ansi_bridge.c os/milton/ansi.c os/milton/console.c os/flair/surface.c
$(BUILD)/test_ansi_bridge: $(ANSI_BRIDGE_DEPS) $(ANSI_BRIDGE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -Ios/milton -Ios/flair -o $@ $(ANSI_BRIDGE_SRC)
test-ansi: test-ansi-bridge
.PHONY: test-ansi-bridge
test-ansi-bridge: $(BUILD)/test_ansi_bridge
	@$<
	@printf ">>> test-ansi-bridge: green\n"

$(BUILD)/test_ansi_bridge_mutant: $(ANSI_BRIDGE_DEPS) $(ANSI_BRIDGE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -DANSI_MUTATE_BRIDGE_CLAMP -Ios/milton -Ios/flair -o $@ $(ANSI_BRIDGE_SRC)
.PHONY: test-ansi-bridge-mutant
test-ansi-bridge-mutant: test-ansi-bridge $(BUILD)/test_ansi_bridge_mutant
	@set +e; $(BUILD)/test_ansi_bridge_mutant > $(BUILD)/ansi-bridge-mutant.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED ANSI bridge: CUD bottom row expected=24 actual=25' $(BUILD)/ansi-bridge-mutant.log || { cat $(BUILD)/ansi-bridge-mutant.log; exit 1; }
	@cat $(BUILD)/ansi-bridge-mutant.log
	@printf '>>> test-ansi-bridge-mutant: green (relative high clamp restored RED)\n'
test-ansi-mutant: test-ansi-bridge-mutant
TEST_UNIT_GATES += test-ansi-bridge test-ansi-bridge-mutant

# Same exact PRG through the hosted interpreter, with a fail-loud PAL bound.
$(BUILD)/test_samir_continuation_mutant: $(DBF_DIFF_DIR)/test_samir_say.c $(SAMIR_DIR)/core/screen.c $(INTERP_PROC_ENG) $(SAMIR_PAL_HOST_SRC) $(SAMIR_HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_REVIEW_MUTANT_CONTINUATION -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $(filter %.c,$^)

CONSOLE_REVIEW_MUTANTS := continuation:CONTINUATION keys:KEYS color:COLOR controls:REDIRECT
.PHONY: test-samir-console-review-mutant
test-samir-console-review-mutant: $(BUILD)/test_samir_continuation_mutant $(PROMPT_SCREEN_DEPS) $(BUILD)/test_console_review $(foreach m,$(CONSOLE_REVIEW_MUTANTS),$(BUILD)/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_$(lastword $(subst :, ,$(m))).EXE)
	@set +e; $(BUILD)/test_samir_continuation_mutant > $(BUILD)/console-review-host-bound.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED continuation draw bound: row=24 col=3 n=80' $(BUILD)/console-review-host-bound.log || { cat $(BUILD)/console-review-host-bound.log; exit 1; }
	@cat $(BUILD)/console-review-host-bound.log
	@set -e; for m in $(CONSOLE_REVIEW_MUTANTS); do mode=$${m%%:*}; flag=$${m#*:}; log=$(BUILD)/console-review-mut-$$mode.log; \
	 if sh $(DBF_DIFF_DIR)/console_review_emu.sh $$mode $(BUILD)/SAMIR_SCREEN_SAMIR_REVIEW_MUTANT_$$flag.EXE > $$log 2>&1; then cat $$log; printf '!!! console-review mutant %s survived\n' $$flag; exit 1; fi; \
	 case $$mode in continuation) want='RED console-review: continuation native draw bound HALT';; keys) want='RED console-review: Left pair is navigation, never text K';; color) want='RED console-review: native SAY selected attribute 1F';; controls) want='RED console-review: redirected SAY paints real Z 0D Q 0A R cells';; esac; \
	 grep -m1 -F "$$want" $$log || { cat $$log; exit 1; }; done
	@printf '>>> test-samir-console-review-mutant: green (continuation bound HALT, keys, color, redirected literals RED)\n'
TEST_EMU_GATES += test-samir-console-review-mutant

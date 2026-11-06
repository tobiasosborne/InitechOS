# W4.0 is deliberately opt-in; no Bochs, full vector or typesetting dependency.
SCREEN_MINT_DIR := harness/diff/dbf_diff/xbase_prog_diff/screen-mint
SCREEN_MINT_CASES := $(sort $(notdir $(patsubst %/,%,$(dir $(wildcard $(SCREEN_MINT_DIR)/*/KEYS)))))
SCREEN_CASE ?= $(SCREEN_MINT_DIR)/dot
SCREEN_OUT ?= $(BUILD)/screen-mint/single
$(BUILD)/screen_tool: harness/diff/dbf_diff/screen_tool.c harness/diff/dbf_diff/screen_file.h | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(BUILD)/SCREEN.COM: harness/diff/dbf_diff/screen_tsr.asm | $(BUILD)
	nasm -f bin -o $@ $<
$(BUILD)/screen_capture: harness/diff/dbf_diff/screen_capture.c harness/diff/dbf_diff/screen_file.h | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
.PHONY: mint-screen mint-screen-seeds test-screen-mint test-screen-mint-repro
mint-screen: $(BUILD)/screen_tool $(BUILD)/SCREEN.COM
	@sh harness/diff/dbf_diff/screen_mint.sh "$(SCREEN_CASE)" "$(SCREEN_OUT)"
mint-screen-seeds: $(BUILD)/screen_tool $(BUILD)/SCREEN.COM
	@set -e; for c in $(SCREEN_MINT_CASES); do \
	 sh harness/diff/dbf_diff/screen_mint.sh "$(SCREEN_MINT_DIR)/$$c" "$(BUILD)/screen-mint/seeds/$$c"; done
test-screen-mint: $(BUILD)/screen_tool $(BUILD)/SCREEN.COM
	@sh harness/diff/dbf_diff/screen_mint_test.sh
test-screen-mint-repro: $(BUILD)/screen_tool $(BUILD)/SCREEN.COM
	@sh harness/diff/dbf_diff/screen_mint_repro.sh

.PHONY: test-screen-mint-samir
test-screen-mint-samir: $(BUILD)/screen_capture $(BUILD)/screen_tool $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG)
	@sh harness/diff/dbf_diff/screen_mint_samir.sh

$(BUILD)/test_qemu_screen_helper: harness/emu/test_qemu_screen_helper.c harness/emu/qemu.c harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/emu -o $@ $<
.PHONY: test-screen-mint-mutants
test-screen-mint-mutants: $(BUILD)/test_qemu_screen_helper $(BUILD)/screen_tool $(BUILD)/SCREEN.COM $(BUILD)/screen_capture $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG)
	@$(BUILD)/test_qemu_screen_helper
	@sh harness/diff/dbf_diff/screen_mint_mutants.sh

# W3 forms (initech-p3nn0): SAMIR replayed against real III+ @...TO/@...CLEAR/CLEAR.
.PHONY: test-samir-forms-emu
test-samir-forms-emu: $(BUILD)/screen_capture $(BUILD)/screen_tool $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG) $(SAMIR_EXE)
	@sh harness/diff/dbf_diff/screen_forms_emu.sh
# Each mutant must fail on the dump that pins its rule.
FORMS_MUTANTS := glyph:at-to-a/TA cursor:at-to-b/TE clear:at-to-a/TA wrap:at-to-b/TE narrow:at-large/LA subcmd:at-clear-d/DA
.PHONY: test-samir-forms-emu-mutant
test-samir-forms-emu-mutant: $(BUILD)/screen_capture $(BUILD)/screen_tool $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG) \
		$(foreach m,$(FORMS_MUTANTS),$(BUILD)/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_$(firstword $(subst :, ,$(m))).EXE)
	@set -e; for m in $(FORMS_MUTANTS); do k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-forms-mut-$$k.log; \
	  if sh harness/diff/dbf_diff/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_FORMS_MUTANT_$$k.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
	  grep -q "^RED: $$want" $$log || { cat $$log; exit 1; }; \
	  printf 'SAMIR_FORMS_MUTANT_%s RED: ' $$k; grep -m1 "^RED: $$want" $$log; done
	@printf '>>> test-samir-forms-emu-mutant: green (glyph, cursor, clear, wrap, narrow, subcmd RED)\n'

# initech-0spyf/fzsvo: exact real III+ prompt and off-page BIOS cursor.
PROMPT_SCREEN_DEPS = $(BUILD)/screen_capture $(BUILD)/screen_tool $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG)
.PHONY: test-samir-dotprompt test-samir-cursor25 test-samir-dotprompt-mutant test-samir-cursor25-mutant
test-samir-dotprompt: $(PROMPT_SCREEN_DEPS) $(SAMIR_EXE) $(SAMIR_OVL)
	@SCREEN_FORMS_SUITE=dotprompt SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_dotprompt_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh
test-samir-cursor25: $(PROMPT_SCREEN_DEPS) $(SAMIR_EXE) $(SAMIR_OVL)
	@SCREEN_FORMS_SUITE=cursor25 SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_cursor25_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh
# Unmasked TE scrolls; edge/prompt exercises blanking while retaining col 79.
DOTPROMPT_MUTANTS := bottom:at-dot/d2 blank79:at-say-edge/prompt scoreboard:at-dot-c/e1 clear:at-dot-c/e4 enter:at-enter/entered saywrap:at-say-edge/edge saycontrol:at-say-edge-controls/controls
test-samir-dotprompt-mutant: $(PROMPT_SCREEN_DEPS) $(foreach m,$(DOTPROMPT_MUTANTS),$(BUILD)/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_$(firstword $(subst :, ,$(m))).EXE)
	@set -e; for m in $(DOTPROMPT_MUTANTS); do k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-dotprompt-mut-$$k.log; \
	  if SCREEN_FORMS_SUITE=dotprompt SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_dotprompt_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_DOTPROMPT_MUTANT_$$k.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
	  grep -q "^RED: $$want: RED: first difference" $$log || { cat $$log; exit 1; }; \
	  printf 'SAMIR_DOTPROMPT_MUTANT_%s RED: ' $$k; grep -m1 "^RED: $$want" $$log; done
	@printf '>>> test-samir-dotprompt-mutant: green (bottom, blank79, scoreboard, clear, enter, saywrap, saycontrol RED)\n'
test-samir-cursor25-mutant: $(PROMPT_SCREEN_DEPS) $(BUILD)/SAMIR_SCREEN_SAMIR_CURSOR25_MUTANT.EXE
	@log=$(BUILD)/samir-cursor25-mut.log; \
	 if SCREEN_FORMS_SUITE=cursor25 SCREEN_FORMS_GATED=$(DBF_DIFF_DIR)/screen_cursor25_gated.txt sh $(DBF_DIFF_DIR)/screen_forms_emu.sh $(BUILD)/SAMIR_SCREEN_SAMIR_CURSOR25_MUTANT.EXE > $$log 2>&1; then cat $$log; exit 1; fi; \
	 grep -q '^RED: at-to-b/TE: RED: first difference cursor-row golden=25 actual=24' $$log || { cat $$log; exit 1; }; \
	 cat $$log
	@printf '>>> test-samir-cursor25-mutant: green (row 25 clamped to 24 RED)\n'
TEST_EMU_GATES += test-samir-dotprompt test-samir-dotprompt-mutant test-samir-cursor25 test-samir-cursor25-mutant test-samir-forms-emu test-samir-forms-emu-mutant

# Real III+ STATUS layout is tracked separately (initech-ebwsw, not gated).
# Keep this out of TEST_EMU_GATES until both dumps and a mutant pass.
.PHONY: test-samir-status-screen-pending
test-samir-status-screen-pending: $(PROMPT_SCREEN_DEPS) $(SAMIR_EXE) $(SAMIR_OVL)
	@sh $(DBF_DIFF_DIR)/screen_status_pending.sh

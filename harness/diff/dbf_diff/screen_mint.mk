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

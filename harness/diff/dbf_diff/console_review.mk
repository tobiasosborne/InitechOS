# Review regression vector; PRD 6.6, initech-0spyf/fzsvo.
$(BUILD)/test_console_review: $(DBF_DIFF_DIR)/test_console_review.c $(DBF_DIFF_DIR)/screen_file.h | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
CONSOLE_REVIEW_CASES := continuation keys color controls
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

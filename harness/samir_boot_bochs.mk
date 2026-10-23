# initech-hdlb / ADR-0009 DEC-08 / PRD Sec 6.6, 8: replay the QEMU S8.2
# trace through Bochs's PS/2 keyboard, then grade its actual framebuffer
# EXACT against test-samir-boot's independent QEMU still (WL-0170).
SAMIR_BOCHS_NAME := samir_boot_bochs
SAMIR_BOCHS_SOURCE ?= $(SAMIR_LIST_IMG)
SAMIR_BOCHS_DATA := $(BUILD)/$(SAMIR_BOCHS_NAME)_data.img
SAMIR_BOCHS_REPORT := $(BUILD)/$(SAMIR_BOCHS_NAME).report
SAMIR_BOCHS_SERIAL := $(BUILD)/$(SAMIR_BOCHS_NAME).serial

# Factory-only transport check: no RFB socket or Bochs process needed.
$(BUILD)/test_samir_bochs_keys: harness/emu/test_samir_bochs_keys.c $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/emu $< -o $@
$(BUILD)/test_samir_bochs_keys_mut: harness/emu/test_samir_bochs_keys.c $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
	$(CC) $(CFLAGS) -DBOCHS_KEYS_MUT_RETURN -Iharness/emu $< -o $@
.PHONY: test-samir-bochs-keys test-samir-bochs-keys-mutant
test-samir-bochs-keys: $(BUILD)/test_samir_bochs_keys
	@$<
test-samir-bochs-keys-mutant: test-samir-bochs-keys $(BUILD)/test_samir_bochs_keys_mut
	@if $(BUILD)/test_samir_bochs_keys_mut > $(BUILD)/samir-bochs-keys-mutant.log 2>&1; then printf '!!! %s FAIL: Return mutant survived\n' "$@"; exit 1; fi
	@grep -qxF 'RED samir-bochs-keys: Return keysym must be ff0d' $(BUILD)/samir-bochs-keys-mutant.log || { cat $(BUILD)/samir-bochs-keys-mutant.log; exit 1; }
	@cat $(BUILD)/samir-bochs-keys-mutant.log
	@printf '>>> test-samir-bochs-keys-mutant: green (Return-to-BackSpace branch correctly RED)\n'
TEST_UNIT_GATES += test-samir-bochs-keys test-samir-bochs-keys-mutant

.PHONY: test-samir-boot-bochs test-samir-boot-bochs-mutant
ifeq ($(SKIP_BOCHS),1)
test-samir-boot-bochs test-samir-boot-bochs-mutant:
	@printf '!!! %s SKIPPED (SKIP_BOCHS=1 opt-out) -- the Bochs leg (Rule 5) was NOT run.\n' "$@"
else
test-samir-boot-bochs: test-samir-boot test-samir-bochs-keys $(BOCHS_BIN) $(BUILD)/ppm_bochs_check $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG) $(SAMIR_BOCHS_SOURCE)
	@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! %s FAIL: bochs not found (apt install bochs)\n' "$@"; exit 1; }
	@# The shared host has one RFB port. Poll before EVERY invocation; the
	@# driver also holds its existing host lock through reaping (WL-0170).
	@tries=0; while pgrep -x bochs >/dev/null; do \
	 if [ "$$tries" -ge 60 ]; then printf '!!! %s FAIL: Bochs busy after 30 minutes\n' "$@"; exit 1; fi; \
	 printf 'Waiting for the shared Bochs process (30s poll)\n'; sleep 30; tries=$$((tries+1)); \
	done
	@$(call fixture-preflight,$(SAMIR_BOCHS_SOURCE))
	cp -f $(SAMIR_BOCHS_SOURCE) $(SAMIR_BOCHS_DATA)
	@$(BOCHS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(SAMIR_BOCHS_DATA)" \
	 --expect SHELL-READY --quit-after SHELL-READY \
	 --keys "$(SAMIRBOOT_SCRN_KEYS)" --keys-after SHELL-READY \
	 --screendump-after SHELL-READY \
	 --name "$(SAMIR_BOCHS_NAME)" --out "$(BUILD)" --timeout-ms 120000 \
	 2> "$(SAMIR_BOCHS_REPORT)" || { cat "$(SAMIR_BOCHS_REPORT)"; exit 1; }
	@cat "$(SAMIR_BOCHS_REPORT)"
	@sh harness/emu/bochs_leg_check.sh test-samir-boot-bochs "$(SAMIR_BOCHS_REPORT)" "$(SAMIR_BOCHS_SERIAL)" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
	@rc=0; sh harness/emu/samir_boot_check.sh $@ "$(SAMIR_BOCHS_SERIAL)" || rc=1; \
	 $(BUILD)/ppm_bochs_check "$(SAMIRBOOT_PPM)" "$(BUILD)/$(SAMIR_BOCHS_NAME).snap.ppm" "$(BUILD)/$(SAMIR_BOCHS_NAME).rfb.pgm" || rc=1; \
	 exit $$rc
	@printf '>>> test-samir-boot-bochs: green (USE/LIST serial + framebuffer == QEMU golden, pixel-exact + displayed)\n'

# Rule 6: the existing pal_milton SHORT_READ branch changes only target
# file reads. The clean QEMU golden stays fixed; require BOTH wrong-data
# serial and pixel REDs, with a successful capture and no crash.
test-samir-boot-bochs-mutant: test-samir-boot-bochs $(SAMIR_LIST_MUT_IMG)
	@$(MAKE) --no-print-directory test-samir-boot-bochs SAMIR_BOCHS_SOURCE=$(SAMIR_LIST_MUT_IMG) SAMIR_BOCHS_NAME=samir_boot_bochs_mut > $(BUILD)/samir-bochs-mutant.log 2>&1; \
	 rc=$$?; test "$$rc" -ne 0 || { cat $(BUILD)/samir-bochs-mutant.log; printf '!!! %s FAIL: short-read mutant survived\n' "$@"; exit 1; }
	@grep -q '^RED test-samir-boot-bochs serial: LIST row missing:' $(BUILD)/samir-bochs-mutant.log \
	 && grep -q '^RED bochs-exact RGB ' $(BUILD)/samir-bochs-mutant.log \
	 && grep -q 'triple_fault=0' $(BUILD)/samir_boot_bochs_mut.report \
	 && grep -q 'snapshot_ok=1' $(BUILD)/samir_boot_bochs_mut.report \
	 || { cat $(BUILD)/samir-bochs-mutant.log; printf '!!! %s FAIL: missing wrong-data serial/pixel RED or failed capture\n' "$@"; exit 1; }
	@grep '^RED ' $(BUILD)/samir-bochs-mutant.log
	@printf '>>> test-samir-boot-bochs-mutant: green (short-read mutant RED on exact LIST rows AND pixels; no crash)\n'
endif

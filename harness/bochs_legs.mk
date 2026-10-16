# harness/bochs_legs.mk -- generic BOCHS boot legs for FLAIR gate kernels (Rule 5,
# PRD Sec 8; beads initech-cqej). One leg per DISTINCT gate kernel image: the
# differential is per image, not per gate (every gate that boots FLAIRTENANTS_IMG
# is covered by test-flair-appswitch-bochs; the shell-image gates by test-boot-bochs).
# Same TRUTHFUL CONSTRAINT as test-flair-appswitch-bochs: Bochs 2.7 ENOMODEs the
# 640x480 VBE mode, the desktop guard fails loud and HALTS before any tenant, so the
# leg proves the SAME kernel runs the boot chain + FLAIR heap + FAT mount identically
# and the guard fires (no triple fault). SKIP_BOCHS=1 is the one documented opt-out.
# $(call bochs-flair-leg,<gate name>,<kernel image>,<data image>)
define bochs-flair-leg
.PHONY: test-$(1)-bochs
ifeq ($$(SKIP_BOCHS),1)
test-$(1)-bochs:
	@printf '!!! test-$(1)-bochs SKIPPED (SKIP_BOCHS=1 opt-out) -- the Bochs leg (Rule 5) was NOT run. This is a LOUD, explicit opt-out, not a pass -- unset SKIP_BOCHS and re-run before trusting this gate.\n'
else
test-$(1)-bochs: $$(BOCHS_BIN) $(2) $(3)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-$(1)-bochs : BOCHS boot leg of %s\n' "$(2)"
	@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; bead initech-cqej.\n'
	@printf '======================================================================\n'
	@command -v $$(BOCHS) >/dev/null 2>&1 || { printf '!!! test-$(1)-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
	cp -f $(3) $$(BUILD)/$(1)_bochs_data.img
	@$$(BOCHS_BIN) --disk "$(2)" --disk2 "$$(BUILD)/$(1)_bochs_data.img" --expect HALTED \
		--name "$(1)_bochs" --out "$$(BUILD)" --timeout-ms 45000 \
		2> "$$(BUILD)/$(1)_bochs.report.txt" || true
	@cat "$$(BUILD)/$(1)_bochs.report.txt"
	@grep -q 'rfb_unblocked=1' "$$(BUILD)/$(1)_bochs.report.txt" \
		|| { printf '!!! test-$(1)-bochs FAIL: RFB unblock failed -- Bochs did not run the guest\n'; exit 1; }
	@if grep -q 'triple_fault=1' "$$(BUILD)/$(1)_bochs.report.txt"; then \
		printf '!!! test-$(1)-bochs FAIL: TRIPLE FAULT under Bochs\n'; exit 1; \
	fi
	@if [ ! -s "$$(BUILD)/$(1)_bochs.serial" ]; then \
		printf '!!! test-$(1)-bochs FAIL: no serial captured\n'; exit 1; \
	fi
	@for m in VBE-ENOMODE VGA13 S1 PM KERNEL CONSOLE BANNER FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK HALTED; do \
		grep -q "^$$$$m$$$$" "$$(BUILD)/$(1)_bochs.serial" \
			|| { printf '!!! test-$(1)-bochs FAIL: marker %s missing\n' "$$$$m"; exit 1; }; \
	done
	@printf '>>> test-$(1)-bochs: green (Bochs boot leg: fallback + shared milestones + the 640x480 guard, no triple fault)\n'
endif
TEST_EMU_GATES += test-$(1)-bochs
endef
$(eval $(call bochs-flair-leg,flair-menubar-clock,$(BUILD)/flair_tenants_mut_menubar_clock_gate.img,$(FLAIR_DATA_IMG)))
$(eval $(call bochs-flair-leg,flair-iword,$(BUILD)/flair_tenants_mut_iword_gate.img,$(FLAIR_DATA_IMG)))

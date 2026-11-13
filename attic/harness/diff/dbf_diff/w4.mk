# DEC-03a.10 / initech-9nzpr. Factory only; no W4 app target instantiated.
W4_DIR := harness/diff/dbf_diff
W4_SPEC := spec/samir/screen_regrade.json
W4_KINDS := frm lbl qry scr fmt config prg vue exact
W4_DIFFS := $(addprefix $(BUILD)/,$(addsuffix _diff,$(W4_KINDS)))
W4_TRACE_INPUTS := $(BUILD)/screen_capture $(BUILD)/screen_tool $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_LIST_IMG) $(SAMIR_EXE) $(BUILD)/w4_screen_check $(BUILD)/ppm_text_check

$(W4_DIFFS): $(BUILD)/%_diff: $(W4_DIR)/w4_file_diff.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -DKIND='"$*"' -o $@ $<
$(BUILD)/w4_inventory: $(W4_DIR)/w4_inventory.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(BUILD)/w4_mutate: $(W4_DIR)/w4_mutate.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(BUILD)/w4_screen_check: $(W4_DIR)/w4_screen_check.c $(W4_DIR)/screen_file.h | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
.PHONY: test-w4-inventory test-w4-inventory-mutant test-w4-files test-w4-trace-smoke test-w4-structure-mutant test-w4-retained
test-w4-inventory: $(BUILD)/w4_inventory $(W4_DIFFS)
	@$(BUILD)/w4_inventory $(W4_SPEC)
test-w4-inventory-mutant: $(BUILD)/w4_inventory $(W4_DIFFS) $(BUILD)/w4_mutate
	@sh $(W4_DIR)/w4_inventory_test.sh
test-w4-files: $(W4_DIFFS) $(BUILD)/w4_mutate
	@DBASE3_DECOMP="$(DBASE3_DECOMP)" sh $(W4_DIR)/w4_files_paths_test.sh
test-w4-retained: $(BUILD)/SCREEN.COM $(BUILD)/screen_tool $(BUILD)/w4_screen_check
	@DBASE3_DECOMP="$(DBASE3_DECOMP)" sh $(W4_DIR)/w4_retained_test.sh
test-w4-trace-smoke: $(W4_TRACE_INPUTS)
	@sh $(W4_DIR)/w4_trace.sh smoke
	@sh $(W4_DIR)/w4_trace.sh mixed-case
	@sh $(W4_DIR)/w4_trace.sh setup-prelude
test-w4-structure-mutant: test-w4-trace-smoke $(BUILD)/w4_mutate
	@sh $(W4_DIR)/w4_smoke_mutants.sh

# Future lane: $(eval $(call w4-acceptance,browse)). CHECK performs L1 and
# structural checks; each inventory mutant is a failing-behavior proof script.
# Required retained dumps are compared independently after CHECK. L3 is an
# actual record-flair + repro run, not an existence-only clip assertion.
define w4-acceptance
.PHONY: test-w4-$(1)
test-w4-$(1): test-w4-inventory $(W4_TRACE_INPUTS)
	@$(BUILD)/w4_inventory $(W4_SPEC) --ready $(1)
	@sh $(W4_DIR)/w4_accept.sh $(1)
	+$$(MAKE) --no-print-directory record-flair SCRIPT=$(1)
	+$$(MAKE) --no-print-directory record-flair-repro SCRIPT=$(1)
	@test -s $(BUILD)/clips/$(1).gif && test -s $(BUILD)/clips/$(1).mp4
	@printf 'GREEN: test-w4-$(1) L1/L2/L3 and retained exact screens\n'
endef

# Inventory/tool infrastructure is distinct from future per-app acceptance.
TEST_UNIT_GATES += test-w4-inventory test-w4-inventory-mutant test-w4-files test-w4-retained
TEST_EMU_GATES += test-w4-trace-smoke test-w4-structure-mutant

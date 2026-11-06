# Merge-revert gates (initech-ror2g, F10 of epic initech-0uphq).
#
# tools/revert-merge.sh is the PM's one-command recovery for a bad lane merge
# (policy: docs/process/MERGE-REVERT.md). These host-only gates build a
# throwaway git repository (local bare origin, three no-ff merges, a fake green
# and a fake red gate) under $(BUILD) and prove every refusal, the newest-first
# two-step revert, active first-parent revert parity, literal path selection,
# conflict/abort diagnostics, the smoke gate and the push gate.
#
# Gate and mutant use separate scratch dirs, so they are parallel-safe.
# Include AFTER every TEST_*_GATES := assignment (next to gate_pairs.mk).
REVERT_MERGE_TOOL := tools/revert-merge.sh
REVERT_MERGE_TEST := harness/test_revert_merge.sh
REVERT_MERGE_MUTANT := harness/test_revert_merge_mutant.sh

.PHONY: test-revert-merge test-revert-merge-mutant
test-revert-merge: $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) | $(BUILD)
	@sh $(REVERT_MERGE_TEST) $(REVERT_MERGE_TOOL) $(BUILD)/revert-merge

# All three mutants are sed-patched COPIES under $(BUILD); the original is
# never edited. The oracle must go RED on the case that guards the mutated line
# (and only a RED naming that case counts; any other failure is a broken mutant).
test-revert-merge-mutant: $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) $(REVERT_MERGE_MUTANT) | $(BUILD)
	@sh $(REVERT_MERGE_MUTANT) $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) $(BUILD)/revert-merge-mut

TEST_UNIT_GATES += test-revert-merge test-revert-merge-mutant

# test-merge-samir: the per-merge SAMIR gate set (initech-2kkqr, F5; plan
# docs/plans/INITECHBASE-ship-plan.md sec 1/5). Operator ruling 2026-10-25:
# typesetting runs once per session, never per merge -- so NO test-pubs-*
# LaTeX gate is ever in this set. Contents:
#   every TEST_UNIT_GATES / TEST_EMU_GATES entry named test-samir,
#   test-samir-*, test-flair-samir-* or samir-*-goldens
#   with their -mutant / -emu / -emu-mutant legs), plus the source-only
#   `test-pubs-lockstep-src` IF the Makefile defines it (another lane adds it).
# The list is built at RECIPE time (after the whole Makefile is parsed), so
# the target's position in the include order does not matter. Run it as
#   make -j32 test-merge-samir   (Bochs legs are per-session, not per-merge)
# The legs use private boot overlays or private --boot-rw copies, per-case
# scratch dirs, no .NOTPARALLEL); -k reports every red gate, not just the first.
MERGE_SAMIR_CANDIDATES = $(sort $(filter test-samir test-samir-% test-flair-samir-% samir-%-goldens,$(TEST_UNIT_GATES) $(TEST_EMU_GATES)))
MERGE_SAMIR_OMISSIONS = $(filter %-bochs %-bochs-mutant,$(MERGE_SAMIR_CANDIDATES))
MERGE_SAMIR_GATES = $(filter-out $(MERGE_SAMIR_OMISSIONS),$(MERGE_SAMIR_CANDIDATES))
.PHONY: test-merge-samir
test-merge-samir:
	@gates="$(MERGE_SAMIR_GATES)"; \
	if $(MAKE) -n test-pubs-lockstep-src >/dev/null 2>&1; then gates="$$gates test-pubs-lockstep-src"; lock=yes; else lock=absent; fi; \
	printf '>>> test-merge-samir: %s gates (test-pubs-lockstep-src: %s; no LaTeX)\n' "$$(echo $$gates | wc -w)" "$$lock"; \
	printf '>>> declared omissions (Bochs per-session): %s\n' '$(MERGE_SAMIR_OMISSIONS)'; \
	$(MAKE) -k $$gates && printf '>>> test-merge-samir: ALL GREEN\n'

# test-par (initech-2kkqr, F7): the same gate vector as `make test`
# (TEST_UNIT_GATES + TEST_EMU_GATES) as ONE parallel make, so -jN actually
# parallelises. `make test` itself drives the gates from serial shell loops
# (test-unit / test-emu) and ignores -j. -k: report every red gate.
#   make -j32 test-par
.PHONY: test-par
test-par:
	@gates="$(sort $(TEST_UNIT_GATES) $(TEST_EMU_GATES))"; \
	printf '>>> test-par: %s gates (unit+emu), one parallel make\n' "$$(echo $$gates | wc -w)"; \
	$(MAKE) -k $$gates && printf '>>> test-par: ALL GREEN\n'

# Header edits must rebuild every accepted and mutant binary using the lock.
$(TEST_SAMIR_VIEW) $(TEST_SAMIR_USE) $(SAMIR_VIEW_MUTS) $(SAMIR_USE_MUTS) \
$(SAMIR_LIST_LAYOUT_MUTS) $(SAMIR_UNDEFINED_MUT) $(TEST_SAMIR_REPORT) \
$(SAMIR_REPORT_MUTS) $(TEST_SAMIR_AUDIT_EMU) $(TEST_SAMIR_REPORT_EMU) \
$(TEST_SAMIR_DBSAFE_EMU) $(TEST_SAMIR_LIST_LAYOUT_EMU) $(BUILD)/test_samir_core_emu: $(DBF_DIFF_DIR)/scratch_lock.h
$(TEST_SAMIR_AUDIT_EMU) $(TEST_SAMIR_REPORT_EMU) $(TEST_SAMIR_DBSAFE_EMU) \
$(TEST_SAMIR_LIST_LAYOUT_EMU) $(BUILD)/test_samir_core_emu $(BUILD)/test_samir_screen_emu \
$(SAMIR_DML_TEST) $(SAMIR_DML_MUTS) $(SAMIR_FNS_TEST) $(addprefix $(BUILD)/test_samir_fns_,$(SAMIR_FNS_CASES)): $(DBF_DIFF_DIR)/trace_complete.h

.PHONY: test-merge-train test-samir-scratch-lock
test-merge-train:
	+@sh tools/test-merge-train.sh
$(BUILD)/test_samir_scratch_lock: $(DBF_DIFF_DIR)/test_scratch_lock.c $(DBF_DIFF_DIR)/scratch_lock.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -o $@ $<
test-samir-scratch-lock: $(BUILD)/test_samir_scratch_lock
	@sh $(DBF_DIFF_DIR)/test_scratch_lock.sh $<
TEST_UNIT_GATES += test-samir-scratch-lock test-merge-train

$(BUILD)/snapshot_write.bin: harness/emu/snapshot_write.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/snapshot_debug_exit.bin: harness/emu/snapshot_write.asm | $(BUILD)
	$(NASM) -f bin -DEXIT_IMMEDIATE $< -o $@
.PHONY: test-qemu-snapshot
test-qemu-snapshot: $(HARNESS_BIN) $(BUILD)/snapshot_write.bin $(BUILD)/snapshot_debug_exit.bin
	@sh harness/emu/test_snapshot.sh $(HARNESS_BIN) $(BUILD)/snapshot_write.bin $(BUILD)/snapshot_debug_exit.bin
TEST_EMU_GATES += test-qemu-snapshot

.PHONY: test-samir-trace
test-samir-trace: $(SAMIR_LIST_MUT_IMG)
	+@sh tools/test-emu-trace.sh
TEST_UNIT_GATES += test-samir-trace

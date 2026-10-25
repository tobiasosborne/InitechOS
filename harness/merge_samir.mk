# test-merge-samir: the per-merge SAMIR gate set (initech-2kkqr, F5; plan
# docs/plans/INITECHBASE-ship-plan.md sec 1/5). Operator ruling 2026-10-25:
# typesetting runs once per session, never per merge -- so NO test-pubs-*
# LaTeX gate is ever in this set. Contents:
#   every TEST_UNIT_GATES / TEST_EMU_GATES entry named test-samir-* or
#   samir-prog-goldens (test-samir, -fns-*, -dml-*, -size, -set-color, ...
#   with their -mutant / -emu / -emu-mutant legs), plus the source-only
#   `test-pubs-lockstep-src` IF the Makefile defines it (another lane adds it).
# The list is built at RECIPE time (after the whole Makefile is parsed), so
# the target's position in the include order does not matter. Run it as
#   make -j32 test-merge-samir   (Bochs legs are per-session, not per-merge)
# The legs are parallel-safe (read-only --snapshot boot image, per-case
# scratch dirs, no .NOTPARALLEL); -k reports every red gate, not just the first.
MERGE_SAMIR_GATES = $(sort $(filter-out %-bochs %-bochs-mutant,$(filter test-samir-% samir-prog-goldens,$(TEST_UNIT_GATES) $(TEST_EMU_GATES))))
.PHONY: test-merge-samir
test-merge-samir:
	@gates="$(MERGE_SAMIR_GATES)"; \
	if $(MAKE) -n test-pubs-lockstep-src >/dev/null 2>&1; then gates="$$gates test-pubs-lockstep-src"; lock=yes; else lock=absent; fi; \
	printf '>>> test-merge-samir: %s gates (test-pubs-lockstep-src: %s; no LaTeX)\n' "$$(echo $$gates | wc -w)" "$$lock"; \
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

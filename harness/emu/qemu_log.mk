# initech-tfn6r; PRD Sec 8 / CLAUDE.md Rules 2,6: bound diagnostic storage,
# never the serial/fault oracle. See harness/README.md for measured headroom.
QEMU_LOG_CAP_BYTES ?= 67108864
export INITECH_QEMU_LOG_CAP_BYTES = $(QEMU_LOG_CAP_BYTES)

# Historical GREEN forms logs reached 350 MB; DML logs reached 1.3 GB.
# Private scope keeps shared prerequisites and other families at 64 MiB.
test-samir-forms-emu test-samir-forms-emu-mutant: private QEMU_LOG_CAP_BYTES = 536870912
test-samir-dml-%-emu: private QEMU_LOG_CAP_BYTES = 2147483648
test-samir-dml-%-emu-mutant: private QEMU_LOG_CAP_BYTES = 2147483648
test-samir-abbrev-emu test-samir-print-emu-mutant: private QEMU_LOG_CAP_BYTES = 2147483648

$(BUILD)/qemu_harness_mut_keep_log: $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC) harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) -DHARNESS_MUTATE_KEEP_GREEN_LOG -Iharness/emu -o $@ $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC)
$(BUILD)/qemu_harness_mut_no_log_cap: $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC) harness/emu/qemu.h | $(BUILD)
	$(CC) $(CFLAGS) -DHARNESS_MUTATE_NO_QEMU_LOG_CAP -Iharness/emu -o $@ $(HARNESS_DRV_SRC) $(HARNESS_LIB_SRC)

.PHONY: test-harness-qemu-log test-harness-qemu-log-mutant
test-harness-qemu-log: $(HARNESS_BIN) $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF)
	@sh harness/emu/test_qemu_log.sh $(HARNESS_BIN) $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF) $(BUILD)
test-harness-qemu-log-mutant: $(BUILD)/qemu_harness_mut_keep_log $(BUILD)/qemu_harness_mut_no_log_cap $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF)
	@set -eu; for m in keep_log no_log_cap; do \
	 log=$(BUILD)/qemu-log-mut-$$m.log; \
	 if sh harness/emu/test_qemu_log.sh $(BUILD)/qemu_harness_mut_$$m $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF) $(BUILD) > $$log 2>&1; then \
	   cat $$log; printf '!!! test-harness-qemu-log-mutant FAIL: %s survived\n' $$m; exit 1; \
	 fi; \
	 case $$m in keep_log) why=GREEN-RETAINED ;; no_log_cap) why=CAP-MISSING ;; esac; \
	 grep "^RED qemu-log: $$why" $$log || { cat $$log; exit 1; }; \
	 done
	@printf '>>> test-harness-qemu-log-mutant: green (retention and cap mutants both RED)\n'
TEST_EMU_GATES += test-harness-qemu-log test-harness-qemu-log-mutant

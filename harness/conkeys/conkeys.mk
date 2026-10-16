# initech-1p1g: tests first, shared AH=0Ah editor; m0dc injected-shell pattern.
CONKEYS_HOST := $(BUILD)/test_conkeys
CONKEYS_PROG := $(BUILD)/conkeys_program.bin
CONKEYS_MUTANTS := F1 F2 F3 F4 F5 ESC INS DEL PAUSE PRN
$(CONKEYS_HOST): harness/conkeys/test_conkeys.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -Ios/milton -Iseed -Ibuild -o $@ $< $(TEST_CONIN_DEPS)
$(CONKEYS_PROG): os/milton/conkeys_program.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
define conkeys-mutant
$(BUILD)/test_conkeys_$(1): harness/conkeys/test_conkeys.c $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -DCONKEYS_MUT_$(1) -Ispec -Ios/milton -Iseed -Ibuild -o $$@ $$< $$(TEST_CONIN_DEPS)
endef
$(foreach m,$(CONKEYS_MUTANTS),$(eval $(call conkeys-mutant,$(m))))
.PHONY: test-conkeys-unit test-conkeys-mutant test-conkeys test-conkeys-emu-mutant
test-conkeys-unit: $(CONKEYS_HOST)
	@$(CONKEYS_HOST)
test-conkeys-mutant: $(foreach m,$(CONKEYS_MUTANTS),$(BUILD)/test_conkeys_$(m))
	@set -eu; for m in $(CONKEYS_MUTANTS); do \
	 case $$m in F1) why='F1';; F2) why='F2 excludes target';; F3) why='F3 rest';; \
	 F4) why='F4 excludes target';; F5) why='F5 replaces template';; ESC) why='Esc retains template';; \
	 INS) why='Ins toggles';; DEL) why='Del skips template';; PAUSE) why='Ctrl-S consumes resume';; \
	 PRN) why='Ctrl-P echoes PRN; Ctrl-N ends echo';; esac; \
	 log=$(BUILD)/conkeys-$$m.log; \
	 if $(BUILD)/test_conkeys_$$m >$$log 2>&1; then echo "FAIL CONKEYS $$m survived"; exit 1; fi; \
	 grep -Fx "RED CONKEYS $$why" $$log; printf 'GREEN test-conkeys-mutant: %s rejected\n' $$m; done
test-conkeys: $(CONKEYS_PROG) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/conkeys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(CONKEYS_PROG)
# Inject a kernel mutant, not a mutant test client. The saved lines must differ.
$(BUILD)/int21_conkeys_f3.o: $(KERNEL_INT21_C) $(TEST_CONIN_HDRS)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -DCONKEYS_MUT_F3 -Ispec -Ios/milton -Ibuild -c $< -o $@
$(BUILD)/conkeys_mut.elf: $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_conkeys_f3.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/conkeys_mut.bin: $(BUILD)/conkeys_mut.elf
	$(OBJCOPY) -O binary $< $@
	@set -e; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	 dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$((max - sz))" conv=notrunc status=none
$(BUILD)/conkeys_mut.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/conkeys_mut.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/conkeys_mut.bin $(BOOT_SYSTEM_TOOL)
test-conkeys-emu-mutant: $(CONKEYS_PROG) $(HARNESS_BIN) $(BUILD)/conkeys_mut.img
	@if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/conkeys.sh $(BUILD) $(BUILD)/conkeys_mut.img $(CONKEYS_PROG) >$(BUILD)/conkeys-emu-mutant.log 2>&1; then \
	 echo "FAIL CONKEYS emulator F3 mutant survived"; exit 1; fi
	@grep "RED CONKEYS template-lines" $(BUILD)/conkeys-emu-mutant.log
	@printf "GREEN test-conkeys-emu-mutant: F3 kernel mutant rejected\n"
TEST_UNIT_GATES += test-conkeys-unit test-conkeys-mutant
TEST_EMU_GATES += test-conkeys test-conkeys-emu-mutant

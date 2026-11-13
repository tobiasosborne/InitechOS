# initech-iw0h: MS-DOS 3.3 User's Guide pp.154-155,157. The EDLIN
# work supplied the editor; these gates lock COMMAND.COM's retained template.
F3_TEMPLATE_MUT := $(BUILD)/test_conin_mutant_template
$(F3_TEMPLATE_MUT): $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS) $(TEST_CONIN_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DCMD_MUT_NO_TEMPLATE -Ispec -I$(MILTON_DIR) -Iseed -Ibuild \
		-o $@ $(TEST_CONIN_SRC) $(TEST_CONIN_DEPS)

.PHONY: test-f3template-unit test-f3template-unit-mutant test-f3template test-f3template-mutant
test-f3template-unit: test-kbd-unit test-conin-unit
	@printf '>>> test-f3template-unit: green (00,3D ring + AH=0Ah full/partial template recall)\n'
test-f3template-unit-mutant: $(TEST_KBD_MUT_EXT) $(F3_TEMPLATE_MUT)
	@if $(TEST_KBD_MUT_EXT) >$(BUILD)/f3template-kbd-mutant.log 2>&1; then \
		printf '!!! test-f3template-unit-mutant FAIL: KBD_MUT_DROP_EXTENDED survived\n'; exit 1; fi
	@grep -q 'FAIL.*F3 transport:' $(BUILD)/f3template-kbd-mutant.log
	@printf '>>> test-f3template-unit-mutant: KBD_MUT_DROP_EXTENDED RED (F3 transport lost)\n'
	@if $(F3_TEMPLATE_MUT) >$(BUILD)/f3template-cmd-mutant.log 2>&1; then \
		printf '!!! test-f3template-unit-mutant FAIL: CMD_MUT_NO_TEMPLATE survived\n'; exit 1; fi
	@grep -q 'FAIL.*F3 template: returned command' $(BUILD)/f3template-cmd-mutant.log
	@printf '>>> test-f3template-unit-mutant: CMD_MUT_NO_TEMPLATE RED (previous command lost)\n'

# Mutation swaps ONLY the kernel's AH=0Ah implementation, never the oracle.
$(BUILD)/int21_f3template_mut.o: $(KERNEL_INT21_C) $(TEST_CONIN_HDRS)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -DCMD_MUT_NO_TEMPLATE \
		-Ispec -I$(MILTON_DIR) -Ibuild -c $< -o $@
$(BUILD)/f3template_mut.elf: $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_f3template_mut.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/f3template_mut.bin: $(BUILD)/f3template_mut.elf
	$(OBJCOPY) -O binary $< $@
	@set -e; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	 dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$((max - sz))" conv=notrunc status=none
$(BUILD)/f3template_mut.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/f3template_mut.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/f3template_mut.bin $(BOOT_SYSTEM_TOOL)

test-f3template: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(FAT_EXEC_IMG)
	@INITECH_QMP_STDIO=1 sh harness/conkeys/f3template.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FAT_EXEC_IMG) f3template
test-f3template-mutant: $(HARNESS_BIN) $(BUILD)/f3template_mut.img $(FAT_EXEC_IMG)
	@if INITECH_QMP_STDIO=1 sh harness/conkeys/f3template.sh $(BUILD) $(BUILD)/f3template_mut.img $(FAT_EXEC_IMG) f3template_mut \
		>$(BUILD)/f3template-emu-mutant.log 2>&1; then \
		printf '!!! test-f3template-mutant FAIL: CMD_MUT_NO_TEMPLATE survived QEMU\n'; exit 1; fi
	@grep -Fx 'RED F3TEMPLATE repeat-count: expected 2, got 1' $(BUILD)/f3template-emu-mutant.log
	@printf '>>> test-f3template-mutant: CMD_MUT_NO_TEMPLATE RED (QEMU REPEAT only once)\n'

TEST_UNIT_GATES += test-f3template-unit test-f3template-unit-mutant
TEST_EMU_GATES += test-f3template test-f3template-mutant

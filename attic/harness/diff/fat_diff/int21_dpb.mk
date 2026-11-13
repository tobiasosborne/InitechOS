# WL-0167 / initech-dpb1: INT 21h AH=1Fh + AH=32h (DOS 3.x Drive Parameter
# Block) graded against minfo/mdir of the same images. Never run full test.
# Ref: docs/research/dpb-ground-truth.md; ADR-0003 Appendix A (0Fh-24h, 32h),
# Appendix B.4; spec/int21h_calling_convention.json 1Fh/32h.
INT21_DPB_SH := harness/diff/fat_diff/int21_dpb.sh
$(BUILD)/int21_dpb_host: harness/diff/fat_diff/int21_dpb_host.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(DOS_VOLUMES_FLAGS) $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
$(BUILD)/int21_dpb_host_first_data: harness/diff/fat_diff/int21_dpb_host.c $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(DOS_VOLUMES_FLAGS) -DINT21_MUTATE_DPB_FIRST_DATA $< $(TEST_FILEIO_SUBDIR_DEPS) -o $@
$(BUILD)/int21_dpb_program.bin: harness/diff/fat_diff/int21_dpb_program.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/int21_dpb_first_data.o: $(KERNEL_INT21_C) $(TEST_DEVWIRE_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -DINT21_MUTATE_DPB_FIRST_DATA -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
$(eval $(call j1lu-emu-variant,dpb_first_data,$(KERNEL_INT21_OBJ),$(BUILD)/int21_dpb_first_data.o))
.PHONY: test-int21-dpb test-int21-dpb-mutant test-int21-dpb-emu test-int21-dpb-emu-mutant
test-int21-dpb: $(BUILD)/int21_dpb_host $(SECONDARY_TRACER_IMG)
	@sh $(INT21_DPB_SH) host $(BUILD)/int21_dpb_host $(SECONDARY_TRACER_IMG)
test-int21-dpb-mutant: test-int21-dpb $(BUILD)/int21_dpb_host_first_data $(SECONDARY_TRACER_IMG)
	@mkdir -p $(BUILD)/int21-dpb
	@if sh $(INT21_DPB_SH) host $(BUILD)/int21_dpb_host_first_data $(SECONDARY_TRACER_IMG) first-data > $(BUILD)/int21-dpb/first-data.log 2>&1; then echo 'FAIL int21-dpb FIRST_DATA survived'; exit 1; fi
	@grep '^RED int21-dpb A first data sector 0Bh' $(BUILD)/int21-dpb/first-data.log | head -1
	@echo 'GREEN test-int21-dpb-mutant: FIRST_DATA rejected'
test-int21-dpb-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/int21_dpb_program.bin
	@sh $(INT21_DPB_SH) emu $(SECONDARY_TRACER_IMG)
test-int21-dpb-emu-mutant: test-int21-dpb-emu $(HARNESS_BIN) $(BUILD)/j1lu_dpb_first_data.img $(BUILD)/int21_dpb_program.bin
	@mkdir -p $(BUILD)/int21-dpb
	@if sh $(INT21_DPB_SH) emu $(BUILD)/j1lu_dpb_first_data.img first-data > $(BUILD)/int21-dpb/first-data-emu.log 2>&1; then echo 'FAIL int21-dpb FIRST_DATA survived QEMU'; exit 1; fi
	@grep '^RED int21-dpb A first data sector 0Bh' $(BUILD)/int21-dpb/first-data-emu.log | head -1
	@echo 'GREEN test-int21-dpb-emu-mutant: FIRST_DATA rejected in QEMU'
TEST_UNIT_GATES += test-int21-dpb test-int21-dpb-mutant
TEST_EMU_GATES += test-int21-dpb-emu test-int21-dpb-emu-mutant

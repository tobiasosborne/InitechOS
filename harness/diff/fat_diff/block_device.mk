# DEC-17: host + QEMU gates and real implementation mutants, before service.
BLOCK_DEVICE_SRC := os/milton/block_device.c
BLOCK_DEVICE_TEST := harness/diff/fat_diff/test_block_device.c
BLOCK_DEVICE_FLAGS := -std=c11 -O2 -Wall -Wextra -Werror -no-pie -DBLOCK_DEVICE_HOST -Ios/milton -Ispec
$(BUILD)/gen_block_device_spec: tools/gen_block_device_spec.c | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
.PHONY: test-block-device-spec test-block-device-spec-mutant
test-block-device-spec: $(BUILD)/gen_block_device_spec spec/block_device_contract.json spec/block_device_contract.h spec/mbr_partition_contract.json
	@$(BUILD)/gen_block_device_spec spec/block_device_contract.json spec/mbr_partition_contract.json > $(BUILD)/block_device_contract.h
	@cmp spec/block_device_contract.h $(BUILD)/block_device_contract.h
	@echo 'GREEN test-block-device-spec: JSON/header equal'
test-block-device-spec-mutant: $(BUILD)/gen_block_device_spec spec/block_device_contract.json
	@sed 's/"heads": 16/"heads": 15/' spec/block_device_contract.json > $(BUILD)/block_device_mut.json
	@$(BUILD)/gen_block_device_spec $(BUILD)/block_device_mut.json > $(BUILD)/block_device_mut.h
	@if cmp -s spec/block_device_contract.h $(BUILD)/block_device_mut.h; then echo 'FAIL spec mutant survived'; exit 1; fi
	@echo 'RED block-device SPEC heads drift'
	@echo 'GREEN test-block-device-spec-mutant: JSON heads drift rejected'
test-spec: test-block-device-spec
$(BUILD)/test_block_device: $(BLOCK_DEVICE_TEST) $(BLOCK_DEVICE_SRC) os/milton/block_device.h spec/block_device_contract.h | $(BUILD)
	$(CC) $(BLOCK_DEVICE_FLAGS) $(BLOCK_DEVICE_TEST) $(BLOCK_DEVICE_SRC) -o $@
$(BUILD)/block_device_probe.bin: os/milton/block_device_probe.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(KERNEL_BLOCK_DEVICE_OBJ): $(BLOCK_DEVICE_SRC) os/milton/block_device.h spec/block_device_contract.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_BLOCK_DEVICE_OPT) -Ispec -Ios/milton -c $< -o $@
define block-device-mutant
$(BUILD)/test_block_device_$(1): $(BLOCK_DEVICE_TEST) $(BLOCK_DEVICE_SRC) spec/block_device_contract.h | $(BUILD)
	$$(CC) $$(BLOCK_DEVICE_FLAGS) -DBLOCK_DEVICE_MUT_$(1) $$(BLOCK_DEVICE_TEST) $$(BLOCK_DEVICE_SRC) -o $$@
endef
$(foreach m,HEADS CARRY,$(eval $(call block-device-mutant,$(m))))
.PHONY: test-block-device test-block-device-mutant test-block-device-emu test-block-device-emu-mutant
test-block-device: $(BUILD)/test_block_device
	@$<
test-block-device-mutant: $(BUILD)/test_block_device_HEADS $(BUILD)/test_block_device_CARRY
	@set -eu; for m in HEADS CARRY; do \
	 log=$(BUILD)/block-device-$$m.log; \
	 if $(BUILD)/test_block_device_$$m > $$log 2>&1; then echo "FAIL block-device $$m survived"; exit 1; fi; \
	 grep '^RED block-device ' $$log; done
	@echo 'GREEN test-block-device-mutant: HEADS and CARRY rejected'
test-block-device-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/test_partition $(BUILD)/block_device_probe.bin
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/block_device.sh $(BUILD) $(SECONDARY_TRACER_IMG)
$(BUILD)/block_device_relative.o: $(BLOCK_DEVICE_SRC) spec/block_device_contract.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_BLOCK_DEVICE_OPT) -DBLOCK_DEVICE_MUT_RELATIVE -Ispec -Ios/milton -c $< -o $@
$(BUILD)/kernel_block_device_relative.elf: $(filter-out $(KERNEL_BLOCK_DEVICE_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/block_device_relative.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_block_device_relative.bin: $(BUILD)/kernel_block_device_relative.elf
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,block-device-relative)
$(BUILD)/block_device_relative_boot.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_block_device_relative.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_block_device_relative.bin $(BOOT_SYSTEM_TOOL)
test-block-device-emu-mutant: $(HARNESS_BIN) $(BUILD)/block_device_relative_boot.img $(BUILD)/test_partition $(BUILD)/block_device_probe.bin
	@mkdir -p $(BUILD)/block-device
	@if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/block_device.sh $(BUILD) $(BUILD)/block_device_relative_boot.img > $(BUILD)/block-device/relative.log 2>&1; then echo 'FAIL block-device relative mutant survived'; exit 1; fi
	@grep '^RED block-device write device LBA 32' $(BUILD)/block-device/relative.log
	@echo 'GREEN test-block-device-emu-mutant: volume-relative write rejected'
TEST_UNIT_GATES += test-block-device-spec test-block-device-spec-mutant test-block-device test-block-device-mutant
TEST_EMU_GATES += test-block-device-emu test-block-device-emu-mutant

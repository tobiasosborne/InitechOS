# initech-kzfs. Oracles authored before the mount implementation.
# Ref: ADR-0003 DEC-07a/DEC-15; IBM DOS 3.30 TR pp2-31,6-24..25,9-3..9.
PARTITION_DIR := harness/diff/fat_diff
PARTITION_FLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -I$(PARTITION_DIR)
PARTITION_SRC := $(PARTITION_DIR)/test_partition.c os/milton/fat12.c $(PARTITION_DIR)/blockdev_file.c
PARTITION_MUTANTS := IGNORE_BASE EXTENDED SIGNATURE
PARTITIONED_DATA_IMG := $(BUILD)/kzfs/partitioned.img

$(BUILD)/test_partition: $(PARTITION_SRC) os/milton/fat12.h | $(BUILD)
	$(CC) $(PARTITION_FLAGS) $(PARTITION_SRC) -o $@
$(BUILD)/partition_probe.bin: os/milton/partition_probe.asm | $(BUILD)
	$(NASM) -f bin $< -o $@

define partition-mutant
$(BUILD)/test_partition_$(1): $(PARTITION_SRC) os/milton/fat12.h | $(BUILD)
	$$(CC) $$(PARTITION_FLAGS) -DFAT_PART_MUT_$(1) $$(PARTITION_SRC) -o $$@
$(BUILD)/fat12_partition_$(1).o: os/milton/fat12.c os/milton/fat12.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) $$(KERNEL_SECTION_FLAGS) -Ispec -Ios/milton -DFAT_PART_MUT_$(1) -c $$< -o $$@
$(BUILD)/kernel_partition_$(1).elf: $(filter-out $(KERNEL_FAT12_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fat12_partition_$(1).o $(KERNEL_LD)
	$$(KERNEL_LINK) -o $$@ $$(filter %.o,$$^)
$(BUILD)/kernel_partition_$(1).bin: $(BUILD)/kernel_partition_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
	@set -eu; sz=$$$$(wc -c < $$@); max=$$$$(( $$(KERNEL_SECTORS) * 512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek=$$$$sz count=$$$$((max-sz)) conv=notrunc status=none
$(BUILD)/partition_$(1)_boot.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_partition_$(1).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $$(MBR_BIN) $$(STAGE2_BIN) $$(IO_SYS_BIN) $(BUILD)/kernel_partition_$(1).bin $$(BOOT_SYSTEM_TOOL)
endef
$(foreach m,$(PARTITION_MUTANTS),$(eval $(call partition-mutant,$(m))))

.PHONY: partitioned-data test-fat-partition test-fat-partition-mutant test-fat-partition-emu test-fat-partition-emu-mutant
$(PARTITIONED_DATA_IMG): $(BUILD)/test_partition $(PARTITION_DIR)/partition.sh
	@sh $(PARTITION_DIR)/partition.sh mint $(BUILD)
partitioned-data: $(PARTITIONED_DATA_IMG)
	@echo 'GREEN partitioned-data: MBR + FAT12 at LBA 63'

test-fat-partition: $(FAT_DUMP_BIN) $(BUILD)/test_partition
	@sh $(PARTITION_DIR)/partition.sh host $(BUILD)
test-fat-partition-mutant: $(foreach m,$(PARTITION_MUTANTS),$(BUILD)/test_partition_$(m)) $(BUILD)/test_partition
	@sh $(PARTITION_DIR)/partition.sh mint $(BUILD)
	@set -eu; for m in $(PARTITION_MUTANTS); do \
	 log=$(BUILD)/kzfs/mut-$$m.log; \
	 if $(BUILD)/test_partition_$$m $(PARTITIONED_DATA_IMG) > $$log 2>&1; then echo "FAIL partition $$m survived"; exit 1; fi; \
	 grep '^RED partition ' $$log || { cat $$log; exit 1; }; \
	 echo "GREEN partition mutant $$m rejected"; done
test-fat-partition-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/test_partition $(BUILD)/partition_probe.bin $(CHKDSK_PROG_BIN)
	@INITECH_QMP_STDIO=1 sh $(PARTITION_DIR)/partition.sh emu $(BUILD) $(SECONDARY_TRACER_IMG)
test-fat-partition-emu-mutant: $(HARNESS_BIN) $(BUILD)/test_partition $(BUILD)/partition_probe.bin $(CHKDSK_PROG_BIN) $(foreach m,$(PARTITION_MUTANTS),$(BUILD)/partition_$(m)_boot.img)
	@set -eu; for m in $(PARTITION_MUTANTS); do \
	 log=$(BUILD)/kzfs/emu-mut-$$m.log; mkdir -p $(BUILD)/kzfs; \
	 if INITECH_QMP_STDIO=1 sh $(PARTITION_DIR)/partition.sh emu-mut $(BUILD) $(BUILD)/partition_$${m}_boot.img $$m > $$log 2>&1; then \
	 echo "FAIL partition emu $$m survived"; exit 1; fi; \
	 grep '^RED partition ' $$log || { cat $$log; exit 1; }; \
	 echo "GREEN partition emu mutant $$m rejected"; done
TEST_UNIT_GATES += test-fat-partition test-fat-partition-mutant
TEST_EMU_GATES += test-fat-partition-emu test-fat-partition-emu-mutant

# Own contract gate; intentionally outside the INT21h Appendix-A AH walk.
.PHONY: test-partition-spec test-partition-spec-mutant
test-partition-spec: $(BUILD)/test_partition spec/mbr_partition_contract.json
	@$(BUILD)/test_partition --spec spec/mbr_partition_contract.json
test-partition-spec-mutant: $(BUILD)/test_partition spec/mbr_partition_contract.json
	@sed 's/"table_offset": 446/"table_offset": 445/' spec/mbr_partition_contract.json > $(BUILD)/partition_spec_mut.json
	@if $(BUILD)/test_partition --spec $(BUILD)/partition_spec_mut.json > $(BUILD)/partition-spec-mut.log 2>&1; then echo 'FAIL partition spec mutant survived'; exit 1; fi
	@grep '^RED partition SPEC contract drift' $(BUILD)/partition-spec-mut.log
	@echo 'GREEN test-partition-spec-mutant: wrong table offset rejected'
test-spec: test-partition-spec
TEST_UNIT_GATES += test-partition-spec test-partition-spec-mutant

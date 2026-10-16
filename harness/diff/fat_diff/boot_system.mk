# initech-kod: authored before loader/layout implementation.
# Ref: ADR-0003 DEC-01/07/08, Appendix B.1/C; IBM DOS 3.30 TR printed
# 1-7,5-4,5-7..11 (PDF24,111,114..118); MS-DOS 3.3 User's Guide
# printed 41/119 (PDF54/204). Contiguous output is lane policy; p119 permits
# fragmented system files, so the loader must walk FAT rather than assume LBA.
BOOT_SYSTEM_DIR := harness/diff/fat_diff
BOOT_SYSTEM_MUTANTS := ORDER ATTR_R CHAIN
$(BUILD)/boot_system: $(BOOT_SYSTEM_DIR)/boot_system.c | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@
.PHONY: test-boot-system test-boot-system-mutant test-boot-system-emu test-boot-system-negative test-boot-system-emu-mutant test-boot-system-repro test-boot-system-budget

test-boot-system: $(BUILD)/boot_system $(SECONDARY_TRACER_IMG) $(FLAIRTENANTS_IMG)
	@sh $(BOOT_SYSTEM_DIR)/boot_system.sh host $(BUILD) $(SECONDARY_TRACER_IMG)
	@sh $(BOOT_SYSTEM_DIR)/boot_system.sh host $(BUILD) $(FLAIRTENANTS_IMG) shipped $(INITDOS_SYS_BIN)
test-boot-system-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/boot_system
	@INITECH_QMP_STDIO=1 sh $(BOOT_SYSTEM_DIR)/boot_system.sh emu $(BUILD) $(SECONDARY_TRACER_IMG)
	@cp -f $(SECONDARY_TRACER_IMG) $(BUILD)/boot-system/fragmented.img
	@$(BUILD)/boot_system $(BUILD)/boot-system/fragmented.img FRAGMENT
	@set -eu; for f in IO.SYS INITDOS.SYS; do \
	 mcopy -i $(BUILD)/boot-system/fragmented.img@@32256 ::$$f - > $(BUILD)/boot-system/fragmented-$$f; \
	 ref=$(IO_SYS_BIN); [ $$f != INITDOS.SYS ] || ref=$(BUILD)/kernel_shell_secondary.bin; \
	 cmp $$ref $(BUILD)/boot-system/fragmented-$$f; done
	@dd if=$(BUILD)/boot-system/fragmented.img of=$(BUILD)/boot-system/fragmented-volume.img bs=512 skip=63 count=2880 status=none
	@fsck.fat -n $(BUILD)/boot-system/fragmented-volume.img > $(BUILD)/boot-system/fragmented-fsck.log 2>&1
	@INITECH_QMP_STDIO=1 sh $(BOOT_SYSTEM_DIR)/boot_system.sh emu $(BUILD) $(BUILD)/boot-system/fragmented.img fragmented
test-boot-system-mutant: test-boot-system
	@set -eu; for m in $(BOOT_SYSTEM_MUTANTS); do \
	 cp -f $(SECONDARY_TRACER_IMG) $(BUILD)/boot-system/host-$$m.img; \
	 $(BUILD)/boot_system $(BUILD)/boot-system/host-$$m.img $$m; \
	 if $(BUILD)/boot_system $(BUILD)/boot-system/host-$$m.img check > $(BUILD)/boot-system/host-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep '^RED boot-system ' $(BUILD)/boot-system/host-$$m.log; \
	 echo "GREEN test-boot-system-mutant $$m rejected"; done
test-boot-system-negative: $(HARNESS_BIN) $(BUILD)/boot_system $(SECONDARY_TRACER_IMG)
	@mkdir -p $(BUILD)/boot-system
	@set -eu; for m in MISSING $(BOOT_SYSTEM_MUTANTS); do \
	 cp -f $(SECONDARY_TRACER_IMG) $(BUILD)/boot-system/negative-$$m.img; \
	 if [ $$m = MISSING ]; then \
	 mattrib -i $(BUILD)/boot-system/negative-$$m.img@@32256 -h -s -r ::IO.SYS; \
	 mdel -i $(BUILD)/boot-system/negative-$$m.img@@32256 ::IO.SYS; \
	 else $(BUILD)/boot_system $(BUILD)/boot-system/negative-$$m.img $$m; fi; \
	 INITECH_QMP_STDIO=1 sh $(BOOT_SYSTEM_DIR)/boot_system.sh negative $(BUILD) $(BUILD)/boot-system/negative-$$m.img $$m; done
TEST_UNIT_GATES += test-boot-system test-boot-system-mutant test-boot-system-repro test-boot-system-budget
TEST_EMU_GATES += test-boot-system-emu test-boot-system-negative test-boot-system-emu-mutant

# Real artifact mutants. Each altered loader boots the same image fault that
# the production loader rejects. The negative oracle must itself go RED, and
# KERNEL must be reached, proving the intended guard was bypassed.
define boot-system-mutant
$(BUILD)/stage2_system_$(1).bin: $(STAGE2_ASM) $(IO_SYS_BIN) Makefile
	$$(NASM) -f bin -DMK_KERNEL_SECTORS=$$(KERNEL_SECTORS) -DIO_SYS_PATH='"$$(IO_SYS_BIN)"' -DBOOT_MUT_$(1) $$< -o $$@
endef
$(foreach m,$(BOOT_SYSTEM_MUTANTS),$(eval $(call boot-system-mutant,$(m))))

test-boot-system-emu-mutant: $(HARNESS_BIN) $(BUILD)/boot_system $(SECONDARY_TRACER_IMG) $(foreach m,$(BOOT_SYSTEM_MUTANTS),$(BUILD)/stage2_system_$(m).bin)
	@mkdir -p $(BUILD)/boot-system
	@set -eu; for m in $(BOOT_SYSTEM_MUTANTS); do \
	 img=$(BUILD)/boot-system/loader-$$m.img; log=$(BUILD)/boot-system/loader-$$m.log; \
	 cp -f $(SECONDARY_TRACER_IMG) $$img; \
	 dd if=$(BUILD)/stage2_system_$$m.bin of=$$img bs=512 seek=1 conv=notrunc status=none; \
	 $(BUILD)/boot_system $$img $$m; \
	 if INITECH_QMP_STDIO=1 sh $(BOOT_SYSTEM_DIR)/boot_system.sh negative $(BUILD) $$img loader-$$m > $$log 2>&1; then echo "FAIL loader $$m survived"; exit 1; fi; \
	 grep "^RED boot-system loader-$$m missing Non-System message$$" $$log || { cat $$log; exit 1; }; \
	 grep -qx KERNEL $(BUILD)/boot-system/boot-system-loader-$$m.serial; \
	 echo "GREEN test-boot-system-emu-mutant $$m rejected (fault incorrectly booted to KERNEL)"; done

test-boot-system-repro: $(FLAIRTENANTS_IMG) $(BOOT_SYSTEM_MINT) $(BOOT_SYSTEM_TOOL)
	@mkdir -p $(BUILD)/boot-system
	@set -eu; for tag in a b; do \
	 sh $(BOOT_SYSTEM_MINT) $(BUILD)/boot-system/repro-$$tag.img $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(INITDOS_SYS_BIN) $(BOOT_SYSTEM_TOOL); done
	@cmp $(BUILD)/boot-system/repro-a.img $(BUILD)/boot-system/repro-b.img
	@cmp -n $$(( 2944 * 512 )) $(FLAIRTENANTS_IMG) $(BUILD)/boot-system/repro-a.img
	@echo 'GREEN test-boot-system-repro: two fresh FAT builds == each other == shipped boot image, byte for byte'

test-boot-system-budget: $(STAGE2_BIN) $(IO_SYS_BIN) $(KERNEL_FLAIRTENANTS_ELF) $(BUILD)/kernel_shell_secondary.elf
	@set -eu; test $$(wc -c < $(STAGE2_BIN)) -eq 8192; io=$$(wc -c < $(IO_SYS_BIN)); test $$io -le 4096; \
	 echo "GREEN test-boot-system-budget: stage2 8192/8192 B (bootstrap <=4096 + IO.SYS $$io/4096); assembly guards enforced"; \
	 for e in $(KERNEL_FLAIRTENANTS_ELF) $(BUILD)/kernel_shell_secondary.elf; do \
	 $(OBJCOPY) -O binary $$e $(BUILD)/boot-budget.bin; size=$$(wc -c < $(BUILD)/boot-budget.bin); \
	 left=$$(( $(KERNEL_SECTORS)*512-size )); test $$left -ge $(KERNEL_HEADROOM_FLOOR); \
	 echo "GREEN test-boot-system-budget: $$e flat loaded=$$size B, bounce headroom=$$left B >= $(KERNEL_HEADROOM_FLOOR) B"; done
	@rm -f $(BUILD)/boot-budget.bin

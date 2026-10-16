# CHKDSK one-shot lane (initech-kd4a). Tests were authored before chkdsk.c.
# Ref: ADR-0003 DEC-08; m0dc FIND_PROG/MORE_PROG recipes + injected shell trace.
CHKDSK_DIR := harness/diff/fat_diff
CHKDSK_SRC := os/milton/chkdsk.c
CHKDSK_ADAPTER := os/milton/chkdsk_program.c
CHKDSK_PROG_BIN := $(BUILD)/chkdsk_program.bin
CHKDSK_FIXTURE := $(BUILD)/chkdsk_fixture
CHKDSK_HOST := $(BUILD)/chkdsk_host
CHKDSK_MUTANTS := REPORT LOST CROSS INVALID VERBOSE FRAG NOFIX RECOVER DISCARD MEMORY MIRROR
CHKDSK_INC := -Ios/milton -Ispec -I$(CHKDSK_DIR) -I$(BUILD)
CHKDSK_HOST_SRC := $(CHKDSK_DIR)/chkdsk_host.c $(CHKDSK_SRC) os/milton/mcb.c $(CHKDSK_DIR)/blockdev_file.c

$(CHKDSK_FIXTURE): $(CHKDSK_DIR)/chkdsk_fixture.c | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation $< -o $@
$(CHKDSK_HOST): $(CHKDSK_HOST_SRC) os/milton/chkdsk.h $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $(CHKDSK_INC) $(CHKDSK_HOST_SRC) -o $@
$(BUILD)/chkdsk_start.o: os/milton/chkdsk_start.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/chkdsk_core.o: $(CHKDSK_SRC) os/milton/chkdsk.h $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Os $(CHKDSK_INC) -c $< -o $@
$(BUILD)/chkdsk_program.o: $(CHKDSK_ADAPTER) os/milton/chkdsk.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) -Os $(CHKDSK_INC) -c $< -o $@
$(BUILD)/chkdsk.elf: $(BUILD)/chkdsk_start.o $(BUILD)/chkdsk_core.o $(BUILD)/chkdsk_program.o $(KERNEL_MCB_OBJ) os/milton/chkdsk.ld
	$(LD) -m elf_i386 -T os/milton/chkdsk.ld -o $@ $(filter %.o,$^)
$(CHKDSK_PROG_BIN): $(BUILD)/chkdsk.elf
	$(OBJCOPY) -O binary $< $@

define chkdsk-mutant
$(BUILD)/chkdsk_host_$(1): $(CHKDSK_HOST_SRC) os/milton/chkdsk.h $(DOS_MESSAGES_H) | $(BUILD)
	$$(CC) -std=c11 -O2 -Wall -Wextra -Werror -DCHKDSK_MUT_$(1) $$(CHKDSK_INC) $$(CHKDSK_HOST_SRC) -o $$@
$(BUILD)/chkdsk_core_$(1).o: $(CHKDSK_SRC) os/milton/chkdsk.h $(DOS_MESSAGES_H) | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) -Os -DCHKDSK_MUT_$(1) $$(CHKDSK_INC) -c $$< -o $$@
$(BUILD)/chkdsk_$(1).elf: $(BUILD)/chkdsk_start.o $(BUILD)/chkdsk_core_$(1).o $(BUILD)/chkdsk_program.o $(KERNEL_MCB_OBJ) os/milton/chkdsk.ld
	$$(LD) -m elf_i386 -T os/milton/chkdsk.ld -o $$@ $$(filter %.o,$$^)
$(BUILD)/chkdsk_$(1).bin: $(BUILD)/chkdsk_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
endef
$(foreach m,$(CHKDSK_MUTANTS),$(eval $(call chkdsk-mutant,$(m))))

# The cache-coherency rule lives in the kernel, so its mutant must boot a
# different kernel with the SAME unmutated CHKDSK.COM and injected trace.
$(BUILD)/kmain_chkdsk_cache.o: $(KERNEL_MAIN_C) $(KERNEL_SECONDARY_SHELL_MAIN_OBJ)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DBOOT_SHELL -DBOOT_SECONDARY_SHELL -DCHKDSK_MUT_CACHE -Ispec -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/kernel_chkdsk_cache.elf: $(filter-out $(KERNEL_SECONDARY_SHELL_MAIN_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/kmain_chkdsk_cache.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_chkdsk_cache.bin: $(BUILD)/kernel_chkdsk_cache.elf
	$(OBJCOPY) -O binary $< $@
	@set -e; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$((max-sz))" conv=notrunc status=none
$(BUILD)/chkdsk_cache_boot.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_chkdsk_cache.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_chkdsk_cache.bin $(BOOT_SYSTEM_TOOL)

.PHONY: test-chkdsk test-chkdsk-mutant test-chkdsk-emu test-chkdsk-emu-mutant
test-chkdsk: $(CHKDSK_HOST) $(CHKDSK_FIXTURE)
	@sh $(CHKDSK_DIR)/chkdsk.sh host $(BUILD) $(CHKDSK_HOST) $(CHKDSK_FIXTURE)
test-chkdsk-emu: $(CHKDSK_PROG_BIN) $(CHKDSK_FIXTURE) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(CHKDSK_PROG_BIN) $(CHKDSK_FIXTURE) $(SECONDARY_TRACER_IMG)
test-chkdsk-mutant: $(foreach m,$(CHKDSK_MUTANTS),$(BUILD)/chkdsk_host_$(m)) $(CHKDSK_FIXTURE)
	@set -eu; for m in $(CHKDSK_MUTANTS); do \
	 log=$(BUILD)/chkdsk-mutant-$$m.log; \
	 if sh $(CHKDSK_DIR)/chkdsk.sh host $(BUILD) $(BUILD)/chkdsk_host_$$m $(CHKDSK_FIXTURE) > $$log 2>&1; then \
	  echo "FAIL CHKDSK mutant $$m survived"; exit 1; fi; \
	 grep 'RED CHKDSK-' $$log | tail -1 || { cat $$log; exit 1; }; \
	 echo "GREEN CHKDSK mutant $$m rejected"; done
test-chkdsk-emu-mutant: $(foreach m,$(CHKDSK_MUTANTS),$(BUILD)/chkdsk_$(m).bin) $(CHKDSK_FIXTURE) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/chkdsk_cache_boot.img $(CHKDSK_PROG_BIN)
	@set -eu; for m in $(CHKDSK_MUTANTS); do \
	 log=$(BUILD)/chkdsk-emu-mutant-$$m.log; \
	 if INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(BUILD)/chkdsk_$$m.bin $(CHKDSK_FIXTURE) $(SECONDARY_TRACER_IMG) > $$log 2>&1; then \
	  echo "FAIL CHKDSK emu mutant $$m survived"; exit 1; fi; \
	 grep 'RED CHKDSK-' $$log | tail -1 || { cat $$log; exit 1; }; \
	 echo "GREEN CHKDSK emu mutant $$m rejected"; done
	@log=$(BUILD)/chkdsk-emu-mutant-CACHE.log; \
	 if INITECH_QMP_STDIO=1 sh $(CHKDSK_DIR)/chkdsk.sh emu $(BUILD) $(CHKDSK_PROG_BIN) $(CHKDSK_FIXTURE) $(BUILD)/chkdsk_cache_boot.img > $$log 2>&1; then \
	 echo 'FAIL CHKDSK CACHE mutant survived'; exit 1; fi; \
	 grep -F 'RED CHKDSK-CACHE' $$log || { cat $$log; exit 1; }; \
	 echo 'GREEN CHKDSK emu mutant CACHE rejected'
TEST_UNIT_GATES += test-chkdsk test-chkdsk-mutant
TEST_EMU_GATES += test-chkdsk-emu test-chkdsk-emu-mutant

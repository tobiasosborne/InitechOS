# initech-udr1 EDLIN only. Oracle/gates precede implementation (Rule 1).
# Ref: MS-DOS 3.3 User's Guide pp.162-193; ADR-0003 App A / DEC-08;
# m0dc FIND_PROG_/MORE_PROG_ + secondary-shell injected-key gates.
EDLIN_PROG_BIN := $(BUILD)/edlin_program.bin
EDLIN_MODEL := $(BUILD)/edlin_model
EDLIN_HOST := $(BUILD)/edlin_host
EDLIN_CORE := os/milton/edlin.c
EDLIN_CORE_HDRS := os/milton/edlin.h os/milton/conedit.h
EDLIN_MUTANTS := NUMBER BACKUP FIRST QUIT
EDLIN_FLAGS := -std=c11 -O2 -Wall -Wextra -Werror -Ios/milton -I$(BUILD)
$(EDLIN_MODEL): harness/edlin/model.c | $(BUILD)
	$(CC) $(EDLIN_FLAGS) $< -o $@
$(EDLIN_HOST): harness/edlin/host.c $(wildcard $(EDLIN_CORE)) $(EDLIN_CORE_HDRS) $(DOS_MESSAGES_H) | $(BUILD)
	$(CC) $(EDLIN_FLAGS) $(if $(wildcard $(EDLIN_CORE)),,-DEDLIN_ABSENT) $(filter %.c,$^) -o $@
$(BUILD)/edlin_start.o: os/milton/edlin_start.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/edlin_program.o: os/milton/edlin_program.c os/milton/edlin.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS_CDECL) -Os -Ios/milton -Ispec -c $< -o $@
$(BUILD)/edlin_core.o: $(EDLIN_CORE) $(EDLIN_CORE_HDRS) $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS_CDECL) -Os -Ios/milton -I$(BUILD) -c $< -o $@
$(BUILD)/edlin.elf: $(BUILD)/edlin_start.o $(BUILD)/edlin_core.o $(BUILD)/edlin_program.o os/milton/edlin.ld
	$(LD) -m elf_i386 -T os/milton/edlin.ld -o $@ $(filter %.o,$^)
$(EDLIN_PROG_BIN): $(BUILD)/edlin.elf
	$(OBJCOPY) -O binary $< $@
define edlin-mutant
$(BUILD)/edlin_host_$(1): harness/edlin/host.c $(EDLIN_CORE) $(EDLIN_CORE_HDRS) $(DOS_MESSAGES_H) | $(BUILD)
	$$(CC) $$(EDLIN_FLAGS) -DEDLIN_MUT_$(1) $$(filter %.c,$$^) -o $$@
$(BUILD)/edlin_core_$(1).o: $(EDLIN_CORE) $(EDLIN_CORE_HDRS) $(DOS_MESSAGES_H) | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS_CDECL) -Os -Ios/milton -I$(BUILD) -DEDLIN_MUT_$(1) -c $$< -o $$@
$(BUILD)/edlin_$(1).elf: $(BUILD)/edlin_start.o $(BUILD)/edlin_core_$(1).o $(BUILD)/edlin_program.o os/milton/edlin.ld
	$$(LD) -m elf_i386 -T os/milton/edlin.ld -o $$@ $$(filter %.o,$$^)
$(BUILD)/edlin_$(1).bin: $(BUILD)/edlin_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
endef
$(foreach m,$(EDLIN_MUTANTS),$(eval $(call edlin-mutant,$(m))))
.PHONY: test-edlin-host test-edlin-emu test-edlin-host-mutant test-edlin-emu-mutant
test-edlin-host: $(EDLIN_MODEL) $(EDLIN_HOST)
	@sh harness/diff/fat_diff/edlin.sh host $(BUILD) $(EDLIN_HOST) $(EDLIN_MODEL)
test-edlin-emu: $(EDLIN_MODEL) $(EDLIN_PROG_BIN) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/edlin.sh emu $(BUILD) $(EDLIN_PROG_BIN) $(EDLIN_MODEL) $(SECONDARY_TRACER_IMG)
define edlin-mutant-gate
test-edlin-$(1)-mutant: $(EDLIN_MODEL) $(foreach m,$(EDLIN_MUTANTS),$(BUILD)/edlin_$(if $(filter host,$(1)),host_,)$(m)$(if $(filter emu,$(1)),.bin)) $(if $(filter emu,$(1)),$(HARNESS_BIN) $(SECONDARY_TRACER_IMG))
	@set -eu; for pair in NUMBER:transcript BACKUP:backup FIRST:output QUIT:transcript; do \
	 m=$$$${pair%:*}; why=$$$${pair#*:}; log=$(BUILD)/edlin-$(1)-$$$$m.log; \
	 if INITECH_QMP_STDIO=1 EDLIN_ONLY=$$$$m sh harness/diff/fat_diff/edlin.sh $(1) $(BUILD) $(BUILD)/edlin_$(if $(filter host,$(1)),host_,)$$$$m$(if $(filter emu,$(1)),.bin) $(EDLIN_MODEL) $(SECONDARY_TRACER_IMG) >$$$$log 2>&1; then \
	 echo "FAIL EDLIN $$$$m survived"; exit 1; fi; \
	 grep "RED EDLIN-$$$$why" $$$$log || { cat $$$$log; exit 1; }; \
	 echo "GREEN test-edlin-$(1)-mutant $$$$m rejected"; done
endef
$(eval $(call edlin-mutant-gate,host))
$(eval $(call edlin-mutant-gate,emu))
TEST_UNIT_GATES += test-edlin-host test-edlin-host-mutant
TEST_EMU_GATES += test-edlin-emu test-edlin-emu-mutant

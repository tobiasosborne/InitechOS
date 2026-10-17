# initech-s25. Gates FIRST: PRD Sec 5 / Appendix B, CLAUDE Rule 2.
# PANIC.COM is a test fixture only; it never grows the shipped data volume.
PANIC_SCREEN_DIR := $(BUILD)/panic-screen
PANIC_SCREEN_DEPS := os/milton/panic_screen.h os/milton/console.h os/milton/boot_info.h
$(KERNEL_PANIC_OBJ): $(PANIC_SCREEN_DEPS)
$(PANIC_SCREEN_DIR): | $(BUILD)
	@mkdir -p $@
$(BUILD)/test_panic_layout: harness/proptest/test_panic_layout.c os/milton/panic_screen.h | $(BUILD)
	$(CC) $(CFLAGS) -Ios/milton $< -o $@
$(BUILD)/test_panic_layout_mut: harness/proptest/test_panic_layout.c os/milton/panic_screen.h | $(BUILD)
	$(CC) $(CFLAGS) -DPANIC_MUTATE_OFF_CENTRE -Ios/milton $< -o $@
$(BUILD)/ppm_panic_check: tools/ppm_panic_check.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
$(PANIC_SCREEN_DIR)/ud.com: os/milton/panic_program.asm | $(PANIC_SCREEN_DIR)
	$(NASM) -f bin $< -o $@
$(PANIC_SCREEN_DIR)/gp.com: os/milton/panic_program.asm | $(PANIC_SCREEN_DIR)
	$(NASM) -f bin -DPANIC_FIXTURE_GP $< -o $@

# Isolated objects: the shipped panic.o has debug OFF; debug and mutant
# kernels substitute just panic.o, keeping the actual DOS-only console path.
define panic-screen-kernel
$(BUILD)/panic_screen_$(1).o: $(KERNEL_PANIC_C) $(PANIC_SCREEN_DEPS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_LOWLEVEL_OPT) --param=min-pagesize=0 $(2) -I$(KERNEL_DIR) -c $$< -o $$@
$(BUILD)/kernel_panic_screen_$(1).elf: $(filter-out $(KERNEL_PANIC_OBJ),$(KERNEL_SHELL_OBJS)) $(BUILD)/panic_screen_$(1).o $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $(filter-out $(KERNEL_PANIC_OBJ),$(KERNEL_SHELL_OBJS)) $(BUILD)/panic_screen_$(1).o
$(BUILD)/kernel_panic_screen_$(1).bin: $(BUILD)/kernel_panic_screen_$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@set -e; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS)*512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$((max-sz))" conv=notrunc status=none
	$$(call kernel-end-guard,$$<,panic-screen-$(1))
endef
$(eval $(call panic-screen-kernel,debug,-DPANIC_DEBUG=1))
$(eval $(call panic-screen-kernel,no8,-DPANIC_DEBUG=1 -DPANIC_MUTATE_NO_8BPP))

$(PANIC_SCREEN_DIR)/IO_vga13.SYS: $(BOOT_DIR)/io_sys.asm | $(PANIC_SCREEN_DIR)
	$(NASM) -f bin -DFORCE_VBE_101 -DVBE_MUT_NO_101 -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) $< -o $@
$(PANIC_SCREEN_DIR)/stage2_vga13.bin: $(STAGE2_ASM) $(PANIC_SCREEN_DIR)/IO_vga13.SYS
	$(NASM) -f bin -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) -DIO_SYS_PATH='"$(PANIC_SCREEN_DIR)/IO_vga13.SYS"' $< -o $@
define panic-screen-image
$(PANIC_SCREEN_DIR)/$(1).img: $(MBR_BIN) $(2) $(3) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(2) $(4) $(3) $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call panic-screen-image,debug32,$(STAGE2_BIN),$(BUILD)/kernel_panic_screen_debug.bin,$(IO_SYS_BIN)))
$(eval $(call panic-screen-image,debug8,$(VESA8_DIR)/stage2.bin,$(BUILD)/kernel_panic_screen_debug.bin,$(VESA8_DIR)/IO.SYS))
$(eval $(call panic-screen-image,debug13,$(PANIC_SCREEN_DIR)/stage2_vga13.bin,$(BUILD)/kernel_panic_screen_debug.bin,$(PANIC_SCREEN_DIR)/IO_vga13.SYS))
$(eval $(call panic-screen-image,no8,$(VESA8_DIR)/stage2.bin,$(BUILD)/kernel_panic_screen_no8.bin,$(VESA8_DIR)/IO.SYS))

.PHONY: test-panic-layout test-panic-layout-mutant test-panic-screen test-panic-screen-mutant test-panic-screen-bochs
test-panic-layout: $(BUILD)/test_panic_layout
	@$<
test-panic-layout-mutant: $(BUILD)/test_panic_layout_mut
	@if $< > $(BUILD)/panic_layout_mut.red 2>&1; then echo 'FAIL PANIC_MUTATE_OFF_CENTRE survived'; exit 1; fi
	@grep '^RED panic-layout: off-centre ' $(BUILD)/panic_layout_mut.red
	@echo 'GREEN test-panic-layout-mutant: PANIC_MUTATE_OFF_CENTRE rejected'
test-panic-screen: $(HARNESS_BIN) $(TRACER_IMG) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/debug8.img $(PANIC_SCREEN_DIR)/debug13.img $(PANIC_SCREEN_DIR)/ud.com $(PANIC_SCREEN_DIR)/gp.com $(BUILD)/ppm_panic_check
	@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/ud.com ud32 640 480 6 1
	@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/gp.com gp32 640 480 13 1
	@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug8.img $(PANIC_SCREEN_DIR)/ud.com ud8 640 480 6 1
	@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug13.img $(PANIC_SCREEN_DIR)/ud.com ud13 320 200 6 1
	@sh harness/emu/panic_screen.sh $(BUILD) $(TRACER_IMG) $(PANIC_SCREEN_DIR)/gp.com quiet32 640 480 13 0
	@echo 'GREEN test-panic-screen: #UD/#GP, 32bpp/VBE8/VGA13 pixels, debug on/off'
test-panic-screen-mutant: $(HARNESS_BIN) $(PANIC_SCREEN_DIR)/no8.img $(PANIC_SCREEN_DIR)/ud.com $(BUILD)/ppm_panic_check
	@if sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/no8.img $(PANIC_SCREEN_DIR)/ud.com no8 640 480 6 1 > $(PANIC_SCREEN_DIR)/no8.red 2>&1; then echo 'FAIL PANIC_MUTATE_NO_8BPP survived'; exit 1; fi
	@grep '^GREEN panic-serial no8:' $(PANIC_SCREEN_DIR)/no8.red
	@grep '^RED panic-pixels:' $(PANIC_SCREEN_DIR)/no8.red
	@echo 'GREEN test-panic-screen-mutant: PANIC_MUTATE_NO_8BPP rejected by pixels after real fault'

# Bochs RFB does not render VGA (harness/emu/bochs.h). This is an explicit
# serial/exception leg, not a claim of Bochs pixel grading. PM runs this leg.
ifeq ($(SKIP_BOCHS),1)
test-panic-screen-bochs:
	@echo 'SKIPPED test-panic-screen-bochs: SKIP_BOCHS=1; no accuracy coverage'
else
test-panic-screen-bochs: $(BOCHS_BIN) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/ud.com
	@dd if=/dev/zero of=$(PANIC_SCREEN_DIR)/bochs-data.img bs=512 count=2880 status=none
	@mformat -i $(PANIC_SCREEN_DIR)/bochs-data.img -f 1440 ::
	@mcopy -i $(PANIC_SCREEN_DIR)/bochs-data.img $(PANIC_SCREEN_DIR)/ud.com ::PANIC.COM
	@printf '@ECHO OFF\r\nPANIC\r\n' > $(PANIC_SCREEN_DIR)/bochs.bat
	@mcopy -i $(PANIC_SCREEN_DIR)/bochs-data.img $(PANIC_SCREEN_DIR)/bochs.bat ::AUTOEXEC.BAT
	@$(call fixture-check,recipe,$(PANIC_SCREEN_DIR)/bochs-data.img,PANIC.COM AUTOEXEC.BAT)
	@$(BOCHS_BIN) --disk $(PANIC_SCREEN_DIR)/debug32.img --disk2 $(PANIC_SCREEN_DIR)/bochs-data.img --name panic_screen_bochs --out $(PANIC_SCREEN_DIR) --timeout-ms 45000 --expect HALTED --quit-after HALTED 2> $(PANIC_SCREEN_DIR)/bochs.report
	@grep -q 'rfb_unblocked=1' $(PANIC_SCREEN_DIR)/bochs.report
	@grep -q 'triple_fault=0' $(PANIC_SCREEN_DIR)/bochs.report
	@tr -d '\r' < $(PANIC_SCREEN_DIR)/panic_screen_bochs.serial > $(PANIC_SCREEN_DIR)/bochs.norm
	@grep -qx 'PANIC-PROBE-ARMED' $(PANIC_SCREEN_DIR)/bochs.norm
	@grep -q '^PANIC vec=06 err=00000000$$' $(PANIC_SCREEN_DIR)/bochs.norm
	@grep -q '^  eax=12345678 ebx=23456789 ecx=3456789A edx=456789AB$$' $(PANIC_SCREEN_DIR)/bochs.norm
	@echo 'GREEN test-panic-screen-bochs: real FAT .COM #UD, debug register dump, no triple fault (serial only)'
endif
TEST_UNIT_GATES += test-panic-layout test-panic-layout-mutant
TEST_EMU_GATES += test-panic-screen test-panic-screen-mutant test-panic-screen-bochs

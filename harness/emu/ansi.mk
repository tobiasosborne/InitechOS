# ANSI console lane. Ground truth: IBM DOS 3.30 Technical Reference printed
# pp. 3-4, 3-7/3-8, 3-10/3-11, 3-12, 3-15; ADR-0003 Appendix A.
# No FAT algorithm changed: mtools mints and reads back the identical fixture.
# The console oracle is independent literal report/attribute data, not mtools.

$(KERNEL_INT21_OBJ): $(MILTON_DIR)/ansi.c $(MILTON_DIR)/ansi.h
ANSI_LANE_MUTANTS := ANSI_MUTATE_CONCEAL_BRIGHT ANSI_MUTATE_PARAM_UNBOUNDED ANSI_MUTATE_ZERO_LITERAL ANSI_MUTATE_STALE_REL
ANSI_WIRE_LANE_MUTANTS := $(ANSI_LANE_MUTANTS) ANSIWIRE_MUTATE_DROP_REL ANSIWIRE_MUTATE_DROP_DSR

$(BUILD)/test_ansi_lane_mut_%: $(TEST_ANSI_SRC) $(TEST_ANSI_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -D$* -I$(MILTON_DIR) -Iseed -o $@ $<
$(BUILD)/test_ansi_wire_lane_mut_%: $(TEST_ANSI_WIRE_SRC) $(TEST_ANSI_WIRE_DEPS) $(TEST_ANSI_WIRE_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -D$* -Ispec -I$(MILTON_DIR) -Iseed -Ibuild -o $@ $< $(TEST_ANSI_WIRE_DEPS)

.PHONY: test-ansi-lane-mutant
test-ansi-lane-mutant: $(addprefix $(BUILD)/test_ansi_lane_mut_,$(ANSI_LANE_MUTANTS)) $(addprefix $(BUILD)/test_ansi_wire_lane_mut_,$(ANSI_WIRE_LANE_MUTANTS))
	@set -e; for bin in $^; do \
	 if $$bin > $$bin.log 2>&1; then printf '!!! %s survived\n' "$$bin"; exit 1; fi; \
	 grep -q 'checks,.* failures' $$bin.log; grep -q '^  FAIL ' $$bin.log; \
	 printf '>>> %s correctly RED: %s\n' "$$bin" "$$(grep -m1 '^  FAIL ' $$bin.log)"; done
	@printf '>>> test-ansi-lane-mutant: green (10 mutant executions rejected)\n'

$(BUILD)/ansi_probe.com: $(MILTON_DIR)/ansi_probe_program.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/ansi_probe.bat: | $(BUILD)
	@printf '@ECHO OFF\r\nANSIPROB\r\n' > $@
$(BUILD)/ansi_probe_data.img: $(BUILD)/ansi_probe.com $(BUILD)/ansi_probe.bat | $(BUILD)
	@dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	@mformat -i $@ -f 1440 ::
	@mcopy -i $@ $(BUILD)/ansi_probe.com ::ANSIPROB.COM
	@mcopy -i $@ $(BUILD)/ansi_probe.bat ::AUTOEXEC.BAT
	@mtype -i $@ ::ANSIPROB.COM | cmp - $(BUILD)/ansi_probe.com
	@$(call fixture-check,recipe,$@,ANSIPROB.COM AUTOEXEC.BAT)

# Mutate the shipped dispatcher, not the probe or golden. The same flat .COM
# runs under the same boot setup; only one implementation branch changes.
define ansi-kernel-mutant
$(BUILD)/int21_ansi_$(1).o: $(KERNEL_INT21_C) $(TEST_ANSI_WIRE_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -D$(1) -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $$< -o $$@
$(BUILD)/kernel_ansi_$(1).elf: $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_ansi_$(1).o $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_ansi_$(1).o
$(BUILD)/kernel_ansi_$(1).bin: $(BUILD)/kernel_ansi_$(1).elf harness/emu/ansi.mk
	$(OBJCOPY) -O binary $$< $$@
	@set -e; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS) * 512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$((max - sz))" conv=notrunc status=none
	$$(call kernel-end-guard,$$<,ansi-$(1))
$(BUILD)/ansi_$(1).img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_ansi_$(1).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_ansi_$(1).bin $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call ansi-kernel-mutant,ANSIWIRE_MUTATE_DROP_REL))
$(eval $(call ansi-kernel-mutant,ANSIWIRE_MUTATE_DROP_DSR))

define ansi-probe-run
	@$(call fixture-preflight,$(BUILD)/ansi_probe_data.img)
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk "$(1)" --disk2 "$(BUILD)/ansi_probe_data.img" \
	 --name "$(2)" --out "$(BUILD)" --timeout-ms 25000 \
	 --keys "e,x,i,t,ret" --keys-after "SHELL-READY" $(SHELL_OS_COMPLETION) \
	 2> "$(BUILD)/$(2).report" || true
	@! grep -q 'triple_fault=1' $(BUILD)/$(2).report
	@grep -q '^SHELL-READY$$' $(BUILD)/$(2).serial
	@grep -q '^SHELL-EXIT$$' $(BUILD)/$(2).serial
	@tr -d '\r' < $(BUILD)/$(2).serial | sed -n '/^ANSI-PROBE /p' > $(BUILD)/$(2).got
endef

.PHONY: test-ansi-emu test-ansi-emu-mutant
test-ansi-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/ansi_probe_data.img harness/emu/ansi_probe.golden
	$(call ansi-probe-run,$(SECONDARY_TRACER_IMG),ansi_probe)
	@diff -u harness/emu/ansi_probe.golden $(BUILD)/ansi_probe.got
	@printf '>>> test-ansi-emu: green (15 AH=09h sequences, exact DSR replies via INT 21h stdin, SHELL-EXIT)\n'
test-ansi-emu-mutant: $(HARNESS_BIN) $(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_REL.img $(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_DSR.img $(BUILD)/ansi_probe_data.img harness/emu/ansi_probe.golden
	$(call ansi-probe-run,$(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_REL.img,ansi_probe_mut_rel)
	@grep -q '^ANSI-PROBE FAIL DSR$$' $(BUILD)/ansi_probe_mut_rel.got
	@! diff -q harness/emu/ansi_probe.golden $(BUILD)/ansi_probe_mut_rel.got >/dev/null
	@printf '>>> test-ansi-emu-mutant: ANSIWIRE_MUTATE_DROP_REL correctly RED: ANSI-PROBE FAIL DSR\n'
	$(call ansi-probe-run,$(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_DSR.img,ansi_probe_mut_dsr)
	@grep -q '^ANSI-PROBE FAIL DSR$$' $(BUILD)/ansi_probe_mut_dsr.got
	@! diff -q harness/emu/ansi_probe.golden $(BUILD)/ansi_probe_mut_dsr.got >/dev/null
	@printf '>>> test-ansi-emu-mutant: ANSIWIRE_MUTATE_DROP_DSR correctly RED: ANSI-PROBE FAIL DSR\n'
	@printf '>>> test-ansi-emu-mutant: green (both implementation mutants rejected)\n'

TEST_UNIT_GATES += test-ansi-lane-mutant
TEST_EMU_GATES += test-ansi-emu test-ansi-emu-mutant

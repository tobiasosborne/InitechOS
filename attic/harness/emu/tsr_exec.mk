# initech-h4bk: shell EXEC after KEEP. DEC-08a.1/.4; DEC-13; TSR contract.
# EDLIN is deliberately fixed-base: it runs before KEEP and must be refused
# afterward. Two relocated MZ executions and a live resident popup prove that
# refusal/termination reclaim the child MCB without overwriting the TSR.
TSR_EXEC_KEYS := WSHELL-READY,ke,kd,kl,ki,kn,kspc,kt,ke,ks,kt,kdot,kb,ka,kk,kret,WCannot edit,kt,ks,kr,kret,WSK-INSTALLED,km,kz,ke,kx,ke,kc,kret,ke,kd,kl,ki,kn,kspc,kt,ke,ks,kt,kdot,kb,ka,kk,kret,km,kz,ke,kx,ke,kc,kret,kw,ka,ki,kt,kk,ke,ky,kret,WWAITKEY-WAITING,kctrl-alt,WSK-POPUP,kx,WWAITKEY-GOT,ke,kx,ki,kt,kret
$(eval $(call fixture-contract,$(BUILD)/fat_tsr_exec.img,HELLO.TXT GREET.COM TSR.COM WAITKEY.COM MZEXEC.EXE EDLIN.COM))
$(BUILD)/tsr_exec_fixture.bin: $(MZEXEC_FIX_ASM) | $(BUILD)
	$(NASM) -f bin -DMZEXEC_TSR_CHECK $< -o $@
$(BUILD)/tsr_exec_fixture.exe: $(BUILD)/tsr_exec_fixture.bin $(MZLINK_BIN)
	$(MZLINK_BIN) $< $@ --reloc 8 --entry 7 --minalloc 16
$(BUILD)/fat_tsr_exec.img: $(BUILD)/fat_tsr.img $(BUILD)/tsr_exec_fixture.exe $(EDLIN_PROG_BIN)
	cp -f $< $@
	mcopy -o -i $@ $(BUILD)/tsr_exec_fixture.exe ::MZEXEC.EXE
	mcopy -o -i $@ $(EDLIN_PROG_BIN) ::EDLIN.COM
	@$(call fixture-check,recipe,$@)
define tsr-exec-boot
@$(call fixture-preflight,$(BUILD)/fat_tsr_exec.img)
@cp -f $(BUILD)/fat_tsr_exec.img $(BUILD)/$(2)_data.img
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(1) --disk2 $(BUILD)/$(2)_data.img \
	--name $(2) --out $(BUILD) --mouse '$(TSR_EXEC_KEYS)' --keys-after SHELL-READY \
	--quit-after SHELL-DONE --timeout-ms 45000 2>$(BUILD)/$(2).report || true
endef
.PHONY: test-tsr-exec test-tsr-exec-mutant
test-tsr-exec: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/fat_tsr_exec.img
	$(call tsr-exec-boot,$(SECONDARY_TRACER_IMG),tsr_exec)
	@sh harness/emu/tsr_exec_check.sh $(BUILD)/tsr_exec
	@printf '>>> test-tsr-exec: green (EDLIN fixed-base before KEEP; named refusal after KEEP; MZ relocates twice; resident popup survives)\n'

# Same input disk, trace and serial oracle for all three implementation
# mutations. Require the successful pre-KEEP EDLIN and installer, so a broken
# boot/fixture cannot count as killing a mutant (Rule 6).
TSR_EXEC_MUTANTS := fixed:LOADER_MUT_TSR_EXEC_FIXED_RELOC \
                   flat:LOADER_MUT_TSR_EXEC_ALLOW_FLAT \
                   overlap:LOADER_MUT_TSR_EXEC_OVERLAP
define tsr-exec-mutant
$(BUILD)/loader_tsr_exec_$(1).o: $(KERNEL_LOADER_C) $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/mz.c $(KERNEL_DIR)/mz.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) $$(KERNEL_SECTION_FLAGS) $$(KERNEL_LOADER_OPT) -D$(2) -Ispec -I$$(KERNEL_DIR) -c $$< -o $$@
$(BUILD)/kernel_tsr_exec_mut_$(1).elf: $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/loader_tsr_exec_$(1).o $(KERNEL_LD)
	$$(KERNEL_LINK) -o $$@ $$(filter %.o,$$^)
$(BUILD)/kernel_tsr_exec_mut_$(1).bin: $(BUILD)/kernel_tsr_exec_mut_$(1).elf
	$$(OBJCOPY) -O binary $$< $$@
	@test $$$$(wc -c < $$@) -le $$$$(( $$(KERNEL_SECTORS) * 512 ))
	@truncate -s $$$$(( $$(KERNEL_SECTORS) * 512 )) $$@
	$$(call kernel-end-guard,$$<,tsr-exec-$(1))
$(BUILD)/tsr_exec_$(1).img: $(BUILD)/kernel_tsr_exec_mut_$(1).bin $(MBR_BIN) $(STAGE2_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $$(BOOT_SYSTEM_MINT) $$@ $$(MBR_BIN) $$(STAGE2_BIN) $$(IO_SYS_BIN) $$< $$(BOOT_SYSTEM_TOOL)
endef
$(foreach m,$(TSR_EXEC_MUTANTS),$(eval $(call tsr-exec-mutant,$(word 1,$(subst :, ,$(m))),$(word 2,$(subst :, ,$(m))))))
test-tsr-exec-mutant: test-tsr-exec $(foreach m,$(TSR_EXEC_MUTANTS),$(BUILD)/tsr_exec_$(word 1,$(subst :, ,$(m))).img)
	$(call tsr-exec-boot,$(BUILD)/tsr_exec_fixed.img,tsr_exec_fixed)
	$(call tsr-exec-boot,$(BUILD)/tsr_exec_flat.img,tsr_exec_flat)
	$(call tsr-exec-boot,$(BUILD)/tsr_exec_overlap.img,tsr_exec_overlap)
	@set -eu; for m in fixed flat overlap; do \
	 s=$(BUILD)/tsr_exec_$$m; \
	 if sh harness/emu/tsr_exec_check.sh $$s >$$s.check 2>&1; then \
	  printf '!!! test-tsr-exec-mutant FAIL: %s survived\n' $$m; exit 1; fi; \
	 grep -qx 'SK-INSTALLED' $$s.repl; \
	 grep -qx 'Cannot edit .BAK file--rename file' $$s.repl; \
	 grep '^!!! test-tsr-exec RED:' $$s.check; \
	 printf '>>> test-tsr-exec-mutant: %s RED after successful installer\n' $$m; \
	done
	@printf '>>> test-tsr-exec-mutant: green (fixed relocation base, unchecked raw flat image, overlapping MCB all RED)\n'
TEST_EMU_GATES += test-tsr-exec test-tsr-exec-mutant

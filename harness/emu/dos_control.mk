# K12 / initech-j1lu mutation proofs. Each uses the real gate's oracle.
J1LU_KBD_KNOB_ctrl := KBD_MUTATE_NO_CTRL
J1LU_KBD_KNOB_e0 := KBD_MUTATE_E0
J1LU_KBD_KNOB_release := KBD_MUTATE_CTRL_RELEASE_ALL
J1LU_KBD_MUTANTS := $(addprefix $(BUILD)/test_j1lu_kbd_,ctrl e0 release)
$(BUILD)/test_j1lu_kbd_%: $(TEST_KBD_SRC) $(TEST_KBD_DEPS) $(TEST_KBD_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -D$(J1LU_KBD_KNOB_$*) -I$(MILTON_DIR) -Iseed -o $@ $(TEST_KBD_SRC) $(TEST_KBD_DEPS)

J1LU_CON_KNOB_eof := INT21_MUTATE_CON_EOF_IGNORED
J1LU_CON_KNOB_raw := INT21_MUTATE_CON_EXPLICIT_RAW
J1LU_CON_KNOB_abort := INT21_MUTATE_BREAK_RECOVERY
J1LU_CON_MUTANTS := $(addprefix $(BUILD)/test_j1lu_con_,eof raw abort)
$(BUILD)/test_j1lu_con_%: $(TEST_DEVWIRE_SRC) $(TEST_DEVWIRE_DEPS) $(TEST_DEVWIRE_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -D$(J1LU_CON_KNOB_$*) -Ispec -I$(MILTON_DIR) -Iseed -I$(BUILD) -o $@ $(TEST_DEVWIRE_SRC) $(TEST_DEVWIRE_DEPS)

.PHONY: test-j1lu-kbd-mutant test-j1lu-con-mutant test-j1lu-control-emu-mutant
test-j1lu-kbd-mutant: $(J1LU_KBD_MUTANTS)
	@set -e; for m in ctrl e0 release; do \
	  rc=0; $(BUILD)/test_j1lu_kbd_$$m >$(BUILD)/j1lu_kbd_$$m.log 2>&1 || rc=$$?; \
	  test $$rc -eq 1; grep -Eq '^test-kbd-unit: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/j1lu_kbd_$$m.log; \
	  printf '>>> test-j1lu-kbd-mutant: green (%s correctly RED: ' "$$m"; \
	  tail -1 $(BUILD)/j1lu_kbd_$$m.log | tr '\n' ')'; printf '\n'; \
	done
test-j1lu-con-mutant: $(J1LU_CON_MUTANTS)
	@set -e; for m in eof raw abort; do \
	  rc=0; $(BUILD)/test_j1lu_con_$$m >$(BUILD)/j1lu_con_$$m.log 2>&1 || rc=$$?; \
	  test $$rc -eq 1; grep -Eq '^test_devwire: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/j1lu_con_$$m.log; \
	  printf '>>> test-j1lu-con-mutant: green (%s correctly RED: ' "$$m"; \
	  tail -1 $(BUILD)/j1lu_con_$$m.log | tr '\n' ')'; printf '\n'; \
	done

# Same secondary shell objects, one mutated translation or INT 21h object.
$(BUILD)/kbd_j1lu_ctrl.o: $(KERNEL_KBD_C) $(TEST_KBD_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DKBD_MUTATE_NO_CTRL -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/int21_j1lu_%.o: $(KERNEL_INT21_C) $(TEST_DEVWIRE_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -D$(J1LU_CON_KNOB_$*) -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
$(BUILD)/command_j1lu_restore.o: $(KERNEL_COMMAND_C) $(KERNEL_DIR)/command.h $(TEST_DEVWIRE_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DCOMMAND_KERNEL_REPL -DCMD_MUTATE_REDIR_NO_RESTORE -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@

define j1lu-emu-variant
J1LU_OBJS_$(1) := $(filter-out $(2),$(KERNEL_SECONDARY_SHELL_OBJS)) $(3)
$(BUILD)/kernel_shell_j1lu_$(1).elf: $$(J1LU_OBJS_$(1)) $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $$(J1LU_OBJS_$(1))
$(BUILD)/kernel_shell_j1lu_$(1).bin: $(BUILD)/kernel_shell_j1lu_$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@set -e; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS) * 512 )); test $$$$sz -le $$$$max; \
	printf '>>> j1lu headroom ($(1)): loaded %s B, headroom %s B\n' "$$$$sz" "$$$$(( max - sz ))"; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$(( max - sz ))" conv=notrunc status=none
	$$(call kernel-end-guard,$$<,j1lu-$(1))
$(BUILD)/j1lu_$(1).img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_shell_j1lu_$(1).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_shell_j1lu_$(1).bin $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call j1lu-emu-variant,ctrl,$(KERNEL_KBD_OBJ),$(BUILD)/kbd_j1lu_ctrl.o))
$(eval $(call j1lu-emu-variant,restore,$(KERNEL_COMMAND_OBJ),$(BUILD)/command_j1lu_restore.o))
$(foreach m,eof raw abort,$(eval $(call j1lu-emu-variant,$(m),$(KERNEL_INT21_OBJ),$(BUILD)/int21_j1lu_$(m).o)))

test-j1lu-control-emu-mutant: $(HARNESS_BIN) $(addprefix $(BUILD)/j1lu_,ctrl.img eof.img raw.img abort.img restore.img)
	@set -e; for m in ctrl eof raw abort restore; do \
	  for attempt in 1 2; do \
	    log=$(BUILD)/j1lu_emu_$${m}_$${attempt}.log; rc=0; \
	    J1LU_TIMEOUT_MS=25000 sh harness/emu/test_dos_control.sh $(HARNESS_BIN) $(BUILD)/j1lu_$$m.img $(BUILD) j1lu_$${m}_$${attempt} >$$log 2>&1 || rc=$$?; \
	    test $$rc -eq 1; \
	    grep -Fq 'FAIL: prompt did not return before' $$log; \
	    grep -Fq SHELL-READY $(BUILD)/j1lu_$${m}_$${attempt}.serial; \
	    if [ "$$m" = abort ]; then \
	      grep -Fq INT23-BREAK $(BUILD)/j1lu_$${m}_$${attempt}.serial; \
	      grep -Fq 'before abortok' $$log; \
	    elif [ "$$m" = restore ]; then grep -Fq 'before restored' $$log; \
	    else grep -Fq 'before eofok' $$log; fi; \
	    printf '>>> test-j1lu-control-emu-mutant: %s attempt %s correctly RED (booted; ' "$$m" "$$attempt"; \
	    sed -n 's/.*FAIL: / /p' $$log | tr '\n' ')'; printf '\n'; \
	  done; \
	done
	@printf '>>> test-j1lu-control-emu-mutant: green (ctrl/eof/raw/abort/restore each RED twice, isolated QEMU reruns)\n'

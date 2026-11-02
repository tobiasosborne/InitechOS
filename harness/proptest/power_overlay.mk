# Ref: ADR-0014 DEC-14-8/-9(a)/-10 ("shutdown and exit"), Rule 6; bead
# initech-qshpn. Special > Restart / Shut Down / Exit to InitechDOS run in
# section COLD (os/milton/power_cold.c). A kmain mutant whose knob sits in
# that body must perturb the OVERLAID body, not only its resident kmain:
# each tag below rebuilds power_cold_ovl.o with the kmain template's own
# recorded flags (OVL_ACTION_FLAGS_<tag>, Makefile flair-tenants-kmain-
# mutant-rules) and links its kernel's FLAIR.OVL from it; the per-kernel
# identity/CRC packing is unchanged. Unaffected variants share COLD.
#   shutdown_gate_no_flush   KMAIN_MUT_POWER_NO_FLUSH    test-flair-shutdown-mutant
#   shutdown_gate_returns    KMAIN_MUT_SHUTDOWN_RETURNS  test-flair-shutdown-mutant
#   shutdown_gate_no_ask     KMAIN_MUT_POWER_NO_ASK      test-flair-shutdown(-cancel)-mutant
#   no_pulse                 POWER_MUT_NO_PULSE          test-flair-restart-mutant
#   power_silent_cancel      POWER_MUT_SILENT_CANCEL     test-flair-power-notice-mutant
#   exit_no_teardown         KMAIN_MUT_EXIT_NO_TEARDOWN  test-flair-exit-to-dos-mutant
#   command_volume/_magic/_message  KMAIN_MUT_COMMAND_*  test-command-file-emu-mutant
OVL_POWER_TAGS := shutdown_gate_no_flush shutdown_gate_returns shutdown_gate_no_ask no_pulse power_silent_cancel exit_no_teardown command_volume command_magic command_message
$(foreach t,$(OVL_POWER_TAGS),$(eval $(call finder-ovl-object,power,$(t),os/milton/power_cold.c,$(POWER_COLD_DEPS),$(OVL_ACTION_FLAGS_$(t)))))
$(foreach t,$(OVL_POWER_TAGS),$(eval $(call finder-ovl-kernel,kernel_flairtenants_mut_$(t),$(filter-out $(BUILD)/power_cold_ovl.o,$(OVL_SECTION_OBJS)) $(BUILD)/finder_power_ovl_$(t).o)))

# DEC-14-9(g), Rule 6: independently prove the absence check catches a
# resident copy of the power/exit body. Link the flagship with power_cold.c
# compiled INTO the kernel (entry points renamed, kept by -u): the check must
# name a moved symbol or the export as resident.
$(OVL_GATE_DIR)/power_cold_kept.o: $(POWER_COLD_DEPS) | $(OVL_GATE_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(OVL_SECTION_OPT) $(FINDER_OVL_INC) -Dfinder_power=finder_power_kept -Dfinder_exit_dos=finder_exit_dos_kept -c $< -o $@
.PHONY: test-ovl-absence-power-mutant
test-ovl-absence-power-mutant: test-ovl-absence $(OVL_GATE_DIR)/power_cold_kept.o
	$(KERNEL_LINK) -u finder_power_kept -u finder_exit_dos_kept -o $(OVL_GATE_DIR)/power_kept.elf $(KERNEL_FLAIRTENANTS_OBJS) $(OVL_GATE_DIR)/power_cold_kept.o @$(TRANSIENT_IMPORTS_LD)
	@if ( $(call ovl-absence-check,$(OVL_GATE_DIR)/power_kept.elf) ) > $(OVL_GATE_DIR)/power_kept.log 2>&1; then printf '!!! test-ovl-absence-power-mutant FAIL: a resident power/exit body survived\n'; exit 1; fi
	@grep -E '!!! ovl-absence FAIL: (export finder_(power|exit_dos) resident [12] times|moved symbol (finder_shutdown_screen|finder_power_ask_apps|flair_exit_to_dos|exitdos_fail) still resident)' $(OVL_GATE_DIR)/power_kept.log
	@printf '>>> test-ovl-absence-power-mutant: green (resident power/exit body RED)\n'
test-ovl-absence-mutant: test-ovl-absence-power-mutant
TEST_UNIT_GATES += test-ovl-absence-power-mutant

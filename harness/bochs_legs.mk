# harness/bochs_legs.mk -- generic BOCHS boot legs for FLAIR gate kernels (Rule 5,
# PRD Sec 8; beads initech-cqej). Each named QEMU gate gets an explicit leg;
# their boot differentials may intentionally exercise the same kernel image.
# Same TRUTHFUL CONSTRAINT as test-flair-appswitch-bochs: Bochs 2.7 ENOMODEs the
# 640x480 VBE mode, the desktop guard fails loud and HALTS before any tenant, so the
# leg proves the SAME kernel runs the boot chain + FLAIR heap + FAT mount identically
# and the guard fires (no triple fault). SKIP_BOCHS=1 is the one documented opt-out.
# $(call bochs-flair-leg,<gate name>,<kernel image>,<data image>,<profile>)
# shell selects the DOS gate's actual boot boundary instead of the FLAIR
# guard. These are BOOT legs, just like the pre-existing desktop/appswitch
# legs: no claim of Bochs input replay or pixel coverage is made here.
# exec/mzexec select those self-test kernels' real completion markers;
# vga13 also checks the actual indexed handoff; nodrive omits the slave.
# Ref: os/milton/kmain.c BOOT_EXEC/BOOT_MZEXEC/BOOT_SHELL and VIDEO record;
# IBM DOS 3.30 TR p.1-7 (PDF24, system-file boot chain); ADR-0003 Appendix A.
BOCHS_LEG_END_flair := HALTED
BOCHS_LEG_END_vga13 := HALTED
BOCHS_LEG_END_shell := SHELL-READY
BOCHS_LEG_END_nodrive := SHELL-READY
BOCHS_LEG_END_exec := EXEC-4B-END
BOCHS_LEG_END_mzexec := MZEXEC-END
BOCHS_LEG_MARKERS_flair := FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK HALTED
BOCHS_LEG_MARKERS_vga13 := $(BOCHS_LEG_MARKERS_flair)
BOCHS_LEG_MARKERS_shell := IRQ-LIVE CONIN-LIVE SHELL-READY
BOCHS_LEG_MARKERS_nodrive := $(BOCHS_LEG_MARKERS_shell)
BOCHS_LEG_MARKERS_exec := IRQ-LIVE CONIN-LIVE EXEC-4B-END
BOCHS_LEG_MARKERS_mzexec := IRQ-LIVE CONIN-LIVE MZEXEC-END

# Independent inventory/gate wiring oracle FIRST, cqej second half.
$(BUILD)/test_bochs_legs: harness/emu/test_bochs_legs.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-bochs-legs test-bochs-legs-mutant
test-bochs-legs: $(BUILD)/test_bochs_legs
	@sh harness/emu/test_bochs_legs.sh $(BUILD) green
test-bochs-legs-mutant: $(BUILD)/test_bochs_legs
	@sh harness/emu/test_bochs_legs.sh $(BUILD) mutants
TEST_UNIT_GATES += test-bochs-legs test-bochs-legs-mutant

define bochs-flair-leg
$(if $(BOCHS_LEG_END_$(4)),,$(error unknown Bochs leg profile $(4) for $(1)))
.PHONY: test-$(1)-bochs
ifeq ($$(SKIP_BOCHS),1)
test-$(1)-bochs:
	@printf '!!! test-$(1)-bochs SKIPPED (SKIP_BOCHS=1 opt-out) -- the Bochs leg (Rule 5) was NOT run. This is a LOUD, explicit opt-out, not a pass -- unset SKIP_BOCHS and re-run before trusting this gate.\n'
else
test-$(1)-bochs: $$(BOCHS_BIN) $(2) $(3)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-$(1)-bochs : BOCHS boot leg of %s\n' "$(2)"
	@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; bead initech-cqej.\n'
	@printf '======================================================================\n'
	@command -v $$(BOCHS) >/dev/null 2>&1 || { printf '!!! test-$(1)-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
	$(if $(strip $(3)),cp -f $(3) $$(BUILD)/$(1)_bochs_data.img,@true)
	@$$(BOCHS_BIN) --disk "$(2)" $(if $(strip $(3)),--disk2 "$$(BUILD)/$(1)_bochs_data.img") --expect $(BOCHS_LEG_END_$(4)) \
		--name "$(1)_bochs" --out "$$(BUILD)" --timeout-ms 45000 \
		2> "$$(BUILD)/$(1)_bochs.report.txt" || true
	@cat "$$(BUILD)/$(1)_bochs.report.txt"
	@grep -q 'rfb_unblocked=1' "$$(BUILD)/$(1)_bochs.report.txt" \
		|| { printf '!!! test-$(1)-bochs FAIL: RFB unblock failed -- Bochs did not run the guest\n'; exit 1; }
	@if grep -q 'triple_fault=1' "$$(BUILD)/$(1)_bochs.report.txt"; then \
		printf '!!! test-$(1)-bochs FAIL: TRIPLE FAULT under Bochs\n'; exit 1; \
	fi
	@if [ ! -s "$$(BUILD)/$(1)_bochs.serial" ]; then \
		printf '!!! test-$(1)-bochs FAIL: no serial captured\n'; exit 1; \
	fi
	@for m in VBE-ENOMODE VGA13 S1 PM KERNEL CONSOLE BANNER $(BOCHS_LEG_MARKERS_$(4)); do \
		grep -q "^$$$$m$$$$" "$$(BUILD)/$(1)_bochs.serial" \
			|| { printf '!!! test-$(1)-bochs FAIL: marker %s missing\n' "$$$$m"; exit 1; }; \
	done
	@$(if $(filter flair vga13,$(4)),grep -qxF 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' "$$(BUILD)/$(1)_bochs.serial",! grep -q '^PANIC' "$$(BUILD)/$(1)_bochs.serial") \
		|| { printf '!!! test-$(1)-bochs FAIL: $(if $(filter flair vga13,$(4)),the 640x480 guard did not fire,PANIC on the DOS path)\n'; exit 1; }
$(if $(filter vga13,$(4)),	@grep -qxF 'VIDEO 320x200 bpp=8 pitch=320 lfb=0x000A0000 dac=6' "$$(BUILD)/$(1)_bochs.serial" || { printf '!!! test-$(1)-bochs FAIL: expected six-bit VGA13 handoff\n'; exit 1; })
$(if $(filter nodrive,$(4)),	@grep -qxF 'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0' "$$(BUILD)/$(1)_bochs.serial" || { printf '!!! test-$(1)-bochs FAIL: missing slave must report READ failure\n'; exit 1; })
	@printf '>>> test-$(1)-bochs: green (Bochs BOOT ONLY: fallback + shared milestones + $(if $(filter flair vga13,$(4)),640x480 guard,$(BOCHS_LEG_END_$(4))); no triple fault)\n'
endif
TEST_EMU_GATES += test-$(1)-bochs
endef
$(eval $(call bochs-flair-leg,flair-menubar-clock,$(BUILD)/flair_tenants_mut_menubar_clock_gate.img,$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-iword,$(BUILD)/flair_tenants_mut_iword_gate.img,$(FLAIR_DATA_IMG),flair))

# QEMU-green kernels listed by the operator in initech-cqej. Several gates
# intentionally share a kernel: retain every requested gate name in the
# vector so omissions cannot hide behind another gate's result (Rule 5).
$(eval $(call bochs-flair-leg,flair-exit-to-dos,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-tps-journey,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
# initech-tdnl.106 (ADR-0003 DEC-19): on Bochs the flagship boots to
# COMMAND.COM, AUTOEXEC.BAT runs FLAIR, INT 81h arms the desktop and the
# 640x480 guard halts it -- the same flair profile. Written, not run, by the
# lane (the PM runs the Bochs legs).
$(eval $(call bochs-flair-leg,flair-from-dos,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-file-copy,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-paint,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-itext,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-itext-scroll,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-calc,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-calcda,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-apple-menu,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,dos-break-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,dos-not-ready-emu,$(BUILD)/dosmsg.img,$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,chkdsk-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,xcopy-tree-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,dosutil-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,edlin-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,flair-live-scene,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,flair-shutdown,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_POWER_CANCEL_IMG),flair))
$(eval $(call bochs-flair-leg,flair-restart,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))

# Tonight's merged lane kernels, cqej second half. Preserve every QEMU gate
# name even when several gates share a boot image (WL-0131 Bochs row).
# Command negative fixtures differ AFTER the desktop guard; this boot leg
# deliberately makes no claim about the missing/magic error UI.
$(eval $(call bochs-flair-leg,command-file-emu,$(COMMAND_FILE_IMG)/fragment.img,$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,command-file-negative,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
# FORCE_VBE_101 still negotiates through the real BIOS. Bochs rejects LFB
# and falls back to mode 13h. Check VIDEO, not the QEMU 640x480 RGB golden;
# the guard prevents the desktop presenter/DAC mutation cases from running.
$(eval $(call bochs-flair-leg,flair-desktop-8bpp,$(VESA8_DIR)/flagship.img,$(FLAIR_DATA_IMG),vga13))

# WL-0143: all desk accessory traces use the InitechText gate kernel.
$(foreach gate,flair-notepad flair-scrapbook flair-notepad-itext flair-notepad-restart,$(eval $(call bochs-flair-leg,$(gate),$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair)))
$(eval $(call bochs-flair-leg,flair-notepad-corrupt,$(FLAIR_ITEXT_GATE_IMG),$(DA_CORRUPT_DATA),flair))
# WL-0145: chooser, five panels, settings, restart and corrupt-record boot.
$(foreach gate,flair-cpanels flair-cpanels-date flair-cpanels-patterns flair-cpanels-mouse flair-cpanels-sound flair-cpanels-memory flair-cpanels-settings flair-cpanels-persist,$(eval $(call bochs-flair-leg,$(gate),$(CP_GATE_IMG),$(FLAIR_DATA_IMG),flair)))
$(eval $(call bochs-flair-leg,flair-cpanels-corrupt,$(CP_GATE_IMG),$(BUILD)/cp_corrupt_data.img,flair))
$(eval $(call bochs-flair-leg,flair-grow-box,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))

# WL-0141/0147: physical I/O, FDISK and FORMAT/SYS use the shell kernel.
# /S generated-volume boot, editor input and disk mutation remain QEMU
# acceptance; these macro legs certify only their shared kernel boot chain.
# test-diskcopy-emu has NO leg: it is QEMU-RED (fixed ATA medium cannot swap),
# not one of the QEMU-green gates in scope (WL-0147, Named incomplete leg).
$(foreach gate,block-device-emu fdisk-emu formatsys-emu format-system-boot,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell)))
$(eval $(call bochs-flair-leg,dos-folder,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(foreach gate,conkeys j1lu-control-emu,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell)))
$(eval $(call bochs-flair-leg,ansi-emu,$(SECONDARY_TRACER_IMG),$(BUILD)/ansi_probe_data.img,shell))
$(foreach gate,flair-finder-icons flair-trash-full flair-toolbox-hardening,$(eval $(call bochs-flair-leg,$(gate),$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair)))

# Kernel hygiene: preserve self-test kernel identity and completion markers.
# MZ invalid-entry stimulus and baked BUSY semantics remain the QEMU oracle;
# the Bochs macro certifies the real self-test image's boot/completion only.
$(eval $(call bochs-flair-leg,fat-hygiene-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,trash-hidden-emu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))
$(eval $(call bochs-flair-leg,ata-no-drive-emu,$(SECONDARY_TRACER_IMG),,nodrive))
$(eval $(call bochs-flair-leg,baked-exec-guard,$(BUILD)/baked_guard.img,$(FAT_EXEC_IMG),exec))
$(eval $(call bochs-flair-leg,mzload-entry-emu,$(MZEXEC_IMG),$(FAT_MZEXEC_IMG),mzexec))

# WL-0148 and the SAMIR DML lane: 16 distinct named leaves + aggregates.
# No interpreter/FAT mutation parity is claimed without input replay.
$(eval $(call bochs-flair-leg,samir-core-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(eval $(call bochs-flair-leg,samir-dml-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))
$(foreach verb,$(SAMIR_DML_CASES),$(eval $(call bochs-flair-leg,samir-dml-$(verb)-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell)))
$(eval $(call bochs-flair-leg,samir-print-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell))

# WL-0154/0155: unchanged DOS shell image; host mtools/dosfstools and QEMU
# injected traces still grade COPY/REN, nested COMMAND /C, XCOPY CWD and
# AH40 CX=0 truncation (IBM DOS 3.30 TR p.6-140/PDF263; ADR-0003 App A).
$(foreach gate,shell-files-emu command-c-emu xcopy-cwd-emu fat-truncate-emu dir-match-emu shell-dir-look,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell)))
# WL-0146 and WL-0152: every additional IWORD trace and the size wrapper.
$(foreach gate,flair-iword-codes flair-iword-block flair-iword-format flair-iword-fkeys flair-iword-search flair-iword-replace flair-iword-undo flair-iword-setup,$(eval $(call bochs-flair-leg,$(gate),$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),flair)))
$(eval $(call bochs-flair-leg,kernel-size-r4-emu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair))

# Factory oracle FIRST (Rule 1). The same second-boot parser is used by the
# live driver. The mutant cuts its wait to the first boot (Rule 6).
$(BUILD)/test_bochs_completion: harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/emu $< -o $@
$(BUILD)/test_bochs_completion_mut: harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h | $(BUILD)
	$(CC) $(CFLAGS) -DBOCHS_MUTATE_NO_REBOOT_WAIT -Iharness/emu $< -o $@
.PHONY: test-bochs-completion test-bochs-completion-mutant
test-bochs-completion: $(BUILD)/test_bochs_completion
	@$<
test-bochs-completion-mutant: $(BUILD)/test_bochs_completion_mut
	@if $< > $(BUILD)/bochs_completion_mut.log 2>&1; then echo 'FAIL BOCHS_MUTATE_NO_REBOOT_WAIT survived'; exit 1; fi
	@grep -qxF 'RED bochs-completion: one boot must not satisfy a two-boot wait' $(BUILD)/bochs_completion_mut.log
	@printf 'GREEN test-bochs-completion-mutant: BOCHS_MUTATE_NO_REBOOT_WAIT caught: %s\n' "$$(cat $(BUILD)/bochs_completion_mut.log)"
TEST_UNIT_GATES += test-bochs-completion test-bochs-completion-mutant

# Exactness and complete-line requirements each have their own named mutant.
define bochs-completion-parser-mutant
$(BUILD)/test_bochs_completion_$(1): harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h | $(BUILD)
	$(CC) $(CFLAGS) -DBOCHS_MUTATE_$(2) -Iharness/emu $$< -o $$@
.PHONY: test-bochs-completion-$(1)-mutant
test-bochs-completion-$(1)-mutant: $(BUILD)/test_bochs_completion_$(1)
	@if $$< > $(BUILD)/bochs_completion_$(1).log 2>&1; then echo 'FAIL BOCHS_MUTATE_$(2) survived'; exit 1; fi
	@grep -qxF 'RED bochs-completion: $(3)' $(BUILD)/bochs_completion_$(1).log
	@printf 'GREEN test-bochs-completion-$(1)-mutant: BOCHS_MUTATE_$(2) caught: %s\n' "$$$$(cat $(BUILD)/bochs_completion_$(1).log)"
TEST_UNIT_GATES += test-bochs-completion-$(1)-mutant
endef
$(eval $(call bochs-completion-parser-mutant,substring,MARKER_SUBSTRING,marker substrings must not count as boot lines))
$(eval $(call bochs-completion-parser-mutant,partial,PARTIAL_LINE,partial serial lines must not count as boot lines))

$(BUILD)/bochs_reboot.boot: harness/emu/fixtures/bochs_reboot.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/bochs_reboot.img: $(BUILD)/bochs_reboot.boot
	dd if=/dev/zero of=$@ bs=512 count=128 status=none
	dd if=$< of=$@ conv=notrunc status=none
$(BUILD)/bochs_no_reboot.boot: harness/emu/fixtures/bochs_reboot.asm | $(BUILD)
	$(NASM) -f bin -DBOCHS_FIXTURE_NO_RESET $< -o $@
$(BUILD)/bochs_no_reboot.img: $(BUILD)/bochs_no_reboot.boot
	dd if=/dev/zero of=$@ bs=512 count=128 status=none
	dd if=$< of=$@ conv=notrunc status=none
$(BUILD)/bochs_harness_mut_nowait: $(BOCHS_DRV_SRC) $(BOCHS_LIB_SRC) harness/emu/bochs.h | $(BUILD)
	$(CC) $(CFLAGS) -DBOCHS_MUTATE_NO_REBOOT_WAIT -Iharness/emu -o $@ $(BOCHS_DRV_SRC) $(BOCHS_LIB_SRC)

# Always copy: the fixture records its first boot in sector 2. Require the
# complete second-boot DONE line, then independently count the READY lines.
define bochs-reboot-run
cp -f $(BUILD)/bochs_reboot.img $(BUILD)/$(2).img
$(1) --disk $(BUILD)/$(2).img --name $(2) --out $(BUILD) \
 --expect BOCHS-REBOOT-READY --allow-reboot --quit-after BOCHS-REBOOT-READY \
 --quit-after-count 2 --timeout-ms 90000 2> $(BUILD)/$(2).report || true
endef
.PHONY: test-bochs-reboot test-bochs-reboot-mutant
ifeq ($(SKIP_BOCHS),1)
test-bochs-reboot test-bochs-reboot-mutant:
	@printf '!!! %s SKIPPED (SKIP_BOCHS=1 opt-out): reboot wait NOT run\n' "$@"
else
test-bochs-reboot: $(BOCHS_BIN) $(BUILD)/bochs_reboot.img
	$(call bochs-reboot-run,$(BOCHS_BIN),bochs_reboot_green)
	@cat $(BUILD)/bochs_reboot_green.report
	@grep -q 'OK=1' $(BUILD)/bochs_reboot_green.report || { echo 'RED bochs-reboot: harness did not complete'; exit 1; }
	@sh harness/emu/bochs_reboot_check.sh $(BUILD)/bochs_reboot_green.serial
# Negative stimulus never resets: rejecting one READY is deterministic even
# on a loaded host. A no-wait driver falsely reports OK; the SAME independent
# serial checker used by the positive reset fixture must reject it.
test-bochs-reboot-mutant: $(BUILD)/bochs_harness_mut_nowait $(BUILD)/bochs_no_reboot.img
	cp -f $(BUILD)/bochs_no_reboot.img $(BUILD)/bochs_reboot_nowait.img
	@$(BUILD)/bochs_harness_mut_nowait --disk $(BUILD)/bochs_reboot_nowait.img --name bochs_reboot_nowait --out $(BUILD) \
	 --expect BOCHS-REBOOT-READY --allow-reboot --quit-after BOCHS-REBOOT-READY \
	 --quit-after-count 2 --timeout-ms 90000 2> $(BUILD)/bochs_reboot_nowait.report || true
	@grep -q 'OK=1' $(BUILD)/bochs_reboot_nowait.report || { cat $(BUILD)/bochs_reboot_nowait.report; echo 'FAIL no-wait mutant did not run'; exit 1; }
	@if sh harness/emu/bochs_reboot_check.sh $(BUILD)/bochs_reboot_nowait.serial > $(BUILD)/bochs_reboot_nowait.check 2>&1; then echo 'FAIL BOCHS_MUTATE_NO_REBOOT_WAIT survived Bochs'; exit 1; fi
	@grep -qxF 'RED bochs-reboot: READY seen 1 times, want 2 (must wait for reboot)' $(BUILD)/bochs_reboot_nowait.check
	@printf 'GREEN test-bochs-reboot-mutant: BOCHS_MUTATE_NO_REBOOT_WAIT caught: %s\n' "$$(cat $(BUILD)/bochs_reboot_nowait.check)"
endif
test-flair-restart-bochs: test-bochs-reboot
TEST_EMU_GATES += test-bochs-reboot test-bochs-reboot-mutant
TEST_EMU_GATES += test-flair-shutdown-cancel test-flair-shutdown-cancel-mutant

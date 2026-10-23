# harness/bochs_legs.mk -- generic BOCHS boot legs for FLAIR gate kernels (Rule 5,
# PRD Sec 8; beads initech-cqej), re-keyed by WL-0170 to the video path Bochs
# 2.7 REALLY takes. Each named QEMU gate gets an explicit leg; their boot
# differentials may intentionally exercise the same kernel image.
#
# VIDEO PATHS (harness/emu/bochs_leg_check.sh is the ONE serial verdict):
#   vbe101     the stock Bochs 2.7 LGPL vgabios offers VESA 0101h 640x480x8 with
#              an LFB at 0xE0000000 (vga.h VBE_DISPI_LFB_PHYSICAL_ADDRESS); since
#              initech-2gva IO.SYS takes it (VBE-MODE=0101, 8-bit DAC) and the
#              desktop PAINTS: WL-0170 graded the Bochs framebuffer against the
#              QEMU 32bpp golden EXACT on all 307200 pixels (bochs_pixel_leg.sh).
#              The pre-WL-0170 "Bochs ENOMODEs -> mode 13h -> the 640x480 guard"
#              claim is dead for the stock ROM; it was written before vesa8.
#   vbe101pad  vbe101 on the FORCE_VBE_101 gate IO.SYS (vesa8.mk): its 4F06h call
#              pads the logical scanline to 648 pixels, graded on the VIDEO line.
#   vga13      ISA Cirrus CL-GD54xx (vga: extension=cirrus, the vgabios package's
#              cirrus ROM -- bochs Depends: vgabios | seabios): VBE 1.2 offers
#              0101h WITHOUT an LFB -> VBE-ENOMODE -> the mode-13h floor + the
#              640x480 guard. A real period card; keeps ENOMODE + VGA13 graded.
#   e00        plain VGA, no VBE at all (vga: extension=none, the stock ROM's
#              4F00h fails) -> VBE-E00 -> mode 13h. The 1990 VGA floor.
# These are BOOT legs (serial): no claim of Bochs input replay is made here.
# The PIXEL legs (bochs-pixel-leg below; harness/emu/bochs_pixel_leg.sh +
# tools/ppm_bochs_check.c) grade the Bochs frame against a QEMU capture of the
# SAME scene: EXACT RGB on Bochs's own snapshot and BGR233 on the RFB display.
# Ref: os/milton/kmain.c BOOT_EXEC/BOOT_MZEXEC/BOOT_SHELL and VIDEO record;
# IBM DOS 3.30 TR p.1-7 (PDF24, system-file boot chain); ADR-0003 Appendix A.
BOCHS_CIRRUS_VGABIOS ?= /usr/share/vgabios/vgabios.cirrus.bin
BOCHS_LEG_VIDEO_vbe101 :=
BOCHS_LEG_VIDEO_vbe101pad :=
BOCHS_LEG_VIDEO_vga13 := --vga-ext cirrus --vgabios $(BOCHS_CIRRUS_VGABIOS)
BOCHS_LEG_VIDEO_e00 := --vga-ext none
# Profiles: the completion line (--expect + --quit-after) and the leg checks
# (complete exact serial lines; !PANIC = no PANIC line at all). On the 0101h
# path every BOOT_FLAIR_LIVE kernel presents the desktop (FLAIR-DESKTOP) and
# arms the pump (FLAIR-LIVE-READY); under mode 13h the 640x480 guard fails
# loud instead (guard / guardmount), which is the vga13/e00 leg's tooth.
BOCHS_LEG_END_flair := FLAIR-LIVE-READY
BOCHS_LEG_END_guard := HALTED
BOCHS_LEG_END_guardmount := HALTED
BOCHS_LEG_END_shell := SHELL-READY
BOCHS_LEG_END_nodrive := SHELL-READY
BOCHS_LEG_END_exec := EXEC-4B-END
BOCHS_LEG_END_mzexec := MZEXEC-END
BOCHS_LEG_MARKERS_flair := FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
# flairdb: the flair profile on a PRISTINE data volume (no DESKTOP.DB, no
# \TRASH): the Finder's bootstrap lines must also be present (the pre-WL-0170
# file-ops / trash-window legs graded them; WL-0162 keeps that tooth).
BOCHS_LEG_END_flairdb := FLAIR-LIVE-READY
BOCHS_LEG_MARKERS_flairdb := FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK DESKTOP-DB-CREATE TRASH-CREATE FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
BOCHS_LEG_MARKERS_guard := FLAIR-HEAP-OK 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED
BOCHS_LEG_MARKERS_guardmount := FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK 'PANIC flair-desktop: LFB smaller than 640x480 w=320 h=200' HALTED
BOCHS_LEG_MARKERS_shell := IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
BOCHS_LEG_MARKERS_nodrive := 'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0' IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
BOCHS_LEG_MARKERS_exec := IRQ-LIVE CONIN-LIVE EXEC-4B-END !PANIC
BOCHS_LEG_MARKERS_mzexec := IRQ-LIVE CONIN-LIVE MZEXEC-END !PANIC

# Independent inventory/gate wiring oracle FIRST, cqej second half.
$(BUILD)/test_bochs_legs: harness/emu/test_bochs_legs.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
.PHONY: test-bochs-legs test-bochs-legs-mutant
test-bochs-legs: $(BUILD)/test_bochs_legs
	@sh harness/emu/test_bochs_legs.sh $(BUILD) green
test-bochs-legs-mutant: $(BUILD)/test_bochs_legs
	@sh harness/emu/test_bochs_legs.sh $(BUILD) mutants
TEST_UNIT_GATES += test-bochs-legs test-bochs-legs-mutant

# $(call bochs-flair-leg,<gate name>,<kernel image>,<data image>,<profile>[,<video path>])
define bochs-flair-leg
$(if $(BOCHS_LEG_END_$(4)),,$(error unknown Bochs leg profile $(4) for $(1)))
$(if $(filter $(or $(5),vbe101),vbe101 vbe101pad vga13 e00),,$(error unknown Bochs video path "$(5)" for $(1)))
.PHONY: test-$(1)-bochs
ifeq ($$(SKIP_BOCHS),1)
test-$(1)-bochs:
	@printf '!!! test-$(1)-bochs SKIPPED (SKIP_BOCHS=1 opt-out) -- the Bochs leg (Rule 5) was NOT run. This is a LOUD, explicit opt-out, not a pass -- unset SKIP_BOCHS and re-run before trusting this gate.\n'
else
test-$(1)-bochs: $$(BOCHS_BIN) $(2) $(3)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-$(1)-bochs : BOCHS boot leg of %s (%s video path)\n' "$(2)" "$(or $(5),vbe101)"
	@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
	@printf '======================================================================\n'
	@command -v $$(BOCHS) >/dev/null 2>&1 || { printf '!!! test-$(1)-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
	$(if $(filter vga13,$(5)),@test -r $(BOCHS_CIRRUS_VGABIOS) || { printf '!!! test-$(1)-bochs FAIL: ISA Cirrus VGA BIOS %s missing (apt install vgabios; bochs Depends: vgabios | seabios). A skipped oracle is worse than a red one (Law 2).\n' "$(BOCHS_CIRRUS_VGABIOS)"; exit 1; },@true)
	$(if $(strip $(3)),cp -f $(3) $$(BUILD)/$(1)_bochs_data.img,@true)
	@$$(BOCHS_BIN) --disk "$(2)" $(if $(strip $(3)),--disk2 "$$(BUILD)/$(1)_bochs_data.img") $(BOCHS_LEG_VIDEO_$(or $(5),vbe101)) \
		--expect $(BOCHS_LEG_END_$(4)) --quit-after $(BOCHS_LEG_END_$(4)) \
		--name "$(1)_bochs" --out "$$(BUILD)" --timeout-ms 45000 \
		2> "$$(BUILD)/$(1)_bochs.report.txt" || true
	@cat "$$(BUILD)/$(1)_bochs.report.txt"
	@sh harness/emu/bochs_leg_check.sh test-$(1)-bochs "$$(BUILD)/$(1)_bochs.report.txt" "$$(BUILD)/$(1)_bochs.serial" $(or $(5),vbe101) $(BOCHS_LEG_MARKERS_$(4))
endif
TEST_EMU_GATES += test-$(1)-bochs
endef

# The PIXEL leg: $(call bochs-pixel-leg,<tag>,<bochs image>,<golden image>,<data|->,<capture line>)
# QEMU boots the golden image (32bpp VBE on QEMU: an independent emulator AND
# an independent depth path), Bochs boots the leg image (0101h, 8-bit DAC);
# both RTC-pinned to 1993-03-01T09:00:00 UTC and captured at the same complete
# serial line. ppm_bochs_check grades Bochs's own snapshot EXACT and the RFB
# display through BGR233 -- never a tolerance (Law 2; CLAUDE.md stop conditions).
$(BUILD)/ppm_bochs_check: tools/ppm_bochs_check.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@
define bochs-pixel-leg
sh harness/emu/bochs_pixel_leg.sh $(BUILD) $(1) $(2) $(3) $(4) '$(5)'
endef
BOCHS_PIXEL_TENANTS_LINE := FLAIR-TENANTS-READY hello+notes co-resident; HELLO foreground
BOCHS_PIXEL_DEPS := $(BOCHS_BIN) $(HARNESS_BIN) $(BUILD)/ppm_bochs_check

$(eval $(call bochs-flair-leg,flair-menubar-clock,$(BUILD)/flair_tenants_mut_menubar_clock_gate.img,$(FLAIR_DATA_IMG),flair,vbe101))
$(eval $(call bochs-flair-leg,flair-iword,$(BUILD)/flair_tenants_mut_iword_gate.img,$(FLAIR_DATA_IMG),flair,vbe101))

# QEMU-green kernels listed by the operator in initech-cqej. Several gates
# intentionally share a kernel: retain every requested gate name in the
# vector so omissions cannot hide behind another gate's result (Rule 5).
$(eval $(call bochs-flair-leg,flair-exit-to-dos,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-tps-journey,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-file-copy,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-paint,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-itext,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-itext-scroll,$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-calc,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-calcda,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-apple-menu,$(FLAIR_CALCDA_GATE_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,dos-break-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,dos-not-ready-emu,$(BUILD)/dosmsg.img,$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,chkdsk-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,xcopy-tree-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,dosutil-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,edlin-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,flair-live-scene,$(FLAIRLIVE_INTERACTIVE_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-shutdown,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_POWER_CANCEL_IMG),flair,))
$(eval $(call bochs-flair-leg,flair-restart,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))

# Tonight's merged lane kernels, cqej second half. Preserve every QEMU gate
# name even when several gates share a boot image (WL-0131 Bochs row).
# Command negative fixtures differ AFTER the desktop guard; this boot leg
# deliberately makes no claim about the missing/magic error UI.
$(eval $(call bochs-flair-leg,command-file-emu,$(COMMAND_FILE_IMG)/fragment.img,$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,command-file-negative,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))

# WL-0170: the VESA8 flagship (FORCE_VBE_101 + the 648-pixel 4F06h padding)
# under Bochs: the boot leg grades the padded VIDEO handoff; the pixel leg
# grades the presented desktop against the QEMU 32bpp capture of the SHIPPED
# flagship image at the tenants-ready line. The mutant twin boots the four
# FLAIR_DAC_MUT_* kernels (vesa8.mk; five since size round 7) under Bochs against the CLEAN golden: a
# Bochs grade that cannot go RED on a wrong DAC/row copy is decoration (Rule 6).
$(eval $(call bochs-flair-leg,flair-desktop-8bpp,$(VESA8_DIR)/flagship.img,$(FLAIR_DATA_IMG),flair,vbe101pad))
.PHONY: test-flair-desktop-8bpp-bochs-pixel test-flair-desktop-8bpp-bochs-mutant
ifeq ($(SKIP_BOCHS),1)
test-flair-desktop-8bpp-bochs-pixel test-flair-desktop-8bpp-bochs-mutant:
	@printf '!!! %s SKIPPED (SKIP_BOCHS=1 opt-out): the Bochs pixel grade was NOT run\n' "$@"
else
test-flair-desktop-8bpp-bochs-pixel: $(BOCHS_PIXEL_DEPS) $(VESA8_DIR)/flagship.img $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG)
	@$(call bochs-pixel-leg,flair_desktop_8bpp,$(VESA8_DIR)/flagship.img,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),$(BOCHS_PIXEL_TENANTS_LINE))
	@sh harness/emu/bochs_leg_check.sh test-flair-desktop-8bpp-bochs-pixel $(BUILD)/bochs-pixel/flair_desktop_8bpp-bochs.report $(BUILD)/bochs-pixel/flair_desktop_8bpp-bochs.serial vbe101pad FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP '$(BOCHS_PIXEL_TENANTS_LINE)' !PANIC
test-flair-desktop-8bpp-bochs-mutant: test-flair-desktop-8bpp-bochs-pixel $(foreach m,$(VESA8_MUTANTS),$(VESA8_DIR)/mut_$(m).img)
	@set -eu; for m in $(VESA8_MUTANTS); do \
	 if $(call bochs-pixel-leg,bochs_mut_$$m,$(VESA8_DIR)/mut_$$m.img,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),$(BOCHS_PIXEL_TENANTS_LINE)) > $(BUILD)/bochs-pixel/mut_$$m.red 2>&1; then echo "FAIL $$m survived under Bochs"; cat $(BUILD)/bochs-pixel/mut_$$m.red; exit 1; fi; \
	 grep -q "^RED bochs-pixel bochs_mut_$$m:" $(BUILD)/bochs-pixel/mut_$$m.red || { cat $(BUILD)/bochs-pixel/mut_$$m.red; echo "FAIL $$m: capture broken, not a pixel RED"; exit 1; }; \
	 printf '%s: ' "$$m"; grep -m1 '^RED bochs-exact RGB ' $(BUILD)/bochs-pixel/mut_$$m.red; done
	@echo 'GREEN test-flair-desktop-8bpp-bochs-mutant: omissions 119/165, row copy, DAC precision and channel order each RED under Bochs'
endif
test-flair-desktop-8bpp-bochs: test-flair-desktop-8bpp-bochs-pixel
TEST_EMU_GATES += test-flair-desktop-8bpp-bochs-mutant

# WL-0170: the FORCED mode-13h legs. The stock ROM takes 0101h, so the VGA13
# floor and the VBE-ENOMODE / VBE-E00 fallbacks are graded on two real period
# video configurations: an ISA Cirrus (VBE without LFB) and a plain VGA.
$(eval $(call bochs-flair-leg,boot-vga13,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,vga13))
$(eval $(call bochs-flair-leg,boot-novbe,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,e00))
$(eval $(call bochs-flair-leg,flair-desktop-vga13,$(FLAIRSHELL_IMG),,guard,vga13))
$(eval $(call bochs-flair-leg,flair-appswitch-vga13,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),guardmount,vga13))
$(eval $(call bochs-flair-leg,flair-desktop-novbe,$(FLAIRSHELL_IMG),,guard,e00))

# WL-0143: all desk accessory traces use the InitechText gate kernel.
$(foreach gate,flair-notepad flair-scrapbook flair-notepad-itext flair-notepad-restart,$(eval $(call bochs-flair-leg,$(gate),$(FLAIR_ITEXT_GATE_IMG),$(FLAIR_DATA_IMG),flair,)))
$(eval $(call bochs-flair-leg,flair-notepad-corrupt,$(FLAIR_ITEXT_GATE_IMG),$(DA_CORRUPT_DATA),flair,))
# WL-0145: chooser, five panels, settings, restart and corrupt-record boot.
$(foreach gate,flair-cpanels flair-cpanels-date flair-cpanels-patterns flair-cpanels-mouse flair-cpanels-sound flair-cpanels-memory flair-cpanels-settings flair-cpanels-persist,$(eval $(call bochs-flair-leg,$(gate),$(CP_GATE_IMG),$(FLAIR_DATA_IMG),flair,)))
$(eval $(call bochs-flair-leg,flair-cpanels-corrupt,$(CP_GATE_IMG),$(BUILD)/cp_corrupt_data.img,flair,))
$(eval $(call bochs-flair-leg,flair-grow-box,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))

# WL-0141/0147: physical I/O, FDISK and FORMAT/SYS use the shell kernel.
# /S generated-volume boot, editor input and disk mutation remain QEMU
# acceptance; these macro legs certify only their shared kernel boot chain.
# test-diskcopy-emu has NO leg: it is QEMU-RED (fixed ATA medium cannot swap),
# not one of the QEMU-green gates in scope (WL-0147, Named incomplete leg).
$(foreach gate,block-device-emu fdisk-emu formatsys-emu format-system-boot,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,)))
$(eval $(call bochs-flair-leg,dos-folder,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(foreach gate,conkeys j1lu-control-emu,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,)))
$(eval $(call bochs-flair-leg,ansi-emu,$(SECONDARY_TRACER_IMG),$(BUILD)/ansi_probe_data.img,shell,))
$(foreach gate,flair-finder-icons flair-trash-full flair-toolbox-hardening,$(eval $(call bochs-flair-leg,$(gate),$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,)))
# WL-0162 (lane/finderalerts, PM): the last four hand-rolled ENOMODE legs,
# re-keyed to this macro. Disk windows, file ops and the Trash window boot the
# flagship on the pristine data volume (profile flairdb); the alert-chrome leg
# boots its i123 gate image on the i123 quit data (profile flair).
$(foreach gate,flair-disk-windows flair-file-ops flair-trash-window,$(eval $(call bochs-flair-leg,$(gate),$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flairdb,)))
$(eval $(call bochs-flair-leg,flair-alert-chrome,$(FLAIR_I123_FILE_GATE_IMG),$(FLAIR_I123_QUIT_DATA),flair,))

# Kernel hygiene: preserve self-test kernel identity and completion markers.
# MZ invalid-entry stimulus and baked BUSY semantics remain the QEMU oracle;
# the Bochs macro certifies the real self-test image's boot/completion only.
$(eval $(call bochs-flair-leg,fat-hygiene-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,trash-hidden-emu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))
$(eval $(call bochs-flair-leg,ata-no-drive-emu,$(SECONDARY_TRACER_IMG),,nodrive,))
$(eval $(call bochs-flair-leg,baked-exec-guard,$(BUILD)/baked_guard.img,$(FAT_EXEC_IMG),exec,))
$(eval $(call bochs-flair-leg,mzload-entry-emu,$(MZEXEC_IMG),$(FAT_MZEXEC_IMG),mzexec,))

# WL-0148 and the SAMIR DML lane: 16 distinct named leaves + aggregates.
# No interpreter/FAT mutation parity is claimed without input replay.
$(eval $(call bochs-flair-leg,samir-core-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(eval $(call bochs-flair-leg,samir-dml-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))
$(foreach verb,$(SAMIR_DML_CASES),$(eval $(call bochs-flair-leg,samir-dml-$(verb)-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,)))
$(eval $(call bochs-flair-leg,samir-print-emu,$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,))

# WL-0154/0155: unchanged DOS shell image; host mtools/dosfstools and QEMU
# injected traces still grade COPY/REN, nested COMMAND /C, XCOPY CWD and
# AH40 CX=0 truncation (IBM DOS 3.30 TR p.6-140/PDF263; ADR-0003 App A).
$(foreach gate,shell-files-emu command-c-emu xcopy-cwd-emu fat-truncate-emu dir-match-emu shell-dir-look,$(eval $(call bochs-flair-leg,$(gate),$(SECONDARY_TRACER_IMG),$(FLAIR_DATA_IMG),shell,)))
# WL-0146 and WL-0152: every additional IWORD trace and the size wrapper.
$(foreach gate,flair-iword-codes flair-iword-block flair-iword-format flair-iword-fkeys flair-iword-search flair-iword-replace flair-iword-undo flair-iword-setup,$(eval $(call bochs-flair-leg,$(gate),$(FLAIR_IWORD_GATE_IMG),$(FLAIR_DATA_IMG),flair,)))
$(eval $(call bochs-flair-leg,kernel-size-r4-emu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),flair,))

# Factory oracle FIRST (Rule 1). The same second-boot parser is used by the
# live driver. The mutant cuts its wait to the first boot (Rule 6).
$(BUILD)/test_bochs_completion: harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/emu $< -o $@
$(BUILD)/test_bochs_completion_mut: harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
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
$(BUILD)/test_bochs_completion_$(1): harness/emu/test_bochs_completion.c $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
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
$(BUILD)/bochs_harness_mut_nowait: $(BOCHS_DRV_SRC) $(BOCHS_LIB_SRC) harness/emu/bochs.h harness/emu/bochs_trace.h | $(BUILD)
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
# Mixed mouse/keyboard acceptance, defined in emu/copybreak.mk (not a boot-only leg).
TEST_EMU_GATES += test-flair-copybreak-bochs

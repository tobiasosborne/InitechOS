## test-boot

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19401):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/ppm_text_check spec/dos_banner.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-boot : InitechDOS banner boot gate\n'
@printf '  Ref: ADR-0003 DEC-12 / Appendix D.1; spec/dos_banner.txt (LOCKED).\n'
@printf '  beads initech-bea. CLAUDE.md Law 1/2/4, Rule 2/8/11.\n'
@printf '  TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (raw disk, real boot chain -> C kernel -> banner)\n' "$(SECONDARY_TRACER_IMG)"
@printf 'Expecting : serial S1/PM/OK/FONT/KERNEL/BI-OK/CONSOLE + banner literal + screendump text\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# Boot the live guest with serial + screendump capture (same path as
@# test-tracer-boot). The guest hlt-loops, so it times out -- expected.
@# --screendump-after BANNER (beads initech-3pe): wait for the paint-complete
@# marker before the framebuffer grab (same rationale as test-tracer-boot);
@# BANNER is serial-emitted only after both banner lines are blitted to the
@# LFB. The 6000 ms timeout remains the hard backstop for a never-painting guest.
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --screendump --screendump-after BANNER \
	--name "$(BOOT_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(BOOT_REPORT)" || true
@cat "$(BOOT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. Triple-fault check. ----
@if grep -q 'triple_fault=1' "$(BOOT_REPORT)"; then \
	printf '!!! test-boot FAIL: TRIPLE FAULT detected in the boot transition\n'; \
	exit 1; \
fi
@printf '>>> test-boot [1/4]: no triple-fault\n'
@# ---- 2. Serial milestone + banner markers + banner literals. ----
@if [ ! -s "$(BOOT_SERIAL)" ]; then \
	printf '!!! test-boot FAIL: no serial captured at %s\n' "$(BOOT_SERIAL)"; \
	exit 1; \
fi
@for m in S1 PM OK FONT KERNEL INT21 BI-OK CONSOLE BANNER-BEGIN BANNER-END BANNER; do \
	grep -q "^$$m$$" "$(BOOT_SERIAL)" \
		|| { printf '!!! test-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
done
@grep -q '^InitechDOS  Version 3.30$$' "$(BOOT_SERIAL)" \
	|| { printf '!!! test-boot FAIL: banner line 1 "InitechDOS  Version 3.30" (double space) missing on serial\n'; exit 1; }
@grep -q '^Copyright (C) 1991 Initech Systems Corporation.  All Rights Reserved.$$' "$(BOOT_SERIAL)" \
	|| { printf '!!! test-boot FAIL: banner copyright line missing on serial\n'; exit 1; }
@printf '>>> test-boot [2/4]: serial markers + banner literals present\n'
@# ---- 3. Byte-exactness vs spec/dos_banner.txt: extract the lines between
@#         BANNER-BEGIN and BANNER-END and diff against the LOCKED spec. ----
@awk '/^BANNER-BEGIN$$/{f=1;next} /^BANNER-END$$/{f=0} f{print}' \
	"$(BOOT_SERIAL)" > "$(BOOT_LINES)"
@diff -u "$(SPEC_BANNER)" "$(BOOT_LINES)" \
	|| { printf '!!! test-boot FAIL: kernel banner DRIFTED from spec/dos_banner.txt (Law 1 / Rule 8)\n'; exit 1; }
@printf '>>> test-boot [3/4]: serial banner == spec/dos_banner.txt byte-for-byte\n'
@# ---- 4. Screendump: banner blitted at its known origin on seafoam desktop. ----
@if [ ! -s "$(BOOT_PPM)" ]; then \
	printf '!!! test-boot FAIL: no screendump captured at %s (live guest required)\n' "$(BOOT_PPM)"; \
	exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(BOOT_PPM)" \
	|| { printf '!!! test-boot FAIL: screendump does not show the banner on the seafoam desktop\n'; exit 1; }
@printf '>>> test-boot [4/4]: screendump banner-text check\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS banner printed (serial == spec, rendered on screen), no triple-fault\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-boot: green (oracle completed)\n'
```

## test-boot-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19314):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-boot-bochs : BOCHS leg of the boot gate\n'
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator). beads initech-564 / initech-x0i\n'
@printf '  Bochs: legacy BIOS + LGPL vgabios + pentium; VESA 0101h 640x480x8 LFB (WL-0170).\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-boot-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2). Set SKIP_BOCHS=1 to explicitly (and loudly) opt out.\n'; exit 1; }
@printf 'Booting   : %s under Bochs (RFB headless; serial via com1=file)\n' "$(SECONDARY_TRACER_IMG)"
@printf 'Expecting : VBE-MODE=0101 + the 8-bit DAC VIDEO handoff, then the SAME kernel markers as QEMU\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# The run stops at the complete SHELL-READY line (--quit-after); OK is the
@# RFB unblock + no-triple-fault + the --expect marker, not the exit code.
@$(BOCHS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --expect SHELL-READY --quit-after SHELL-READY \
	--name "$(BOCHS_BOOT_NAME)" --out "$(BUILD)" --timeout-ms 45000 \
	2> "$(BOCHS_BOOT_REPORT)" || true
@cat "$(BOCHS_BOOT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# 1-3b. The ONE Bochs serial verdict (harness/emu/bochs_leg_check.sh): RFB
@#     unblock, no triple fault, the vbe101 video path (VBE-MODE=0101 + the
@#     VIDEO 640x480 bpp=8 dac=8 handoff, and NOT the mode-13h markers), the
@#     shared S1/PM/KERNEL/CONSOLE/BANNER milestones, then the SHARED kernel
@#     milestone set -- the differential vs QEMU (test-tracer-boot asserts
@#     these on the QEMU leg) -- through to the shell prompt.
@sh harness/emu/bochs_leg_check.sh test-boot-bochs "$(BOCHS_BOOT_REPORT)" "$(BOCHS_BOOT_SERIAL)" vbe101 \
	S2 FONT E820 KLOAD A20 GDT LFB OK KHI INT21 BI-OK IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
@# 3c. The same high-placement assertion under the strict-transition emulator
@#     (kernel-runway-relocation.md K3; bead initech-tdnl.29).
@kat=$$(grep -m1 '^KAT 0x' "$(BOCHS_BOOT_SERIAL)" || true); \
	range=$${kat#KAT }; start=$${range%%-*}; end=$${range#*-}; \
	if [ "$$start" != "0x00500000" ]; then \
		printf '!!! test-boot-bochs FAIL: KAT start=%s, expected 0x00500000\n' "$$start"; exit 1; \
	fi; \
	case "$$end" in 0x*) ;; *) printf '!!! test-boot-bochs FAIL: malformed KAT end in "%s"\n' "$$kat"; exit 1;; esac; \
	endv=$$(( end )); \
	if [ "$$endv" -ge $$(( 0x00600000 )) ]; then \
		printf '!!! test-boot-bochs FAIL: KAT end=%s reaches KERNEL_CEIL=0x00600000\n' "$$end"; exit 1; \
	fi; \
	printf '>>> test-boot-bochs: green KHI + KAT placement (%s)\n' "$$range"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- tracer booted under Bochs on the VESA 0101h 8bpp path,\n'
@printf '            reached the same kernel milestones as QEMU, no triple-fault\n'
@printf '======================================================================\n'
@printf '>>> test-boot-bochs: green\n'
```

## test-boot-system

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 13):
Prerequisites: `build/boot_system build/tracer_secondary.img build/flair_tenants.img build/.makefile.stamp`
```make
@sh $(BOOT_SYSTEM_DIR)/boot_system.sh host $(BUILD) $(SECONDARY_TRACER_IMG)
@sh $(BOOT_SYSTEM_DIR)/boot_system.sh host $(BUILD) $(FLAIRTENANTS_IMG) shipped $(INITDOS_SYS_BIN)
```

## test-boot-system-budget

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 75):
Prerequisites: `build/stage2.bin build/IO.SYS build/kernel_flairtenants.elf build/kernel_shell_secondary.elf build/.makefile.stamp`
```make
@set -eu; test $$(wc -c < $(STAGE2_BIN)) -eq 8192; io=$$(wc -c < $(IO_SYS_BIN)); test $$io -le 4096; \
 echo "GREEN test-boot-system-budget: stage2 8192/8192 B (bootstrap <=4096 + IO.SYS $$io/4096); assembly guards enforced"; \
 for e in $(KERNEL_FLAIRTENANTS_ELF) $(BUILD)/kernel_shell_secondary.elf; do \
 $(OBJCOPY) -O binary $$e $(BUILD)/boot-budget.bin; size=$$(wc -c < $(BUILD)/boot-budget.bin); \
 left=$$(( $(KERNEL_SECTORS)*512-size )); test $$left -ge $(KERNEL_HEADROOM_FLOOR); \
 echo "GREEN test-boot-system-budget: $$e flat loaded=$$size B, bounce headroom=$$left B >= $(KERNEL_HEADROOM_FLOOR) B"; done
@rm -f $(BUILD)/boot-budget.bin
```

## test-boot-system-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 16):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/boot_system build/.makefile.stamp`
```make
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
```

## test-boot-system-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 55):
Prerequisites: `build/qemu_harness build/boot_system build/tracer_secondary.img build/stage2_system_ORDER.bin build/stage2_system_ATTR_R.bin build/stage2_system_CHAIN.bin build/.makefile.stamp`
```make
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
```

## test-boot-system-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 27):
Prerequisites: `test-boot-system build/.makefile.stamp`
```make
@set -eu; for m in $(BOOT_SYSTEM_MUTANTS); do \
 cp -f $(SECONDARY_TRACER_IMG) $(BUILD)/boot-system/host-$$m.img; \
 $(BUILD)/boot_system $(BUILD)/boot-system/host-$$m.img $$m; \
 if $(BUILD)/boot_system $(BUILD)/boot-system/host-$$m.img check > $(BUILD)/boot-system/host-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 grep '^RED boot-system ' $(BUILD)/boot-system/host-$$m.log; \
 echo "GREEN test-boot-system-mutant $$m rejected"; done
```

## test-boot-system-negative

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 34):
Prerequisites: `build/qemu_harness build/boot_system build/tracer_secondary.img build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/boot-system
@set -eu; for m in MISSING $(BOOT_SYSTEM_MUTANTS); do \
 cp -f $(SECONDARY_TRACER_IMG) $(BUILD)/boot-system/negative-$$m.img; \
 if [ $$m = MISSING ]; then \
 mattrib -i $(BUILD)/boot-system/negative-$$m.img@@32256 -h -s -r ::IO.SYS; \
 mdel -i $(BUILD)/boot-system/negative-$$m.img@@32256 ::IO.SYS; \
 else $(BUILD)/boot_system $(BUILD)/boot-system/negative-$$m.img $$m; fi; \
 INITECH_QMP_STDIO=1 sh $(BOOT_SYSTEM_DIR)/boot_system.sh negative $(BUILD) $(BUILD)/boot-system/negative-$$m.img $$m; done
```

## test-boot-system-repro

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/boot_system.mk', line 67):
Prerequisites: `build/flair_tenants.img tools/mk_boot_system.sh build/boot_system build/flairovl_kernel_flairtenants.ovl build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/boot-system
@set -eu; for tag in a b; do \
 sh $(BOOT_SYSTEM_MINT) $(BUILD)/boot-system/repro-$$tag.img $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(INITDOS_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BUILD)/transient_flairtenants.bin $(BUILD)/flairovl_kernel_flairtenants.ovl; done
@cmp $(BUILD)/boot-system/repro-a.img $(BUILD)/boot-system/repro-b.img
@cmp $(FLAIRTENANTS_IMG) $(BUILD)/boot-system/repro-a.img
@echo 'GREEN test-boot-system-repro: two fresh FAT builds == each other == shipped boot image, byte for byte'
```

## test-tracer-boot

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19195):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-tracer-boot : real MBR->LFB boot oracle\n'
@printf '  Ref: PRD Sec 5 (hardware contract) / Sec 11 (M1). beads initech-f8v.2\n'
@printf '  NOTE: pure-seafoam grid SUPERSEDED by ppm_text_check (banner blitted);\n'
@printf '        this gate is now STRICTLY STRONGER (banner + seafoam desktop). initech-bea\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (raw disk, custom MBR -> stage2 -> 32-bit flat -> VESA LFB)\n' "$(SECONDARY_TRACER_IMG)"
@printf 'Expecting : serial S1/.../KLOAD/KHI/KERNEL/KAT/BI-OK/BANNER + high placement + screendump\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# Boot via the disk path with a live-guest screendump. The guest hlt-loops,
@# so it will time out -- that is expected; we do not gate on the exit code.
@# --screendump-after BANNER (beads initech-3pe): WAIT for the guest's
@# paint-complete serial marker before grabbing the framebuffer, removing the
@# wall-clock race that blanked the screendump under a loaded host. BANNER is
@# emitted (kmain.c) AFTER both banner lines are blitted to the LFB console
@# (dos_puts -> con sink -> console_putc -> console_draw_glyph). The 6000 ms
@# timeout stays the HARD backstop: a guest that never paints still RED.
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --screendump --screendump-after BANNER \
	--name "$(TRACER_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(TRACER_REPORT)" || true
@cat "$(TRACER_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# 1. Triple-fault check (the minefield): the -d log must report none.
@if grep -q 'triple_fault=1' "$(TRACER_REPORT)"; then \
	printf '!!! test-tracer-boot FAIL: TRIPLE FAULT detected in the real->protected transition\n'; \
	exit 1; \
fi
@# 2. Serial stage markers. Require at least S1 (MBR), PM (protected mode),
@#    OK (framebuffer filled). Report the full set we saw.
@printf 'Serial markers captured:\n'
@if [ ! -s "$(TRACER_SERIAL)" ]; then \
	printf '!!! test-tracer-boot FAIL: no serial captured at %s\n' "$(TRACER_SERIAL)"; \
	exit 1; \
fi
@for m in S1 S2 VBE FONT KLOAD A20 GDT PM LFB OK KHI KERNEL INT21 BI-OK CONSOLE BANNER; do \
	if grep -q "^$$m$$" "$(TRACER_SERIAL)"; then \
		printf '  %-6s : present\n' "$$m"; \
	else \
		printf '  %-6s : MISSING\n' "$$m"; \
	fi; \
done
@# Required subset: the full handoff chain. KERNEL proves the far jump into
@# the C kernel; BI-OK proves boot_info is readable in C (beads initech-d00);
@# CONSOLE proves the LFB text console initialized (initech-yqb); BANNER
@# proves the InitechDOS banner was rendered (initech-bea).
@for m in S1 PM OK FONT KLOAD KHI KERNEL INT21 BI-OK CONSOLE BANNER; do \
	grep -q "^$$m$$" "$(TRACER_SERIAL)" \
		|| { printf '!!! test-tracer-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
done
@# 3. Kernel high-placement leg (runway design K3 / bead initech-tdnl.29).
@kat=$$(grep -m1 '^KAT 0x' "$(TRACER_SERIAL)" || true); \
	range=$${kat#KAT }; start=$${range%%-*}; end=$${range#*-}; \
	if [ "$$start" != "0x00500000" ]; then \
		printf '!!! test-tracer-boot FAIL: KAT start=%s, expected 0x00500000\n' "$$start"; exit 1; \
	fi; \
	case "$$end" in 0x*) ;; *) printf '!!! test-tracer-boot FAIL: malformed KAT end in "%s"\n' "$$kat"; exit 1;; esac; \
	endv=$$(( end )); \
	if [ "$$endv" -ge $$(( 0x00600000 )) ]; then \
		printf '!!! test-tracer-boot FAIL: KAT end=%s reaches KERNEL_CEIL=0x00600000\n' "$$end"; exit 1; \
	fi; \
	printf '>>> test-tracer-boot: KHI + KAT placement green (%s)\n' "$$range"
@# 4. Screendump check: banner blitted at its known origin AND the desktop is
@#    still seafoam below it (ppm_text_check -- STRICTLY STRONGER than the old
@#    pure-seafoam grid, which the banner now legitimately perturbs).
@if [ ! -s "$(TRACER_PPM)" ]; then \
	printf '!!! test-tracer-boot FAIL: no screendump captured at %s (live guest required)\n' "$(TRACER_PPM)"; \
	exit 1; \
fi
@$(PPM_TEXT_CHECK_BIN) "$(TRACER_PPM)" \
	|| { printf '!!! test-tracer-boot FAIL: banner not rendered on the seafoam desktop\n'; exit 1; }
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- real boot chain reached protected mode, blitted the InitechDOS banner on the seafoam desktop, no triple-fault\n'
@printf '======================================================================\n'
```

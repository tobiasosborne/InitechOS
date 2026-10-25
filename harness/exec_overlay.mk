# INT 21h AH=4Bh AL=03h LOAD OVERLAY (ADR-0003 Amendment DEC-19; bead
# initech-dewv, epic initech-5etom). Un-defers DEC-08a.7: the OS service the
# SAMIR resident-core + SAMIR.OVL split (Phase B) will call.
# Ref: IBM DOS 3.30 TR pp. 6-196/197 (AL=03H, the {load, factor} block),
# p. 11-5 (no PSP, no allocation, caller-owned target), p. 6-42 (codes).
# Included after the exec-nested block (NEST_OBJS) and fixture_volumes.mk.
#
# Host gate (TEST_UNIT_GATES): test-overlay drives the REAL loader_overlay_core,
# int21 AH=4Bh AL=03h dispatch and int21_overlay_dest_ok (os/milton/
# test_overlay.c). test-overlay-mutant: four mutants, each RED for its own
# named check -- factor not applied, off-by-one ownership bound, untagged MZ
# accepted, and premature destination writes.
#
# Emu gate (TEST_EMU_GATES, QEMU -- the EXEC family's tri-emulator status:
# test-exec / test-mzexec / test-exec-nested are QEMU-only, Bochs/86Box ride
# initech-x0i): the exec-nested kernel compiled -DBOOT_EXEC_OVERLAY EXECs
# OVLTEST.COM, which loads OVLT.OVL (mzlink InitechMZ, relocations DERIVED by
# --reloc-diff), OVLR.BIN (raw) and FOREIGN.OVL (untagged) through a real
# `int 0x21` and grades itself into serial markers. test-exec-overlay-mutant
# boots the same disks on kernels carrying the three mutant objects.

OVL_PROBE          := 0x01010110
OVL_FIX_ASM        := $(KERNEL_DIR)/ovl_fixture.asm
OVLTEST_ASM        := $(KERNEL_DIR)/ovltest_program.asm
OVL_EXE            := $(BUILD)/ovlt.ovl
OVL_FOREIGN_EXE    := $(BUILD)/ovl_foreign.ovl
OVL_RAW            := $(BUILD)/ovlr.bin
OVLTEST_BIN        := $(BUILD)/ovltest_program.bin
OVLTEST_FOREIGN_BIN := $(BUILD)/ovltest_foreign.bin
FAT_OVL_IMG        := $(BUILD)/fat_ovl.img
FAT_OVL_FOREIGN_IMG := $(BUILD)/fat_ovl_foreign.img

# ---- host oracle -----------------------------------------------------------
TEST_OVERLAY_SRC   := $(MILTON_DIR)/test_overlay.c
TEST_OVERLAY_LINK  := $(TEST_EXEC_DEPS) $(KERNEL_LOADER_C) $(MILTON_DIR)/fat12.c
TEST_OVERLAY_DEPS  := $(TEST_OVERLAY_SRC) $(TEST_OVERLAY_LINK) $(TEST_EXEC_HDRS) \
                      $(MILTON_DIR)/loader.h $(MILTON_DIR)/mz.h $(MILTON_DIR)/mz.c \
                      $(MILTON_DIR)/mcb.h $(MILTON_DIR)/fat12.h $(MILTON_DIR)/blockdev.h \
                      spec/memory_map.h spec/toolbox_gate.h
TEST_OVERLAY       := $(BUILD)/test_overlay
OVL_DOS33_REF      := harness/fixtures/dos33_overlay
$(BUILD)/overlay_dos33_check: $(OVL_DOS33_REF)/check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
$(BUILD)/overlay_dos33_check_mut: $(OVL_DOS33_REF)/check.c | $(BUILD)
	$(CC) $(CFLAGS) -DMUTATE_EXPECT_LOAD_PATCH -o $@ $<
.PHONY: test-overlay-dos33-reference
test-overlay-dos33-reference: $(BUILD)/overlay_dos33_check $(BUILD)/overlay_dos33_check_mut
	@$(BUILD)/overlay_dos33_check $(OVL_DOS33_REF)/factor-different.bin $(OVL_DOS33_REF)/factor-equal.bin
	@if $(BUILD)/overlay_dos33_check_mut $(OVL_DOS33_REF)/factor-different.bin $(OVL_DOS33_REF)/factor-equal.bin > $(BUILD)/overlay_dos33_check_mut.log 2>&1; then \
		printf '!!! DOS33 load-based-address mutant survived\n'; exit 1; fi
	@grep -qx 'FAIL DOS33 different-factor fixup address' $(BUILD)/overlay_dos33_check_mut.log
	@printf '>>> test-overlay-dos33-reference: green; load-based-address mutant RED\n'
# name:define pairs; the name is the mutant binary suffix.
OVL_HOST_MUTANTS   := nofactor:LOADER_MUTATE_OVL_NO_FACTOR \
                      obo:INT21_MUTATE_OVL_BOUNDS_OBO \
                      foreign:LOADER_MUTATE_OVL_ACCEPT_FOREIGN \
                      atomic:LOADER_MUTATE_OVL_EARLY_COMMIT

$(TEST_OVERLAY): $(TEST_OVERLAY_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -Ibuild \
		-o $@ $(TEST_OVERLAY_SRC) $(TEST_OVERLAY_LINK)

define ovl-host-mutant
$(BUILD)/test_overlay_mutant_$(1): $(TEST_OVERLAY_DEPS) | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -D$(2) -Ispec -I$$(MILTON_DIR) -Iseed -Ibuild \
		-o $$@ $$(TEST_OVERLAY_SRC) $$(TEST_OVERLAY_LINK)
endef
$(foreach m,$(OVL_HOST_MUTANTS),$(eval $(call ovl-host-mutant,$(word 1,$(subst :, ,$(m))),$(word 2,$(subst :, ,$(m))))))

# $(call ovl-host-red,<suffix>,<the FAIL text that must appear>,<label>)
define ovl-host-red
	@if $(BUILD)/test_overlay_mutant_$(1) > $(BUILD)/test_overlay_mutant_$(1).log 2>&1; then \
		printf '!!! test-overlay-mutant FAIL: %s mutant PASSED -- the oracle is decoration\n' '$(3)'; exit 1; fi
	@grep -qF '$(2)' $(BUILD)/test_overlay_mutant_$(1).log || { cat $(BUILD)/test_overlay_mutant_$(1).log; \
		printf '!!! test-overlay-mutant FAIL: %s mutant RED, but not for its named reason\n' '$(3)'; exit 1; }
	@printf '>>> test-overlay-mutant: %s RED (%s)\n' '$(3)' '$(2)'
endef

.PHONY: test-overlay test-overlay-mutant
test-overlay: $(TEST_OVERLAY) test-overlay-dos33-reference
	@printf '>>> test-overlay: AH=4Bh AL=03h core (factor reloc, footprint, raw, refusals) + dispatch + DEC-19.4 ownership\n'
	@$(TEST_OVERLAY)
	@printf '>>> test-overlay: green\n'

test-overlay-mutant: test-overlay $(foreach m,$(OVL_HOST_MUTANTS),$(BUILD)/test_overlay_mutant_$(word 1,$(subst :, ,$(m))))
	$(call ovl-host-red,nofactor,A1 reloc @4: 0x10 + factor 0x123400 = 0x123410,factor-not-applied)
	$(call ovl-host-red,obo,C5 one byte past the block (the next MCB header) refused,bounds-off-by-one)
	$(call ovl-host-red,foreign,A5 untagged MZ -> LOADER_ERR_FOREIGN_MZ,untagged-MZ-accepted)
	$(call ovl-host-red,atomic,A12 failed read leaves the WHOLE destination untouched,early-destination-write)
	@printf '>>> test-overlay-mutant: green (all four mutants RED for their named reasons)\n'
TEST_UNIT_GATES += test-overlay test-overlay-mutant

# ---- emulator fixtures -------------------------------------------------------
$(BUILD)/ovl_fixture.o0.bin: $(OVL_FIX_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/ovl_fixture.o1.bin: $(OVL_FIX_ASM) | $(BUILD)
	$(NASM) -f bin -DOVL_ORG=$(OVL_PROBE) $< -o $@
$(OVL_EXE): $(BUILD)/ovl_fixture.o0.bin $(BUILD)/ovl_fixture.o1.bin $(MZLINK_BIN)
	$(MZLINK_BIN) $(BUILD)/ovl_fixture.o0.bin $@ --reloc-diff $(BUILD)/ovl_fixture.o1.bin $(OVL_PROBE) --entry 8
$(OVL_FOREIGN_EXE): $(BUILD)/ovl_fixture.o0.bin $(BUILD)/ovl_fixture.o1.bin $(MZLINK_BIN)
	$(MZLINK_BIN) $(BUILD)/ovl_fixture.o0.bin $@ --reloc-diff $(BUILD)/ovl_fixture.o1.bin $(OVL_PROBE) --entry 8 --foreign
# The raw overlay: exactly RAW_LEN (19) bytes, the marker the program prints.
$(OVL_RAW): harness/exec_overlay.mk | $(BUILD)
	@printf 'OVL-RAW-VERBATIM\r\n$$' > $@
	@test "$$(wc -c < $@)" -eq 19 || { printf '!!! %s is not 19 bytes\n' "$@"; exit 1; }
$(OVLTEST_BIN): $(OVLTEST_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@
$(OVLTEST_FOREIGN_BIN): $(OVLTEST_ASM) | $(BUILD)
	$(NASM) -f bin -DOVL_FOREIGN $< -o $@

$(eval $(call fixture-contract,$(FAT_OVL_IMG),HELLO.TXT GREET.COM OVLTEST.COM OVLT.OVL OVLR.BIN))
$(eval $(call fixture-contract,$(FAT_OVL_FOREIGN_IMG),HELLO.TXT GREET.COM OVLTEST.COM FOREIGN.OVL))
$(FAT_OVL_IMG): $(FAT_EXEC_IMG) $(OVLTEST_BIN) $(OVL_EXE) $(OVL_RAW)
	@cp -f $< $@
	@mcopy -o -i $@ $(OVLTEST_BIN) ::OVLTEST.COM
	@mcopy -o -i $@ $(OVL_EXE) ::OVLT.OVL
	@mcopy -o -i $@ $(OVL_RAW) ::OVLR.BIN
	@$(call fixture-check,recipe,$@)
$(FAT_OVL_FOREIGN_IMG): $(FAT_EXEC_IMG) $(OVLTEST_FOREIGN_BIN) $(OVL_FOREIGN_EXE)
	@cp -f $< $@
	@mcopy -o -i $@ $(OVLTEST_FOREIGN_BIN) ::OVLTEST.COM
	@mcopy -o -i $@ $(OVL_FOREIGN_EXE) ::FOREIGN.OVL
	@$(call fixture-check,recipe,$@)

# ---- kernels: the exec-nested objects, kmain -DBOOT_EXEC_OVERLAY ----------
OVL_MAIN := $(BUILD)/kmain_exec_overlay.o
OVL_OBJS := $(filter-out $(BUILD)/kmain_exec_nested.o,$(NEST_OBJS)) $(OVL_MAIN)
$(OVL_MAIN): $(KERNEL_MAIN_C) $(SERIAL_TRACE_PACK) os/milton/loader.h os/milton/int21.h | $(BUILD)
	$(call kernel-main-compile,-DBOOT_EXEC_NESTED -DBOOT_EXEC_OVERLAY -Ispec -I$(KERNEL_DIR),$@)
$(BUILD)/loader_ovl_mut_nofactor.o: $(KERNEL_LOADER_C) os/milton/loader.h os/milton/int21.h os/milton/mz.c | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_LOADER_OPT) -DLOADER_MUTATE_OVL_NO_FACTOR -Ispec -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/loader_ovl_mut_foreign.o: $(KERNEL_LOADER_C) os/milton/loader.h os/milton/int21.h os/milton/mz.c | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_LOADER_OPT) -DLOADER_MUTATE_OVL_ACCEPT_FOREIGN -Ispec -I$(KERNEL_DIR) -c $< -o $@
$(BUILD)/int21_ovl_mut_obo.o: $(KERNEL_INT21_C) $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/mcb.h spec/dos_structs.h $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -DINT21_MUTATE_OVL_BOUNDS_OBO -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $(KERNEL_INT21_C) -o $@
$(BUILD)/kernel_exec_overlay.elf: $(OVL_OBJS) $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(OVL_OBJS)
$(BUILD)/kernel_exec_overlay_mut_nofactor.elf: $(filter-out $(KERNEL_LOADER_OBJ),$(OVL_OBJS)) $(BUILD)/loader_ovl_mut_nofactor.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_LD),$^)
$(BUILD)/kernel_exec_overlay_mut_foreign.elf: $(filter-out $(KERNEL_LOADER_OBJ),$(OVL_OBJS)) $(BUILD)/loader_ovl_mut_foreign.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_LD),$^)
$(BUILD)/kernel_exec_overlay_mut_obo.elf: $(filter-out $(KERNEL_INT21_OBJ),$(OVL_OBJS)) $(BUILD)/int21_ovl_mut_obo.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_LD),$^)
OVL_KERNELS := exec_overlay exec_overlay_mut_nofactor exec_overlay_mut_foreign exec_overlay_mut_obo
$(foreach k,$(OVL_KERNELS),$(BUILD)/kernel_$(k).bin): $(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary $< $@
	@test $$(wc -c < $@) -le $$(( $(KERNEL_SECTORS) * 512 ))
	@truncate -s $$(( $(KERNEL_SECTORS) * 512 )) $@
	$(call kernel-end-guard,$<,exec-overlay)
$(foreach k,$(OVL_KERNELS),$(BUILD)/$(k).img): $(BUILD)/%.img: $(BUILD)/kernel_%.bin $(MBR_BIN) $(STAGE2_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $< $(BOOT_SYSTEM_TOOL)

# $(call ovl-boot,<kernel image stem>,<data image>,<run name>,<expect>)
define ovl-boot
@$(call fixture-preflight,$(2))
@cp -f $(2) $(BUILD)/$(3)_data.img
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/$(1).img --disk2 $(BUILD)/$(3)_data.img \
	--expect '$(4)' --name $(3) --out $(BUILD) --timeout-ms 20000 2> $(BUILD)/$(3).report || true
@! grep -q 'triple_fault=1' $(BUILD)/$(3).report || { cat $(BUILD)/$(3).report; printf '!!! %s: triple fault\n' '$(3)'; exit 1; }
@tr -d '\r' < $(BUILD)/$(3).serial > $(BUILD)/$(3).txt
endef
OVL_GREEN_MARKERS := OVL-MZ-CALLED OVL-MZ-OK OVL-FACTOR-OK OVL-RAW-VERBATIM OVL-BLOCK-OK OVL-REFUSE-OK OVL-ALL-OK

.PHONY: test-exec-overlay test-exec-overlay-mutant
test-exec-overlay: $(HARNESS_BIN) $(BUILD)/exec_overlay.img $(FAT_OVL_IMG) $(FAT_OVL_FOREIGN_IMG)
	$(call ovl-boot,exec_overlay,$(FAT_OVL_IMG),exec_overlay,EXEC-NESTED-EXIT rc=)
	@for m in $(OVL_GREEN_MARKERS); do grep -qx "$$m" $(BUILD)/exec_overlay.txt || { cat $(BUILD)/exec_overlay.txt; printf '!!! test-exec-overlay RED: marker %s missing\n' "$$m"; exit 1; }; done
	@! grep -q '^OVL-FAIL' $(BUILD)/exec_overlay.txt || { grep '^OVL-FAIL' $(BUILD)/exec_overlay.txt; printf '!!! test-exec-overlay RED\n'; exit 1; }
	@grep -qx 'EXEC-NESTED-EXIT rc=0' $(BUILD)/exec_overlay.txt || { cat $(BUILD)/exec_overlay.txt; printf '!!! test-exec-overlay RED: OVLTEST.COM did not exit rc=0\n'; exit 1; }
	@printf '>>> test-exec-overlay [1/2]: InitechMZ overlay relocated by the factor + called; raw verbatim; AH=48h exact fit; kernel/PSP/MCB/one-past refused 0009h; missing 0002h\n'
	$(call ovl-boot,exec_overlay,$(FAT_OVL_FOREIGN_IMG),exec_overlay_foreign,PANIC foreign-mz)
	@grep -q '^PANIC foreign-mz' $(BUILD)/exec_overlay_foreign.txt && ! grep -q '^OVL-FOREIGN-AFTER' $(BUILD)/exec_overlay_foreign.txt \
		|| { cat $(BUILD)/exec_overlay_foreign.txt; printf '!!! test-exec-overlay RED: an untagged MZ overlay did not panic (DEC-08a.5)\n'; exit 1; }
	@printf '>>> test-exec-overlay [2/2]: an untagged 16-bit MZ overlay panicked fail-loud, never loaded (DEC-08a.5)\n'
	@printf '>>> test-exec-overlay: green (QEMU; the EXEC family is QEMU-only pending initech-x0i)\n'

test-exec-overlay-mutant: test-exec-overlay $(foreach k,$(filter-out exec_overlay,$(OVL_KERNELS)),$(BUILD)/$(k).img)
	$(call ovl-boot,exec_overlay_mut_nofactor,$(FAT_OVL_IMG),exec_overlay_mut_nofactor,EXEC-NESTED-EXIT rc=)
	@grep -qx 'OVL-FAIL mz-reloc' $(BUILD)/exec_overlay_mut_nofactor.txt && ! grep -q '^OVL-ALL-OK' $(BUILD)/exec_overlay_mut_nofactor.txt \
		|| { cat $(BUILD)/exec_overlay_mut_nofactor.txt; printf '!!! test-exec-overlay-mutant FAIL: factor-not-applied survived\n'; exit 1; }
	@printf '>>> test-exec-overlay-mutant: LOADER_MUTATE_OVL_NO_FACTOR RED (OVL-FAIL mz-reloc)\n'
	$(call ovl-boot,exec_overlay_mut_obo,$(FAT_OVL_IMG),exec_overlay_mut_obo,EXEC-NESTED-EXIT rc=)
	@grep -qx 'OVL-FAIL one-past' $(BUILD)/exec_overlay_mut_obo.txt && ! grep -q '^OVL-ALL-OK' $(BUILD)/exec_overlay_mut_obo.txt \
		|| { cat $(BUILD)/exec_overlay_mut_obo.txt; printf '!!! test-exec-overlay-mutant FAIL: bounds off-by-one survived\n'; exit 1; }
	@printf '>>> test-exec-overlay-mutant: INT21_MUTATE_OVL_BOUNDS_OBO RED (OVL-FAIL one-past)\n'
	$(call ovl-boot,exec_overlay_mut_foreign,$(FAT_OVL_FOREIGN_IMG),exec_overlay_mut_foreign,EXEC-NESTED-EXIT rc=)
	@grep -qx 'OVL-FOREIGN-AFTER' $(BUILD)/exec_overlay_mut_foreign.txt && ! grep -q '^PANIC foreign-mz' $(BUILD)/exec_overlay_mut_foreign.txt \
		|| { cat $(BUILD)/exec_overlay_mut_foreign.txt; printf '!!! test-exec-overlay-mutant FAIL: untagged-MZ-accepted mutant did not change the outcome\n'; exit 1; }
	@printf '>>> test-exec-overlay-mutant: LOADER_MUTATE_OVL_ACCEPT_FOREIGN RED (no panic; OVL-FOREIGN-AFTER printed)\n'
	@printf '>>> test-exec-overlay-mutant: green (all three kernel mutants RED for their named reasons)\n'
TEST_EMU_GATES += test-exec-overlay test-exec-overlay-mutant

# The shipping FLAIR kernel reaches its text tenant through the GUI pump,
# before the common text-kernel backend bindings. Drive the SAME OVLTEST
# bytes under SAMIR.COM, with the system's real Ctrl-Alt-backslash launcher.
# Ref: ADR-0013 Sec 3.2 (class-2 tenant); review 2026-10-25, initech-i15.
FAT_OVL_FLAIR_IMG := $(BUILD)/fat_ovl_flair.img
$(eval $(call fixture-contract,$(FAT_OVL_FLAIR_IMG),HELLO.TXT GREET.COM OVLTEST.COM OVLT.OVL OVLR.BIN SAMIR.COM))
$(FAT_OVL_FLAIR_IMG): $(FAT_OVL_IMG) $(OVLTEST_BIN)
	@cp -f $< $@
	@mcopy -o -i $@ $(OVLTEST_BIN) ::SAMIR.COM
	@$(call fixture-check,recipe,$@)

$(BUILD)/kmain_flairtenants_mut_ovl_unbound.o: $(KERNEL_FLAIRTENANTS_MAIN_OBJ) $(KERNEL_MAIN_C) | $(BUILD)
	$(call kernel-main-compile,$(KERNEL_TENANTS_OPT) -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -DKMAIN_MUT_OVL_UNBOUND -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -I$(KERNEL_DIR),$@)
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_FLAIRTENANTS_MAIN_OBJ),$(BUILD)/kmain_flairtenants_mut_ovl_unbound.o,ovl_unbound))

# $(call ovl-flair-boot,<boot image>,<run name>)
define ovl-flair-boot
@$(call fixture-preflight,$(FAT_OVL_FLAIR_IMG))
@cp -f $(FAT_OVL_FLAIR_IMG) $(BUILD)/$(2)_data.img
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(1) --disk2 $(BUILD)/$(2)_data.img \
	--expect FLAIR-RESUME --name $(2) --out $(BUILD) --timeout-ms 20000 \
	--keys ctrl-alt-bsl --keys-after FLAIR-LIVE-READY --quit-after FLAIR-RESUME \
	2> $(BUILD)/$(2).report
@tr -d '\r' < $(BUILD)/$(2).serial > $(BUILD)/$(2).txt
endef

.PHONY: test-exec-overlay-flair test-exec-overlay-flair-mutant
test-exec-overlay-flair: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FAT_OVL_FLAIR_IMG)
	$(call ovl-flair-boot,$(FLAIRTENANTS_IMG),exec_overlay_flair)
	@for m in FLAIR-SUSPEND $(OVL_GREEN_MARKERS) 'FLAIR-TEXT-EXIT rc=0' FLAIR-RESUME; do \
		grep -qx "$$m" $(BUILD)/exec_overlay_flair.txt || { cat $(BUILD)/exec_overlay_flair.txt; \
		printf '!!! test-exec-overlay-flair RED: marker %s missing\n' "$$m"; exit 1; }; done
	@! grep -Eq '^(OVL-FAIL|PANIC)' $(BUILD)/exec_overlay_flair.txt
	@printf '>>> test-exec-overlay-flair: green (flagship GUI text tenant loads, calls and refuses overlays, then resumes)\n'

test-exec-overlay-flair-mutant: test-exec-overlay-flair $(BUILD)/flair_tenants_mut_ovl_unbound.img
	$(call ovl-flair-boot,$(BUILD)/flair_tenants_mut_ovl_unbound.img,exec_overlay_flair_mut_unbound)
	@grep -qx 'OVL-FAIL mz-load' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
		&& grep -qx 'FLAIR-TEXT-EXIT rc=1' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
		&& ! grep -qx 'OVL-ALL-OK' $(BUILD)/exec_overlay_flair_mut_unbound.txt \
		|| { cat $(BUILD)/exec_overlay_flair_mut_unbound.txt; printf '!!! overlay backend unbound mutant survived\n'; exit 1; }
	@printf '>>> test-exec-overlay-flair-mutant: KMAIN_MUT_OVL_UNBOUND RED (OVL-FAIL mz-load, rc=1)\n'

test-exec-overlay: test-exec-overlay-flair
test-exec-overlay-mutant: test-exec-overlay-flair-mutant

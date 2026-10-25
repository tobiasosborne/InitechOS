# execguard.mk -- EXEC of COMMAND.COM on the FLAGSHIP image (bead initech-13ig).
# Included from the Makefile after harness/emu/exit_dos.mk (it reuses that
# file's exitdos-boot / exitdos-transient-rules / exitdos-image-recipe).
#
# THE BUG (initech-13ig, found by the samirrun lane): on the flagship kernel the
# boot volume's COMMAND.COM is the TRANSIENT overlay (os/milton/transient.h,
# linked at TRANSIENT_BASE behind a COMMAND3 header), not a flat .COM. A program
# that EXECs it (AH=4Bh: dBASE RUN / !, os/samir/pal/pal_milton.c milton_exec)
# read the overlay to PROGRAM_IMAGE and JMPed into its header.
#
# THE DOS-3.3-TRUE BEHAVIOUR (MS-DOS 3.3 Reference pp.46-47: COMMAND /C "loads
# a second copy of COMMAND.COM", runs one command and returns; ADR-0003
# DEC-03/DEC-11: the interpreter is resident/transient, one code):
#   - once the transient is resident (Exit to InitechDOS), an EXEC of
#     COMMAND.COM by name runs the transient's command_exec_secondary -- the
#     SAME secondary processor the shell kernels run (initech-n8q8); the
#     header's exec_secondary entry carries it (os/milton/transient_hdr.asm);
#   - the overlay's BYTES are never a program: the loader recognises the
#     COMMAND3 magic in anything it reads (C:\COMMAND.COM, a copy under any
#     name) and refuses LOADER_ERR_BAD_FORMAT -> DOS EXEC error 11 "Invalid
#     format" (IBM DOS 3.30 TR p. 6-200); dBASE then raises its error 92.
#
# GATES (QEMU; the Bochs leg is the PM's):
#   test-flair-execguard: boot the flagship, Exit to InitechDOS (the locked
#     FLAIR_EXECGUARD_SPEC trace), type EXECGD at A:\> (EXECGD.COM on the
#     gate's data volume, os/milton/execguard_program.asm). Serial must show,
#     in order: EXECGUARD-BEGIN ; EXECGUARD-CHILD-OK (ECHOed by the COMMAND /C
#     child) ; EXECGUARD-COMSPEC-OK (CF clear, AH=4Dh rc 0) ; LOADER-REFUSE-
#     TRANSIENT name=CMD2.COM ; EXECGUARD-OVERLAY-REFUSED ax=000B ; EXECGUARD-
#     END ; no PANIC. CMD2.COM is a byte copy of the boot volume's COMMAND.COM.
#   test-flair-execguard-mutant (Rule 6): NO_COMSPEC (-DKMAIN_MUT_EXEC_NO_
#     COMSPEC: no route, the EXEC goes to the loader; COMMAND.COM is absent on
#     A:, so 0002h and no child echo) RED; NO_GUARD (-DLOADER_MUT_NO_TRANSIENT_
#     GUARD: the pre-13ig loader runs the overlay's header) RED -- no
#     LOADER-REFUSE-TRANSIENT / no OVERLAY-REFUSED; IMPORT_SKEW (the flagship
#     kernel + a transient whose LAST generated import address is +16: an
#     import the old hand-kept five-entry check never saw) RED -- MSG-DOS-0014
#     + PANIC "transient linked against another kernel build v=<last index>",
#     no SHELL-READY.

EXECGUARD_NAME     := flair_execguard
EXECGUARD_DONE     := EXECGUARD-END
EXECGUARD_PROG_ASM := os/milton/execguard_program.asm
EXECGUARD_PROG_BIN := $(BUILD)/execguard_program.bin
$(EXECGUARD_PROG_BIN): $(EXECGUARD_PROG_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@

# The gate's data volume: the stock flagship volume + EXECGD.COM + CMD2.COM
# (the boot volume's COMMAND.COM, byte for byte: mtools @@32256 = the DEC-07a
# boot partition at LBA 63). Contract: harness/fixture_volumes.mk (the derived
# stock contract + the two files).
FLAIR_DATA_EXECGUARD_IMG := $(BUILD)/flair_data_execguard.img
$(FLAIR_DATA_EXECGUARD_IMG): $(FLAIR_DATA_IMG) $(EXECGUARD_PROG_BIN) $(FLAIRTENANTS_IMG) | $(BUILD)
	cp -f $(FLAIR_DATA_IMG) $@
	mcopy -n -o -i $(FLAIRTENANTS_IMG)@@32256 ::COMMAND.COM $(BUILD)/execguard_overlay.bin
	@head -c 8 $(BUILD)/execguard_overlay.bin | grep -q 'COMMAND3' || { printf '!!! %s: the boot volume COMMAND.COM is not a COMMAND3 transient\n' "$@"; exit 1; }
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@ $(EXECGUARD_PROG_BIN) ::EXECGD.COM
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -i $@ $(BUILD)/execguard_overlay.bin ::CMD2.COM
	@printf '>>> execguard data volume: %s (+EXECGD.COM, +CMD2.COM = the COMMAND3 overlay)\n' "$@"
	@$(call fixture-check,recipe,$@)

# Mutant images. NO_COMSPEC: the kmain template's kernel + its OWN transient.
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_EXEC_NO_COMSPEC,exec_no_comspec))
$(eval $(call exitdos-transient-rules,exec_no_comspec,$(BUILD)/kernel_flairtenants_mut_exec_no_comspec.elf))
EXECGUARD_MUT_NOCOMSPEC_IMG := $(BUILD)/flair_execguard_mut_no_comspec.img
$(EXECGUARD_MUT_NOCOMSPEC_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/flairovl_kernel_flairtenants_mut_exec_no_comspec.sys $(BUILD)/flairovl_kernel_flairtenants_mut_exec_no_comspec.ovl $(BUILD)/transient_exec_no_comspec.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_mut_exec_no_comspec.bin,$(BUILD)/transient_exec_no_comspec.bin)
	@printf ">>> execguard MUTANT image (-DKMAIN_MUT_EXEC_NO_COMSPEC): %s\n" "$@"
# NO_GUARD: the flagship objects with loader.o built -DLOADER_MUT_NO_TRANSIENT_GUARD
# (the cwdreset pattern, harness/emu/exit_dos.mk), + its own transient.
$(BUILD)/loader_mut_noguard.o: $(KERNEL_LOADER_C) $(KERNEL_DIR)/loader.h $(KERNEL_DIR)/psp.h spec/memory_map.h $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/fat12.h $(KERNEL_DIR)/blockdev.h $(KERNEL_DIR)/transient.h spec/dos_structs.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_LOADER_OPT) -DLOADER_MUT_NO_TRANSIENT_GUARD -Ispec -I$(KERNEL_DIR) -c $(KERNEL_LOADER_C) -o $@
$(BUILD)/kernel_flairtenants_mut_noguard.elf: $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/loader_mut_noguard.o $(KERNEL_LD) $(TRANSIENT_IMPORTS_LD) | $(BUILD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/loader_mut_noguard.o @$(TRANSIENT_IMPORTS_LD)
$(BUILD)/kernel_flairtenants_mut_noguard.bin: $(BUILD)/kernel_flairtenants_mut_noguard.elf | $(BUILD)
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); [ "$$sz" -le "$$max" ] || { printf '!!! %s (%s B) exceeds the KERNEL_SECTORS window\n' "$@" "$$sz"; exit 1; }; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$(( max - sz ))" conv=notrunc status=none
	$(call kernel-end-guard,$<,flairtenants-mut-noguard)
$(eval $(call exitdos-transient-rules,noguard,$(BUILD)/kernel_flairtenants_mut_noguard.elf))
EXECGUARD_MUT_NOGUARD_IMG := $(BUILD)/flair_execguard_mut_noguard.img
$(EXECGUARD_MUT_NOGUARD_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/flairovl_kernel_flairtenants_mut_noguard.sys $(BUILD)/flairovl_kernel_flairtenants_mut_noguard.ovl $(BUILD)/transient_noguard.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_mut_noguard.bin,$(BUILD)/transient_noguard.bin)
	@printf ">>> execguard MUTANT image (-DLOADER_MUT_NO_TRANSIENT_GUARD): %s\n" "$@"

# IMPORT_SKEW: the stock flagship kernel + a transient built with
# -DTRANSIENT_LAST_IMPORT_SKEW=16 (os/milton/transient_hdr.asm), linked against
# that same kernel: every other header word is right, only the last import
# (index transient_import_count-1 >= 5) is wrong.
TRANSIENT_HDR_MUT_SKEW_OBJ := $(BUILD)/transient_hdr_mut_skew.o
$(TRANSIENT_HDR_MUT_SKEW_OBJ): $(TRANSIENT_HDR_ASM) $(TRANSIENT_IMPORTS_LD) | $(BUILD)
	$(NASM) -f elf32 -I$(BUILD)/ -DTRANSIENT_LAST_IMPORT_SKEW=16 $< -o $@
$(BUILD)/transient_mut_skew.elf: $(KERNEL_FLAIRTENANTS_ELF) $(TRANSIENT_HDR_MUT_SKEW_OBJ) $(TRANSIENT_CODE_OBJS) $(TRANSIENT_LD) | $(BUILD)
	$(LD) -m elf_i386 -T $(TRANSIENT_LD) --just-symbols=$< -o $@ $(TRANSIENT_HDR_MUT_SKEW_OBJ) $(TRANSIENT_CODE_OBJS)
$(BUILD)/transient_mut_skew.bin: $(BUILD)/transient_mut_skew.elf
	$(OBJCOPY) -O binary $< $@
EXECGUARD_MUT_SKEW_IMG := $(BUILD)/flair_execguard_mut_skew.img
$(EXECGUARD_MUT_SKEW_IMG): $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/flairovl_kernel_flairtenants.sys $(BUILD)/flairovl_kernel_flairtenants.ovl $(BUILD)/transient_mut_skew.bin $(IO_SYS_BIN) $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT) | $(BUILD)
	$(call exitdos-image-recipe,$(KERNEL_FLAIRTENANTS_BIN),$(BUILD)/transient_mut_skew.bin)
	@printf ">>> execguard MUTANT image (transient with its last import skewed): %s\n" "$@"

# $(call execguard-check,<name>) -- the verdict, shared VERBATIM by the gate
# and its mutants. Exit 0 iff green.
define execguard-check
tr -d '\r' < "$(BUILD)/$(1).serial" > "$(BUILD)/$(1).sx"; s="$(BUILD)/$(1).sx"; \
grep -qxF 'FLAIR-EXIT-TO-DOS' $$s && grep -qE '^COMMAND-TRANSIENT-OK bytes=[0-9]+$$' $$s && grep -qxF 'SHELL-READY' $$s || { printf 'EXECGUARD: the desktop never handed over to the prompt (transient + SHELL-READY)\n'; exit 1; }; \
after=$$(sed -n '/^SHELL-READY$$/,$$p' $$s); \
printf '%s\n' "$$after" | grep -qxF 'EXECGUARD-BEGIN' || { printf 'EXECGUARD: EXECGD did not run at the prompt (no EXECGUARD-BEGIN): %s\n' "$$(printf '%s\n' "$$after" | grep -m1 -iE 'bad command|execgd')"; exit 1; }; \
run=$$(printf '%s\n' "$$after" | sed -n '/^EXECGUARD-BEGIN$$/,/^EXECGUARD-END$$/p'); \
printf '%s\n' "$$run" | grep -qxF 'EXECGUARD-CHILD-OK' || { printf 'EXECGUARD: the COMMAND /C child never echoed (EXEC COMMAND.COM did not run the secondary processor): %s\n' "$$(printf '%s\n' "$$run" | grep -m1 '^EXECGUARD-COMSPEC')"; exit 1; }; \
printf '%s\n' "$$run" | grep -qxF 'EXECGUARD-COMSPEC-OK' || { printf 'EXECGUARD: EXEC COMMAND.COM did not return CF clear + rc 0: %s\n' "$$(printf '%s\n' "$$run" | grep -m1 '^EXECGUARD-COMSPEC')"; exit 1; }; \
[ "$$(printf '%s\n' "$$run" | grep -nxE 'EXECGUARD-CHILD-OK|EXECGUARD-COMSPEC-OK' | head -1 | sed 's/:.*//')" -lt "$$(printf '%s\n' "$$run" | grep -nxF 'EXECGUARD-COMSPEC-OK' | sed 's/:.*//')" ] 2>/dev/null || { printf 'EXECGUARD: the child echoed AFTER the EXEC returned\n'; exit 1; }; \
printf '%s\n' "$$run" | grep -qxF 'LOADER-REFUSE-TRANSIENT name=CMD2.COM' || { printf 'EXECGUARD: the loader did not recognise the COMMAND3 overlay (no LOADER-REFUSE-TRANSIENT)\n'; exit 1; }; \
printf '%s\n' "$$run" | grep -qxF 'EXECGUARD-OVERLAY-REFUSED ax=000B' || { printf 'EXECGUARD: EXEC of the overlay copy was not refused with 000Bh: %s\n' "$$(printf '%s\n' "$$run" | grep -m1 '^EXECGUARD-OVERLAY')"; exit 1; }; \
printf '%s\n' "$$run" | grep -qxF 'EXECGUARD-END' || { printf 'EXECGUARD: EXECGD never reached EXECGUARD-END\n'; exit 1; }; \
! grep -qE '^(PANIC|HALTED)' $$s || { printf 'EXECGUARD: %s\n' "$$(grep -m1 -E '^(PANIC|HALTED)' $$s)"; exit 1; }
endef

.PHONY: test-flair-execguard test-flair-execguard-mutant
test-flair-execguard: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_EXECGUARD_IMG)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-flair-execguard : EXEC COMMAND.COM on the flagship (initech-13ig)\n'
	@printf '  COMMAND /C runs the resident transient; the COMMAND3 overlay is never run as a .COM\n'
	@printf '======================================================================\n'
	$(call exitdos-boot,$(FLAIRTENANTS_IMG),$(EXECGUARD_NAME),$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
	@( $(call execguard-check,$(EXECGUARD_NAME)) ) || { printf '!!! test-flair-execguard FAIL: (above)\n'; grep -E '^(FLAIR-EXIT|COMMAND-|SHELL-|EXECGUARD-|LOADER-REFUSE|PANIC|HALTED)' $(BUILD)/$(EXECGUARD_NAME).serial | head -20; exit 1; }
	@printf '>>> test-flair-execguard: EXEC COMMAND.COM " /C ECHO" -> the transient secondary processor echoed, rc 0; EXEC CMD2.COM (the COMMAND3 overlay) -> %s, ax=000B; no PANIC\n' "$$(grep -m1 '^LOADER-REFUSE-TRANSIENT' $(BUILD)/$(EXECGUARD_NAME).sx)"
	@printf '>>> test-flair-execguard: green (QEMU; Bochs leg owed)\n'

test-flair-execguard-mutant: $(HARNESS_BIN) $(EXECGUARD_MUT_NOCOMSPEC_IMG) $(EXECGUARD_MUT_NOGUARD_IMG) $(EXECGUARD_MUT_SKEW_IMG) $(FLAIR_DATA_EXECGUARD_IMG)
	$(call exitdos-boot,$(EXECGUARD_MUT_NOCOMSPEC_IMG),flair_execguard_mut_nocomspec,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
	@out=$$( ( $(call execguard-check,flair_execguard_mut_nocomspec) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'the COMMAND /C child never echoed' || { printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	grep -qxF 'EXECGUARD-COMSPEC-ERR ax=0002' $(BUILD)/flair_execguard_mut_nocomspec.sx || { printf '!!! test-flair-execguard-mutant FAIL: NO_COMSPEC did not fail the EXEC with 0002h (COMMAND.COM absent on A:)\n'; exit 1; }; \
	printf '>>> test-flair-execguard-mutant: NO_COMSPEC correctly RED (%s)\n' "$$out"
	$(call exitdos-boot,$(EXECGUARD_MUT_NOGUARD_IMG),flair_execguard_mut_noguard,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),$(EXECGUARD_DONE),0,40000)
	@out=$$( ( $(call execguard-check,flair_execguard_mut_noguard) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD PASSED -- decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qE 'did not recognise the COMMAND3 overlay|PANIC|HALTED' || { printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	! grep -qxF 'LOADER-REFUSE-TRANSIENT name=CMD2.COM' $(BUILD)/flair_execguard_mut_noguard.sx || { printf '!!! test-flair-execguard-mutant FAIL: NO_GUARD still refused the overlay\n'; exit 1; }; \
	printf '>>> test-flair-execguard-mutant: NO_GUARD correctly RED (%s)\n' "$$out"
	$(call exitdos-boot,$(EXECGUARD_MUT_SKEW_IMG),flair_execguard_mut_skew,$(FLAIR_DATA_EXECGUARD_IMG),$(FLAIR_EXECGUARD_SPEC),HALTED,0,40000)
	@out=$$( ( $(call execguard-check,flair_execguard_mut_skew) ) 2>&1 ); rc=$$?; \
	if [ $$rc -eq 0 ]; then printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW PASSED -- the import check is decoration\n'; exit 1; fi; \
	printf '%s\n' "$$out" | grep -qF 'never handed over to the prompt' || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW RED for the wrong reason: %s\n' "$$out"; exit 1; }; \
	s=$(BUILD)/flair_execguard_mut_skew.sx; k=$$(sed -n 's/^PANIC exit-to-dos: transient linked against another kernel build v=//p' $$s | head -1); \
	[ -n "$$k" ] || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW was not refused as linked against another kernel build: %s\n' "$$(grep -m1 '^PANIC' $$s)"; exit 1; }; \
	last=$$(( $$(wc -l < $(TRANSIENT_IMPORTS_LD).keep) - 1 )); \
	[ "$$(printf '%d' "0x$$k")" -eq $$last ] && [ $$last -ge 5 ] || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW refused on import %s, expected the last (%s, beyond the old five)\n' "$$k" "$$last"; exit 1; }; \
	grep -qxF 'Bad or missing Command Interpreter' $$s && ! grep -qxF 'SHELL-READY' $$s || { printf '!!! test-flair-execguard-mutant FAIL: IMPORT_SKEW did not halt with MSG-DOS-0014 before the prompt\n'; exit 1; }; \
	printf '>>> test-flair-execguard-mutant: IMPORT_SKEW correctly RED (refused on import index %s of %s: %s)\n' "$$k" "$$(( last + 1 ))" "$$(sed -n 's/^\(PANIC exit-to-dos: transient linked[^v]*\).*/\1/p' $$s | head -1)"
	@printf '>>> test-flair-execguard-mutant: green (3/3 RED for the named reason)\n'

TEST_EMU_GATES += test-flair-execguard test-flair-execguard-mutant

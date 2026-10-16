# fcb_tps.mk -- the FCB legacy file API (INT 21h AH=0Fh-24h, beads
# initech-509.9) and the TPS Report Generator that genuinely needs it (beads
# initech-8479.1). Included from the Makefile before harness/fixture_volumes.mk.
#
# THE GOLDEN IS REAL DOS. os/milton/fcbx_program.asm assembles twice: -DREALDOS
# gives a 16-bit .COM that `make fcb-mint-realdos` runs under a real MS-DOS 6.22
# boot diskette in dosbox-x (`FCBX > FCBX.OUT`, then an APM power-off); the
# transcript, the surviving FCB*.DAT listing (mattrib + mdir name/size) and the
# FCBKEPT.DAT bytes it leaves ARE harness/diff/fcb/*_expected.*. The flat-32
# build runs the SAME source the SAME way on InitechOS (QEMU) and must match
# byte for byte. MS-DOS 6.22 is the nearest real DOS on this host; the FCB
# contracts used are the DOS 3.30 ones (IBM 80X0945 pp. 6-67..6-88), and the
# two places the 3.30 prose is ambiguous (hole bytes, read-only rename) are
# settled by the real-DOS bytes (Law 2) and recorded in fcbx_program.asm.
# Rule 6: three int21.c mutants (sequential position never advances; an
# extended FCB's create attribute is dropped; a partial record is not
# zero-filled) must each turn the gate RED; EXT_NOATTR must be caught by the
# mtools FAT differential alone (hidden/read-only bits on disk).

FCB_DIR        := harness/diff/fcb
FCBX_ASM       := $(KERNEL_DIR)/fcbx_program.asm
FCBX_BIN       := $(BUILD)/fcbx_program.bin
FCBX_BIN16     := $(BUILD)/FCBX16.COM
FCBX_BAT       := $(FCB_DIR)/autoexec_fcbx.bat
FCBX_GOLD      := $(FCB_DIR)/fcbx_expected.txt
FCBX_FAT_GOLD  := $(FCB_DIR)/fcbx_fat_expected.txt
FCBX_KEPT_GOLD := $(FCB_DIR)/fcbkept_expected.dat
FCBX_IMG       := $(BUILD)/fcbx_data.img
FCBX_FNAME     := fcbx

$(FCBX_BIN): $(FCBX_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@
$(FCBX_BIN16): $(FCBX_ASM) | $(BUILD)
	$(NASM) -f bin -DREALDOS $< -o $@

# Mutant secondary-shell kernels: one int21.o swapped (pattern: dos_control.mk).
FCB_MUTANTS := seq noattr nozero
FCB_KNOB_seq    := INT21_MUTATE_FCB_SEQ_NO_ADVANCE
FCB_KNOB_noattr := INT21_MUTATE_FCB_EXT_NOATTR
FCB_KNOB_nozero := INT21_MUTATE_FCB_PARTIAL_NOZERO
$(BUILD)/int21_fcb_%.o: $(KERNEL_INT21_C) $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h spec/dos_structs.h $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_INT21_OPT) -D$(FCB_KNOB_$*) -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
define fcb-emu-variant
FCB_OBJS_$(1) := $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_fcb_$(1).o
$(BUILD)/kernel_shell_fcb_$(1).elf: $$(FCB_OBJS_$(1)) $(KERNEL_LD)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -o $$@ $$(FCB_OBJS_$(1))
$(BUILD)/kernel_shell_fcb_$(1).bin: $(BUILD)/kernel_shell_fcb_$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@set -e; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS) * 512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$(( max - sz ))" conv=notrunc status=none
	$$(call kernel-end-guard,$$<,fcb-$(1))
$(BUILD)/fcb_$(1).img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_shell_fcb_$(1).bin
	@dd if=/dev/zero of=$$@ bs=512 count=$(IMG_SECTORS) status=none
	@dd if=$(MBR_BIN) of=$$@ bs=512 seek=0 conv=notrunc status=none
	@dd if=$(STAGE2_BIN) of=$$@ bs=512 seek=1 conv=notrunc status=none
	@dd if=$(BUILD)/kernel_shell_fcb_$(1).bin of=$$@ bs=512 seek=17 conv=notrunc status=none
endef
$(foreach m,$(FCB_MUTANTS),$(eval $(call fcb-emu-variant,$(m))))

# $(1) data image, $(2) boot image, $(3) run name. Mints a fresh 1.44 MB data
# volume (same geometry as the real-DOS diskette: 1Bh/1Ch report it), boots the
# COMMAND.COM shell, lets AUTOEXEC.BAT run `FCBX > FCBX.OUT`, then types EXIT.
define fcbx-run
	@dd if=/dev/zero of=$(1) bs=512 count=2880 status=none
	@mformat -i $(1) -f 1440 ::
	@mcopy -i $(1) $(FCBX_BAT) ::AUTOEXEC.BAT
	@mcopy -i $(1) $(FCBX_BIN) ::FCBX.COM
	@$(call fixture-check,recipe,$(1),AUTOEXEC.BAT FCBX.COM)
	@$(HARNESS_BIN) --disk "$(2)" --disk2 "$(1)" \
		--name "$(3)" --out "$(BUILD)" --timeout-ms 30000 \
		--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
		$(SHELL_OS_COMPLETION) 2> "$(BUILD)/$(3).report" || true
	@mtype -i $(1) ::FCBX.OUT 2>/dev/null | tr -d '\r' > "$(BUILD)/$(3).got" || true
	@{ mattrib -i $(1) '::FCB*.DAT' | sed 's/  */ /g' | LC_ALL=C sort; \
	   mdir -a -i $(1) :: | awk '$$1 ~ /^FCB/ && $$2 == "DAT" {print $$1, $$2, $$3}' | LC_ALL=C sort; } \
		> "$(BUILD)/$(3).fat" 2>/dev/null || true
	@mtype -i $(1) ::FCBKEPT.DAT > "$(BUILD)/$(3).kept" 2>/dev/null || true
endef

.PHONY: test-fcb-api test-fcb-api-mutant fcb-mint-realdos
test-fcb-api: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(FCBX_BIN) $(FCBX_BAT) $(FCBX_GOLD) $(FCBX_FAT_GOLD) $(FCBX_KEPT_GOLD)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-fcb-api : FCB legacy API 0Fh-24h (initech-509.9)\n'
	@printf '  FCBX > FCBX.OUT on InitechOS == the SAME program under real MS-DOS 6.22.\n'
	@printf '======================================================================\n'
	@command -v mformat >/dev/null 2>&1 && command -v mattrib >/dev/null 2>&1 || { printf '!!! test-fcb-api FAIL: mtools missing\n'; exit 1; }
	$(call fcbx-run,$(FCBX_IMG),$(SECONDARY_TRACER_IMG),$(FCBX_FNAME))
	@if grep -q 'triple_fault=1' "$(BUILD)/$(FCBX_FNAME).report"; then printf '!!! test-fcb-api FAIL: TRIPLE FAULT\n'; exit 1; fi
	@grep -q '^SHELL-EXIT$$' "$(BUILD)/$(FCBX_FNAME).serial" || { printf '!!! test-fcb-api FAIL: SHELL-EXIT missing (FCBX hung or panicked)\n'; exit 1; }
	@printf '>>> test-fcb-api [1/4]: booted, FCBX ran, EXIT reached the REPL\n'
	@diff "$(FCBX_GOLD)" "$(BUILD)/$(FCBX_FNAME).got" || { printf '!!! test-fcb-api FAIL: FCBX transcript != the real MS-DOS 6.22 golden (root-cause do_fcb in int21.c, Rule 3)\n'; exit 1; }
	@printf '>>> test-fcb-api [2/4]: transcript == real-DOS golden (%s lines, T01..T20)\n' "$$(wc -l < $(FCBX_GOLD))"
	@diff "$(FCBX_FAT_GOLD)" "$(BUILD)/$(FCBX_FNAME).fat" || { printf '!!! test-fcb-api FAIL: surviving FCB*.DAT set/attributes/sizes != real DOS (mtools FAT differential)\n'; exit 1; }
	@printf '>>> test-fcb-api [3/4]: mtools FAT differential: FCB*.DAT names/attrs/sizes == real DOS\n'
	@cmp "$(FCBX_KEPT_GOLD)" "$(BUILD)/$(FCBX_FNAME).kept" || { printf '!!! test-fcb-api FAIL: FCBKEPT.DAT bytes != real DOS\n'; exit 1; }
	@printf '>>> test-fcb-api [4/4]: FCBKEPT.DAT (sequential + random 10-byte records) byte-identical to real DOS\n'
	@printf 'VERDICT   : PASS -- the FCB API agrees with real DOS (QEMU only; Bochs/86Box legs not run)\n'
	@printf '======================================================================\n'

test-fcb-api-mutant: $(HARNESS_BIN) $(FCBX_BIN) $(FCBX_BAT) $(FCBX_GOLD) $(FCBX_FAT_GOLD) $(FCBX_KEPT_GOLD) $(addprefix $(BUILD)/fcb_,$(addsuffix .img,$(FCB_MUTANTS)))
	$(call fcbx-run,$(BUILD)/fcbx_data_seq.img,$(BUILD)/fcb_seq.img,fcbx_mut_seq)
	$(call fcbx-run,$(BUILD)/fcbx_data_noattr.img,$(BUILD)/fcb_noattr.img,fcbx_mut_noattr)
	$(call fcbx-run,$(BUILD)/fcbx_data_nozero.img,$(BUILD)/fcb_nozero.img,fcbx_mut_nozero)
	@set -e; for m in $(FCB_MUTANTS); do \
	  t=0; f=0; diff -q $(FCBX_GOLD) $(BUILD)/fcbx_mut_$$m.got >/dev/null 2>&1 || t=1; \
	  { diff -q $(FCBX_FAT_GOLD) $(BUILD)/fcbx_mut_$$m.fat && cmp -s $(FCBX_KEPT_GOLD) $(BUILD)/fcbx_mut_$$m.kept; } >/dev/null 2>&1 || f=1; \
	  if [ $$t = 0 ] && [ $$f = 0 ]; then printf '!!! test-fcb-api-mutant FAIL: mutant %s matched every golden -- the gate is decoration\n' $$m; exit 1; fi; \
	  if [ $$m = noattr ] && [ $$f = 0 ]; then printf '!!! test-fcb-api-mutant FAIL: noattr not caught by the FAT differential alone\n'; exit 1; fi; \
	  printf '>>> test-fcb-api-mutant: %s correctly RED (transcript-red=%s fat-red=%s): ' $$m $$t $$f; \
	  diff $(FCBX_GOLD) $(BUILD)/fcbx_mut_$$m.got | sed -n 2p; [ -n "$$(diff $(FCBX_GOLD) $(BUILD)/fcbx_mut_$$m.got)" ] || printf '\n'; \
	done
	@printf '>>> test-fcb-api-mutant: green (3/3 int21.c FCB mutants RED)\n'

# Re-mint the real-DOS golden (NOT part of `make test`; needs dosbox-x and a
# real MS-DOS boot diskette image: DOS622_IMG=/path/to/dos622.img). The run
# diskette is the boot diskette with its CONFIG.SYS/AUTOEXEC.BAT replaced and
# large unused files removed for room; OFF.COM is the APM power-off that ends
# dosbox-x cleanly so the image is flushed.
DOS622_IMG ?= ../lotus123-decomp/mint/dos622.img
FCB_MINT_DIR := $(BUILD)/fcb_mint
fcb-mint-realdos: $(FCBX_BIN16)
	@test -f "$(DOS622_IMG)" || { printf '!!! fcb-mint-realdos: DOS622_IMG=%s not found\n' "$(DOS622_IMG)"; exit 1; }
	@command -v dosbox-x >/dev/null 2>&1 || { printf '!!! fcb-mint-realdos: dosbox-x missing\n'; exit 1; }
	@mkdir -p $(FCB_MINT_DIR)
	@printf 'bits 16\norg 0x100\nmov ax,0x5301\nxor bx,bx\nint 0x15\nmov ax,0x530E\nxor bx,bx\nmov cx,0x0102\nint 0x15\nmov ax,0x5307\nmov bx,1\nmov cx,3\nint 0x15\nmov ax,0x4C00\nint 0x21\n' > $(FCB_MINT_DIR)/off.asm
	@$(NASM) -f bin $(FCB_MINT_DIR)/off.asm -o $(FCB_MINT_DIR)/OFF.COM
	@cp -f "$(DOS622_IMG)" $(FCB_MINT_DIR)/run.img
	@mdel -i $(FCB_MINT_DIR)/run.img ::QBASIC.EXE ::EMM386.EXE ::MSCDEX.EXE ::CD2.SYS ::CD3.SYS 2>/dev/null || true
	@printf 'FILES=30\r\n' > $(FCB_MINT_DIR)/CONFIG.SYS
	@printf '@ECHO OFF\r\nFCBX > FCBX.OUT\r\nOFF\r\n' > $(FCB_MINT_DIR)/AUTOEXEC.BAT
	@mcopy -o -i $(FCB_MINT_DIR)/run.img $(FCB_MINT_DIR)/CONFIG.SYS ::CONFIG.SYS
	@mcopy -o -i $(FCB_MINT_DIR)/run.img $(FCB_MINT_DIR)/AUTOEXEC.BAT ::AUTOEXEC.BAT
	@mcopy -o -i $(FCB_MINT_DIR)/run.img $(FCBX_BIN16) ::FCBX.COM
	@mcopy -o -i $(FCB_MINT_DIR)/run.img $(FCB_MINT_DIR)/OFF.COM ::OFF.COM
	@printf '[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=8\nquit warning=false\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[autoexec]\nimgmount a %s -t floppy\nboot a:\n' "$(abspath $(FCB_MINT_DIR))/run.img" > $(FCB_MINT_DIR)/run.conf
	@SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 120 dosbox-x -conf $(FCB_MINT_DIR)/run.conf -nogui -nomenu > $(FCB_MINT_DIR)/dosbox.log 2>&1 || true
	@mtype -i $(FCB_MINT_DIR)/run.img ::FCBX.OUT | tr -d '\r' > $(FCB_MINT_DIR)/fcbx_expected.txt
	@{ mattrib -i $(FCB_MINT_DIR)/run.img '::FCB*.DAT' | sed 's/  */ /g' | LC_ALL=C sort; \
	   mdir -a -i $(FCB_MINT_DIR)/run.img :: | awk '$$1 ~ /^FCB/ && $$2 == "DAT" {print $$1, $$2, $$3}' | LC_ALL=C sort; } > $(FCB_MINT_DIR)/fcbx_fat_expected.txt
	@mtype -i $(FCB_MINT_DIR)/run.img ::FCBKEPT.DAT > $(FCB_MINT_DIR)/fcbkept_expected.dat
	@for f in fcbx_expected.txt fcbx_fat_expected.txt fcbkept_expected.dat; do \
	  cmp -s $(FCB_MINT_DIR)/$$f $(FCB_DIR)/$$f && printf '>>> fcb-mint-realdos: %s reproduces the committed golden\n' $$f \
	  || printf '!!! fcb-mint-realdos: %s DIFFERS from the committed golden (%s)\n' $$f $(FCB_MINT_DIR)/$$f; done

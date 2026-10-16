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
	$(NASM) -I$(BUILD)/ -f bin $< -o $@
$(FCBX_BIN16): $(FCBX_ASM) | $(BUILD)
	$(NASM) -I$(BUILD)/ -f bin -DREALDOS $< -o $@

# Mutant secondary-shell kernels: one int21.o swapped (pattern: dos_control.mk).
FCB_MUTANTS := seq noattr nozero
FCB_KNOB_seq    := INT21_MUTATE_FCB_SEQ_NO_ADVANCE
FCB_KNOB_noattr := INT21_MUTATE_FCB_EXT_NOATTR
FCB_KNOB_nozero := INT21_MUTATE_FCB_PARTIAL_NOZERO
$(BUILD)/int21_fcb_%.o: $(KERNEL_INT21_C) $(KERNEL_DIR)/int21.h $(KERNEL_DIR)/sft.h $(KERNEL_DIR)/psp.h spec/dos_structs.h $(DOS_MESSAGES_H) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_INT21_OPT) -D$(FCB_KNOB_$*) -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
define fcb-emu-variant
FCB_OBJS_$(1) := $(filter-out $(KERNEL_INT21_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/int21_fcb_$(1).o
$(BUILD)/kernel_shell_fcb_$(1).elf: $$(FCB_OBJS_$(1)) $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $$(FCB_OBJS_$(1))
$(BUILD)/kernel_shell_fcb_$(1).bin: $(BUILD)/kernel_shell_fcb_$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@set -e; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS) * 512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$(( max - sz ))" conv=notrunc status=none
	$$(call kernel-end-guard,$$<,fcb-$(1))
$(BUILD)/fcb_$(1).img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_shell_fcb_$(1).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_shell_fcb_$(1).bin $(BOOT_SYSTEM_TOOL)
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
# real MS-DOS boot diskette image: DOS622_IMG=/path/to/dos622.img). See
# harness/emu/realdos_run.sh for how the boot diskette is prepared and ended.
DOS622_IMG ?= ../lotus123-decomp/mint/dos622.img
FCB_MINT_DIR := $(BUILD)/fcb_mint
fcb-mint-realdos: $(FCBX_BIN16)
	@test -f "$(DOS622_IMG)" || { printf '!!! fcb-mint-realdos: DOS622_IMG=%s not found\n' "$(DOS622_IMG)"; exit 1; }
	@command -v dosbox-x >/dev/null 2>&1 || { printf '!!! fcb-mint-realdos: dosbox-x missing\n'; exit 1; }
	@mkdir -p $(FCB_MINT_DIR)
	@printf 'FCBX > FCBX.OUT\n' > $(FCB_MINT_DIR)/lines
	@harness/emu/realdos_run.sh "$(DOS622_IMG)" $(FCB_MINT_DIR)/run.img $(FCB_MINT_DIR)/lines $(FCBX_BIN16)=FCBX.COM
	@mtype -i $(FCB_MINT_DIR)/run.img ::FCBX.OUT | tr -d '\r' > $(FCB_MINT_DIR)/fcbx_expected.txt
	@{ mattrib -i $(FCB_MINT_DIR)/run.img '::FCB*.DAT' | sed 's/  */ /g' | LC_ALL=C sort; \
	   mdir -a -i $(FCB_MINT_DIR)/run.img :: | awk '$$1 ~ /^FCB/ && $$2 == "DAT" {print $$1, $$2, $$3}' | LC_ALL=C sort; } > $(FCB_MINT_DIR)/fcbx_fat_expected.txt
	@mtype -i $(FCB_MINT_DIR)/run.img ::FCBKEPT.DAT > $(FCB_MINT_DIR)/fcbkept_expected.dat
	@for f in fcbx_expected.txt fcbx_fat_expected.txt fcbkept_expected.dat; do \
	  cmp -s $(FCB_MINT_DIR)/$$f $(FCB_DIR)/$$f && printf '>>> fcb-mint-realdos: %s reproduces the committed golden\n' $$f \
	  || printf '!!! fcb-mint-realdos: %s DIFFERS from the committed golden (%s)\n' $$f $(FCB_MINT_DIR)/$$f; done

# =============================================================================
# TPS REPORT GENERATOR (beads initech-8479.1) -- run FROM THE DOS PROMPT.
# os/milton/tpsrep_program.asm, same two-build pattern: `make tps-mint-realdos`
# runs the 16-bit build under real MS-DOS 6.22 (keys from a redirected stdin;
# AH=08h reads stdin there) and its transcripts + report file ARE the goldens
# in harness/diff/tps. On InitechOS the SAME keystrokes are typed through QMP
# (AH=08h reads the keyboard; nothing is echoed, so the transcript does not
# depend on the input path). The table harness/diff/tps/tps.dbf was WRITTEN BY
# REAL dBASE III PLUS 1.1 (../dbase3-decomp/mint/work DBASE.EXE under dosbox-x)
# running harness/diff/tps/tpsmk.prg; only header byte 1 (last-update year,
# a non-meaningful byte per CLAUDE.md's .dbf note) was normalized 1Ah -> 59h
# so the cover sheet reads 10/16/89 instead of the mint host's year.
# Legs: [A] the full operating procedure typed at the A:\> prompt -- every
# refusal of spec/tps_ritual.json R2-R4 exercised once, then satisfied -- the
# console transcript AND TPSREPT.TXT (FCB-written, Ctrl-Z padded) byte-equal
# real DOS, TPSREPT.BAK == the previous report (FCB rename), the older .BAK
# deleted; [B] no TPSPARM.DAT -> TPS-001 + ERRORLEVEL 1; [C] cover sheet N ->
# TPS-005 + ERRORLEVEL 5 and no report file; [spec] every literal of the
# locked spec appears in the program, and the source issues no handle call;
# [tacit] the R2 prompt never states the count or the key (operator Law-4
# ruling 2026-10-17, beads initech-5igf: the 7 presses of the SPACE BAR are
# written only in the Operating Procedures, OP-7.4) -- checked on the locked
# JSON and on every R2 prompt the program actually printed in leg A, and the
# prompt is reprinted once per refused count.
# Rule 6: TPS_MUTATE_COUNT6 (six presses accepted) must turn leg A RED, and so
# must the FCB kernel mutant `seq` (the report depends on the FCB layer), and
# TPS_MUTATE_SAY7 (the count stated in the R2 prompt) must turn [tacit] RED.
TPS_DIR        := harness/diff/tps
TPSREP_ASM     := $(KERNEL_DIR)/tpsrep_program.asm
TPSREP_BIN     := $(FLAIR_TPSREP_COM)
TPSREP_MUT_BIN := $(BUILD)/tpsrep_program_count6.bin
TPSREP_SAY7_BIN := $(BUILD)/tpsrep_program_say7.bin
TPSREP_BIN16   := $(BUILD)/TPSREP16.COM
TPS_SPEC       := spec/tps_ritual.json
TPS_FIXTURES   := $(TPS_DIR)/tps.dbf $(TPS_DIR)/tpsparm.dat $(TPS_DIR)/old_tpsrept.txt \
                  $(TPS_DIR)/old_tpsrept.bak $(TPS_DIR)/keys_a.txt $(TPS_DIR)/keys_c.txt \
                  $(TPS_DIR)/autoexec_b.bat $(TPS_DIR)/autoexec_c.bat
TPS_GOLDENS    := $(TPS_DIR)/tps_a_expected.txt $(TPS_DIR)/tpsrept_expected.txt \
                  $(TPS_DIR)/tps_b_expected.txt $(TPS_DIR)/tps_c_expected.txt
TPS_IMG_A      := $(BUILD)/tps_a.img
TPS_IMG_B      := $(BUILD)/tps_b.img
TPS_IMG_C      := $(BUILD)/tps_c.img

$(TPSREP_BIN): $(TPSREP_ASM) | $(BUILD)
	$(NASM) -f bin $< -o $@
$(TPSREP_MUT_BIN): $(TPSREP_ASM) | $(BUILD)
	$(NASM) -f bin -DTPS_MUTATE_COUNT6 $< -o $@
$(TPSREP_SAY7_BIN): $(TPSREP_ASM) | $(BUILD)
	$(NASM) -f bin -DTPS_MUTATE_SAY7 $< -o $@
$(TPSREP_BIN16): $(TPSREP_ASM) | $(BUILD)
	$(NASM) -f bin -DREALDOS $< -o $@

# keys file (raw bytes) -> QMP tokens: space spc, CR ret, '-' minus.
tps-keys = $(shell od -An -tx1 -v $(1) | tr -s ' \n' '\n\n' | sed '/^$$/d' | \
	awk 'BEGIN{for (i = 48; i < 123; i++) c[sprintf("%02x", i)] = sprintf("%c", i)} \
	{h=$$1; if (h=="20") t="spc"; else if (h=="0d") t="ret"; else if (h=="2d") t="minus"; \
	else t=c[h]; printf "%s%s", (NR>1?",":""), t}')
TPS_TYPE_CMD := t,p,s,r,e,p,spc,t,p,s,dot,d,b,f,ret
TPS_EXIT_KEYS := e,x,i,t,ret

# $(1) image $(2) boot image $(3) run name $(4) program bin $(5) keys
# $(6) extra mcopy pairs host=DOS (space separated)
define tps-run
	@dd if=/dev/zero of=$(1) bs=512 count=2880 status=none
	@mformat -i $(1) -f 1440 ::
	@mcopy -i $(1) $(4) ::TPSREP.COM
	@mcopy -i $(1) $(TPS_DIR)/tps.dbf ::TPS.DBF
	@for p in $(6); do mcopy -i $(1) "$${p%%=*}" "::$${p#*=}"; done
	@$(call fixture-check,recipe,$(1),TPSREP.COM TPS.DBF $(foreach p,$(6),$(lastword $(subst =, ,$(p)))))
	@$(HARNESS_BIN) --disk "$(2)" --disk2 "$(1)" \
		--name "$(3)" --out "$(BUILD)" --timeout-ms 40000 \
		--keys "$(5)" --keys-after "SHELL-READY" \
		$(SHELL_OS_COMPLETION) 2> "$(BUILD)/$(3).report" || true
	@tr -d '\r' < "$(BUILD)/$(3).serial" | sed -n '/^INITECH CORPORATION$$/,/^\(TPS-LEG-END\|SHELL-EXIT\)$$/p' | \
		sed '/^TPS-LEG-END$$/d;/^SHELL-EXIT$$/d;/^A:\\>/d' > "$(BUILD)/$(3).got"
	@mtype -i $(1) ::TPSREPT.TXT > "$(BUILD)/$(3).rept" 2>/dev/null || rm -f "$(BUILD)/$(3).rept"
	@mtype -i $(1) ::TPSREPT.BAK > "$(BUILD)/$(3).bak" 2>/dev/null || rm -f "$(BUILD)/$(3).bak"
endef

TPS_A_KEYS = $(TPS_TYPE_CMD),$(call tps-keys,$(TPS_DIR)/keys_a.txt),e,x,i,t,ret
TPS_C_KEYS = $(call tps-keys,$(TPS_DIR)/keys_c.txt),e,x,i,t,ret

# leg A checks: $(1) run name. Exit 0 = green.
define tps-check-a
diff -q $(TPS_DIR)/tps_a_expected.txt $(BUILD)/$(1).got >/dev/null 2>&1 && \
cmp -s $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/$(1).rept && \
cmp -s $(TPS_DIR)/old_tpsrept.txt $(BUILD)/$(1).bak
endef

# [tacit] check: $(1) run name. Exit 0 = green. Every R2 prompt span the program
# printed (TOP OF FORM ALIGNMENT .. the line ending PRESS ENTER.) is free of the
# digit 7 and of the word SPACE, and there is one span per attempt (the refusals
# + the accepted one): the count and the key live only in the manual (OP-7.4).
define tps-check-tacit
sed -n '/^TOP OF FORM ALIGNMENT/,/PRESS ENTER\.$$/p' $(BUILD)/$(1).got > $(BUILD)/$(1).r2span && test -s $(BUILD)/$(1).r2span && ! grep -qE '7|SPACE' $(BUILD)/$(1).r2span && test "$$(grep -c '^TOP OF FORM ALIGNMENT' $(BUILD)/$(1).got)" -eq "$$(( $$(grep -c '^FORM NOT ALIGNED' $(BUILD)/$(1).got) + 1 ))"
endef

.PHONY: test-tps-report test-tps-report-mutant tps-mint-realdos
test-tps-report: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(TPSREP_BIN) $(TPS_FIXTURES) $(TPS_GOLDENS) $(TPS_SPEC)
	@printf '======================================================================\n'
	@printf 'InitechOS (STAPLER) -- make test-tps-report : TPS Report Generator (initech-8479.1)\n'
	@printf '  typed at A:\\> ; every file byte through the FCB API ; goldens = real MS-DOS 6.22\n'
	@printf '======================================================================\n'
	@set -e; n=0; awk '{ line = $$0; while (1) { \
	  if (!f) { if (match(line, /"(prompt|refuse|accept|refuse_missing_key)": *\[/)) { f = 1; line = substr(line, RSTART + RLENGTH) } else break } \
	  e = index(line, "]"); q = match(line, /"[^"]*"/); \
	  if (q && (e == 0 || RSTART < e)) { print substr(line, RSTART + 1, RLENGTH - 2); line = substr(line, RSTART + RLENGTH) } \
	  else if (e) { f = 0; line = substr(line, e + 1) } else break } }' $(TPS_SPEC) > $(BUILD)/tps_spec_literals.txt; \
	sed -n 's/.*"end_line": "\(.*\)".*/\1/p' $(TPS_SPEC) >> $(BUILD)/tps_spec_literals.txt; \
	while IFS= read -r lit; do printf '%s\n' "$$lit" | sed 's/{n}/\n/g' | while IFS= read -r frag; do \
	  [ -z "$$frag" ] && continue; grep -aqF -- "$$frag" $(TPSREP_BIN) || { printf '!!! test-tps-report FAIL: spec literal fragment "%s" not in TPSREP.COM\n' "$$frag"; exit 1; }; done; n=$$((n+1)); \
	done < $(BUILD)/tps_spec_literals.txt; \
	[ $$(wc -l < $(BUILD)/tps_spec_literals.txt) -ge 14 ] || { printf '!!! test-tps-report FAIL: spec literal extraction found too few rows\n'; exit 1; }; \
	! grep -nE '^\s*mov\s+ah,\s*0x(3[C-F]|4[0-6])\b' $(TPSREP_ASM) || { printf '!!! test-tps-report FAIL: TPSREP issues a handle call\n'; exit 1; }; \
	printf '>>> test-tps-report [spec]: %s locked literals present in TPSREP.COM; no handle call (AH=3Ch-46h) in the source\n' "$$(wc -l < $(BUILD)/tps_spec_literals.txt)"
	@set -e; awk '/"id": *"R2"/ { r = 1; next } /"id": *"/ { r = 0 } r && /"prompt": *\[/ { p = 1; sub(/.*"prompt": *\[/, "") } p { print } p && /\]/ { p = 0 }' $(TPS_SPEC) | \
	  grep -oE '"[^"]*"' > $(BUILD)/tps_spec_r2_prompt.txt; \
	[ $$(wc -l < $(BUILD)/tps_spec_r2_prompt.txt) -eq 3 ] || { printf '!!! test-tps-report FAIL [tacit]: spec R2 prompt is not the three-line alignment prompt\n'; exit 1; }; \
	! grep -nE '7|SPACE' $(BUILD)/tps_spec_r2_prompt.txt || { printf '!!! test-tps-report FAIL [tacit]: the locked R2 prompt states the count or the key (initech-5igf)\n'; exit 1; }; \
	grep -qE '"tacit": *"[^"]*OP-7\.4' $(TPS_SPEC) || { printf '!!! test-tps-report FAIL [tacit]: spec R2 lost its tacit row (the count lives in OP-7.4)\n'; exit 1; }; \
	printf '>>> test-tps-report [tacit/spec]: the locked R2 prompt (3 lines) names neither the count nor the key; the count is the manual'"'"'s (OP-7.4)\n'
	$(call tps-run,$(TPS_IMG_A),$(SECONDARY_TRACER_IMG),tps_a,$(TPSREP_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
	@grep -q '^SHELL-EXIT' $(BUILD)/tps_a.serial || { printf '!!! test-tps-report FAIL [A]: the prompt never came back (TPSREP hung?)\n'; exit 1; }
	@grep -q 'tpsrep tps.dbf' $(BUILD)/tps_a.serial || { printf '!!! test-tps-report FAIL [A]: the command was not typed at the prompt\n'; exit 1; }
	@diff $(TPS_DIR)/tps_a_expected.txt $(BUILD)/tps_a.got || { printf '!!! test-tps-report FAIL [A]: console transcript != real DOS\n'; exit 1; }
	@cmp $(TPS_DIR)/tpsrept_expected.txt $(BUILD)/tps_a.rept || { printf '!!! test-tps-report FAIL [A]: TPSREPT.TXT != real DOS (FCB sequential writes)\n'; exit 1; }
	@cmp $(TPS_DIR)/old_tpsrept.txt $(BUILD)/tps_a.bak || { printf '!!! test-tps-report FAIL [A]: TPSREPT.BAK is not the previous report (FCB delete + rename)\n'; exit 1; }
	@printf '>>> test-tps-report [A]: typed at A:\\>, ritual refused 6/8 presses, mismatched number, wrong memo, then ran; transcript (%s lines) + TPSREPT.TXT (%s B) == real DOS; previous report kept as .BAK\n' "$$(wc -l < $(BUILD)/tps_a.got)" "$$(wc -c < $(BUILD)/tps_a.rept)"
	@$(call tps-check-tacit,tps_a) || { printf '!!! test-tps-report FAIL [tacit]: an R2 prompt the program printed states the count or the key, or was not reprinted per refusal (%s)\n' $(BUILD)/tps_a.r2span; exit 1; }
	@printf '>>> test-tps-report [tacit/A]: %s R2 prompts printed (one per attempt), none states the count or the key\n' "$$(grep -c '^TOP OF FORM ALIGNMENT' $(BUILD)/tps_a.got)"
	$(call tps-run,$(TPS_IMG_B),$(SECONDARY_TRACER_IMG),tps_b,$(TPSREP_BIN),$(TPS_EXIT_KEYS),$(TPS_DIR)/autoexec_b.bat=AUTOEXEC.BAT)
	@diff $(TPS_DIR)/tps_b_expected.txt $(BUILD)/tps_b.got || { printf '!!! test-tps-report FAIL [B]: missing-parameter-file transcript/ERRORLEVEL != real DOS\n'; exit 1; }
	@printf '>>> test-tps-report [B]: no TPSPARM.DAT -> TPS-001 / OP-7.3, ERRORLEVEL 1 (== real DOS)\n'
	$(call tps-run,$(TPS_IMG_C),$(SECONDARY_TRACER_IMG),tps_c,$(TPSREP_BIN),$(TPS_C_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/autoexec_c.bat=AUTOEXEC.BAT)
	@diff $(TPS_DIR)/tps_c_expected.txt $(BUILD)/tps_c.got || { printf '!!! test-tps-report FAIL [C]: cover-sheet-N transcript/ERRORLEVEL != real DOS\n'; exit 1; }
	@! test -f $(BUILD)/tps_c.rept || { printf '!!! test-tps-report FAIL [C]: a report was written although the cover sheet was not attached\n'; exit 1; }
	@printf '>>> test-tps-report [C]: cover sheet N -> TPS-005 memo reference, ERRORLEVEL 5, no TPSREPT.TXT (== real DOS)\n'
	@printf 'VERDICT   : PASS -- the TPS Report Generator works from the prompt through the FCB API (QEMU only)\n'
	@printf '======================================================================\n'

test-tps-report-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/fcb_seq.img $(TPSREP_BIN) $(TPSREP_MUT_BIN) $(TPSREP_SAY7_BIN) $(TPS_FIXTURES) $(TPS_GOLDENS)
	$(call tps-run,$(BUILD)/tps_mut6.img,$(SECONDARY_TRACER_IMG),tps_mut6,$(TPSREP_MUT_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
	@if $(call tps-check-a,tps_mut6); then printf '!!! test-tps-report-mutant FAIL: COUNT6 (six presses accepted) matched every golden\n'; exit 1; fi
	@printf '>>> test-tps-report-mutant: COUNT6 correctly RED: '; diff $(TPS_DIR)/tps_a_expected.txt $(BUILD)/tps_mut6.got | sed -n '2p'
	$(call tps-run,$(BUILD)/tps_mutseq.img,$(BUILD)/fcb_seq.img,tps_mutseq,$(TPSREP_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
	@if $(call tps-check-a,tps_mutseq); then printf '!!! test-tps-report-mutant FAIL: FCB kernel mutant seq matched every golden\n'; exit 1; fi
	@printf '>>> test-tps-report-mutant: FCB kernel mutant seq correctly RED (TPSREPT.TXT %s B vs golden %s B)\n' "$$(wc -c < $(BUILD)/tps_mutseq.rept 2>/dev/null || echo 0)" "$$(wc -c < $(TPS_DIR)/tpsrept_expected.txt)"
	$(call tps-run,$(BUILD)/tps_say7.img,$(SECONDARY_TRACER_IMG),tps_say7,$(TPSREP_SAY7_BIN),$(TPS_A_KEYS),$(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK)
	@grep -q '^FORM ALIGNED\.$$' $(BUILD)/tps_say7.got || { printf '!!! test-tps-report-mutant FAIL: SAY7 did not run the ritual (RED for the wrong reason)\n'; exit 1; }
	@if $(call tps-check-tacit,tps_say7); then printf '!!! test-tps-report-mutant FAIL: SAY7 (the count stated in the R2 prompt) passed [tacit] -- the tacit rule is decoration\n'; exit 1; fi
	@printf '>>> test-tps-report-mutant: SAY7 correctly RED on [tacit]: %s\n' "$$(grep -m1 -E '7|SPACE' $(BUILD)/tps_say7.r2span)"
	@printf '>>> test-tps-report-mutant: green (3/3 mutants RED)\n'

# Re-mint the TPS goldens under real MS-DOS 6.22 (NOT part of `make test`).
TPS_MINT_DIR := $(BUILD)/tps_mint
tps-mint-realdos: $(TPSREP_BIN16) $(TPS_FIXTURES)
	@test -f "$(DOS622_IMG)" || { printf '!!! tps-mint-realdos: DOS622_IMG=%s not found\n' "$(DOS622_IMG)"; exit 1; }
	@mkdir -p $(TPS_MINT_DIR)
	@printf 'TPSREP TPS.DBF < KEYSA.TXT > TPSA.OUT\n' > $(TPS_MINT_DIR)/a.lines
	@harness/emu/realdos_run.sh "$(DOS622_IMG)" $(TPS_MINT_DIR)/a.img $(TPS_MINT_DIR)/a.lines \
		$(TPSREP_BIN16)=TPSREP.COM $(TPS_DIR)/tps.dbf=TPS.DBF $(TPS_DIR)/tpsparm.dat=TPSPARM.DAT \
		$(TPS_DIR)/old_tpsrept.txt=TPSREPT.TXT $(TPS_DIR)/old_tpsrept.bak=TPSREPT.BAK $(TPS_DIR)/keys_a.txt=KEYSA.TXT
	@mtype -i $(TPS_MINT_DIR)/a.img ::TPSA.OUT | tr -d '\r' > $(TPS_MINT_DIR)/tps_a_expected.txt
	@mtype -i $(TPS_MINT_DIR)/a.img ::TPSREPT.TXT > $(TPS_MINT_DIR)/tpsrept_expected.txt
	@mtype -i $(TPS_MINT_DIR)/a.img ::TPSREPT.BAK | cmp -s - $(TPS_DIR)/old_tpsrept.txt || printf '!!! tps-mint-realdos: real DOS did not keep the previous report as .BAK\n'
	@tr -d '\r' < $(TPS_DIR)/autoexec_b.bat | sed -e 's/^TPSREP TPS.DBF$$/TPSREP TPS.DBF > TPSB.OUT/' -e 's/^IF \(.*\) ECHO \(.*\)$$/IF \1 ECHO \2>> TPSB.OUT/' -e '/^@ECHO OFF/d;/TPS-LEG-END/d' > $(TPS_MINT_DIR)/b.lines
	@harness/emu/realdos_run.sh "$(DOS622_IMG)" $(TPS_MINT_DIR)/b.img $(TPS_MINT_DIR)/b.lines \
		$(TPSREP_BIN16)=TPSREP.COM $(TPS_DIR)/tps.dbf=TPS.DBF
	@mtype -i $(TPS_MINT_DIR)/b.img ::TPSB.OUT | tr -d '\r' > $(TPS_MINT_DIR)/tps_b_expected.txt
	@tr -d '\r' < $(TPS_DIR)/autoexec_c.bat | sed -e 's/^TPSREP TPS.DBF$$/TPSREP TPS.DBF < KEYSC.TXT > TPSC.OUT/' -e 's/^IF \(.*\) ECHO \(.*\)$$/IF \1 ECHO \2>> TPSC.OUT/' -e '/^@ECHO OFF/d;/TPS-LEG-END/d' > $(TPS_MINT_DIR)/c.lines
	@harness/emu/realdos_run.sh "$(DOS622_IMG)" $(TPS_MINT_DIR)/c.img $(TPS_MINT_DIR)/c.lines \
		$(TPSREP_BIN16)=TPSREP.COM $(TPS_DIR)/tps.dbf=TPS.DBF $(TPS_DIR)/tpsparm.dat=TPSPARM.DAT $(TPS_DIR)/keys_c.txt=KEYSC.TXT
	@mtype -i $(TPS_MINT_DIR)/c.img ::TPSC.OUT | tr -d '\r' > $(TPS_MINT_DIR)/tps_c_expected.txt
	@! mtype -i $(TPS_MINT_DIR)/c.img ::TPSREPT.TXT >/dev/null 2>&1 || printf '!!! tps-mint-realdos: real DOS leg C wrote a report\n'
	@for f in tps_a_expected.txt tpsrept_expected.txt tps_b_expected.txt tps_c_expected.txt; do \
	  cmp -s $(TPS_MINT_DIR)/$$f $(TPS_DIR)/$$f && printf '>>> tps-mint-realdos: %s reproduces the committed golden\n' $$f \
	  || printf '!!! tps-mint-realdos: %s DIFFERS from the committed golden (%s)\n' $$f $(TPS_MINT_DIR)/$$f; done

TEST_EMU_GATES += test-fcb-api test-fcb-api-mutant test-tps-report test-tps-report-mutant

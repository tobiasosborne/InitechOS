# initech-tdnl.20 / PRD 6.5. Same C tenant/link/volume/boot machinery as i123.
include spec/flair_calc_traces.mk
CALC_SRC := os/apps/calc/calc.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/range.c
CALC_DEP := spec/assets/calc_ledger.h os/i123/core/i123.h spec/i123/wk1_format.h
CALC_INC := -Ios/apps/tbx -Ios/i123/core -Ispec -Ispec/assets -ffunction-sections -fdata-sections
CALC_MUTANTS := CALC_MUT_PIE100 CALC_MUT_LEADING_MINUS CALC_MUT_NEVER_DIRTY CALC_MUT_CANCEL_QUITS
CALC_SAVE_MUTANTS := CALC_MUT_SAVE_STALE CALC_MUT_REPLACE_SILENT CALC_MUT_ONE_FLAG CALC_MUT_STAGE_CLOBBER
CALC_PIXEL_MUTANT := CALC_MUT_MERGE_SLICES
$(eval $(call tenant-exe-rules,calc,$(CALC_SRC),$(CALC_INC),$(CALC_DEP),--gc-sections))
$(foreach m,$(CALC_MUTANTS),$(eval $(call tenant-exe-rules,calc_$(m),$(CALC_SRC),$(CALC_INC) -D$(m),$(CALC_DEP),--gc-sections)))
$(eval $(call tenant-exe-rules,calc_$(CALC_PIXEL_MUTANT),$(CALC_SRC),$(CALC_INC) -D$(CALC_PIXEL_MUTANT),$(CALC_DEP),--gc-sections))
$(BUILD)/test_calc_tenant: harness/proptest/test_calc_tenant.c $(CALC_SRC) $(CALC_DEP) | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -Wl,--gc-sections -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC))
$(BUILD)/test_calc_tenant_mut_%: harness/proptest/test_calc_tenant.c $(CALC_SRC) $(CALC_DEP) | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -Wl,--gc-sections -D$* -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC))
$(BUILD)/ppm_flair_calc_check: tools/ppm_flair_calc_check.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< -lpng
CALC_GOLDEN := $(SYSTEM7_DECOMP)/goldens/captures/s8_doc_window_active.png
$(BUILD)/calc_ledger_gen: tools/calc_ledger_gen.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: test-calc-spec
test-calc-spec: $(BUILD)/calc_ledger_gen
	@$(BUILD)/calc_ledger_gen spec/assets/calc_ledger.json > $(BUILD)/calc_ledger.generated.h
	@cmp spec/assets/calc_ledger.h $(BUILD)/calc_ledger.generated.h
	@printf '>>> test-calc-spec: green (locked JSON -> checked-in C header, byte-identical)\n'

.PHONY: test-calc-tenant test-calc-tenant-mutant test-flair-calc test-flair-calc-mutant
test-calc-tenant: test-calc-spec $(BUILD)/test_calc_tenant
	@$(BUILD)/test_calc_tenant
	@printf '>>> test-calc-tenant: green (PRD literals, two windows, i123 edits, menu/close routing)\n'
test-calc-tenant-mutant: test-calc-tenant $(foreach m,$(CALC_MUTANTS),$(BUILD)/test_calc_tenant_mut_$(m))
	@set -e; for m in $(CALC_MUTANTS); do \
		if $(BUILD)/test_calc_tenant_mut_$$m > $(BUILD)/calc_host_$$m.log 2>&1; then printf 'FAIL calc mutant survived: %s\n' "$$m"; exit 1; fi; \
		case $$m in *PIE*) why=PIE_SUM;; *NEVER_DIRTY*) why=QUIT_ASKS;; *CANCEL_QUITS*) why=QUIT_CANCEL;; *) why=TRAILING_MINUS;; esac; \
		grep -q "FAIL $$why" $(BUILD)/calc_host_$$m.log; \
		printf '>>> test-calc-tenant-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL $$why" $(BUILD)/calc_host_$$m.log)"; \
	done
	@printf '>>> test-calc-tenant-mutant: green (four named mutants RED)\n'

# Swap only Calc's bytes on a stock data volume, preserving directory order.
define calc-mut-volume
FLAIR_DATA_CALC_$(1)_IMG := $(BUILD)/flair_data_calc_$(1).img
$$(FLAIR_DATA_CALC_$(1)_IMG): $(FLAIR_DATA_IMG) $(BUILD)/tenant/calc_$(1).exe
	cp -f $$< $$@
	@SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $$@ $(BUILD)/tenant/calc_$(1).exe ::APPS/CALC.EXE
	@$$(call fixture-check,recipe,$$@)
endef
$(foreach m,$(CALC_MUTANTS),$(eval $(call calc-mut-volume,$(m))))
$(eval $(call calc-mut-volume,$(CALC_PIXEL_MUTANT)))
CALC_TEXT_OK = grep -qx 'TENANT-REGISTER ok app=CALC' $(1) && \
	grep -qx 'FLAIR-DISPATCH app=CALC' $(1) && $(call CALC_DRAW_OK,$(1))
CALC_DRAW_OK = grep -q 'ax=0x0030 win=1 .*s="Film Costs" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=1 .*s="Matte Cutters" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=1 .*s="Exacto Blades" fg=65537 bg=0 -> 0' $(1) && \
	grep -qF 's="$$112,276.87" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=1 .*s="570-" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="40%" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="35%" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="18%" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="14%" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="9%" fg=65537 bg=0 -> 0' $(1) && \
	grep -q 'ax=0x0030 win=2 .*s="116%" fg=65537 bg=0 -> 0' $(1)
test-flair-calc: test-calc-spec $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(BUILD)/ppm_flair_calc_check
	$(call appl-boot,flair_calc,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_calc.serial) && $(call CALC_TEXT_OK,$(BUILD)/flair_calc.serial) || { printf 'FAIL calc PRD text/window trace\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/flair_calc.ppm $(CALC_GOLDEN)
	$(call appl-boot,flair_calc_edit,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_EDIT_SPEC,)
	@grep -q 'ax=0x0030 win=1 .*s="73" fg=65537 bg=0 -> 0' $(BUILD)/flair_calc_edit.serial && grep -qx 'TENANT-EXIT rc=0 via=exit' $(BUILD)/flair_calc_edit.serial || { printf 'FAIL calc edit/quit\n'; exit 1; }
	@printf '>>> test-flair-calc: green (disk launch, two windows, PRD text, historical chrome pixels, edit, quit)\n'

define calc-emu-mutant-leg
	$(call appl-boot,flair_calc_$(1),$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_$(1)_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_calc_$(1).serial) && grep -qx 'TENANT-REGISTER ok app=CALC' $(BUILD)/flair_calc_$(1).serial || { printf 'FAIL calc mutant did not launch\n'; exit 1; }
	@if $(call CALC_TEXT_OK,$(BUILD)/flair_calc_$(1).serial); then printf 'FAIL calc mutant $(1) survived\n'; exit 1; fi
	@grep -q '$(2)' $(BUILD)/flair_calc_$(1).serial || { printf 'FAIL calc mutant wrong reason\n'; exit 1; }
	@printf '%s\n' '>>> test-flair-calc-mutant: $(1) RED: $(3)'
endef
$(eval $(call flair-tenants-tbx-mutant-rules,TBX_MUT_TOKEN_ALIAS,calc_token_alias))
$(eval $(call flair-tenants-tbx-mutant-rules,TBX_MUT_GENEVA_IGNORED,calc_geneva_ignored))
define calc-kernel-mutant-leg
	$(call appl-boot,flair_calc_$(1),$(BUILD)/flair_tenants_mut_calc_$(1).img,$(FLAIR_DATA_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_calc_$(1).serial) && $(call CALC_TEXT_OK,$(BUILD)/flair_calc_$(1).serial) || { printf 'FAIL calc kernel mutant did not draw accepted PRD text\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/flair_calc_$(1).ppm $(CALC_GOLDEN) > $(BUILD)/flair_calc_$(1).pixels 2>&1; rc=$$?; \
	[ "$$rc" = 1 ] && grep -q '$(2)' $(BUILD)/flair_calc_$(1).pixels || { cat $(BUILD)/flair_calc_$(1).pixels; printf 'FAIL calc kernel mutant survived or wrong reason\n'; exit 1; }
	@printf '>>> test-flair-calc-mutant: $(3) RED: %s\n' "$$(grep -m1 '$(2)' $(BUILD)/flair_calc_$(1).pixels)"
endef
test-flair-calc-mutant: test-flair-calc $(foreach m,$(CALC_MUTANTS) $(CALC_PIXEL_MUTANT),$(FLAIR_DATA_CALC_$(m)_IMG)) $(BUILD)/flair_tenants_mut_calc_token_alias.img $(BUILD)/flair_tenants_mut_calc_geneva_ignored.img
	$(call calc-emu-mutant-leg,CALC_MUT_PIE100,win=2 .*s="100%",PIE_SUM rendered 100%)
	$(call calc-emu-mutant-leg,CALC_MUT_LEADING_MINUS,win=1 .*s="-570",TRAILING_MINUS rendered -570)
	$(call calc-kernel-mutant-leg,token_alias,WINDOW_TOKENS,TBX_MUT_TOKEN_ALIAS)
	$(call calc-kernel-mutant-leg,geneva_ignored,GENEVA9,TBX_MUT_GENEVA_IGNORED)
	$(call appl-boot,flair_calc_merge,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_MERGE_SLICES_IMG),FLAIR_CALC_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_calc_merge.serial) && $(call CALC_TEXT_OK,$(BUILD)/flair_calc_merge.serial) || { printf 'FAIL merged-slices mutant did not draw PRD text\n'; exit 1; }
	@$(BUILD)/ppm_flair_calc_check $(BUILD)/flair_calc_merge.ppm $(CALC_GOLDEN) > $(BUILD)/flair_calc_merge.pixels 2>&1; rc=$$?; \
	[ "$$rc" = 1 ] && grep -q 'PIE_FIVE' $(BUILD)/flair_calc_merge.pixels || { cat $(BUILD)/flair_calc_merge.pixels; exit 1; }
	@printf '>>> test-flair-calc-mutant: CALC_MUT_MERGE_SLICES RED: FAIL calc pixels: PIE_FIVE five visible sectors\n'
	@printf '>>> test-flair-calc-mutant: green (five named mutants RED)\n'
TEST_UNIT_GATES += test-calc-spec test-calc-tenant test-calc-tenant-mutant
TEST_EMU_GATES += test-flair-calc test-flair-calc-mutant

# ===========================================================================
# initech-bf2 -- THE FILE COPY DIALOG (PRD 6.3/6.5, Appendix A/B; spec/
# toolbox_gate.h Sec 12). Calc's File > Save asks TBX_FILECOPY; the shell runs
# the modal FILE COPY box (Dialog Manager) with the Platinum CDEF 5 bar
# (Control Manager, spec/progress_platinum.h), reads PROGRESS_BAR_RATE from
# APPS\MICHAEL.CFG through the DOS handle API and REALLY copies APPS\LEDGER.TXT
# to the new APPS\TABLES.TXT. Oracles:
#   host   test-calc-file-copy-routing (+ CALC_MUT_CAPTION RED), and
#          test-control PROPERTY 5b (the bar vs the MINTED captures).
#   emu    test-flair-file-copy: leg MID (screendump at TENANT-FILECOPY-MID:
#          the bar graded pixel-for-pixel against ../system7-decomp
#          s8_progress_determinate*.png + the caption ink), leg DONE (box gone;
#          the RATE oracle: completion ticks == ceil(10000 / rate) with rate
#          read from the volume by mtools; the FAT differential: TABLES.TXT ==
#          LEDGER.TXT == the shipped fixture by mtools AND the independent
#          python FAT12 reader, fsck.fat clean).
#   mutant test-flair-file-copy-mutant: rate ignored, copy not performed,
#          caption text, the retired flat bar -- each RED for its own reason.
# ===========================================================================
$(BUILD)/test_calc_file_copy: harness/proptest/test_calc_file_copy.c $(CALC_SRC) $(CALC_DEP) | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -Wl,--gc-sections -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC))
$(BUILD)/test_calc_file_copy_mut_caption: harness/proptest/test_calc_file_copy.c $(CALC_SRC) $(CALC_DEP) | $(BUILD)
	$(CC) $(CFLAGS) $(CALC_INC) -DCALC_MUT_CAPTION -Wl,--gc-sections -o $@ $< $(filter-out os/apps/calc/calc.c,$(CALC_SRC))
.PHONY: test-calc-file-copy-routing test-calc-file-copy-routing-mutant
test-calc-file-copy-routing: $(BUILD)/test_calc_file_copy
	@$(BUILD)/test_calc_file_copy
	@printf '>>> test-calc-file-copy-routing: green (Save enabled, asks the FILE COPY box once, PRD caption, a real src -> new dst)\n'
define calc-save-host-mutant
$$(BUILD)/test_calc_file_copy_mut_$(1): harness/proptest/test_calc_file_copy.c $$(CALC_SRC) $$(CALC_DEP) | $$(BUILD)
	$$(CC) $$(CFLAGS) $$(CALC_INC) -D$(1) -Wl,--gc-sections -o $$@ $$< $$(filter-out os/apps/calc/calc.c,$$(CALC_SRC))
endef
$(foreach m,$(CALC_SAVE_MUTANTS),$(eval $(call calc-save-host-mutant,$(m))))
test-calc-file-copy-routing-mutant: test-calc-file-copy-routing $(BUILD)/test_calc_file_copy_mut_caption $(foreach m,$(CALC_SAVE_MUTANTS),$(BUILD)/test_calc_file_copy_mut_$(m))
	@set -e; for pair in CALC_MUT_SAVE_STALE:SAVE_EDIT CALC_MUT_REPLACE_SILENT:REPLACE_ASKS CALC_MUT_ONE_FLAG:SAVE_REPEAT CALC_MUT_STAGE_CLOBBER:STAGE_CLOBBER; do m=$${pair%%:*}; why=$${pair#*:}; \
		if $(BUILD)/test_calc_file_copy_mut_$$m > $(BUILD)/test_calc_file_copy_mut_$$m.log 2>&1; then printf 'FAIL file-copy routing mutant %s survived\n' "$$m"; exit 1; fi; \
		grep -q "FAIL file-copy routing: $$why" $(BUILD)/test_calc_file_copy_mut_$$m.log || { printf 'FAIL %s RED for the wrong reason\n' "$$m"; exit 1; }; \
		printf '>>> test-calc-file-copy-routing-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL file-copy routing: $$why" $(BUILD)/test_calc_file_copy_mut_$$m.log)"; done
	@if $(BUILD)/test_calc_file_copy_mut_caption > $(BUILD)/test_calc_file_copy_mut_caption.log 2>&1; then printf 'FAIL file-copy routing mutant CALC_MUT_CAPTION survived\n'; exit 1; fi
	@grep -q 'FAIL file-copy routing: SAVE_CAPTION' $(BUILD)/test_calc_file_copy_mut_caption.log
	@printf '>>> test-calc-file-copy-routing-mutant: CALC_MUT_CAPTION RED: %s\n' "$$(grep -m1 SAVE_CAPTION $(BUILD)/test_calc_file_copy_mut_caption.log)"
TEST_UNIT_GATES += test-calc-file-copy-routing test-calc-file-copy-routing-mutant

PPM_FLAIR_FILE_COPY_CHECK_BIN := $(BUILD)/ppm_flair_file_copy_check
$(PPM_FLAIR_FILE_COPY_CHECK_BIN): tools/ppm_flair_file_copy_check.c harness/render/progress_golden.h spec/assets/chicago12.h | $(BUILD)
	$(CC) $(CFLAGS) -Iharness/render -Ispec -Ispec/assets -o $@ $< -lpng

# Kernel mutants: the Toolbox Gate's copy engine (tbxgate.o) and the Control
# Manager's bar (control.o, the objswap pattern).
$(eval $(call flair-tenants-tbx-mutant-rules,TBX_MUT_FILECOPY_RATE_IGNORED,filecopy_rate_ignored))
$(eval $(call flair-tenants-tbx-mutant-rules,TBX_MUT_FILECOPY_NO_COPY,filecopy_no_copy))
$(BUILD)/control_mut_progress_flat.o: os/flair/control.c os/flair/control.h spec/progress_platinum.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(FLAIR_CONTROL_OPT) -DCTRL_MUT_PROGRESS_FLAT -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/control.c -o $@
$(eval $(call flair-tenants-objswap-mutant-rules,$(KERNEL_CONTROL_OBJ),$(BUILD)/control_mut_progress_flat.o,progress_flat))
# Calc tenant mutant: the wrong caption, on an otherwise stock volume.
$(eval $(call tenant-exe-rules,calc_CALC_MUT_CAPTION,$(CALC_SRC),$(CALC_INC) -DCALC_MUT_CAPTION,$(CALC_DEP),--gc-sections))
$(eval $(call calc-mut-volume,CALC_MUT_CAPTION))

FILE_COPY_CAPTION := Saving tables to disk...
# $(call file-copy-serial-ok,<serial>,<data img>): the request, the rate read
# by the SHELL equals the one mtools reads from the same volume, the size,
# the delivery. Exit 0 iff all hold.
define file-copy-serial-ok
rate=$$(mtype -i "$(2)" ::APPS/MICHAEL.CFG | tr -d '\r' | sed -n 's/^PROGRESS_BAR_RATE=\([0-9][0-9]*\)$$/\1/p'); \
size=$$(mcopy -n -i "$(2)" ::APPS/LEDGER.TXT - | wc -c); \
[ -n "$$rate" ] && \
grep -qxF 'TENANT-GATE ax=0x0091 s="$(FILE_COPY_CAPTION)" src="APPS\LEDGER.TXT" dst="APPS\TABLES.TXT" -> 0' "$(1)" && \
grep -qxF "TENANT-FILECOPY s=\"$(FILE_COPY_CAPTION)\" src=APPS\\LEDGER.TXT dst=APPS\\TABLES.TXT rate=$$rate" "$(1)" && \
grep -qxF "TENANT-FILECOPY up bytes=$$size" "$(1)"
endef
# $(call file-copy-rate-ok,<serial>,<data img>): completion ticks are the
# configured rate's: ceil(10000/rate) .. +6 (PIT 100 Hz, 1-tick WaitNextEvent
# slices). The rate is read by mtools, never from the kernel's own line.
define file-copy-rate-ok
rate=$$(mtype -i "$(2)" ::APPS/MICHAEL.CFG | tr -d '\r' | sed -n 's/^PROGRESS_BAR_RATE=\([0-9][0-9]*\)$$/\1/p'); \
size=$$(mcopy -n -i "$(2)" ::APPS/LEDGER.TXT - | wc -c); \
want=$$(( (10000 + rate - 1) / rate )); \
t=$$(sed -n "s/^TENANT-FILECOPY-DONE rc=$$size ticks=\([0-9][0-9]*\)$$/\1/p" "$(1)"); \
[ -n "$$t" ] && [ "$$t" -ge "$$want" ] && [ "$$t" -le "$$(( want + 6 ))" ] && \
grep -qx 'TENANT-EVT what=145' "$(1)"
endef
# $(call file-copy-fat-ok,<data img>,<name>): the FAT differential.
define file-copy-fat-ok
mcopy -n -i "$(1)" ::APPS/TABLES.TXT - > $(BUILD)/$(2).tables.mtools 2>/dev/null && \
cmp -s $(BUILD)/$(2).tables.mtools $(FLAIR_DATA_LEDGER) && \
mcopy -n -i "$(1)" ::APPS/LEDGER.TXT - | cmp -s - $(FLAIR_DATA_LEDGER) && \
python3 -I harness/diff/fat_diff/fat12_ref.py "$(1)" --cat 'APPS\TABLES.TXT' > $(BUILD)/$(2).tables.ref && \
cmp -s $(BUILD)/$(2).tables.ref $(FLAIR_DATA_LEDGER) && \
fsck.fat -n "$(1)" > $(BUILD)/$(2).fsck 2>&1
endef

.PHONY: test-flair-file-copy test-flair-file-copy-mutant
test-flair-file-copy: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_FILE_COPY_CHECK_BIN)
	@command -v mtype >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-file-copy FAIL: mtools / fsck.fat missing\n'; exit 1; }
	$(call appl-boot,flair_file_copy_mid,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
	@$(call APPL_ALIVE,$(BUILD)/flair_file_copy_mid.serial) && ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_mid.serial,$(BUILD)/flair_file_copy_mid_data.img) ) && grep -q '^TENANT-FILECOPY-MID v=' $(BUILD)/flair_file_copy_mid.serial || { printf '!!! test-flair-file-copy FAIL: the MID leg serial (request / rate / size)\n'; grep -E '^(TENANT-(GATE ax=0x0091|FILECOPY|EVT what=145)|FILECOPY)' $(BUILD)/flair_file_copy_mid.serial; exit 1; }
	@$(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_mid.ppm || { printf '!!! test-flair-file-copy FAIL: the mid-copy box pixels\n'; exit 1; }
	@printf '>>> test-flair-file-copy [1/2]: MID -- Save asked the box; the shell read rate=%s from MICHAEL.CFG; the partial Platinum bar matches the minted captures pixel-for-pixel; the PRD caption ink\n' "$$(sed -n 's/^TENANT-FILECOPY s=.* rate=//p' $(BUILD)/flair_file_copy_mid.serial)"
	$(call appl-boot,flair_file_copy_done,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_file_copy_done.serial) && ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_done.serial,$(BUILD)/flair_file_copy_done_data.img) ) || { printf '!!! test-flair-file-copy FAIL: the DONE leg serial\n'; exit 1; }
	@( $(call file-copy-rate-ok,$(BUILD)/flair_file_copy_done.serial,$(BUILD)/flair_file_copy_done_data.img) ) || { printf '!!! test-flair-file-copy FAIL: RATE -- completion ticks are not the configured PROGRESS_BAR_RATE\n'; grep -E '^TENANT-FILECOPY' $(BUILD)/flair_file_copy_done.serial; exit 1; }
	@( $(call file-copy-fat-ok,$(BUILD)/flair_file_copy_done_data.img,flair_file_copy_done) ) || { printf '!!! test-flair-file-copy FAIL: FAT -- APPS\\TABLES.TXT is not a byte-exact copy of the ledger (mtools / fat12_ref.py / fsck.fat)\n'; mdir -i $(BUILD)/flair_file_copy_done_data.img ::APPS; exit 1; }
	@$(PPM_FLAIR_FILE_COPY_CHECK_BIN) done $(BUILD)/flair_file_copy_done.ppm || { printf '!!! test-flair-file-copy FAIL: the box is still on screen after completion\n'; exit 1; }
	@printf '>>> test-flair-file-copy [2/2]: DONE -- %s; TABLES.TXT == LEDGER.TXT == fixture (mtools + python FAT12 reader), fsck.fat clean; box disposed\n' "$$(grep -m1 '^TENANT-FILECOPY-DONE' $(BUILD)/flair_file_copy_done.serial)"
	@printf '>>> test-flair-file-copy: green\n'

test-flair-file-copy-mutant: test-flair-file-copy $(BUILD)/flair_tenants_mut_filecopy_rate_ignored.img $(BUILD)/flair_tenants_mut_filecopy_no_copy.img $(BUILD)/flair_tenants_mut_progress_flat.img $(FLAIR_DATA_CALC_CALC_MUT_CAPTION_IMG)
	$(call appl-boot,flair_file_copy_rate,$(BUILD)/flair_tenants_mut_filecopy_rate_ignored.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,)
	@if ( $(call file-copy-rate-ok,$(BUILD)/flair_file_copy_rate.serial,$(BUILD)/flair_file_copy_rate_data.img) ); then printf '!!! test-flair-file-copy-mutant FAIL: TBX_MUT_FILECOPY_RATE_IGNORED survived\n'; exit 1; fi
	@grep -q '^TENANT-FILECOPY-DONE rc=' $(BUILD)/flair_file_copy_rate.serial || { printf '!!! rate mutant did not run the box\n'; exit 1; }
	@printf '>>> test-flair-file-copy-mutant: TBX_MUT_FILECOPY_RATE_IGNORED RED: RATE (%s, want ceil(10000/rate))\n' "$$(grep -m1 '^TENANT-FILECOPY-DONE' $(BUILD)/flair_file_copy_rate.serial)"
	$(call appl-boot,flair_file_copy_nocopy,$(BUILD)/flair_tenants_mut_filecopy_no_copy.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,)
	@grep -q '^TENANT-FILECOPY-DONE rc=' $(BUILD)/flair_file_copy_nocopy.serial || { printf '!!! no-copy mutant did not run the box\n'; exit 1; }
	@if ( $(call file-copy-fat-ok,$(BUILD)/flair_file_copy_nocopy_data.img,flair_file_copy_nocopy) ); then printf '!!! test-flair-file-copy-mutant FAIL: TBX_MUT_FILECOPY_NO_COPY survived\n'; exit 1; fi
	@printf '>>> test-flair-file-copy-mutant: TBX_MUT_FILECOPY_NO_COPY RED: FAT (%s)\n' "$$(mdir -b -i $(BUILD)/flair_file_copy_nocopy_data.img ::APPS/TABLES.TXT 2>&1 | head -1)"
	$(call appl-boot,flair_file_copy_caption,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_CAPTION_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
	@grep -q '^TENANT-FILECOPY-MID' $(BUILD)/flair_file_copy_caption.serial || { printf '!!! caption mutant did not run the box\n'; exit 1; }
	@if ( $(call file-copy-serial-ok,$(BUILD)/flair_file_copy_caption.serial,$(BUILD)/flair_file_copy_caption_data.img) ); then printf '!!! test-flair-file-copy-mutant FAIL: CALC_MUT_CAPTION survived the serial leg\n'; exit 1; fi
	@if $(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_caption.ppm > $(BUILD)/flair_file_copy_caption.pixels 2>&1; then printf '!!! test-flair-file-copy-mutant FAIL: CALC_MUT_CAPTION survived the pixel leg\n'; exit 1; fi
	@grep -q 'FAIL file-copy CAPTION' $(BUILD)/flair_file_copy_caption.pixels || { cat $(BUILD)/flair_file_copy_caption.pixels; exit 1; }
	@printf '>>> test-flair-file-copy-mutant: CALC_MUT_CAPTION RED: %s\n' "$$(grep -m1 'FAIL file-copy CAPTION' $(BUILD)/flair_file_copy_caption.pixels)"
	$(call appl-boot,flair_file_copy_flat,$(BUILD)/flair_tenants_mut_progress_flat.img,$(FLAIR_DATA_IMG),FLAIR_FILE_COPY_SPEC,--screendump --screendump-after TENANT-FILECOPY-MID)
	@grep -q '^TENANT-FILECOPY-MID' $(BUILD)/flair_file_copy_flat.serial || { printf '!!! flat-bar mutant did not run the box\n'; exit 1; }
	@if $(PPM_FLAIR_FILE_COPY_CHECK_BIN) mid $(BUILD)/flair_file_copy_flat.ppm > $(BUILD)/flair_file_copy_flat.pixels 2>&1; then printf '!!! test-flair-file-copy-mutant FAIL: CTRL_MUT_PROGRESS_FLAT survived the pixel leg\n'; exit 1; fi
	@grep -q 'FAIL file-copy BAR' $(BUILD)/flair_file_copy_flat.pixels || { cat $(BUILD)/flair_file_copy_flat.pixels; exit 1; }
	@printf '>>> test-flair-file-copy-mutant: CTRL_MUT_PROGRESS_FLAT RED: %s\n' "$$(grep -m1 'FAIL file-copy BAR' $(BUILD)/flair_file_copy_flat.pixels)"
	@printf '>>> test-flair-file-copy-mutant: green (four named mutants RED)\n'
TEST_EMU_GATES += test-flair-file-copy test-flair-file-copy-mutant

# initech-tdnl.20 (bar carrier): PAINT.EXE, the canon Photoshop bar over a
# blank canvas. Two named mutants: a "corrected" title, every item enabled.
PAINT_SRC := os/apps/paint/paint.c
PAINT_DEP := spec/assets/menu_canon.h
PAINT_MUTANTS := PAINT_MUT_BAR_TEXT PAINT_MUT_ITEMS_ENABLED
$(eval $(call tenant-exe-rules,paint,$(PAINT_SRC),,$(PAINT_DEP),))
$(foreach m,$(PAINT_MUTANTS),$(eval $(call tenant-exe-rules,paint_$(m),$(PAINT_SRC),-D$(m),$(PAINT_DEP),)))

PPM_FLAIR_PAINT_CHECK_BIN := $(BUILD)/ppm_flair_paint_check
$(PPM_FLAIR_PAINT_CHECK_BIN): tools/ppm_flair_paint_check.c spec/chicago12_nfnt_golden.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec -o $@ $<
# Paint tenant mutant volumes: only PAINT.EXE's bytes are swapped.
define paint-mut-volume
FLAIR_DATA_PAINT_$(1)_IMG := $(BUILD)/flair_data_paint_$(1).img
$$(FLAIR_DATA_PAINT_$(1)_IMG): $(FLAIR_DATA_IMG) $(BUILD)/tenant/paint_$(1).exe
	cp -f $$< $$@
	@SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $$@ $(BUILD)/tenant/paint_$(1).exe ::APPS/PAINT.EXE
	@$$(call fixture-check,recipe,$$@)
endef
$(foreach m,$(PAINT_MUTANTS),$(eval $(call paint-mut-volume,$(m))))

# PAINT_UP: registered, the canon bar installed, the canvas titled, foreground.
PAINT_UP = grep -qx 'TENANT-REGISTER ok app=PAINT' $(1) && grep -qx 'FLAIR-DISPATCH app=PAINT' $(1) && \
	grep -q '^TENANT-GATE ax=0x0050 bar=+0x[0-9a-f]* -> 0$$' $(1) && \
	grep -qx 'TENANT-GATE ax=0x0022 win=1 s="Untitled-1" -> 0' $(1)
# PAINT_GRAYED: the Edit panel dropped, the release on the grayed Cut delivered
# NOTHING to the tenant, and File > Quit (the one live item) ended it.
PAINT_GRAYED = grep -qx 'FLAIR-MENU-XDROP menu=129' $(1) && grep -qx 'TENANT-EXIT rc=0 via=exit' $(1) && \
	[ "$$(grep -c '^TENANT-EVT what=81$$' $(1))" = 1 ]
.PHONY: test-flair-paint test-flair-paint-mutant
test-flair-paint: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(PPM_FLAIR_APPL_CHECK_BIN) $(PPM_FLAIR_PAINT_CHECK_BIN)
	$(call appl-boot,flair_paint,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_PAINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call APPL_ALIVE,$(BUILD)/flair_paint.serial) && $(call PAINT_UP,$(BUILD)/flair_paint.serial) || { printf '!!! test-flair-paint FAIL: PAINT did not register / install its bar / title its canvas\n'; grep -E '^(TENANT|FLAIR-DISPATCH)' $(BUILD)/flair_paint.serial | head -20; exit 1; }
	@$(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: band 2 is not the canon File Edit Image Layer Select View Window Help bar\n'; exit 1; }
	@$(PPM_FLAIR_PAINT_CHECK_BIN) canvas $(BUILD)/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: the canvas\n'; exit 1; }
	@printf '>>> test-flair-paint [1/2]: BAR -- PAINT.EXE launched from disk; band 2 is pixel-exactly the canon Photoshop bar (Chicago 12, NFNT 5478); blank white canvas\n'
	$(call appl-boot,flair_paint_menu,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_PAINT_MENU_SPEC,--screendump --screendump-after FLAIR-MENU-XDROP)
	@$(call APPL_ALIVE,$(BUILD)/flair_paint_menu.serial) && $(call PAINT_GRAYED,$(BUILD)/flair_paint_menu.serial) || { printf '!!! test-flair-paint FAIL: the grayed release delivered something, or Quit did not end PAINT\n'; grep -E '^(TENANT-EVT|TENANT-EXIT|FLAIR-MENU)' $(BUILD)/flair_paint_menu.serial; exit 1; }
	@$(PPM_FLAIR_PAINT_CHECK_BIN) menu $(BUILD)/flair_paint_menu.ppm || { printf '!!! test-flair-paint FAIL: the Edit panel is not all period-grayed\n'; exit 1; }
	@printf '>>> test-flair-paint [2/2]: MENUS -- Edit drops with only disabled ink; releasing on a grayed item delivers nothing; File > Quit exits\n'
	@printf '>>> test-flair-paint: green\n'
test-flair-paint-mutant: test-flair-paint $(foreach m,$(PAINT_MUTANTS),$(FLAIR_DATA_PAINT_$(m)_IMG))
	$(call appl-boot,flair_paint_bartext,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_PAINT_PAINT_MUT_BAR_TEXT_IMG),FLAIR_PAINT_SPEC,--screendump --screendump-after FLAIR-LIVE-OK)
	@$(call PAINT_UP,$(BUILD)/flair_paint_bartext.serial) || { printf '!!! bar-text mutant did not launch\n'; exit 1; }
	@if $(PPM_FLAIR_APPL_CHECK_BIN) bar-photoshop $(BUILD)/flair_paint_bartext.ppm > $(BUILD)/flair_paint_bartext.pixels 2>&1; then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_BAR_TEXT survived\n'; exit 1; fi
	@printf '>>> test-flair-paint-mutant: PAINT_MUT_BAR_TEXT RED: %s\n' "$$(grep -m1 FAIL $(BUILD)/flair_paint_bartext.pixels | cut -c1-160)"
	$(call appl-boot,flair_paint_enabled,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_PAINT_PAINT_MUT_ITEMS_ENABLED_IMG),FLAIR_PAINT_MENU_SPEC,--screendump --screendump-after FLAIR-MENU-XDROP)
	@grep -qx 'FLAIR-MENU-XDROP menu=129' $(BUILD)/flair_paint_enabled.serial || { printf '!!! items-enabled mutant did not drop Edit\n'; exit 1; }
	@if $(call PAINT_GRAYED,$(BUILD)/flair_paint_enabled.serial); then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_ITEMS_ENABLED survived the serial leg\n'; exit 1; fi
	@if $(PPM_FLAIR_PAINT_CHECK_BIN) menu $(BUILD)/flair_paint_enabled.ppm > $(BUILD)/flair_paint_enabled.pixels 2>&1; then printf '!!! test-flair-paint-mutant FAIL: PAINT_MUT_ITEMS_ENABLED survived the pixel leg\n'; exit 1; fi
	@printf '>>> test-flair-paint-mutant: PAINT_MUT_ITEMS_ENABLED RED: %s; serial %s TENANT-EVT what=81\n' "$$(grep -m1 FAIL $(BUILD)/flair_paint_enabled.pixels)" "$$(grep -c '^TENANT-EVT what=81$$' $(BUILD)/flair_paint_enabled.serial)"
	@printf '>>> test-flair-paint-mutant: green (four named mutants RED)\n'
TEST_EMU_GATES += test-flair-paint test-flair-paint-mutant
# Host routing for Paint (+ both tenant mutants RED for their own reasons).
PAINT_HOST_INC := -Ios/apps/tbx -Ispec -Ispec/assets
$(BUILD)/test_paint_tenant: harness/proptest/test_paint_tenant.c $(PAINT_SRC) $(PAINT_DEP) spec/assets/calc_ledger.h | $(BUILD)
	$(CC) $(CFLAGS) $(PAINT_HOST_INC) -o $@ $<
$(BUILD)/test_paint_tenant_mut_%: harness/proptest/test_paint_tenant.c $(PAINT_SRC) $(PAINT_DEP) spec/assets/calc_ledger.h | $(BUILD)
	$(CC) $(CFLAGS) $(PAINT_HOST_INC) -D$* -o $@ $<
.PHONY: test-paint-tenant test-paint-tenant-mutant
test-paint-tenant: $(BUILD)/test_paint_tenant
	@$(BUILD)/test_paint_tenant
	@printf '>>> test-paint-tenant: green (the canon bar, no Apple slot, all items grayed but File > Quit, blank canvas)\n'
test-paint-tenant-mutant: test-paint-tenant $(foreach m,$(PAINT_MUTANTS),$(BUILD)/test_paint_tenant_mut_$(m))
	@set -e; for m in $(PAINT_MUTANTS); do \
		if $(BUILD)/test_paint_tenant_mut_$$m > $(BUILD)/paint_host_$$m.log 2>&1; then printf 'FAIL paint mutant survived: %s\n' "$$m"; exit 1; fi; \
		case $$m in *BAR*) why=BAR_TEXT;; *) why=GRAYED;; esac; \
		grep -q "FAIL paint routing: $$why" $(BUILD)/paint_host_$$m.log; \
		printf '>>> test-paint-tenant-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL paint routing: $$why" $(BUILD)/paint_host_$$m.log)"; \
	done
	@printf '>>> test-paint-tenant-mutant: green (four named mutants RED)\n'
TEST_UNIT_GATES += test-paint-tenant test-paint-tenant-mutant

# ===========================================================================
# initech-qxft -- InitechCalc's EXIT GUARD, SAVE CONTENT and REPLACE QUESTION
# (audit 2026-10-23-das L007 / L006 / L009). Ground truth: Macintosh document
# conventions (initech-manuals-ref/macos: Quit and the close box ask Save /
# Cancel / Don't Save about unsaved work; Save over an existing file asks to
# replace it) and the in-tree Initech 123 guard (initech-fvnj, test-flair-i123-
# quitguard). The Finder Shut Down / Restart path delivers File > Quit to the
# resident tenant (kmain finder_power_ask_apps), so Calc's one guard also
# answers it: a Cancel declines the shut-down (test-flair-shutdown owns that
# route). The "Saving tables to disk..." box is the deliberate canon and is
# untouched (test-flair-file-copy). Eleven boots of the flagship desktop, each
# on a fresh copy of the stock data volume, locked traces in
# spec/flair_calc_traces.mk. Expected file bytes are read by mtools, never from
# Calc. Mutants (Rule 6): NEVER_DIRTY (leg 1), CANCEL_QUITS (leg 2),
# SAVE_STALE (leg 8), REPLACE_SILENT (leg 9). Rule 14 clip: record-flair
# SCRIPT=calc_quit_guard.
# ===========================================================================
$(foreach m,$(CALC_SAVE_MUTANTS),$(eval $(call tenant-exe-rules,calc_$(m),$(CALC_SRC),$(CALC_INC) -D$(m),$(CALC_DEP),--gc-sections)))
$(foreach m,$(CALC_SAVE_MUTANTS),$(eval $(call calc-mut-volume,$(m))))
# The replace-question volume: the stock volume plus an APPS\TABLES.TXT the
# audit's L009 fixture text (written by mtools, so Calc cannot grade itself).
CALC_SEED_TABLES := ORIGINAL TABLES MUST SURVIVE FAILED COPY
CALC_SEEDED_DATA := $(BUILD)/flair_calc_seeded_tables.img
$(CALC_SEEDED_DATA): $(FLAIR_DATA_IMG) | $(BUILD)
	cp -f $(FLAIR_DATA_IMG) $@
	@printf '%s\r\n' '$(CALC_SEED_TABLES)' > $@.seed.txt
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $@.seed.txt ::APPS/TABLES.TXT
	@$(call fixture-check,recipe,$@)
CALC_SEEDED_SILENT_DATA := $(BUILD)/flair_calc_seeded_tables_silent.img
$(CALC_SEEDED_SILENT_DATA): $(CALC_SEEDED_DATA) $(BUILD)/tenant/calc_CALC_MUT_REPLACE_SILENT.exe | $(BUILD)
	cp -f $(CALC_SEEDED_DATA) $@
	SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mcopy -o -i $@ $(BUILD)/tenant/calc_CALC_MUT_REPLACE_SILENT.exe ::APPS/CALC.EXE
	@$(call fixture-check,recipe,$@)
CQ_SEED_KEPT = [ "$$(mcopy -n -i "$(1)" ::APPS/TABLES.TXT - | tr -d '\r')" = "$(CALC_SEED_TABLES)" ]
CQ_ASKED = grep -qx 'TENANT-GATE ax=0x0090 def="Save" cancel="Cancel" other="Don.t Save" -> 0' $(1) && grep -qx 'TENANT-ALERT-LINE Save changes to the worksheet' $(1) && grep -qx 'TENANT-ALERT-LINE "TABLES.TXT"' $(1) && grep -qx 'TENANT-ALERT up lines=3 buttons=3' $(1)
CQ_NO_EXIT = ! grep -q '^TENANT-EXIT' $(1) && ! grep -q '^FLAIR-TENANT-EXIT' $(1)
CQ_REPLACE_ASKED = grep -qx 'TENANT-GATE ax=0x0090 def="Yes" cancel="No" other=? -> 0' $(1) && grep -qx 'TENANT-ALERT-LINE "TABLES.TXT" already exists.' $(1) && grep -qx 'TENANT-ALERT up lines=3 buttons=2' $(1)
CQ_TABLES = mcopy -n -i "$(1)" ::APPS/TABLES.TXT - 2>/dev/null
CQ_NO_TABLES = ! mdir -b -i "$(1)" ::APPS 2>/dev/null | grep -qi 'TABLES\.TXT'
CQ_NO_STAGE = ! mdir -b -i "$(1)" ::APPS 2>/dev/null | grep -qi 'TABLES\.TMP'
CQ_HAS_EDIT = $(call CQ_TABLES,$(1)) | tr -d '\r' | sed -n '2p' | grep -q '^$(2) *TODO_FRAME$$' && ! $(call CQ_TABLES,$(1)) | grep -q 'Film Costs'

.PHONY: test-flair-calc-quit test-flair-calc-quit-mutant
test-flair-calc-quit: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(CALC_SEEDED_DATA) $(FLAIR_DATA_IMG) $(PPM_FLAIR_TRASH_CHECK_BIN)
	@command -v mdir >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-calc-quit FAIL: mtools / fsck.fat missing\n'; exit 1; }
	$(call appl-boot,flair_calc_qg_alert,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_ALERT,--screendump --screendump-after "TENANT-ALERT up")
	@f=$(BUILD)/flair_calc_qg_alert.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_alert_data.img) || { printf '!!! test-flair-calc-quit [1/11] FAIL: ALERT -- an edited Ctrl-Q must ask Save / Cancel / Don'"'"'t Save and never exit (L007)\n'; grep -E '^(TENANT-(GATE ax=0x0090|ALERT|EXIT|MENU|CRASH)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
	@$(PPM_FLAIR_TRASH_CHECK_BIN) tenant3 $(BUILD)/flair_calc_qg_alert.ppm > $(BUILD)/flair_calc_qg_alert.pixels || { cat $(BUILD)/flair_calc_qg_alert.pixels; printf '!!! test-flair-calc-quit [1/11] FAIL: ALERT -- the three-button alert is not drawn as graded\n'; exit 1; }
	@printf '>>> test-flair-calc-quit [1/11] ALERT: Ctrl-Q over the edited A1 asks (3 lines, 3 buttons, pixel-graded), no exit, nothing written\n'
	$(call appl-boot,flair_calc_qg_cancel,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CANCEL,--screendump --screendump-after FLAIR-LIVE-OK)
	@f=$(BUILD)/flair_calc_qg_cancel.serial; $(call APPL_ALIVE,$$f) && [ "$$(grep -c '^TENANT-ALERT up lines=3 buttons=3$$' $$f)" = 2 ] && [ "$$(grep -c '^TENANT-ALERT-HIT item=2$$' $$f)" = 2 ] && $(call CQ_NO_EXIT,$$f) && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_cancel_data.img) || { printf '!!! test-flair-calc-quit [2/11] FAIL: CANCEL -- Escape must keep Calc open and dirty (asked twice)\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [2/11] CANCEL: Escape twice -> two Cancel answers, no exit, still dirty (asked again), nothing written\n'
	$(call appl-boot,flair_calc_qg_dontsave,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_DONTSAVE,)
	@f=$(BUILD)/flair_calc_qg_dontsave.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=3' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && $(call CQ_NO_TABLES,$(BUILD)/flair_calc_qg_dontsave_data.img) && ! grep -q '^TENANT-GATE ax=0x0091' $$f || { printf '!!! test-flair-calc-quit [3/11] FAIL: DONTSAVE -- the third button must end Calc and write nothing\n'; grep -E '^(TENANT-(ALERT|EXIT|GATE ax=0x009)|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [3/11] DONTSAVE: the third button -> EXIT(0); no FILE COPY, no TABLES.TXT on the volume\n'
	$(call appl-boot,flair_calc_qg_savequit,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_SAVEQUIT,)
	@f=$(BUILD)/flair_calc_qg_savequit.serial; d=$(BUILD)/flair_calc_qg_savequit_data.img; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-ALERT-HIT item=1' $$f && grep -q '^TENANT-FILECOPY-DONE' $$f && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [4/11] FAIL: SAVEQUIT -- Save from the alert must write the EDITED worksheet, then end Calc\n'; grep -E '^(TENANT-(ALERT|EXIT|FILECOPY|GATE ax=0x009)|PANIC)' $$f; $(call CQ_TABLES,$$d) | head -3; exit 1; }
	@printf '>>> test-flair-calc-quit [4/11] SAVEQUIT: Save -> FILE COPY box -> TABLES.TXT line 2 is the edited 73 (mtools), no stage left, fsck clean, then EXIT(0)\n'
	$(call appl-boot,flair_calc_qg_clean,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CLEAN,)
	@f=$(BUILD)/flair_calc_qg_clean.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-EXIT rc=0 via=exit' $$f && ! grep -q '^TENANT-ALERT' $$f || { printf '!!! test-flair-calc-quit [5/11] FAIL: CLEAN -- an unedited worksheet must quit at once, never ask\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [5/11] CLEAN: no edit -> Ctrl-Q exits at once, no question\n'
	$(call appl-boot,flair_calc_qg_pending,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_PENDING,)
	@f=$(BUILD)/flair_calc_qg_pending.serial; $(call APPL_ALIVE,$$f) && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) || { printf '!!! test-flair-calc-quit [6/11] FAIL: PENDING -- an entry being typed is unsaved work: Quit must ask\n'; grep -E '^(TENANT-(ALERT|EXIT)|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [6/11] PENDING: "7" typed, not entered -> Ctrl-Q asks\n'
	$(call appl-boot,flair_calc_qg_close,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_CLOSE,)
	@f=$(BUILD)/flair_calc_qg_close.serial; $(call APPL_ALIVE,$$f) && grep -q '^FLAIR-CLOSE-ASK win ' $$f && $(call CQ_ASKED,$$f) && $(call CQ_NO_EXIT,$$f) || { printf '!!! test-flair-calc-quit [7/11] FAIL: CLOSE -- the close box over an edited sheet must ask, not terminate\n'; grep -E '^(FLAIR-CLOSE|TENANT-(ALERT|EXIT|EVT)|FLAIR-TENANT-EXIT|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [7/11] CLOSE: the close box over an edited sheet -> FLAIR-CLOSE-ASK, the alert, no exit\n'
	$(call appl-boot,flair_calc_qg_save,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_IMG),FLAIR_CALC_QG_SAVE,)
	@f=$(BUILD)/flair_calc_qg_save.serial; d=$(BUILD)/flair_calc_qg_save_data.img; $(call APPL_ALIVE,$$f) && grep -q '^TENANT-FILECOPY-DONE' $$f && ! grep -q '^TENANT-ALERT' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && mcopy -n -i $$d ::APPS/LEDGER.TXT - | cmp -s - $(FLAIR_DATA_LEDGER) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [8/11] FAIL: SAVE -- Save must write the EDITED worksheet (L006: TABLES.TXT line 2 = 73, not Film Costs)\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
	@printf '>>> test-flair-calc-quit [8/11] SAVE: A1 := 73, Ctrl-S -> TABLES.TXT holds 73 and no Film Costs (mtools), LEDGER.TXT untouched, fsck clean\n'
	$(call appl-boot,flair_calc_qg_replaceask,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACEASK,--screendump --screendump-after "TENANT-ALERT up")
	@f=$(BUILD)/flair_calc_qg_replaceask.serial; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && ! grep -q '^TENANT-GATE ax=0x0091' $$f && $(call CQ_SEED_KEPT,$(BUILD)/flair_calc_qg_replaceask_data.img) || { printf '!!! test-flair-calc-quit [9/11] FAIL: REPLACEASK -- a Save onto an existing TABLES.TXT must name it and ask (L009)\n'; grep -E '^(TENANT-(ALERT|FILECOPY-DONE|GATE ax=0x009)|PANIC)' $$f; exit 1; }
	@printf '>>> test-flair-calc-quit [9/11] REPLACEASK: Save over a seeded TABLES.TXT -> "TABLES.TXT" already exists. ... Yes / No, nothing copied yet\n'
	$(call appl-boot,flair_calc_qg_replacecancel,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACECANCEL,)
	@f=$(BUILD)/flair_calc_qg_replacecancel.serial; d=$(BUILD)/flair_calc_qg_replacecancel_data.img; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=2' $$f && ! grep -q '^TENANT-GATE ax=0x0091' $$f && $(call CQ_SEED_KEPT,$$d) && $(call CQ_NO_STAGE,$$d) || { printf '!!! test-flair-calc-quit [10/11] FAIL: REPLACECANCEL -- Cancel must leave the existing TABLES.TXT alone\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
	@printf '>>> test-flair-calc-quit [10/11] REPLACECANCEL: Cancel -> the seeded TABLES.TXT is byte-identical (mtools), nothing copied\n'
	$(call appl-boot,flair_calc_qg_replaceyes,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_DATA),FLAIR_CALC_QG_REPLACEYES,)
	@f=$(BUILD)/flair_calc_qg_replaceyes.serial; d=$(BUILD)/flair_calc_qg_replaceyes_data.img; $(call APPL_ALIVE,$$f) && $(call CQ_REPLACE_ASKED,$$f) && grep -qx 'TENANT-ALERT-HIT item=1' $$f && grep -q '^TENANT-FILECOPY-DONE' $$f && ( $(call CQ_HAS_EDIT,$$d,73) ) && $(call CQ_NO_STAGE,$$d) && fsck.fat -n $$d >/dev/null || { printf '!!! test-flair-calc-quit [11/11] FAIL: REPLACEYES -- Replace must overwrite TABLES.TXT with the worksheet (73)\n'; $(call CQ_TABLES,$$d) | head -3; exit 1; }
	@printf '>>> test-flair-calc-quit [11/11] REPLACEYES: Replace -> TABLES.TXT now holds the 73 worksheet (mtools), no stage left, fsck clean\n'
	@printf '>>> test-flair-calc-quit: green\n'

test-flair-calc-quit-mutant: test-flair-calc-quit $(foreach m,CALC_MUT_NEVER_DIRTY CALC_MUT_CANCEL_QUITS CALC_MUT_SAVE_STALE,$(FLAIR_DATA_CALC_$(m)_IMG)) $(CALC_SEEDED_SILENT_DATA)
	$(call appl-boot,flair_calc_qgm_dirty,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_NEVER_DIRTY_IMG),FLAIR_CALC_QG_ALERT,--screendump --screendump-after FLAIR-LIVE-OK)
	@f=$(BUILD)/flair_calc_qgm_dirty.serial; if $(call CQ_ASKED,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_NEVER_DIRTY survived\n'; exit 1; fi; grep -qx 'TENANT-EXIT rc=0 via=exit' $$f || { printf '!!! test-flair-calc-quit-mutant: NEVER_DIRTY RED for the wrong reason (no silent exit)\n'; exit 1; }
	@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_NEVER_DIRTY RED: an edited Ctrl-Q quit silently (L007)\n'
	$(call appl-boot,flair_calc_qgm_cancel,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_CANCEL_QUITS_IMG),FLAIR_CALC_QG_CANCEL,--screendump --screendump-after FLAIR-LIVE-OK)
	@f=$(BUILD)/flair_calc_qgm_cancel.serial; if $(call CQ_NO_EXIT,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_CANCEL_QUITS survived\n'; exit 1; fi
	@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_CANCEL_QUITS RED: Cancel ended the session\n'
	$(call appl-boot,flair_calc_qgm_stale,$(FLAIRTENANTS_IMG),$(FLAIR_DATA_CALC_CALC_MUT_SAVE_STALE_IMG),FLAIR_CALC_QG_SAVE,)
	@f=$(BUILD)/flair_calc_qgm_stale.serial; d=$(BUILD)/flair_calc_qgm_stale_data.img; grep -q '^TENANT-FILECOPY-DONE' $$f || { printf '!!! test-flair-calc-quit-mutant: SAVE_STALE did not run the copy\n'; exit 1; }; if ( $(call CQ_HAS_EDIT,$$d,73) ); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_SAVE_STALE survived\n'; exit 1; fi
	@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_SAVE_STALE RED: TABLES.TXT is the stale ledger, the 73 edit is lost (L006)\n'
	$(call appl-boot,flair_calc_qgm_silent,$(FLAIRTENANTS_IMG),$(CALC_SEEDED_SILENT_DATA),FLAIR_CALC_QG_REPLACEASK,--screendump --screendump-after FLAIR-LIVE-OK)
	@f=$(BUILD)/flair_calc_qgm_silent.serial; if $(call CQ_REPLACE_ASKED,$$f); then printf '!!! test-flair-calc-quit-mutant FAIL: CALC_MUT_REPLACE_SILENT survived\n'; exit 1; fi; grep -q '^TENANT-GATE ax=0x0091' $$f || { printf '!!! test-flair-calc-quit-mutant: REPLACE_SILENT RED for the wrong reason (the copy did not run)\n'; exit 1; }
	@printf '>>> test-flair-calc-quit-mutant: CALC_MUT_REPLACE_SILENT RED: the second Save replaced TABLES.TXT unasked (L009)\n'
	@printf '>>> test-flair-calc-quit-mutant: green (four mutants RED for the named reason)\n'
TEST_EMU_GATES += test-flair-calc-quit test-flair-calc-quit-mutant

# initech-tdnl.20 / PRD 6.5. Same C tenant/link/volume/boot machinery as i123.
include spec/flair_calc_traces.mk
CALC_SRC := os/apps/calc/calc.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/range.c
CALC_DEP := spec/assets/calc_ledger.h os/i123/core/i123.h spec/i123/wk1_format.h
CALC_INC := -Ios/apps/tbx -Ios/i123/core -Ispec -Ispec/assets -ffunction-sections -fdata-sections
CALC_MUTANTS := CALC_MUT_PIE100 CALC_MUT_LEADING_MINUS
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
		case $$m in *PIE*) why=PIE_SUM;; *) why=TRAILING_MINUS;; esac; \
		grep -q "FAIL $$why" $(BUILD)/calc_host_$$m.log; \
		printf '>>> test-calc-tenant-mutant: %s RED: %s\n' "$$m" "$$(grep -m1 "FAIL $$why" $(BUILD)/calc_host_$$m.log)"; \
	done
	@printf '>>> test-calc-tenant-mutant: green (both named mutants RED)\n'

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

# Rule-2 boundary probes extend the original suites. Rule-6 mutants perturb
# ONLY artifact TUs. Keep compile failures distinct from runtime RED results.
TBX_HARDENING_MUTANTS :=
define tbx-hardening-mutant
TBX_HARDENING_MUTANTS += $(1)
$(BUILD)/tbx_mut_$(1): $$($(2)_SRC) $$($(2)_DEPS) $(4) harness/proptest/expect_abort.h | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) $(3) -D$(1) -o $$@ $$($(2)_SRC) $(4) $(5)
.PHONY: test-tbx-mutant-$(1)
test-tbx-mutant-$(1): $(BUILD)/tbx_mut_$(1)
	@ulimit -c 0; if $$< > $$<.log 2>&1; then printf 'FAIL $(1): mutant survived\n'; exit 1; fi; \
	grep -Eq '$(6)' $$<.log || { printf 'FAIL $(1): RED for the wrong reason\n'; cat $$<.log; exit 1; }; \
	printf '>>> $(1): RED: '; grep -Em1 '$(6)' $$<.log
endef

$(eval $(call tbx-hardening-mutant,LIST_MUT_NULL_CLICK_MISS,TEST_LIST,$(PROCESS_INC),$(TEST_LIST_LINK),,FAIL.*6vvw))
$(eval $(call tbx-hardening-mutant,LIST_MUT_ROW_WRAP,TEST_LIST,$(PROCESS_INC),$(TEST_LIST_LINK),,FAIL.*8mip))
$(eval $(call tbx-hardening-mutant,LIST_MUT_NO_OVERFLOW_CHECK,TEST_LIST,$(PROCESS_INC),$(TEST_LIST_LINK),,FAIL.*nbnx))
$(eval $(call tbx-hardening-mutant,LIST_MUT_COLUMN_HEADROOM,TEST_LIST,$(PROCESS_INC),$(TEST_LIST_LINK),,FAIL.*42dq))
$(eval $(call tbx-hardening-mutant,RES_MUT_MAP_PREAMBLE,TEST_RESOURCE,$(PROCESS_INC),$(TEST_RESOURCE_LINK),,FAIL.*6mfi))
$(eval $(call tbx-hardening-mutant,RES_MUT_TYPE_WRAP,TEST_RESOURCE,$(PROCESS_INC),$(TEST_RESOURCE_LINK),,FAIL.*s3bh))
$(eval $(call tbx-hardening-mutant,RES_MUT_DATA_WRAP,TEST_RESOURCE,$(PROCESS_INC),$(TEST_RESOURCE_LINK),,FAIL.*nc0z))
$(eval $(call tbx-hardening-mutant,RES_MUT_DATA_ESCAPE,TEST_RESOURCE,$(PROCESS_INC),$(TEST_RESOURCE_LINK),,FAIL.*u37e))
$(eval $(call tbx-hardening-mutant,SCRAP_MUT_FULL_FLAVOR,TEST_SCRAP,$(PROCESS_INC),$(TEST_SCRAP_LINK),,FAIL.*iuwq))
$(eval $(call tbx-hardening-mutant,SCRAP_MUT_NULL_DATA,TEST_SCRAP,$(PROCESS_INC),$(TEST_SCRAP_LINK),,FAIL.*m1ry))
$(eval $(call tbx-hardening-mutant,FLAIR_HEAP_MUTATE_ALIGN_OVERFLOW,TEST_FLAIR_HEAP,$(FLAIR_HEAP_INC),$(FLAIR_HEAP_C),,FAIL.*r081))
$(eval $(call tbx-hardening-mutant,FLAIR_HEAP_MUTATE_SPAN_OVERFLOW,TEST_FLAIR_HEAP,$(FLAIR_HEAP_INC),$(FLAIR_HEAP_C),,FAIL.*5l5n))
$(eval $(call tbx-hardening-mutant,FLAIR_HEAP_MUTATE_WALK_REUSE,TEST_FLAIR_HEAP,$(FLAIR_HEAP_INC),$(FLAIR_HEAP_C),,FAIL.*zhjo))
$(eval $(call tbx-hardening-mutant,PROC_MUT_REGISTERED_FREE,TEST_PROCESS,$(PROCESS_INC),$(PROCESS_LINK),,FAIL.*k0n8))
$(eval $(call tbx-hardening-mutant,PROC_MUT_STALE_MAGIC,TEST_PROCESS,$(PROCESS_INC),$(PROCESS_LINK),,FAIL.*vibs))
$(eval $(call tbx-hardening-mutant,CTRL_MUT_NULL_SILENT,TEST_CONTROL,$(CONTROL_INC),$(CONTROL_LINK),-lpng,FAIL.*gxo2))
$(eval $(call tbx-hardening-mutant,CTRL_MUT_DISABLED_HIT,TEST_CONTROL,$(CONTROL_INC),$(CONTROL_LINK),-lpng,FAIL.*ct5n))
$(eval $(call tbx-hardening-mutant,CTRL_MUT_SHORT_BUTTON,TEST_CONTROL,$(CONTROL_INC),$(CONTROL_LINK),-lpng,FAIL.*44la))
$(eval $(call tbx-hardening-mutant,CTRL_MUT_SHORT_LIVE,TEST_CONTROL,$(CONTROL_INC),$(CONTROL_LINK),-lpng,FAIL.*mktj))
$(eval $(call tbx-hardening-mutant,SHELL_MUT_MENU_COUNT,TEST_SHELL,$(SHELL_INC),$(SHELL_C) $(SHELL_LINK),-lpng,FAIL.*uskw))
$(eval $(call tbx-hardening-mutant,SHELL_MUT_SHORT_SURFACE,TEST_SHELL,$(SHELL_INC),$(SHELL_C) $(SHELL_LINK),-lpng,FAIL.*mqsn))
$(eval $(call tbx-hardening-mutant,SURFACE_MUT_BGR_SWAP,TEST_BLITTER,$(BLITTER_INC),$(BLITTER_LINK),,FAIL.*asci))
$(eval $(call tbx-hardening-mutant,BLITTER_MUT_COPY32_ZERO,TEST_BLITTER,$(BLITTER_INC),$(BLITTER_LINK),,FAIL.*asci))
$(eval $(call tbx-hardening-mutant,SURFACE_MUT_INDEX_ZERO,TEST_BLITTER,$(BLITTER_INC),$(BLITTER_LINK),,FAIL.*5le3))
$(eval $(call tbx-hardening-mutant,SURFACE_MUT_CLEAR_FLAT,TEST_BLITTER,$(BLITTER_INC),$(BLITTER_LINK),,FAIL.*gv2x))
$(eval $(call tbx-hardening-mutant,EVENT_MUT_TICK_WRAP,TEST_EVENT,$(EVENT_INC),os/flair/event.c,,FAIL.*gwts))
$(eval $(call tbx-hardening-mutant,EVENT_MUT_SILENT_DROP,TEST_EVENT,$(EVENT_INC),os/flair/event.c,,FAIL.*owpg))
$(eval $(call tbx-hardening-mutant,KBD_MUT_SILENT_DROP,TEST_KBD,-Ios/milton -Iseed,$(TEST_KBD_DEPS),,FAIL.*owpg))
$(eval $(call tbx-hardening-mutant,MOUSE_MUT_SIGN_BYTE,TEST_MOUSE_PRODUCER,$(MOUSE_PRODUCER_INC),os/flair/event.c,,FAIL.*qupy))
$(eval $(call tbx-hardening-mutant,MOUSE_MUT_EARLY_BUTTON,TEST_MOUSE_PRODUCER,$(MOUSE_PRODUCER_INC),os/flair/event.c,,FAIL.*qupy))
$(eval $(call tbx-hardening-mutant,SF_MUT_SWALLOW_NAV_ERROR,TEST_STDFILE,$(PROCESS_INC),$(TEST_STDFILE_LINK),,FAIL.*q9cr))

# Region mutants may hit the normal-form assertion BEFORE returning a value;
# isolate them in the existing region suite's child probes below.
$(eval $(call tbx-hardening-mutant,RGN_MUT_ALIAS_RESET,TEST_REGION,$(REGION_INC),$(REGION_ENGINE_C),,FAIL.*bz8f))
$(eval $(call tbx-hardening-mutant,RGN_MUT_REPEATED_UNION,TEST_REGION,$(REGION_INC),$(REGION_ENGINE_C),,FAIL.*0i1m))

.PHONY: test-flair-tbxhardening-mutant test-flair-tbxhardening-host
test-flair-tbxhardening-mutant: $(addprefix test-tbx-mutant-,$(TBX_HARDENING_MUTANTS))
	@printf '>>> test-flair-tbxhardening-mutant: green (%s named mutants RED)\n' '$(words $(TBX_HARDENING_MUTANTS))'
test-flair-tbxhardening-host: test-control test-control-mutant test-list test-list-mutant \
 test-resource test-resource-mutant test-scrap test-scrap-mutant test-flair-heap test-flair-heap-mutant \
 test-process test-process-mutant test-process-teardown test-process-teardown-mutant \
 test-process-budget test-process-budget-mutant test-process-activate test-process-activate-mutant \
 test-process-update test-process-update-mutant test-flair-shell test-flair-shell-mutant \
 test-blitter test-blitter-mutant test-fbagree test-event test-event-mutant \
 test-region test-region-mutant test-region-gdi test-region-gdi-mutant \
 test-mouse-producer test-mouse-producer-mutant test-kbd-unit test-kbd-unit-mutant \
 test-stdfile test-stdfile-mutant test-text test-flair-tbxhardening-mutant
	@printf '>>> test-flair-tbxhardening-host: green\n'
TEST_UNIT_GATES += test-flair-tbxhardening-mutant

$(KERNEL_KBD_OBJ) $(TEST_EVENT) $(TEST_KBD): os/flair/ring_report.h
$(TEST_KBD) $(TEST_KBD_MUT_RING) $(TEST_KBD_MUT_TABLE): harness/proptest/expect_abort.h os/flair/ring_report.h

.PHONY: test-flair-toolbox-hardening test-flair-toolbox-hardening-mutant test-flair-toolbox-captures
test-flair-toolbox-hardening: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) $(SCROLL_LOOK_CHECK)
	$(call scroll-look-boot,$(FLAIRTENANTS_IMG),toolbox_hardening,$(FLAIR_TOOLBOX_HARDENING_SPEC))
	@grep -qx 'FLAIR-SCROLL win 0 h part=21 value=16 max=171' $(BUILD)/toolbox_hardening.serial
	@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' $(BUILD)/toolbox_hardening.serial
	@$(SCROLL_LOOK_CHECK) click $(BUILD)/toolbox_hardening.ppm
	@printf '>>> test-flair-toolbox-hardening: green (locked trace; independent Mac OS 8.1 crops)\n'
test-flair-toolbox-hardening-mutant: test-flair-scroll-look-mutant
	@printf '>>> test-flair-toolbox-hardening-mutant: green (shared independent crop mutants RED)\n'
# qg22/ucch/whs3 are BLOCKED, never silently omitted or fabricated.
test-flair-toolbox-captures: $(TEST_CONTROL)
	@$(TEST_CONTROL) --hardening-captures
# PM 2026-10-17: test-flair-toolbox-captures is a named PREFLIGHT that stays RED until the
# pressed/disabled control captures are minted (qg22/ucch/whs3); a known-red gate never
# sits in the certificate vector (gate honesty, WL-0131). Run it by name.
TEST_EMU_GATES += test-flair-toolbox-hardening test-flair-toolbox-hardening-mutant

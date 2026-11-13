## test-adr-citations

Declared tier: HOST; #  recipe to execute (from 'harness/adr_citations.mk', line 24):
Prerequisites: `build/check_adr_citations build/.makefile.stamp`
```make
@$(ADR_CITATIONS_CHECK) --root . --adr-dir docs/adr $(ADR_CITATIONS_SCOPE) --inventory $(BUILD)/adr-citations.inventory
```

## test-adr-citations-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/adr_citations.mk', line 45):
Prerequisites: `test-adr-citations build/.makefile.stamp`
```make
$(call adr-citations-mutant,ADR_UNFILED,rm -f $(ADR_CITATIONS_MUT)/ADR_UNFILED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 is cited at)
$(call adr-citations-mutant,ADR1_DEC02_RENAMED,sed -i 's/^### DEC-02 -- /### DEC-92 -- /' $(ADR_CITATIONS_MUT)/ADR1_DEC02_RENAMED/ADR-0001-*.md,!!! test-adr-citations FAIL: ADR-0001 DEC-02 is cited at)
$(call adr-citations-mutant,ADR2_DEC03_RENAMED,sed -i 's/^### DEC-03 -- /### DEC-93 -- /' $(ADR_CITATIONS_MUT)/ADR2_DEC03_RENAMED/ADR-0002-*.md,!!! test-adr-citations FAIL: ADR-0002 DEC-03 is cited at)
$(call adr-citations-mutant,ADR8_BOGUS_CITATION,printf '%s %s\n' 'ADR-0008' 'DEC-99' > $(ADR_CITATIONS_MUT)/ADR8_BOGUS_CITATION/bogus.md,DEC-99 is cited at 1 site(s),$(ADR_CITATIONS_MUT)/ADR8_BOGUS_CITATION)
@printf '>>> test-adr-citations-mutant: green (4 mutants RED for the named reason)\n'
```

## test-bochs-completion

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 266):
Prerequisites: `build/test_bochs_completion build/.makefile.stamp`
```make
@$<
```

## test-bochs-completion-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 268):
Prerequisites: `build/test_bochs_completion_mut build/.makefile.stamp`
```make
@if $< > $(BUILD)/bochs_completion_mut.log 2>&1; then echo 'FAIL BOCHS_MUTATE_NO_REBOOT_WAIT survived'; exit 1; fi
@grep -qxF 'RED bochs-completion: one boot must not satisfy a two-boot wait' $(BUILD)/bochs_completion_mut.log
@printf 'GREEN test-bochs-completion-mutant: BOCHS_MUTATE_NO_REBOOT_WAIT caught: %s\n' "$$(cat $(BUILD)/bochs_completion_mut.log)"
```

## test-bochs-completion-partial-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 285):
Prerequisites: `build/test_bochs_completion_partial build/.makefile.stamp`
```make
@if $< > build/bochs_completion_partial.log 2>&1; then echo 'FAIL BOCHS_MUTATE_PARTIAL_LINE survived'; exit 1; fi
@grep -qxF 'RED bochs-completion: partial serial lines must not count as boot lines' build/bochs_completion_partial.log
@printf 'GREEN test-bochs-completion-partial-mutant: BOCHS_MUTATE_PARTIAL_LINE caught: %s\n' "$$(cat build/bochs_completion_partial.log)"
```

## test-bochs-completion-substring-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 284):
Prerequisites: `build/test_bochs_completion_substring build/.makefile.stamp`
```make
@if $< > build/bochs_completion_substring.log 2>&1; then echo 'FAIL BOCHS_MUTATE_MARKER_SUBSTRING survived'; exit 1; fi
@grep -qxF 'RED bochs-completion: marker substrings must not count as boot lines' build/bochs_completion_substring.log
@printf 'GREEN test-bochs-completion-substring-mutant: BOCHS_MUTATE_MARKER_SUBSTRING caught: %s\n' "$$(cat build/bochs_completion_substring.log)"
```

## test-bochs-concurrent

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 12225):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-bochs-concurrent: two Bochs runs started together must both reach the banner (d9tt)\n'
@$(call bochs-concurrent-pair,$(BOCHS_BIN),bochs_conc); \
if [ "$$good" -ne 2 ]; then \
	printf '!!! test-bochs-concurrent FAIL: %s of 2 concurrent Bochs runs reached the banner\n' "$$good"; \
	cat $(BUILD)/bochs_conc_a.report.txt $(BUILD)/bochs_conc_b.report.txt; exit 1; fi
@printf '>>> test-bochs-concurrent: green (2 of 2 reached the banner)\n'
```

## test-bochs-concurrent-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 12233):
Prerequisites: `build/bochs_harness_mut_nolock build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-bochs-concurrent-mutant: confirming the NO-HOST-LOCK mutant goes RED (Rule 6)\n'
@exec 9>/tmp/initech-bochs.lock; flock 9; \
$(call bochs-concurrent-pair,$(BOCHS_MUT_NOLOCK_BIN),bochs_conc_mut); \
if [ "$$good" -eq 2 ]; then \
	printf '!!! test-bochs-concurrent-mutant FAIL: both unlocked runs passed -- the gate is decoration\n'; exit 1; fi; \
printf '>>> test-bochs-concurrent-mutant: green (NO_HOST_LOCK correctly RED -- %s of 2 reached the banner)\n' "$$good"
```

## test-bochs-legs

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 63):
Prerequisites: `build/test_bochs_legs build/.makefile.stamp`
```make
@sh harness/emu/test_bochs_legs.sh $(BUILD) green
```

## test-bochs-legs-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/bochs_legs.mk', line 65):
Prerequisites: `build/test_bochs_legs build/.makefile.stamp`
```make
@sh harness/emu/test_bochs_legs.sh $(BUILD) mutants
```

## test-bochs-pending

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12140):
Prerequisites: `test-samir-boot-bochs test-samir-boot-bochs-mutant build/.makefile.stamp`
```make
@printf '>>> test-bochs-pending: green (oracle completed)\n'
```

## test-bochs-reboot

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 314):
Prerequisites: `build/bochs_harness build/bochs_reboot.img build/.makefile.stamp`
```make
$(call bochs-reboot-run,$(BOCHS_BIN),bochs_reboot_green)
@cat $(BUILD)/bochs_reboot_green.report
@grep -q 'OK=1' $(BUILD)/bochs_reboot_green.report || { echo 'RED bochs-reboot: harness did not complete'; exit 1; }
@sh harness/emu/bochs_reboot_check.sh $(BUILD)/bochs_reboot_green.serial
```

## test-bochs-reboot-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 322):
Prerequisites: `build/bochs_harness_mut_nowait build/bochs_no_reboot.img build/.makefile.stamp`
```make
cp -f $(BUILD)/bochs_no_reboot.img $(BUILD)/bochs_reboot_nowait.img
@$(BUILD)/bochs_harness_mut_nowait --disk $(BUILD)/bochs_reboot_nowait.img --name bochs_reboot_nowait --out $(BUILD) \
 --expect BOCHS-REBOOT-READY --allow-reboot --quit-after BOCHS-REBOOT-READY \
 --quit-after-count 2 --timeout-ms 90000 2> $(BUILD)/bochs_reboot_nowait.report || true
@grep -q 'OK=1' $(BUILD)/bochs_reboot_nowait.report || { cat $(BUILD)/bochs_reboot_nowait.report; echo 'FAIL no-wait mutant did not run'; exit 1; }
@if sh harness/emu/bochs_reboot_check.sh $(BUILD)/bochs_reboot_nowait.serial > $(BUILD)/bochs_reboot_nowait.check 2>&1; then echo 'FAIL BOCHS_MUTATE_NO_REBOOT_WAIT survived Bochs'; exit 1; fi
@grep -qxF 'RED bochs-reboot: READY seen 1 times, want 2 (must wait for reboot)' $(BUILD)/bochs_reboot_nowait.check
@printf 'GREEN test-bochs-reboot-mutant: BOCHS_MUTATE_NO_REBOOT_WAIT caught: %s\n' "$$(cat $(BUILD)/bochs_reboot_nowait.check)"
```

## test-bochs-trace

Declared tier: HOST; #  recipe to execute (from 'harness/emu/copybreak.mk', line 104):
Prerequisites: `build/test_bochs_trace build/.makefile.stamp`
```make
@$<
@printf '>>> test-bochs-trace: green (oracle completed)\n'
```

## test-bochs-trace-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/copybreak.mk', line 107):
Prerequisites: `build/test_bochs_trace_mut build/test_bochs_trace_origin_mut build/test_bochs_trace_sync_mut build/test_bochs_trace_capture_mut build/.makefile.stamp`
```make
@if $< >$(BUILD)/bochs_trace_mut.log 2>&1; then echo 'FAIL Return mutant survived'; exit 1; fi
@grep -qx 'RED bochs trace parser' $(BUILD)/bochs_trace_mut.log
@if $(BUILD)/test_bochs_trace_origin_mut >$(BUILD)/bochs_trace_origin_mut.log 2>&1; then echo 'FAIL pointer origin mutant survived'; exit 1; fi
@grep -qx 'RED bochs pointer origin: opening step 0 at 574,479 want 220,140' $(BUILD)/bochs_trace_origin_mut.log
@if $(BUILD)/test_bochs_trace_sync_mut >$(BUILD)/bochs_trace_sync_mut.log 2>&1; then echo 'FAIL free-running replay clock mutant survived'; exit 1; fi
@grep -qx 'RED bochs replay clock: sync=none, [0-9]* ms host-paced trace outlives the 15000 ms guest budget' $(BUILD)/bochs_trace_sync_mut.log
@if $(BUILD)/test_bochs_trace_capture_mut >$(BUILD)/bochs_trace_capture_mut.log 2>&1; then echo 'FAIL uncaptured mouse mutant survived'; exit 1; fi
@grep -qx 'RED bochs pointer origin: opening step 0 at 320,240 want 220,140' $(BUILD)/bochs_trace_capture_mut.log
@echo 'GREEN bochs trace mutants: Return, unsynchronised pointer origin, free-running replay clock and uncaptured mouse rejected'
@printf '>>> test-bochs-trace-mutant: green (oracle completed)\n'
```

## test-cert-runner

Declared tier: HOST; #  recipe to execute (from 'harness/cert_runner.mk', line 5):
Prerequisites: `build/.makefile.stamp`
```make
@tools/cert-sharded-selftest.sh
```

## test-cert-runner-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/cert_runner.mk', line 7):
Prerequisites: `build/.makefile.stamp | build`
```make
@sed 's/^    *flock 9$$/    : # mutant: queue flock removed/' tools/cert-sharded.sh > $(BUILD)/cert-sharded-nolock.sh
@chmod +x $(BUILD)/cert-sharded-nolock.sh
@rc=0; tools/cert-sharded-selftest.sh $(BUILD)/cert-sharded-nolock.sh > $(BUILD)/cert-runner-mutant.log 2>&1 || rc=$$?; \
if [ "$$rc" != 1 ] || ! grep -q '^RED cert-runner QUEUE-LOCK:' $(BUILD)/cert-runner-mutant.log; then \
	cat $(BUILD)/cert-runner-mutant.log; echo 'FAIL cert-runner: queue flock mutant survived or failed infrastructure'; exit 1; fi
@cat $(BUILD)/cert-runner-mutant.log
@echo 'GREEN test-cert-runner-mutant: queue flock removal caught'
```

## test-egg-jtc-pubs

Declared tier: outside active vector; #  recipe to execute (from 'harness/proptest/egg_jtc.mk', line 37):
Prerequisites: `pubs-inventory build/.makefile.stamp`
```make
@! grep -Fi 'Jump to Conclusions' $(PUBS_CHECK)/shipped
@! grep -RFi 'Jump to Conclusions' pubs --include='*.tex' --include='*.cmds'
@printf '>>> test-egg-jtc-pubs: green (hidden title absent from shipped inventory and manuals; no exemption needed)\n'
```

## test-emu

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 31907):
Prerequisites: `build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf '>>> test-emu: QEMU keystone vector (%s gates) -- in-emulator boot\n' "$(words $(TEST_EMU_GATES))"
@printf '======================================================================\n'
@for g in $(TEST_EMU_GATES); do \
	printf '>>> test-emu: running %s\n' "$$g"; \
	$(MAKE) --no-print-directory $$g || { \
		printf '!!! test-emu FAIL: gate %s went RED\n' "$$g"; exit 1; }; \
done
@printf '======================================================================\n'
@printf 'test-emu: ALL GREEN (%s QEMU gates)\n' "$(words $(TEST_EMU_GATES))"
@printf '======================================================================\n'
```

## test-fixture-volumes

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14032):
Prerequisites: `build/check_fixture_volumes build/.makefile.stamp`
```make
@$(MAKE) --no-print-directory -s fixture-volume-recipe-lint
@# initech-1x6x2.1: -q, not -n. Under a parallel parent make the -n command
@# echo of `help` ("printf 'Build:...") was flushed into the middle of the -p
@# database ("unsupported target in make database"). -q prints the same
@# database and echoes nothing; its exit 1 just means `help` is out of date.
@rc=0; $(MAKE) --no-print-directory -pq help SKIP_BOCHS=0 > $(FIXTURE_VOLUMES_SCAN).db || rc=$$?; \
 [ "$$rc" -le 1 ] || { printf '!!! test-fixture-volumes FAIL: the make database dump failed (exit %s)\n' "$$rc"; exit 1; }
@$(FIXTURE_VOLUMES_CHECK) --probe $(FIXTURE_VOLUMES_SCAN).db $(FIXTURE_VOLUMES_SCAN).mk $(FIXTURE_VOLUMES_SCAN).expanded
@# Grouped pattern targets must reach the probe whole (both members, one recipe).
@grep -q '^tflp_target_[0-9]* := $(BUILD)/flairovl_%\.ovl $(BUILD)/flairovl_%\.sys$$' $(FIXTURE_VOLUMES_SCAN).mk \
 || { printf '!!! test-fixture-volumes FAIL: grouped flairovl pattern target missing from the probe\n'; exit 1; }
@$(MAKE) --no-print-directory -s -f $(firstword $(MAKEFILE_LIST)) -f $(FIXTURE_VOLUMES_SCAN).mk fixture-volume-probe SKIP_BOCHS=0
@$(FIXTURE_VOLUMES_CHECK) --check $(FIXTURE_VOLUMES_SCAN).expanded $(FIXTURE_VOLUMES_SCAN).tsv
```

## test-fixture-volumes-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14046):
Prerequisites: `test-fixture-volumes build/.makefile.stamp`
```make
@if $(MAKE) --no-print-directory test-fixture-volumes FLAIR_I123_QUIT_DATA=$(BUILD)/flair_i123_quit_data.img FIXTURE_VOLUMES_SCAN=$(BUILD)/fixture-volumes-mutant > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: QUIT_DATA_COLLISION survived\n'; exit 1; fi
@grep -F 'scratch $(BUILD)/flair_i123_quit_data.img (gate test-flair-i123) collides with fixture $(BUILD)/flair_i123_quit_data.img' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: wrong RED reason\n'; exit 1; }
@# Ref: PRD Sec 8; CLAUDE.md Rule 6; initech-o0cb. A Bochs-only
@# scratch copy must collide even when execution of Bochs legs is skipped.
@if $(MAKE) --no-print-directory test-fixture-volumes SKIP_BOCHS=1 FLAIR_ALERTCHROME_BOCHS_DATA=$(FLAIR_I123_QUIT_DATA) FIXTURE_VOLUMES_SCAN=$(BUILD)/fixture-volumes-mutant > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: BOCHS_DATA_COLLISION survived with SKIP_BOCHS=1\n'; exit 1; fi
@grep -F 'scratch $(FLAIR_I123_QUIT_DATA) (gate test-flair-alert-chrome-bochs) collides with fixture $(FLAIR_I123_QUIT_DATA)' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: BOCHS_DATA_COLLISION wrong RED reason\n'; exit 1; }
@# initech-a5sr mutants. HANDLIST_DRIFT: the pre-8080833 hand list (no
@# no-block-on InitechWord volume) -- the exact 2026-10-16 regression.
@if $(MAKE) --no-print-directory test-fixture-volumes FIXTURE_MUT_HANDLIST=1 FIXTURE_VOLUMES_SCAN=$(BUILD)/fixture-volumes-mutant > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: HANDLIST_DRIFT survived\n'; exit 1; fi
@grep -F '!!! test-fixture-volumes FAIL: flair-data image $(FLAIR_DATA_IWORD_NOBLK_IMG) has no declared fixture contract' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: HANDLIST_DRIFT wrong RED reason\n'; exit 1; }
@# UNDECLARED_VARIANT: a FLAIR_DATA_ZZZ_IMG flair-data volume with no contract.
@if $(MAKE) --no-print-directory test-fixture-volumes FIXTURE_MUT_UNDECLARED_VARIANT=1 FIXTURE_VOLUMES_SCAN=$(BUILD)/fixture-volumes-mutant > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: UNDECLARED_VARIANT survived\n'; exit 1; fi
@grep -F '!!! test-fixture-volumes FAIL: flair-data image $(BUILD)/flair_data_zzz.img has no declared fixture contract' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: UNDECLARED_VARIANT wrong RED reason\n'; exit 1; }
@# UNSEEDED_MASK: the C inventory leg alone, fed the real expansion minus
@# one seeded volume's contract, must not let the @unseeded fallback hide it.
@awk -v p='@@contract $(FLAIR_DATA_IWORD_NOBLK_IMG) ' 'index($$0, p) != 1' $(FIXTURE_VOLUMES_SCAN).expanded > $(BUILD)/fixture-volumes-unseeded.expanded
@test $$(( $$(wc -l < $(FIXTURE_VOLUMES_SCAN).expanded) - $$(wc -l < $(BUILD)/fixture-volumes-unseeded.expanded) )) -eq 1 \
 || { printf '!!! test-fixture-volumes-mutant FAIL: UNSEEDED_MASK did not drop exactly one contract line\n'; exit 1; }
@if $(FIXTURE_VOLUMES_CHECK) --check $(BUILD)/fixture-volumes-unseeded.expanded $(BUILD)/fixture-volumes-unseeded.tsv > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: UNSEEDED_MASK survived\n'; exit 1; fi
@grep -F '!!! test-fixture-volumes FAIL: fixture $(FLAIR_DATA_IWORD_NOBLK_IMG) has no declared contract' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: UNSEEDED_MASK wrong RED reason\n'; exit 1; }
@# BROKEN_CONTRACT: the end-of-recipe verifier fails; the half-built image
@# must be deleted, and the next plain make must REBUILD it (not call it
@# up to date) -- the stale-image GREEN of 2026-10-16.
@rm -f $(FIXTURE_RMPROOF_IMG)
@if $(MAKE) --no-print-directory $(FIXTURE_RMPROOF_IMG) FIXTURE_MUT_BROKEN_CONTRACT=1 > $(BUILD)/fixture-volumes-mutant.log 2>&1; then \
 printf '!!! test-fixture-volumes-mutant FAIL: BROKEN_CONTRACT survived\n'; exit 1; fi
@grep -F 'fixture-volume recipe ERROR: $(FIXTURE_RMPROOF_IMG): missing fixture ::ZZZMUT.TXT' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: BROKEN_CONTRACT wrong RED reason\n'; exit 1; }
@test ! -e $(FIXTURE_RMPROOF_IMG) \
 || { printf '!!! test-fixture-volumes-mutant FAIL: BROKEN_CONTRACT left the half-built %s on disk\n' '$(FIXTURE_RMPROOF_IMG)'; exit 1; }
@$(MAKE) --no-print-directory $(FIXTURE_RMPROOF_IMG) > $(BUILD)/fixture-volumes-mutant.log 2>&1 \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: clean rebuild of %s failed\n' '$(FIXTURE_RMPROOF_IMG)'; exit 1; }
@grep -F '>>> fixture_rmproof: minted $(FIXTURE_RMPROOF_IMG)' $(BUILD)/fixture-volumes-mutant.log \
 || { cat $(BUILD)/fixture-volumes-mutant.log; printf '!!! test-fixture-volumes-mutant FAIL: next make did not rebuild %s\n' '$(FIXTURE_RMPROOF_IMG)'; exit 1; }
@printf '>>> test-fixture-volumes-mutant: green (QUIT_DATA_COLLISION, BOCHS_DATA_COLLISION, HANDLIST_DRIFT, UNDECLARED_VARIANT, UNSEEDED_MASK, BROKEN_CONTRACT correctly RED; failed image deleted and rebuilt; real Makefile restored)\n'
```

## test-flair-toolbox-captures

Declared tier: outside active vector; #  recipe to execute (from 'harness/toolbox_hardening.mk', line 81):
Prerequisites: `build/test_control build/.makefile.stamp`
```make
@$(TEST_CONTROL) --hardening-captures
```

## test-for

Declared tier: outside active vector; #  recipe to execute (from 'harness/gate_map.mk', line 50):
Prerequisites: `build/gate_map build/gate_inventory.txt build/.makefile.stamp`
```make
@set -eu; d=$$(mktemp -d $(BUILD)/gate-map-run.XXXXXX); \
 $(MAKE) --no-print-directory -f $(GATE_MAP_MAKEFILE) -s gates-for > "$$d/gates"; \
 : > "$$d/green"; \
 awk -v green="$$d/green" '{print ".PHONY: gate-map-done-" $$0; print "gate-map-done-" $$0 ": " $$0; print "\t@printf '\''%s\\n'\'' '\''" $$0 "'\'' >> " green}' "$$d/gates" > "$$d/done.mk"; \
 gates=$$(cat "$$d/gates"); done_gates=$$(sed 's/^/gate-map-done-/' "$$d/gates"); rc=0; \
 $(MAKE) --no-print-directory -k -j16 -f $(GATE_MAP_MAKEFILE) -f "$$d/done.mk" $$gates $$done_gates || rc=$$?; \
 n=$$(wc -l < "$$d/gates"); green=$$(wc -l < "$$d/green"); red=$$((n - green)); \
 printf '>>> test-for: %s gates, %s red\n' "$$n" "$$red"; \
 [ "$$rc" -eq 0 ] && [ "$$red" -eq 0 ]
```

## test-gate-honesty

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14095):
Prerequisites: `harness/test_gate_honesty.sh harness/missing_reference.sh harness/oracle_policy.h build/.makefile.stamp | build`
```make
@sh harness/test_gate_honesty.sh "$(MAKE)" "$(abspath $(firstword $(MAKEFILE_LIST)))" "$(abspath $(BUILD)/gate-honesty)"
```

## test-gate-honesty-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14097):
Prerequisites: `harness/test_gate_honesty.sh build/.makefile.stamp | build`
```make
@sh harness/test_gate_honesty.sh "$(MAKE)" "$(abspath $(firstword $(MAKEFILE_LIST)))" "$(abspath $(BUILD)/gate-honesty-mutant)" "CFLAGS=$(CFLAGS) -DGATE_HONESTY_MUTATE_SKIP" > $(BUILD)/gate-honesty-mutant.log 2>&1; rc=$$?; \
[ "$$rc" -ne 0 ] && grep -qF 'test-gate-honesty FAIL: test-chicago-metrics accepted missing SYSTEM7_DECOMP' $(BUILD)/gate-honesty-mutant.log \
	|| { cat $(BUILD)/gate-honesty-mutant.log; printf '!!! test-gate-honesty-mutant FAIL: old skip-to-green policy was not caught for the named reason\n'; exit 1; }; \
printf '>>> test-gate-honesty-mutant: green (old Chicago LOUD-SKIP-to-green correctly RED on missing SYSTEM7_DECOMP)\n'
```

## test-gate-map

Declared tier: HOST; #  recipe to execute (from 'harness/gate_map.mk', line 61):
Prerequisites: `build/gate_map build/gate_inventory.txt spec/gate_map.json build/.makefile.stamp`
```make
@set -eu; p=$$(mktemp $(BUILD)/gate-map-tracked.XXXXXX); trap 'rm -f "$$p"' EXIT HUP INT TERM; \
 git -c core.quotePath=false ls-files > "$$p"; \
 $(GATE_MAP_TOOL) --audit $(GATE_MAP_JSON) $(GATE_MAP_INVENTORY) < "$$p"; \
 $(GATE_MAP_TOOL) --audit-consumers $(GATE_MAP_JSON) $(GATE_MAP_INVENTORY) $(GATE_MAP_INVENTORY).db < "$$p"
@$(MAKE) -s --no-print-directory gates-for FILES=os/samir/samir_main.c DIFF= > $(BUILD)/gate-map-samir-selection.txt
@if grep -qxF test-samir-status-screen-pending $(BUILD)/gate-map-samir-selection.txt; then \
 printf 'RED pending selection: STATUS must be opt-in\n'; exit 1; fi
@printf '>>> test-gate-map: green (structural/consumer audit; pending STATUS opt-in)\n'
```

## test-gate-map-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/gate_map.mk', line 76):
Prerequisites: `test-gate-map build/.makefile.stamp`
```make
@set -eu; for m in drop-family duplicate-gate drop-consumer pending-active pending-fns-active; do \
 if $(MAKE) --no-print-directory -f $(GATE_MAP_MAKEFILE) test-gate-map GATE_MAP_MUT=$$m > $(BUILD)/gate-map-$$m.log 2>&1; then \
 printf 'gate-map RED: %s mutant survived\n' "$$m"; exit 1; fi; \
 case $$m in pending-active) reason='active gate test-boot in opt-in pending family';; pending-fns-active) reason='active gate test-samir-fns-row in opt-in pending family';; drop-family) reason='unmapped gate';; duplicate-gate) reason='listed twice';; \
 drop-consumer) reason='consumer harness/proptest/serial_trace_fixture.c -> test-serial-trace-pack (family harness not selected)'; \
   git -c core.quotePath=false ls-files | $(GATE_MAP_TOOL) --audit $(BUILD)/gate-map-$$m.json $(GATE_MAP_INVENTORY);; esac; \
 grep -m1 "gate-map RED: .*$$reason" $(BUILD)/gate-map-$$m.log || { cat $(BUILD)/gate-map-$$m.log; exit 1; }; \
 done; printf '>>> test-gate-map-mutant: green (drop-family, duplicate-gate, drop-consumer, pending-active and pending-fns-active correctly RED; drop-consumer passes structural audit)\n'
```

## test-harness

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 19145):
Prerequisites: `build/qemu_harness build/serial_hello.elf build/triple_fault.elf build/.makefile.stamp`
```make
@printf ">>> test-harness: GOOD fixture (serial_hello) -- expect marker, no fault\n"
@$(HARNESS_BIN) --kernel "$(SERIAL_HELLO_ELF)" --expect "$(HARNESS_MARKER)" \
	--name th_good --timeout-ms 5000 \
	|| { printf "!!! test-harness FAIL: serial_hello did not satisfy the oracle (marker missing, fault, or timeout)\n"; exit 1; }
@printf ">>> test-harness: BAD fixture (triple_fault) -- expect fault detected, marker absent\n"
@rep=$$($(HARNESS_BIN) --kernel "$(TRIPLE_FAULT_ELF)" --expect "$(HARNESS_MARKER)" \
	--name th_bad --timeout-ms 5000 2>&1); \
	printf '%s\n' "$$rep"; \
	echo "$$rep" | grep -q 'triple_fault=1' \
		|| { printf "!!! test-harness FAIL: triple_fault was NOT detected on the bad fixture\n"; exit 1; }; \
	echo "$$rep" | grep -q 'marker_found=0' \
		|| { printf "!!! test-harness FAIL: HARNESS-OK marker unexpectedly present on the bad fixture\n"; exit 1; }; \
	echo "$$rep" | grep -q 'timed_out=0' \
		|| { printf "!!! test-harness FAIL: bad fixture timed out instead of exiting on the fault\n"; exit 1; }
@printf ">>> test-harness: all green (marker caught on good fixture, triple-fault caught on bad)\n"
```

## test-harness-bare-keys

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18809):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-harness-bare-keys: bare --keys (no --quit-after/--screendump-after/--record/--legacy-quit) must be REFUSED\n'
@rm -f "$(BUILD)/harness_bare_keys.stderr" "$(BUILD)/harness_bare_keys_ok.stderr"
@if $(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	--name harness_bare_keys_probe --out "$(BUILD)" --timeout-ms 5000 \
	>/dev/null 2>"$(BUILD)/harness_bare_keys.stderr"; then \
	printf '!!! test-harness-bare-keys FAIL: the harness ACCEPTED a bare --keys invocation -- the initech-qed1 safety check is not wired\n'; exit 1; \
fi
@grep -q 'initech-qed1' "$(BUILD)/harness_bare_keys.stderr" \
	|| { printf '!!! test-harness-bare-keys FAIL: the harness refused for the WRONG reason (no initech-qed1 diagnostic on stderr):\n'; cat "$(BUILD)/harness_bare_keys.stderr"; exit 1; }
@printf '>>> test-harness-bare-keys [1/2]: the real harness refused the bare invocation, citing initech-qed1\n'
@# leg 2: the SAME bare invocation + --legacy-quit must proceed (the opt-out works).
@if ! $(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" --legacy-quit \
	--name harness_bare_keys_optout --out "$(BUILD)" --timeout-ms 15000 \
	>/dev/null 2>"$(BUILD)/harness_bare_keys_ok.stderr"; then \
	printf '!!! test-harness-bare-keys FAIL: --legacy-quit did NOT let the invocation proceed\n'; cat "$(BUILD)/harness_bare_keys_ok.stderr"; exit 1; \
fi
@if grep -q 'initech-qed1' "$(BUILD)/harness_bare_keys_ok.stderr"; then \
	printf '!!! test-harness-bare-keys FAIL: --legacy-quit was passed but the safety check still fired\n'; exit 1; \
fi
@printf '>>> test-harness-bare-keys [2/2]: --legacy-quit is an honest opt-out (the invocation proceeded, no refusal)\n'
@printf 'VERDICT   : PASS -- the initech-qed1 bare-injection safety check refuses by default and --legacy-quit is a real opt-out\n'
```

## test-harness-bare-keys-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18835):
Prerequisites: `build/qemu_harness_mut_nocheck build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '>>> test-harness-bare-keys-mutant: HARNESS_MUTATE_NO_BARE_KEYS_CHECK must ACCEPT the bare invocation (Rule 6)\n'
@rm -f "$(BUILD)/harness_bare_keys_mut.stderr"
@if ! $(HARNESS_MUT_NOCHECK_BIN) --disk "$(SECONDARY_TRACER_IMG)" \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	--name harness_bare_keys_mut --out "$(BUILD)" --timeout-ms 15000 \
	>/dev/null 2>"$(BUILD)/harness_bare_keys_mut.stderr"; then \
	printf '!!! test-harness-bare-keys-mutant FAIL: the mutant STILL refused the bare invocation -- the check-disable mutant is not effective, so test-harness-bare-keys proves nothing\n'; cat "$(BUILD)/harness_bare_keys_mut.stderr"; exit 1; \
fi
@printf '>>> test-harness-bare-keys-mutant: green (the check-disabled build accepted the SAME bare invocation the real harness refuses -- the gate bites)\n'
```

## test-harness-key-ack

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12149):
Prerequisites: `build/test_qemu_key_ack build/.makefile.stamp`
```make
@$<
```

## test-harness-key-ack-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12151):
Prerequisites: `test-harness-key-ack build/test_qemu_key_ack_mut build/.makefile.stamp`
```make
@if $(BUILD)/test_qemu_key_ack_mut > $(BUILD)/qemu_key_ack_mut.log 2>&1; then printf '!!! key ack mutant survived\n'; exit 1; fi
@grep '^RED qemu-key-ack: --mouse k must wait for each new guest release' $(BUILD)/qemu_key_ack_mut.log
@printf '>>> test-harness-key-ack-mutant: green (NO_KEY_ACK correctly RED: next key outruns guest consumption)\n'
```

## test-harness-key-pace

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12164):
Prerequisites: `build/test_qemu_key_pace build/.makefile.stamp`
```make
@$<
```

## test-harness-key-pace-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12166):
Prerequisites: `test-harness-key-pace build/test_qemu_key_pace_mut build/.makefile.stamp`
```make
@if $(BUILD)/test_qemu_key_pace_mut > $(BUILD)/qemu_key_pace_mut.log 2>&1; then printf '!!! key pace mutant survived\n'; exit 1; fi
@grep '^RED qemu-key-pace: a busy QEMU must receive the CPU budget' $(BUILD)/qemu_key_pace_mut.log
@printf '>>> test-harness-key-pace-mutant: green (NO_KEY_PACE correctly RED: next stroke outruns a starved guest)\n'
```

## test-harness-progress

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12122):
Prerequisites: `build/test_qemu_progress build/.makefile.stamp`
```make
@$(BUILD)/test_qemu_progress
```

## test-harness-progress-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12129):
Prerequisites: `build/test_qemu_progress_mut build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_qemu_progress_mut > $(BUILD)/qemu_progress_mut.log 2>&1; rc=$$?; set -e; \
  cat $(BUILD)/qemu_progress_mut.log; \
  test $$rc -eq 1 && grep -q '^RED qemu-progress: echoes must renew the completion deadline' $(BUILD)/qemu_progress_mut.log \
  || { printf '!!! test-harness-progress-mutant FAIL: missing expected progress-reset RED\n'; exit 1; }
@printf '>>> test-harness-progress-mutant: green (disabled progress-reset branch went RED)\n'
```

## test-harness-qemu-log

Declared tier: EMU; #  recipe to execute (from 'harness/emu/qemu_log.mk', line 20):
Prerequisites: `build/qemu_harness build/serial_hello.elf build/triple_fault.elf build/.makefile.stamp`
```make
@sh harness/emu/test_qemu_log.sh $(HARNESS_BIN) $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF) $(BUILD)
```

## test-harness-qemu-log-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/qemu_log.mk', line 22):
Prerequisites: `build/qemu_harness_mut_keep_log build/qemu_harness_mut_no_log_cap build/serial_hello.elf build/triple_fault.elf build/.makefile.stamp`
```make
@set -eu; for m in keep_log no_log_cap; do \
 log=$(BUILD)/qemu-log-mut-$$m.log; \
 if sh harness/emu/test_qemu_log.sh $(BUILD)/qemu_harness_mut_$$m $(SERIAL_HELLO_ELF) $(TRIPLE_FAULT_ELF) $(BUILD) > $$log 2>&1; then \
   cat $$log; printf '!!! test-harness-qemu-log-mutant FAIL: %s survived\n' $$m; exit 1; \
 fi; \
 case $$m in keep_log) why=GREEN-RETAINED ;; no_log_cap) why=CAP-MISSING ;; esac; \
 grep "^RED qemu-log: $$why" $$log || { cat $$log; exit 1; }; \
 done
@printf '>>> test-harness-qemu-log-mutant: green (retention and cap mutants both RED)\n'
```

## test-harness-qemu-slots

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12180):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@$(call qemu_slot_pair,gate,$(HARNESS_BIN)); test $$ms -ge 5500 || { printf '!!! test-harness-qemu-slots: one slot ran two QEMUs together (%s ms)\n' $$ms; exit 1; }; \
printf '>>> test-harness-qemu-slots: green (one slot serialised two runs: %s ms)\n' $$ms
```

## test-harness-qemu-slots-mutant

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 12183):
Prerequisites: `test-harness-qemu-slots build/qemu_harness_mut_noslots build/tracer_secondary.img build/.makefile.stamp`
```make
@$(call qemu_slot_pair,mut,$(BUILD)/qemu_harness_mut_noslots); test $$ms -lt 5500 || { printf '!!! test-harness-qemu-slots-mutant: mutant still queued (%s ms)\n' $$ms; exit 1; }; \
printf '>>> test-harness-qemu-slots-mutant: green (NO_QEMU_SLOTS correctly RED: two QEMUs together, %s ms)\n' $$ms
```

## test-harness-quit-after

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18744):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/harness_quit_after.img build/.makefile.stamp`
```make
@printf '>>> test-harness-quit-after: typed-ahead exit + --quit-after SHELL-EXIT must let a %s-EXEC batch finish\n' "$(HARNESS_QA_EXECS)"
@rm -f "$(BUILD)/$(HARNESS_QA_NAME).serial"
@$(call fixture-preflight,$(HARNESS_QA_IMG))
@cp -f "$(HARNESS_QA_IMG)" "$(BUILD)/$(HARNESS_QA_NAME)_data.img"
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BUILD)/$(HARNESS_QA_NAME)_data.img" \
	--name "$(HARNESS_QA_NAME)" --out "$(BUILD)" --timeout-ms 120000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(BUILD)/$(HARNESS_QA_NAME).report" || true
@sh harness/emu/require_trace.sh "$(BUILD)/$(HARNESS_QA_NAME).report" "$(BUILD)/$(HARNESS_QA_NAME).serial" 5
@if grep -q 'triple_fault=1' "$(BUILD)/$(HARNESS_QA_NAME).report"; then \
	printf '!!! test-harness-quit-after FAIL: boot triple-faulted\n'; exit 1; fi
@grep -q 'quit_after_found=1' "$(BUILD)/$(HARNESS_QA_NAME).report" \
	|| { printf '!!! test-harness-quit-after FAIL: harness did not see SHELL-EXIT before quitting\n'; exit 1; }
@tr -d '\r' < "$(BUILD)/$(HARNESS_QA_NAME).serial" > "$(BUILD)/$(HARNESS_QA_NAME).normalized"
@n=$$(grep -c '^TPS-CORPUS-END ' "$(BUILD)/$(HARNESS_QA_NAME).normalized"); \
	test "$$n" -eq $(HARNESS_QA_EXECS) \
	|| { printf '!!! test-harness-quit-after FAIL: %s/%s batch programs completed\n' "$$n" "$(HARNESS_QA_EXECS)"; exit 1; }
@grep -q '^SHELL-EXIT$$' "$(BUILD)/$(HARNESS_QA_NAME).normalized" \
	|| { printf '!!! test-harness-quit-after FAIL: SHELL-EXIT missing from serial\n'; exit 1; }
@printf '>>> test-harness-quit-after: green (%s/%s EXECs completed, typed-ahead exit reached the prompt, harness quit only after SHELL-EXIT)\n' "$(HARNESS_QA_EXECS)" "$(HARNESS_QA_EXECS)"
```

## test-harness-quit-after-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 18768):
Prerequisites: `build/qemu_harness_mut_noquit build/tracer_secondary.img build/harness_quit_after.img build/.makefile.stamp`
```make
@printf '>>> test-harness-quit-after-mutant: HARNESS_MUTATE_NO_QUIT_WAIT must truncate the batch (Rule 6)\n'
@rm -f "$(BUILD)/$(HARNESS_QA_MUT_NAME).serial"
@$(call fixture-preflight,$(HARNESS_QA_IMG))
@cp -f "$(HARNESS_QA_IMG)" "$(BUILD)/$(HARNESS_QA_MUT_NAME)_data.img"
@$(HARNESS_MUT_NOQUIT_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BUILD)/$(HARNESS_QA_MUT_NAME)_data.img" \
	--name "$(HARNESS_QA_MUT_NAME)" --out "$(BUILD)" --timeout-ms 120000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(TPS_OS_COMPLETION) \
	2> "$(BUILD)/$(HARNESS_QA_MUT_NAME).report" || true
@sh harness/emu/require_trace.sh "$(BUILD)/$(HARNESS_QA_MUT_NAME).report" "$(BUILD)/$(HARNESS_QA_MUT_NAME).serial" 5
@test -s "$(BUILD)/$(HARNESS_QA_MUT_NAME).serial" \
	|| { printf '!!! test-harness-quit-after-mutant FAIL: no serial from the mutant boot\n'; exit 1; }
@grep -q 'SHELL-READY' "$(BUILD)/$(HARNESS_QA_MUT_NAME).serial" \
	|| { printf '!!! test-harness-quit-after-mutant FAIL: mutant boot never reached SHELL-READY (wrong reason)\n'; exit 1; }
@grep -q 'quit_after_found=1' "$(BUILD)/$(HARNESS_QA_MUT_NAME).report" \
	|| { printf '!!! test-harness-quit-after-mutant FAIL: mutant did not claim completion (wrong reason)\n'; exit 1; }
@tr -d '\r' < "$(BUILD)/$(HARNESS_QA_MUT_NAME).serial" > "$(BUILD)/$(HARNESS_QA_MUT_NAME).normalized"
@n=$$(grep -c '^TPS-CORPUS-END ' "$(BUILD)/$(HARNESS_QA_MUT_NAME).normalized"); \
	if grep -q '^SHELL-EXIT$$' "$(BUILD)/$(HARNESS_QA_MUT_NAME).normalized" \
	   || test "$$n" -ge $(HARNESS_QA_EXECS); then \
		printf '!!! test-harness-quit-after-mutant FAIL: the no-wait mutant still completed %s/%s EXECs + SHELL-EXIT -- the gate is decoration\n' "$$n" "$(HARNESS_QA_EXECS)"; exit 1; \
	fi; \
	printf '>>> test-harness-quit-after-mutant: green (no-wait mutant truncated the batch at %s/%s EXECs, no SHELL-EXIT -- the completion wait is load-bearing)\n' "$$n" "$(HARNESS_QA_EXECS)"
```

## test-kernel-flair-repro

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size.mk', line 85):
Prerequisites: `build/.makefile.stamp | build`
```make
@set -eu; d=$(KERNEL_FLAIR_REPRO_DIR); for pass in 1 2; do \
	rm -rf $$d; mkdir -p $$d; \
	$(REPRO_NOCACHE) $(MAKE) --no-print-directory -o $(PALETTE_H) BUILD=$$d $$d/$(notdir $(KERNEL_FLAIRTENANTS_BIN)) >$(BUILD)/kernel-flair-repro-$$pass.log 2>&1 \
		|| { cat $(BUILD)/kernel-flair-repro-$$pass.log; exit 1; }; \
	cp -f $$d/$(notdir $(KERNEL_FLAIRTENANTS_BIN)) $(BUILD)/kernel-flair-repro-$$pass.bin; \
done
@cmp -s $(BUILD)/kernel-flair-repro-1.bin $(BUILD)/kernel-flair-repro-2.bin || { printf '!!! test-kernel-flair-repro FAIL: clean flagship rebuilds differ\n'; exit 1; }
@printf '>>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 %s)\n' "$$(sha256sum < $(BUILD)/kernel-flair-repro-1.bin | cut -d' ' -f1)"
```

## test-kernel-repro

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28644):
Prerequisites: `build/kernel.bin build/kernel_flairtenants.bin build/.makefile.stamp | build`
```make
@printf '>>> test-kernel-repro: baseline AND flagship across a clean rebuild (tgcu; Rule 11)\n'
@set -eu; d=$(KERNEL_REPRO_DIR); rm -rf $$d; mkdir -p $$d; \
for bin in $(KERNEL_REPRO_BINS); do cp -f $$bin $$d/$$(basename $$bin).repro.1; done; \
$(REPRO_NOCACHE) $(MAKE) --no-print-directory -o $(PALETTE_H) BUILD=$$d/build $(patsubst $(BUILD)/%,$$d/build/%,$(KERNEL_REPRO_BINS)) > $$d/clean-build.log 2>&1 \
	|| { cat $$d/clean-build.log; printf '!!! test-kernel-repro: the private clean build failed\n'; exit 1; }; \
for bin in $(KERNEL_REPRO_BINS); do \
	cp -f $$d/build/$$(basename $$bin) $$d/$$(basename $$bin).repro.2; \
	r=$$d/$$(basename $$bin); \
	if [ "$(KERNEL_REPRO_MUTATE_FLAGSHIP)" = 1 ] && [ "$$bin" = "$(KERNEL_FLAIRTENANTS_BIN)" ]; then printf X >> $$r.repro.2; fi; \
	a=$$(sha256sum < $$r.repro.1 | cut -d' ' -f1); b=$$(sha256sum < $$r.repro.2 | cut -d' ' -f1); \
	if [ "$$a" != "$$b" ]; then printf 'RED test-kernel-repro: %s is NOT reproducible\n' "$$bin"; exit 1; fi; \
	printf '>>> test-kernel-repro: green -- %s reproducible (sha256 %s)\n' "$$bin" "$$a"; \
done
```

## test-kernel-repro-flagship

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 140):
Prerequisites: `test-kernel-repro build/.makefile.stamp`
```make
@test -s $(KERNEL_REPRO_DIR)/$(notdir $(KERNEL_FLAIRTENANTS_BIN)).repro.1 -a -s $(KERNEL_REPRO_DIR)/$(notdir $(KERNEL_FLAIRTENANTS_BIN)).repro.2 || { echo 'RED flagship repro: flagship rebuild comparison missing'; exit 1; }
@cmp $(KERNEL_REPRO_DIR)/$(notdir $(KERNEL_FLAIRTENANTS_BIN)).repro.1 $(KERNEL_REPRO_DIR)/$(notdir $(KERNEL_FLAIRTENANTS_BIN)).repro.2
@echo 'GREEN test-kernel-repro-flagship: both flagship builds byte-identical'
```

## test-kernel-repro-flagship-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 149):
Prerequisites: `build/kernel.bin build/kernel_flairtenants.bin build/.makefile.stamp | build/khygiene-evidence`
```make
@if $(MAKE) --no-print-directory $(addprefix -o ,$(KERNEL_REPRO_BINS) $(MAKEFILE_STAMP)) test-kernel-repro KERNEL_REPRO_DIR=$(BUILD)/kernel-repro-mutant KERNEL_REPRO_MUTATE_FLAGSHIP=1 > $(BUILD)/khygiene-evidence/mutant-flagship-repro.log 2>&1; then echo 'FAIL flagship second-build byte mutant survived'; exit 1; fi
@grep 'RED test-kernel-repro: $(KERNEL_FLAIRTENANTS_BIN) is NOT reproducible' $(BUILD)/khygiene-evidence/mutant-flagship-repro.log
@echo 'GREEN test-kernel-repro-flagship-mutant: KERNEL_REPRO_MUTATE_FLAGSHIP correctly RED'
```

## test-kernel-repro-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28666):
Prerequisites: `build/kernel.bin build/.makefile.stamp | build`
```make
@printf ">>> test-kernel-repro-mutant: confirming the sha256 comparison bites on a byte change (Rule 6)\n"
@cp -f $(KERNEL_BIN) $(BUILD)/kernel.mut.a
@cp -f $(KERNEL_BIN) $(BUILD)/kernel.mut.b
@printf 'X' >> $(BUILD)/kernel.mut.b
@a=$$(sha256sum < $(BUILD)/kernel.mut.a | cut -d' ' -f1); \
 b=$$(sha256sum < $(BUILD)/kernel.mut.b | cut -d' ' -f1); \
 if [ "$$a" = "$$b" ]; then \
	printf '!!! test-kernel-repro-mutant FAIL: sha256 comparison did NOT detect a byte change -- the repro gate is decoration\n'; \
	exit 1; \
 fi; \
 printf '>>> test-kernel-repro-mutant: green (byte change detected -- the repro comparison bites)\n'
```

## test-lane-cert

Declared tier: HOST; #  recipe to execute (from 'harness/lane_cert.mk', line 7):
Prerequisites: `build/.makefile.stamp`
```make
+@set -eu; \
fail() { printf '!!! test-lane-cert FAIL: %s\n' "$$1"; exit 1; }; \
[ "$(filter test-lane-cert test-lane-cert-mutant,$(GATE_VECTOR))" = 'test-lane-cert test-lane-cert-mutant' ] || fail 'gate vector registration'; \
unset CERT_SCOPE LANE_CERT_JOBS REQUIRED_GATES GATES; \
cert_tool="$(abspath $(LANE_CERT_SCRIPT))"; \
check_tool="$(abspath tools/cert-check.sh)"; \
replay_tool="$(abspath tools/proof-replay.sh)"; \
for tool in "$$cert_tool" "$$check_tool" "$$replay_tool"; do \
    [ -f "$$tool" ] || fail "script missing: $${tool##*/}"; \
done; \
out="$(abspath $(BUILD))/lane-cert/unit"; \
if [ -d "$$out/repo/.git" ]; then git -C "$$out/repo" worktree prune; fi; \
rm -rf "$$out"; mkdir -p "$$out/repo"; \
git init -q --bare --initial-branch=main "$$out/origin.git"; \
cd "$$out/repo"; git init -q --initial-branch=main; \
git config user.name 'Lane fixture'; git config user.email 'lane@example.invalid'; \
git config commit.gpgSign false; git config core.hooksPath /dev/null; \
printf '/build/\n' > .gitignore; \
printf '%s\n' '.PHONY: test-one test-two' \
    'test-one:' \
    ' @case "$$$$(cat state)" in green) echo ">>> test-one: green (fixture)";; false-green) echo ">>> test-one: green (fixture)"; exit 1;; red-zero) echo ">>> test-one: red (fixture)";; quiet) echo no-marker;; *) echo ">>> test-one: red (fixture)"; exit 1;; esac' \
    'test-two:' \
    ' @echo ">>> test-two: red (intermediate)"' \
    ' @echo ">>> test-two: green (fixture)"' | sed 's/^ /\t/' > Makefile; \
printf 'red\n' > state; git add .; git commit -qm 'fixture red base'; \
base=$$(git rev-parse HEAD); \
git remote add origin "$$out/origin.git"; git push -q origin main; \
git switch -qc lane/proof; \
printf 'green\n' > state; git commit -qam 'fixture green lane'; \
head=$$(git rev-parse HEAD); cert=build/cert/lane-proof.cert; \
run_cert() { rc=0; REQUIRED_GATES='test-one test-two' GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; }; \
run_check() { REQUIRED_GATES='test-one test-two' sh "$$check_tool" "$$@"; }; \
refused() { [ "$$rc" -eq 2 ] && grep -F "$$1" "$$out/cert.log" >/dev/null && [ ! -f "$$cert" ]; }; \
printf 'dirty\n' >> state; run_cert; \
refused 'dirty tree' || fail 'dirty tree refusal'; git restore state; \
printf 'untracked\n' > untracked; run_cert; \
refused 'dirty tree' || fail 'untracked tree refusal'; rm -f untracked; \
printf 'staged\n' > staged; git add staged; run_cert; \
refused 'dirty tree' || fail 'staged tree refusal'; git reset -q; rm -f staged; \
git switch -q main; git commit -q --allow-empty -m 'advance main'; \
git push -q origin main; git switch -q lane/proof; run_cert; \
refused 'rebase required' || fail 'unrebased branch refusal'; \
git switch -q main; git reset -q --hard "$$base"; git push -q --force origin main; \
git switch -q lane/proof; \
printf 'red\n' > state; git commit -qam 'fixture mutant red gate'; \
mutant=$$(git rev-parse HEAD); run_cert; \
refused 'red gate: test-one' || fail 'red gate refusal (no cert)'; \
printf 'false-green\n' > state; git commit -qam 'false green with inherited make modes'; \
for flags_env in MAKEFLAGS GNUMAKEFLAGS; do \
    for flags in -i --ignore-errors -n --dry-run -q -t --touch -oMakefile -WMakefile --old-file=Makefile --what-if=Makefile --assume-new=Makefile ik; do \
        rc=0; env "$$flags_env=$$flags" GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
        refused "unsafe $$flags_env" || fail "inherited $$flags_env=$$flags false-green accepted"; \
    done; \
done; \
printf '>>> test-lane-cert: MAKEFLAGS false-green refused (both variables; unsafe modes)\n'; \
rc=0; REQUIRED_GATES='test-one test-two' MAKEFLAGS=-k GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
refused 'red gate: test-one' || fail 'MAKEFLAGS=-k false-green accepted'; \
git reset -q --hard "$$mutant"; \
for state in false-green red-zero quiet; do \
    printf '%s\n' "$$state" > state; git commit -qam "fixture $$state"; run_cert; \
    refused 'red gate: test-one' || fail "$$state accepted"; \
done; \
git reset -q --hard "$$head"; \
rc=0; env -u GATES REQUIRED_GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
refused 'no gate list' || fail 'missing gate list refusal'; \
run_cert; [ "$$rc" -eq 0 ] && [ -f "$$cert" ] || { cat "$$out/cert.log"; fail 'green certification'; }; \
for field in "base_sha: $$base" "head_sha: $$head" 'branch: lane/proof' 'gate_count: 2' \
    'gate test-one >>> test-one: green (fixture)' 'gate test-two >>> test-two: green (fixture)'; do \
    grep -qxF "$$field" "$$cert" || fail "certificate field: $$field"; \
done; \
for field in toolchain host utc; do grep -q "^$$field: ." "$$cert" || fail "certificate field: $$field"; done; \
for tool in gcc nasm qemu-system-i386 bochs; do grep -q "$$tool=" "$$cert" || fail "toolchain field: $$tool"; done; \
[ -z "$$(LC_ALL=C tr -d '\011\012\015\040-\176' < "$$cert")" ] || fail 'non-ASCII certificate'; \
! grep -F "$$HOME" "$$cert" >/dev/null || fail 'home path in certificate'; \
cp -f "$$cert" "$$out/good.cert"; \
review_failed=0; \
review_bad() { printf '!!! test-lane-cert FAIL: coverage %s\n' "$$1"; review_failed=1; }; \
rc=0; env -u REQUIRED_GATES GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
if refused 'coverage unchecked'; then printf '>>> test-lane-cert: coverage absent lane without REQUIRED_GATES refused\n'; else review_bad 'absent lane without REQUIRED_GATES accepted'; fi; \
rc=0; env -u REQUIRED_GATES sh "$$check_tool" "$$out/good.cert" lane/proof > "$$out/check.log" 2>&1 || rc=$$?; \
if [ "$$rc" -eq 2 ] && grep -F 'coverage unchecked' "$$out/check.log" >/dev/null; then printf '>>> test-lane-cert: coverage absent checker without REQUIRED_GATES refused\n'; else review_bad 'absent checker without REQUIRED_GATES accepted'; fi; \
run_cert; \
if [ "$$rc" -eq 0 ] && grep -qxF 'selector: absent' "$$cert"; then printf '>>> test-lane-cert: coverage REQUIRED_GATES subset certified (selector: absent)\n'; else review_bad 'fallback selector provenance missing'; fi; \
rc=0; env -u REQUIRED_GATES sh "$$check_tool" "$$out/good.cert" lane/proof --require 'test-one' > "$$out/check.log" 2>&1 || rc=$$?; \
if [ "$$rc" -eq 0 ] && grep -F 'cert-check: PASS' "$$out/check.log" >/dev/null; then printf '>>> test-lane-cert: coverage PM --require subset PASS\n'; else review_bad 'PM --require subset rejected'; fi; \
rc=0; REQUIRED_GATES='test-one test-three' GATES='test-one test-two' MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/cert.log" 2>&1 || rc=$$?; \
if refused 'missing gate: test-three'; then printf '>>> test-lane-cert: coverage lane missing REQUIRED_GATES refused\n'; else review_bad 'lane missing REQUIRED_GATES accepted'; fi; \
rc=0; REQUIRED_GATES='test-one test-three' sh "$$check_tool" "$$out/good.cert" lane/proof > "$$out/check.log" 2>&1 || rc=$$?; \
if [ "$$rc" -eq 2 ] && grep -F 'missing gate: test-three' "$$out/check.log" >/dev/null; then printf '>>> test-lane-cert: coverage checker missing REQUIRED_GATES refused\n'; else review_bad 'checker missing REQUIRED_GATES accepted'; fi; \
printf '\n.PHONY: gates-for\ngates-for: missing-selector-input\n\t@echo test-one\n' >> Makefile; \
git commit -qam 'selector has broken prerequisite'; \
broken=$$(git rev-parse HEAD); run_cert; \
if refused 'gate selection failed' && grep -F 'missing-selector-input' "$$out/cert.log" >/dev/null; then printf '>>> test-lane-cert: coverage broken lane selector refused (stderr retained)\n'; else review_bad 'broken lane selector accepted'; fi; \
sed "s/^head_sha: .*/head_sha: $$broken/" "$$out/good.cert" > "$$out/broken.cert"; \
rc=0; run_check "$$out/broken.cert" lane/proof > "$$out/check.log" 2>&1 || rc=$$?; \
if [ "$$rc" -eq 2 ] && grep -F 'gate selection failed' "$$out/check.log" >/dev/null && grep -F 'missing-selector-input' "$$out/check.log" >/dev/null; then printf '>>> test-lane-cert: coverage broken checker selector refused (stderr retained)\n'; else review_bad 'broken checker selector accepted'; fi; \
git reset -q --hard "$$head"; \
[ "$$review_failed" -eq 0 ] || exit 1; \
printf 'red\n' > state; git commit -qam 'revoke previous green'; run_cert; \
refused 'red gate: test-one' || fail 'old certificate not revoked on red'; \
git reset -q --hard "$$head"; \
git commit -q --allow-empty -m 'lane advanced after certificate'; \
rc=0; sh "$$check_tool" "$$out/good.cert" lane/proof > "$$out/check.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'head_sha mismatch' "$$out/check.log" >/dev/null || fail 'advanced lane head mismatch not rejected'; \
printf '>>> test-lane-cert: advanced lane head_sha mismatch refused\n'; \
git reset -q --hard "$$head"; \
rc=0; sh "$$check_tool" "$$out/good.cert" > "$$out/check.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'usage:' "$$out/check.log" >/dev/null || fail 'lane argument not required'; \
git switch -q main; run_check "$$out/good.cert" lane/proof > "$$out/check.log" 2>&1 || { cat "$$out/check.log"; fail 'fresh certificate check'; }; \
unrelated=$$(printf 'unrelated lane\n' | git commit-tree "$$(git rev-parse "$$head^{tree}")"); \
sed "s/^head_sha: .*/head_sha: $$unrelated/" "$$out/good.cert" > "$$out/unrelated.cert"; \
rc=0; run_check "$$out/unrelated.cert" "$$unrelated" > "$$out/check.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'not an ancestor' "$$out/check.log" >/dev/null || fail 'unrelated base accepted'; \
git commit -q --allow-empty -m 'stale certificate base'; \
if run_check "$$out/good.cert" lane/proof > "$$out/check.log" 2>&1; then fail 'stale base accepted'; fi; \
grep -F 'stale base_sha' "$$out/check.log" >/dev/null || fail 'stale base reason'; \
printf '>>> test-lane-cert: stale base_sha refused (current main advanced)\n'; \
git reset -q --hard "$$base"; \
printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
    ' @echo "test-one test-two"' | sed 's/^ /\t/' > gate-map.mk; \
printf '\ninclude gate-map.mk\n' >> Makefile; \
git add .; git commit -qm 'fixture gates-for'; \
mapbase=$$(git rev-parse HEAD); git push -q origin main; git switch -qc lane/mapped; \
printf 'green\n' > state; git commit -qam 'mapped lane green'; \
run_cert; [ "$$rc" -eq 0 ] || { cat "$$out/cert.log"; fail 'gates-for certification'; }; \
mapped=build/cert/lane-mapped.cert; \
grep -qxF "base_sha: $$mapbase" "$$mapped" || fail 'mapped base'; \
grep -qxF 'selector: gates-for' "$$mapped" || fail 'mapped selector provenance'; \
git switch -q main; run_check "$$mapped" lane/mapped > "$$out/check.log" 2>&1 || fail 'mapped superset'; \
sed '/^gate test-two /d; s/^gate_count: 2$$/gate_count: 1/' "$$mapped" > "$$out/missing.cert"; \
if run_check "$$out/missing.cert" lane/mapped > "$$out/check.log" 2>&1; then fail 'missing gate accepted'; fi; \
grep -F 'missing gate: test-two' "$$out/check.log" >/dev/null || fail 'missing gate reason'; \
git switch -q lane/mapped; \
rc=0; CERT_SCOPE=required LANE_CERT_JOBS=2 REQUIRED_GATES=test-one MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/scoped.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 0 ] && grep -qxF 'scope: required' "$$mapped" && grep -qxF 'gate_count: 1' "$$mapped" || { cat "$$out/scoped.log"; fail 'explicit batch subset'; }; \
rc=0; env -u CERT_SCOPE sh "$$check_tool" "$$mapped" lane/mapped > "$$out/check.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'scoped certificate needs' "$$out/check.log" >/dev/null || fail 'scoped certificate accepted as full coverage'; \
CERT_SCOPE=required REQUIRED_GATES=test-one sh "$$check_tool" "$$mapped" lane/mapped > "$$out/check.log" 2>&1 || fail 'explicit PM subset check'; \
rc=0; CERT_SCOPE=required REQUIRED_GATES='test-one test-two' sh "$$check_tool" "$$mapped" lane/mapped > "$$out/check.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'missing gate: test-two' "$$out/check.log" >/dev/null || fail 'scoped certificate missing required gate'; \
rc=0; CERT_SCOPE=required REQUIRED_GATES=test-three MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/scoped.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'required gate not selected' "$$out/scoped.log" >/dev/null && [ ! -f "$$mapped" ] || fail 'subset outside derived selection'; \
printf 'false-green\n' > state; git commit -qam 'batch false green'; \
rc=0; CERT_SCOPE=required LANE_CERT_JOBS=2 REQUIRED_GATES=test-one MAKE="$(MAKE)" sh "$$cert_tool" > "$$out/scoped.log" 2>&1 || rc=$$?; \
[ "$$rc" -eq 2 ] && grep -F 'red gate batch' "$$out/scoped.log" >/dev/null && [ ! -f "$$mapped" ] || fail 'batch false green accepted'; \
printf 'green\n' > state; git commit -qam 'batch green restored'; \
printf '>>> test-lane-cert: explicit subset, full-coverage refusal, missing gate and false-green batch checked\n'; \
git switch -q lane/mapped; \
printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
    ' @echo "test-two"' | sed 's/^ /\t/' > gate-map.mk; \
git commit -qam 'selector overrides GATES'; run_cert; \
[ "$$rc" -eq 0 ] && grep -qxF 'gate_count: 1' "$$mapped" || fail 'gates-for precedence'; \
printf '%s\n' '.PHONY: gates-for' 'gates-for:' \
    ' @echo selector-broken; exit 1' | sed 's/^ /\t/' > gate-map.mk; \
git commit -qam 'broken selector'; run_cert; \
[ "$$rc" -eq 2 ] && [ ! -f "$$mapped" ] || fail 'broken selector accepted'; \
git switch -q main; mkdir -p build/proof; \
printf 'RED|%s|make -s test-one|>>> test-one: red (fixture)\nGREEN|%s|make -s test-one|>>> test-one: green (fixture)\nMUTANT|%s|make -s test-one|>>> test-one: red (fixture)\n' \
    "$$base" "$$head" "$$mutant" > build/proof/manifest; \
sh "$$replay_tool" > "$$out/replay.log" 2>&1 || { cat "$$out/replay.log"; fail 'proof replay'; }; \
[ "$$(grep -c '^PASS ' "$$out/replay.log")" -eq 3 ] || fail 'proof replay PASS records'; \
sh "$$replay_tool" > "$$out/replay.log" 2>&1 || fail 'proof worktree reuse'; \
printf 'GREEN|%s|cat; make -s test-one|>>> test-one: green (fixture)\nGREEN|%s|make -s test-one|>>> test-one: green (fixture)\n' \
    "$$head" "$$head" > build/proof/manifest; \
sh "$$replay_tool" > "$$out/replay.log" 2>&1 || fail 'proof stdin isolation'; \
[ "$$(grep -c '^PASS ' "$$out/replay.log")" -eq 2 ] || fail 'proof stdin consumed records'; \
printf 'GREEN|%s|make -s test-one|wrong EXPECT\n' "$$head" > build/proof/manifest; \
if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'wrong EXPECT accepted'; fi; \
grep -q '^FAIL ' "$$out/replay.log" || fail 'proof replay FAIL record'; \
printf 'GREEN|%s|make -s test-one|>>> test-one: red (fixture)\n' "$$base" > build/proof/manifest; \
if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'GREEN with red exit accepted'; fi; \
printf 'RED|%s|make -s test-one|>>> test-one: green (fixture)\n' "$$head" > build/proof/manifest; \
if sh "$$replay_tool" > "$$out/replay.log" 2>&1; then fail 'RED with green exit accepted'; fi; \
printf '>>> test-lane-cert: green (dirty, stale base, red, fields, selector, proof replay)\n'
```

## test-lane-cert-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/lane_cert.mk', line 183):
Prerequisites: `build/.makefile.stamp`
```make
+@set -eu; out="$(abspath $(BUILD))/lane-cert/mutants"; mkdir -p "$$out"; \
cp -f tools/cert-summary.sh "$$out/cert-summary.sh"; \
sed 's/^if \[ -n "$$dirty" \]; then$$/if false; then # mutant: skip dirty check/' tools/lane-cert.sh > "$$out/dirty.sh"; \
sed 's/^    if \[ "$$gate_ok" != yes \]; then$$/    if false; then # mutant: certify red gate/' tools/lane-cert.sh > "$$out/red.sh"; \
sed 's/^check_make_flags .*$$/: # mutant: ignore MAKEFLAGS check/' tools/lane-cert.sh > "$$out/flags.sh"; \
sed 's/^    require_coverage$$/    required=$$gates # mutant: coverage fails open/' tools/lane-cert.sh > "$$out/coverage.sh"; \
for mutant in dirty red flags coverage; do \
    ! cmp -s tools/lane-cert.sh "$$out/$$mutant.sh"; \
    rc=0; $(MAKE) --no-print-directory test-lane-cert \
        "LANE_CERT_SCRIPT=$$out/$$mutant.sh" "BUILD=$$out/$$mutant-build" > "$$out/$$mutant.log" 2>&1 || rc=$$?; \
    case $$mutant in dirty) reason='dirty tree refusal';; red) reason='red gate refusal (no cert)';; flags) reason='inherited MAKEFLAGS=-i false-green accepted';; coverage) reason='coverage absent lane without REQUIRED_GATES accepted';; esac; \
    [ "$$rc" -ne 0 ] && grep -qF "!!! test-lane-cert FAIL: $$reason" "$$out/$$mutant.log" || { cat "$$out/$$mutant.log"; exit 1; }; \
    printf '>>> test-lane-cert-mutant: %s RED (%s)\n' "$$mutant" "$$reason"; \
done; \
printf '>>> test-lane-cert-mutant: green (skip-dirty, certify-red, ignore MAKEFLAGS check, coverage fails open killed)\n'
```

## test-lane-cert-summaries

Declared tier: HOST; #  recipe to execute (from 'harness/lane_cert.mk', line 203):
Prerequisites: `build/.makefile.stamp`
```make
@sh harness/test_cert_summaries.sh $(abspath tools/lane-cert.sh) $(abspath tools/cert-check.sh) $(abspath $(BUILD))/lane-cert/summaries
```

## test-lane-cert-summaries-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/lane_cert.mk', line 207):
Prerequisites: `test-lane-cert-summaries build/.makefile.stamp`
```make
@sh harness/test_cert_summaries_mutant.sh $(abspath $(BUILD))/lane-cert/summaries-mutant
```

## test-makefile-vars

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 14007):
Prerequisites: `build/.makefile.stamp | build`
```make
@printf '>>> test-makefile-vars: no rule reads a variable before its definition (tdnl.81)\n'
@$(MAKE) --no-print-directory --warn-undefined-variables -pn help >/dev/null 2>$(MAKEFILE_VARS_LOG) \
	|| { printf '!!! test-makefile-vars FAIL: the Makefile did not parse\n'; cat $(MAKEFILE_VARS_LOG); exit 1; }
@bad=$$(sed -n "s/^\(.*\): warning: undefined variable '\(.*\)'$$/\2 \1/p" $(MAKEFILE_VARS_LOG) \
	| grep -v -E '^(SKIP_BOCHS|SKIP_FPC|SKIP_PUBS|SCRIPT|RECORD_IMAGE_|RECORD_DATA_) ' || true); \
if [ -n "$$bad" ]; then printf '!!! test-makefile-vars FAIL: read before definition (variable, site):\n%s\n' "$$bad"; exit 1; fi
@# Leg 2 (2026-10-16, found by the b52a7ce certificate at emu gate 147): a
@# blank line right after a backslash-continued recipe line ends the shell
@# command early, so a multi-line for/if loses its done/fi and the gate dies
@# with "Syntax error: end of file unexpected" instead of grading anything.
@split=$$(awk 'prev ~ /\\$$/ && $$0 ~ /^[ \t]*$$/ { printf "%d ", NR } { prev = $$0 }' $(firstword $(MAKEFILE_LIST))); \
if [ -n "$$split" ]; then printf '!!! test-makefile-vars FAIL: blank line after a backslash-continued line splits a recipe command (Makefile lines %s)\n' "$$split"; exit 1; fi
@printf '>>> test-makefile-vars: green\n'
```

## test-merge-train

Declared tier: HOST; #  recipe to execute (from 'harness/merge_samir.mk', line 47):
Prerequisites: `build/.makefile.stamp`
```make
+@sh tools/test-merge-train.sh
```

## test-ovl-repro

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 276):
Prerequisites: `build/flairovl_kernel_flairtenants.ovl build/INITDOS.SYS build/flair_tenants.img build/.makefile.stamp | build/ovl-gates`
```make
@set -eu; \
 rm -rf $(OVL_REPRO_B); LC_ALL=C.utf8 LANG=C.utf8 $(REPRO_NOCACHE) $(MAKE) --no-print-directory -o $(PALETTE_H) BUILD=$(OVL_REPRO_B) $(OVL_REPRO_B)/flairovl_kernel_flairtenants.ovl $(OVL_REPRO_B)/INITDOS.SYS $(OVL_REPRO_B)/flair_tenants.img > $(OVL_GATE_DIR)/repro-b.log 2>&1 || { tail -20 $(OVL_GATE_DIR)/repro-b.log; exit 1; }; \
 for f in flairovl_kernel_flairtenants.ovl INITDOS.SYS flair_tenants.img; do \
	a=$$(sha256sum < $(BUILD)/$$f | cut -d' ' -f1); b=$$(sha256sum < $(OVL_REPRO_B)/$$f | cut -d' ' -f1); \
	[ "$(OVL_REPRO_MUTATE)" != 1 ] || [ "$$f" != flairovl_kernel_flairtenants.ovl ] || b=$$(printf X | cat $(OVL_REPRO_B)/$$f - | sha256sum | cut -d' ' -f1); \
	[ "$$a" = "$$b" ] || { printf 'RED test-ovl-repro: %s differs across directories/locales\n' "$$f"; exit 1; }; \
	printf '>>> test-ovl-repro: green -- %s reproducible (sha256 %s)\n' "$$f" "$$a"; \
 done
```

## test-ovl-repro-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/overlay.mk', line 285):
Prerequisites: `build/flairovl_kernel_flairtenants.ovl build/INITDOS.SYS build/flair_tenants.img build/.makefile.stamp | build/ovl-gates`
```make
@if $(MAKE) --no-print-directory $(addprefix -o ,$(OVL_FLAG).ovl $(INITDOS_SYS_BIN) $(FLAIRTENANTS_IMG) $(MAKEFILE_STAMP)) test-ovl-repro OVL_REPRO_B=$(BUILD)/ovl-repro-mutant-b OVL_REPRO_MUTATE=1 > $(OVL_GATE_DIR)/repro-mut.log 2>&1; then printf '!!! test-ovl-repro-mutant FAIL: a changed FLAIR.OVL passed\n'; exit 1; fi
@grep 'RED test-ovl-repro: flairovl_kernel_flairtenants.ovl differs' $(OVL_GATE_DIR)/repro-mut.log
@printf '>>> test-ovl-repro-mutant: green (a one-byte FLAIR.OVL change is detected)\n'
```

## test-par

Declared tier: outside active vector; #  recipe to execute (from 'harness/merge_samir.mk', line 32):
Prerequisites: `build/.makefile.stamp`
```make
@gates="$(sort $(TEST_UNIT_GATES) $(TEST_EMU_GATES))"; \
printf '>>> test-par: %s gates (unit+emu), one parallel make\n' "$$(echo $$gates | wc -w)"; \
$(MAKE) -k $$gates && printf '>>> test-par: ALL GREEN\n'
```

## test-pubs

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 448):
Prerequisites: `test-pubs-build test-pubs-commands test-pubs-tps test-pubs-lint test-pubs-examples test-pubs-screens test-pubs-samir-programs test-pubs-iword test-pubs-inconsistency test-pubs-label test-pubs-turbo test-pubs-flair build/.makefile.stamp`
```make
@printf '>>> test-pubs: green (command coverage, TPS message inventory, InitechBase programs, InitechWord surface and replay, example replay, source lints, build cap, house register)\n'
```

## test-pubs-aux

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 166):
Prerequisites: `build/.makefile.stamp`
```make
+@MAKE="$(MAKE)" sh harness/pubs/aux_check.sh $(PUBS_BUILD)
```

## test-pubs-aux-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 170):
Prerequisites: `build/.makefile.stamp`
```make
@mkdir -p $(PUBS_BUILD)
+@if $(MAKE) --no-print-directory -j3 test-pubs-aux PUBS_MUT_KEEP_AUX=1 > $(PUBS_BUILD)/aux-mutant.log 2>&1; then \
	cat $(PUBS_BUILD)/aux-mutant.log; printf '!!! test-pubs-aux-mutant FAIL: stale-aux mutant survived\n'; exit 1; fi
@grep -F 'Undefined control sequence' $(PUBS_BUILD)/aux-mutant.log && \
	grep -F '!!! test-pubs-aux RED: newer dependency left an incompatible main.aux' $(PUBS_BUILD)/aux-mutant.log || { \
	cat $(PUBS_BUILD)/aux-mutant.log; printf '!!! test-pubs-aux-mutant FAIL: wrong failure\n'; exit 1; }
@rm -rf $(PUBS_BUILD)/initechword-install
+@$(MAKE) --no-print-directory -j3 pubs-initechword-install
@printf '>>> test-pubs-aux-mutant: green (cleanup-disabled branch rejected by the stale-aux oracle)\n'
```

## test-pubs-build

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 186):
Prerequisites: `build/pubs/gen/dos_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex test-pubs-entry-index build/pubs/gen/dos_messages.tex build/pubs/gen/tps_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/entries-initechbase-quick-reference-cmd.tex build/pubs/gen/entries-initechbase-quick-reference-fn.tex build/pubs/gen/entries-initechbase-quick-reference-set.tex build/pubs/gen/entries-initechbase-volume-one-cmd.tex build/pubs/gen/entries-initechbase-volume-one-fn.tex pubs-turbo-gen pubs-flair-gen build/.makefile.stamp`
```make
@printf '>>> test-pubs-build: every manual typesets clean (initech-ipu5.5; D-11 LuaLaTeX)\n'
@for t in $(LUALATEX) $(LATEXMK); do command -v $$t >/dev/null 2>&1 || { \
	printf '!!! test-pubs-build FAIL: %s not found (TeX Live is required to typeset the manuals). A skipped oracle is worse than a red one (Law 2). Set SKIP_PUBS=1 to explicitly (and loudly) opt out.\n' "$$t"; exit 1; }; done
@bad=$$(LC_ALL=C grep -l -P '[^\x00-\x7F]' $(PUBS_SOURCES) harness/console_text.c tools/pubs_msgtable.c harness/pubs/tps_inventory.c \
	harness/pubs/pubs_gate.c harness/pubs/srclockstep.h harness/pubs/slip_places.sh harness/pubs/slip_mutants.sh spec/pubs_inconsistencies.json || true); \
if [ -n "$$bad" ]; then printf '!!! test-pubs-build FAIL: non-ASCII bytes in (Rule 12): %s\n' "$$bad"; exit 1; fi
$(call pubs-build-into,$(PUBS_BUILD)/gate)
@set -e; for log in $(PUBS_BUILD)/gate/*/*.log; do \
	doc=$${log%.log}; name=$${doc#$(PUBS_BUILD)/gate/}; \
	over=$$(grep -c '^Overfull \\[hv]box' $$log || true); \
	sh harness/pubs/check_log.sh "$$log" "$(PUBS_OVERFULL_MAX)"; \
	if grep -E -q 'There were undefined references|Reference .* undefined|Label\(s\) may have changed' $$log; then \
		printf '!!! test-pubs-build FAIL: %s: undefined or unsettled references\n' "$$name"; exit 1; fi; \
	if grep -q 'Missing character' $$log; then grep 'Missing character' $$log | head -5; \
		printf '!!! test-pubs-build FAIL: %s: glyphs missing from the fonts\n' "$$name"; exit 1; fi; \
	if grep -q 'No index file' $$log; then \
		printf '!!! test-pubs-build FAIL: %s: the index was not generated\n' "$$name"; exit 1; fi; \
	pages=$$(pdfinfo $$doc.pdf | sed -n 's/^Pages: *//p'); \
	drafts=$$(sed -n 's/^initech-manual: \([0-9]*\) DRAFT markers$$/\1/p' $$log); \
	case $$name in */main) \
		[ $$((pages % 4)) -eq 0 ] || { printf '!!! test-pubs-build FAIL: %s: %s pages is not a multiple of 4 (printer signatures)\n' "$$name" "$$pages"; exit 1; } ;; esac; \
	printf '>>> test-pubs-build: %s.pdf %s pages, %s overfull, %s DRAFT markers\n' "$$name" "$$pages" "$$over" "$${drafts:-0}"; \
done
@printf '>>> test-pubs-build: green\n'
```

## test-pubs-commands

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 462):
Prerequisites: `pubs-inventory test-pubs-build build/.makefile.stamp`
```make
@set -e; for idx in $(PUBS_BUILD)/gate/*/main.cmds $(PUBS_BUILD)/gate/*/main.mentions; do \
	case $$idx in */initechdos-*/*|*/$(PUBS_TPS_BOOK)/*|*/initechbase-*/*|*/turbo-initech-*/*|*/initechword-*/main.cmds|*/flair-*/main.cmds) continue;; esac; \
	[ -z "$$(grep -v '^#' "$$idx" || true)" ] || { printf '!!! test-pubs RED COMMAND-SURFACE: %s needs its own shipped inventory before it can contain entries\n' "$$idx"; exit 1; }; done
@$(PUBS_GATE) commands $(PUBS_CHECK)/shipped $(PUBS_ALLOW) $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_DOS_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
@sh harness/pubs/check_allowlist.sh $(PUBS_GATE) $(PUBS_ALLOW) $(PUBS_CHECK)
```

## test-pubs-entry-index

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 121):
Prerequisites: `build/.makefile.stamp`
```make
+@MAKE="$(MAKE)" sh harness/pubs/entry_index_check.sh
```

## test-pubs-examples

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 515):
Prerequisites: `test-pubs-build build/pubs_samir_replay build/pubs_gate test-pubs-samir-disk build/.makefile.stamp`
```make
@sh harness/pubs/replay.sh $(PUBS_REPLAY) $(PUBS_GATE) $(PUBS_BUILD)/gate $(PUBS_CHECK) $(PUBS_SAMIR_DISK)
```

## test-pubs-families

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 551):
Prerequisites: `build/pubs_cover_check build/.makefile.stamp`
```make
@command -v pdftoppm >/dev/null 2>&1 || { printf '!!! test-pubs-families FAIL: pdftoppm (poppler-utils) not found; the identity leg renders the covers with it\n'; exit 1; }
@COVER_CHECK=$(PUBS_COVER_CHECK) sh harness/pubs/families.sh
```

## test-pubs-flair

Declared tier: outside active vector; #  recipe to execute (from 'pubs/flair.mk', line 124):
Prerequisites: `pubs-flair-inventory test-pubs-build build/pubs_gate build/.makefile.stamp`
```make
@# KIND operation is proved by check_flair.sh (a marker the source prints,
@# replayed by a capture), not by the inventory: the gate gets the rest.
@set -e; for b in $(PUBS_FLAIR_BOOKS); do \
	grep -v '|operation|' $(PUBS_BUILD)/gate/$$b/main.cmds > $(PUBS_CHECK)/flair-$$b.cmds; done
@$(PUBS_GATE) commands $(PUBS_CHECK)/flair.shipped $(PUBS_FLAIR_ALLOW) \
	$(foreach b,$(PUBS_FLAIR_BOOKS),$(PUBS_CHECK)/flair-$(b).cmds)
@sh harness/pubs/check_flair.sh $(PUBS_BUILD)/gate $(PUBS_FLAIR_BOOKS)
@[ -z "$$(cat $(foreach b,$(PUBS_FLAIR_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions) | grep -v '^#' || true)" ] || { \
	printf '!!! test-pubs RED COMMAND-SURFACE: a FLAIR book names an InitechDOS command with \\cmd (use \\file; the DOS books document commands)\n'; exit 1; }
@printf '>>> test-pubs-flair: green (FLAIR books == shipped menus, files, alerts; figures and operations proved)\n'
```

## test-pubs-flair-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/flair.mk', line 137):
Prerequisites: `test-pubs-flair build/pubs_flair_grey build/qemu_harness build/flair_tenants_pubs.img build/flair_tenants_pubs.img build/flair_data.img build/record_locked_trash.img build/pubs_flair_grey build/console_text pubs/screens/flair_capture.sh spec/assets/clut.json spec/assets/color_canon.json build/.makefile.stamp`
```make
@CC="$(CC)" MENU_INC="$(MENU_INC)" FLAIR_KMAIN_I=$(PUBS_FLAIR_KMAIN) FLAIR_FIXTURES="$(FLAIR_FIXTURES)" \
	HARNESS=$(HARNESS_BIN) FLAIR_BOOT=$(PUBS_FLAIR_BOOT) FLAIR_DATA=$(FLAIR_DATA_IMG) \
	CONSOLE_TEXT=$(CONSOLE_TEXT_BIN) VGABIOS=$(PUBS_VGABIOS) \
	sh harness/pubs/flair_mutants.sh $(PUBS_GATE) $(PUBS_FLAIR_GREY) $(PUBS_CHECK) $(PUBS_BUILD)/gate $(PUBS_FLAIR_ALLOW) $(PUBS_FLAIR_BOOKS)
```

## test-pubs-flair-screens

Declared tier: outside active vector; #  recipe to execute (from 'pubs/flair.mk', line 155):
Prerequisites: `build/qemu_harness build/flair_tenants_pubs.img build/flair_tenants_pubs.img build/flair_data.img build/record_locked_trash.img build/pubs_flair_grey build/console_text pubs/screens/flair_capture.sh spec/assets/clut.json spec/assets/color_canon.json build/pubs_gate test-pubs-iword-flair-screens build/.makefile.stamp`
```make
@set -e; n=0; for b in $(PUBS_FLAIR_BOOKS); do \
	d=$(PUBS_DIR)/manuals/$$b/screens; [ -f $$d/FLAIR ] || continue; \
	rm -rf $(PUBS_BUILD)/flair-screens-check/$$b; \
	sh $(PUBS_DIR)/screens/flair_capture.sh $(HARNESS_BIN) $(PUBS_FLAIR_BOOT) $(FLAIR_DATA_IMG) $(PUBS_FLAIR_GREY) \
		spec/assets/clut.json spec/assets/color_canon.json $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) $$d/FLAIR $(PUBS_BUILD)/flair-screens-check/$$b $(PUBS_FLAIR_STEP) $(PUBS_FLAIR_LOCKED); \
	for t in $(PUBS_BUILD)/flair-screens-check/$$b/*.grey $(PUBS_BUILD)/flair-screens-check/$$b/*.console; do \
		[ -f "$$t" ] || continue; \
		$(PUBS_GATE) same SCREEN-DRIFT $$d/$$(basename $$t) $$t || exit 1; n=$$((n + 1)); done; \
	for c in $$d/*.grey $$d/*.console; do [ -f "$$c" ] || continue; [ -f $(PUBS_BUILD)/flair-screens-check/$$b/$$(basename $$c) ] || { \
		printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s has no FLAIR row\n' "$$c"; exit 1; }; done; done; \
printf '>>> test-pubs-flair-screens: green (%s FLAIR screens in the manuals == live desktop output)\n' "$$n"
```

## test-pubs-honesty

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 214):
Prerequisites: `build/.makefile.stamp`
```make
@if env -u SKIP_PUBS $(MAKE) --no-print-directory test-pubs-build LUALATEX=/nonexistent/lualatex > $(BUILD)/pubs-honesty-missing.log 2>&1; then \
	cat $(BUILD)/pubs-honesty-missing.log; printf '!!! test-pubs-honesty FAIL: test-pubs-build accepted a missing lualatex\n'; exit 1; fi
@grep -F 'test-pubs-build FAIL' $(BUILD)/pubs-honesty-missing.log | grep -qF 'SKIP_PUBS=1' || { \
	cat $(BUILD)/pubs-honesty-missing.log; printf '!!! test-pubs-honesty FAIL: the missing-tool failure does not name SKIP_PUBS=1\n'; exit 1; }
@$(MAKE) --no-print-directory test-pubs-build SKIP_PUBS=1 LUALATEX=/nonexistent/lualatex > $(BUILD)/pubs-honesty-skip.log 2>&1
@grep -qF 'test-pubs-build SKIPPED (SKIP_PUBS=1 opt-out)' $(BUILD)/pubs-honesty-skip.log || { \
	cat $(BUILD)/pubs-honesty-skip.log; printf '!!! test-pubs-honesty FAIL: the SKIP_PUBS=1 opt-out is not shouting\n'; exit 1; }
@printf '>>> test-pubs-honesty: green (missing lualatex RED and names SKIP_PUBS=1; SKIP_PUBS=1 explicitly SKIPPED)\n'
```

## test-pubs-inconsistency

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 450):
Prerequisites: `test-pubs-build build/pubs_gate build/.makefile.stamp`
```make
@$(PUBS_GATE) inconsistency $(PUBS_SLIP_REGISTER) $(PUBS_DIR)/class $(PUBS_DIR)/manuals $(PUBS_BUILD)/gate $(PUBS_SLIP_BOOKS)
@sh harness/pubs/slip_places.sh $(PUBS_BUILD)
```

## test-pubs-label

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 418):
Prerequisites: `build/.makefile.stamp`
```make
@sh harness/pubs/label_check.sh $(PUBS_LABEL_ENTRY) $(PUBS_LABEL_SCREEN).keys $(PUBS_LABEL_SCREEN).txt
```

## test-pubs-label-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 420):
Prerequisites: `test-pubs-label build/.makefile.stamp`
```make
@sh harness/pubs/label_mutants.sh harness/pubs/label_check.sh $(PUBS_LABEL_ENTRY) $(PUBS_LABEL_SCREEN).keys $(PUBS_LABEL_SCREEN).txt $(PUBS_CHECK)/label-mutants
```

## test-pubs-lint

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 511):
Prerequisites: `build/pubs_gate build/.makefile.stamp`
```make
@$(PUBS_GATE) lint pubs/denylist.txt pubs
@sha256sum -c pubs/fonts.lock > $(BUILD)/pubs-fonts.log
@printf '>>> test-pubs-fonts: green (all font files match fonts.lock)\n'
```

## test-pubs-lockstep-src

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 403):
Prerequisites: `build/pubs_gate build/pubs/correctness/samir.shipped build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/entries-initechbase-quick-reference-cmd.tex build/pubs/gen/entries-initechbase-quick-reference-fn.tex build/pubs/gen/entries-initechbase-quick-reference-set.tex build/pubs/gen/entries-initechbase-volume-one-cmd.tex build/pubs/gen/entries-initechbase-volume-one-fn.tex build/.makefile.stamp`
```make
@$(PUBS_GATE) srclockstep $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) \
	$(PUBS_DIR)/manuals/initechbase-volume-one $(PUBS_DIR)/manuals/initechbase-quick-reference \
	$(PUBS_DIR)/manuals/initechbase-volume-two $(PUBS_GEN)
```

## test-pubs-lockstep-src-mutant

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 407):
Prerequisites: `test-pubs-lockstep-src build/.makefile.stamp`
```make
@sh harness/pubs/srclockstep_mutants.sh $(PUBS_GATE) $(PUBS_CHECK)/samir.shipped $(PUBS_SAMIR_ALLOW) \
	$(PUBS_DIR)/manuals $(PUBS_BUILD)/lockstep-src-mutants $(PUBS_GEN)
```

## test-pubs-mutant

Declared tier: HOST; #  recipe to execute (from 'pubs/pubs.mk', line 523):
Prerequisites: `test-pubs test-pubs-label-mutant build/pubs_gate build/pubs_samir_inventory build/pubs_tps_inventory test-pubs-turbo-mutant test-pubs-flair-mutant build/.makefile.stamp`
```make
@sh harness/pubs/mutants.sh $(PUBS_GATE) $(PUBS_REPLAY) $(PUBS_CHECK) $(PUBS_BUILD)/gate $(PUBS_ALLOW) $(PUBS_SAMIR_DISK) $(PUBS_SAMIR_INV) \
	$(PUBS_SAMIR_CAND) $(PUBS_SAMIR_CAND_SRC)
@sh harness/pubs/tps_mutants.sh $(PUBS_TPS_INV_BIN) $(PUBS_GATE) $(PUBS_CHECK) $(PUBS_BUILD)/gate/$(PUBS_TPS_BOOK) $(PUBS_GEN)
@sh harness/pubs/slip_mutants.sh $(PUBS_GATE) $(PUBS_SLIP_REGISTER) $(PUBS_DIR)/class $(PUBS_DIR)/manuals $(PUBS_BUILD)/gate $(PUBS_BUILD) $(PUBS_SLIP_BOOKS)
@if sh harness/pubs/slip_places.sh $(PUBS_BUILD) mutant > $(PUBS_BUILD)/slip-places-mutant.log 2>&1; then \
	cat $(PUBS_BUILD)/slip-places-mutant.log; printf '!!! test-pubs-mutant FAIL: the class place proof stayed green with the running-head place switched off\n'; exit 1; fi
@grep -q 'RED SLIP-PLACES: the class reports' $(PUBS_BUILD)/slip-places-mutant.log || { cat $(PUBS_BUILD)/slip-places-mutant.log; \
	printf '!!! test-pubs-mutant FAIL: the class place mutant went RED for another reason\n'; exit 1; }
@printf '>>> test-pubs-mutant: VO-30 class place proof RED as required (running-head place switched off: %s)\n' \
	"$$(grep '^+VO30' $(PUBS_BUILD)/slip-places-mutant.log | head -1)"
@$(MAKE) --no-print-directory test-pubs-iword-mutant
```

## test-pubs-repro

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 230):
Prerequisites: `build/pubs/gen/dos_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/dos_messages.tex build/pubs/gen/tps_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/entries-initechbase-quick-reference-cmd.tex build/pubs/gen/entries-initechbase-quick-reference-fn.tex build/pubs/gen/entries-initechbase-quick-reference-set.tex build/pubs/gen/entries-initechbase-volume-one-cmd.tex build/pubs/gen/entries-initechbase-volume-one-fn.tex pubs-turbo-gen pubs-flair-gen build/.makefile.stamp`
```make
$(call pubs-build-into,$(PUBS_BUILD)/repro-a,,$(PUBS_REPRO_MANUAL))
$(call pubs-build-into,$(PUBS_BUILD)/repro-b,,$(PUBS_REPRO_MANUAL))
@set -e; n=0; for a in $(PUBS_BUILD)/repro-a/*/*.pdf; do \
	b=$(PUBS_BUILD)/repro-b/$${a#$(PUBS_BUILD)/repro-a/}; \
	cmp "$$a" "$$b" || { printf '!!! test-pubs-repro FAIL: %s differs between two builds\n' "$${a#$(PUBS_BUILD)/repro-a/}"; exit 1; }; \
	printf '>>> test-pubs-repro: cmp %s == %s (%s bytes, sha256 %s)\n' "$$a" "$$b" "$$(wc -c < $$a)" "$$(sha256sum $$a | cut -c1-16)"; \
	n=$$((n + 1)); done; \
[ $$n -gt 0 ] || { printf '!!! test-pubs-repro FAIL: no PDFs built\n'; exit 1; }; \
printf '>>> test-pubs-repro: green (%s PDFs byte-identical across two clean builds)\n' "$$n"
```

## test-pubs-repro-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 243):
Prerequisites: `build/pubs/gen/dos_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/dos_messages.tex build/pubs/gen/tps_messages.tex build/pubs/gen/samir_limits.tex build/pubs/gen/samir_coercion.tex build/pubs/gen/samir_formats.tex build/pubs/gen/samir_messages.tex build/pubs/gen/entries-initechbase-quick-reference-cmd.tex build/pubs/gen/entries-initechbase-quick-reference-fn.tex build/pubs/gen/entries-initechbase-quick-reference-set.tex build/pubs/gen/entries-initechbase-volume-one-cmd.tex build/pubs/gen/entries-initechbase-volume-one-fn.tex pubs-turbo-gen pubs-flair-gen build/.makefile.stamp`
```make
$(call pubs-build-into,$(PUBS_BUILD)/mutant-a,mutant)
$(call pubs-build-into,$(PUBS_BUILD)/mutant-b,mutant)
@if cmp -s $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf; then \
	printf '!!! test-pubs-repro-mutant FAIL: two timestamped builds are identical -- test-pubs-repro is decoration\n'; exit 1; fi
@printf '>>> test-pubs-repro-mutant: green (timestamp-injected builds differ: '; \
	cmp $(PUBS_BUILD)/mutant-a/initechdos-users-guide/main.pdf $(PUBS_BUILD)/mutant-b/initechdos-users-guide/main.pdf | tr -d '\n'; printf ')\n'
```

## test-pubs-repro-single

Declared tier: outside active vector; #  recipe to execute (from 'pubs/pubs.mk', line 227):
Prerequisites: `test-pubs-repro`
```make
@printf '>>> test-pubs-repro-single: green (initechword-install; two clean builds compared byte for byte)\n'
```

## test-pubs-screens

Declared tier: EMU; #  recipe to execute (from 'pubs/pubs.mk', line 294):
Prerequisites: `build/qemu_harness build/tracer_boot.img build/console_text pubs/screens/capture.sh pubs/screens/keys.sh build/pubs/utils/.staged build/flair_tenants.img build/flair_data.img build/pubs/samir-capture/.staged test-pubs-build build/pubs_gate test-pubs-flair-screens test-pubs-flair-screens build/.makefile.stamp`
```make
@set -e; n=0; for m in $(PUBS_MANUALS); do \
	d=$(PUBS_DIR)/manuals/$$m/screens; [ -f $$d/SCREENS ] || continue; \
	$(PUBS_FLAIR_ENV) sh $(PUBS_DIR)/screens/capture.sh $(HARNESS_BIN) $(TRACER_IMG) $(CONSOLE_TEXT_BIN) $(PUBS_VGABIOS) \
		$$d/SCREENS $(PUBS_DIR)/screens/disk $(PUBS_BUILD)/screens-check/$$m $(PUBS_UTILS_DIR) $(PUBS_SAMIR_CAPTURE); \
	for t in $(PUBS_BUILD)/screens-check/$$m/*.txt; do \
		$(PUBS_GATE) same SCREEN-DRIFT $$d/$$(basename $$t) $$t || { diff -u $$d/$$(basename $$t) $$t || true; exit 1; }; \
		n=$$((n + 1)); done; \
	for c in $$d/*.txt; do [ -f $(PUBS_BUILD)/screens-check/$$m/$$(basename $$c) ] || { \
		printf '!!! test-pubs-screens FAIL: %s has no SCREENS entry\n' "$$c"; exit 1; }; done; done; \
printf '>>> test-pubs-screens: green (%s screens in the manuals == live OS output)\n' "$$n"
@sh harness/pubs/check_screens.sh $(PUBS_BUILD)/gate
```

## test-pubs-turbo

Declared tier: outside active vector; #  recipe to execute (from 'pubs/turbo.mk', line 107):
Prerequisites: `test-pubs-build pubs-inventory build/pubs/tps/shipped build/pubs_railroad build/qemu_harness build/tracer_secondary.img build/initechc build/seed_rt.o build/seed_rt_dos.o build/seed_fileio_rt.o build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/compiler-os.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/seed-repro.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-gen-fpc.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-lex-build.txt build/.makefile.stamp`
```make
@printf '>>> test-pubs-turbo: the Turbo Initech manuals against the shipped compiler (initech-ipu5.12)\n'
@test -n "$(PUBS_TURBO_BOOKS)" || { printf '!!! test-pubs RED TPS-BOOKS: no pubs/manuals/turbo-initech-*/main.tex\n'; exit 1; }
@$(PUBS_TURBO_GATE) commands $(PUBS_TURBO_SHIPPED) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.cmds)
@$(PUBS_TURBO_GATE) msgcheck $(PUBS_TURBO_MSGS) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.msgs)
@$(PUBS_TURBO_GATE) mentions $(PUBS_TURBO_SHIPPED) $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.tpsmentions)
@$(PUBS_GATE) mentions $(PUBS_CHECK)/shipped $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.mentions)
@sh harness/pubs/turbo_gates_cited.sh $(foreach b,$(PUBS_TURBO_BOOKS),$(PUBS_BUILD)/gate/$(b)/main.gates)
@sh harness/pubs/turbo_grammar.sh $(PUBS_RAILROAD) $(PUBS_TURBO_GRAMMAR) $(TPS_LEX_FPC_BIN) $(PUBS_TURBO_DIR)/grammar \
	$(PUBS_TURBO_GRAMMAR_N) $(wildcard $(TPS_LEX_FIXTURE_DIR)/gen_*.pas) $(TPS_PARSE_RICH) $(TPS_LEX_SRC) \
	$(foreach f,$(wildcard $(PUBS_DIR)/manuals/turbo-initech-*/examples/*.pas),$(if $(findstring /err-,$(f)),,$(f)))
@$(PUBS_TURBO_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate OUT=$(PUBS_TURBO_DIR)/replay sh harness/pubs/turbo_replay.sh
@printf '>>> test-pubs-turbo: green\n'
```

## test-pubs-turbo-inputs

Declared tier: outside active vector; #  recipe to execute (from 'pubs/turbo.mk', line 66):
Prerequisites: `build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/compiler-os.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/seed-repro.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-gen-fpc.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-lex-build.txt pubs-inventory build/pubs/tps/shipped build/pubs_railroad build/qemu_harness build/tracer_secondary.img build/initechc build/seed_rt.o build/seed_rt_dos.o build/seed_fileio_rt.o build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/compiler-os.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/seed-repro.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-gen-fpc.txt build/pubs/tps/make/turbo-initech-self-compilation-guide/examples/tps-lex-build.txt build/.makefile.stamp`
```make
@set -eu; for cmd in $(PUBS_TURBO_MAKE_CMDS); do \
 actual=$(PUBS_TURBO_MAKE_DIR)/$${cmd#$(PUBS_DIR)/manuals/}; \
 diff -u $${cmd%.cmd}.txt $${actual%.cmd}.txt; \
 done
@printf '>>> test-pubs-turbo-inputs: green (printed make transcripts produced in private build trees)\n'
```

## test-pubs-turbo-mutant

Declared tier: outside active vector; #  recipe to execute (from 'pubs/turbo.mk', line 120):
Prerequisites: `test-pubs-turbo build/.makefile.stamp`
```make
@$(PUBS_TURBO_REPLAY_ENV) BUILT=$(PUBS_BUILD)/gate RAILROAD=$(PUBS_RAILROAD) GRAMMAR=$(PUBS_TURBO_GRAMMAR) \
	SHIPPED=$(PUBS_TURBO_SHIPPED) MSGS=$(PUBS_TURBO_MSGS) OUT=$(PUBS_TURBO_DIR)/mutants \
	PARSE_RICH=$(TPS_PARSE_RICH) FIXTURES=$(TPS_LEX_FIXTURE_DIR) sh harness/pubs/turbo_mutants.sh
```

## test-qemu-paths

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 19):
Prerequisites: `build/test_qemu_paths build/.makefile.stamp`
```make
@$<
```

## test-qemu-paths-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 21):
Prerequisites: `build/test_qemu_paths_mut build/.makefile.stamp`
```make
@if $< >$(BUILD)/qemu_paths_mut.log 2>&1; then echo 'FAIL QEMU_MUTATE_PATH_PREFLIGHT survived'; exit 1; fi
@grep '^RED qemu-paths: QMP socket path too long must fail loudly BEFORE launch' $(BUILD)/qemu_paths_mut.log
@echo 'GREEN test-qemu-paths-mutant: QEMU_MUTATE_PATH_PREFLIGHT caught RED'
```

## test-qemu-snapshot

Declared tier: EMU; #  recipe to execute (from 'harness/merge_samir.mk', line 60):
Prerequisites: `build/qemu_harness build/snapshot_write.bin build/snapshot_debug_exit.bin build/.makefile.stamp`
```make
@sh harness/emu/test_snapshot.sh $(HARNESS_BIN) $(BUILD)/snapshot_write.bin $(BUILD)/snapshot_debug_exit.bin
```

## test-region

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12536):
Prerequisites: `build/test_region build/.makefile.stamp`
```make
@printf ">>> test-region: ATKINSON region engine -- homomorphism (4 ops + complement) + normal-form + idempotence + algebra identities + rect-fast-path\n"
@$(TEST_REGION)
@printf ">>> test-region: green\n"
```

## test-region-gdi

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12629):
Prerequisites: `build/test_region_gdi build/.makefile.stamp`
```make
@printf ">>> test-region-gdi: ATKINSON dual-heritage spine -- L1 cross-family + L2 wine independent golden + L3 single-engine gate (ADR-0005 AM-1)\n"
@$(TEST_REGION_GDI)
@printf ">>> test-region-gdi: green\n"
```

## test-region-gdi-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12639):
Prerequisites: `build/test_region_gdi_mut_dispatch build/test_region_gdi_mut_rectin build/test_region_gdi_mut_emit build/test_region_gdi_mut_parity build/.makefile.stamp`
```make
@printf ">>> test-region-gdi-mutant: confirming the dual-heritage oracle BITES (Rule 6)\n"
@if $(TEST_REGION_GDI_MUT_DISPATCH) >/dev/null 2>&1; then \
	printf '!!! test-region-gdi-mutant FAIL: GDI_MUTATE_DISPATCH PASSED -- the mode->op semantics are not graded\n'; exit 1; \
else printf '>>> test-region-gdi-mutant: green (GDI_MUTATE_DISPATCH correctly RED -- L1+L2 catch a wrong mode->op)\n'; fi
@if $(TEST_REGION_GDI_MUT_EMIT) >/dev/null 2>&1; then \
	printf '!!! test-region-gdi-mutant FAIL: RGN_MUTATE_EMIT_NOCHANGE PASSED -- a wrong engine VALUE slips past\n'; exit 1; \
else printf '>>> test-region-gdi-mutant: green (RGN_MUTATE_EMIT_NOCHANGE correctly RED -- M-VALUE)\n'; fi
@if [ -f "$(WIN31_DECOMP)/oracles/wine/server/region.c" ]; then \
	if $(TEST_REGION_GDI_MUT_RECTIN) >/dev/null 2>&1; then \
		printf '!!! test-region-gdi-mutant FAIL: GDI_MUTATE_RECTIN PASSED -- the RectInRegion overlap value is not graded vs wine\n'; exit 1; \
	else printf '>>> test-region-gdi-mutant: green (GDI_MUTATE_RECTIN correctly RED -- RectInRegion overlap graded vs wine)\n'; fi; \
	if $(TEST_REGION_GDI_MUT_PARITY) >/dev/null 2>&1; then \
		printf '!!! test-region-gdi-mutant FAIL: RGN_MUTATE_PARITY_OFF1 PASSED -- the L2 wine differential is decoration\n'; exit 1; \
	else printf '>>> test-region-gdi-mutant: green (RGN_MUTATE_PARITY_OFF1 correctly RED -- L2 cross-redden)\n'; fi; \
else \
	sh harness/missing_reference.sh WIN31_DECOMP "test-region-gdi-mutant: $(WIN31_DECOMP)/oracles/wine/server/region.c absent" || exit $$?; \
fi
```

## test-region-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 12541):
Prerequisites: `build/test_region_mutant_vrle build/test_region_mutant_parity build/test_region_mutant_emit build/.makefile.stamp`
```make
@printf ">>> test-region-mutant: confirming all three mutants go RED (Rule 6)\n"
@if $(TEST_REGION_MUT_VRLE) >/dev/null 2>&1; then \
	printf '!!! test-region-mutant FAIL: NO_VRLE mutant PASSED -- the vertical-RLE oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-region-mutant: green (NO_VRLE mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_REGION_MUT_PARITY) >/dev/null 2>&1; then \
	printf '!!! test-region-mutant FAIL: PARITY_OFF1 mutant PASSED -- the parity oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-region-mutant: green (PARITY_OFF1 mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_REGION_MUT_EMIT) >/dev/null 2>&1; then \
	printf '!!! test-region-mutant FAIL: EMIT_NOCHANGE mutant PASSED -- the merge oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-region-mutant: green (EMIT_NOCHANGE mutant correctly RED -- the oracle bites)\n'; \
fi
@# NO_XMERGE_CAP (bead initech-mswo): drop the per-band xmerge output bound and
@# confirm the over-cap fail-loud probe goes RED. The scratch overrun is a
@# STACK buffer overflow, so the corruption detector is AddressSanitizer, not a
@# downstream guard (a plain build MASKS it: region_normalize's own cap check
@# aborts too, so the probe cannot tell fixed from reverted). ASAN catches the
@# FIRST OOB write in xmerge (exit != SIGABRT), which the probe distinguishes.
@# Missing ASAN fails loud unless SKIP_REFERENCE_TOOLS=1 explicitly opts out.
@if $(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DRGN_MUTATE_NO_XMERGE_CAP -fsanitize=address \
		$(REGION_INC) -o $(TEST_REGION_MUT_XCAP) $(TEST_REGION_SRC) $(REGION_ENGINE_C) 2>/dev/null; then \
	if ASAN_OPTIONS=abort_on_error=0:detect_leaks=0 $(TEST_REGION_MUT_XCAP) >/dev/null 2>&1; then \
		printf '!!! test-region-mutant FAIL: NO_XMERGE_CAP mutant PASSED -- the over-cap xmerge bound oracle is decoration (bead initech-mswo)\n'; \
		exit 1; \
	else \
		printf '>>> test-region-mutant: green (NO_XMERGE_CAP mutant correctly RED under ASAN -- xmerge scratch-overrun caught)\n'; \
	fi; \
else \
	sh harness/missing_reference.sh REFERENCE_TOOLS "test-region-mutant: ASAN unavailable (cannot build the scratch-overrun detector)" || exit $$?; \
fi
```

## test-revert-merge

Declared tier: HOST; #  recipe to execute (from 'harness/revert_merge.mk', line 18):
Prerequisites: `tools/revert-merge.sh harness/test_revert_merge.sh build/.makefile.stamp | build`
```make
@sh $(REVERT_MERGE_TEST) $(REVERT_MERGE_TOOL) $(BUILD)/revert-merge
```

## test-revert-merge-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/revert_merge.mk', line 24):
Prerequisites: `tools/revert-merge.sh harness/test_revert_merge.sh harness/test_revert_merge_mutant.sh build/.makefile.stamp | build`
```make
@sh $(REVERT_MERGE_MUTANT) $(REVERT_MERGE_TOOL) $(REVERT_MERGE_TEST) $(BUILD)/revert-merge-mut
```

## test-sentinel

Declared tier: HOST; #  recipe to execute (from 'harness/sentinel.mk', line 325):
Prerequisites: `build/.makefile.stamp | build`
```make
@$(file >$(BUILD)/sentinel-selftest.sh,$(value SENTINEL_SELFTEST_SH))
@sh $(BUILD)/sentinel-selftest.sh "$(CURDIR)" "$(CURDIR)/tools" "$(abspath $(BUILD)/test-sentinel)"
```

## test-sentinel-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/sentinel.mk', line 329):
Prerequisites: `build/.makefile.stamp | build`
```make
@$(file >$(BUILD)/sentinel-selftest.sh,$(value SENTINEL_SELFTEST_SH))
@set -eu; for m in newest-first flake-excused build-inherited missing-gate-culprit; do \
    d="$(abspath $(BUILD))/sentinel-mutant-$$m"; mkdir -p "$$d/tools"; \
    cp -f tools/sentinel.sh tools/culprit.sh "$$d/tools/"; \
    case $$m in newest-first) \
        sed -e 's/--reverse --first-parent/--first-parent/' -e 's/: # GREEN-CONTINUE/break # mutant stops at first green/' tools/culprit.sh > "$$d/tools/culprit.sh"; \
        reason=CULPRIT;; \
    flake-excused) \
        sed 's|: # FLAKE-KEEPS-RED|stage1_rc=0; sed "/^$$gate/d" "$$run/stage-1.red" > "$$run/red.next"; mv -f "$$run/red.next" "$$run/stage-1.red"|' tools/sentinel.sh > "$$d/tools/sentinel.sh"; \
        reason=FLAKE;; \
    build-inherited) \
        for tool in sentinel culprit; do sed 's/^unset BUILD /unset /' "tools/$$tool.sh" > "$$d/tools/$$tool.sh"; done; \
        reason=BUILD;; \
    missing-gate-culprit) \
        sed 's/if grep -Fq /if false \&\& grep -Fq /' tools/culprit.sh > "$$d/tools/culprit.sh"; \
        reason=MISSING-GATE;; \
    esac; \
    rc=0; sh $(BUILD)/sentinel-selftest.sh "$(CURDIR)" "$$d/tools" "$$d" > "$$d/oracle.log" 2>&1 || rc=$$?; \
    [ "$$rc" = 1 ] && grep -q "^RED sentinel $$reason:" "$$d/oracle.log" || { cat "$$d/oracle.log"; printf 'RED sentinel mutant survived or failed for wrong reason: %s\n' "$$m"; exit 1; }; \
    grep "^RED sentinel $$reason:" "$$d/oracle.log"; \
    printf 'Mutant %s: RED as required\n' "$$m"; \
done; printf '>>> test-sentinel-mutant: green (all four mutants correctly RED)\n'
```

## test-serial-trace-pack

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 91):
Prerequisites: `build/serial_trace_pack build/.makefile.stamp | build/ksize4`
```make
$(CC) $(CFLAGS) harness/proptest/serial_trace_fixture.c -o $(KERNEL_R4_DIR)/trace-original
$(CC) -E harness/proptest/serial_trace_fixture.c -o $(KERNEL_R4_DIR)/trace.i
$(SERIAL_TRACE_PACK) $(KERNEL_R4_DIR)/trace.i $(KERNEL_R4_DIR)/trace.c
$(CC) $(CFLAGS) $(KERNEL_R4_DIR)/trace.c -o $(KERNEL_R4_DIR)/trace-packed
@$(KERNEL_R4_DIR)/trace-original >$(KERNEL_R4_DIR)/trace-original.txt
@$(KERNEL_R4_DIR)/trace-packed >$(KERNEL_R4_DIR)/trace-packed.txt
@cmp $(KERNEL_R4_DIR)/trace-original.txt $(KERNEL_R4_DIR)/trace-packed.txt
$(CC) $(CFLAGS) harness/proptest/tbx_trace_fixture.c -o $(KERNEL_R4_DIR)/tbx-original
$(CC) -E -Dtputs=serial_puts -Dtputu=serial_putu -Dtputi=serial_puti harness/proptest/tbx_trace_fixture.c -o $(KERNEL_R4_DIR)/tbx.i
$(SERIAL_TRACE_PACK) $(KERNEL_R4_DIR)/tbx.i $(KERNEL_R4_DIR)/tbx.c tbx
$(CC) $(CFLAGS) $(KERNEL_R4_DIR)/tbx.c -o $(KERNEL_R4_DIR)/tbx-packed
@$(KERNEL_R4_DIR)/tbx-original >$(KERNEL_R4_DIR)/tbx-original.txt
@$(KERNEL_R4_DIR)/tbx-packed >$(KERNEL_R4_DIR)/tbx-packed.txt
@cmp $(KERNEL_R4_DIR)/tbx-original.txt $(KERNEL_R4_DIR)/tbx-packed.txt
@printf '>>> test-serial-trace-pack: green (original C calls == indexed packed trace bytes)\n'
```

## test-serial-trace-pack-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 108):
Prerequisites: `test-serial-trace-pack build/.makefile.stamp`
```make
$(CC) $(CFLAGS) -DSERIAL_TRACE_MUT_BYTE $(KERNEL_R4_DIR)/trace.c -o $(KERNEL_R4_DIR)/trace-mutant
@$(KERNEL_R4_DIR)/trace-mutant >$(KERNEL_R4_DIR)/trace-mutant.txt
@if cmp -s $(KERNEL_R4_DIR)/trace-original.txt $(KERNEL_R4_DIR)/trace-mutant.txt; then \
	printf '!!! test-serial-trace-pack-mutant FAIL: trace_byte survived\n'; exit 1; fi
@printf '!!! serial-trace-pack FAIL: trace_byte changes the original transcript\n'
@printf '>>> test-serial-trace-pack-mutant: green (trace_byte RED)\n'
```

## test-unit

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 31894):
Prerequisites: `build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf '>>> test-unit: host oracle vector (%s gates) -- unit + mutant\n' "$(words $(TEST_UNIT_GATES))"
@printf '======================================================================\n'
@for g in $(TEST_UNIT_GATES); do \
	printf '>>> test-unit: running %s\n' "$$g"; \
	$(MAKE) --no-print-directory $$g || { \
		printf '!!! test-unit FAIL: gate %s went RED\n' "$$g"; exit 1; }; \
done
@printf '======================================================================\n'
@printf 'test-unit: ALL GREEN (%s host gates)\n' "$(words $(TEST_UNIT_GATES))"
@printf '======================================================================\n'
```

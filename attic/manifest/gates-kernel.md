## test-40oq

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8066):
Prerequisites: `build/test_40oq build/.makefile.stamp`
```make
@printf ">>> test-40oq: Appendix-A INT 21h coverage certificate (initech-40oq capstone)\n"
@$(TEST_40OQ)
@printf ">>> test-40oq: green\n"
```

## test-40oq-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8071):
Prerequisites: `build/test_40oq_mutant_getver build/.makefile.stamp`
```make
@printf ">>> test-40oq-mutant: CERT_MUTATE_DROP_GETVER must go RED (Rule 6)\n"
@if $(TEST_40OQ_MUT) >/dev/null 2>&1; then \
	printf '!!! test-40oq-mutant FAIL: mutant PASSED -- the dynamic GETVER coverage check is decoration\n'; exit 1; \
else printf '>>> test-40oq-mutant: green (mutant correctly RED -- a real coverage gap is detected)\n'; fi
```

## test-4nbn-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5899):
Prerequisites: `build/test_int21_mutant_4nbn_setinfo build/test_int21_mutant_4nbn_instatus build/test_int21_mutant_4nbn_outstatus build/test_int21_mutant_4nbn_changeable build/.makefile.stamp`
```make
@printf ">>> test-4nbn-mutant: confirming all four AH=44h IOCTL minor mutants go RED (Rule 6; beads initech-4nbn)\n"
@if $(TEST_INT21_MUT_4NBN_SETINFO) >/dev/null 2>&1; then \
	printf '!!! test-4nbn-mutant FAIL: SETINFO_NOGUARD mutant PASSED -- the AL=01 char-device/DH==0 guard is decoration\n'; exit 1; \
else \
	printf '>>> test-4nbn-mutant: green (SETINFO_NOGUARD mutant correctly RED -- the AL=01 char-device-only/DH==0 reject bites)\n'; \
fi
@if $(TEST_INT21_MUT_4NBN_INSTAT) >/dev/null 2>&1; then \
	printf '!!! test-4nbn-mutant FAIL: INSTATUS_EOF_FLIP mutant PASSED -- the AL=06 file EOF test is decoration\n'; exit 1; \
else \
	printf '>>> test-4nbn-mutant: green (INSTATUS_EOF_FLIP mutant correctly RED -- the AL=06 offset>=size EOF answer bites)\n'; \
fi
@if $(TEST_INT21_MUT_4NBN_OUTSTAT) >/dev/null 2>&1; then \
	printf '!!! test-4nbn-mutant FAIL: OUTSTATUS_NOTREADY mutant PASSED -- the AL=07 always-ready answer is decoration\n'; exit 1; \
else \
	printf '>>> test-4nbn-mutant: green (OUTSTATUS_NOTREADY mutant correctly RED -- the AL=07 0xFF ready answer bites)\n'; \
fi
@if $(TEST_INT21_MUT_4NBN_CHANGE) >/dev/null 2>&1; then \
	printf '!!! test-4nbn-mutant FAIL: CHANGEABLE_OK mutant PASSED -- the AL=08 block-device-only invalid-function is decoration\n'; exit 1; \
else \
	printf '>>> test-4nbn-mutant: green (CHANGEABLE_OK mutant correctly RED -- the AL=08 0x0001 invalid-function answer bites)\n'; \
fi
```

## test-4tw

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5737):
Prerequisites: `build/test_conin build/.makefile.stamp`
```make
@printf ">>> test-4tw: CON-input ^C (0x03) on 01h/08h/0Ah -> INT 23h (Fork A; 07h/06h deliver raw)\n"
@$(TEST_CONIN)
@printf ">>> test-4tw: green\n"
```

## test-4tw-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5742):
Prerequisites: `build/test_conin_mutant_noctrlc build/.makefile.stamp`
```make
@printf ">>> test-4tw-mutant: confirming the no-Ctrl-C mutant goes RED (Rule 6)\n"
@if $(TEST_CONIN_MUT_NOCTRLC) >/dev/null 2>&1; then \
	printf '!!! test-4tw-mutant FAIL: no-Ctrl-C mutant PASSED -- the CON ^C -> INT 23h oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-4tw-mutant: green (no-Ctrl-C mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-80k

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5926):
Prerequisites: `build/test_int21_wildcard build/.makefile.stamp`
```make
@printf ">>> test-80k: DOS 8.3 FCB wildcard match (*.*, A*.*, *.TXT, FOO?.*, ?.BAR) + ?-over-pad quirks\n"
@$(TEST_WILDCARD)
@printf ">>> test-80k: green\n"
```

## test-80k-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5933):
Prerequisites: `build/test_int21_wildcard_mutant_qlit build/test_int21_wildcard_mutant_exact build/test_int21_wildcard_mutant_bleed build/test_int21_wildcard_mutant_noup build/.makefile.stamp`
```make
@printf ">>> test-80k-mutant: confirming all four wildcard mutants go RED (Rule 6)\n"
@if $(TEST_WILDCARD_MUT_QLIT) >/dev/null 2>&1; then \
	printf '!!! test-80k-mutant FAIL: qmark-literal mutant PASSED -- the ? wildcard test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-80k-mutant: green (qmark-literal mutant correctly RED)\n'; \
fi
@if $(TEST_WILDCARD_MUT_EXACT) >/dev/null 2>&1; then \
	printf '!!! test-80k-mutant FAIL: match-exact mutant PASSED -- the wildcard match arm is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-80k-mutant: green (match-exact mutant correctly RED)\n'; \
fi
@if $(TEST_WILDCARD_MUT_BLEED) >/dev/null 2>&1; then \
	printf '!!! test-80k-mutant FAIL: star-bleed mutant PASSED -- the per-field '"'"'*'"'"' fill test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-80k-mutant: green (star-bleed mutant correctly RED)\n'; \
fi
@if $(TEST_WILDCARD_MUT_NOUP) >/dev/null 2>&1; then \
	printf '!!! test-80k-mutant FAIL: no-upcase mutant PASSED -- the input upper-case test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-80k-mutant: green (no-upcase mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-absdisk

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4920):
Prerequisites: `build/test_absdisk build/absdisk_scratch.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-absdisk : INT 25h/26h ABSOLUTE DISK (DEC-15)\n'
@printf '  Ref: spec/absdisk_int2526.json (LOCKED); ADR-0003 Amendment DEC-15.\n'
@printf '  beads initech-4mq7. CLAUDE.md Law 2 (oracle is the truth).\n'
@printf '======================================================================\n'
@cp -f "$(ABSDISK_IMG)" "$(BUILD)/absdisk_run.img"
@$(TEST_ABSDISK) "$(BUILD)/absdisk_run.img"
@printf ">>> test-absdisk: green (round-trip + cross-check + non-corruption + error legs)\n"
```

## test-absdisk-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26064):
Prerequisites: `build/qemu_harness build/absdisk_boot.img build/fat_absdisk_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-absdisk-emu : INT 25h/26h asm-stub round-trip\n'
@printf '  Ref: spec/absdisk_int2526.json (LOCKED); ADR-0003 DEC-15; isr.asm.\n'
@printf '  beads initech-8403. CLAUDE.md Law 2 (oracle is the truth), Rule 5.\n'
@printf '======================================================================\n'
@# Idempotent (Rule 11): the kernel WRITEs the scratch disk, so run on a FRESH
@# copy of the blank volume each invocation -- re-running is clean.
@cp -f "$(FAT_ABSDISK_DATA_IMG)" "$(ABSDISK_EMU_RUNDISK)"
@printf 'Booting   : %s + WRITABLE BLANK data disk %s (primary slave)\n' "$(ABSDISK_BOOT_IMG)" "$(ABSDISK_EMU_RUNDISK)"
@printf 'Expecting : ABSDISK-BIND-OK + ABS-W26=OK + ABS-R25=OK + ABS-RT=OK + ABSDISK-EXIT rc=0 + no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(ABSDISK_EMU_RUNDISK))
@$(HARNESS_BIN) --disk "$(ABSDISK_BOOT_IMG)" --disk2 "$(ABSDISK_EMU_RUNDISK)" \
	--name "$(ABSDISK_EMU_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	--expect "ABSDISK-EXIT rc=0" --quit-after "ABSDISK-EXIT rc=0" \
	--screendump --screendump-after "ABSDISK-EXIT rc=0" \
	2> "$(ABSDISK_EMU_REPORT)"
@cat "$(ABSDISK_EMU_REPORT)"
@grep -qxF '[harness] OK=1' "$(ABSDISK_EMU_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(ABSDISK_EMU_REPORT)"; then \
	printf '!!! test-absdisk-emu FAIL: TRIPLE FAULT -- root-cause the int25/26 entry->dispatch->IRETD path (Rule 3)\n'; exit 1; \
fi
@printf '>>> test-absdisk-emu [1/5]: no triple-fault\n'
@if [ ! -s "$(ABSDISK_EMU_SERIAL)" ]; then \
	printf '!!! test-absdisk-emu FAIL: no serial captured at %s\n' "$(ABSDISK_EMU_SERIAL)"; exit 1; \
fi
@tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q '^ABSDISK-BIND-OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABSDISK-BIND-OK missing -- the absolute-disk seam never bound (no --disk2 / mount failed)\n'; exit 1; }
@printf '>>> test-absdisk-emu [2/5]: ABSDISK-BIND-OK (the absolute-disk seam bound from the mounted volume)\n'
@if tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q 'ABS-W26-FAIL\|ABS-R25-FAIL\|ABS-RT-FAIL'; then \
	printf '!!! test-absdisk-emu FAIL: an ABS-*-FAIL marker -- the int25/26 asm round-trip failed (root-cause, Rule 3):\n'; \
	tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep 'ABS-.*-FAIL'; exit 1; \
fi
@tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q '^ABS-W26=OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABS-W26=OK missing -- int $$0x26 did not return CF=0 through the asm stub\n'; exit 1; }
@tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q '^ABS-R25=OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABS-R25=OK missing -- int $$0x25 did not return CF=0 through the asm stub\n'; exit 1; }
@printf '>>> test-absdisk-emu [3/5]: ABS-W26=OK + ABS-R25=OK (int $$0x26 / int $$0x25 returned CF=0 via int26_entry/int25_entry)\n'
@tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q '^ABS-RT=OK$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABS-RT=OK missing -- the read-back bytes != the written pattern (the asm round-trip did not preserve the sector)\n'; exit 1; }
@printf '>>> test-absdisk-emu [4/5]: ABS-RT=OK (read-back == written pattern -- the WHOLE int26/int25 asm round-trip preserved the sector)\n'
@tr -d '\r' < "$(ABSDISK_EMU_SERIAL)" | grep -q '^ABSDISK-EXIT rc=0$$' \
	|| { printf '!!! test-absdisk-emu FAIL: ABSDISK-EXIT rc=0 missing -- the ABSDISK program did not finish cleanly\n'; exit 1; }
@printf '>>> test-absdisk-emu [5/5]: ABSDISK-EXIT rc=0 (clean exit)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a real guest issued int $$0x26 then int $$0x25 through the live IDT\n'
@printf '            trap gates; the int26_entry/int25_entry -> dispatch -> IRETD round-trip\n'
@printf '            wrote and read back a sector (QEMU only; tri-emulator pending initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-absdisk-emu: green (five checks; completed guest trace)\n'
```

## test-absdisk-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4935):
Prerequisites: `build/test_absdisk_mut_lba build/test_absdisk_mut_noop build/test_absdisk_mut_trans build/test_absdisk_mut_nobnd build/test_absdisk_mut_errclass build/test_absdisk_mut_neighbor build/test_absdisk_mut_wart build/test_absdisk_mut_packet build/absdisk_scratch.img build/.makefile.stamp`
```make
@printf ">>> test-absdisk-mutant: confirming all 8 mutants go RED (Rule 6; DEC-15 M1-M8)\n"
@set -e; \
for m in \
	"M1-off-by-one-LBA:$(TEST_ABSDISK_MUT_LBA)" \
	"M2-write-noop:$(TEST_ABSDISK_MUT_NOOP)" \
	"M3-reg-transpose:$(TEST_ABSDISK_MUT_TRANS)" \
	"M4-no-bounds:$(TEST_ABSDISK_MUT_NOBND)" \
	"M5-wrong-errclass:$(TEST_ABSDISK_MUT_ERRCLASS)" \
	"M6-corrupt-neighbor:$(TEST_ABSDISK_MUT_NEIGHBOR)" \
	"M7-stack-wart:$(TEST_ABSDISK_MUT_WART)" \
	"M8-packet-literal:$(TEST_ABSDISK_MUT_PACKET)" ; do \
	name=$${m%%:*}; bin=$${m#*:}; \
	cp -f "$(ABSDISK_IMG)" "$(BUILD)/absdisk_mut_run.img"; \
	if "$$bin" "$(BUILD)/absdisk_mut_run.img" >/dev/null 2>&1; then \
		printf '!!! test-absdisk-mutant FAIL: mutant %s PASSED -- the oracle is decoration\n' "$$name"; \
		exit 1; \
	else \
		printf '>>> test-absdisk-mutant: green (%s correctly RED)\n' "$$name"; \
	fi; \
done
@printf '>>> test-absdisk-mutant: green (all 8 DEC-15 mutants correctly RED) (the oracle bites)\n'
```

## test-ansi

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7740):
Prerequisites: `build/test_ansi test-ansi-bridge build/.makefile.stamp`
```make
@printf ">>> test-ansi: CSI parser + cursor + erase + SGR + save/restore + edge-clamp + mid-seq garbage\n"
@$(TEST_ANSI)
@printf ">>> test-ansi: green\n"
```

## test-ansi-bridge

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 21):
Prerequisites: `build/test_ansi_bridge build/.makefile.stamp`
```make
@$<
@printf ">>> test-ansi-bridge: green\n"
```

## test-ansi-bridge-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/dbf_diff/console_review.mk', line 28):
Prerequisites: `test-ansi-bridge build/test_ansi_bridge_mutant build/.makefile.stamp`
```make
@set +e; $(BUILD)/test_ansi_bridge_mutant > $(BUILD)/ansi-bridge-mutant.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q '^RED ANSI bridge: CUD bottom row expected=24 actual=25' $(BUILD)/ansi-bridge-mutant.log || { cat $(BUILD)/ansi-bridge-mutant.log; exit 1; }
@cat $(BUILD)/ansi-bridge-mutant.log
@printf '>>> test-ansi-bridge-mutant: green (relative high clamp restored RED)\n'
```

## test-ansi-emu

Declared tier: EMU; #  recipe to execute (from 'harness/emu/ansi.mk', line 67):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/ansi_probe_data.img harness/emu/ansi_probe.golden build/.makefile.stamp`
```make
$(call ansi-probe-run,$(SECONDARY_TRACER_IMG),ansi_probe)
@diff -u harness/emu/ansi_probe.golden $(BUILD)/ansi_probe.got
@printf '>>> test-ansi-emu: green (15 AH=09h sequences, exact DSR replies via INT 21h stdin, SHELL-EXIT)\n'
```

## test-ansi-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 222):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/ansi_probe_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-ansi-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-ansi-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/ansi_probe_data.img $(BUILD)/ansi-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/ansi-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "ansi-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/ansi-emu_bochs.report.txt" || true
@cat "$(BUILD)/ansi-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-ansi-emu-bochs "$(BUILD)/ansi-emu_bochs.report.txt" "$(BUILD)/ansi-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-ansi-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/ansi.mk', line 71):
Prerequisites: `build/qemu_harness build/ansi_ANSIWIRE_MUTATE_DROP_REL.img build/ansi_ANSIWIRE_MUTATE_DROP_DSR.img build/ansi_probe_data.img harness/emu/ansi_probe.golden build/.makefile.stamp`
```make
$(call ansi-probe-run,$(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_REL.img,ansi_probe_mut_rel)
@grep -q '^ANSI-PROBE FAIL DSR$$' $(BUILD)/ansi_probe_mut_rel.got
@! diff -q harness/emu/ansi_probe.golden $(BUILD)/ansi_probe_mut_rel.got >/dev/null
@printf '>>> test-ansi-emu-mutant: ANSIWIRE_MUTATE_DROP_REL correctly RED: ANSI-PROBE FAIL DSR\n'
$(call ansi-probe-run,$(BUILD)/ansi_ANSIWIRE_MUTATE_DROP_DSR.img,ansi_probe_mut_dsr)
@grep -q '^ANSI-PROBE FAIL DSR$$' $(BUILD)/ansi_probe_mut_dsr.got
@! diff -q harness/emu/ansi_probe.golden $(BUILD)/ansi_probe_mut_dsr.got >/dev/null
@printf '>>> test-ansi-emu-mutant: ANSIWIRE_MUTATE_DROP_DSR correctly RED: ANSI-PROBE FAIL DSR\n'
@printf '>>> test-ansi-emu-mutant: green (both implementation mutants rejected)\n'
```

## test-ansi-lane-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/ansi.mk', line 17):
Prerequisites: `build/test_ansi_lane_mut_ANSI_MUTATE_CONCEAL_BRIGHT build/test_ansi_lane_mut_ANSI_MUTATE_PARAM_UNBOUNDED build/test_ansi_lane_mut_ANSI_MUTATE_ZERO_LITERAL build/test_ansi_lane_mut_ANSI_MUTATE_STALE_REL build/test_ansi_wire_lane_mut_ANSI_MUTATE_CONCEAL_BRIGHT build/test_ansi_wire_lane_mut_ANSI_MUTATE_PARAM_UNBOUNDED build/test_ansi_wire_lane_mut_ANSI_MUTATE_ZERO_LITERAL build/test_ansi_wire_lane_mut_ANSI_MUTATE_STALE_REL build/test_ansi_wire_lane_mut_ANSIWIRE_MUTATE_DROP_REL build/test_ansi_wire_lane_mut_ANSIWIRE_MUTATE_DROP_DSR build/.makefile.stamp`
```make
@set -e; for bin in $^; do \
 if $$bin > $$bin.log 2>&1; then printf '!!! %s survived\n' "$$bin"; exit 1; fi; \
 grep -q 'checks,.* failures' $$bin.log; grep -q '^  FAIL ' $$bin.log; \
 printf '>>> %s correctly RED: %s\n' "$$bin" "$$(grep -m1 '^  FAIL ' $$bin.log)"; done
@printf '>>> test-ansi-lane-mutant: green (10 mutant executions rejected)\n'
```

## test-ansi-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7751):
Prerequisites: `build/test_ansi_mutant_paramaccum build/test_ansi_mutant_sgrcolor build/test_ansi_mutant_noclamp build/test_ansi_mutant_wrapflip build/test_console_mutant_nowrap test-ansi-bridge-mutant build/.makefile.stamp`
```make
@if $(BUILD)/test_console_mutant_nowrap >/dev/null 2>&1; then \
	printf '!!! test-ansi-mutant FAIL: nowrap-ignored console mutant PASSED\n'; exit 1; \
else printf '>>> test-ansi-mutant: green (nowrap-ignored console mutant correctly RED)\n'; fi
@if $(BUILD)/test_ansi_mutant_wrapflip >/dev/null 2>&1; then \
	printf '!!! test-ansi-mutant FAIL: wrap-flip mutant PASSED\n'; exit 1; \
else printf '>>> test-ansi-mutant: green (wrap-flip mutant correctly RED)\n'; fi
@printf ">>> test-ansi-mutant: confirming all three mutants go RED (Rule 6)\n"
@if $(TEST_ANSI_MUT_PARAM) >/dev/null 2>&1; then \
	printf '!!! test-ansi-mutant FAIL: param-accum mutant PASSED -- the multi-digit param test is decoration\n'; exit 1; \
else printf '>>> test-ansi-mutant: green (param-accum mutant correctly RED)\n'; fi
@if $(TEST_ANSI_MUT_SGR) >/dev/null 2>&1; then \
	printf '!!! test-ansi-mutant FAIL: sgr-color mutant PASSED -- the ANSI-to-CGA colour test is decoration\n'; exit 1; \
else printf '>>> test-ansi-mutant: green (sgr-color mutant correctly RED -- the oracle bites)\n'; fi
@if $(TEST_ANSI_MUT_CLAMP) >/dev/null 2>&1; then \
	printf '!!! test-ansi-mutant FAIL: no-clamp mutant PASSED -- the cursor-edge-clamp test is decoration\n'; exit 1; \
else printf '>>> test-ansi-mutant: green (no-clamp mutant correctly RED -- the oracle bites)\n'; fi
```

## test-ansi-wire

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8022):
Prerequisites: `build/test_ansi_wire build/.makefile.stamp`
```make
@printf ">>> test-ansi-wire: con_putc -> ansi_feed -> console-ops (gate + cursor + erase + SGR + save/restore + plain-text identity)\n"
@$(TEST_ANSI_WIRE)
@printf ">>> test-ansi-wire: green\n"
```

## test-ansi-wire-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8027):
Prerequisites: `build/test_ansi_wire_mutant_gate build/.makefile.stamp`
```make
@printf ">>> test-ansi-wire-mutant: confirming the ignore-gate mutant goes RED (Rule 6)\n"
@if $(TEST_ANSI_WIRE_MUT) >/dev/null 2>&1; then \
	printf '!!! test-ansi-wire-mutant FAIL: ignore-gate mutant PASSED -- the ANSI-OFF literal-render test is decoration\n'; exit 1; \
else printf '>>> test-ansi-wire-mutant: green (ignore-gate mutant correctly RED -- the oracle bites)\n'; fi
```

## test-arena-disjoint

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28695):
Prerequisites: `build/test_arena_disjoint build/.makefile.stamp`
```make
@printf '>>> test-arena-disjoint: AH=48h arena disjoint from program image+BSS / env / stack (DEC-04)\n'
@$(TEST_ARENADJ)
@printf '>>> test-arena-disjoint: green\n'
```

## test-arena-disjoint-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28699):
Prerequisites: `build/test_arena_disjoint_mut build/.makefile.stamp`
```make
@if $(TEST_ARENADJ_MUT) >/dev/null 2>&1; then printf '!!! test-arena-disjoint-mutant FAIL: overlap mutant PASSED -- oracle is decoration\n'; exit 1; \
else printf '>>> test-arena-disjoint-mutant: green (overlap mutant correctly RED)\n'; fi
```

## test-ata

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 53):
Prerequisites: `build/test_ata build/.makefile.stamp | build/khygiene-evidence`
```make
@$<
```

## test-ata-errors-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 136):
Prerequisites: `build/test_ata_errors build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-ata-errors.log 2>&1; then echo 'FAIL ATA_MUTATE_ERROR_COLLAPSE survived'; exit 1; fi
@grep 'FAIL.*distinct post-command error' $(BUILD)/khygiene-evidence/mutant-ata-errors.log
@echo 'GREEN test-ata-errors-mutant: ATA_MUTATE_ERROR_COLLAPSE correctly RED'
```

## test-ata-flush-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 134):
Prerequisites: `build/test_ata_flush build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-ata-flush.log 2>&1; then echo 'FAIL ATA_MUTATE_FLUSH_EARLY survived'; exit 1; fi
@grep 'FAIL.*final WRITE completion' $(BUILD)/khygiene-evidence/mutant-ata-flush.log
@echo 'GREEN test-ata-flush-mutant: ATA_MUTATE_FLUSH_EARLY correctly RED'
```

## test-ata-lba-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 135):
Prerequisites: `build/test_ata_lba build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-ata-lba.log 2>&1; then echo 'FAIL ATA_MUTATE_LBA_WRAP survived'; exit 1; fi
@grep 'FAIL.*LBA28 transfer end' $(BUILD)/khygiene-evidence/mutant-ata-lba.log
@echo 'GREEN test-ata-lba-mutant: ATA_MUTATE_LBA_WRAP correctly RED'
```

## test-ata-no-drive-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 211):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/ata_no_drive.sh $(BUILD) $(SECONDARY_TRACER_IMG) green
```

## test-ata-no-drive-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 239):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-ata-no-drive-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-ata-no-drive-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
@true
@$(BOCHS_BIN) --disk "build/tracer_secondary.img"   --expect SHELL-READY --quit-after SHELL-READY --name "ata-no-drive-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/ata-no-drive-emu_bochs.report.txt" || true
@cat "$(BUILD)/ata-no-drive-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-ata-no-drive-emu-bochs "$(BUILD)/ata-no-drive-emu_bochs.report.txt" "$(BUILD)/ata-no-drive-emu_bochs.serial" vbe101 'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0' IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-ata-no-drive-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 224):
Prerequisites: `build/qemu_harness build/ata_no_drive_success.img build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/ata_no_drive.sh $(BUILD) $(BUILD)/ata_no_drive_success.img mutant > $(BUILD)/khygiene-evidence/mutant-ata-nodrive.log 2>&1; then echo 'FAIL no-drive success mutant survived'; exit 1; fi
@grep 'RED ATA no-drive:' $(BUILD)/khygiene-evidence/mutant-ata-nodrive.log
@grep -qx 'FAT-MOUNT-FAIL rc=-2 base_lba=0 hidden_sectors=0' $(BUILD)/khygiene-ata-mutant/nodrive.serial
@grep -qx SHELL-DONE $(BUILD)/khygiene-ata-mutant/nodrive.serial
@echo 'GREEN test-ata-no-drive-emu-mutant: ATA_MUTATE_NO_DRIVE_SUCCESS correctly RED'
```

## test-ata-prewait-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 133):
Prerequisites: `build/test_ata_prewait build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-ata-prewait.log 2>&1; then echo 'FAIL ATA_MUTATE_NO_PREWAIT survived'; exit 1; fi
@grep 'FAIL.*BSY=0 DRDY=1' $(BUILD)/khygiene-evidence/mutant-ata-prewait.log
@echo 'GREEN test-ata-prewait-mutant: ATA_MUTATE_NO_PREWAIT correctly RED'
```

## test-attrib

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 57):
Prerequisites: `build/dosutil_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dosutil_diff.sh attrib $(DOSUTIL_HOST) attrib
@printf '>>> test-attrib: green (oracle completed)\n'
```

## test-attrib-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 63):
Prerequisites: `build/dosutil_host_R build/dosutil_host_A build/dosutil_host_PRESERVE build/dosutil_host_WILD build/dosutil_host_RECURSE build/dosutil_host_DISPLAY build/dosutil_host_SYNTAX build/.makefile.stamp`
```make
@set -e; for m in $(DOSUTIL_ATTR_MUTANTS); do \
  log=$(BUILD)/dosutil_mut_$$m.log; \
  if sh harness/diff/fat_diff/dosutil_diff.sh attrib $(BUILD)/dosutil_host_$$m mut_$$m >$$log 2>&1; then echo "FAIL: $$m survived"; exit 1; fi; \
  case $$m in R) rule=A1;; A) rule=A2;; PRESERVE) rule=A3;; WILD) rule=A4;; RECURSE) rule=A5;; DISPLAY) rule=A6;; SYNTAX) rule=A7;; esac; \
  grep "^FAIL: $$rule " $$log || { cat $$log; exit 1; }; \
  printf 'PASS: DU_MUTATE_%s caught RED\n' "$$m"; \
done
@printf '>>> test-attrib-mutant: green (oracle completed)\n'
```

## test-b53d

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6867):
Prerequisites: `build/test_fileio_subdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mattrib >/dev/null 2>&1 || { printf '!!! test-b53d FAIL: mtools `mattrib` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-b53d FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-b53d: AH=43h CHMOD GET/SET ATTR persistence vs python --attr + mtools mattrib (beads initech-b53d)\n"
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_B53D_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_B53D_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@# (1) CREATE '\SUB\ATTR.TXT' (0x20 baseline) + AH=43h SET RO|HIDDEN (0x03) +
@# FLUSH; the harness self-checks the 3 in-process read-backs + the dir/missing
@# reject set (10 CHECKs). A non-zero failure count fails the gate.
@$(TEST_FILEIO_SUBDIR) "$(FAT12_B53D_IMG)" "$(FAT12_FIXTURE_DIR)" --write attr-set > $(BUILD)/b53d_attrset.report 2>&1; \
	grep -Eq 'test_fileio_subdir: [0-9]+ checks, 0 failures' $(BUILD)/b53d_attrset.report \
	|| { printf '!!! test-b53d FAIL [SET]: the attr-set harness reported failures (in-process read-back / reject set):\n'; cat $(BUILD)/b53d_attrset.report; exit 1; }
@printf '>>> test-b53d [SET]: \\SUB\\ATTR.TXT on-disk attr set to 0x03 (dispatch GET + fat12_get_attr + resolved entry agree; dir/missing rejects fire)\n'
@# (2) python: the raw on-disk attribute byte (ent[11]) EXACTLY == 3 (0x03).
@test "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --attr 'SUB\ATTR.TXT')" = "3" \
	|| { printf '!!! test-b53d FAIL [SET]: python --attr != 3 (the written RO|HIDDEN 0x03)\n'; \
	     printf '    got: [%s]\n' "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --attr 'SUB\ATTR.TXT')"; exit 1; }
@# (3) mtools mattrib: the SAME byte rendered as the flag string "HR"
@# (Hidden + Read-only) -- an INDEPENDENT decoder agreeing with python.
@mattrib -i $(FAT12_B53D_IMG) ::SUB/ATTR.TXT 2>/dev/null | grep -Eq '^ +HR +' \
	|| { printf '!!! test-b53d FAIL [SET]: mtools mattrib does not render ATTR.TXT as HR (Hidden+Read-only == 0x03)\n'; \
	     mattrib -i $(FAT12_B53D_IMG) ::SUB/ATTR.TXT 2>/dev/null; exit 1; }
@printf '>>> test-b53d [SET]: \\SUB\\ATTR.TXT on-disk attr == 0x03 (python --attr==3 == mtools mattrib HR == fat12_get_attr)\n'
@# (4) Rule 11: the time-set... ahem, the ATTR-set flush touched ONLY the 0x0B
@# byte -- the BODY and mtime/mdate are untouched (deterministic 0/0 baseline).
@python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --cat-path 'SUB\ATTR.TXT' > $(BUILD)/b53d_body.bin
@python3 -c 'import sys; e=bytes(65+((i%26)) for i in range(48)); sys.exit(0 if open("$(BUILD)/b53d_body.bin","rb").read()==e else 1)' \
	|| { printf '!!! test-b53d FAIL [SET]: the file body changed after the attr-set flush (data clobbered)\n'; exit 1; }
@test "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --stat-path-time 'SUB\ATTR.TXT')" = "0 0" \
	|| { printf '!!! test-b53d FAIL [SET]: the attr-set flush DISTURBED mtime/mdate (Rule 11 violated -- the RMW must touch ONLY 0x0B)\n'; exit 1; }
@printf '>>> test-b53d [SET]: ATTR.TXT body + mtime/mdate untouched by CHMOD (attribute-byte-only RMW, Rule 11)\n'
@# (5) GET-on-DIRECTORY fidelity (beads initech-b53d fix): real DOS AH=4300h on
@# a directory SUCCEEDS with CX=0x10 (RBIL, no directory exclusion). The harness
@# attr-set self-check ALREADY proved the in-process legs (AH=43h GET '\SUB\DEEP'
@# -> CF=0/CX=0x10 AND fat12_get_attr off the cached FAT -> 0x10). Here the TWO
@# INDEPENDENT references must AGREE the DEEP directory reads 0x10 -- the 3-way
@# differential (fat12_get_attr / python --attr / mtools mattrib):
@#   python --attr: the raw on-disk attribute byte (ent[11]) EXACTLY == 16 (0x10)
@test "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --attr 'SUB\DEEP')" = "16" \
	|| { printf '!!! test-b53d FAIL [GET-dir]: python --attr SUB\\DEEP != 16 (the dir 0x10 byte)\n'; \
	     printf '    got: [%s]\n' "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --attr 'SUB\DEEP')"; exit 1; }
@#   mtools mattrib: a directory carries NO R/H/S/A flag letters (the 0x10
@#   Directory bit alone) -- an INDEPENDENT decoder agreeing it is not R/H/S/A.
@#   mattrib prints "<flags>   ::/path"; isolate the FLAGS COLUMN (everything
@#   BEFORE '::') so the path text 'SUB' is not mistaken for flag letters.
@mattrib -i $(FAT12_B53D_IMG) ::SUB/DEEP 2>/dev/null | sed 's/::.*//' | grep -Eq '[RHSA]' \
	&& { printf '!!! test-b53d FAIL [GET-dir]: mtools mattrib shows R/H/S/A flags on the DEEP directory (expected bare 0x10)\n'; \
	     mattrib -i $(FAT12_B53D_IMG) ::SUB/DEEP 2>/dev/null; exit 1; } || true
@printf '>>> test-b53d [GET-dir]: \\SUB\\DEEP reads 0x10 (fat12_get_attr == python --attr==16 == mtools mattrib bare; GET-on-dir succeeds, RBIL 4300h)\n'
@printf ">>> test-b53d: green\n"
```

## test-b53d-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6926):
Prerequisites: `build/test_fileio_b53d_mut_noalreject build/test_fileio_b53d_mut_getzero build/test_fileio_b53d_mut_nodispatch build/test_fileio_subdir_b53d_mut_noflush build/test_fileio_subdir_b53d_mut_noreject build/test_fileio_subdir_b53d_mut_notfound build/test_fileio_subdir_b53d_mut_getdirreject build/test_fileio_b53d_mut_nocxreject build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf ">>> test-b53d-mutant: confirming all EIGHT AH=43h mutants go RED for the RIGHT reason (Rule 6; beads initech-b53d / initech-5o6o)\n"
@# --- MUTANT 1 (NO_AL_REJECT): AL=2 no longer rejected (falls to GET) -> the
@# bad-AL host contract (CF=1/0x0001) bites.
@$(TEST_B53D_MUT_NOALREJECT) > $(BUILD)/b53d_mut_noalreject.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/b53d_mut_noalreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_AL_REJECT mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_noalreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_AL_REJECT mutant PASSED -- the AL=2 reject is decoration\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (NO_AL_REJECT ran + RED -- the AL=2 reject bites)\n'
@# --- MUTANT 2 (GET_ZERO): GET returns CX=0 instead of the real attr -> the
@# host GET-returns-CX contract bites.
@$(TEST_B53D_MUT_GETZERO) > $(BUILD)/b53d_mut_getzero.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/b53d_mut_getzero.report \
	|| { printf '!!! test-b53d-mutant FAIL: GET_ZERO mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_getzero.report \
	|| { printf '!!! test-b53d-mutant FAIL: GET_ZERO mutant PASSED -- the GET-returns-CX contract is decoration\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (GET_ZERO ran + RED -- the GET CX=attr contract bites)\n'
@# --- MUTANT 6 (NO_DISPATCH): case 0x43 omitted -> CHMOD falls to the
@# not-yet-impl path (CF=1/0x0001); every chmod host assertion bites.
@$(TEST_B53D_MUT_NODISPATCH) > $(BUILD)/b53d_mut_nodispatch.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/b53d_mut_nodispatch.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_DISPATCH mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_nodispatch.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_DISPATCH mutant PASSED -- the AH=43h dispatch is decoration\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (NO_DISPATCH ran + RED -- the case 0x43 dispatch bites)\n'
@# --- MUTANT 3 (NO_FLUSH): SET patches memory but skips the write-back ->
@# the on-disk attr stays the 0x20 baseline; python --attr != 3. The harness
@# itself ALSO bites (its fat12_get_attr re-scan reads the unflushed disk).
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_B53D_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_B53D_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_B53D_MUT_NOFLUSH) "$(FAT12_B53D_IMG)" "$(FAT12_FIXTURE_DIR)" --write attr-set > $(BUILD)/b53d_mut_noflush.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/b53d_mut_noflush.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_FLUSH mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@if [ "$$(python3 $(FAT12_REF_PY) $(FAT12_B53D_IMG) --attr 'SUB\ATTR.TXT')" = "3" ]; then \
	printf '!!! test-b53d-mutant FAIL: NO_FLUSH mutant PASSED -- the attr landed on disk despite the skipped flush; the flush oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-b53d-mutant: green (NO_FLUSH ran + RED -- the on-disk attr is absent; the write-back flush is load-bearing)\n'; \
fi
@# --- MUTANT 4 (NO_REJECT): the dir/vol-label TARGET reject is dropped -> the
@# SET on the DIRECTORY '\SUB\DEEP' succeeds (CF=0) instead of being denied;
@# the harness dir-reject CHECK bites.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_B53D_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_B53D_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_B53D_MUT_NOREJECT) "$(FAT12_B53D_IMG)" "$(FAT12_FIXTURE_DIR)" --write attr-set > $(BUILD)/b53d_mut_noreject.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/b53d_mut_noreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_REJECT mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_noreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_REJECT mutant PASSED -- the directory-target reject is decoration (a dir was CHMODed)\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (NO_REJECT ran + RED -- the dir/vol-label TARGET reject bites)\n'
@# --- MUTANT 5 (NOTFOUND_ACCESS): fileio_fat maps fat12 NOT_FOUND to 0x0005 ->
@# a CHMOD of a missing file reports 0x0005 not 0x0002; the harness missing-file
@# CHECK bites.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_B53D_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_B53D_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_B53D_MUT_NOTFOUND) "$(FAT12_B53D_IMG)" "$(FAT12_FIXTURE_DIR)" --write attr-set > $(BUILD)/b53d_mut_notfound.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/b53d_mut_notfound.report \
	|| { printf '!!! test-b53d-mutant FAIL: NOTFOUND_ACCESS mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_notfound.report \
	|| { printf '!!! test-b53d-mutant FAIL: NOTFOUND_ACCESS mutant PASSED -- the missing-file 0x0002 contract is decoration\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (NOTFOUND_ACCESS ran + RED -- the missing-file 0x0002 contract bites)\n'
@# --- MUTANT 7 (GETATTR_DIR_REJECT): fat12_get_attr RE-INTRODUCES the removed
@# GET-on-directory reject -> a GET of '\SUB\DEEP' is denied (CF=1/0x0005)
@# instead of returning CF=0/CX=0x10; the subdir harness GET-on-dir legs bite.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_B53D_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_B53D_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_B53D_MUT_GETDIRREJECT) "$(FAT12_B53D_IMG)" "$(FAT12_FIXTURE_DIR)" --write attr-set > $(BUILD)/b53d_mut_getdirreject.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/b53d_mut_getdirreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: GETATTR_DIR_REJECT mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_getdirreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: GETATTR_DIR_REJECT mutant PASSED -- the GET-on-dir CF=0/CX=0x10 contract is decoration (the dir reject was wrongly re-allowed)\n'; exit 1; }
@printf '>>> test-b53d-mutant: green (GETATTR_DIR_REJECT ran + RED -- the GET-on-dir 0x10 contract bites; the reject must stay OUT of GET)\n'
@# --- MUTANT 8 (NO_CX_REJECT): do_chmod drops the SET-time CX re-typing guard
@# (the dispatch-edge "defense in depth" reject, initech-5o6o / ADR DEC-14.2) ->
@# a SET whose CX sets the Directory(0x10)/VolLabel(0x08) bit is NO LONGER denied
@# at the dispatcher. BOTH host dispatch-edge legs bite INDEPENDENTLY: SET
@# HELLO.TXT with CX|0x10 (Directory) AND SET README (a plain file) with CX|0x08
@# (VolLabel) must each report CF=1/0x0005 -- the README target dodges the
@# backend TARGET reject so only the dropped dispatch guard can deny it.
@$(TEST_B53D_MUT_NOCXREJECT) > $(BUILD)/b53d_mut_nocxreject.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/b53d_mut_nocxreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_CX_REJECT mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/b53d_mut_nocxreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_CX_REJECT mutant PASSED -- the dispatch-edge CX re-typing reject is decoration (ADR DEC-14.2 claim false)\n'; exit 1; }
@# Prove BOTH dispatch-edge legs (Directory AND VolLabel) independently bite --
@# guard against the test-ordering mask the adversary flagged (initech-5o6o).
@grep -q 'CX setting the Directory bit' $(BUILD)/b53d_mut_nocxreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_CX_REJECT mutant did not bite the Directory-bit dispatch-edge leg\n'; cat $(BUILD)/b53d_mut_nocxreject.report; exit 1; }
@grep -q 'CX setting the VolLabel bit' $(BUILD)/b53d_mut_nocxreject.report \
	|| { printf '!!! test-b53d-mutant FAIL: NO_CX_REJECT mutant did not bite the VolLabel-bit dispatch-edge leg (masked by test ordering -- the guard is not independently proven)\n'; cat $(BUILD)/b53d_mut_nocxreject.report; exit 1; }
@printf '>>> test-b53d-mutant: green (NO_CX_REJECT ran + RED -- the dispatch-edge CX re-typing reject bites BOTH the Directory AND VolLabel legs independently)\n'
@printf '>>> test-b53d-mutant: green (ALL EIGHT AH=43h mutants ran + RED for the right reason)\n'
```

## test-baked-exec-guard

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 33):
Prerequisites: `build/qemu_harness build/baked_guard.img build/fat_exec.img build/.makefile.stamp | build/khygiene-evidence`
```make
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/baked_guard.img --disk2 "$(FLAIR_GATE_DATA)" --name baked_guard --out $(BUILD) --timeout-ms 8000 --quit-after EXEC-4B-END > $(BUILD)/baked_guard.stdout 2> $(BUILD)/baked_guard.report || true
@! grep -q 'triple_fault=1' $(BUILD)/baked_guard.report
@tr -d '\r' < $(BUILD)/baked_guard.serial > $(BUILD)/baked_guard.normalized
@grep -qx 'KHY-BAKED-NESTED-OK' $(BUILD)/baked_guard.normalized || { echo 'RED baked EXEC guard: nested load did not restore caller PSP/rc'; exit 1; }
@grep -qx 'PROGRAM-EXIT rc=7' $(BUILD)/baked_guard.serial
@grep -qx 'EXEC-4B-RC=7' $(BUILD)/baked_guard.serial
@echo '>>> test-baked-exec-guard: green (nested child 7, restored caller PSP, outer exit 7, subsequent EXEC 7)'
```

## test-baked-exec-guard-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 240):
Prerequisites: `build/bochs_harness build/baked_guard.img build/fat_exec.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-baked-exec-guard-bochs : BOCHS boot leg of %s (%s video path)\n' "build/baked_guard.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-baked-exec-guard-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/fat_exec.img $(BUILD)/baked-exec-guard_bochs_data.img
@$(BOCHS_BIN) --disk "build/baked_guard.img" --disk2 "$(BUILD)/baked-exec-guard_bochs_data.img"  --expect EXEC-4B-END --quit-after EXEC-4B-END --name "baked-exec-guard_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/baked-exec-guard_bochs.report.txt" || true
@cat "$(BUILD)/baked-exec-guard_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-baked-exec-guard-bochs "$(BUILD)/baked-exec-guard_bochs.report.txt" "$(BUILD)/baked-exec-guard_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE EXEC-4B-END !PANIC
```

## test-baked-exec-guard-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 95):
Prerequisites: `build/qemu_harness build/baked_unguarded.img build/fat_exec.img build/.makefile.stamp | build/khygiene-evidence`
```make
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/baked_unguarded.img --disk2 "$(FLAIR_GATE_DATA)" --name baked_unguarded --out $(BUILD) --timeout-ms 6000 > $(BUILD)/baked_unguarded.stdout 2> $(BUILD)/baked_unguarded.report || true
@grep -q 'PROGRAM-BEGIN' $(BUILD)/baked_unguarded.serial
@grep -q '^GREETINGS FROM A:GREET.COM' $(BUILD)/baked_unguarded.serial
@! grep -q 'PROGRAM-EXIT rc=7' $(BUILD)/baked_unguarded.serial || { echo 'FAIL unguarded baked mutant survived'; exit 1; }
@echo '>>> LOADER_MUT_NO_RESTORE RED: outer PROGRAM-EXIT rc=7 missing'
@echo '>>> test-baked-exec-guard-mutant: green (caller PSP restoration mutant caught)'
```

## test-block-device

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 32):
Prerequisites: `build/test_block_device build/.makefile.stamp`
```make
@$<
```

## test-block-device-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 40):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/test_partition build/block_device_probe.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/block_device.sh $(BUILD) $(SECONDARY_TRACER_IMG)
```

## test-block-device-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 219):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-block-device-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-block-device-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/block-device-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/block-device-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "block-device-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/block-device-emu_bochs.report.txt" || true
@cat "$(BUILD)/block-device-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-block-device-emu-bochs "$(BUILD)/block-device-emu_bochs.report.txt" "$(BUILD)/block-device-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-block-device-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 53):
Prerequisites: `build/qemu_harness build/block_device_relative_boot.img build/test_partition build/block_device_probe.bin build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/block-device
@if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/block_device.sh $(BUILD) $(BUILD)/block_device_relative_boot.img > $(BUILD)/block-device/relative.log 2>&1; then echo 'FAIL block-device relative mutant survived'; exit 1; fi
@grep '^RED block-device write device LBA 32' $(BUILD)/block-device/relative.log
@echo 'GREEN test-block-device-emu-mutant: volume-relative write rejected'
```

## test-block-device-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 34):
Prerequisites: `build/test_block_device_HEADS build/test_block_device_CARRY build/test_block_device_WATCH build/.makefile.stamp`
```make
@set -eu; for m in HEADS CARRY WATCH; do \
 log=$(BUILD)/block-device-$$m.log; \
 if $(BUILD)/test_block_device_$$m > $$log 2>&1; then echo "FAIL block-device $$m survived"; exit 1; fi; \
 grep '^RED block-device ' $$log; done
@echo 'GREEN test-block-device-mutant: HEADS, CARRY and WATCH rejected'
```

## test-block-device-spec

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 9):
Prerequisites: `build/gen_block_device_spec spec/block_device_contract.json spec/block_device_contract.h spec/mbr_partition_contract.json build/.makefile.stamp`
```make
@$(BUILD)/gen_block_device_spec spec/block_device_contract.json spec/mbr_partition_contract.json > $(BUILD)/block_device_contract.h
@cmp spec/block_device_contract.h $(BUILD)/block_device_contract.h
@echo 'GREEN test-block-device-spec: JSON/header equal'
```

## test-block-device-spec-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/block_device.mk', line 13):
Prerequisites: `build/gen_block_device_spec spec/block_device_contract.json build/.makefile.stamp`
```make
@sed 's/"heads": 16/"heads": 15/' spec/block_device_contract.json > $(BUILD)/block_device_mut.json
@$(BUILD)/gen_block_device_spec $(BUILD)/block_device_mut.json > $(BUILD)/block_device_mut.h
@if cmp -s spec/block_device_contract.h $(BUILD)/block_device_mut.h; then echo 'FAIL spec mutant survived'; exit 1; fi
@echo 'RED block-device SPEC heads drift'
@echo 'GREEN test-block-device-spec-mutant: JSON heads drift rejected'
```

## test-boot-novbe-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 201):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-boot-novbe-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "e00"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-boot-novbe-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/boot-novbe_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/boot-novbe_bochs_data.img" --vga-ext none --expect SHELL-READY --quit-after SHELL-READY --name "boot-novbe_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/boot-novbe_bochs.report.txt" || true
@cat "$(BUILD)/boot-novbe_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-boot-novbe-bochs "$(BUILD)/boot-novbe_bochs.report.txt" "$(BUILD)/boot-novbe_bochs.serial" e00 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-boot-vga13-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 200):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-boot-vga13-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vga13"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-boot-vga13-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@test -r /usr/share/vgabios/vgabios.cirrus.bin || { printf '!!! test-boot-vga13-bochs FAIL: ISA Cirrus VGA BIOS %s missing (apt install vgabios; bochs Depends: vgabios | seabios). A skipped oracle is worse than a red one (Law 2).\n' "/usr/share/vgabios/vgabios.cirrus.bin"; exit 1; }
cp -f build/flair_data.img $(BUILD)/boot-vga13_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/boot-vga13_bochs_data.img" --vga-ext cirrus --vgabios /usr/share/vgabios/vgabios.cirrus.bin --expect SHELL-READY --quit-after SHELL-READY --name "boot-vga13_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/boot-vga13_bochs.report.txt" || true
@cat "$(BUILD)/boot-vga13_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-boot-vga13-bochs "$(BUILD)/boot-vga13_bochs.report.txt" "$(BUILD)/boot-vga13_bochs.serial" vga13 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-bsy8-pipe

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31413):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin build/gobble_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy8.bat build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-bsy8-pipe : the `|` PIPE (DOS temp-file pipe)\n'
@printf '  beads initech-bsy.8. AUTOEXEC.BAT: GREET.COM | GOBBLE.COM ; TYPE PIPE1.$$$$$$.\n'
@printf '  Proves GREET`s stdout flows through a temp file into GOBBLE`s stdin, GOBBLE\n'
@printf '  echoes it to serial, and the temp is DELETED afterward (TYPE -> File not found).\n'
@printf '  Prereqs bsy.7+bsy.9+hsct.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-bsy8-pipe FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-bsy8-pipe FAIL: mtools `mcopy` not found.\n'; exit 1; }
$(call bsy8-pipe-mint-disk,$(BSY8_PIPE_IMG))
@printf 'Booting   : %s + data disk %s (AUTOEXEC.BAT pipe script + GREET.COM + GOBBLE.COM)\n' "$(SECONDARY_TRACER_IMG)" "$(BSY8_PIPE_IMG)"
@printf 'Expecting : GOBBLE-BEGIN once, GREETINGS once (via the temp pipe), "File not found" (temp gone), clean EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(BSY8_PIPE_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(BSY8_PIPE_IMG)" \
	--name "$(BSY8_PIPE_NAME)" --out "$(BUILD)" --timeout-ms 25000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY8_PIPE_REPORT)" || true
@cat "$(BSY8_PIPE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(BSY8_PIPE_REPORT)"; then \
	printf '!!! test-bsy8-pipe FAIL: TRIPLE FAULT -- the pipe crashed (root-cause the temp create/dup2/JFT-inherit path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-bsy8-pipe [1/6]: no triple-fault\n'
@if [ ! -s "$(BSY8_PIPE_SERIAL)" ]; then \
	printf '!!! test-bsy8-pipe FAIL: no serial captured at %s\n' "$(BSY8_PIPE_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(BSY8_PIPE_SERIAL)" \
	|| { printf '!!! test-bsy8-pipe FAIL: SHELL-READY missing -- the REPL was never entered (the pipe may have hung; check the consumer stdin path)\n'; exit 1; }
@printf '>>> test-bsy8-pipe [2/6]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY8_PIPE_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY8_PIPE_NAME).repl"
@bcnt=$$(grep -oF 'GOBBLE-BEGIN' "$(BUILD)/$(BSY8_PIPE_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$bcnt" != "1" ]; then \
	printf '!!! test-bsy8-pipe FAIL: GOBBLE-BEGIN seen %s time(s), expected EXACTLY 1.  count==0 means the consumer stage of the pipe never ran (root-cause run_pipeline stage dispatch, Rule 3)\n' "$$bcnt"; \
	exit 1; \
fi
@printf '>>> test-bsy8-pipe [3/6]: GOBBLE-BEGIN appears EXACTLY once (the consumer stage ran)\n'
@gcnt=$$(grep -oF 'GREETINGS' "$(BUILD)/$(BSY8_PIPE_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" != "1" ]; then \
	printf '!!! test-bsy8-pipe FAIL: GREETINGS seen %s time(s), expected EXACTLY 1 (echoed once from the redirected stdin via the temp pipe).  count==0 means the pipe did NOT feed GREET`s stdout into GOBBLE`s stdin (root-cause the temp create + dup2(temp,1) on the producer OR dup2(temp,0) on the consumer, Rule 3); count>1 means GREET also leaked to screen (producer stdout not repointed at the temp)\n' "$$gcnt"; \
	exit 1; \
fi
@printf '>>> test-bsy8-pipe [4/6]: GREETINGS appears EXACTLY once -- `GREET.COM | GOBBLE.COM` piped GREET`s stdout through the temp into GOBBLE`s stdin (the temp-file pipe works)\n'
@grep -qF 'File not found' "$(BUILD)/$(BSY8_PIPE_NAME).repl" \
	|| { printf '!!! test-bsy8-pipe FAIL: "File not found" missing -- `TYPE PIPE1.$$$$$$` did NOT report the temp gone; the pipe intermediate was not deleted after the pipeline (root-cause the run_pipeline cleanup/dos_unlink path, Rule 3)\n'; exit 1; }
@printf '>>> test-bsy8-pipe [5/6]: `TYPE PIPE1.$$$$$$` printed "File not found" -- the pipe temp was DELETED after the pipeline (authentic DOS 3.3 cleanup; MS-DOS 3.3 Tech Ref Ch.6)\n'
@grep -q '^SHELL-EXIT$$' "$(BSY8_PIPE_SERIAL)" \
	|| { printf '!!! test-bsy8-pipe FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run (handle 0/1 may not have been restored to CON after the pipeline, or a refcount over-free panicked)\n'; exit 1; }
@printf '>>> test-bsy8-pipe [6/6]: post-pipe `exit` reached the REPL + halted cleanly (handles restored to CON; no SFT over-free)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- `GREET.COM | GOBBLE.COM` really pipes via a DOS temp file on the 386\n'
@printf '            (temp created, fed, and deleted; QEMU only, tri-emulator pending initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-bsy8-pipe: green (oracle completed)\n'
```

## test-bsy8-pipe-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 31506):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_nostdin.img build/greet_program.bin build/gobble_program.bin harness/diff/fat_diff/fixtures/autoexec_bsy8.bat build/.makefile.stamp`
```make
@printf '>>> test-bsy8-pipe-mutant: confirming the NO-STDIN (consumer stdin not repointed) mutant goes RED (Rule 6)\n'
$(call bsy8-pipe-mint-disk,$(BSY8_PIPE_MUT_IMG))
@$(call fixture-preflight,$(BSY8_PIPE_MUT_IMG))
@$(HARNESS_BIN) --disk "$(BSY8_PIPE_TRACER_MUT_IMG)" --disk2 "$(BSY8_PIPE_MUT_IMG)" \
	--name "$(BSY8_PIPE_MUT_NAME)" --out "$(BUILD)" --timeout-ms 20000 \
	--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(BSY8_PIPE_MUT_REPORT)" || true
@if [ ! -s "$(BSY8_PIPE_MUT_SERIAL)" ]; then \
	printf '!!! test-bsy8-pipe-mutant FAIL: no serial from the mutant boot\n'; exit 1; \
fi
@sed -n '/^SHELL-READY$$/,$$p' "$(BSY8_PIPE_MUT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(BSY8_PIPE_MUT_NAME).repl"
@gcnt=$$(grep -oF 'GREETINGS' "$(BUILD)/$(BSY8_PIPE_MUT_NAME).repl" | wc -l | tr -d ' '); \
if [ "$$gcnt" = "1" ]; then \
	printf '!!! test-bsy8-pipe-mutant FAIL: GREETINGS seen 1x under CMD_MUTATE_PIPE_NO_STDIN -- the pipe would pass even with the consumer stdin repoint dead; the emu gate is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-bsy8-pipe-mutant: green (no-stdin mutant correctly RED -- GREETINGS count != 1, the consumer stdin repoint is load-bearing for the pipe)\n'
```

## test-cmdline-fuzz

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8419):
Prerequisites: `build/test_cmdline_fuzz build/.makefile.stamp`
```make
@printf ">>> test-cmdline-fuzz: cmd-line tokenizer + PSP cmd_tail generative fuzzer over seeds %s..%s (no overflow / clamp to 126/127)\n" "$(CMDLINE_FUZZ_SEEDS_LO)" "$(CMDLINE_FUZZ_SEEDS_HI)"
@$(TEST_CMDLINE_FUZZ) --sweep $(CMDLINE_FUZZ_SEEDS_LO) $(CMDLINE_FUZZ_SEEDS_HI) --ops $(CMDLINE_FUZZ_OPS) \
	|| { printf '!!! test-cmdline-fuzz FAIL: a seed broke an invariant (replay above) -- root-cause it (Rule 3)\n'; exit 1; }
@printf ">>> test-cmdline-fuzz: green\n"
```

## test-cmdline-fuzz-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8426):
Prerequisites: `build/test_cmdline_fuzz_mutant_noclamp build/.makefile.stamp`
```make
@printf ">>> test-cmdline-fuzz-mutant: confirming the fuzzer FINDS the no-cmd_tail-clamp mutant (Rule 6)\n"
@if $(TEST_CMDLINE_FUZZ_MUT) --sweep $(CMDLINE_FUZZ_SEEDS_LO) $(CMDLINE_FUZZ_SEEDS_HI) --ops $(CMDLINE_FUZZ_OPS) >/dev/null 2>&1; then \
	printf '!!! test-cmdline-fuzz-mutant FAIL: no-clamp mutant PASSED the fuzzer -- the fuzzer is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-cmdline-fuzz-mutant: green (fuzzer correctly RED on the no-cmd_tail-clamp mutant -- the fuzzer bites)\n'; \
fi
```

## test-command

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7250):
Prerequisites: `build/test_command build/.makefile.stamp`
```make
@printf ">>> test-command: parse/upcase + built-in classify + .COM-append + DIR-line format\n"
@$(TEST_COMMAND)
@printf ">>> test-command: green\n"
```

## test-command-c-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 17):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/command_c.sh $(SECONDARY_TRACER_IMG)
```

## test-command-c-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-command-c-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-command-c-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/command-c-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/command-c-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "command-c-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/command-c-emu_bochs.report.txt" || true
@cat "$(BUILD)/command-c-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-command-c-emu-bochs "$(BUILD)/command-c-emu_bochs.report.txt" "$(BUILD)/command-c-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-command-c-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 59):
Prerequisites: `build/qemu_harness build/tracer_mut_safety_command_c.img build/tracer_mut_safety_command_env.img build/.makefile.stamp`
```make
@set -eu; for m in c env; do \
 if sh $(SHELL_FILES_DIR)/command_c.sh $(BUILD)/tracer_mut_safety_command_$$m.img $$m >$(BUILD)/command_c_mut_$$m.log 2>&1; then echo "FAIL COMMAND_$$m survived";exit 1;fi; \
 grep '^RED command-c:' $(BUILD)/command_c_mut_$$m.log; \
 echo "GREEN test-command-c-emu-mutant: COMMAND_$$m caught RED";done
```

## test-command-file

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/command_file.mk', line 10):
Prerequisites: `build/command_file build/flair_tenants.img build/flair_tenants_interactive.img build/flair_tenants_recorddbl_exitdos.img build/flair_tenants_mut_record.img build/.makefile.stamp`
```make
@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_IMG) $(BUILD)/transient_flairtenants.bin $(BUILD) shipped
@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_INTERACTIVE_IMG) $(BUILD)/transient_flairtenants_int.bin $(BUILD) interactive
@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_RECORDDBL_EXITDOS_IMG) $(BUILD)/transient_recorddbl.bin $(BUILD) clip
@sh $(COMMAND_FILE_DIR)/command_file.sh $(BUILD)/flair_tenants_mut_record.img $(BUILD)/transient_kernel_flairtenants_mut_record.bin $(BUILD) desktop-family
```

## test-command-file-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/command_file.mk', line 51):
Prerequisites: `build/qemu_harness build/command_file build/command-file/fragment.img build/flair_data.img build/ppm_text_check build/.makefile.stamp`
```make
@sh $(COMMAND_FILE_DIR)/command_file.sh $(COMMAND_FILE_IMG)/fragment.img $(BUILD)/transient_flairtenants.bin $(BUILD) fragmented
$(call exitdos-boot,$(COMMAND_FILE_IMG)/fragment.img,command_file_fragment,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
@( $(call exitdos-exit-check,command_file_fragment) )
@echo 'GREEN test-command-file-emu: fragmented COMMAND.COM -> transient -> banner -> SHELL-READY -> injected DIR; text console'
```

## test-command-file-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 138):
Prerequisites: `build/bochs_harness build/command-file/fragment.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-command-file-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/command-file/fragment.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-command-file-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/command-file-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/command-file/fragment.img" --disk2 "$(BUILD)/command-file-emu_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "command-file-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/command-file-emu_bochs.report.txt" || true
@cat "$(BUILD)/command-file-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-command-file-emu-bochs "$(BUILD)/command-file-emu_bochs.report.txt" "$(BUILD)/command-file-emu_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-command-file-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/command_file.mk', line 73):
Prerequisites: `build/qemu_harness build/ppm_text_check build/flair_data.img build/command-file/command_volume.img build/command-file/command_magic.img build/command-file/command_message.img build/.makefile.stamp`
```make
$(call exitdos-boot,$(COMMAND_FILE_IMG)/command_volume.img,command_file_mut_volume,$(FLAIR_DATA_IMG),$(COMMAND_FILE_TRACE),HALTED,0,40000)
@out=$$( ( $(call exitdos-exit-check,command_file_mut_volume) ) 2>&1 ); rc=$$?; \
 test $$rc -ne 0 && echo "$$out" | grep -q 'did not run alert -> put away' || { echo "FAIL COMMAND_VOLUME: $$out"; exit 1; }; \
 grep -q '^PANIC exit-to-dos: COMMAND.COM not found' $(BUILD)/command_file_mut_volume.serial; \
 echo "RED command-file COMMAND_VOLUME: $$out"; echo 'GREEN test-command-file-emu-mutant COMMAND_VOLUME rejected'
@set -eu; img=$(COMMAND_FILE_IMG)/run-command_magic.img; cp -f $(COMMAND_FILE_IMG)/command_magic.img $$img; \
 cp -f $(BUILD)/transient_command_magic.bin $(COMMAND_FILE_IMG)/skip-magic.bin; \
 printf X | dd of=$(COMMAND_FILE_IMG)/skip-magic.bin bs=1 conv=notrunc status=none; \
 touch -d '@315532800' $(COMMAND_FILE_IMG)/skip-magic.bin; \
 TZ=UTC mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/skip-magic.bin ::COMMAND.COM; \
 if sh harness/emu/command_file.sh $(BUILD) $$img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' skip-magic 'no COMMAND3 transient' >$(COMMAND_FILE_IMG)/skip-magic.log 2>&1; then echo 'FAIL COMMAND_MAGIC survived'; exit 1; fi; \
 grep '^RED command-file-emu skip-magic missing exact MSG-DOS-0014' $(COMMAND_FILE_IMG)/skip-magic.log; \
 grep -qx SHELL-READY $(COMMAND_FILE_IMG)/skip-magic.serial; echo 'GREEN test-command-file-emu-mutant COMMAND_MAGIC rejected'
@set -eu; img=$(COMMAND_FILE_IMG)/run-command_message.img; cp -f $(COMMAND_FILE_IMG)/command_message.img $$img; mdel -i $$img@@32256 ::COMMAND.COM; \
 if sh harness/emu/command_file.sh $(BUILD) $$img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' wrong-message 'COMMAND.COM not found' >$(COMMAND_FILE_IMG)/wrong-message.log 2>&1; then echo 'FAIL COMMAND_MESSAGE survived'; exit 1; fi; \
 grep '^RED command-file-emu wrong-message missing exact MSG-DOS-0014' $(COMMAND_FILE_IMG)/wrong-message.log; \
 grep -qx HALTED $(COMMAND_FILE_IMG)/wrong-message.serial; echo 'GREEN test-command-file-emu-mutant COMMAND_MESSAGE rejected'
```

## test-command-file-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/command_file.mk', line 17):
Prerequisites: `test-command-file`
```make
@set -eu; for m in NAME ATTR BYTES GEOMETRY DATE; do \
 img=$(COMMAND_FILE_IMG)/host-$$m.img; cp -f $(FLAIRTENANTS_IMG) $$img; \
 case $$m in \
 NAME) mdel -i $$img@@32256 ::COMMAND.COM;; \
 ATTR) mattrib -i $$img@@32256 +h ::COMMAND.COM;; \
 BYTES) cp -f $(BUILD)/transient_flairtenants.bin $(COMMAND_FILE_IMG)/bad.bin; \
 printf X | dd of=$(COMMAND_FILE_IMG)/bad.bin bs=1 conv=notrunc status=none; \
 touch -d '@315532800' $(COMMAND_FILE_IMG)/bad.bin; \
 mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/bad.bin ::COMMAND.COM;; \
 GEOMETRY) truncate -s 1572864 $$img;; \
 DATE) cp -f $(BUILD)/transient_flairtenants.bin $(COMMAND_FILE_IMG)/bad.bin; \
 touch -d '@315619200' $(COMMAND_FILE_IMG)/bad.bin; mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/bad.bin ::COMMAND.COM;; esac; \
 if $(BUILD)/command_file $$img $(BUILD)/transient_flairtenants.bin check >$(COMMAND_FILE_IMG)/host-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 grep "^RED command-file $$m " $(COMMAND_FILE_IMG)/host-$$m.log; \
 echo "GREEN test-command-file-mutant $$m rejected"; done
```

## test-command-file-negative

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/command_file.mk', line 57):
Prerequisites: `build/qemu_harness build/command-file/missing.img build/command-file/magic.img build/flair_data.img build/.makefile.stamp`
```make
@sh harness/emu/command_file.sh $(BUILD) $(COMMAND_FILE_IMG)/missing.img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' missing 'COMMAND.COM not found'
@sh harness/emu/command_file.sh $(BUILD) $(COMMAND_FILE_IMG)/magic.img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' magic 'no COMMAND3 transient'
```

## test-command-file-negative-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 139):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-command-file-negative-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-command-file-negative-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/command-file-negative_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/command-file-negative_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "command-file-negative_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/command-file-negative_bochs.report.txt" || true
@cat "$(BUILD)/command-file-negative_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-command-file-negative-bochs "$(BUILD)/command-file-negative_bochs.report.txt" "$(BUILD)/command-file-negative_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-command-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7256):
Prerequisites: `build/test_command_mutant_noupcase build/test_command_mutant_com build/test_command_mutant_badcmd build/test_command_mutant_nomdrd build/test_command_mutant_noset build/test_command_mutant_nocopy build/test_command_mutant_nodel build/test_command_mutant_noren build/test_command_mutant_nodate build/test_command_mutant_notime build/test_command_mutant_noprompt build/test_command_mutant_nopath build/test_command_mutant_nosamefile build/.makefile.stamp`
```make
@printf ">>> test-command-mutant: confirming all mutants go RED (Rule 6)\n"
@if $(TEST_COMMAND_MUT_NOUP) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-upcase mutant PASSED -- the parse/upcase test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-upcase mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_COM) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: com-always mutant PASSED -- the .COM-append test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (com-always mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_BADCMD) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: badcmd-builtin mutant PASSED -- the classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (badcmd-builtin mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOMDRD) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-mdrd mutant PASSED -- the MD/RD classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-mdrd mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOSET) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-set mutant PASSED -- the SET classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-set mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOCOPY) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-copy mutant PASSED -- the COPY classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-copy mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_NODEL) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-del mutant PASSED -- the DEL/ERASE classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-del mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOREN) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-ren mutant PASSED -- the REN/RENAME classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-ren mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_NODATE) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-date mutant PASSED -- the DATE classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-date mutant correctly RED)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOTIME) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-time mutant PASSED -- the TIME classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-time mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOPROMPT) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-prompt mutant PASSED -- the PROMPT classify test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-prompt mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOPATH) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-path mutant PASSED -- the PATH-dir candidate test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-path mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_COMMAND_MUT_NOSAMEFILE) >/dev/null 2>&1; then \
	printf '!!! test-command-mutant FAIL: no-samefile mutant PASSED -- the COPY same-file guard test is decoration (initech-ojxn data-loss regression uncaught)\n'; \
	exit 1; \
else \
	printf '>>> test-command-mutant: green (no-samefile mutant correctly RED -- the COPY onto-itself guard bites)\n'; \
fi
```

## test-config-fuzz

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8358):
Prerequisites: `build/test_config_sys_fuzz build/.makefile.stamp`
```make
@printf ">>> test-config-fuzz: CONFIG.SYS parser generative fuzzer over seeds %s..%s (no overflow / no crash / clamp / lenient)\n" "$(CONFIG_FUZZ_SEEDS_LO)" "$(CONFIG_FUZZ_SEEDS_HI)"
@$(TEST_CONFIG_FUZZ) --sweep $(CONFIG_FUZZ_SEEDS_LO) $(CONFIG_FUZZ_SEEDS_HI) --ops $(CONFIG_FUZZ_OPS) \
	|| { printf '!!! test-config-fuzz FAIL: a seed broke an invariant (replay above) -- root-cause it (Rule 3)\n'; exit 1; }
@printf ">>> test-config-fuzz: green\n"
```

## test-config-fuzz-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8367):
Prerequisites: `build/test_config_sys_fuzz_mutant_noguard build/test_config_sys_fuzz_mutant_offbyone build/.makefile.stamp`
```make
@printf ">>> test-config-fuzz-mutant: confirming the fuzzer FINDS both overflow-guard mutants (Rule 6)\n"
@if $(TEST_CONFIG_FUZZ_MUT) --sweep $(CONFIG_FUZZ_SEEDS_LO) $(CONFIG_FUZZ_SEEDS_HI) --ops $(CONFIG_FUZZ_OPS) >/dev/null 2>&1; then \
	printf '!!! test-config-fuzz-mutant FAIL: no-guard mutant PASSED the fuzzer -- the fuzzer is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-config-fuzz-mutant: green (fuzzer correctly RED on the no-overflow-guard mutant)\n'; \
fi
@if $(TEST_CONFIG_FUZZ_MUT_OFF) --sweep $(CONFIG_FUZZ_SEEDS_LO) $(CONFIG_FUZZ_SEEDS_HI) --ops $(CONFIG_FUZZ_OPS) >/dev/null 2>&1; then \
	printf '!!! test-config-fuzz-mutant FAIL: off-by-one mutant PASSED the fuzzer -- the 2^32-family saturation test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-config-fuzz-mutant: green (fuzzer correctly RED on the off-by-one mutant -- the corrected saturation bites)\n'; \
fi
```

## test-config-sys

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8297):
Prerequisites: `build/test_config_sys build/.makefile.stamp`
```make
@printf ">>> test-config-sys: CONFIG.SYS parser vs the LOCKED baseline + edge cases (initech-509.2)\n"
@$(TEST_CONFIG_SYS)
@printf ">>> test-config-sys: green\n"
```

## test-config-sys-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8303):
Prerequisites: `build/test_config_sys_mutant_off build/test_config_sys_mutant_unk build/.makefile.stamp`
```make
@printf ">>> test-config-sys-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_CONFIG_SYS_MUT_OFF) >/dev/null 2>&1; then \
	printf '!!! test-config-sys-mutant FAIL: FILES off-by-one mutant PASSED -- the FILES assertion is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-config-sys-mutant: green (FILES off-by-one mutant correctly RED)\n'; \
fi
@if $(TEST_CONFIG_SYS_MUT_UNK) >/dev/null 2>&1; then \
	printf '!!! test-config-sys-mutant FAIL: fail-on-unknown mutant PASSED -- the leniency assertions are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-config-sys-mutant: green (fail-on-unknown mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-conin

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 28103):
Prerequisites: `build/qemu_harness build/conin_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-conin : INT 21h CON INPUT end-to-end\n'
@printf '  Ref: DOS 3.3 PRM AH=0Ah buffered input. beads initech-n62. Rule 2/5.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (CON-input self-test kernel), injecting --keys "d,i,r,ret"\n' "$(CONIN_IMG)"
@printf 'Expecting : CONIN-LIVE + CONIN-PROG-READY + "CONIN-LINE=dir" + triple_fault=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(CONIN_IMG)" \
	--name "$(CONIN_NAME)" --out "$(BUILD)" --timeout-ms 9000 \
	--keys "d,i,r,ret" --keys-after "CONIN-PROG-READY" \
	--quit-after "CONIN-LINE=dir" \
	2> "$(CONIN_REPORT)" || true
@cat "$(CONIN_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if [ ! -s "$(CONIN_SERIAL)" ]; then \
	printf '!!! test-conin FAIL: no serial captured at %s\n' "$(CONIN_SERIAL)"; \
	exit 1; \
fi
@grep -q '^CONIN-LIVE$$' "$(CONIN_SERIAL)" \
	|| { printf '!!! test-conin FAIL: CONIN-LIVE missing -- the keyboard input source never bound\n'; exit 1; }
@grep -q '^CONIN-PROG-READY$$' "$(CONIN_SERIAL)" \
	|| { printf '!!! test-conin FAIL: CONIN-PROG-READY missing -- the CON-input program never started\n'; exit 1; }
@printf '>>> test-conin [1/3]: CONIN-LIVE + CONIN-PROG-READY (input source bound, program running)\n'
@# The program writes "CONIN-LINE=dir" back through INT 21h AH=40h after the
@# AH=0Ah buffered read of the injected "dir" + Enter. The line ends in CRLF
@# (the program's trailing CRLF), so strip CR before matching the whole line.
@tr -d '\r' < "$(CONIN_SERIAL)" | grep -q '^CONIN-LINE=dir$$' \
	|| { printf '!!! test-conin FAIL: "CONIN-LINE=dir" missing -- AH=0Ah did not read the injected line back via INT 21h (root-cause the CON-input path, Rule 3)\n'; exit 1; }
@printf '>>> test-conin [2/3]: AH=0Ah read the injected line; "CONIN-LINE=dir" echoed back through INT 21h\n'
@if grep -q 'triple_fault=1' "$(CONIN_REPORT)"; then \
	printf '!!! test-conin FAIL: TRIPLE FAULT during the CON-input run\n'; \
	exit 1; \
fi
@grep -q 'keys_sent=4' "$(CONIN_REPORT)" \
	|| { printf '!!! test-conin FAIL: harness did not report keys_sent=4 (d,i,r,ret)\n'; exit 1; }
@printf '>>> test-conin [3/3]: triple_fault=0 + keys_sent=4 (clean run, line injected)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a real program read a line from the keyboard via INT 21h AH=0Ah\n'
@printf '======================================================================\n'
```

## test-conin-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5679):
Prerequisites: `build/test_conin_mutant_noecho build/test_conin_mutant_count build/test_conin_mutant_crwrap build/test_conin_mutant_discard_ring_only build/.makefile.stamp`
```make
@if $(TEST_CONIN_MUT_DISCARD) > $(BUILD)/test_conin_mutant_discard.log 2>&1; then \
	printf '!!! test-conin-mutant FAIL: INT21_MUTATE_DISCARD_RING_ONLY PASSED -- the hotkey-drop test is decoration\n'; exit 1; fi
@grep -q 'FAIL.*discard_through' $(BUILD)/test_conin_mutant_discard.log || { cat $(BUILD)/test_conin_mutant_discard.log; exit 1; }
@printf '>>> test-conin-mutant: green (INT21_MUTATE_DISCARD_RING_ONLY correctly RED: %s)\n' "$$(grep -c 'FAIL.*discard_through' $(BUILD)/test_conin_mutant_discard.log) discard checks"
@printf ">>> test-conin-mutant: confirming all three mutants go RED (Rule 6)\n"
@if $(TEST_CONIN_MUT_NOECHO) >/dev/null 2>&1; then \
	printf '!!! test-conin-mutant FAIL: 01h-no-echo mutant PASSED -- the echo test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-conin-mutant: green (01h-no-echo mutant correctly RED)\n'; \
fi
@if $(TEST_CONIN_MUT_COUNT) >/dev/null 2>&1; then \
	printf '!!! test-conin-mutant FAIL: 0Ah-count-CR mutant PASSED -- the buffered-input count test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-conin-mutant: green (0Ah-count-CR mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_CONIN_MUT_WRAP) >/dev/null 2>&1; then \
	printf '!!! test-conin-mutant FAIL: 0Ah-CR-wrap mutant PASSED -- the max=255 CR-store test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-conin-mutant: green (0Ah-CR-wrap mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-conin-unit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5674):
Prerequisites: `build/test_conin build/.makefile.stamp`
```make
@printf ">>> test-conin-unit: CON input 01h/06h/07h/08h/0Ah(+BS,+clamp)/0Bh/0Ch via mock source\n"
@$(TEST_CONIN)
@printf ">>> test-conin-unit: green\n"
```

## test-conkeys

Declared tier: EMU; #  recipe to execute (from 'harness/conkeys/conkeys.mk', line 27):
Prerequisites: `build/conkeys_program.bin build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/conkeys.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(CONKEYS_PROG)
```

## test-conkeys-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 221):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-conkeys-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-conkeys-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/conkeys_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/conkeys_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "conkeys_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/conkeys_bochs.report.txt" || true
@cat "$(BUILD)/conkeys_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-conkeys-bochs "$(BUILD)/conkeys_bochs.report.txt" "$(BUILD)/conkeys_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-conkeys-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/conkeys/conkeys.mk', line 40):
Prerequisites: `build/conkeys_program.bin build/qemu_harness build/conkeys_mut.img build/.makefile.stamp`
```make
@if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/conkeys.sh $(BUILD) $(BUILD)/conkeys_mut.img $(CONKEYS_PROG) >$(BUILD)/conkeys-emu-mutant.log 2>&1; then \
 echo "FAIL CONKEYS emulator F3 mutant survived"; exit 1; fi
@grep "RED CONKEYS template-lines" $(BUILD)/conkeys-emu-mutant.log
@printf "GREEN test-conkeys-emu-mutant: F3 kernel mutant rejected\n"
```

## test-conkeys-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/conkeys/conkeys.mk', line 18):
Prerequisites: `build/test_conkeys_F1 build/test_conkeys_F2 build/test_conkeys_F3 build/test_conkeys_F4 build/test_conkeys_F5 build/test_conkeys_ESC build/test_conkeys_INS build/test_conkeys_DEL build/test_conkeys_PAUSE build/test_conkeys_PRN build/.makefile.stamp`
```make
@set -eu; for m in $(CONKEYS_MUTANTS); do \
 case $$m in F1) why='F1';; F2) why='F2 excludes target';; F3) why='F3 rest';; \
 F4) why='F4 excludes target';; F5) why='F5 replaces template';; ESC) why='Esc retains template';; \
 INS) why='Ins toggles';; DEL) why='Del skips template';; PAUSE) why='Ctrl-S consumes resume';; \
 PRN) why='Ctrl-P echoes PRN; Ctrl-N ends echo';; esac; \
 log=$(BUILD)/conkeys-$$m.log; \
 if $(BUILD)/test_conkeys_$$m >$$log 2>&1; then echo "FAIL CONKEYS $$m survived"; exit 1; fi; \
 grep -Fx "RED CONKEYS $$why" $$log; printf 'GREEN test-conkeys-mutant: %s rejected\n' $$m; done
```

## test-conkeys-unit

Declared tier: HOST; #  recipe to execute (from 'harness/conkeys/conkeys.mk', line 16):
Prerequisites: `build/test_conkeys build/.makefile.stamp`
```make
@$(CONKEYS_HOST)
```

## test-console

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5244):
Prerequisites: `build/test_console build/.makefile.stamp`
```make
@printf ">>> test-console: 8x16 glyph blit (MSB-left, bpp 32/24) + cursor/wrap/scroll\n"
@$(TEST_CONSOLE)
@printf ">>> test-console: green\n"
```

## test-console-cursor25-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5253):
Prerequisites: `build/test_console_cursor25_mutant build/.makefile.stamp`
```make
@set +e; $< > $(BUILD)/console-cursor25-mutant.log 2>&1; rc=$$?; set -e; \
 test $$rc = 1 && grep -q 'FAIL.*BIOS off-grid cursor retained' $(BUILD)/console-cursor25-mutant.log || { cat $(BUILD)/console-cursor25-mutant.log; exit 1; }; \
 printf 'CONSOLE_MUTATE_OFFGRID_CLAMP RED: '; grep -m1 'FAIL.*BIOS off-grid cursor retained' $(BUILD)/console-cursor25-mutant.log
@printf '>>> test-console-cursor25-mutant: green (off-grid clamp RED)\n'
```

## test-console-look

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 19):
Prerequisites: `build/test_console_look build/.makefile.stamp`
```make
@$(BUILD)/test_console_look
```

## test-d27i

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16439):
Prerequisites: `build/test_fat16_window build/fat16_fixture.img build/fat12_fixture.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-d27i : WINDOWED FAT-sector read\n'
@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-d27i.\n'
@printf '  FAT16 + FAT12 read through a 2-sector FAT window (no whole-FAT slurp).\n'
@printf '======================================================================\n'
@printf '>>> test-d27i: FAT16 windowed read + FAT12 straddle byte-identical\n'
@$(TEST_D27I) "$(FAT16_IMG)" "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" \
	|| { printf '!!! test-d27i FAIL: windowed FAT-sector read oracle red\n'; exit 1; }
@printf '>>> test-d27i: green (FAT16 mounts + reads windowed; FAT12 straddle byte-identical)\n'
```

## test-d27i-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16453):
Prerequisites: `build/test_fat16_window_mut_secoff build/test_fat16_window_mut_straddle build/test_fat16_window_mut_nowindow build/fat16_fixture.img build/fat12_fixture.img build/.makefile.stamp`
```make
@printf '>>> test-d27i-mutant: confirming the THREE windowed-read mutants go RED (Rule 6; beads initech-d27i)\n'
@for m in SECOFF:$(TEST_D27I_MUT_SECOFF) STRADDLE:$(TEST_D27I_MUT_STRADDLE) NOWINDOW:$(TEST_D27I_MUT_NOWINDOW); do \
	name=$${m%%:*}; bin=$${m#*:}; \
	"$$bin" "$(FAT16_IMG)" "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" 2>/dev/null | grep -q 'checks,' \
		|| { printf '!!! test-d27i-mutant FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if "$$bin" "$(FAT16_IMG)" "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
		printf '!!! test-d27i-mutant FAIL: %s PASSED -- the windowed-read oracle is decoration\n' "$$name"; exit 1; \
	else \
		printf '>>> test-d27i-mutant: green (%s correctly RED -- its windowed-read primitive bites)\n' "$$name"; \
	fi; \
done
@printf '>>> test-d27i-mutant: green (ALL THREE windowed-read mutants ran + RED for the right reason)\n'
```

## test-datetime

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26232):
Prerequisites: `build/qemu_harness build/datetime_boot.img build/fat_multiopen.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-datetime : resident clock + free-space + PSP\n'
@printf '  Ref: beads initech-yv9; MC146818 RTC (0x70/0x71); INT 21h 2Ah/2Ch/36h/62h.\n'
@printf '  PINNED RTC base=%s (deterministic, Rule 11).\n' "$(DATETIME_RTC_BASE)"
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (primary slave), -rtc base=%s\n' "$(DATETIME_IMG)" "$(FAT_MULTIOPEN_IMG)" "$(DATETIME_RTC_BASE)"
@printf 'Expecting : DT-YEAR=2026 MON=6 DAY=9 DOW=2(Tue) HOUR=12 MIN=34 SEC=56 + FREE>0 + PSP>0 + rc=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_MULTIOPEN_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_MULTIOPEN_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_MULTIOPEN_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_MULTIOPEN_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_MULTIOPEN_IMG))),:)
@$(HARNESS_BIN) --disk "$(DATETIME_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--rtc-base "$(DATETIME_RTC_BASE)" --rtc-deterministic \
	--name "$(DATETIME_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(DATETIME_REPORT)" || true
@cat "$(DATETIME_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(DATETIME_REPORT)"; then \
	printf '!!! test-datetime FAIL: TRIPLE FAULT (root-cause the clock/query path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-datetime [1/5]: no triple-fault\n'
@if [ ! -s "$(DATETIME_SERIAL)" ]; then \
	printf '!!! test-datetime FAIL: no serial captured at %s\n' "$(DATETIME_SERIAL)"; exit 1; \
fi
@sed -n '/^DATETIME-OUTPUT-BEGIN$$/,/^DATETIME-OUTPUT-END$$/p' "$(DATETIME_SERIAL)" | tr -d '\r' > "$(BUILD)/$(DATETIME_NAME).out"
@for kv in DT-YEAR=2026 DT-MON=6 DT-DAY=9 DT-DOW=2; do \
	grep -qxF "$$kv" "$(BUILD)/$(DATETIME_NAME).out" \
	|| { printf '!!! test-datetime FAIL: %s missing -- AH=2Ah GET DATE did not return the PINNED date (2026-06-09) / Tuesday (DOW=2)\n' "$$kv"; cat "$(BUILD)/$(DATETIME_NAME).out"; exit 1; }; \
done
@printf '>>> test-datetime [2/5]: AH=2Ah GET DATE == 2026-06-09, DOW=2 (Tuesday)\n'
@for kv in DT-HOUR=12 DT-MIN=34 DT-SEC=56; do \
	grep -qxF "$$kv" "$(BUILD)/$(DATETIME_NAME).out" \
	|| { printf '!!! test-datetime FAIL: %s missing -- AH=2Ch GET TIME did not return the PINNED time (12:34:56)\n' "$$kv"; cat "$(BUILD)/$(DATETIME_NAME).out"; exit 1; }; \
done
@printf '>>> test-datetime [3/5]: AH=2Ch GET TIME == 12:34:56\n'
@grep -qxF 'DT-BPS=512' "$(BUILD)/$(DATETIME_NAME).out" \
	|| { printf '!!! test-datetime FAIL: DT-BPS=512 missing -- AH=36h did not report 512 bytes/sector\n'; cat "$(BUILD)/$(DATETIME_NAME).out"; exit 1; }
@free=$$(grep -m1 -oE '^DT-FREE=[0-9]+$$' "$(BUILD)/$(DATETIME_NAME).out" | cut -d= -f2); \
if [ -z "$$free" ] || [ "$$free" -le 0 ]; then \
	printf '!!! test-datetime FAIL: AH=36h DT-FREE not > 0 (got "%s") -- no mounted volume / no room\n' "$$free"; cat "$(BUILD)/$(DATETIME_NAME).out"; exit 1; \
fi
@printf '>>> test-datetime [4/5]: AH=36h GET DISK FREE SPACE (bps=512, free clusters > 0)\n'
@psp=$$(grep -m1 -oE '^DT-PSP=[0-9]+$$' "$(BUILD)/$(DATETIME_NAME).out" | cut -d= -f2); \
if [ -z "$$psp" ] || [ "$$psp" -le 0 ]; then \
	printf '!!! test-datetime FAIL: AH=62h DT-PSP not nonzero (got "%s")\n' "$$psp"; cat "$(BUILD)/$(DATETIME_NAME).out"; exit 1; \
fi; \
grep -q '^DATETIME-EXIT rc=0$$' "$(DATETIME_SERIAL)" \
	|| { printf '!!! test-datetime FAIL: DATETIME-EXIT rc=0 missing -- the program did not finish cleanly\n'; cat "$(DATETIME_SERIAL)"; exit 1; }
@printf '>>> test-datetime [5/5]: AH=62h GET PSP nonzero + DATETIME-EXIT rc=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS read the PINNED RTC (2026-06-09 12:34:56, Tuesday),\n'
@printf '            reported real free space (bps=512, free>0) + the current PSP (QEMU)\n'
@printf '======================================================================\n'
```

## test-del-directory-b023

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 66):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/del_b023.sh $(SECONDARY_TRACER_IMG)
```

## test-del-directory-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 6):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/dos_safety_fixture build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/del_directory.sh emu $(SECONDARY_TRACER_IMG)
```

## test-del-directory-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 49):
Prerequisites: `build/qemu_harness build/dos_safety_fixture build/tracer_mut_safety_del_directory.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/del_directory.sh emu $(BUILD)/tracer_mut_safety_del_directory.img raw >$(BUILD)/del_directory_emu_mut.log 2>&1; then echo 'RED directory UNLINK mutant survived QEMU';exit 1;fi
@grep '^RED del-directory: directory confirmation missing' $(BUILD)/del_directory_emu_mut.log
@echo 'GREEN del-directory-emu-mutant: raw directory UNLINK caught'
```

## test-del-directory-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 4):
Prerequisites: `build/shell_files build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/del_directory.sh host $<
```

## test-del-directory-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doscmd_round2.mk', line 39):
Prerequisites: `build/shell_files_mut_CMD_MUTATE_DEL_DIRECTORY_UNLINK build/shell_files_mut_CMD_MUTATE_DEL_NO_CONFIRM build/shell_files_mut_CMD_MUTATE_DEL_DIRECTORY_RECURSE build/.makefile.stamp`
```make
@set -eu; for m in $(DEL_DIRECTORY_MUTANTS); do \
 if sh harness/diff/fat_diff/del_directory.sh host $(BUILD)/shell_files_mut_$$m $$m > $(BUILD)/del_directory_mut_$$m.log 2>&1; then echo "RED DEL mutant survived: $$m";exit 1;fi; \
 grep '^RED del-directory:' $(BUILD)/del_directory_mut_$$m.log; done
@echo 'GREEN del-directory-mutant: raw UNLINK / no confirmation / nested recursion each caught'
```

## test-desktop-db

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4130):
Prerequisites: `build/test_desktop_db build/.makefile.stamp`
```make
@dd if=/dev/zero of=$(DESKTOP_DB_HOST_IMG) bs=512 count=2880 status=none
@SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mformat -i $(DESKTOP_DB_HOST_IMG) \
	-f 1440 -N 0x494e4954 ::
@$(TEST_DESKTOP_DB) "$(DESKTOP_DB_HOST_IMG)"
@printf ">>> test-desktop-db: green (create/read/corrupt-detect/regen + TRASH)\n"
```

## test-desktop-db-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4137):
Prerequisites: `build/test_desktop_db_mut_write_skip build/test_desktop_db_mut_magic_flip build/.makefile.stamp`
```make
@dd if=/dev/zero of=$(DESKTOP_DB_HOST_IMG) bs=512 count=2880 status=none
@SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mformat -i $(DESKTOP_DB_HOST_IMG) \
	-f 1440 -N 0x494e4954 ::
@if $(TEST_DESKTOP_DB_MUT_SKIP) "$(DESKTOP_DB_HOST_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-desktop-db-mutant FAIL: DB_WRITE_SKIP passed -- persistence diff is decoration\n'; exit 1; \
else \
	printf '>>> test-desktop-db-mutant: DB_WRITE_SKIP correctly RED\n'; \
fi
@dd if=/dev/zero of=$(DESKTOP_DB_HOST_IMG) bs=512 count=2880 status=none
@SOURCE_DATE_EPOCH=$(FLAIR_DATA_EPOCH) TZ=UTC mformat -i $(DESKTOP_DB_HOST_IMG) \
	-f 1440 -N 0x494e4954 ::
@if $(TEST_DESKTOP_DB_MUT_MAGIC) "$(DESKTOP_DB_HOST_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-desktop-db-mutant FAIL: DB_MAGIC_FLIP passed -- magic oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-desktop-db-mutant: DB_MAGIC_FLIP correctly RED\n'; \
fi
```

## test-devices

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7891):
Prerequisites: `build/test_devices build/.makefile.stamp`
```make
@printf ">>> test-devices: chain + attr bits + NUL/CON/PRN/AUX/CLOCK$$ dispatch\n"
@$(TEST_DEVICES)
@printf ">>> test-devices: green\n"
```

## test-devices-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7896):
Prerequisites: `build/test_devices_mutant_nodonebit build/test_devices_mutant_nulreadbyte build/.makefile.stamp`
```make
@printf ">>> test-devices-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_DEVICES_MUT_NODONE) >/dev/null 2>&1; then \
	printf '!!! test-devices-mutant FAIL: no-done-bit mutant PASSED -- the DONE test is decoration\n'; exit 1; \
else printf '>>> test-devices-mutant: green (no-done-bit mutant correctly RED)\n'; fi
@if $(TEST_DEVICES_MUT_NULREAD) >/dev/null 2>&1; then \
	printf '!!! test-devices-mutant FAIL: nul-read-byte mutant PASSED -- the NUL EOF test is decoration\n'; exit 1; \
else printf '>>> test-devices-mutant: green (nul-read-byte mutant correctly RED -- the oracle bites)\n'; fi
```

## test-devwire

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7984):
Prerequisites: `build/test_devwire build/.makefile.stamp`
```make
@printf ">>> test-devwire: AH=3Dh OPEN-by-name CON/NUL/PRN/CLOCK$$ + READ/WRITE routing + legacy-CON preserved\n"
@$(TEST_DEVWIRE)
@printf ">>> test-devwire: green\n"
```

## test-devwire-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7989):
Prerequisites: `build/test_devwire_mutant_nodevice build/.makefile.stamp`
```make
@printf ">>> test-devwire-mutant: confirming the no-device mutant goes RED (Rule 6)\n"
@if $(TEST_DEVWIRE_MUT) >/dev/null 2>&1; then \
	printf '!!! test-devwire-mutant FAIL: no-device mutant PASSED -- the OPEN-by-name oracle is decoration\n'; exit 1; \
else printf '>>> test-devwire-mutant: green (no-device mutant correctly RED -- the oracle bites)\n'; fi
```

## test-dir

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25755):
Prerequisites: `build/qemu_harness build/demo_boot.img build/fat_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dir : a PROGRAM lists a real directory\n'
@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.3. beads initech-509.5.\n'
@printf '  Prove FINDFIRST(4Eh)/FINDNEXT(4Fh) into the DTA find-data block.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(DEMO_IMG)" "$(FAT_DATA_IMG)"
@printf 'Expecting : FILEIO-BIND-OK + fixture names between DIR-PROG-OUTPUT-BEGIN/END + DIR-PROG-EXIT rc=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_DATA_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_DATA_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(DEMO_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(DIRP_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(DIRP_REPORT)" || true
@cat "$(DIRP_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(DIRP_REPORT)"; then \
	printf '!!! test-dir FAIL: TRIPLE FAULT\n'; exit 1; \
fi
@printf '>>> test-dir [1/3]: no triple-fault\n'
@if [ ! -s "$(DIRP_SERIAL)" ]; then \
	printf '!!! test-dir FAIL: no serial captured at %s\n' "$(DIRP_SERIAL)"; exit 1; \
fi
@grep -q '^FILEIO-BIND-OK$$' "$(DIRP_SERIAL)" \
	|| { printf '!!! test-dir FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
@printf '>>> test-dir [2/3]: FILEIO-BIND-OK\n'
@# The DIR program output is bracketed; assert every fixture name appears
@# within the bracketed region (sed extracts it; grep asserts each name).
@sed -n '/^DIR-PROG-OUTPUT-BEGIN$$/,/^DIR-PROG-OUTPUT-END$$/p' "$(DIRP_SERIAL)" > "$(BUILD)/$(DIRP_NAME).names"
@for n in $(DIRP_NAMES); do \
	grep -qF "$$n" "$(BUILD)/$(DIRP_NAME).names" \
		|| { printf '!!! test-dir FAIL: FINDFIRST/FINDNEXT did not list %s\n' "$$n"; exit 1; }; \
done
@grep -q '^DIR-PROG-EXIT rc=0$$' "$(DIRP_SERIAL)" \
	|| { printf '!!! test-dir FAIL: DIR-PROG-EXIT rc=0 missing -- the DIR program did not finish cleanly\n'; exit 1; }
@printf '>>> test-dir [3/3]: the program listed %s via FINDFIRST/FINDNEXT + DIR-PROG-EXIT rc=0\n' "$(DIRP_NAMES)"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a flat program enumerated a real FAT12 directory via INT 21h FINDFIRST/FINDNEXT\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-dir-abi

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 28):
Prerequisites: `build/dir_abi build/.makefile.stamp`
```make
@$< spec/int21h_calling_convention.json
```

## test-dir-abi-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 30):
Prerequisites: `build/dir_abi build/.makefile.stamp`
```make
@set -eu; for ah in 39 3A 3B 47 40; do \
 sed "s/\"ah\": \"$${ah}h\"/\"ah\": \"FFh\"/" spec/int21h_calling_convention.json > $(BUILD)/dir_abi_$$ah.json; \
 if $< $(BUILD)/dir_abi_$$ah.json >$(BUILD)/dir_abi_$$ah.log 2>&1; then echo "FAIL ABI_$$ah survived";exit 1;fi; \
 grep "^RED dir-abi: $${ah}h row missing" $(BUILD)/dir_abi_$$ah.log; done
@sed 's/ECX=0/ECX=1/g' spec/int21h_calling_convention.json > $(BUILD)/dir_abi_40_zero.json
@if $< $(BUILD)/dir_abi_40_zero.json >$(BUILD)/dir_abi_40_zero.log 2>&1; then echo 'FAIL ABI_40_ZERO survived';exit 1;fi
@grep '^RED dir-abi: 40h zero-count size contract' $(BUILD)/dir_abi_40_zero.log
@echo 'GREEN test-dir-abi-mutant: ABI_39 ABI_3A ABI_3B ABI_47 ABI_40 ABI_40_ZERO caught RED'
```

## test-dir-args

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 39):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin build/dos_parity_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh $(if $(filter test-dos-stamp,$@),stamp,$(if $(filter test-dir-args,$@),dir,path)) $(SECONDARY_TRACER_IMG)
```

## test-dir-args-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 43):
Prerequisites: `build/qemu_harness build/tracer_mut_safety_dirargs.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_parity.sh dir $(BUILD)/tracer_mut_safety_dirargs.img mutant > $(BUILD)/dir_args_mutant.log 2>&1; then echo 'FAIL: DIR mutant survived'; exit 1; fi
@grep 'dir RED: DIR B: ignored its drive' $(BUILD)/dir_args_mutant.log
@printf 'PASS DIR mutant: CMD_MUTATE_DIR_ARGS caught RED\n'
```

## test-dir-free-space

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 21):
Prerequisites: `build/test_dir_disk build/flair_data.img build/.makefile.stamp`
```make
@LC_ALL=C TZ=UTC mdir -i $(FLAIR_DATA_IMG) :: > $(BUILD)/dir_space.mdir
@$(BUILD)/test_dir_disk $(FLAIR_DATA_IMG) $(BUILD)/dir_space.mdir $(BUILD)/dir_space.cells
```

## test-dir-free-space-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 67):
Prerequisites: `build/test_dir_disk_mut_free build/flair_data.img build/.makefile.stamp`
```make
@LC_ALL=C TZ=UTC mdir -i $(FLAIR_DATA_IMG) :: > $(BUILD)/dir_space_mut.mdir
@if $(BUILD)/test_dir_disk_mut_free $(FLAIR_DATA_IMG) $(BUILD)/dir_space_mut.mdir $(BUILD)/dir_space_mut.cells > $(BUILD)/dir_space_mut.log 2>&1; then echo 'FAIL: free-space mutant passed'; exit 1; fi
@grep 'DIR SPACE RED:' $(BUILD)/dir_space_mut.log
@echo 'DIR SPACE MUTANT PASS: zero-free-count RED against mdir'
```

## test-dir-look

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 17):
Prerequisites: `build/test_dir_look build/.makefile.stamp`
```make
@$(BUILD)/test_dir_look $(DOSLOOK_DIR)/dos33_dir.golden
```

## test-dir-look-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 57):
Prerequisites: `build/test_dir_look_mut_date build/test_dir_look_mut_joined build/test_dir_look_mut_lower build/.makefile.stamp`
```make
@set -e; for tag in date joined lower; do \
  log=$(BUILD)/dir_look_mut_$$tag.log; \
  if $(BUILD)/test_dir_look_mut_$$tag $(DOSLOOK_DIR)/dos33_dir.golden > $$log 2>&1; then cat $$log; echo "FAIL: $$tag mutant passed"; exit 1; fi; \
  grep -q 'DIR LOOK RED:' $$log; \
  printf 'DIR LOOK MUTANT PASS: %s RED against independent manual golden\n' $$tag; \
done
```

## test-dir-match-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 111):
Prerequisites: `build/dir_match build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dir_match.sh emu $(SECONDARY_TRACER_IMG)
```

## test-dir-match-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dir-match-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-dir-match-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/dir-match-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/dir-match-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "dir-match-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/dir-match-emu_bochs.report.txt" || true
@cat "$(BUILD)/dir-match-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-dir-match-emu-bochs "$(BUILD)/dir-match-emu_bochs.report.txt" "$(BUILD)/dir-match-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-dir-match-emu-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 113):
Prerequisites: `build/dir_match build/qemu_harness build/tracer_mut_safety_dirargs.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dir_match.sh emu $(BUILD)/tracer_mut_safety_dirargs.img mutant >$(BUILD)/dir_match_mutant.log 2>&1; then echo 'FAIL CMD_MUTATE_DIR_ARGS survived'; exit 1; fi
@grep '^RED dir-match: TXT shell membership != mdir/test-80k matcher' $(BUILD)/dir_match_mutant.log
@echo 'GREEN test-dir-match-emu-mutant: CMD_MUTATE_DIR_ARGS caught RED'
```

## test-dir-match-host

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 105):
Prerequisites: `build/dir_match build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dir_match.sh host
```

## test-dir-match-host-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 107):
Prerequisites: `build/dir_match_mut build/.makefile.stamp`
```make
@if DIR_MATCH_HOST=$(BUILD)/dir_match_mut sh harness/diff/fat_diff/dir_match.sh host unused mutant >$(BUILD)/dir_match_host_mutant.log 2>&1; then echo 'FAIL INT21_MUTATE_WILD_QMARK_LITERAL survived'; exit 1; fi
@grep '^RED dir-match: \*.TXT host != mdir' $(BUILD)/dir_match_host_mutant.log
@echo 'GREEN test-dir-match-host-mutant: INT21_MUTATE_WILD_QMARK_LITERAL caught RED'
```

## test-dir-page

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 41):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh page $(SECONDARY_TRACER_IMG)
```

## test-dir-volume

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 35):
Prerequisites: `build/test_dir_disk build/flair_data.img build/.makefile.stamp`
```make
@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_root.img root
@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_boot.img boot
@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_none.img none
@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_root.img 'ROOT LABEL'
@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_boot.img 'BOOT LABEL'
@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_none.img ''
```

## test-dir-volume-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/doslook.mk', line 44):
Prerequisites: `test-dir-volume build/test_dir_disk_mut_label build/.makefile.stamp`
```make
@if $(BUILD)/test_dir_disk_mut_label --label $(BUILD)/dir_label_root.img 'ROOT LABEL' > $(BUILD)/dir_label_mut.log 2>&1; then echo 'FAIL: BPB-only label mutant passed'; exit 1; fi
@grep 'DIR LABEL RED:' $(BUILD)/dir_label_mut.log
@echo 'DIR LABEL MUTANT PASS: BPB-only label RED on conflicting root label'
```

## test-dos-break

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 25):
Prerequisites: `build/test_dos_break build/.makefile.stamp`
```make
@$<
```

## test-dos-break-cleanup-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 136):
Prerequisites: `build/test_dos_break_cleanup_mut build/.makefile.stamp`
```make
@rc=0; $< >$(BUILD)/dos_break_cleanup_mut.log 2>&1 || rc=$$?; test $$rc -eq 1
@grep '^RED dos-break: cancel cleanup must CLOSE and restore stdin/stdout on every path' $(BUILD)/dos_break_cleanup_mut.log
@echo 'GREEN test-dos-break-cleanup-mutant: queued Ctrl-C cleanup regression caught'
```

## test-dos-break-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 31):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/dos_break_program.bin build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_break.sh $(SECONDARY_TRACER_IMG)
```

## test-dos-break-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 124):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dos-break-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-dos-break-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/dos-break-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/dos-break-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "dos-break-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/dos-break-emu_bochs.report.txt" || true
@cat "$(BUILD)/dos-break-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-dos-break-emu-bochs "$(BUILD)/dos-break-emu_bochs.report.txt" "$(BUILD)/dos-break-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-dos-break-emu-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 33):
Prerequisites: `build/qemu_harness build/j1lu_dosbreak.img build/dos_break_program.bin build/console_text build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_break.sh $(BUILD)/j1lu_dosbreak.img mutant >$(BUILD)/dos_break_emu_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_DISK_BREAK survived QEMU'; exit 1; fi
@grep '^RED dos-break-emu: on did not return after disk Ctrl-C' $(BUILD)/dos_break_emu_mut.log
@echo 'GREEN test-dos-break-emu-mutant: INT21_MUTATE_DISK_BREAK caught RED'
```

## test-dos-break-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 27):
Prerequisites: `build/test_dos_break_mut build/.makefile.stamp`
```make
@if $< >$(BUILD)/dos_break_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_DISK_BREAK survived'; exit 1; fi
@grep '^RED dos-break: ON must abort non-console call before service' $(BUILD)/dos_break_mut.log
@echo 'GREEN test-dos-break-mutant: INT21_MUTATE_DISK_BREAK caught RED'
```

## test-dos-not-ready

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 61):
Prerequisites: `build/test_dos_not_ready build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dos-not-ready-host
@dd if=/dev/zero of=$(BUILD)/dos-not-ready-host/data.img bs=512 count=2880 status=none
@mformat -i $(BUILD)/dos-not-ready-host/data.img -f 1440 -N 102 ::
@cp -f $(BUILD)/dos-not-ready-host/data.img $(BUILD)/dos-not-ready-host/reference.img
@$< $(BUILD)/dos-not-ready-host/data.img
@cmp $(BUILD)/dos-not-ready-host/data.img $(BUILD)/dos-not-ready-host/reference.img
@fsck.fat -n $(BUILD)/dos-not-ready-host/data.img >$(BUILD)/dos-not-ready-host/fsck.log
@echo 'GREEN test-dos-not-ready: identical mformat image + dosfstools clean'
```

## test-dos-not-ready-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 74):
Prerequisites: `build/qemu_harness build/dosmsg.img build/console_text build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_not_ready.sh $(BUILD)/dosmsg.img
```

## test-dos-not-ready-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 125):
Prerequisites: `build/bochs_harness build/dosmsg.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dos-not-ready-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/dosmsg.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-dos-not-ready-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/dos-not-ready-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/dosmsg.img" --disk2 "$(BUILD)/dos-not-ready-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "dos-not-ready-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/dos-not-ready-emu_bochs.report.txt" || true
@cat "$(BUILD)/dos-not-ready-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-dos-not-ready-emu-bochs "$(BUILD)/dos-not-ready-emu_bochs.report.txt" "$(BUILD)/dos-not-ready-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-dos-not-ready-emu-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 76):
Prerequisites: `build/qemu_harness build/dosmsg_mut.img build/console_text build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_not_ready.sh $(BUILD)/dosmsg_mut.img mutant >$(BUILD)/dos_not_ready_emu_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_NOT_READY survived QEMU'; exit 1; fi
@grep '^RED dos-not-ready-emu: MSG-DOS-0006 missing' $(BUILD)/dos_not_ready_emu_mut.log
@echo 'GREEN test-dos-not-ready-emu-mutant: INT21_MUTATE_NOT_READY caught RED'
```

## test-dos-not-ready-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 70):
Prerequisites: `test-dos-not-ready build/test_dos_not_ready_mut build/.makefile.stamp`
```make
@if $(BUILD)/test_dos_not_ready_mut $(BUILD)/dos-not-ready-host/data.img >$(BUILD)/dos_not_ready_mut.log 2>&1; then echo 'FAIL INT21_MUTATE_NOT_READY survived'; exit 1; fi
@grep '^RED dos-not-ready: missing exact MSG-DOS-0006' $(BUILD)/dos_not_ready_mut.log
@echo 'GREEN test-dos-not-ready-mutant: INT21_MUTATE_NOT_READY caught RED'
```

## test-dos-resident-boot-write-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 158):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/dos_resident_break_check build/resident_boot_write.bin build/.makefile.stamp`
```make
@rc=0; sh harness/diff/fat_diff/dos_resident_break.sh $(SECONDARY_TRACER_IMG) bootwrite controlshort yes > $(BUILD)/resident-bootwrite.log 2>&1 || rc=$$?; \
 test $$rc = 1 && grep -Fx 'RED dos-resident-break: boot disk changed' $(BUILD)/resident-bootwrite.log || { cat $(BUILD)/resident-bootwrite.log; exit 1; }; \
 root=$$(sed -n 's/^oracle artifacts: //p' $(BUILD)/resident-bootwrite.log); \
 grep -q BOOT-WRITE-COMMITTED $$root/controlshort/run.serial
@echo 'GREEN test-dos-resident-boot-write-mutant: actual guest boot-disk write rejected by independent cmp'
```

## test-dos-resident-break-emu

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 124):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/console_text build/dos_resident_break_check build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_resident_break.sh $(SECONDARY_TRACER_IMG)
```

## test-dos-resident-break-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 140):
Prerequisites: `build/qemu_harness build/console_text build/dos_resident_break_check build/j1lu_resident_type.img build/j1lu_resident_dir.img build/.makefile.stamp`
```make
@set -eu; for m in type dir; do \
  mode=controlshort; test $$m != dir || mode=gaps2; \
  rc=0; sh harness/diff/fat_diff/dos_resident_break.sh $(BUILD)/j1lu_resident_$$m.img mutant-$$m $$mode >$(BUILD)/dos_resident_break_$$m.log 2>&1 || rc=$$?; \
  test $$rc -eq 1; \
  if test $$m = type; then why='TYPE emitted bytes after cancellation'; else why='gaps2 did not return a usable prompt'; fi; \
  grep -E "^RED dos-resident-break: ($$why|listing reached the final line after cancellation)$$" $(BUILD)/dos_resident_break_$$m.log; \
  root=$$(sed -n 's/^oracle artifacts: //p' $(BUILD)/dos_resident_break_$$m.log); \
  grep -q INT23-BREAK $$root/$$mode/run.serial; \
  echo "GREEN test-dos-resident-break-mutant: $$m cancellation caught"; \
done
```

## test-dos-stamp

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 39):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin build/dos_parity_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh $(if $(filter test-dos-stamp,$@),stamp,$(if $(filter test-dir-args,$@),dir,path)) $(SECONDARY_TRACER_IMG)
```

## test-dos-stamp-copy

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/copy_stamp.mk', line 23):
Prerequisites: `build/dos_parity_host build/qemu_harness build/tracer_secondary.img build/xcopy_program.bin build/.makefile.stamp`
```make
@sh $(COPY_STAMP_DIR)/copy_stamp.sh emu $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN)
```

## test-dos-stamp-copy-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/copy_stamp.mk', line 25):
Prerequisites: `build/dos_parity_host build/qemu_harness build/tracer_secondary.img build/xcopy_program.bin build/tracer_mut_safety_copytime.img build/tracer_mut_safety_copydate.img build/xcopy_stamp_mut_program.bin build/.makefile.stamp`
```make
@set -eu; for m in COPY_TIME COPY_DATE XCOPY_TIME; do \
  boot=$(SECONDARY_TRACER_IMG); x=$(XCOPY_PROG_BIN); \
  case $$m in COPY_TIME) boot=$(BUILD)/tracer_mut_safety_copytime.img;; COPY_DATE) boot=$(BUILD)/tracer_mut_safety_copydate.img;; XCOPY_TIME) x=$(BUILD)/xcopy_stamp_mut_program.bin;; esac; \
  log=$(BUILD)/copy_stamp_emu_$$m.log; \
  if sh $(COPY_STAMP_DIR)/copy_stamp.sh emu $$boot $$x $$m > $$log 2>&1; then echo "FAIL copy stamp: $$m survived"; exit 1; fi; \
  grep '^copy-stamp RED:.*stamp' $$log || { cat $$log; exit 1; }; \
  echo "PASS copy-stamp emulator mutant: $$m booted, completed and caught RED"; \
done
```

## test-dos-stamp-copy-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/copy_stamp.mk', line 12):
Prerequisites: `build/dos_parity_host build/test_xcopy_tree build/.makefile.stamp`
```make
@sh $(COPY_STAMP_DIR)/copy_stamp.sh host
```

## test-dos-stamp-copy-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/copy_stamp.mk', line 14):
Prerequisites: `build/dos_parity_host build/dos_copy_stamp_host_mut build/test_xcopy_tree build/test_xcopy_tree_mut_TIME build/.makefile.stamp`
```make
@set -eu; for m in GET_SWAP XCOPY_TIME; do \
  h=$(BUILD)/dos_parity_host; x=$(ZUVO_HOST); \
  case $$m in GET_SWAP) h=$(BUILD)/dos_copy_stamp_host_mut;; XCOPY_TIME) x=$(BUILD)/test_xcopy_tree_mut_TIME;; esac; \
  log=$(BUILD)/copy_stamp_host_$$m.log; \
  if COPY_STAMP_HOST=$$h COPY_STAMP_XHOST=$$x sh $(COPY_STAMP_DIR)/copy_stamp.sh host unused unused $$m > $$log 2>&1; then echo "FAIL copy stamp: $$m survived"; exit 1; fi; \
  grep '^copy-stamp RED:.*stamp' $$log || { cat $$log; exit 1; }; \
  echo "PASS copy-stamp host mutant: $$m caught RED"; \
done
```

## test-dos-stamp-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 35):
Prerequisites: `build/qemu_harness build/j1lu_stamp.img build/dos_parity_host build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_parity.sh stamp $(BUILD)/j1lu_stamp.img mutant > $(BUILD)/dos_stamp_emu_mutant.log 2>&1; then echo 'FAIL: stamp emulator mutant survived'; exit 1; fi
@grep 'stamp RED: NEW.TXT stamp' $(BUILD)/dos_stamp_emu_mutant.log
@printf 'PASS stamp emulator mutant: FILEIO_MUTATE_NO_STAMP caught RED\n'
```

## test-dos-stamp-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 29):
Prerequisites: `build/dos_parity_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh stamp-host
```

## test-dos-stamp-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 31):
Prerequisites: `build/dos_parity_host_mut build/.makefile.stamp`
```make
@DOS_PARITY_HOST=$(BUILD)/dos_parity_host_mut sh harness/diff/fat_diff/dos_parity.sh stamp-host unused mutant > $(BUILD)/dos_stamp_mutant.log 2>&1; rc=$$?; \
[ $$rc -ne 0 ] && grep 'stamp-host RED:.*stamp' $(BUILD)/dos_stamp_mutant.log || { cat $(BUILD)/dos_stamp_mutant.log; exit 1; }; \
printf 'PASS stamp mutant: FILEIO_MUTATE_NO_STAMP caught RED\n'
```

## test-dos-startup

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 51):
Prerequisites: `build/qemu_harness build/tracer_boot.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh startup $(TRACER_IMG)
@sh harness/diff/fat_diff/dos_parity.sh startup-set $(TRACER_IMG)
@sh harness/diff/fat_diff/dos_parity.sh startup-autoexec $(TRACER_IMG)
```

## test-dos-startup-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 55):
Prerequisites: `build/qemu_harness build/tracer_startup_mut.img build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_parity.sh startup $(BUILD)/tracer_startup_mut.img mutant > $(BUILD)/dos_startup_mutant.log 2>&1; then echo 'FAIL: startup mutant survived'; exit 1; fi
@grep 'startup RED: startup date/prompt missing' $(BUILD)/dos_startup_mutant.log
@printf 'PASS startup mutant: CMD_MUTATE_STARTUP caught RED\n'
```

## test-dos-volumes

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 9):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/transient_flairtenants.bin build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_volumes.sh emu $(SECONDARY_TRACER_IMG)
```

## test-dos-volumes-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 33):
Prerequisites: `test-dos-volumes build/qemu_harness build/tracer_mut_safety_volume_prompt.img build/tracer_mut_safety_volume_message.img build/transient_flairtenants.bin build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dos-volumes
@if sh harness/diff/fat_diff/dos_volumes.sh emu $(BUILD)/tracer_mut_safety_volume_prompt.img prompt > $(BUILD)/dos-volumes/prompt.log 2>&1; then echo 'FAIL dos-volumes PROMPT survived'; exit 1; fi
@grep '^RED dos-volumes PROMPT current C' $(BUILD)/dos-volumes/prompt.log
@if sh harness/diff/fat_diff/dos_volumes.sh emu $(BUILD)/tracer_mut_safety_volume_message.img message > $(BUILD)/dos-volumes/message.log 2>&1; then echo 'FAIL dos-volumes MESSAGE survived'; exit 1; fi
@grep '^RED dos-volumes INVALID B/D message' $(BUILD)/dos-volumes/message.log
@echo 'GREEN test-dos-volumes-emu-mutant: PROMPT MESSAGE rejected'
```

## test-dos-volumes-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 7):
Prerequisites: `build/dos_volumes_host build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_volumes.sh host $(SECONDARY_TRACER_IMG)
```

## test-dos-volumes-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 18):
Prerequisites: `test-dos-volumes-host build/dos_volumes_host_SELECT build/dos_volumes_host_PATH build/dos_volumes_host_CWD build/dos_volumes_host_HANDLE build/dos_volumes_host_SEARCH build/dos_volumes_host_CACHE build/dos_volumes_host_INVALID build/dos_volumes_host_LIMIT build/dos_volumes_host_LASTDRIVE build/tracer_secondary.img build/.makefile.stamp`
```make
@set -eu; for m in $(DOS_VOLUMES_MUTANTS); do \
 log=$(BUILD)/dos-volumes/mut-$$m.log; mkdir -p $(BUILD)/dos-volumes; \
 if DOS_VOLUMES_HOST=$(BUILD)/dos_volumes_host_$$m sh harness/diff/fat_diff/dos_volumes.sh host $(SECONDARY_TRACER_IMG) $$m > $$log 2>&1; then echo "FAIL dos-volumes $$m survived"; exit 1; fi; \
 grep '^RED dos-volumes ' $$log; done
@echo 'GREEN test-dos-volumes-mutant: SELECT PATH CWD HANDLE SEARCH CACHE INVALID LIMIT LASTDRIVE rejected'
```

## test-dosmsg

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28475):
Prerequisites: `build/dos_messages.h build/kernel_shell.bin build/kernel_flairtenants.bin build/flairovl_kernel_flairtenants.ovl build/dosmsg_scan harness/diff/fat_diff/dosmsg.allow harness/diff/fat_diff/dosmsg.allow.baseline build/attrib_program.bin build/label_program.bin build/chkdsk_program.bin build/xcopy_program.bin build/tree_program.bin build/edlin_program.bin build/find_program.bin build/sort_program.bin build/dosmsg_scan harness/diff/fat_diff/dosmsg.allow harness/diff/fat_diff/dosmsg.allow.baseline build/attrib_program.bin build/label_program.bin build/chkdsk_program.bin build/xcopy_program.bin build/tree_program.bin build/edlin_program.bin build/find_program.bin build/sort_program.bin build/.makefile.stamp | build`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dosmsg : DEC-13 controlled-vocabulary oracle\n'
@printf '  Ref: ADR-0003 Appendix C / DEC-13; spec/dos_messages.json (LOCKED).\n'
@printf '  CLAUDE.md Law 2 (oracle is the truth) / Rule 6 (mutation-proven).\n'
@printf '======================================================================\n'
@printf '>>> test-dosmsg [1/3]: CONSISTENCY -- build/dos_messages.h encodes the spec verbatim\n'
@python3 -c "import json,re; \
```

## test-dosmsg-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28550):
Prerequisites: `test-dosmsg-source-mutant build/dos_messages.h build/kernel_shell.bin build/kernel_flairtenants.bin build/flairovl_kernel_flairtenants.ovl build/.makefile.stamp | build`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-dosmsg-mutant : Rule 6 (oracle must bite)\n'
@printf '======================================================================\n'
@printf '>>> test-dosmsg-mutant [B]: PRESENCE tooth must detect a reworded message in the image\n'
@rm -rf $(DOSMSG_MUT_DIR) && mkdir -p $(DOSMSG_MUT_DIR)
@$(MAKE) --no-print-directory BUILD=$(DOSMSG_MUT_DIR) $(DOSMSG_MUT_DIR)/$(notdir $(DOS_MESSAGES_H)) > $(DOSMSG_MUT_DIR)/build.log 2>&1 \
	|| { cat $(DOSMSG_MUT_DIR)/build.log; exit 1; }
@python3 -c "h=open('$(DOSMSG_MUT_DIR)/$(notdir $(DOS_MESSAGES_H))').read(); \
@$(MAKE) --no-print-directory -o $(DOSMSG_MUT_DIR)/$(notdir $(DOS_MESSAGES_H)) -o $(PALETTE_H) BUILD=$(DOSMSG_MUT_DIR) \
	$(DOSMSG_MUT_DIR)/$(notdir $(KERNEL_SHELL_BIN)) >> $(DOSMSG_MUT_DIR)/build.log 2>&1 \
	|| { cat $(DOSMSG_MUT_DIR)/build.log; exit 1; }
@python3 -c "import json,re,subprocess,sys; \
```

## test-dosmsg-source-mutant

Declared tier: outside active vector; #  recipe to execute (from 'harness/diff/fat_diff/dosmsg.mk', line 96):
Prerequisites: `build/dosmsg_scan build/dos_messages.h harness/diff/fat_diff/dosmsg.allow harness/diff/fat_diff/dosmsg.allow.baseline build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dosmsg_mutants.sh $(DOSMSG_SCAN) $(DOS_MESSAGES_H) $(DOSMSG_ALLOW) $(DOSMSG_BASELINE) $(DOSMSG_AUDIT_SRCS)
```

## test-drive-letters

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 100):
Prerequisites: `build/test_drive_letters build/.makefile.stamp`
```make
@$<
```

## test-drive-letters-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 107):
Prerequisites: `build/test_drive_letters_DISK build/test_drive_letters_RUNNING build/.makefile.stamp`
```make
@set -eu; for m in DISK RUNNING; do \
 case $$m in DISK) why='RED drive-letters A physical identity';; RUNNING) why='RED drive-letters running pair';; esac; \
 if $(BUILD)/test_drive_letters_$$m > $(BUILD)/drive-letters-mut-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
 grep -Fx "$$why" $(BUILD)/drive-letters-mut-$$m.log; done
@echo 'GREEN test-drive-letters-mutant: DISK RUNNING rejected'
```

## test-drive-letters-spec

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 88):
Prerequisites: `build/gen_drive_letters spec/drive_letters.json spec/drive_letters.h build/.makefile.stamp`
```make
@$< spec/drive_letters.json > $(BUILD)/drive_letters.h
@cmp spec/drive_letters.h $(BUILD)/drive_letters.h
@echo 'GREEN test-drive-letters-spec: JSON/header equal'
```

## test-drive-letters-spec-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/formatsys.mk', line 92):
Prerequisites: `build/gen_drive_letters build/.makefile.stamp`
```make
@sed 's/"a_disk": 129/"a_disk": 128/' spec/drive_letters.json > $(BUILD)/drive_letters_mut.json
@$< $(BUILD)/drive_letters_mut.json > $(BUILD)/drive_letters_mut.h
@if cmp -s spec/drive_letters.h $(BUILD)/drive_letters_mut.h; then echo 'FAIL drive letters spec mutant survived'; exit 1; fi
@echo 'RED drive-letters SPEC A disk drift'
@echo 'GREEN test-drive-letters-spec-mutant: A disk drift rejected'
```

## test-drive-operands

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/drive_operands.mk', line 9):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/transient_flairtenants.bin build/test_xcopy_tree build/xcopy_program.bin build/tree_program.bin build/attrib_program.bin build/label_program.bin build/chkdsk_program.bin build/fdisk_program.bin build/.makefile.stamp`
```make
@sh $(DRIVE_OPS) emu $(SECONDARY_TRACER_IMG) green $(DRIVE_OPS_EXT)
@sh $(DRIVE_OPS) fdisk $(SECONDARY_TRACER_IMG) green $(DRIVE_OPS_EXT)
```

## test-drive-operands-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/drive_operands.mk', line 25):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/transient_flairtenants.bin build/test_xcopy_tree build/xcopy_program.bin build/tree_program.bin build/attrib_program.bin build/label_program.bin build/chkdsk_program.bin build/fdisk_program.bin build/xcopy_drive_mut_program.bin build/tree_drive_mut_program.bin build/attrib_drive_mut_program.bin build/fdisk_LETTER.bin build/tracer_mut_safety_cd_drive.img build/label_drive_mut_program.bin build/j1lu_absdrive.img build/.makefile.stamp`
```make
@set -eu; mkdir -p $(BUILD)/drive-operands; for m in $(DRIVE_OPS_MUTANTS); do \
 x=$(XCOPY_PROG_BIN); t=$(TREE_PROG_BIN); a=$(ATTRIB_PROG_BIN); l=$(LABEL_PROG_BIN); c=$(CHKDSK_PROG_BIN); f=$(FDISK_PROG_BIN); \
 boot=$(SECONDARY_TRACER_IMG); mode=emu; \
 case $$m in \
  xcopy) x=$(BUILD)/xcopy_drive_mut_program.bin; why='L032 XCOPY overwrote A:\DHOME\ONE.TXT';; \
  tree) t=$(BUILD)/tree_drive_mut_program.bin; why='L014 TREE D: walked the wrong drive/volume';; \
  attrib) a=$(BUILD)/attrib_drive_mut_program.bin; why='L012 ATTRIB +R did not reach D: (mattrib)';; \
  fdisk) f=$(BUILD)/fdisk_LETTER.bin; mode=fdisk; why='L028 FDISK second primary not D:';; \
  cd) boot=$(BUILD)/tracer_mut_safety_cd_drive.img; why='L011 CD D: did not display D:\DHOME';; \
  label) l=$(BUILD)/label_drive_mut_program.bin; why='L015 LABEL D: label not on D:';; \
  absdisk) boot=$(BUILD)/j1lu_absdrive.img; why='L013 CHKDSK C: reported a disk error';; \
 esac; log=$(BUILD)/drive-operands/mut-$$m.log; \
 if sh $(DRIVE_OPS) $$mode $$boot mut-$$m $$x $$t $$a $$l $$c $$f > $$log 2>&1; then echo "FAIL drive-operands $$m mutant survived"; exit 1; fi; \
 grep -Fx "RED drive-operands $$why" $$log || { cat $$log; exit 1; }; \
 echo "GREEN test-drive-operands-mutant: $$m caught"; done
```

## test-edlin-emu

Declared tier: EMU; #  recipe to execute (from 'harness/edlin/edlin.mk', line 40):
Prerequisites: `build/edlin_model build/edlin_program.bin build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/edlin.sh emu $(BUILD) $(EDLIN_PROG_BIN) $(EDLIN_MODEL) $(SECONDARY_TRACER_IMG)
```

## test-edlin-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 129):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-edlin-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-edlin-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/edlin-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/edlin-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "edlin-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/edlin-emu_bochs.report.txt" || true
@cat "$(BUILD)/edlin-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-edlin-emu-bochs "$(BUILD)/edlin-emu_bochs.report.txt" "$(BUILD)/edlin-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-edlin-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/edlin/edlin.mk', line 51):
Prerequisites: `build/edlin_model build/edlin_NUMBER.bin build/edlin_BACKUP.bin build/edlin_FIRST.bin build/edlin_QUIT.bin build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@set -eu; for pair in NUMBER:transcript BACKUP:backup FIRST:output QUIT:transcript; do m=$${pair%:*}; why=$${pair#*:}; log=build/edlin-emu-$$m.log; if INITECH_QMP_STDIO=1 EDLIN_ONLY=$$m sh harness/diff/fat_diff/edlin.sh emu build build/edlin_$$m.bin build/edlin_model build/tracer_secondary.img >$$log 2>&1; then echo "FAIL EDLIN $$m survived"; exit 1; fi; grep "RED EDLIN-$$why" $$log || { cat $$log; exit 1; }; echo "GREEN test-edlin-emu-mutant $$m rejected"; done
```

## test-edlin-host

Declared tier: HOST; #  recipe to execute (from 'harness/edlin/edlin.mk', line 38):
Prerequisites: `build/edlin_model build/edlin_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/edlin.sh host $(BUILD) $(EDLIN_HOST) $(EDLIN_MODEL)
```

## test-edlin-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/edlin/edlin.mk', line 50):
Prerequisites: `build/edlin_model build/edlin_host_NUMBER build/edlin_host_BACKUP build/edlin_host_FIRST build/edlin_host_QUIT build/.makefile.stamp`
```make
@set -eu; for pair in NUMBER:transcript BACKUP:backup FIRST:output QUIT:transcript; do m=$${pair%:*}; why=$${pair#*:}; log=build/edlin-host-$$m.log; if INITECH_QMP_STDIO=1 EDLIN_ONLY=$$m sh harness/diff/fat_diff/edlin.sh host build build/edlin_host_$$m build/edlin_model build/tracer_secondary.img >$$log 2>&1; then echo "FAIL EDLIN $$m survived"; exit 1; fi; grep "RED EDLIN-$$why" $$log || { cat $$log; exit 1; }; echo "GREEN test-edlin-host-mutant $$m rejected"; done
```

## test-env

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7449):
Prerequisites: `build/test_env build/.makefile.stamp`
```make
@printf ">>> test-env: init + set/get + upcase + upsert + unset + overflow + serialize + iteration\n"
@$(TEST_ENV)
@printf ">>> test-env: green\n"
```

## test-env-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7455):
Prerequisites: `build/test_env_mutant_noupcase build/test_env_mutant_nodedup build/.makefile.stamp`
```make
@printf ">>> test-env-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_ENV_MUT_NOUP) >/dev/null 2>&1; then \
	printf '!!! test-env-mutant FAIL: no-upcase mutant PASSED -- the upcase test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-env-mutant: green (no-upcase mutant correctly RED)\n'; \
fi
@if $(TEST_ENV_MUT_NODUP) >/dev/null 2>&1; then \
	printf '!!! test-env-mutant FAIL: no-dedup mutant PASSED -- the UPSERT test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-env-mutant: green (no-dedup mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-er3h

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5829):
Prerequisites: `build/test_int21 build/.makefile.stamp`
```make
@printf ">>> test-er3h: AH=33h GET/SET CTRL-BREAK (DL normalize, out-of-scope AL, boot default ON) + CONFIG BREAK= flow (DEC-16)\n"
@$(TEST_INT21)
@printf ">>> test-er3h: green\n"
```

## test-er3h-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5835):
Prerequisites: `build/test_int21_mutant_er3h_nodisp build/test_int21_mutant_er3h_dlraw build/test_int21_mutant_er3h_defoff build/test_int21_mutant_er3h_setal build/.makefile.stamp`
```make
@printf ">>> test-er3h-mutant: confirming all four AH=33h mutants go RED (Rule 6)\n"
@if $(TEST_INT21_MUT_ER3H_NODISP) >/dev/null 2>&1; then \
	printf '!!! test-er3h-mutant FAIL: AH=33h-no-dispatch mutant PASSED -- the AH=33h GET/SET oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-er3h-mutant: green (AH=33h-no-dispatch mutant correctly RED)\n'; \
fi
@if $(TEST_INT21_MUT_ER3H_DLRAW) >/dev/null 2>&1; then \
	printf '!!! test-er3h-mutant FAIL: DL-raw mutant PASSED -- the DL-normalization (DEC-16 3.2) oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-er3h-mutant: green (DL-raw mutant correctly RED -- the normalize-on-SET oracle bites)\n'; \
fi
@if $(TEST_INT21_MUT_ER3H_DEFOFF) >/dev/null 2>&1; then \
	printf '!!! test-er3h-mutant FAIL: default-OFF mutant PASSED -- the boot-default-ON (DEC-16 3.3) oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-er3h-mutant: green (default-OFF mutant correctly RED -- the boot-default oracle bites)\n'; \
fi
@if $(TEST_INT21_MUT_ER3H_SETAL) >/dev/null 2>&1; then \
	printf '!!! test-er3h-mutant FAIL: SET-writes-AL mutant PASSED -- the no-output-register (DEC-16 3.2) oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-er3h-mutant: green (SET-writes-AL mutant correctly RED -- the SET no-output-register contract bites)\n'; \
fi
```

## test-exec

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25824):
Prerequisites: `build/qemu_harness build/exec_boot.img build/fat_exec.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-exec : LOAD A .COM FROM THE FAT VOLUME\n'
@printf '  Ref: beads initech-saw; psp-loader-ground-truth.md Sec 4/5; DOS 3.3 PRM 4Bh/4Dh.\n'
@printf '  Prove a flat .COM (GREET.COM, NOT baked) is loaded BY NAME from FAT and runs,\n'
@printf '  via load_program_from_fat (saw core) AND INT 21h AH=4Bh EXEC + AH=4Dh.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (primary slave; carries GREET.COM)\n' "$(EXEC_IMG)" "$(FAT_EXEC_IMG)"
@printf 'Expecting : "%s" + EXEC-SAW-EXIT rc=7 + EXEC-4B-RC=7 + no triple-fault\n' "$(EXEC_OUTPUT)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_EXEC_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@$(HARNESS_BIN) --disk "$(EXEC_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(EXEC_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(EXEC_REPORT)" || true
@cat "$(EXEC_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(EXEC_REPORT)"; then \
	printf '!!! test-exec FAIL: TRIPLE FAULT -- the FAT load or control transfer crashed\n'; \
	exit 1; \
fi
@printf '>>> test-exec [1/4]: no triple-fault\n'
@if [ ! -s "$(EXEC_SERIAL)" ]; then \
	printf '!!! test-exec FAIL: no serial captured at %s\n' "$(EXEC_SERIAL)"; exit 1; \
fi
@grep -q '^LOADER-FAT-BIND-OK$$' "$(EXEC_SERIAL)" \
	|| { printf '!!! test-exec FAIL: LOADER-FAT-BIND-OK missing -- the loader FAT volume never bound\n'; exit 1; }
@printf '>>> test-exec [2/4]: LOADER-FAT-BIND-OK (the loader can read .COMs off the volume)\n'
@# ---- 2a. THE SAW CORE: GREET.COM loaded FROM FAT, ran, exited rc=7. ----
@if grep -q '^EXEC-SAW-FAIL' "$(EXEC_SERIAL)"; then \
	printf '!!! test-exec FAIL: EXEC-SAW-FAIL -- load_program_from_fat("GREET.COM") failed (root-cause the saw path, Rule 3):\n'; \
	grep '^EXEC-SAW-FAIL' "$(EXEC_SERIAL)"; \
	exit 1; \
fi
@grep -qF '$(EXEC_OUTPUT)' "$(EXEC_SERIAL)" \
	|| { printf '!!! test-exec FAIL: "%s" missing -- the .COM loaded from FAT did not run/print via INT 21h\n' "$(EXEC_OUTPUT)"; exit 1; }
@grep -q '^EXEC-SAW-EXIT rc=7$$' "$(EXEC_SERIAL)" \
	|| { printf '!!! test-exec FAIL: EXEC-SAW-EXIT rc=7 missing -- the FROM-FAT program did not return rc=7 to the loader\n'; exit 1; }
@printf '>>> test-exec [3/4]: GREET.COM loaded FROM FAT, ran (printed via INT 21h), and returned rc=7\n'
@# ---- 2b. INT 21h AH=4Bh EXEC + AH=4Dh GET-RETURN-CODE from kernel ctx. ----
@if grep -q '^EXEC-4B-FAIL' "$(EXEC_SERIAL)"; then \
	printf '!!! test-exec FAIL: EXEC-4B-FAIL -- INT 21h AH=4Bh EXEC of GREET.COM failed (root-cause do_exec / the seam, Rule 3):\n'; \
	grep '^EXEC-4B-FAIL' "$(EXEC_SERIAL)"; \
	exit 1; \
fi
@grep -q '^EXEC-4B-RC=7$$' "$(EXEC_SERIAL)" \
	|| { printf '!!! test-exec FAIL: EXEC-4B-RC=7 missing -- AH=4Bh EXEC + AH=4Dh did not return the child rc=7\n'; exit 1; }
@printf '>>> test-exec [4/4]: INT 21h AH=4Bh EXEC ran GREET.COM; AH=4Dh returned rc=7\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS loaded a flat .COM BY NAME from a FAT12 volume and ran it (saw + AH=4Bh EXEC)\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
@printf '>>> test-exec: green\n'
```

## test-exec-high

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 29):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_exec_high.img build/.makefile.stamp`
```make
$(call exec-high-boot,$(SECONDARY_TRACER_IMG),exec_high)
@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high
@printf '>>> test-exec-high: green (8 MiB, high fixup/entry/heap/stack, KEEP and second-high refusals, conventional child, reclaim, legacy low, oversize)\n'
```

## test-exec-high-bochs

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/exec_high.mk', line 62):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/fat_exec_high.img build/.makefile.stamp`
```make
@cp -f $(BUILD)/fat_exec_high.img $(BUILD)/exec_high_bochs_data.img
@INITECH_TEST_RAM_MB=$(EXEC_HIGH_RAM_MB) $(BOCHS_BIN) --disk $(SECONDARY_TRACER_IMG) --disk2 $(BUILD)/exec_high_bochs_data.img --name exec_high_bochs --out $(BUILD) --keys '$(EXEC_HIGH_BOCHS_KEYS)' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 60000 2>$(BUILD)/exec_high_bochs.report || true
@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_bochs
@printf '>>> test-exec-high-bochs: green (8 MiB; same high allocation/return oracle as QEMU)\n'
```

## test-exec-high-dirty

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 181):
Prerequisites: `build/qemu_harness build/tsr_exec_high_dirty.img build/fat_exec_high.img build/.makefile.stamp`
```make
$(call exec-high-boot,$(BUILD)/tsr_exec_high_dirty.img,exec_high_dirty)
@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_dirty
@printf '>>> test-exec-high-dirty: green (A5/5A dirty high RAM zeroed before load; heap/reserve/stack and guard verified on both launches)\n'
```

## test-exec-high-exits

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 198):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_exec_high.img build/exec_high_exit00.exe build/exec_high_exit20.exe build/.makefile.stamp`
```make
@set -eu; for mode in 00 20; do s=$(BUILD)/exec_high_exit$$mode; \
 cp -f $(BUILD)/fat_exec_high.img $$s-data.img; \
 mcopy -o -i $$s-data.img $(BUILD)/exec_high_exit$$mode.exe ::HIGH.EXE; \
 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(SECONDARY_TRACER_IMG) --disk2 $$s-data.img --name exec_high_exit$$mode --out $(BUILD) --mouse '$(EXEC_HIGH_KEYS)' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 45000 2>$$s.report || true; \
 sh harness/emu/exec_high_check.sh $$s; done
@cp -f $(BUILD)/fat_exec_high.img $(BUILD)/exec_high_alias-data.img
@mcopy -o -i $(BUILD)/exec_high_alias-data.img $(BUILD)/exec_high.exe ::HIGH.COM
@INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(SECONDARY_TRACER_IMG) --disk2 $(BUILD)/exec_high_alias-data.img --name exec_high_alias --out $(BUILD) --mouse '$(EXEC_HIGH_ALIAS_KEYS)' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 45000 2>$(BUILD)/exec_high_alias.report || true
@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_alias
@printf '>>> test-exec-high-exits: green (AH=00h and INT20 each reclaim twice; extended content also dispatches high under .COM filename)\n'
```

## test-exec-high-io

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 177):
Prerequisites: `build/qemu_harness build/tsr_exec_high_io.img build/fat_exec_high.img`
```make
$(call exec-high-boot,$(BUILD)/tsr_exec_high_io.img,exec_high_io)
@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_io
@printf '>>> test-exec-high-io: green (one failed read after high writes never transfers; subsequent two launches reclaim low/native resources)\n'
```

## test-exec-high-lowmem

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 33):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@INITECH_TEST_RAM_MB=$(EXEC_HIGH_LOW_RAM_MB) $(HARNESS_BIN) --disk $(SECONDARY_TRACER_IMG) --name exec_high_lowmem --out $(BUILD) --quit-after HALTED --timeout-ms 10000 2>$(BUILD)/exec_high_lowmem.report || true
@grep -q 'PC LOAD LETTER.*Insufficient memory' $(BUILD)/exec_high_lowmem.serial || { printf '!!! test-exec-high-lowmem RED: missing MSG-DOS-0035 before high copy\n'; exit 1; }
@grep -q 'ext_mem_kb=4096 required_kb=7168' $(BUILD)/exec_high_lowmem.serial
@! grep -q 'FLAIR-HEAP-OK\|SHELL-READY' $(BUILD)/exec_high_lowmem.serial
@printf '>>> test-exec-high-lowmem: green (5 MiB refuses with MSG-DOS-0035 before desktop/high writes)\n'
```

## test-exec-high-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 52):
Prerequisites: `test-exec-high build/tsr_exec_high_low.img build/tsr_exec_high_owner.img build/tsr_exec_high_reclaim.img build/.makefile.stamp`
```make
$(call exec-high-boot,$(BUILD)/tsr_exec_high_low.img,exec_high_mut_low)
$(call exec-high-boot,$(BUILD)/tsr_exec_high_owner.img,exec_high_mut_owner)
$(call exec-high-boot,$(BUILD)/tsr_exec_high_reclaim.img,exec_high_mut_reclaim)
@set -eu; for m in low owner reclaim; do s=$(BUILD)/exec_high_mut_$$m; \
 if sh harness/emu/exec_high_check.sh $$s > $$s.check 2>&1; then printf '!!! high mutant %s survived\n' $$m; exit 1; fi; \
 grep -q '^SHELL-READY' $$s.serial; grep '^!!! test-exec-high RED:' $$s.check; \
 printf '>>> test-exec-high-mutant: %s RED after successful boot\n' $$m; done
@printf '>>> test-exec-high-mutant: green (dispatch low, no single-owner refusal, skipped reclaim all RED)\n'
```

## test-exec-high-ram

Declared tier: EMU; #  recipe to execute (from 'harness/emu/exec_high.mk', line 80):
Prerequisites: `build/qemu_harness build/exec_high_ram_below.img build/exec_high_ram_exact.img build/exec_high_ram_hole.img build/exec_high_ram_lowhole.img build/.makefile.stamp`
```make
@set -eu; for m in below hole lowhole; do ram=8; test $$m != hole || ram=$(EXEC_HIGH_HOLE_RAM_MB); \
 INITECH_TEST_RAM_MB=$$ram $(HARNESS_BIN) --disk $(BUILD)/exec_high_ram_$$m.img --name exec_high_ram_$$m --out $(BUILD) --quit-after HALTED --timeout-ms 5000 2>$(BUILD)/exec_high_ram_$$m.report || true; \
 grep -q 'PC LOAD LETTER.*Insufficient memory' $(BUILD)/exec_high_ram_$$m.serial; \
 ! grep -q '^KHI\|^FLAIR-HEAP-OK' $(BUILD)/exec_high_ram_$$m.serial; done
@grep -q 'ext_mem_kb=7167 required_kb=7168' $(BUILD)/exec_high_ram_below.serial
@INITECH_TEST_RAM_MB=8 $(HARNESS_BIN) --disk $(BUILD)/exec_high_ram_exact.img --name exec_high_ram_exact --out $(BUILD) --expect SHELL-DONE --keys e,x,i,t,ret --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 5000 2>$(BUILD)/exec_high_ram_exact.report
@grep -q '^SHELL-READY' $(BUILD)/exec_high_ram_exact.serial
@printf '>>> test-exec-high-ram: green (7167 RED, 7168 GREEN, known E820 hole RED before copy to high kernel)\n'
```

## test-exec-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7128):
Prerequisites: `build/test_exec_mutant_rc build/test_exec_mutant_pathscan build/.makefile.stamp`
```make
@printf ">>> test-exec-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_EXEC_MUT_RC) >/dev/null 2>&1; then \
	printf '!!! test-exec-mutant FAIL: 4Dh-rc-zero mutant PASSED -- the GET-RETURN-CODE test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-exec-mutant: green (4Dh-rc-zero mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_EXEC_MUT_PATHSCAN) >/dev/null 2>&1; then \
	printf '!!! test-exec-mutant FAIL: pathscan-nobound mutant PASSED -- the overlength-path guard test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-exec-mutant: green (pathscan-nobound mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-exec-nested

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35514):
Prerequisites: `build/qemu_harness build/exec_nested.img build/fat_exec_nested.img build/.makefile.stamp`
```make
@$(call fixture-preflight,$(BUILD)/fat_exec_nested.img)
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/exec_nested.img --disk2 $(BUILD)/fat_exec_nested.img --expect 'EXEC-NESTED-EXIT rc=0' --name exec_nested --out $(BUILD) --timeout-ms 6000 2> $(BUILD)/exec_nested.report || true
@! grep -q 'triple_fault=1' $(BUILD)/exec_nested.report
@grep -q '^EXEC-NESTED-RC=7 PSP-RESTORED INT20-OK' $(BUILD)/exec_nested.serial && grep -q '^EXEC-NESTED-DEPTH-4-BUSY' $(BUILD)/exec_nested.serial && grep -q '^EXEC-NESTED-EXIT rc=0' $(BUILD)/exec_nested.serial || { cat $(BUILD)/exec_nested.serial; printf '!!! test-exec-nested RED: caller did not resume with child rc=7 and restored PSP\n'; exit 1; }
@printf '>>> test-exec-nested: green (GREET rc=7 three times; PSP, CWD, heap restored; INT20; depth-4 BUSY)\n'
```

## test-exec-nested-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35520):
Prerequisites: `test-exec-nested build/exec_nested_mut.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/exec_nested_mut.img --disk2 $(BUILD)/fat_exec_nested.img --expect 'EXEC-NESTED-EXIT rc=1' --name exec_nested_mut --out $(BUILD) --timeout-ms 6000 2> $(BUILD)/exec_nested_mut.report || true
@! grep -q 'triple_fault=1' $(BUILD)/exec_nested_mut.report
@grep -q '^EXEC-NESTED-FAIL' $(BUILD)/exec_nested_mut.serial && grep -q '^EXEC-NESTED-EXIT rc=1' $(BUILD)/exec_nested_mut.serial || { cat $(BUILD)/exec_nested_mut.serial; printf '!!! test-exec-nested-mutant: mutant survived or failed infrastructure\n'; exit 1; }
@printf '>>> test-exec-nested-mutant: green (LOADER_MUT_NO_RESTORE RED: EXEC-NESTED-FAIL, EXEC-NESTED-EXIT rc=1)\n'
```

## test-exec-overlay

Declared tier: EMU; #  recipe to execute (from 'harness/exec_overlay.mk', line 168):
Prerequisites: `build/qemu_harness build/exec_overlay.img build/fat_ovl.img build/fat_ovl_foreign.img test-exec-overlay-flair test-overlay-memory build/.makefile.stamp`
```make
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
```

## test-exec-overlay-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/exec_overlay.mk', line 180):
Prerequisites: `test-exec-overlay build/exec_overlay_mut_nofactor.img build/exec_overlay_mut_foreign.img build/exec_overlay_mut_obo.img test-exec-overlay-flair-mutant build/.makefile.stamp`
```make
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
```

## test-exec-unit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7122):
Prerequisites: `build/test_exec build/.makefile.stamp`
```make
@printf ">>> test-exec-unit: AH=4Bh EXEC (path validation + not-found/nested mapping) + AH=4Dh GET-RETURN-CODE via mock backend\n"
@$(TEST_EXEC)
@printf ">>> test-exec-unit: green\n"
```

## test-exit-handles

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26475):
Prerequisites: `build/qemu_harness build/exith_boot.img build/fat_exith.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-exit-handles : EXIT reclaims a process handles\n'
@printf '  Ref: beads initech-6hk / epic initech-6qy; sft_close_process on 4Ch/00h/INT20h.\n'
@printf '  EXEC a leaky child (4 opens, no close) %s times; 4*%s=%s > 16 file slots.\n' "$(EXITH_RUNS)" "$(EXITH_RUNS)" "$$(( 4 * $(EXITH_RUNS) ))"
@printf '======================================================================\n'
@printf 'Booting   : %s + leaky-child disk %s (primary slave)\n' "$(EXITH_IMG)" "$(FAT_EXITH_IMG)"
@printf 'Expecting : EXITH-RUN1..%s-OK + EXITH-DONE rc=0 + no triple-fault\n' "$(EXITH_RUNS)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_EXITH_IMG))
@$(HARNESS_BIN) --disk "$(EXITH_IMG)" --disk2 "$(FAT_EXITH_IMG)" \
	--name "$(EXITH_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(EXITH_REPORT)" || true
@cat "$(EXITH_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(EXITH_REPORT)"; then \
	printf '!!! test-exit-handles FAIL: TRIPLE FAULT (root-cause the EXIT teardown path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-exit-handles [1/4]: no triple-fault\n'
@if [ ! -s "$(EXITH_SERIAL)" ]; then \
	printf '!!! test-exit-handles FAIL: no serial captured at %s\n' "$(EXITH_SERIAL)"; exit 1; \
fi
@if grep -q 'EXITH-CHILD-OPENFAIL' "$(EXITH_SERIAL)"; then \
	printf '!!! test-exit-handles FAIL: EXITH-CHILD-OPENFAIL -- a child OPEN failed (the SFT exhausted -> handles leaked on EXIT, Rule 3):\n'; \
	grep -n 'EXITH-RUN\|EXITH-CHILD' "$(EXITH_SERIAL)"; exit 1; \
fi
@if grep -q 'EXITH-RUN[0-9]*-FAIL' "$(EXITH_SERIAL)"; then \
	printf '!!! test-exit-handles FAIL: an EXITH-RUN*-FAIL marker -- a run did not complete clean (the leak resurfaced, Rule 3):\n'; \
	grep -n 'EXITH-RUN.*-FAIL' "$(EXITH_SERIAL)"; exit 1; \
fi
@printf '>>> test-exit-handles [2/4]: no EXITH-CHILD-OPENFAIL and no EXITH-RUN*-FAIL\n'
@n=1; while [ "$$n" -le "$(EXITH_RUNS)" ]; do \
	grep -q "^EXITH-RUN$$n-OK$$" "$(EXITH_SERIAL)" \
		|| { printf '!!! test-exit-handles FAIL: EXITH-RUN%s-OK missing -- run %s did not EXEC a clean leaky child\n' "$$n" "$$n"; \
		     grep -n 'EXITH-RUN' "$(EXITH_SERIAL)"; exit 1; }; \
	n=$$(( n + 1 )); \
done
@printf '>>> test-exit-handles [3/4]: all %s runs OK (EXITH-RUN1..%s-OK) -- EXIT reclaimed every run handles\n' "$(EXITH_RUNS)" "$(EXITH_RUNS)"
@grep -q '^EXITH-DONE rc=0$$' "$(EXITH_SERIAL)" \
	|| { printf '!!! test-exit-handles FAIL: EXITH-DONE rc=0 missing -- the sweep did not finish clean\n'; \
	     grep -n 'EXITH-DONE' "$(EXITH_SERIAL)"; exit 1; }
@printf '>>> test-exit-handles [4/4]: EXITH-DONE rc=0 (clean sweep of all %s runs)\n' "$(EXITH_RUNS)"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS reclaims an exiting process file handles on 4Ch,\n'
@printf '            so a %s-deep EXEC chain of a leaky child (24 opens) never exhausts the\n' "$(EXITH_RUNS)"
@printf '            16-slot file SFT (QEMU only; tri-emulator pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-exit-handles-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26538):
Prerequisites: `build/qemu_harness build/exith_mut_boot.img build/fat_exith.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-exit-handles-mutant : prove the oracle bites (Rule 6)\n'
@printf '  sft_close_process elided -> handles leak across EXEC runs -> SFT exhausts.\n'
@printf '======================================================================\n'
@$(call fixture-preflight,$(FAT_EXITH_IMG))
@$(HARNESS_BIN) --disk "$(EXITH_MUT_IMG)" --disk2 "$(FAT_EXITH_IMG)" \
	--name "$(EXITH_MUT_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(EXITH_MUT_REPORT)" || true
@cat "$(EXITH_MUT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if [ ! -s "$(EXITH_MUT_SERIAL)" ]; then \
	printf '!!! test-exit-handles-mutant FAIL: no serial captured (cannot judge the mutant)\n'; exit 1; \
fi
@if grep -q 'EXITH-CHILD-OPENFAIL' "$(EXITH_MUT_SERIAL)" \
   || grep -q 'EXITH-RUN[0-9]*-FAIL' "$(EXITH_MUT_SERIAL)" \
   || ! grep -q '^EXITH-DONE rc=0$$' "$(EXITH_MUT_SERIAL)"; then \
	printf '>>> test-exit-handles-mutant: green (mutant correctly RED -- a run OPEN failed / no clean DONE; the oracle bites):\n'; \
	grep -n 'EXITH-RUN\|EXITH-CHILD\|EXITH-DONE' "$(EXITH_MUT_SERIAL)" || true; \
else \
	printf '!!! test-exit-handles-mutant FAIL: the NO-CLOSE mutant PASSED clean -- test-exit-handles is DECORATION\n'; \
	grep -n 'EXITH-RUN\|EXITH-CHILD\|EXITH-DONE' "$(EXITH_MUT_SERIAL)" || true; \
	exit 1; \
fi
@printf '======================================================================\n'
```

## test-f3template

Declared tier: EMU; #  recipe to execute (from 'harness/conkeys/f3template.mk', line 35):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_exec.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh harness/conkeys/f3template.sh $(BUILD) $(SECONDARY_TRACER_IMG) $(FAT_EXEC_IMG) f3template
```

## test-f3template-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/conkeys/f3template.mk', line 37):
Prerequisites: `build/qemu_harness build/f3template_mut.img build/fat_exec.img build/.makefile.stamp`
```make
@if INITECH_QMP_STDIO=1 sh harness/conkeys/f3template.sh $(BUILD) $(BUILD)/f3template_mut.img $(FAT_EXEC_IMG) f3template_mut \
	>$(BUILD)/f3template-emu-mutant.log 2>&1; then \
	printf '!!! test-f3template-mutant FAIL: CMD_MUT_NO_TEMPLATE survived QEMU\n'; exit 1; fi
@grep -Fx 'RED F3TEMPLATE repeat-count: expected 2, got 1' $(BUILD)/f3template-emu-mutant.log
@printf '>>> test-f3template-mutant: CMD_MUT_NO_TEMPLATE RED (QEMU REPEAT only once)\n'
```

## test-f3template-unit

Declared tier: HOST; #  recipe to execute (from 'harness/conkeys/f3template.mk', line 10):
Prerequisites: `test-kbd-unit test-conin-unit build/.makefile.stamp`
```make
@printf '>>> test-f3template-unit: green (00,3D ring + AH=0Ah full/partial template recall)\n'
```

## test-f3template-unit-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/conkeys/f3template.mk', line 12):
Prerequisites: `build/test_kbd_mutant_extended build/test_conin_mutant_template build/.makefile.stamp`
```make
@if $(TEST_KBD_MUT_EXT) >$(BUILD)/f3template-kbd-mutant.log 2>&1; then \
	printf '!!! test-f3template-unit-mutant FAIL: KBD_MUT_DROP_EXTENDED survived\n'; exit 1; fi
@grep -q 'FAIL.*F3 transport:' $(BUILD)/f3template-kbd-mutant.log
@printf '>>> test-f3template-unit-mutant: KBD_MUT_DROP_EXTENDED RED (F3 transport lost)\n'
@if $(F3_TEMPLATE_MUT) >$(BUILD)/f3template-cmd-mutant.log 2>&1; then \
	printf '!!! test-f3template-unit-mutant FAIL: CMD_MUT_NO_TEMPLATE survived\n'; exit 1; fi
@grep -q 'FAIL.*F3 template: returned command' $(BUILD)/f3template-cmd-mutant.log
@printf '>>> test-f3template-unit-mutant: CMD_MUT_NO_TEMPLATE RED (previous command lost)\n'
```

## test-fat

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16213):
Prerequisites: `build/fat_dump build/fat12_fixture.img harness/diff/fat_diff/fat12_ref.py build/test_fat12_bpb build/test_fat12_chain build/test_fat12_dir build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat : FAT12 differential oracle\n'
@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-5cu / initech-adf.\n'
@printf '  Our C reader vs TWO independent refs (mtools + python3) on one image.\n'
@printf '======================================================================\n'
@# ---- (D) Tool presence: fail loud, NEVER silently skip (Law 2). ----
@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat FAIL: python3 not found (needed for the independent reference reader).\n'; exit 1; }
@printf '>>> test-fat [D]: reference tools present (mtools mdir/mcopy + python3)\n'
@# ---- (C) The three FAT12 unit oracles must pass first. ----
@printf '>>> test-fat [C]: FAT12 unit oracles (bpb / chain / dir)\n'
@$(TEST_FAT12_BPB)   "$(FAT12_IMG)" \
	|| { printf '!!! test-fat FAIL: test_fat12_bpb red\n'; exit 1; }
@$(TEST_FAT12_CHAIN) "$(FAT12_IMG)" \
	|| { printf '!!! test-fat FAIL: test_fat12_chain red\n'; exit 1; }
@$(TEST_FAT12_DIR)   "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" \
	|| { printf '!!! test-fat FAIL: test_fat12_dir red\n'; exit 1; }
@# ---- (A) NAMES + SIZES: triple agreement (our == python == mdir). ----
@printf '>>> test-fat [A]: names+sizes -- our fat_dump == python ref == normalized mdir\n'
@$(FAT_DUMP_BIN) "$(FAT12_IMG)" --list > "$(FAT12_OUR_LIST)" \
	|| { printf '!!! test-fat FAIL: fat_dump --list errored\n'; exit 1; }
@python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --list > "$(FAT12_PY_LIST)" \
	|| { printf '!!! test-fat FAIL: fat12_ref.py --list errored\n'; exit 1; }
@# Normalize mdir: keep only rows carrying a YYYY-MM-DD date (real entries),
@# reassemble the space-grouped thousands in the size, drop date/time/serial,
@# emit "NAME.EXT <size>" and sort -- timestamps/serial normalized away.
@mdir -i "$(FAT12_IMG)" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$$1; ext=$$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $$i } \
	if(sz !~ /^[0-9]+$$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "$(FAT12_MTOOLS_LIST)"
@diff -u "$(FAT12_OUR_LIST)" "$(FAT12_PY_LIST)" \
	|| { printf '!!! test-fat FAIL [A]: our listing != python reference listing\n'; exit 1; }
@diff -u "$(FAT12_OUR_LIST)" "$(FAT12_MTOOLS_LIST)" \
	|| { printf '!!! test-fat FAIL [A]: our listing != normalized mdir listing\n'; exit 1; }
@printf '    triple agreement on names+sizes (%s files):\n' "$$(wc -l < "$(FAT12_OUR_LIST)" | tr -d ' ')"
@sed 's/^/      /' "$(FAT12_OUR_LIST)"
@# ---- (B) CONTENT per file: our == mcopy == python, byte-for-byte. ----
@printf '>>> test-fat [B]: content -- our fat_dump --cat == mcopy == python ref, per file\n'
@for f in $(FAT12_GATE_NAMES); do \
	$(FAT_DUMP_BIN) "$(FAT12_IMG)" --cat "$$f" > "$(BUILD)/fat12_our_$$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: fat_dump --cat %s errored\n' "$$f"; exit 1; }; \
	mcopy -i "$(FAT12_IMG)" "::$$f" - > "$(BUILD)/fat12_mtools_$$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat FAIL [B]: mcopy ::%s errored\n' "$$f"; exit 1; }; \
	python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --cat "$$f" > "$(BUILD)/fat12_py_$$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
	cmp -s "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_mtools_$$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != mcopy bytes\n' "$$f"; cmp "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_mtools_$$f.bin"; exit 1; }; \
	cmp -s "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_py_$$f.bin" \
		|| { printf '!!! test-fat FAIL [B]: %s -- our bytes != python ref bytes\n' "$$f"; cmp "$(BUILD)/fat12_our_$$f.bin" "$(BUILD)/fat12_py_$$f.bin"; exit 1; }; \
	printf '    %-12s %6s bytes : our == mcopy == python\n' "$$f" "$$(wc -c < "$(BUILD)/fat12_our_$$f.bin" | tr -d ' ')"; \
done
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- our FAT12 reader agrees with mtools AND an independent\n'
@printf '            python3 reader on names, sizes, and content (triple oracle).\n'
@printf '======================================================================\n'
```

## test-fat-corrupt-fuzz

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5187):
Prerequisites: `build/fat12_corrupt_fuzz build/fat12_fuzz_blank.img build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-corrupt-fuzz : FAT12 corruption fuzzer (deterministic seed sweep)\n'
@printf '======================================================================\n'
@printf '>>> test-fat-corrupt-fuzz: malformed FAT12 inputs over seeds %s..%s (fail-loud + bounded)\n' "$(FAT12_CORRUPT_SEEDS_LO)" "$(FAT12_CORRUPT_SEEDS_HI)"
@timeout 120 $(TEST_FAT12_CORRUPT_FUZZ) --sweep $(FAT12_CORRUPT_SEEDS_LO) $(FAT12_CORRUPT_SEEDS_HI) --ops $(FAT12_CORRUPT_OPS) "$(FUZZ_GATE_BLANK)" \
	|| { printf '!!! test-fat-corrupt-fuzz FAIL: a malformed input hung, went OOB, or was NOT rejected fail-loud -- root-cause it (Rule 3)\n'; rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch; exit 1; }
@rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch
@printf '>>> test-fat-corrupt-fuzz: green (every malformed FAT12 input failed loud + bounded -- no hang, no OOB)\n'
```

## test-fat-corrupt-fuzz-chain-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/proptest/kernel_size_r7.mk', line 51):
Prerequisites: `test-fat-corrupt-fuzz build/ksize7/fat12_corrupt_fuzz_mut_chain build/fat12_fuzz_blank.img build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@if timeout 120 $(KERNEL_R7_DIR)/fat12_corrupt_fuzz_mut_chain --sweep $(FAT12_CORRUPT_SEEDS_LO) $(FAT12_CORRUPT_SEEDS_HI) --ops $(FAT12_CORRUPT_OPS) "$(FUZZ_GATE_BLANK)" >$(KERNEL_R7_DIR)/chain_mutant.log 2>&1; then \
	rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch; printf '!!! test-fat-corrupt-fuzz-chain-mutant FAIL: FAT12_MUT_CHAIN_GUARD survived\n'; exit 1; fi
@rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch
@grep -m1 'VIOLATION.*out-of-range' $(KERNEL_R7_DIR)/chain_mutant.log
@printf '>>> test-fat-corrupt-fuzz-chain-mutant: green (FAT12_MUT_CHAIN_GUARD RED)\n'
```

## test-fat-corrupt-fuzz-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5206):
Prerequisites: `build/fat12_corrupt_fuzz_mut_step build/fat12_corrupt_fuzz_mut_bpb build/fat12_fuzz_blank.img build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@printf '>>> test-fat-corrupt-fuzz-mutant: confirming the fuzzer DETECTS the no-step-guard + accept-bad-BPB mutants (Rule 6)\n'
@if timeout 120 $(TEST_FAT12_CORRUPT_FUZZ_MUT_STEP) --sweep $(FAT12_CORRUPT_SEEDS_LO) $(FAT12_CORRUPT_SEEDS_HI) --ops $(FAT12_CORRUPT_OPS) "$(FUZZ_GATE_BLANK)" >/dev/null 2>&1; then \
	printf '!!! test-fat-corrupt-fuzz-mutant FAIL: no-step-guard mutant PASSED the fuzzer -- the fuzzer is decoration\n'; \
	rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch; \
	exit 1; \
else \
	printf '>>> test-fat-corrupt-fuzz-mutant: green leg 1/2 (fuzzer RED on the no-step-guard mutant)\n'; \
fi
@if timeout 120 $(TEST_FAT12_CORRUPT_FUZZ_MUT_BPB) --sweep $(FAT12_CORRUPT_SEEDS_LO) $(FAT12_CORRUPT_SEEDS_HI) --ops $(FAT12_CORRUPT_OPS) "$(FUZZ_GATE_BLANK)" >/dev/null 2>&1; then \
	printf '!!! test-fat-corrupt-fuzz-mutant FAIL: accept-bad-BPB mutant PASSED the fuzzer -- the fuzzer is decoration\n'; \
	rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch; \
	exit 1; \
else \
	printf '>>> test-fat-corrupt-fuzz-mutant: green leg 2/2 (fuzzer RED on the accept-bad-BPB mutant)\n'; \
fi
@rm -f "$(FUZZ_GATE_BLANK)".cseed*.scratch
@printf '>>> test-fat-corrupt-fuzz-mutant: green (fuzzer correctly RED on both corruption mutants)\n'
```

## test-fat-fault-rollback

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4617):
Prerequisites: `build/test_fat12_fault_rollback build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-fault-rollback FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@printf ">>> test-fat-fault-rollback: FAT12 write-fault rollback atomicity -- nothing leaks (beads initech-lpf3)\n"
@dd if=/dev/zero of=$(FAT12_LPF3_FAULT_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_LPF3_FAULT_IMG) -f 1440 ::
@$(TEST_LPF3_FAULT) "$(FAT12_LPF3_FAULT_IMG)"
@printf ">>> test-fat-fault-rollback: green\n"
```

## test-fat-fault-rollback-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4631):
Prerequisites: `build/test_fat12_fault_rollback_mut_writefile build/test_fat12_fault_rollback_mut_growdir build/test_fat12_fault_rollback_mut_mkdir build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-fault-rollback-mutant FAIL: mtools `mformat` not found.\n'; exit 1; }
@printf ">>> test-fat-fault-rollback-mutant: confirming the THREE rollback mutants go RED for the RIGHT reason (Rule 6; beads initech-lpf3)\n"
@for spec in \
	"$(TEST_LPF3_FAULT_MUT_A)|m-writefile-noroll|A] in-memory free-cluster count restored" \
	"$(TEST_LPF3_FAULT_MUT_B)|m-growdir-noroll|B] in-memory free-cluster count restored" \
	"$(TEST_LPF3_FAULT_MUT_C)|m-mkdir-noroll|C] free-cluster count restored"; do \
	bin=`echo "$$spec" | cut -d'|' -f1`; \
	name=`echo "$$spec" | cut -d'|' -f2`; \
	want=`echo "$$spec" | cut -d'|' -f3`; \
	dd if=/dev/zero of=$(FAT12_LPF3_FAULT_IMG) bs=512 count=2880 status=none || exit 1; \
	mformat -i $(FAT12_LPF3_FAULT_IMG) -f 1440 :: || exit 1; \
	out=`"$$bin" "$(FAT12_LPF3_FAULT_IMG)" 2>&1`; rc=$$?; \
	echo "$$out" | grep -q 'checks,' \
		|| { printf '!!! test-fat-fault-rollback-mutant FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if [ $$rc -eq 0 ]; then \
		printf '!!! test-fat-fault-rollback-mutant FAIL: %s PASSED -- the fault-injection oracle is decoration\n' "$$name"; exit 1; \
	fi; \
	echo "$$out" | grep -qF "$$want" \
		|| { printf '!!! test-fat-fault-rollback-mutant FAIL: %s RED but NOT on its target leak assertion -- wrong reason\n' "$$name"; exit 1; }; \
	printf '>>> test-fat-fault-rollback-mutant: green (%s correctly RED -- its rollback leak bites)\n' "$$name"; \
done
@printf '>>> test-fat-fault-rollback-mutant: green (ALL THREE rollback mutants ran + RED for the right reason)\n'
```

## test-fat-fuzz

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5105):
Prerequisites: `build/fat12_fuzz build/fat12_fuzz_blank.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-fuzz : FAT12 generative fuzzer (deterministic seed sweep)\n'
@printf '======================================================================\n'
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-fuzz FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-fuzz FAIL: python3 not found (needed for the independent reference).\n'; exit 1; }
@printf '>>> test-fat-fuzz [A]: model + remount + FAT-sync over seeds %s..%s (fast, every op)\n' "$(FAT12_FUZZ_SEEDS_LO)" "$(FAT12_FUZZ_SEEDS_HI)"
@$(TEST_FAT12_FUZZ) --sweep $(FAT12_FUZZ_SEEDS_LO) $(FAT12_FUZZ_SEEDS_HI) --ops $(FAT12_FUZZ_OPS) --no-external "$(FUZZ_GATE_BLANK)" \
	|| { printf '!!! test-fat-fuzz FAIL [A]: a seed diverged (shrunk reproducer above) -- root-cause it (Rule 3)\n'; exit 1; }
@printf '>>> test-fat-fuzz [B]: + mtools/python external cross-check over seeds %s..%s\n' "$(FAT12_FUZZ_SEEDS_LO)" "$(FAT12_FUZZ_EXT_HI)"
@$(TEST_FAT12_FUZZ) --sweep $(FAT12_FUZZ_SEEDS_LO) $(FAT12_FUZZ_EXT_HI) --ops $(FAT12_FUZZ_OPS) "$(FUZZ_GATE_BLANK)" \
	|| { printf '!!! test-fat-fuzz FAIL [B]: mtools/python disagree with the model (shrunk reproducer above)\n'; exit 1; }
@rm -f "$(FUZZ_GATE_BLANK)".seed*.scratch "$(FUZZ_GATE_BLANK)".*.bin
@printf '>>> test-fat-fuzz: green (deterministic seed sweep -- model + remount + FAT-sync + mtools/python all agree)\n'
```

## test-fat-fuzz-gap-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 55):
Prerequisites: `build/fat12_fuzz_mut_stale_gap build/fat12_fuzz_blank.img build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@mkdir -p $(BUILD)/dos-volumes
@if $< --sweep 7 7 --ops $(FAT12_FUZZ_OPS) --no-external "$(FUZZ_GATE_BLANK)" > $(BUILD)/dos-volumes/stale-gap.log 2>&1; then echo 'FAIL STALE_GAP survived fuzzer'; exit 1; fi
@grep 'DIVERGENCE on seed 7' $(BUILD)/dos-volumes/stale-gap.log
@grep 'SHRUNK REPRODUCER (seed=7)' $(BUILD)/dos-volumes/stale-gap.log
@echo 'GREEN test-fat-fuzz-gap-mutant: STALE_GAP found and shrunk'
```

## test-fat-fuzz-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5125):
Prerequisites: `build/fat12_fuzz_mut_rmw build/fat12_fuzz_blank.img build/.makefile.stamp`
```make
@cp -f $(FAT12_FUZZ_BLANK_IMG) $(FUZZ_GATE_BLANK)
@printf '>>> test-fat-fuzz-mutant: confirming the fuzzer FINDS + SHRINKS the no-RMW mutant (Rule 6)\n'
@if $(TEST_FAT12_FUZZ_MUT_RMW) --sweep $(FAT12_FUZZ_SEEDS_LO) $(FAT12_FUZZ_SEEDS_HI) --ops $(FAT12_FUZZ_OPS) --no-external "$(FUZZ_GATE_BLANK)" >/dev/null 2>&1; then \
	printf '!!! test-fat-fuzz-mutant FAIL: no-RMW mutant PASSED the fuzzer -- the fuzzer is decoration\n'; \
	rm -f "$(FUZZ_GATE_BLANK)".seed*.scratch; \
	exit 1; \
else \
	printf '>>> test-fat-fuzz-mutant: green (fuzzer correctly RED on the no-RMW mutant; shrunk reproducer:)\n'; \
	$(TEST_FAT12_FUZZ_MUT_RMW) --sweep $(FAT12_FUZZ_SEEDS_LO) $(FAT12_FUZZ_SEEDS_HI) --ops $(FAT12_FUZZ_OPS) --no-external "$(FUZZ_GATE_BLANK)" 2>&1 | sed -n '/SHRUNK/,/----/p' || true; \
	rm -f "$(FUZZ_GATE_BLANK)".seed*.scratch; \
fi
```

## test-fat-hygiene-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 46):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/hygiene_probe.bin test-fat-hygiene-grow build/.makefile.stamp | build/khygiene-evidence`
```make
@sh harness/diff/fat_diff/kernel_hygiene_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG)
```

## test-fat-hygiene-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 237):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-hygiene-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-fat-hygiene-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/fat-hygiene-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/fat-hygiene-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "fat-hygiene-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/fat-hygiene-emu_bochs.report.txt" || true
@cat "$(BUILD)/fat-hygiene-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-fat-hygiene-emu-bochs "$(BUILD)/fat-hygiene-emu_bochs.report.txt" "$(BUILD)/fat-hygiene-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-fat-hygiene-grow

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 9):
Prerequisites: `build/test_kernel_hygiene build/.makefile.stamp | build/khygiene-evidence`
```make
@sh harness/diff/fat_diff/kernel_hygiene.sh $(patsubst test-fat-hygiene-%,%,$@) $(BUILD) $<
```

## test-fat-hygiene-grow-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 181):
Prerequisites: `build/qemu_harness build/hygiene_grow.img build/hygiene_probe.bin test-fat-hygiene-grow build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/kernel_hygiene_emu.sh $(BUILD) build/hygiene_grow.img -grow > build/khygiene-evidence/mutant-emu-grow.log 2>&1; then echo 'FAIL FAT grow emu mutant survived'; exit 1; fi
@grep 'RED FAT hygiene emu: missing KHY-GROW-OK' build/khygiene-evidence/mutant-emu-grow.log
@grep -qx KHY-FAIL build/khygiene-emu-grow/serial.txt
@grep -qx SHELL-DONE build/khygiene-emu-grow/serial.txt
@echo 'GREEN test-fat-hygiene-grow-emu-mutant: $(KHY_FLAG_grow) correctly RED'
```

## test-fat-hygiene-grow-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 70):
Prerequisites: `build/test_kernel_hygiene_grow build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/kernel_hygiene.sh grow $(BUILD) $< > $(BUILD)/khygiene-evidence/mutant-fat-grow.log 2>&1; then echo 'FAIL grow mutant survived'; exit 1; fi
@grep 'FAIL.*fov8' $(BUILD)/khygiene-evidence/mutant-fat-grow.log
@echo 'GREEN test-fat-hygiene-grow-mutant: $(KHY_FLAG_grow) correctly RED'
```

## test-fat-hygiene-hidden

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 9):
Prerequisites: `build/test_kernel_hygiene build/.makefile.stamp | build/khygiene-evidence`
```make
@sh harness/diff/fat_diff/kernel_hygiene.sh $(patsubst test-fat-hygiene-%,%,$@) $(BUILD) $<
```

## test-fat-hygiene-hidden-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 71):
Prerequisites: `build/test_kernel_hygiene_hidden build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/kernel_hygiene.sh hidden $(BUILD) $< > $(BUILD)/khygiene-evidence/mutant-fat-hidden.log 2>&1; then echo 'FAIL hidden mutant survived'; exit 1; fi
@grep 'FAIL.*tdnl.30' $(BUILD)/khygiene-evidence/mutant-fat-hidden.log
@echo 'GREEN test-fat-hygiene-hidden-mutant: $(KHY_FLAG_hidden) correctly RED'
```

## test-fat-hygiene-rename

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 9):
Prerequisites: `build/test_kernel_hygiene build/.makefile.stamp | build/khygiene-evidence`
```make
@sh harness/diff/fat_diff/kernel_hygiene.sh $(patsubst test-fat-hygiene-%,%,$@) $(BUILD) $<
```

## test-fat-hygiene-rename-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 180):
Prerequisites: `build/qemu_harness build/hygiene_rename.img build/hygiene_probe.bin test-fat-hygiene-grow build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/kernel_hygiene_emu.sh $(BUILD) build/hygiene_rename.img -rename > build/khygiene-evidence/mutant-emu-rename.log 2>&1; then echo 'FAIL FAT rename emu mutant survived'; exit 1; fi
@grep 'RED FAT hygiene emu: missing KHY-RENAME-OK' build/khygiene-evidence/mutant-emu-rename.log
@grep -qx KHY-FAIL build/khygiene-emu-rename/serial.txt
@grep -qx SHELL-DONE build/khygiene-emu-rename/serial.txt
@echo 'GREEN test-fat-hygiene-rename-emu-mutant: $(KHY_FLAG_rename) correctly RED'
```

## test-fat-hygiene-rename-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 69):
Prerequisites: `build/test_kernel_hygiene_rename build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/kernel_hygiene.sh rename $(BUILD) $< > $(BUILD)/khygiene-evidence/mutant-fat-rename.log 2>&1; then echo 'FAIL rename mutant survived'; exit 1; fi
@grep 'FAIL.*f0hh' $(BUILD)/khygiene-evidence/mutant-fat-rename.log
@echo 'GREEN test-fat-hygiene-rename-mutant: $(KHY_FLAG_rename) correctly RED'
```

## test-fat-move

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4354):
Prerequisites: `build/test_fat12_move harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mmd     >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mmd` not found.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mcopy` not found.\n'; exit 1; }
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mdir` not found.\n'; exit 1; }
@command -v mtype   >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mtype` not found.\n'; exit 1; }
@command -v mlabel  >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: mtools `mlabel` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-move FAIL: python3 not found (the INDEPENDENT reference reader).\n'; exit 1; }
@printf ">>> test-fat-move: cross-directory dir-entry transplant + Trash suffix (beads initech-tdnl.26)\n"
$(MINT_FAT12_MOVE_IMAGES)
@mdir -i $(FAT12_MOVE_DIFF_IMG) :: | grep 'bytes free' > $(BUILD)/fat12_move_free_before.txt
@$(TEST_FAT12_MOVE) "$(FAT12_MOVE_DIFF_IMG)" "$(FAT12_MOVE_STRUCT_IMG)"
@printf '>>> test-fat-move [host]: transplant legs green -- chain/size/attr/timestamps, both FAT copies and the data clusters proven unchanged\n'
@mdir -i $(FAT12_MOVE_DIFF_IMG) :: | grep -q '^MOVEME' \
	&& { printf '!!! test-fat-move FAIL [mtools]: MOVEME.TXT is STILL in the root -- the source slot was not released\n'; exit 1; } || true
@mdir -i $(FAT12_MOVE_DIFF_IMG) :: | grep -q '^SUBFILE' \
	|| { printf '!!! test-fat-move FAIL [mtools]: SUBFILE.TXT is missing from the root -- the subdir->root move did not land\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::DST | grep -q '^MOVEME' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks MOVEME.TXT -- the root->subdir move did not land\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::DST | grep -q '^CROSS' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks CROSS.TXT -- the subdir->subdir move did not land\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::DST | grep -q '^MOVEDIR .*<DIR>' \
	|| { printf '!!! test-fat-move FAIL [mtools]: DST lacks the MOVEDIR <DIR> entry -- the directory move did not land\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::SRC | grep -qE '^(CROSS|SUBFILE|REPORT|MOVEDIR)' \
	&& { printf '!!! test-fat-move FAIL [mtools]: SRC still lists a moved entry -- the source slots were not released\n'; exit 1; } || true
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::TRASH | grep -q '^REPORT   DBF' \
	|| { printf '!!! test-fat-move FAIL [mtools]: the ORIGINAL TRASH/REPORT.DBF vanished -- staging clobbered it\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::TRASH | grep -q '^REPOR001 DBF' \
	|| { printf '!!! test-fat-move FAIL [mtools]: TRASH lacks the suffixed REPOR001.DBF -- the F1.4 collision ladder did not land\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) ::DST/MOVEDIR | grep -q '^INSIDE' \
	|| { printf '!!! test-fat-move FAIL [mtools]: the moved folder is not traversable at its new home (INSIDE.TXT unreachable)\n'; exit 1; }
@# The '..' fixup needs an INDEPENDENT reader, not mdir: mtools resolves
@# '..' TEXTUALLY from the path string, so `mdir ::DST/MOVEDIR/..` prints
@# DST's contents even when the on-disk '..' still names the OLD parent
@# (verified against the NO_DOTDOT mutant image -- that grep would have been
@# decoration, Rule 6). python3 reads the '..' bytes straight out of slot 1
@# of the folder's first cluster and we compare against the NEW parent's own
@# start cluster, both from first principles.
@dd_have=$$(python3 $(FAT12_REF_PY) $(FAT12_MOVE_DIFF_IMG) --dotdot 'DST\MOVEDIR') || \
	{ printf "!!! test-fat-move FAIL [python]: cannot read the moved folder's '..' entry\n"; exit 1; }; \
dd_want=$$(python3 $(FAT12_REF_PY) $(FAT12_MOVE_DIFF_IMG) --dirstart 'DST') || \
	{ printf '!!! test-fat-move FAIL [python]: cannot read DST start cluster\n'; exit 1; }; \
if [ "$$dd_have" != "$$dd_want" ]; then \
	printf "!!! test-fat-move FAIL [python]: the moved folder's '..' says cluster %s but its new parent DST is cluster %s -- the '..' fixup is wrong\n" "$$dd_have" "$$dd_want"; exit 1; \
fi
@python3 $(FAT12_REF_PY) $(FAT12_MOVE_DIFF_IMG) --list-path 'DST\MOVEDIR' | grep -q '^INSIDE.TXT ' \
	|| { printf '!!! test-fat-move FAIL [python]: the independent reader cannot enumerate the moved folder at its new home\n'; exit 1; }
@mtype -i $(FAT12_MOVE_DIFF_IMG) ::DST/MOVEME.TXT | cmp -s - $(FAT12_FIXTURE_DIR)/chain.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the moved file CONTENT differs from the seed body -- a move must not touch data\n'; exit 1; }
@mtype -i $(FAT12_MOVE_DIFF_IMG) ::TRASH/REPOR001.DBF | cmp -s - $(FAT12_FIXTURE_DIR)/hello.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the STAGED item content differs -- the rename-while-move touched data\n'; exit 1; }
@mtype -i $(FAT12_MOVE_DIFF_IMG) ::TRASH/REPORT.DBF | cmp -s - $(FAT12_FIXTURE_DIR)/second.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the ORIGINAL trashed item content changed -- the suffix ladder aliased it\n'; exit 1; }
@mdir -i $(FAT12_MOVE_DIFF_IMG) :: | grep 'bytes free' > $(BUILD)/fat12_move_free_after.txt
@cmp -s $(BUILD)/fat12_move_free_before.txt $(BUILD)/fat12_move_free_after.txt \
	|| { printf '!!! test-fat-move FAIL [mtools]: the free-space count CHANGED -- a same-volume move must spend no cluster\n'; exit 1; }
@printf '>>> test-fat-move [mtools]: entries transplanted, content identical, folder traversable, free space unchanged\n'
@printf ">>> test-fat-move: green\n"
```

## test-fat-move-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4422):
Prerequisites: `test-fat-move build/test_fat12_move_mut_relinks build/test_fat12_move_mut_loses build/test_fat12_move_mut_nodotdot build/test_fat12_move_mut_noroll build/test_fat12_move_mut_nosuffix build/test_fat12_move_mut_frees build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-fat-move-mutant FAIL: mtools `mformat` not found.\n'; exit 1; }
@printf ">>> test-fat-move-mutant: confirming all SIX MOVE mutants go RED for the RIGHT reason (Rule 6; beads initech-tdnl.26)\n"
@set -e; for m in \
	"$(TEST_FAT12_MOVE_MUT_RELINKS):relinks-chain:preserved bit-for-bit" \
	"$(TEST_FAT12_MOVE_MUT_LOSES):loses-entry:PRESENT in the destination" \
	"$(TEST_FAT12_MOVE_MUT_NODOTDOT):no-dotdot-fix:now names DST" \
	"$(TEST_FAT12_MOVE_MUT_NOROLL):no-grow-rollback:back to ONE cluster" \
	"$(TEST_FAT12_MOVE_MUT_NOSUFFIX):no-collide-check:REPOR001.DBF" \
	"$(TEST_FAT12_MOVE_MUT_FREES):frees-chain:on-disk FAT copies byte-unchanged" ; do \
	bin=$${m%%:*}; rest=$${m#*:}; name=$${rest%%:*}; want=$${rest#*:}; \
	$(MAKE) --no-print-directory -o $(MAKEFILE_STAMP) fat12-move-images-quiet; \
	out=$$($$bin "$(FAT12_MOVE_DIFF_IMG)" "$(FAT12_MOVE_STRUCT_IMG)" 2>&1) && rcx=0 || rcx=1; \
	printf '%s' "$$out" | grep -q 'checks, [0-9]* failures' \
		|| { printf '!!! test-fat-move-mutant FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if [ $$rcx -eq 0 ]; then \
		printf '!!! test-fat-move-mutant FAIL: %s mutant PASSED -- that oracle leg is decoration\n' "$$name"; exit 1; \
	fi; \
	printf '%s' "$$out" | grep -q "$$want" \
		|| { printf '!!! test-fat-move-mutant FAIL: %s RED but its target assertion (%s) never ran -- wrong reason\n' "$$name" "$$want"; exit 1; }; \
	printf '>>> test-fat-move-mutant: green (%s correctly RED -- its rule bites)\n' "$$name"; \
done
@printf '>>> test-fat-move-mutant: green (ALL SIX MOVE mutants ran + RED for the right reason)\n'
```

## test-fat-partial

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4682):
Prerequisites: `build/test_fat12_partial build/fat12_fixture.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-partial : FAT12 positioned-read oracle\n'
@printf '======================================================================\n'
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: python3 not found (needed for the independent --cat-range reference).\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-partial FAIL: mtools `mcopy` not found (apt install mtools).\n'; exit 1; }
@printf '>>> test-fat-partial [A]: host matrix -- fat12_read_partial == fixture slice (byte-for-byte)\n'
@$(TEST_FAT12_PARTIAL) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" \
	|| { printf '!!! test-fat-partial FAIL [A]: host matrix oracle red\n'; exit 1; }
@printf '>>> test-fat-partial [A]: green\n'
@printf '>>> test-fat-partial [B]: python3 --cat-range == mcopy+dd of the SAME range (reference proven on real FAT12)\n'
@set -e; \
for spec in "CHAIN.TXT 0 64" "CHAIN.TXT 500 100" "CHAIN.TXT 600 500" \
            "CHAIN.TXT 1024 512" "CHAIN.TXT 1550 200" "CHAIN.TXT 1 1599" \
            "HELLO.TXT 4 10" "BLOCK.BIN 512 512" "BLOCK.BIN 1000 4096" \
            "SECOND.TXT 50 200"; do \
	set -- $$spec; nm="$$1"; off="$$2"; ln="$$3"; \
	python3 "$(FAT12_REF_PY)" "$(FAT12_IMG)" --cat-range "$$nm" "$$off" "$$ln" \
		> "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: python --cat-range %s %s %s errored\n' "$$nm" "$$off" "$$ln"; exit 1; }; \
	mcopy -n -i "$(FAT12_IMG)" "::$$nm" "$(BUILD)/fatp_whole_$$nm.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: mcopy ::%s errored\n' "$$nm"; exit 1; }; \
	dd if="$(BUILD)/fatp_whole_$$nm.bin" of="$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin" \
		bs=1 skip="$$off" count="$$ln" status=none 2>/dev/null || true; \
	cmp -s "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" "$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin" \
		|| { printf '!!! test-fat-partial FAIL [B]: %s [%s,+%s) -- python ref bytes != dd-of-mcopy bytes\n' "$$nm" "$$off" "$$ln"; \
		     cmp "$(BUILD)/fatp_py_$${nm}_$${off}_$${ln}.bin" "$(BUILD)/fatp_dd_$${nm}_$${off}_$${ln}.bin"; exit 1; }; \
done
@printf '>>> test-fat-partial [B]: green (python --cat-range agrees with mcopy+dd on every range)\n'
@printf '>>> test-fat-partial: green\n'
```

## test-fat-partial-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4717):
Prerequisites: `build/test_fat12_partial_mut_skip build/test_fat12_partial_mut_pos build/fat12_fixture.img build/.makefile.stamp`
```make
@printf '>>> test-fat-partial-mutant: confirming both positioned-read mutants go RED (Rule 6)\n'
@if $(TEST_FAT12_PARTIAL_MUT_SKIP) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-partial-mutant FAIL: skip-count mutant PASSED -- the matrix is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-partial-mutant: green (skip-count mutant correctly RED)\n'; \
fi
@if $(TEST_FAT12_PARTIAL_MUT_POS) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-partial-mutant FAIL: within-cluster-offset mutant PASSED -- the matrix is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-partial-mutant: green (within-cluster-offset mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-fat-partition

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 37):
Prerequisites: `build/fat_dump build/test_partition build/.makefile.stamp`
```make
@sh $(PARTITION_DIR)/partition.sh host $(BUILD)
```

## test-fat-partition-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 46):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/test_partition build/partition_probe.bin build/chkdsk_program.bin build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 sh $(PARTITION_DIR)/partition.sh emu $(BUILD) $(SECONDARY_TRACER_IMG)
```

## test-fat-partition-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 48):
Prerequisites: `build/qemu_harness build/test_partition build/partition_probe.bin build/chkdsk_program.bin build/partition_IGNORE_BASE_boot.img build/partition_EXTENDED_boot.img build/partition_SIGNATURE_boot.img build/.makefile.stamp`
```make
@set -eu; for m in $(PARTITION_MUTANTS); do \
 log=$(BUILD)/kzfs/emu-mut-$$m.log; mkdir -p $(BUILD)/kzfs; \
 if INITECH_QMP_STDIO=1 sh $(PARTITION_DIR)/partition.sh emu-mut $(BUILD) $(BUILD)/partition_$${m}_boot.img $$m > $$log 2>&1; then \
 echo "FAIL partition emu $$m survived"; exit 1; fi; \
 grep '^RED partition ' $$log || { cat $$log; exit 1; }; \
 echo "GREEN partition emu mutant $$m rejected"; done
```

## test-fat-partition-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 39):
Prerequisites: `build/test_partition_IGNORE_BASE build/test_partition_EXTENDED build/test_partition_SIGNATURE build/test_partition build/.makefile.stamp`
```make
@sh $(PARTITION_DIR)/partition.sh mint $(BUILD)
@set -eu; for m in $(PARTITION_MUTANTS); do \
 log=$(BUILD)/kzfs/mut-$$m.log; \
 if $(BUILD)/test_partition_$$m $(PARTITIONED_DATA_IMG) > $$log 2>&1; then echo "FAIL partition $$m survived"; exit 1; fi; \
 grep '^RED partition ' $$log || { cat $$log; exit 1; }; \
 echo "GREEN partition mutant $$m rejected"; done
```

## test-fat-readfile-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4761):
Prerequisites: `build/test_fat12_dir_mut_step build/test_fat12_dir_mut_eoc build/test_fat12_dir_mut_trunc build/fat12_fixture.img build/.makefile.stamp`
```make
@printf '>>> test-fat-readfile-mutant: confirming all THREE streaming read mutants go RED (Rule 6)\n'
@if timeout 60 $(TEST_FAT12_DIR_MUT_STEP) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-readfile-mutant FAIL: step-bound(-1) mutant PASSED -- the anti-hang bound is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-readfile-mutant: green M1/3 (step-bound(-1) mutant correctly RED)\n'; \
fi
@if timeout 60 $(TEST_FAT12_DIR_MUT_EOC) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-readfile-mutant FAIL: follow-EOC mutant PASSED -- the EOC check is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-readfile-mutant: green M2/3 (follow-EOC mutant correctly RED)\n'; \
fi
@if timeout 60 $(TEST_FAT12_DIR_MUT_TRUNC) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-readfile-mutant FAIL: drop-truncation mutant PASSED -- RISK-5 is unguarded\n'; \
	exit 1; \
else \
	printf '>>> test-fat-readfile-mutant: green M3/3 (drop-truncation mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-fat-spc

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/fat_spc.mk', line 8):
Prerequisites: `build/test_fat_spc build/fat_dump build/.makefile.stamp`
```make
@sh $(FAT_DIFF_DIR)/fat_spc.sh $<
```

## test-fat-spc-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/fat_spc.mk', line 16):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@INITECH_QMP_STDIO=1 FAT_SPC=4 sh $(FAT_DIFF_DIR)/dos_volumes.sh fat16 $(SECONDARY_TRACER_IMG) spc4
@echo '>>> test-fat-spc-emu: green (spc=4 FAT16 ATA mount, DIR/TYPE vs mtools)'
```

## test-fat-spc-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/fat_spc.mk', line 10):
Prerequisites: `build/test_fat_spc_mut build/fat_dump build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/fat-spc
@set -eu; for type in 12 16; do \
 if FAT_SPC_TYPES=$$type sh $(FAT_DIFF_DIR)/fat_spc.sh $< mutant-$$type > $(BUILD)/fat-spc/mutant-$$type.log 2>&1; then echo "FAIL FAT_MUT_SPC_ONE survived FAT$$type"; exit 1; fi; \
 grep -Fx 'RED fat-spc whole-file bytes vs mcopy' $(BUILD)/fat-spc/mutant-$$type.log; done
@echo '>>> test-fat-spc-mutant: green (FAT_MUT_SPC_ONE correctly RED for FAT12 and FAT16)'
```

## test-fat-subdir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16489):
Prerequisites: `build/fat_dump build/test_fat12_subdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-subdir : FAT12 subdir/path oracle\n'
@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-ti8.\n'
@printf '  Our C reader vs TWO independent refs (mtools + python3) on a nested tree.\n'
@printf '======================================================================\n'
@# ---- (D) Tool presence: fail loud, NEVER silently skip (Law 2). ----
@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat-subdir FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-subdir FAIL: python3 not found (needed for the independent reference reader).\n'; exit 1; }
@printf '>>> test-fat-subdir [D]: reference tools present (mtools mdir + python3)\n'
@# ---- (C) The subdir unit oracle (enumerate + resolve + multi-cluster read). ----
@printf '>>> test-fat-subdir [C]: unit oracle (subdir enumerate + path resolve + FILE20 multi-cluster read)\n'
@$(TEST_FAT12_SUBDIR) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)" \
	|| { printf '!!! test-fat-subdir FAIL: test_fat12_subdir red\n'; exit 1; }
@# ---- (A) NAMES+SIZES per directory: triple agreement (our == python == mdir). ----
@printf '>>> test-fat-subdir [A]: names+sizes per dir -- fat_dump == python ref == normalized mdir\n'
@for d in $(FAT12_SUB_PATHS); do \
	$(FAT_DUMP_BIN) "$(FAT12_NESTED_IMG)" --list-path "$$d" | sort > "$(FAT12_SUB_OUR_LIST)" \
		|| { printf '!!! test-fat-subdir FAIL [A]: fat_dump --list-path %s errored\n' "$$d"; exit 1; }; \
	python3 "$(FAT12_REF_PY)" "$(FAT12_NESTED_IMG)" --list-path "$$d" | sort > "$(FAT12_SUB_PY_LIST)" \
		|| { printf '!!! test-fat-subdir FAIL [A]: fat12_ref.py --list-path %s errored\n' "$$d"; exit 1; }; \
	mp=$$(printf '%s' "$$d" | sed -e 's#^\\##' -e 's#\\#/#g'); \
	mdir -i "$(FAT12_NESTED_IMG)" "::$$mp" | awk '{ \
		hd=0; di=0; \
		for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
		if(!hd) next; \
		name=$$1; ext=$$2; sz=""; \
		for(i=3;i<di;i++){ sz=sz $$i } \
		if(sz !~ /^[0-9]+$$/) next; \
		fn=(ext!="")?name"."ext:name; \
		print fn, sz \
	}' | sort > "$(FAT12_SUB_MTOOLS_LIST)"; \
	diff -u "$(FAT12_SUB_OUR_LIST)" "$(FAT12_SUB_PY_LIST)" \
		|| { printf '!!! test-fat-subdir FAIL [A]: %s -- our listing != python reference listing\n' "$$d"; exit 1; }; \
	diff -u "$(FAT12_SUB_OUR_LIST)" "$(FAT12_SUB_MTOOLS_LIST)" \
		|| { printf '!!! test-fat-subdir FAIL [A]: %s -- our listing != normalized mdir listing\n' "$$d"; exit 1; }; \
	printf '    %-14s %s files : our == python == mdir\n' "$$d" "$$(wc -l < "$(FAT12_SUB_OUR_LIST)" | tr -d ' ')"; \
done
@# ---- (B) MULTI-CLUSTER proof: BIGDIR has > 16 files (3-cluster chain walked). ----
@printf '>>> test-fat-subdir [B]: BIGDIR multi-cluster -- > 16 files (single-sector reader would lose FILE17+)\n'
@bign=$$($(FAT_DUMP_BIN) "$(FAT12_NESTED_IMG)" --list-path '\BIGDIR' | wc -l | tr -d ' '); \
	if [ "$$bign" -le 16 ]; then \
		printf '!!! test-fat-subdir FAIL [B]: BIGDIR listed only %s files (<=16) -- multi-cluster walk broken\n' "$$bign"; exit 1; \
	fi; \
	printf '    BIGDIR: %s files across a 3-cluster chain (multi-cluster walk confirmed)\n' "$$bign"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- our subdir reader agrees with mtools AND an independent\n'
@printf '            python3 reader on the nested tree (incl. the multi-cluster BIGDIR).\n'
@printf '======================================================================\n'
```

## test-fat-subdir-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16547):
Prerequisites: `build/test_fat12_subdir_mut_single build/test_fat12_subdir_mut_noattr build/fat12_nested.img build/.makefile.stamp`
```make
@printf '>>> test-fat-subdir-mutant: confirming both subdir mutants go RED (Rule 6)\n'
@if $(TEST_FAT12_SUBDIR_MUT_SINGLE) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-subdir-mutant FAIL: single-sector mutant PASSED -- the multi-cluster oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-subdir-mutant: green (single-sector subdir mutant correctly RED -- BIGDIR loses FILE17+)\n'; \
fi
@if $(TEST_FAT12_SUBDIR_MUT_NOATTR) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-fat-subdir-mutant FAIL: no-attr mutant PASSED -- the directory-gate oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-subdir-mutant: green (no-attr mutant correctly RED -- a file wrongly traversed)\n'; \
fi
```

## test-fat-truncate-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 98):
Prerequisites: `build/truncate build/truncate_program.bin build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/truncate.sh emu $(SECONDARY_TRACER_IMG)
```

## test-fat-truncate-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 253):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-truncate-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-fat-truncate-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/fat-truncate-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/fat-truncate-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "fat-truncate-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/fat-truncate-emu_bochs.report.txt" || true
@cat "$(BUILD)/fat-truncate-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-fat-truncate-emu-bochs "$(BUILD)/fat-truncate-emu_bochs.report.txt" "$(BUILD)/fat-truncate-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-fat-truncate-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 104):
Prerequisites: `build/truncate build/truncate_program.bin build/qemu_harness build/tracer_mut_truncate.img build/.makefile.stamp`
```make
@if sh $(SHELL_FILES_DIR)/truncate.sh emu $(BUILD)/tracer_mut_truncate.img mutant >$(BUILD)/truncate_emu_mut.log 2>&1; then echo 'FAIL FAT12_MUTATE_TRUNCATE_NO_FREE survived QEMU';exit 1;fi
@grep '^RED truncate: released clusters != mtools' $(BUILD)/truncate_emu_mut.log
@grep -qx TRUNCATE-PASS $(BUILD)/truncate-emu-mutant/text
@echo 'GREEN test-fat-truncate-emu-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE booted and caught RED'
```

## test-fat-truncate-gap-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 62):
Prerequisites: `build/truncate build/truncate_program.bin build/qemu_harness build/j1lu_stale_gap.img build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dos-volumes
@if sh harness/diff/fat_diff/truncate.sh emu $(BUILD)/j1lu_stale_gap.img stale-gap > $(BUILD)/dos-volumes/stale-gap-emu.log 2>&1; then echo 'FAIL STALE_GAP survived ATA'; exit 1; fi
@grep '^RED truncate: GAPIN bytes != mcopy' $(BUILD)/dos-volumes/stale-gap-emu.log
@echo 'GREEN test-fat-truncate-gap-emu-mutant: STALE_GAP rejected after real AH40/ATA'
```

## test-fat-truncate-host

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 96):
Prerequisites: `build/truncate build/.makefile.stamp`
```make
@sh $(SHELL_FILES_DIR)/truncate.sh host $<
```

## test-fat-truncate-host-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 100):
Prerequisites: `build/truncate build/truncate_mut build/.makefile.stamp`
```make
@if sh $(SHELL_FILES_DIR)/truncate.sh host $(BUILD)/truncate_mut mutant >$(BUILD)/truncate_host_mut.log 2>&1; then echo 'FAIL FAT12_MUTATE_TRUNCATE_NO_FREE survived';exit 1;fi
@grep '^RED truncate: released clusters != mtools' $(BUILD)/truncate_host_mut.log
@echo 'GREEN test-fat-truncate-host-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE caught RED'
```

## test-fat-write

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16593):
Prerequisites: `build/test_fat12_write harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-write : FAT12 WRITE round-trip oracle\n'
@printf '  Ref: PRD Sec 6.1 / ADR-0003 DEC-07. beads initech-509.11.\n'
@printf '  OUR writer mints a blank image + writes files; mtools + python3 read it back.\n'
@printf '======================================================================\n'
@# ---- (B) Tool presence: fail loud, NEVER silently skip (Law 2). ----
@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: mtools `mcopy` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-write FAIL: python3 not found.\n'; exit 1; }
@printf '>>> test-fat-write [B]: reference tools present (mtools + python3)\n'
@# ---- (A) The WRITE unit + in-memory round-trip oracle. ----
@dd if=/dev/zero of="$(BUILD)/fatw_unit.img" bs=512 count=2880 status=none
@mformat -i "$(BUILD)/fatw_unit.img" -f 1440 ::
@$(TEST_FAT12_WRITE) "$(BUILD)/fatw_unit.img" \
	|| { printf '!!! test-fat-write FAIL [A]: test_fat12_write unit/round-trip red\n'; exit 1; }
@printf '>>> test-fat-write [A]: WRITE unit + in-memory round-trip green\n'
@# ---- (C) Mint a clean BLANK image and write the 4 fixtures with OUR writer. ----
@dd if=/dev/zero of="$(BUILD)/fatw_diff.img" bs=512 count=2880 status=none
@mformat -i "$(BUILD)/fatw_diff.img" -f 1440 ::
@$(TEST_FAT12_WRITE) "$(BUILD)/fatw_diff.img" --diff \
	|| { printf '!!! test-fat-write FAIL [C]: writing the diff fixtures failed\n'; exit 1; }
@printf '>>> test-fat-write [C]: OUR writer wrote 4 files into a blank image\n'
@# ---- (D) NAMES+SIZES: normalized mdir == python3 --list. ----
@printf '>>> test-fat-write [D]: names+sizes -- python3 reader == normalized mdir on OUR image\n'
@python3 "$(FAT12_REF_PY)" "$(BUILD)/fatw_diff.img" --list > "$(FATW_PY_LIST)" \
	|| { printf '!!! test-fat-write FAIL [D]: fat12_ref.py --list errored on our written image\n'; exit 1; }
@mdir -i "$(BUILD)/fatw_diff.img" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$$1; ext=$$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $$i } \
	if(sz !~ /^[0-9]+$$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "$(FATW_MTOOLS_LIST)"
@diff -u "$(FATW_PY_LIST)" "$(FATW_MTOOLS_LIST)" \
	|| { printf '!!! test-fat-write FAIL [D]: python listing != normalized mdir listing of OUR written image\n'; exit 1; }
@printf '    agreement on names+sizes (%s files):\n' "$$(wc -l < "$(FATW_PY_LIST)" | tr -d ' ')"
@sed 's/^/      /' "$(FATW_PY_LIST)"
@# ---- (E) CONTENT per file: mcopy == python3 --cat == our deterministic bytes. ----
@printf '>>> test-fat-write [E]: content -- mcopy == python ref == our written bytes, per file\n'
@for f in $(FAT12_WRITE_NAMES); do \
	mcopy -i "$(BUILD)/fatw_diff.img" "::$$f" - > "$(BUILD)/fatw_mtools_$$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat-write FAIL [E]: mcopy ::%s errored (mtools could not read our written file)\n' "$$f"; exit 1; }; \
	python3 "$(FAT12_REF_PY)" "$(BUILD)/fatw_diff.img" --cat "$$f" > "$(BUILD)/fatw_py_$$f.bin" \
		|| { printf '!!! test-fat-write FAIL [E]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
	cmp -s "$(BUILD)/fatw_mtools_$$f.bin" "$(BUILD)/fatw_py_$$f.bin" \
		|| { printf '!!! test-fat-write FAIL [E]: %s -- mcopy bytes != python ref bytes (root-cause the writer, Rule 3)\n' "$$f"; cmp "$(BUILD)/fatw_mtools_$$f.bin" "$(BUILD)/fatw_py_$$f.bin"; exit 1; }; \
	printf '    %-12s %6s bytes : mcopy == python\n' "$$f" "$$(wc -c < "$(BUILD)/fatw_mtools_$$f.bin" | tr -d ' ')"; \
done
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a volume written ENTIRELY by InitechDOS reads back correctly\n'
@printf '            in mtools AND an independent python3 reader (names, sizes, content)\n'
@printf '            AND in our own reader (in-memory round-trip).\n'
@printf '======================================================================\n'
```

## test-fat-write-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16654):
Prerequisites: `build/test_fat12_write_mut_onefat build/test_fat12_write_mut_eoc build/.makefile.stamp`
```make
@printf '>>> test-fat-write-mutant: confirming both WRITE mutants go RED (Rule 6)\n'
@dd if=/dev/zero of="$(BUILD)/fatw_mut1.img" bs=512 count=2880 status=none
@mformat -i "$(BUILD)/fatw_mut1.img" -f 1440 ::
@if $(TEST_FAT12_WRITE_MUT_ONEFAT) "$(BUILD)/fatw_mut1.img" >/dev/null 2>&1; then \
	printf '!!! test-fat-write-mutant FAIL: one-FAT-copy mutant PASSED -- the both-FAT-sync check is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-write-mutant: green (one-FAT-copy mutant correctly RED)\n'; \
fi
@dd if=/dev/zero of="$(BUILD)/fatw_mut2.img" bs=512 count=2880 status=none
@mformat -i "$(BUILD)/fatw_mut2.img" -f 1440 ::
@if $(TEST_FAT12_WRITE_MUT_EOC) "$(BUILD)/fatw_mut2.img" >/dev/null 2>&1; then \
	printf '!!! test-fat-write-mutant FAIL: wrong-EOC mutant PASSED -- the round-trip check is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-write-mutant: green (wrong-EOC mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-fat-write-partial

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5004):
Prerequisites: `build/test_fat12_write_partial build/fat12_blank_wp.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat-write-partial : FAT12 positioned-WRITE oracle\n'
@printf '======================================================================\n'
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat-write-partial FAIL: python3 not found (needed for the independent reference).\n'; exit 1; }
@printf '>>> test-fat-write-partial [A]: in-process matrix -- read_file/read_partial == byte model (+ disk-full rollback)\n'
@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_unit.img"
@$(TEST_FAT12_WRITE_PARTIAL) "$(BUILD)/fatwp_unit.img" \
	|| { printf '!!! test-fat-write-partial FAIL [A]: in-process matrix red\n'; exit 1; }
@printf '>>> test-fat-write-partial [A]: green\n'
@printf '>>> test-fat-write-partial [B]: OUR writer writes the files into a fresh blank image\n'
@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_diff.img"
@$(TEST_FAT12_WRITE_PARTIAL) "$(BUILD)/fatwp_diff.img" --diff \
	|| { printf '!!! test-fat-write-partial FAIL [B]: writing the diff fixtures failed\n'; exit 1; }
@printf '>>> test-fat-write-partial [C]: content -- mcopy == python3 ref, per positioned-written file\n'
@set -e; \
for f in $(FAT12_WRITE_PARTIAL_NAMES); do \
	mcopy -n -i "$(BUILD)/fatwp_diff.img" "::$$f" "$(BUILD)/fatwp_mtools_$$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: mcopy ::%s errored (mtools could not read our written file)\n' "$$f"; exit 1; }; \
	python3 "$(FAT12_REF_PY)" "$(BUILD)/fatwp_diff.img" --cat "$$f" > "$(BUILD)/fatwp_py_$$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: fat12_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
	cmp -s "$(BUILD)/fatwp_mtools_$$f.bin" "$(BUILD)/fatwp_py_$$f.bin" \
		|| { printf '!!! test-fat-write-partial FAIL [C]: %s -- mcopy bytes != python ref bytes (root-cause the writer, Rule 3)\n' "$$f"; \
		     cmp "$(BUILD)/fatwp_mtools_$$f.bin" "$(BUILD)/fatwp_py_$$f.bin"; exit 1; }; \
done
@printf '>>> test-fat-write-partial [C]: green (mcopy + python3 agree on every positioned-written file)\n'
@printf '>>> test-fat-write-partial: green\n'
```

## test-fat-write-partial-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5037):
Prerequisites: `build/test_fat12_write_partial_mut_rmw build/test_fat12_write_partial_mut_ext build/fat12_blank_wp.img build/.makefile.stamp`
```make
@printf '>>> test-fat-write-partial-mutant: confirming both positioned-write mutants go RED (Rule 6)\n'
@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_mut_rmw.img"
@if $(TEST_FAT12_WRITE_PARTIAL_MUT_RMW) "$(BUILD)/fatwp_mut_rmw.img" >/dev/null 2>&1; then \
	printf '!!! test-fat-write-partial-mutant FAIL: no-RMW mutant PASSED -- the matrix is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-write-partial-mutant: green (no-RMW mutant correctly RED)\n'; \
fi
@cp -f "$(FAT12_BLANK_WP_IMG)" "$(BUILD)/fatwp_mut_ext.img"
@if $(TEST_FAT12_WRITE_PARTIAL_MUT_EXT) "$(BUILD)/fatwp_mut_ext.img" >/dev/null 2>&1; then \
	printf '!!! test-fat-write-partial-mutant FAIL: short-extend mutant PASSED -- the matrix is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fat-write-partial-mutant: green (short-extend mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-fat12-bpb

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 2351):
Prerequisites: `build/test_fat12_bpb build/fat12_fixture.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-bpb: mount + BPB parse + geometry vs verified constants\n"
@$(TEST_FAT12_BPB) "$(FAT12_IMG)"
@printf ">>> test-fat12-bpb: green\n"
```

## test-fat12-chain

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 3984):
Prerequisites: `build/test_fat12_chain build/fat12_fixture.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-chain: FAT12 12-bit decode + cluster-chain walk + anti-hang guard\n"
@$(TEST_FAT12_CHAIN) "$(FAT12_IMG)"
@printf ">>> test-fat12-chain: green\n"
```

## test-fat12-dir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4001):
Prerequisites: `build/test_fat12_dir build/fat12_fixture.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-dir: root-dir enumerate + find + file-read (byte-for-byte golden)\n"
@$(TEST_FAT12_DIR) "$(FAT12_IMG)" "$(FAT12_FIXTURE_DIR)"
@printf ">>> test-fat12-dir: green\n"
```

## test-fat12-mkdir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4064):
Prerequisites: `build/test_fat12_mkdir build/fat12_mkdir_golden.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-mkdir: MKDIR/RMDIR write side vs mmd golden ('.'/'..'/EOC byte-for-byte)\n"
@dd if=/dev/zero of=$(FAT12_MKDIR_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_MKDIR_BLANK_IMG) -f 1440 ::
@$(TEST_FAT12_MKDIR) "$(FAT12_MKDIR_BLANK_IMG)" "$(FAT12_MKDIR_GOLDEN_IMG)"
@printf ">>> test-fat12-mkdir: green\n"
```

## test-fat12-mkdir-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4077):
Prerequisites: `build/test_fat12_mkdir_mut_dotdot build/test_fat12_mkdir_mut_noeoc build/test_fat12_mkdir_mut_noempty build/test_fat12_mkdir_mutant_stale_trusted build/fat12_mkdir_golden.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-mkdir-mutant: confirming all four MKDIR/RMDIR mutants go RED (Rule 6)\n"
@dd if=/dev/zero of=$(FAT12_MKDIR_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_MKDIR_BLANK_IMG) -f 1440 ::
@if $(TEST_FAT12_MKDIR_MUT_DOTDOT) "$(FAT12_MKDIR_BLANK_IMG)" "$(FAT12_MKDIR_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-fat12-mkdir-mutant FAIL: dotdot-self mutant PASSED -- the .. diff is decoration\n'; exit 1; \
else \
	printf '>>> test-fat12-mkdir-mutant: green (dotdot-self mutant correctly RED -- the .. rule bites)\n'; \
fi
@dd if=/dev/zero of=$(FAT12_MKDIR_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_MKDIR_BLANK_IMG) -f 1440 ::
@if $(TEST_FAT12_MKDIR_MUT_NOEOC) "$(FAT12_MKDIR_BLANK_IMG)" "$(FAT12_MKDIR_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-fat12-mkdir-mutant FAIL: no-EOC mutant PASSED -- the FAT-entry diff is decoration\n'; exit 1; \
else \
	printf '>>> test-fat12-mkdir-mutant: green (no-EOC mutant correctly RED -- the EOC link bites)\n'; \
fi
@dd if=/dev/zero of=$(FAT12_MKDIR_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_MKDIR_BLANK_IMG) -f 1440 ::
@if $(TEST_FAT12_MKDIR_MUT_NOEMPTY) "$(FAT12_MKDIR_BLANK_IMG)" "$(FAT12_MKDIR_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-fat12-mkdir-mutant FAIL: no-empty-check mutant PASSED -- the RMDIR empty-check is decoration\n'; exit 1; \
else \
	printf '>>> test-fat12-mkdir-mutant: green (no-empty-check mutant correctly RED -- the empty-check bites)\n'; \
fi
@dd if=/dev/zero of=$(FAT12_MKDIR_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_MKDIR_BLANK_IMG) -f 1440 ::
@if $(TEST_FAT12_MKDIR_MUT_STALE) "$(FAT12_MKDIR_BLANK_IMG)" "$(FAT12_MKDIR_GOLDEN_IMG)" > $(BUILD)/test_fat12_mkdir_mutant_stale.log 2>&1; then \
	printf '!!! test-fat12-mkdir-mutant FAIL: stale-trusted mutant PASSED -- the L002 directory-lifetime leg is decoration\n'; exit 1; \
fi
@grep -q 'FAIL.*L002 mkdir into a PURGED directory cluster' $(BUILD)/test_fat12_mkdir_mutant_stale.log || { printf '!!! test-fat12-mkdir-mutant FAIL: stale-trusted mutant went RED for the wrong reason\n'; cat $(BUILD)/test_fat12_mkdir_mutant_stale.log; exit 1; }
@printf '>>> test-fat12-mkdir-mutant: green (stale-trusted mutant correctly RED -- mkdir into a purged directory cluster is refused, L002)\n'
```

## test-fat12-subdir

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4025):
Prerequisites: `build/test_fat12_subdir build/fat12_nested.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-subdir: subdir enumerate + path resolve (byte-for-byte golden)\n"
@$(TEST_FAT12_SUBDIR) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)"
@printf ">>> test-fat12-subdir: green\n"
```

## test-fat12-write

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4837):
Prerequisites: `build/test_fat12_write build/fat12_blank_unit.img build/.makefile.stamp`
```make
@printf ">>> test-fat12-write: FAT12 WRITE unit (12-bit encode + alloc + both-FAT sync + round-trip + full-volume fail-loud + unlink)\n"
@cp -f "$(FAT12_BLANK_UNIT_IMG)" "$(BUILD)/fat12_write_unit_run.img"
@$(TEST_FAT12_WRITE) "$(BUILD)/fat12_write_unit_run.img"
@printf ">>> test-fat12-write: green\n"
```

## test-fat16

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16320):
Prerequisites: `build/fat_dump build/test_fat16 build/fat16_fixture.img harness/diff/fat_diff/fat16_ref.py build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fat16 : FAT16 READ-ONLY differential\n'
@printf '  Ref: PRD Sec 5/6.1 / ADR-0003 DEC-07. beads initech-z01.\n'
@printf '  Our C reader vs TWO independent refs (mtools + python3) on a FAT16 vol.\n'
@printf '======================================================================\n'
@# ---- (D) Tool presence: fail loud, NEVER silently skip (Law 2). ----
@command -v mkfs.fat >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mkfs.fat not found (apt install dosfstools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mdir   >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mtools `mdir` not found (apt install mtools).\n'; exit 1; }
@command -v mcopy  >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: mtools `mcopy` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-fat16 FAIL: python3 not found (independent reference reader).\n'; exit 1; }
@printf '>>> test-fat16 [D]: reference tools present (mkfs.fat + mtools mdir/mcopy + python3)\n'
@# ---- (C) The FAT16 unit oracle (mount+classify, enumerate, read). ----
@printf '>>> test-fat16 [C]: unit oracle (mount+classify FAT16, enumerate, byte-for-byte read)\n'
@$(TEST_FAT16) "$(FAT16_IMG)" "$(FAT12_FIXTURE_DIR)" \
	|| { printf '!!! test-fat16 FAIL: test_fat16 unit oracle red\n'; exit 1; }
@# ---- (A) NAMES + SIZES: triple agreement (our == python == mdir). ----
@printf '>>> test-fat16 [A]: names+sizes -- our fat_dump == python ref == normalized mdir\n'
@$(FAT_DUMP_BIN) "$(FAT16_IMG)" --list > "$(FAT16_OUR_LIST)" \
	|| { printf '!!! test-fat16 FAIL: fat_dump --list errored\n'; exit 1; }
@python3 "$(FAT16_REF_PY)" "$(FAT16_IMG)" --list > "$(FAT16_PY_LIST)" \
	|| { printf '!!! test-fat16 FAIL: fat16_ref.py --list errored\n'; exit 1; }
@mdir -i "$(FAT16_IMG)" :: | awk '{ \
	hd=0; di=0; \
	for(i=1;i<=NF;i++){ if($$i ~ /^[0-9]{4}-[0-9]{2}-[0-9]{2}$$/){hd=1;di=i;break} } \
	if(!hd) next; \
	name=$$1; ext=$$2; sz=""; \
	for(i=3;i<di;i++){ sz=sz $$i } \
	if(sz !~ /^[0-9]+$$/) next; \
	fn=(ext!="")?name"."ext:name; \
	print fn, sz \
}' | sort > "$(FAT16_MTOOLS_LIST)"
@diff -u "$(FAT16_OUR_LIST)" "$(FAT16_PY_LIST)" \
	|| { printf '!!! test-fat16 FAIL [A]: our listing != python reference listing\n'; exit 1; }
@diff -u "$(FAT16_OUR_LIST)" "$(FAT16_MTOOLS_LIST)" \
	|| { printf '!!! test-fat16 FAIL [A]: our listing != normalized mdir listing\n'; exit 1; }
@printf '    triple agreement on names+sizes (%s files):\n' "$$(wc -l < "$(FAT16_OUR_LIST)" | tr -d ' ')"
@sed 's/^/      /' "$(FAT16_OUR_LIST)"
@# ---- (B) CONTENT per file: our == mcopy == python, byte-for-byte. ----
@printf '>>> test-fat16 [B]: content -- our fat_dump --cat == mcopy == python ref, per file\n'
@for f in $(FAT16_GATE_NAMES); do \
	$(FAT_DUMP_BIN) "$(FAT16_IMG)" --cat "$$f" > "$(BUILD)/fat16_our_$$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: fat_dump --cat %s errored\n' "$$f"; exit 1; }; \
	mcopy -i "$(FAT16_IMG)" "::$$f" - > "$(BUILD)/fat16_mtools_$$f.bin" 2>/dev/null \
		|| { printf '!!! test-fat16 FAIL [B]: mcopy ::%s errored\n' "$$f"; exit 1; }; \
	python3 "$(FAT16_REF_PY)" "$(FAT16_IMG)" --cat "$$f" > "$(BUILD)/fat16_py_$$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: fat16_ref.py --cat %s errored\n' "$$f"; exit 1; }; \
	cmp -s "$(BUILD)/fat16_our_$$f.bin" "$(BUILD)/fat16_mtools_$$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: %s -- our bytes != mcopy bytes\n' "$$f"; exit 1; }; \
	cmp -s "$(BUILD)/fat16_our_$$f.bin" "$(BUILD)/fat16_py_$$f.bin" \
		|| { printf '!!! test-fat16 FAIL [B]: %s -- our bytes != python ref bytes\n' "$$f"; exit 1; }; \
	printf '    %-12s %7s bytes : our == mcopy == python\n' "$$f" "$$(wc -c < "$(BUILD)/fat16_our_$$f.bin" | tr -d ' ')"; \
done
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- our FAT16 reader agrees with mtools AND an independent\n'
@printf '            python3 reader on names, sizes, and content (triple oracle).\n'
@printf '======================================================================\n'
```

## test-fat16-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 11):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_volumes.sh fat16 $(SECONDARY_TRACER_IMG)
```

## test-fat16-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 40):
Prerequisites: `test-fat16-emu build/qemu_harness build/j1lu_volumes_fat16.img build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/dos-volumes
@if sh harness/diff/fat_diff/dos_volumes.sh fat16 $(BUILD)/j1lu_volumes_fat16.img corrupt > $(BUILD)/dos-volumes/fat16.log 2>&1; then echo 'FAIL dos-volumes FAT16 survived'; exit 1; fi
@grep '^RED dos-volumes FAT16 ATA TYPE bytes' $(BUILD)/dos-volumes/fat16.log
@echo 'GREEN test-fat16-emu-mutant: FAT16 byte corruption rejected'
```

## test-fat16-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 16386):
Prerequisites: `build/test_fat16_mut_m1 build/test_fat16_mut_m2 build/test_fat16_mut_m3 build/test_fat16_mut_m4 build/test_fat16_mut_m5 build/fat16_fixture.img build/.makefile.stamp`
```make
@printf '>>> test-fat16-mutant: confirming all FIVE FAT16 read mutants go RED (Rule 6; beads initech-z01)\n'
@for m in M1:$(TEST_FAT16_MUT_M1) M2:$(TEST_FAT16_MUT_M2) M3:$(TEST_FAT16_MUT_M3) M4:$(TEST_FAT16_MUT_M4) M5:$(TEST_FAT16_MUT_M5); do \
	name=$${m%%:*}; bin=$${m#*:}; \
	"$$bin" "$(FAT16_IMG)" "$(FAT12_FIXTURE_DIR)" 2>/dev/null | grep -q 'checks,' \
		|| { printf '!!! test-fat16-mutant FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if "$$bin" "$(FAT16_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
		printf '!!! test-fat16-mutant FAIL: %s PASSED -- the FAT16 read oracle is decoration\n' "$$name"; exit 1; \
	else \
		printf '>>> test-fat16-mutant: green (%s correctly RED -- its FAT16 read primitive bites)\n' "$$name"; \
	fi; \
done
@printf '>>> test-fat16-mutant: green (ALL FIVE FAT16 mutants ran + RED for the right reason)\n'
```

## test-fatwrite

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25976):
Prerequisites: `build/qemu_harness build/write_boot.img build/fat_write.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fatwrite : WRITE+READ round-trip on ATA\n'
@printf '  Ref: ATA/ATAPI-6 WRITE SECTORS + FLUSH CACHE; FAT12 spec; ADR-0003 DEC-07.\n'
@printf '  beads initech-509.11. CREAT+WRITE+CLOSE then OPEN+READ OUT.TXT, in one boot.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + WRITABLE data disk %s (primary slave)\n' "$(WRITE_IMG)" "$(FAT_WRITE_IMG)"
@printf 'Expecting : FILEIO-BIND-OK + "%s" between WRITE-OUTPUT-BEGIN/END + WRITE-EXIT rc=0 + no triple-fault\n' "$(WRITE_EXPECT)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_WRITE_IMG))
@$(HARNESS_BIN) --disk "$(WRITE_IMG)" --disk2 "$(FAT_WRITE_IMG)" \
	--name "$(WRITE_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(WRITE_REPORT)" || true
@cat "$(WRITE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(WRITE_REPORT)"; then \
	printf '!!! test-fatwrite FAIL: TRIPLE FAULT -- root-cause the ATA WRITE protocol order (Rule 3)\n'; exit 1; \
fi
@printf '>>> test-fatwrite [1/4]: no triple-fault\n'
@if [ ! -s "$(WRITE_SERIAL)" ]; then \
	printf '!!! test-fatwrite FAIL: no serial captured at %s\n' "$(WRITE_SERIAL)"; exit 1; \
fi
@grep -q '^FILEIO-BIND-OK$$' "$(WRITE_SERIAL)" \
	|| { printf '!!! test-fatwrite FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
@if grep -q 'WRITE-CREAT-FAIL\|WRITE-WRITE-FAIL\|WRITE-CLOSE-FAIL\|WRITE-REOPEN-FAIL' "$(WRITE_SERIAL)"; then \
	printf '!!! test-fatwrite FAIL: a WRITE-*-FAIL marker -- the CREAT/WRITE/CLOSE/REOPEN path failed (root-cause, Rule 3):\n'; \
	grep 'WRITE-.*-FAIL' "$(WRITE_SERIAL)"; exit 1; \
fi
@printf '>>> test-fatwrite [2/4]: FILEIO-BIND-OK + no WRITE-*-FAIL\n'
@sed -n '/^WRITE-OUTPUT-BEGIN$$/,/^WRITE-OUTPUT-END$$/p' "$(WRITE_SERIAL)" > "$(BUILD)/$(WRITE_NAME).out"
@grep -qF '$(WRITE_EXPECT)' "$(BUILD)/$(WRITE_NAME).out" \
	|| { printf '!!! test-fatwrite FAIL: "%s" not READ BACK from OUT.TXT -- the write did not commit to disk (root-cause ATA WRITE / FAT flush, Rule 3)\n' "$(WRITE_EXPECT)"; exit 1; }
@grep -q '^WRITE-EXIT rc=0$$' "$(WRITE_SERIAL)" \
	|| { printf '!!! test-fatwrite FAIL: WRITE-EXIT rc=0 missing -- the WRITE program did not finish cleanly\n'; exit 1; }
@printf '>>> test-fatwrite [3/4]: OUT.TXT written, READ BACK through the OS, WRITE-EXIT rc=0\n'
@# ---- 4. BONUS: mtools reads OUT.TXT off the post-run disk, content exact. ----
@if command -v mcopy >/dev/null 2>&1; then \
	mcopy -i "$(FAT_WRITE_IMG)" ::OUT.TXT - 2>/dev/null > "$(BUILD)/$(WRITE_NAME).outtxt" || true; \
	if [ -s "$(BUILD)/$(WRITE_NAME).outtxt" ]; then \
		printf 'hello\r\n' > "$(BUILD)/$(WRITE_NAME).expect"; \
		if cmp -s "$(BUILD)/$(WRITE_NAME).outtxt" "$(BUILD)/$(WRITE_NAME).expect"; then \
			printf '>>> test-fatwrite [4/4]: mtools read OUT.TXT off the disk -- content == "hello\\r\\n" (independent confirmation)\n'; \
		else \
			printf '!!! test-fatwrite FAIL [4/4]: mtools read OUT.TXT but content != "hello\\r\\n" (the on-disk bytes are wrong)\n'; \
			cmp "$(BUILD)/$(WRITE_NAME).outtxt" "$(BUILD)/$(WRITE_NAME).expect" || true; exit 1; \
		fi; \
	else \
		printf '!!! test-fatwrite FAIL [4/4]: mtools could not read OUT.TXT off the post-run disk -- the directory entry / chain never reached the medium\n'; exit 1; \
	fi; \
else \
	sh harness/missing_reference.sh REFERENCE_TOOLS "test-fatwrite [4/4]: mtools absent (post-run extraction requires mcopy)" || exit $$?; \
fi
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS WROTE a file to a FAT12 volume over ATA and read it back\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-fbagree

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5277):
Prerequisites: `build/test_fbagree build/.makefile.stamp`
```make
@printf ">>> test-fbagree: one-surface invariant (console == surface, bpp 32/24/8)\n"
@$(TEST_FBAGREE)
@printf ">>> test-fbagree: green\n"
```

## test-fbagree-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5283):
Prerequisites: `build/test_fbagree_mutant build/.makefile.stamp`
```make
@printf ">>> test-fbagree-mutant: FBAGREE_MUTATE_SECOND_PATH must go RED (Rule 6)\n"
@$(TEST_FBAGREE_MUT); test $$? -ne 0
@printf ">>> test-fbagree-mutant: confirmed RED (oracle bites)\n"
```

## test-fcb-api

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 79):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fcbx_program.bin harness/diff/fcb/autoexec_fcbx.bat harness/diff/fcb/fcbx_expected.txt harness/diff/fcb/fcbx_fat_expected.txt harness/diff/fcb/fcbkept_expected.dat build/.makefile.stamp`
```make
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
```

## test-fcb-api-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/fcb_tps.mk', line 98):
Prerequisites: `build/qemu_harness build/fcbx_program.bin harness/diff/fcb/autoexec_fcbx.bat harness/diff/fcb/fcbx_expected.txt harness/diff/fcb/fcbx_fat_expected.txt harness/diff/fcb/fcbkept_expected.dat build/fcb_seq.img build/fcb_noattr.img build/fcb_nozero.img build/.makefile.stamp`
```make
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
```

## test-fileio

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6076):
Prerequisites: `build/test_fileio build/.makefile.stamp`
```make
@printf ">>> test-fileio: OPEN/READ/CLOSE/LSEEK + FINDFIRST/NEXT + SETDTA/GETDTA via mock backend (initech-509.5)\n"
@$(TEST_FILEIO)
@printf ">>> test-fileio: green\n"
```

## test-fileio-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6082):
Prerequisites: `build/test_fileio_mutant_read build/test_fileio_mutant_lseek build/.makefile.stamp`
```make
@printf ">>> test-fileio-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_FILEIO_MUT_READ) >/dev/null 2>&1; then \
	printf '!!! test-fileio-mutant FAIL: READ-ignore-offset mutant PASSED -- the offset-advance test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fileio-mutant: green (READ-ignore-offset mutant correctly RED)\n'; \
fi
@if $(TEST_FILEIO_MUT_LSEEK) >/dev/null 2>&1; then \
	printf '!!! test-fileio-mutant FAIL: LSEEK-whence mutant PASSED -- the SEEK_END test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-fileio-mutant: green (LSEEK-whence mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-fpu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27864):
Prerequisites: `build/qemu_harness build/fpu_test.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fpu : x87 bring-up in the boot path (QEMU)\n'
@printf '  Ref: beads initech-zj6w; PRD Sec 5; ADR-0001 DEC-04; spec/hardware.json fpu\n'
@printf '======================================================================\n'
@$(call fpu-qemu-run,$(FPU_GATE_IMG),fpu_test)
@cat "$(BUILD)/fpu_test.report"
@grep -E '^(FPU|PANIC)' "$(BUILD)/fpu_test.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
@$(call fpu-gate-check,$(BUILD)/fpu_test.serial,$(BUILD)/fpu_test.report,test-fpu)
@printf 'VERDICT   : PASS -- CR0 MP=1 EM=0 TS=0 NE=1, x87 63/125 exact, SW clean, CW == spec (QEMU)\n'
@printf '======================================================================\n'
```

## test-fpu-absent

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27946):
Prerequisites: `build/qemu_harness build/fpu_absent.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fpu-absent : no-FPU boot + #NM on first x87 op\n'
@printf '  (probe verdict FORCED absent -- fault injection; see the Makefile comment)\n'
@printf '======================================================================\n'
@$(call fpu-qemu-run,$(FPU_ABSENT_IMG),fpu_absent)
@cat "$(BUILD)/fpu_absent.report"
@grep -E '^(FPU|PANIC|BANNER$$)' "$(BUILD)/fpu_absent.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
@$(call fpu-absent-check,$(BUILD)/fpu_absent.serial,$(BUILD)/fpu_absent.report,test-fpu-absent)
@printf 'VERDICT   : PASS -- booted without an FPU; EM=1; first x87 op -> #NM panic\n'
@printf '======================================================================\n'
```

## test-fpu-absent-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27958):
Prerequisites: `build/qemu_harness build/fpu_absent_mut_em.img build/.makefile.stamp`
```make
@$(call fpu-qemu-run,$(BUILD)/fpu_absent_mut_em.img,fpu_absent_mut_em)
@out=$$( $(call fpu-absent-check,$(BUILD)/fpu_absent_mut_em.serial,$(BUILD)/fpu_absent_mut_em.report,absent-mutant) ); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^/    | /'; \
[ "$$rc" -ne 0 ] || { printf '!!! test-fpu-absent-mutant FAIL: EM-clear mutant PASSED -- the gate is decoration\n'; exit 1; }; \
printf '%s\n' "$$out" | grep -q 'FAIL \[nm\]' || { printf '!!! test-fpu-absent-mutant FAIL: RED, but not on [nm]\n'; exit 1; }; \
printf '>>> test-fpu-absent-mutant: green (absent branch without EM=1 -> no #NM -> RED on [nm])\n'
```

## test-fpu-bochs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27906):
Prerequisites: `build/bochs_harness build/fpu_test.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fpu-bochs : x87 bring-up, BOCHS leg (Rule 5)\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-fpu-bochs FAIL: bochs not found (required base tool; SKIP_BOCHS=1 to opt out loudly)\n'; exit 1; }
@$(BOCHS_BIN) --disk "$(FPU_GATE_IMG)" --expect FPU-TEST-DONE \
	--name fpu_test_bochs --out "$(BUILD)" --timeout-ms 45000 \
	2> "$(BUILD)/fpu_test_bochs.report" || true
@cat "$(BUILD)/fpu_test_bochs.report"
@grep -q 'rfb_unblocked=1' "$(BUILD)/fpu_test_bochs.report" \
	|| { printf '!!! test-fpu-bochs FAIL: RFB unblock failed -- Bochs did not run the guest\n'; exit 1; }
@grep -E '^(FPU|PANIC)' "$(BUILD)/fpu_test_bochs.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
@$(call fpu-gate-check,$(BUILD)/fpu_test_bochs.serial,$(BUILD)/fpu_test_bochs.report,test-fpu-bochs)
@printf 'VERDICT   : PASS -- same x87 state + results under Bochs as under QEMU\n'
@printf '======================================================================\n'
```

## test-fpu-mf

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27981):
Prerequisites: `build/qemu_harness build/fpu_mf.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fpu-mf : unmasked x87 fault -> #MF panic (NE=1)\n'
@printf '======================================================================\n'
@$(call fpu-qemu-run,$(FPU_MF_IMG),fpu_mf)
@cat "$(BUILD)/fpu_mf.report"
@grep -E '^(FPU|PANIC)' "$(BUILD)/fpu_mf.serial" | tr -d '\r' | sed 's/^/    serial| /' || true
@$(call fpu-mf-check,$(BUILD)/fpu_mf.serial,$(BUILD)/fpu_mf.report,test-fpu-mf)
@printf 'VERDICT   : PASS -- unmasked #Z delivered as #MF (vector 16), fail-loud panic\n'
@printf '======================================================================\n'
```

## test-fpu-mf-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27992):
Prerequisites: `build/qemu_harness build/fpu_mf_mut_skip.img build/.makefile.stamp`
```make
@$(call fpu-qemu-run,$(BUILD)/fpu_mf_mut_skip.img,fpu_mf_mut_skip)
@out=$$( $(call fpu-mf-check,$(BUILD)/fpu_mf_mut_skip.serial,$(BUILD)/fpu_mf_mut_skip.report,mf-mutant) ); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^/    | /'; \
[ "$$rc" -ne 0 ] || { printf '!!! test-fpu-mf-mutant FAIL: NE=0 mutant PASSED -- the gate is decoration\n'; exit 1; }; \
printf '%s\n' "$$out" | grep -q 'FAIL \[mf\]' || { printf '!!! test-fpu-mf-mutant FAIL: RED, but not on [mf]\n'; exit 1; }; \
printf '>>> test-fpu-mf-mutant: green (NE=0 -> no #MF -> RED on [mf])\n'
```

## test-fpu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27882):
Prerequisites: `build/qemu_harness build/fpu_mut_skip.img build/fpu_mut_cw.img build/.makefile.stamp`
```make
@$(call fpu-qemu-run,$(BUILD)/fpu_mut_skip.img,fpu_mut_skip)
@out=$$( $(call fpu-gate-check,$(BUILD)/fpu_mut_skip.serial,$(BUILD)/fpu_mut_skip.report,skip-mutant) ); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^/    | /'; \
[ "$$rc" -ne 0 ] || { printf '!!! test-fpu-mutant FAIL: skip-init mutant PASSED -- the gate is decoration\n'; exit 1; }; \
printf '%s\n' "$$out" | grep -q 'FAIL \[cr0\]' || { printf '!!! test-fpu-mutant FAIL: skip-init mutant RED, but not on [cr0] (wrong reason)\n'; exit 1; }; \
if printf '%s\n' "$$out" | grep -q 'FAIL \[\(armed\|init\|triple\|serial\|spec\)\]'; then printf '!!! test-fpu-mutant FAIL: skip-init mutant RED for a harness/boot reason, not the hardware state\n'; exit 1; fi; \
printf '>>> test-fpu-mutant [1/2]: skip-init mutant RED on [cr0] (self-report ignored) -- bites\n'
@$(call fpu-qemu-run,$(BUILD)/fpu_mut_cw.img,fpu_mut_cw)
@out=$$( $(call fpu-gate-check,$(BUILD)/fpu_mut_cw.serial,$(BUILD)/fpu_mut_cw.report,cw-mutant) ); rc=$$?; \
printf '%s\n' "$$out" | sed 's/^/    | /'; \
[ "$$rc" -ne 0 ] || { printf '!!! test-fpu-mutant FAIL: control-word mutant PASSED -- the [cw] arm is decoration\n'; exit 1; }; \
printf '%s\n' "$$out" | grep -q 'FAIL \[cw\]' || { printf '!!! test-fpu-mutant FAIL: control-word mutant RED, but not on [cw]\n'; exit 1; }; \
if printf '%s\n' "$$out" | grep -q 'FAIL \[\(cr0\|result\|armed\|triple\)\]'; then printf '!!! test-fpu-mutant FAIL: control-word mutant also broke [cr0]/[result]/boot -- not an isolated CW bite\n'; exit 1; fi; \
printf '>>> test-fpu-mutant [2/2]: control-word mutant RED on [cw] in isolation -- bites\n'
@printf '>>> test-fpu-mutant: green (both mutants RED for the right reason)\n'
```

## test-fpu-probe

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28763):
Prerequisites: `build/test_fpu_probe build/.makefile.stamp`
```make
@printf '>>> test-fpu-probe: x87 probe verdict + CR0 arithmetic (os/milton/fpu.h; beads initech-zj6w)\n'
@$(TEST_FPU_PROBE)
```

## test-fpu-probe-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28766):
Prerequisites: `build/test_fpu_probe_mut_sw_only build/test_fpu_probe_mut_absent_em build/.makefile.stamp`
```make
@for m in $(TEST_FPU_PROBE_MUT1) $(TEST_FPU_PROBE_MUT2); do \
	if $$m >/dev/null 2>&1; then printf '!!! test-fpu-probe-mutant FAIL: %s PASSED -- oracle is decoration\n' "$$m"; exit 1; fi; \
	printf '    %s: RED (correct)\n' "$$(basename $$m)"; \
done
@printf '>>> test-fpu-probe-mutant: green (both mutants correctly RED)\n'
```

## test-fs

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25543):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_data.img build/ppm_text_check build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-fs : MOUNT A REAL FILESYSTEM over ATA\n'
@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 1/2/5.1.\n'
@printf '  beads initech-saw (+ validates initech-adf ATA on the emulator).\n'
@printf '  FIRST functional run of os/milton/ata.c on the emulator.\n'
@printf '  TRI-EMULATOR: QEMU only -- Bochs/86Box deferred to beads initech-x0i.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (boot disk, primary master)\n' "$(SECONDARY_TRACER_IMG)"
@printf 'Data disk : %s (primary SLAVE, if=ide,index=1)\n' "$(FAT_DATA_IMG)"
@printf 'Expecting : FAT-MOUNT-OK + proto-DIR filenames (HELLO.TXT ...) + DIR-OK + no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@# qskz: mount-check filenames and DIR-OK remain on serial; they no longer
@# paint the console. Synchronize the screenshot with the secondary shell,
@# then grade its prompt and the blank former mount-listing rows explicitly.
@# Primary DOS startup is separately graded by test-dos-startup.
@$(call fixture-preflight,$(FAT_DATA_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_DATA_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FLAIR_GATE_DATA)" --screendump \
	--screendump-after SHELL-READY \
	--name "$(FS_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(FS_REPORT)" || true
@cat "$(FS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. Triple-fault check. ----
@if grep -q 'triple_fault=1' "$(FS_REPORT)"; then \
	printf '!!! test-fs FAIL: TRIPLE FAULT detected booting with the data disk\n'; \
	exit 1; \
fi
@printf '>>> test-fs [1/4]: no triple-fault\n'
@# ---- 2. Mount succeeded (ata.c + fat12.c worked end-to-end). ----
@if [ ! -s "$(FS_SERIAL)" ]; then \
	printf '!!! test-fs FAIL: no serial captured at %s\n' "$(FS_SERIAL)"; \
	exit 1; \
fi
@if grep -q '^FAT-MOUNT-FAIL' "$(FS_SERIAL)"; then \
	printf '!!! test-fs FAIL: kernel reported FAT-MOUNT-FAIL (ATA read or BPB check failed) -- root-cause the ATA protocol, do NOT paper over (Rule 3):\n'; \
	grep '^FAT-MOUNT-FAIL' "$(FS_SERIAL)"; \
	exit 1; \
fi
@grep -q '^FAT-MOUNT-OK$$' "$(FS_SERIAL)" \
	|| { printf '!!! test-fs FAIL: FAT-MOUNT-OK missing -- ata.c never delivered sector 0 to fat12_mount (the FIRST emulator ATA run failed)\n'; exit 1; }
@printf '>>> test-fs [2/4]: FAT-MOUNT-OK (ata.c READ SECTORS + fat12_mount green on the emulator)\n'
@# ---- 3. Proto-DIR enumerated the root directory + completed. ----
@for n in $(FS_NAMES); do \
	grep -q "$$n" "$(FS_SERIAL)" \
		|| { printf '!!! test-fs FAIL: proto-DIR filename %s missing -- root-dir enumeration did not list the fixtures\n' "$$n"; exit 1; }; \
done
@grep -q '^DIR-OK$$' "$(FS_SERIAL)" \
	|| { printf '!!! test-fs FAIL: DIR-OK missing -- fat12_read_root_dir did not complete the scan\n'; exit 1; }
@printf '>>> test-fs [3/4]: proto-DIR listed %s + DIR-OK\n' "$(FS_NAMES)"
@# ---- 4. BONUS screendump: text rendered on the seafoam desktop. ----
@if [ ! -s "$(FS_PPM)" ]; then \
	printf '!!! test-fs FAIL: no screendump captured at %s (live guest required)\n' "$(FS_PPM)"; \
	exit 1; \
fi
@{ printf '2 0 A:\\>\n'; for row in 3 4 5 6 7 8; do printf '%s 0 %80s\n' $$row ''; done; } > $(BUILD)/fs_console.cells
@$(PPM_TEXT_CHECK_BIN) --cells "$(FS_PPM)" $(BUILD)/fs_console.cells \
	|| { printf '!!! test-fs FAIL: mount diagnostics leaked onto the DOS console\n'; exit 1; }
@printf '>>> test-fs [4/4]: mount listing stays on serial; console shows the exact secondary prompt\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS mounted FAT12 over ATA; root scan on serial, clean DOS console\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-gnrc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4189):
Prerequisites: `build/test_fat12_rename build/fat12_rename_golden.img build/fat12_rename_subdir_golden.img build/.makefile.stamp`
```make
@command -v mren >/dev/null 2>&1 || { printf '!!! test-gnrc FAIL: mtools `mren` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mmd  >/dev/null 2>&1 || { printf '!!! test-gnrc FAIL: mtools `mmd` not found.\n'; exit 1; }
@printf ">>> test-gnrc: AH=56h RENAME write side vs mren golden (name-field byte-for-byte; chain/size/FAT untouched) + NON-ROOT same-dir leg (beads initech-isil)\n"
@dd if=/dev/zero of=$(FAT12_RENAME_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_ART_IMG) -f 1440 ::
@mcopy -i $(FAT12_RENAME_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::OLD.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_ART_IMG))
@dd if=/dev/zero of=$(FAT12_RENAME_SUBDIR_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_SUBDIR_ART_IMG) -f 1440 ::
@mmd -i $(FAT12_RENAME_SUBDIR_ART_IMG) ::SUB
@mcopy -i $(FAT12_RENAME_SUBDIR_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::SUB/OLD2.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_SUBDIR_ART_IMG))
@$(TEST_FAT12_RENAME) "$(FAT12_RENAME_ART_IMG)" "$(FAT12_RENAME_GOLDEN_IMG)" \
	"$(FAT12_RENAME_SUBDIR_ART_IMG)" "$(FAT12_RENAME_SUBDIR_GOLDEN_IMG)"
@printf ">>> test-gnrc: green\n"
```

## test-gnrc-int21-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6190):
Prerequisites: `build/test_fileio_mutant_gnrc_nosamedir build/test_fileio_mutant_gnrc_notfound build/test_fileio_mutant_gnrc_nodispatch build/.makefile.stamp`
```make
@printf ">>> test-gnrc-int21-mutant: confirming all three do_rename mutants go RED (Rule 6; beads initech-gnrc)\n"
@if $(TEST_FILEIO_MUT_GNRC_NOSAMEDIR) >/dev/null 2>&1; then \
	printf '!!! test-gnrc-int21-mutant FAIL: NO_SAMEDIR_GUARD mutant PASSED -- the cross-dir 0x0011 guard is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-int21-mutant: green (NO_SAMEDIR_GUARD mutant correctly RED -- the cross-dir 0x0011 guard bites)\n'; \
fi
@if $(TEST_FILEIO_MUT_GNRC_NOTFOUND) >/dev/null 2>&1; then \
	printf '!!! test-gnrc-int21-mutant FAIL: NOTFOUND_PATH mutant PASSED -- the source-missing 0x0002 contract is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-int21-mutant: green (NOTFOUND_PATH mutant correctly RED -- the source-missing 0x0002 contract bites)\n'; \
fi
@if $(TEST_FILEIO_MUT_GNRC_NODISPATCH) >/dev/null 2>&1; then \
	printf '!!! test-gnrc-int21-mutant FAIL: NO_DISPATCH mutant PASSED -- the case 0x56 wiring is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-int21-mutant: green (NO_DISPATCH mutant correctly RED -- the case 0x56 dispatch bites)\n'; \
fi
```

## test-gnrc-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4213):
Prerequisites: `build/test_fat12_rename_mut_nodest build/test_fat12_rename_mut_chain build/test_fat12_rename_mut_name8 build/test_fat12_rename_mut_dirstart build/fat12_rename_golden.img build/fat12_rename_subdir_golden.img build/.makefile.stamp`
```make
@printf ">>> test-gnrc-mutant: confirming all four RENAME mutants go RED (Rule 6)\n"
@dd if=/dev/zero of=$(FAT12_RENAME_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_ART_IMG) -f 1440 ::
@mcopy -i $(FAT12_RENAME_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::OLD.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_ART_IMG))
@if $(TEST_FAT12_RENAME_MUT_NODEST) "$(FAT12_RENAME_ART_IMG)" "$(FAT12_RENAME_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-gnrc-mutant FAIL: no-dest-check mutant PASSED -- the dest-exists reject is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-mutant: green (no-dest-check mutant correctly RED -- the dest-exists reject bites)\n'; \
fi
@dd if=/dev/zero of=$(FAT12_RENAME_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_ART_IMG) -f 1440 ::
@mcopy -i $(FAT12_RENAME_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::OLD.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_ART_IMG))
@if $(TEST_FAT12_RENAME_MUT_CHAIN) "$(FAT12_RENAME_ART_IMG)" "$(FAT12_RENAME_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-gnrc-mutant FAIL: touch-chain mutant PASSED -- the start_cluster/size-unchanged assertion is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-mutant: green (touch-chain mutant correctly RED -- rename is name-field-only)\n'; \
fi
@dd if=/dev/zero of=$(FAT12_RENAME_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_ART_IMG) -f 1440 ::
@mcopy -i $(FAT12_RENAME_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::OLD.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_ART_IMG))
@if $(TEST_FAT12_RENAME_MUT_NAME8) "$(FAT12_RENAME_ART_IMG)" "$(FAT12_RENAME_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-gnrc-mutant FAIL: name-only8 mutant PASSED -- the extension is dropped from the name-field diff\n'; exit 1; \
else \
	printf '>>> test-gnrc-mutant: green (name-only8 mutant correctly RED -- the extension rewrite bites)\n'; \
fi
@# m4 (beads initech-isil): the ignore-dir_start mutant must turn the NON-ROOT
@# same-dir leg RED. Re-seed BOTH images (the root leg + the subdir leg run);
@# the subdir leg fails to find \SUB\OLD2.TXT root-anchored -> NOT_FOUND -> RED.
@dd if=/dev/zero of=$(FAT12_RENAME_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_ART_IMG) -f 1440 ::
@mcopy -i $(FAT12_RENAME_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::OLD.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_ART_IMG))
@dd if=/dev/zero of=$(FAT12_RENAME_SUBDIR_ART_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_RENAME_SUBDIR_ART_IMG) -f 1440 ::
@mmd -i $(FAT12_RENAME_SUBDIR_ART_IMG) ::SUB
@mcopy -i $(FAT12_RENAME_SUBDIR_ART_IMG) $(FAT12_FIXTURE_DIR)/chain.txt ::SUB/OLD2.TXT
@$(call fixture-check,recipe,$(FAT12_RENAME_SUBDIR_ART_IMG))
@if $(TEST_FAT12_RENAME_MUT_DIRSTART) "$(FAT12_RENAME_ART_IMG)" "$(FAT12_RENAME_GOLDEN_IMG)" \
	"$(FAT12_RENAME_SUBDIR_ART_IMG)" "$(FAT12_RENAME_SUBDIR_GOLDEN_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-gnrc-mutant FAIL: ignore-dir_start mutant PASSED -- the NON-ROOT same-dir leg is decoration\n'; exit 1; \
else \
	printf '>>> test-gnrc-mutant: green (ignore-dir_start mutant correctly RED -- the subdir dir_start path bites)\n'; \
fi
```

## test-hardware-spec

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28732):
Prerequisites: `build/test_hardware_spec build/.makefile.stamp`
```make
@printf '>>> test-hardware-spec: spec/hardware.json contract (DEC-07)\n'
@$(TEST_HWSPEC) spec/hardware.json
@printf '>>> test-hardware-spec: green\n'
```

## test-hardware-spec-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28738):
Prerequisites: `build/test_hardware_spec_mut build/test_hardware_spec_mut_fpu build/.makefile.stamp`
```make
@if $(TEST_HWSPEC_MUT) spec/hardware.json >/dev/null 2>&1; then printf '!!! test-hardware-spec-mutant FAIL: mutant PASSED -- oracle is decoration\n'; exit 1; \
else printf '>>> test-hardware-spec-mutant: all-arms mutant correctly RED\n'; fi
@out=$$($(TEST_HWSPEC_MUT_FPU) spec/hardware.json 2>&1); rc=$$?; \
[ "$$rc" -ne 0 ] || { printf '!!! test-hardware-spec-mutant FAIL: fpu boot_control_word mutant PASSED -- the arm is decoration\n'; exit 1; }; \
n=$$(printf '%s\n' "$$out" | grep -c '  FAIL '); \
printf '%s\n' "$$out" | grep -q 'FAIL .*fpu.boot_control_word == 0x027F' && [ "$$n" -eq 1 ] \
	|| { printf '!!! test-hardware-spec-mutant FAIL: fpu mutant RED for the wrong reason (%s failures):\n%s\n' "$$n" "$$out"; exit 1; }; \
printf '>>> test-hardware-spec-mutant: green (fpu boot_control_word mutant RED on exactly that arm)\n'
```

## test-host-mapping-arena_disjoint-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 120):
Prerequisites: `build/test_mapping_arena_disjoint build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-mapping-arena_disjoint.log 2>&1; then echo 'FAIL zero-owner mapping mutant survived'; exit 1; fi
@grep 'FAIL.*deterministic PSP' $(BUILD)/khygiene-evidence/mutant-mapping-arena_disjoint.log
@echo 'GREEN test-host-mapping-arena_disjoint-mutant: HOST_LOW_MUTATE_ZERO_OWNER correctly RED'
```

## test-host-mapping-keep-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 119):
Prerequisites: `build/test_mapping_keep build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-mapping-keep.log 2>&1; then echo 'FAIL zero-owner mapping mutant survived'; exit 1; fi
@grep 'FAIL.*deterministic PSP' $(BUILD)/khygiene-evidence/mutant-mapping-keep.log
@echo 'GREEN test-host-mapping-keep-mutant: HOST_LOW_MUTATE_ZERO_OWNER correctly RED'
```

## test-host-mapping-mcb_int21-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 121):
Prerequisites: `build/test_mapping_mcb_int21 build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-mapping-mcb_int21.log 2>&1; then echo 'FAIL zero-owner mapping mutant survived'; exit 1; fi
@grep 'FAIL.*deterministic PSP' $(BUILD)/khygiene-evidence/mutant-mapping-mcb_int21.log
@echo 'GREEN test-host-mapping-mcb_int21-mutant: HOST_LOW_MUTATE_ZERO_OWNER correctly RED'
```

## test-idt

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5311):
Prerequisites: `build/test_idt build/.makefile.stamp`
```make
@printf ">>> test-idt: IDT gate encode (offset-split) + int_frame_t field offsets\n"
@$(TEST_IDT)
@printf ">>> test-idt: green\n"
```

## test-idt-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5318):
Prerequisites: `build/test_idt_mutant build/.makefile.stamp`
```make
@printf ">>> test-idt-mutant: confirming the offset-hi mutant goes RED (Rule 6)\n"
@if $(TEST_IDT_MUT) >/dev/null 2>&1; then \
	printf '!!! test-idt-mutant FAIL: mutant PASSED the oracle -- the encode test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-idt-mutant: green (mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-int21

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5752):
Prerequisites: `build/test_int21 build/.makefile.stamp`
```make
@printf ">>> test-int21: AH-dispatch + console subset (02h/09h/40h/30h/4Ch) + controlled scope\n"
@$(TEST_INT21)
@printf ">>> test-int21: green\n"
```

## test-int21-dpb

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/int21_dpb.mk', line 17):
Prerequisites: `build/int21_dpb_host build/tracer_secondary.img build/.makefile.stamp`
```make
@sh $(INT21_DPB_SH) host $(BUILD)/int21_dpb_host $(SECONDARY_TRACER_IMG)
```

## test-int21-dpb-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/int21_dpb.mk', line 24):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/int21_dpb_program.bin build/.makefile.stamp`
```make
@sh $(INT21_DPB_SH) emu $(SECONDARY_TRACER_IMG)
```

## test-int21-dpb-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/int21_dpb.mk', line 26):
Prerequisites: `test-int21-dpb-emu build/qemu_harness build/j1lu_dpb_first_data.img build/int21_dpb_program.bin build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/int21-dpb
@if sh $(INT21_DPB_SH) emu $(BUILD)/j1lu_dpb_first_data.img first-data > $(BUILD)/int21-dpb/first-data-emu.log 2>&1; then echo 'FAIL int21-dpb FIRST_DATA survived QEMU'; exit 1; fi
@grep '^RED int21-dpb A first data sector 0Bh' $(BUILD)/int21-dpb/first-data-emu.log | head -1
@echo 'GREEN test-int21-dpb-emu-mutant: FIRST_DATA rejected in QEMU'
```

## test-int21-dpb-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/int21_dpb.mk', line 19):
Prerequisites: `test-int21-dpb build/int21_dpb_host_first_data build/tracer_secondary.img build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/int21-dpb
@if sh $(INT21_DPB_SH) host $(BUILD)/int21_dpb_host_first_data $(SECONDARY_TRACER_IMG) first-data > $(BUILD)/int21-dpb/first-data.log 2>&1; then echo 'FAIL int21-dpb FIRST_DATA survived'; exit 1; fi
@grep '^RED int21-dpb A first data sector 0Bh' $(BUILD)/int21-dpb/first-data.log | head -1
@echo 'GREEN test-int21-dpb-mutant: FIRST_DATA rejected'
```

## test-int21-edge

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7064):
Prerequisites: `build/test_int21_edge build/.makefile.stamp`
```make
@printf ">>> test-int21-edge: INT 21h error paths (bad/closed handle, double-close, CX=0, short read) + 0Eh/19h/47h/59h/62h via mock backend (initech-xrd/1zk)\n"
@$(TEST_INT21EDGE)
@printf ">>> test-int21-edge: green\n"
```

## test-int21-edge-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7070):
Prerequisites: `build/test_int21_edge_mutant_close build/test_int21_edge_mutant_ptr build/.makefile.stamp`
```make
@printf ">>> test-int21-edge-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_INT21EDGE_MUT_CLOSE) >/dev/null 2>&1; then \
	printf '!!! test-int21-edge-mutant FAIL: close-no-refguard mutant PASSED -- the double-close no-underflow test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int21-edge-mutant: green (close-no-refguard mutant correctly RED -- the ref_count guard oracle bites)\n'; \
fi
@if $(TEST_INT21EDGE_MUT_PTR) >/dev/null 2>&1; then \
	printf '!!! test-int21-edge-mutant FAIL: no-ptr-guard mutant PASSED -- the user-pointer guard test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int21-edge-mutant: green (no-ptr-guard mutant correctly RED -- the DEC-14 user-pointer guard bites)\n'; \
fi
```

## test-int21-irqstorm

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26324):
Prerequisites: `build/qemu_harness build/irqstorm_boot.img build/fat_irqstorm.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-int21-irqstorm : INT 21h reentrancy under an IRQ storm\n'
@printf '  Ref: beads initech-xk2; TRAP gate leaves IF set; irq.c/int21.c guard + InDOS.\n'
@printf '  FINDFIRST/NEXT + a multi-cluster READ + a 2nd handle, WHILE keys storm (IRQ1) + PIT (IRQ0).\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + storm disk %s (primary slave)\n' "$(IRQSTORM_IMG)" "$(FAT_IRQSTORM_IMG)"
@printf 'Expecting : exact dir enum + STORM-SIG=%s + STORM-B=alpha + IRQSTORM-EXIT rc=0 + NO reentry panic + keys_sent>0\n' "$(STORM_SIG)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_IRQSTORM_IMG))
@$(HARNESS_BIN) --disk "$(IRQSTORM_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
	--name "$(IRQSTORM_NAME)" --out "$(BUILD)" --timeout-ms 10000 \
	--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
	--quit-after "IRQSTORM-EXIT rc=0" \
	2> "$(IRQSTORM_REPORT)" || true
@cat "$(IRQSTORM_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(IRQSTORM_REPORT)"; then \
	printf '!!! test-int21-irqstorm FAIL: TRIPLE FAULT (root-cause the reentrancy path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-int21-irqstorm [1/6]: no triple-fault\n'
@if [ ! -s "$(IRQSTORM_SERIAL)" ]; then \
	printf '!!! test-int21-irqstorm FAIL: no serial captured at %s\n' "$(IRQSTORM_SERIAL)"; exit 1; \
fi
@# 2. The guard did NOT fire in normal operation (no ISR calls DOS).
@if grep -q 'INT21-REENTRY-PANIC\|PANIC vec=' "$(IRQSTORM_SERIAL)"; then \
	printf '!!! test-int21-irqstorm FAIL: the reentrancy guard FIRED in normal operation -- an ISR entered the dispatcher (root-cause, Rule 3):\n'; \
	grep 'INT21-REENTRY-PANIC\|PANIC vec=' "$(IRQSTORM_SERIAL)"; exit 1; \
fi
@printf '>>> test-int21-irqstorm [2/6]: reentrancy guard stayed quiescent (no ISR called DOS)\n'
@# 3. The keystroke storm actually happened (else the IRQ1 pressure is absent).
@if grep -q 'keys_sent=0' "$(IRQSTORM_REPORT)"; then \
	printf '!!! test-int21-irqstorm FAIL: keys_sent=0 -- the IRQ1 storm never fired, the test proves nothing (Law 2)\n'; exit 1; \
fi
@printf '>>> test-int21-irqstorm [3/6]: the keystroke storm fired (keys_sent>0, IRQ1 pressure present)\n'
@grep -q '^FILEIO-BIND-OK$$' "$(IRQSTORM_SERIAL)" \
	|| { printf '!!! test-int21-irqstorm FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
@if grep -q 'STORM-OPENA-FAIL\|STORM-OPENB-FAIL\|STORM-READ-FAIL' "$(IRQSTORM_SERIAL)"; then \
	printf '!!! test-int21-irqstorm FAIL: a STORM-*-FAIL marker -- an OPEN/READ failed under the storm (root-cause, Rule 3):\n'; \
	grep 'STORM-.*-FAIL' "$(IRQSTORM_SERIAL)"; exit 1; \
fi
@# 4. The FINDFIRST/NEXT enumeration returned EXACTLY the right filenames. The
@# program emits CRLF lines, so strip CR before the anchored sed slice.
@tr -d '\r' < "$(IRQSTORM_SERIAL)" | sed -n '/^STORM-DIR-BEGIN$$/,/^STORM-DIR-END$$/p' > "$(BUILD)/$(IRQSTORM_NAME).dir"
@for n in $(IRQSTORM_NAMES); do \
	grep -qF "$$n" "$(BUILD)/$(IRQSTORM_NAME).dir" \
		|| { printf '!!! test-int21-irqstorm FAIL: FINDFIRST/NEXT enumeration missing %s -- async g_find/g_dta corruption under the storm (Rule 3):\n' "$$n"; cat "$(BUILD)/$(IRQSTORM_NAME).dir"; exit 1; }; \
done
@# Exactly 5 surviving names (no skip, no duplicate). Count non-marker lines.
@cnt=$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/$(IRQSTORM_NAME).dir"); \
if [ "$$cnt" -ne 5 ]; then \
	printf '!!! test-int21-irqstorm FAIL: expected exactly 5 enumerated names, got %s -- async corruption skipped/duplicated an entry (Rule 3):\n' "$$cnt"; \
	cat "$(BUILD)/$(IRQSTORM_NAME).dir"; exit 1; \
fi
@printf '>>> test-int21-irqstorm [4/6]: FINDFIRST/NEXT enumerated EXACTLY the 5 files under the storm\n'
@# 5. The multi-cluster READ + the 2nd handle returned the EXACT bytes.
@grep -qF 'STORM-SIG=$(STORM_SIG)' "$(IRQSTORM_SERIAL)" \
	|| { printf '!!! test-int21-irqstorm FAIL: STORM-SIG=%s missing -- the multi-cluster READ returned wrong bytes (FAT cache/cluster scratch corrupted by an IRQ mid-read, Rule 3)\n' "$(STORM_SIG)"; grep 'STORM-SIG' "$(IRQSTORM_SERIAL)" || true; exit 1; }
@grep -qF 'STORM-B=alpha' "$(IRQSTORM_SERIAL)" \
	|| { printf '!!! test-int21-irqstorm FAIL: STORM-B=alpha missing -- the 2nd handle read wrong bytes (per-handle position/SFT corrupted, Rule 3)\n'; grep 'STORM-B' "$(IRQSTORM_SERIAL)" || true; exit 1; }
@printf '>>> test-int21-irqstorm [5/6]: multi-cluster READ (STORM-SIG=%s) + 2nd handle (STORM-B=alpha) EXACT under the storm\n' "$(STORM_SIG)"
@# 6. The program finished cleanly (the run_baked -EXIT marker; substring match
@# because the program's preceding stdout line carries no trailing newline).
@grep -qF 'IRQSTORM-EXIT rc=0' "$(IRQSTORM_SERIAL)" \
	|| { printf '!!! test-int21-irqstorm FAIL: IRQSTORM-EXIT rc=0 missing -- the program did not finish cleanly under the storm\n'; exit 1; }
@printf '>>> test-int21-irqstorm [6/6]: IRQSTORM-EXIT rc=0 (clean finish under the storm)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- INT 21h stayed reentrancy-safe with IRQs LIVE under a keystroke storm:\n'
@printf '            no frame/result corruption, guard quiescent (QEMU only; tri-emulator pending initech-x0i)\n'
@printf '======================================================================\n'
```

## test-int21-irqstorm-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26409):
Prerequisites: `build/qemu_harness build/irqstorm_muta_boot.img build/irqstorm_mutb_boot.img build/fat_irqstorm.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-int21-irqstorm-mutant : prove the storm oracle BITES (Rule 6)\n'
@printf '======================================================================\n'
@# ---- MUTANT A: scribble a DOS global from the PIT ISR -> enum goes WRONG. ----
@# MUTANT A still runs to completion (a data bug, not a crash) -- quit-after
@# the SAME clean-finish marker as the base gate.
@$(call fixture-preflight,$(FAT_IRQSTORM_IMG))
@$(HARNESS_BIN) --disk "$(IRQSTORM_MUTA_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
	--name "irqstorm_muta" --out "$(BUILD)" --timeout-ms 10000 \
	--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
	--quit-after "IRQSTORM-EXIT rc=0" \
	2> "$(BUILD)/irqstorm_muta.report" || true
@tr -d '\r' < "$(BUILD)/irqstorm_muta.serial" 2>/dev/null | sed -n '/^STORM-DIR-BEGIN$$/,/^STORM-DIR-END$$/p' > "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || true
@cnt=$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || echo 0); \
if [ "$$cnt" -eq 5 ]; then \
	printf '!!! test-int21-irqstorm-mutant FAIL: MUTANT A enumerated all 5 names -- the storm oracle did NOT detect the async g_find corruption (it is decoration):\n'; \
	cat "$(BUILD)/irqstorm_muta.dir" 2>/dev/null; exit 1; \
fi
@printf '>>> test-int21-irqstorm-mutant: MUTANT A correctly RED (PIT scribble corrupted the enumeration: %s/5 names)\n' "$$(grep -cE 'ALPHA\.TXT|BRAVO\.TXT|CHARLIE\.TXT|DELTA\.TXT|STORM\.DAT' "$(BUILD)/irqstorm_muta.dir" 2>/dev/null || echo 0)"
@# ---- MUTANT B: issue int 0x21 from the PIT ISR -> the guard PANICS. ----
@# MUTANT B PANICS instead of ever reaching IRQSTORM-EXIT -- the panic
@# marker itself IS this leg's completion signal.
@$(call fixture-preflight,$(FAT_IRQSTORM_IMG))
@$(HARNESS_BIN) --disk "$(IRQSTORM_MUTB_IMG)" --disk2 "$(FAT_IRQSTORM_IMG)" \
	--name "irqstorm_mutb" --out "$(BUILD)" --timeout-ms 10000 \
	--keys "$(IRQSTORM_KEYS)" --keys-after "IRQSTORM-READY" \
	--quit-after "INT21-REENTRY-PANIC" \
	2> "$(BUILD)/irqstorm_mutb.report" || true
@grep -q 'INT21-REENTRY-PANIC' "$(BUILD)/irqstorm_mutb.serial" 2>/dev/null \
	|| { printf '!!! test-int21-irqstorm-mutant FAIL: MUTANT B did NOT trip the reentrancy guard -- the int-0x21-from-IRQ went undetected (the guard is decoration). Serial tail:\n'; tail -20 "$(BUILD)/irqstorm_mutb.serial" 2>/dev/null; exit 1; }
@printf '>>> test-int21-irqstorm-mutant: MUTANT B correctly RED (guard PANICKED: INT21-REENTRY-PANIC on int-0x21-from-IRQ)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- both mutants went RED; the storm oracle BITES on async\n'
@printf '            shared-state corruption (A) AND on a forbidden IRQ-issued reentry (B)\n'
@printf '======================================================================\n'
```

## test-int21-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5758):
Prerequisites: `build/test_int21_mutant_dollar build/test_int21_mutant_noop build/.makefile.stamp`
```make
@printf ">>> test-int21-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_INT21_MUT_DOLLAR) >/dev/null 2>&1; then \
	printf '!!! test-int21-mutant FAIL: 09h-emit-dollar mutant PASSED -- the PUTS test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int21-mutant: green (09h-emit-dollar mutant correctly RED)\n'; \
fi
@if $(TEST_INT21_MUT_NOOP) >/dev/null 2>&1; then \
	printf '!!! test-int21-mutant FAIL: unlisted-AH-noop mutant PASSED -- the controlled-scope test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int21-mutant: green (unlisted-AH-noop mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-int24

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6010):
Prerequisites: `build/test_int24 build/.makefile.stamp`
```make
@printf ">>> test-int24: INT 22/23/24 + SETVECT/GETVECT (25h/35h) + PSP-vector save/restore (DEC-10)\n"
@$(TEST_INT24)
@printf ">>> test-int24: green\n"
```

## test-int24-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6016):
Prerequisites: `build/test_int24_mutant_afswap build/test_int24_mutant_noreprompt build/test_int24_mutant_vecoff build/test_int24_mutant_getvect build/.makefile.stamp`
```make
@printf ">>> test-int24-mutant: confirming all four mutants go RED (Rule 6)\n"
@if $(TEST_INT24_MUT_AFSWAP) >/dev/null 2>&1; then \
	printf '!!! test-int24-mutant FAIL: A/F-swap mutant PASSED -- the crit-action mapping test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int24-mutant: green (crit-action A/F-swap mutant correctly RED)\n'; \
fi
@if $(TEST_INT24_MUT_NOREPROM) >/dev/null 2>&1; then \
	printf '!!! test-int24-mutant FAIL: no-re-prompt mutant PASSED -- the int24 re-prompt test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int24-mutant: green (int24 no-re-prompt mutant correctly RED)\n'; \
fi
@if $(TEST_INT24_MUT_VECOFF) >/dev/null 2>&1; then \
	printf '!!! test-int24-mutant FAIL: vec-offset mutant PASSED -- the PSP save/restore offset test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int24-mutant: green (psp save vec-offset mutant correctly RED)\n'; \
fi
@if $(TEST_INT24_MUT_GETVECT) >/dev/null 2>&1; then \
	printf '!!! test-int24-mutant FAIL: getvect-EAX mutant PASSED -- the GETVECT register test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-int24-mutant: green (do_getvect wrong-register mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-int24-wired

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7940):
Prerequisites: `build/test_int24_wired build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-int24-wired FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@printf ">>> test-int24-wired: INT 24h raised by the disk layer + A/R/F honored (beads initech-mvg)\n"
@dd if=/dev/zero of=$(TEST_INT24_WIRED_IMG) bs=512 count=2880 status=none
@mformat -i $(TEST_INT24_WIRED_IMG) -f 1440 ::
@$(TEST_INT24_WIRED) "$(TEST_INT24_WIRED_IMG)"
@printf ">>> test-int24-wired: green\n"
```

## test-int24-wired-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7948):
Prerequisites: `build/test_int24_wired_mut_noraise build/test_int24_wired_mut_unbound build/.makefile.stamp`
```make
@dd if=/dev/zero of=$(TEST_INT24_WIRED_IMG) bs=512 count=2880 status=none
@mformat -i $(TEST_INT24_WIRED_IMG) -f 1440 ::
@printf ">>> test-int24-wired-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_INT24_WIRED_MUT_NORAISE) "$(TEST_INT24_WIRED_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-int24-wired-mutant FAIL: NO_RAISE mutant PASSED -- the int24-is-raised oracle is decoration\n'; exit 1; \
else printf '>>> test-int24-wired-mutant: green (NO_RAISE correctly RED)\n'; fi
@if timeout 15 $(TEST_INT24_WIRED_MUT_UNBOUND) "$(TEST_INT24_WIRED_IMG)" >/dev/null 2>&1; then \
	printf '!!! test-int24-wired-mutant FAIL: RETRY_UNBOUNDED mutant PASSED -- the retry-bound oracle is decoration\n'; exit 1; \
else printf '>>> test-int24-wired-mutant: green (RETRY_UNBOUNDED correctly RED -- the oracle bites)\n'; fi
```

## test-j1lu-con-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/dos_control.mk', line 25):
Prerequisites: `build/test_j1lu_con_eof build/test_j1lu_con_raw build/test_j1lu_con_abort build/.makefile.stamp`
```make
@set -e; for m in eof raw abort; do \
  rc=0; $(BUILD)/test_j1lu_con_$$m >$(BUILD)/j1lu_con_$$m.log 2>&1 || rc=$$?; \
  test $$rc -eq 1; grep -Eq '^test_devwire: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/j1lu_con_$$m.log; \
  printf '>>> test-j1lu-con-mutant: green (%s correctly RED: ' "$$m"; \
  tail -1 $(BUILD)/j1lu_con_$$m.log | tr '\n' ')'; printf '\n'; \
done
```

## test-j1lu-con-unit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28146):
Prerequisites: `build/test_devwire build/.makefile.stamp`
```make
@$(TEST_DEVWIRE)
@printf '>>> test-j1lu-con-unit: green\n'
```

## test-j1lu-control-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 28149):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/.makefile.stamp`
```make
@sh harness/emu/test_dos_control.sh $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD) j1lu_control
```

## test-j1lu-control-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 221):
Prerequisites: `build/bochs_harness build/tracer_secondary.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-j1lu-control-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/tracer_secondary.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-j1lu-control-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/j1lu-control-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/tracer_secondary.img" --disk2 "$(BUILD)/j1lu-control-emu_bochs_data.img"  --expect SHELL-READY --quit-after SHELL-READY --name "j1lu-control-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/j1lu-control-emu_bochs.report.txt" || true
@cat "$(BUILD)/j1lu-control-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-j1lu-control-emu-bochs "$(BUILD)/j1lu-control-emu_bochs.report.txt" "$(BUILD)/j1lu-control-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE SHELL-READY !PANIC
```

## test-j1lu-control-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/dos_control.mk', line 58):
Prerequisites: `build/qemu_harness build/j1lu_ctrl.img build/j1lu_eof.img build/j1lu_raw.img build/j1lu_abort.img build/j1lu_restore.img build/.makefile.stamp`
```make
@set -e; for m in ctrl eof raw abort restore; do \
  for attempt in 1 2; do \
    log=$(BUILD)/j1lu_emu_$${m}_$${attempt}.log; rc=0; \
    J1LU_TIMEOUT_MS=25000 sh harness/emu/test_dos_control.sh $(HARNESS_BIN) $(BUILD)/j1lu_$$m.img $(BUILD) j1lu_$${m}_$${attempt} >$$log 2>&1 || rc=$$?; \
    test $$rc -eq 1; \
    grep -Fq 'FAIL: prompt did not return before' $$log; \
    grep -Fq SHELL-READY $(BUILD)/j1lu_$${m}_$${attempt}.serial; \
    if [ "$$m" = abort ]; then \
      grep -Fq INT23-BREAK $(BUILD)/j1lu_$${m}_$${attempt}.serial; \
      grep -Fq 'before abortok' $$log; \
    elif [ "$$m" = restore ]; then grep -Fq 'before restored' $$log; \
    else grep -Fq 'before eofok' $$log; fi; \
    printf '>>> test-j1lu-control-emu-mutant: %s attempt %s correctly RED (booted; ' "$$m" "$$attempt"; \
    sed -n 's/.*FAIL: / /p' $$log | tr '\n' ')'; printf '\n'; \
  done; \
done
@printf '>>> test-j1lu-control-emu-mutant: green (ctrl/eof/raw/abort/restore each RED twice, isolated QEMU reruns)\n'
```

## test-j1lu-kbd-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/dos_control.mk', line 18):
Prerequisites: `build/test_j1lu_kbd_ctrl build/test_j1lu_kbd_e0 build/test_j1lu_kbd_release build/.makefile.stamp`
```make
@set -e; for m in ctrl e0 release; do \
  rc=0; $(BUILD)/test_j1lu_kbd_$$m >$(BUILD)/j1lu_kbd_$$m.log 2>&1 || rc=$$?; \
  test $$rc -eq 1; grep -Eq '^test-kbd-unit: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/j1lu_kbd_$$m.log; \
  printf '>>> test-j1lu-kbd-mutant: green (%s correctly RED: ' "$$m"; \
  tail -1 $(BUILD)/j1lu_kbd_$$m.log | tr '\n' ')'; printf '\n'; \
done
```

## test-kbd

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 28080):
Prerequisites: `build/qemu_harness build/kbd_echo_boot.img build/.makefile.stamp`
```make
@$(HARNESS_BIN) --disk "$(KBD_ECHO_IMG)" --name "$(KBD_NAME)" --out "$(BUILD)" \
 --timeout-ms 60000 --keys "$(KBD_KEYS)" --keys-after KBD-ECHO-READY \
 --quit-after KBD-ECHO-END 2> "$(KBD_REPORT)" || { cat "$(KBD_REPORT)"; exit 1; }
@cat "$(KBD_REPORT)"
@sh harness/conkeys/extended_check.sh $@ "$(KBD_SERIAL)" "$(KBD_REPORT)"
```

## test-kbd-bochs

Declared tier: outside active vector; #  recipe to execute (from 'Makefile', line 28234):
Prerequisites: `build/bochs_harness build/kbd_echo_boot.img build/.makefile.stamp`
```make
@$(BOCHS_BIN) --disk "$(KBD_ECHO_IMG)" --name kbd_echo_bochs --out "$(BUILD)" \
 --timeout-ms 60000 --keys "$(KBD_KEYS)" --keys-after KBD-ECHO-READY \
 --expect KBD-ECHO-END --quit-after KBD-ECHO-END \
 2> "$(BUILD)/kbd_echo_bochs.report" || { cat "$(BUILD)/kbd_echo_bochs.report"; exit 1; }
@cat "$(BUILD)/kbd_echo_bochs.report"
@sh harness/emu/bochs_leg_check.sh $@ "$(BUILD)/kbd_echo_bochs.report" \
 "$(BUILD)/kbd_echo_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE KBD-ECHO-READY KBD-ECHO-END !PANIC
@sh harness/conkeys/extended_check.sh $@ "$(BUILD)/kbd_echo_bochs.serial" "$(BUILD)/kbd_echo_bochs.report"
```

## test-kbd-unit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5384):
Prerequisites: `build/test_kbd build/.makefile.stamp`
```make
@printf ">>> test-kbd-unit: ring (enqueue/dequeue/empty/full/wrap) + scancode set 1 -> ASCII (+shift/caps) + PIT divisor math\n"
@$(TEST_KBD)
@printf ">>> test-kbd-unit: green\n"
```

## test-kbd-unit-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5390):
Prerequisites: `build/test_kbd_mutant_ring build/test_kbd_mutant_table build/test_kbd_mutant_shift build/test_kbd_mutant_caps build/test_kbd_mutant_extended build/test_kbd_mutant_nav build/test_kbd_mutant_ctrl_scan build/.makefile.stamp`
```make
@printf ">>> test-kbd-unit-mutant: confirming all mutants go RED (Rule 6)\n"
@if $(TEST_KBD_MUT_RING) >/dev/null 2>&1; then \
	printf '!!! test-kbd-unit-mutant FAIL: ring off-by-one mutant PASSED -- the full/wrap test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-kbd-unit-mutant: green (ring off-by-one mutant correctly RED)\n'; \
fi
@if $(TEST_KBD_MUT_TABLE) >/dev/null 2>&1; then \
	printf '!!! test-kbd-unit-mutant FAIL: scancode-table mutant PASSED -- the translate test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-kbd-unit-mutant: green (scancode-table mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_KBD_MUT_SHIFT) >/dev/null 2>&1; then \
	printf '!!! test-kbd-unit-mutant FAIL: one-Shift mutant PASSED -- the overlapping Shift test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-kbd-unit-mutant: green (one-Shift mutant correctly RED)\n'; \
fi
@if $(TEST_KBD_MUT_CAPS) >/dev/null 2>&1; then \
	printf '!!! test-kbd-unit-mutant FAIL: Caps-repeat mutant PASSED -- the typematic test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-kbd-unit-mutant: green (Caps-repeat mutant correctly RED)\n'; \
fi
@if $(TEST_KBD_MUT_EXT) >$(BUILD)/kbd_mut_extended.log 2>&1; then \
	printf '!!! test-kbd-unit-mutant FAIL: KBD_MUT_DROP_EXTENDED survived\n'; exit 1; fi
@grep -q 'FAIL.*F3 transport:' $(BUILD)/kbd_mut_extended.log
@printf '>>> test-kbd-unit-mutant: green (KBD_MUT_DROP_EXTENDED correctly RED)\n'
@if $(TEST_KBD_MUT_NAV) >$(BUILD)/kbd_mut_nav.log 2>&1; then \
 printf '!!! test-kbd-unit-mutant FAIL: KBD_MUT_DROP_NAV survived\n'; exit 1; fi
@grep -q 'FAIL.*IBM extended pair: NUL prefix' $(BUILD)/kbd_mut_nav.log
@grep -q 'key=48 .*missing/' $(BUILD)/kbd_mut_nav.log
@printf '>>> test-kbd-unit-mutant: green (KBD_MUT_DROP_NAV correctly RED: Up lost)\n'
@if $(TEST_KBD_MUT_CTRL) >$(BUILD)/kbd_mut_ctrl_scan.log 2>&1; then \
 printf '!!! test-kbd-unit-mutant FAIL: KBD_MUT_WRONG_CTRL_SCAN survived\n'; exit 1; fi
@grep -q 'FAIL.*IBM extended pair: exact scan' $(BUILD)/kbd_mut_ctrl_scan.log
@grep -q 'key=47 .*want=00,77 got=78' $(BUILD)/kbd_mut_ctrl_scan.log
@printf '>>> test-kbd-unit-mutant: green (KBD_MUT_WRONG_CTRL_SCAN correctly RED: Ctrl-Home 78 != 77)\n'
```

## test-keep

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7795):
Prerequisites: `build/test_keep build/.makefile.stamp`
```make
@printf ">>> test-keep: AH=31h KEEP (TSR) -- mcb resident model + shrink/mark + ALLOC-avoids-kept + terminate-survives + AH=4Dh type 3\n"
@$(TEST_KEEP)
@printf ">>> test-keep: green\n"
```

## test-keep-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7800):
Prerequisites: `build/test_keep_mutant_noresident build/.makefile.stamp`
```make
@printf ">>> test-keep-mutant: confirming the no-resident mutant goes RED (Rule 6)\n"
@if $(TEST_KEEP_MUT) >/dev/null 2>&1; then \
	printf '!!! test-keep-mutant FAIL: no-resident mutant PASSED -- the resident oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-keep-mutant: green (no-resident mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-kernel-certificate

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 93):
Prerequisites: `test-40oq test-dos-volumes test-fat16-emu test-kernel-coverage build/.makefile.stamp`
```make
@echo 'GREEN test-kernel-certificate: Appendix A + two volumes + FAT16 ATA'
```

## test-kernel-coverage

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 84):
Prerequisites: `build/int21_coverage build/int21_inventory build/.makefile.stamp`
```make
@$(kernel-coverage-inputs)
@$(call kernel-coverage-run,spec/int21h_register.json)
```

## test-kernel-coverage-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dos_volumes.mk', line 87):
Prerequisites: `build/int21_coverage build/int21_inventory build/.makefile.stamp`
```make
@$(kernel-coverage-inputs)
@sed '/"ah": "30h"/d' spec/int21h_register.json > $(BUILD)/int21_register_drop_getver.json
@if $(call kernel-coverage-run,$(BUILD)/int21_register_drop_getver.json) > $(BUILD)/int21_coverage_mutant.log 2>&1; then echo 'FAIL kernel-coverage DROP_REGISTER survived'; exit 1; fi
@grep '^RED kernel-coverage register dropped dispatcher AH=30' $(BUILD)/int21_coverage_mutant.log
@echo 'GREEN test-kernel-coverage-mutant: DROP_REGISTER_GETVER rejected'
```

## test-kji0-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6136):
Prerequisites: `build/test_fileio_mutant_kji0_noguard build/test_fileio_mutant_kji0_invert build/test_fileio_mutant_kji0_wrongconst build/test_fileio_mutant_kji0_nodispatch build/.makefile.stamp`
```make
@printf ">>> test-kji0-mutant: confirming all four CREATNEW mutants go RED (Rule 6; beads initech-kji0)\n"
@if $(TEST_FILEIO_MUT_KJI0_NOGUARD) >/dev/null 2>&1; then \
	printf '!!! test-kji0-mutant FAIL: NO_GUARD mutant PASSED -- the exists-guard is decoration\n'; exit 1; \
else \
	printf '>>> test-kji0-mutant: green (NO_GUARD mutant correctly RED -- the exists-guard bites)\n'; \
fi
@if $(TEST_FILEIO_MUT_KJI0_INVERT) >/dev/null 2>&1; then \
	printf '!!! test-kji0-mutant FAIL: GUARD_INVERT mutant PASSED -- the probe-sense test is decoration\n'; exit 1; \
else \
	printf '>>> test-kji0-mutant: green (GUARD_INVERT mutant correctly RED -- the probe sense bites)\n'; \
fi
@if $(TEST_FILEIO_MUT_KJI0_WRONGCONST) >/dev/null 2>&1; then \
	printf '!!! test-kji0-mutant FAIL: WRONG_CONST mutant PASSED -- the 0x0050 constant is decoration\n'; exit 1; \
else \
	printf '>>> test-kji0-mutant: green (WRONG_CONST mutant correctly RED -- the 0x0050 code bites)\n'; \
fi
@if $(TEST_FILEIO_MUT_KJI0_NODISPATCH) >/dev/null 2>&1; then \
	printf '!!! test-kji0-mutant FAIL: NO_DISPATCH mutant PASSED -- the 0x5B dispatch wiring is decoration\n'; exit 1; \
else \
	printf '>>> test-kji0-mutant: green (NO_DISPATCH mutant correctly RED -- the 0x5B case bites)\n'; \
fi
```

## test-label-vol

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 60):
Prerequisites: `build/dosutil_host build/dosutil_vol build/.makefile.stamp`
```make
@$(BUILD)/dosutil_vol
@sh harness/diff/fat_diff/dosutil_diff.sh label $(DOSUTIL_HOST) label
```

## test-label-vol-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 72):
Prerequisites: `build/dosutil_host_ROOT build/dosutil_host_BPB build/dosutil_host_CONFIRM build/dosutil_host_TRUNCATE build/dosutil_host_PROMPT build/dosutil_host_INVALID build/dosutil_host_ROOM build/dosutil_vol_mut build/.makefile.stamp`
```make
@set -e; for m in $(DOSUTIL_LABEL_MUTANTS); do \
  log=$(BUILD)/dosutil_mut_$$m.log; \
  if sh harness/diff/fat_diff/dosutil_diff.sh label $(BUILD)/dosutil_host_$$m mut_$$m >$$log 2>&1; then echo "FAIL: $$m survived"; exit 1; fi; \
  case $$m in ROOT) rule=L1;; BPB) rule=L2;; CONFIRM) rule=L3;; TRUNCATE) rule=L4;; PROMPT) rule=L5;; INVALID) rule=L6;; ROOM) rule=L7;; esac; \
  grep "^FAIL: $$rule " $$log || { cat $$log; exit 1; }; \
  printf 'PASS: DU_MUTATE_%s caught RED\n' "$$m"; \
done
@if $(BUILD)/dosutil_vol_mut >$(BUILD)/dosutil_mut_VOL.log; then echo 'FAIL: VOL survived'; exit 1; fi
@grep '^FAIL: V1' $(BUILD)/dosutil_mut_VOL.log
@echo 'PASS: DU_MUTATE_VOL caught RED'
```

## test-loader

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8488):
Prerequisites: `build/test_loader build/.makefile.stamp`
```make
@printf ">>> test-loader: loader_prepare layout (psp/image+0x100/entry/stack) + params + fail-loud validation\n"
@$(TEST_LOADER)
@printf ">>> test-loader: green\n"
```

## test-loader-big

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28712):
Prerequisites: `build/test_loader_big build/.makefile.stamp`
```make
@printf '>>> test-loader-big: FAT in-place loader -- >64 KiB .COM loads (PROGRAM_IMAGE_MAX sole bound; za4m)\n'
@$(TEST_LOADER_BIG)
```

## test-loader-big-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28715):
Prerequisites: `build/test_loader_big_mutant_stagingcap build/.makefile.stamp`
```make
@if $(TEST_LOADER_BIG_MUT) >/dev/null 2>&1; then printf '!!! test-loader-big-mutant FAIL: cap mutant PASSED -- decoration\n'; exit 1; \
else printf '>>> test-loader-big-mutant: green (re-imposed-cap mutant correctly RED)\n'; fi
```

## test-loader-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8495):
Prerequisites: `build/test_loader_mutant_nooffset build/test_loader_mutant_forceempty build/test_loader_mutant_noguard build/test_loader_mutant_noovlguard build/.makefile.stamp`
```make
@$(TEST_LOADER_MUT_OVERLAY) > $(BUILD)/test_loader_mut_ovl.log 2>&1 && { printf '!!! test-loader-mutant FAIL: no-overlay-guard mutant PASSED -- the FLAIROVL refusal is decoration\n'; exit 1; } || true
@grep -q 'FAIL.*exactly the 8-byte FLAIROVL magic' $(BUILD)/test_loader_mut_ovl.log || { cat $(BUILD)/test_loader_mut_ovl.log; printf '!!! test-loader-mutant FAIL: no-overlay-guard RED for the wrong reason\n'; exit 1; }
@printf '>>> test-loader-mutant: green (no-overlay-guard mutant correctly RED -- FLAIROVL refusal bites; ADR-0014 DEC-14-2)\n'
@printf ">>> test-loader-mutant: confirming the no-0x100-offset + force-empty-env + no-transient-guard mutants go RED (Rule 6)\n"
@if $(TEST_LOADER_MUT_TRANSIENT) >/dev/null 2>&1; then \
	printf '!!! test-loader-mutant FAIL: no-transient-guard mutant PASSED -- the COMMAND3 refusal cases are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-loader-mutant: green (no-transient-guard mutant correctly RED -- both entries refusal bites)\n'; \
fi
@if $(TEST_LOADER_MUT) >/dev/null 2>&1; then \
	printf '!!! test-loader-mutant FAIL: no-offset mutant PASSED the oracle -- the layout test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-loader-mutant: green (no-offset mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_LOADER_MUT_ENV) >/dev/null 2>&1; then \
	printf '!!! test-loader-mutant FAIL: force-empty-env mutant PASSED -- the EXEC env-inheritance test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-loader-mutant: green (force-empty-env mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-loader-nested-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 35527):
Prerequisites: `build/test_loader_nested_mut test-loader build/.makefile.stamp`
```make
@set +e; $< > $(BUILD)/loader_nested_mut.log 2>&1; rc=$$?; set -e; test $$rc -eq 1 && grep -q 'FAIL.*nested EXEC restores caller PSP' $(BUILD)/loader_nested_mut.log || { cat $(BUILD)/loader_nested_mut.log; exit 1; }
@printf '>>> test-loader-nested-mutant: green (LOADER_MUT_NO_RESTORE RED: nested EXEC restores caller PSP)\n'
```

## test-m0bp

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4489):
Prerequisites: `build/test_fat12_mkdir_nested build/fat12_m0bp_golden.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mmd     >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mmd` not found.\n'; exit 1; }
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: mtools `mdir` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-m0bp FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-m0bp: NESTED MKDIR/RMDIR (non-root parent) vs mmd golden + grow cross-check (beads initech-m0bp)\n"
@dd if=/dev/zero of=$(FAT12_M0BP_BLANK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_M0BP_BLANK_IMG) -f 1440 ::
@mmd -i $(FAT12_M0BP_BLANK_IMG) ::SUB
@$(TEST_M0BP) "$(FAT12_M0BP_BLANK_IMG)" "$(FAT12_M0BP_GOLDEN_IMG)"
@mdir -i $(FAT12_M0BP_BLANK_IMG) ::SUB 2>/dev/null | grep -qi 'GROWDIR' \
	|| { printf '!!! test-m0bp FAIL [GROW]: mdir ::SUB does not list GROWDIR <DIR> -- the grown boundary dir is unreachable to mtools\n'; exit 1; }
@# The python reference lists only REGULAR files, so resolve the nested PATH
@# 'SUB\GROWDIR' instead: descending SUB's GROWN chain to find GROWDIR (a dir
@# living in the appended 2nd cluster, slot 16) and listing it succeeds (rc 0)
@# ONLY if GROWDIR is reachable; a missing dir resolves rc != 0 (the check has
@# teeth -- proven against SUB\NOSUCHDIR). Independent of mtools (Rule 5/Law 2).
@python3 $(FAT12_REF_PY) $(FAT12_M0BP_BLANK_IMG) --list-path 'SUB\GROWDIR' >/dev/null 2>&1 \
	|| { printf '!!! test-m0bp FAIL [GROW]: python cannot resolve SUB\\GROWDIR -- the grown boundary dir is unreachable to the independent reader\n'; exit 1; }
@printf '>>> test-m0bp [GROW]: GROWDIR in the appended 2nd cluster of \\SUB; mtools (mdir <DIR>) + python (path-resolve) agree\n'
@printf ">>> test-m0bp: green\n"
```

## test-m0bp-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4516):
Prerequisites: `build/test_fat12_mkdir_nested_mut_norootmk build/test_fat12_mkdir_nested_mut_rootscan build/test_fat12_mkdir_nested_mut_rootslot build/test_fat12_mkdir_nested_mut_nogrow build/test_fat12_mkdir_nested_mut_norootrd build/fat12_m0bp_golden.img build/.makefile.stamp`
```make
@printf ">>> test-m0bp-mutant: confirming all FIVE nested MKDIR/RMDIR mutants go RED for the RIGHT reason (Rule 6; beads initech-m0bp)\n"
@for m in NOROOTMK:m-noroot-mkdir ROOTSCAN:m-rootscan-mkdir ROOTSLOT:m-rootslot-write NOGROW:m-nogrow-parent NOROOTRD:m-noroot-rmdir; do \
	key=$${m%%:*}; name=$${m##*:}; \
	case $$key in \
		NOROOTMK) bin="$(TEST_M0BP_MUT_NOROOTMK)";; \
		ROOTSCAN) bin="$(TEST_M0BP_MUT_ROOTSCAN)";; \
		ROOTSLOT) bin="$(TEST_M0BP_MUT_ROOTSLOT)";; \
		NOGROW)   bin="$(TEST_M0BP_MUT_NOGROW)";; \
		NOROOTRD) bin="$(TEST_M0BP_MUT_NOROOTRD)";; \
	esac; \
	dd if=/dev/zero of=$(FAT12_M0BP_BLANK_IMG) bs=512 count=2880 status=none; \
	mformat -i $(FAT12_M0BP_BLANK_IMG) -f 1440 ::; \
	mmd -i $(FAT12_M0BP_BLANK_IMG) ::SUB; \
	out=$$("$$bin" "$(FAT12_M0BP_BLANK_IMG)" "$(FAT12_M0BP_GOLDEN_IMG)" 2>&1); rc=$$?; \
	echo "$$out" | grep -q 'checks,' \
		|| { printf '!!! test-m0bp-mutant FAIL: %s produced no TEST_SUMMARY -- harness dead, RED is meaningless\n' "$$name"; exit 1; }; \
	if [ $$rc -eq 0 ]; then \
		printf '!!! test-m0bp-mutant FAIL: %s PASSED -- the nested oracle is decoration\n' "$$name"; exit 1; \
	else \
		printf '>>> test-m0bp-mutant: green (%s correctly RED -- the nested primitive bites)\n' "$$name"; \
	fi; \
done
@printf '>>> test-m0bp-mutant: green (ALL FIVE nested mutants ran + RED for the right reason)\n'
```

## test-m0bp-rollback

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4560):
Prerequisites: `build/test_fat12_mkdir_rollback build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mmd     >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback FAIL: mtools `mmd` not found.\n'; exit 1; }
@printf ">>> test-m0bp-rollback: MKDIR post-grow rollback atomicity (NO_SPACE) -- nothing leaks (beads initech-m0bp rollback fix)\n"
@dd if=/dev/zero of=$(FAT12_M0BP_ROLLBACK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_M0BP_ROLLBACK_IMG) -f 1440 ::
@mmd -i $(FAT12_M0BP_ROLLBACK_IMG) ::SUB
@$(TEST_M0BP_ROLLBACK) "$(FAT12_M0BP_ROLLBACK_IMG)"
@printf ">>> test-m0bp-rollback: green\n"
```

## test-m0bp-rollback-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 4575):
Prerequisites: `build/test_fat12_mkdir_rollback_mut_nospace build/.makefile.stamp`
```make
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback-mutant FAIL: mtools `mformat` not found.\n'; exit 1; }
@command -v mmd     >/dev/null 2>&1 || { printf '!!! test-m0bp-rollback-mutant FAIL: mtools `mmd` not found.\n'; exit 1; }
@printf ">>> test-m0bp-rollback-mutant: confirming m-nospace-noroll goes RED for the RIGHT reason (Rule 6; beads initech-m0bp rollback fix)\n"
@dd if=/dev/zero of=$(FAT12_M0BP_ROLLBACK_IMG) bs=512 count=2880 status=none
@mformat -i $(FAT12_M0BP_ROLLBACK_IMG) -f 1440 ::
@mmd -i $(FAT12_M0BP_ROLLBACK_IMG) ::SUB
@out=$$("$(TEST_M0BP_ROLLBACK_MUT)" "$(FAT12_M0BP_ROLLBACK_IMG)" 2>&1); rc=$$?; \
echo "$$out" | grep -q 'checks,' \
	|| { printf '!!! test-m0bp-rollback-mutant FAIL: m-nospace-noroll produced no TEST_SUMMARY -- harness dead, RED is meaningless\n'; exit 1; }; \
if [ $$rc -eq 0 ]; then \
	printf '!!! test-m0bp-rollback-mutant FAIL: m-nospace-noroll PASSED -- the rollback oracle is decoration\n'; exit 1; \
fi; \
echo "$$out" | grep -qi 'BACK to 1 cluster' \
	|| { printf '!!! test-m0bp-rollback-mutant FAIL: m-nospace-noroll RED but not on the rollback assertion -- wrong reason\n'; exit 1; }
@printf '>>> test-m0bp-rollback-mutant: green (m-nospace-noroll correctly RED -- the grown parent cluster leaks without the rollback)\n'
```

## test-mcb

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8546):
Prerequisites: `build/test_mcb build/.makefile.stamp`
```make
@printf ">>> test-mcb: MCB arena 48h/49h/4Ah -- chain invariants + first-fit + split/coalesce + setblock + data-integrity fuzz\n"
@$(TEST_MCB)
@printf ">>> test-mcb: green\n"
```

## test-mcb-emu

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25898):
Prerequisites: `build/qemu_harness build/memtest_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-mcb-emu : AH=48h/49h/4Ah over the MCB arena\n'
@printf '  Ref: beads initech-509.6; spec/memory_map.h; mcb.c; DOS 3.3 PRM 48h/49h/4Ah.\n'
@printf '  Prove the arena seam end-to-end via the REAL int 0x21 trap path.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (memtest self-test kernel)\n' "$(MEMTEST_IMG)"
@printf 'Expecting : MEM-A=<seg> + MEM-WROTE + MEM-GROW-OK + MEM-FREED + MEM-OK + no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(MEMTEST_IMG)" \
	--name "$(MEMTEST_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(MEMTEST_REPORT)" || true
@cat "$(MEMTEST_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(MEMTEST_REPORT)"; then \
	printf '!!! test-mcb-emu FAIL: TRIPLE FAULT during the arena self-test\n'; exit 1; \
fi
@printf '>>> test-mcb-emu [1/5]: no triple-fault\n'
@if [ ! -s "$(MEMTEST_SERIAL)" ]; then \
	printf '!!! test-mcb-emu FAIL: no serial captured at %s\n' "$(MEMTEST_SERIAL)"; exit 1; \
fi
@if grep -q '^MEM-BAD' "$(MEMTEST_SERIAL)"; then \
	printf '!!! test-mcb-emu FAIL: a MEM-BAD marker appeared (root-cause the arena seam, Rule 3):\n'; \
	grep '^MEM-BAD' "$(MEMTEST_SERIAL)"; exit 1; \
fi
@tr -d '\r' < "$(MEMTEST_SERIAL)" | grep -q '^MEM-A=' \
	|| { printf '!!! test-mcb-emu FAIL: MEM-A=<seg> missing -- AH=48h ALLOC did not return a segment\n'; exit 1; }
@printf '>>> test-mcb-emu [2/5]: AH=48h ALLOC returned a DOS segment\n'
@tr -d '\r' < "$(MEMTEST_SERIAL)" | grep -q '^MEM-WROTE$$' \
	|| { printf '!!! test-mcb-emu FAIL: MEM-WROTE missing -- the allocated segment was not usable memory\n'; exit 1; }
@printf '>>> test-mcb-emu [3/5]: the allocated block is real memory (sentinel read back)\n'
@tr -d '\r' < "$(MEMTEST_SERIAL)" | grep -q '^MEM-GROW-OK$$' \
	|| { printf '!!! test-mcb-emu FAIL: MEM-GROW-OK missing -- AH=4Ah SETBLOCK grow failed\n'; exit 1; }
@tr -d '\r' < "$(MEMTEST_SERIAL)" | grep -q '^MEM-FREED$$' \
	|| { printf '!!! test-mcb-emu FAIL: MEM-FREED missing -- AH=49h FREE failed\n'; exit 1; }
@printf '>>> test-mcb-emu [4/5]: AH=4Ah SETBLOCK grew + AH=49h FREE succeeded\n'
@tr -d '\r' < "$(MEMTEST_SERIAL)" | grep -q '^MEM-OK$$' \
	|| { printf '!!! test-mcb-emu FAIL: MEM-OK missing -- free+re-alloc did NOT return the same segment (the seam mis-threads the conversion)\n'; exit 1; }
@printf '>>> test-mcb-emu [5/5]: free + re-ALLOC of the same size returned the SAME segment\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS AH=48h/49h/4Ah operate on a real MCB arena via int 0x21\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-mcb-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25942):
Prerequisites: `build/qemu_harness build/memtest_mut_boot.img build/.makefile.stamp`
```make
@printf '>>> test-mcb-emu-mutant: confirming the no-segbase kernel goes RED (Rule 6)\n'
@$(HARNESS_BIN) --disk "$(MEMTEST_MUT_IMG)" \
	--name "$(MEMTEST_MUT_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(MEMTEST_MUT_REPORT)" || true
@if tr -d '\r' < "$(MEMTEST_MUT_SERIAL)" 2>/dev/null | grep -q '^MEM-OK$$'; then \
	printf '!!! test-mcb-emu-mutant FAIL: MEM-OK present under the no-segbase mutant -- the emu gate is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mcb-emu-mutant: green (no-segbase kernel correctly did NOT reach MEM-OK -- the gate bites)\n'; \
fi
```

## test-mcb-int21

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8591):
Prerequisites: `build/test_mcb_int21 build/.makefile.stamp`
```make
@printf ">>> test-mcb-int21: AH=48h/49h/4Ah dispatch -- seam binding + segment conversion + owner == PSP + shrink-then-alloc + round-trip\n"
@$(TEST_MCBI21)
@printf ">>> test-mcb-int21: green\n"
```

## test-mcb-int21-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8596):
Prerequisites: `build/test_mcb_int21_mutant_segbase build/.makefile.stamp`
```make
@printf ">>> test-mcb-int21-mutant: confirming the no-segbase mutant goes RED (Rule 6)\n"
@if $(TEST_MCBI21_MUT_SEGBASE) >/dev/null 2>&1; then \
	printf '!!! test-mcb-int21-mutant FAIL: no-segbase mutant PASSED -- the segment-conversion oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mcb-int21-mutant: green (no-segbase mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-mcb-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8551):
Prerequisites: `build/test_mcb_mutant_coalesce build/test_mcb_mutant_owner build/.makefile.stamp`
```make
@printf ">>> test-mcb-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_MCB_MUT_COALESCE) >/dev/null 2>&1; then \
	printf '!!! test-mcb-mutant FAIL: no-coalesce mutant PASSED -- the coalesce oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mcb-mutant: green (no-coalesce mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_MCB_MUT_OWNER) >/dev/null 2>&1; then \
	printf '!!! test-mcb-mutant FAIL: no-owner-check mutant PASSED -- the owner-guard oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mcb-mutant: green (no-owner-check mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-memory-map

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28809):
Prerequisites: `build/test_memory_map build/.makefile.stamp`
```make
@printf '>>> test-memory-map: physical map windows pairwise disjoint (bead initech-xu97)\n'
@$(TEST_MEMMAP)
@printf '>>> test-memory-map: green\n'
```

## test-memory-map-high-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/exec_high.mk', line 151):
Prerequisites: `build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/memory-map-high-mutant
@set -eu; for m in heap guard floor; do \
 dir=$(BUILD)/memory-map-high-mutant/$$m; mkdir -p $$dir; \
 case $$m in heap) sed 's/^#define EXT_APP_BASE .*/#define EXT_APP_BASE 0x00100000u/' spec/memory_map.h >$$dir/memory_map.h;; \
 guard) sed 's/^#define EXT_APP_BASE .*/#define EXT_APP_BASE 0x005E0000u/' spec/memory_map.h >$$dir/memory_map.h;; \
 floor) sed 's/(((EXT_APP_CEIL) - 0x00100000u) \/ 1024u)/5120u/' spec/memory_map.h >$$dir/memory_map.h;; esac; \
 if $(CC) $(CFLAGS) -I$$dir -Ispec -Ios/milton -c harness/proptest/test_memory_map.c -o $$dir/probe.o >$$dir/compile.log 2>&1; then echo "!!! high map mutant $$m survived"; exit 1; fi; \
 grep -Eq 'static assertion failed.*(EXT_APP|kernel guard|global 8 MiB)' $$dir/compile.log; \
 printf '>>> test-memory-map-high-mutant: %s RED\n' $$m; done
@printf '>>> test-memory-map-high-mutant: green (FLAIR overlap, kernel guard overlap, old 5120 KiB floor all RED)\n'
```

## test-memory-map-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28827):
Prerequisites: `build/test_memory_map test-memory-map-high-mutant build/.makefile.stamp`
```make
@$(TEST_MEMMAP) >/dev/null || { printf '!!! test-memory-map-mutant: the real gate is not green -- nothing to mutate\n'; exit 1; }
@printf '>>> test-memory-map-mutant: confirming each mutant goes RED for its named reason (Rule 6)\n'
$(call memmap-header-mutant,stack_in_heap,0x004F0000,0x004FFFFC,memory map windows overlap: FLAIR_HEAP vs KSTACK)
$(call memmap-header-mutant,stack_in_transient,0x005B0000,0x005BFFFC,memory map windows overlap: TRANSIENT vs KSTACK)
$(call memmap-header-mutant,stack_in_overlay,0x005D0000,0x005DFFFC,memory map windows overlap: OVERLAY vs KSTACK)
$(call memmap-header-mutant,stack_eats_guard,0x005E0000,0x005EFFFC,overlay window must keep its 64 KiB guard below the kernel stack)
@mkdir -p $(TEST_MEMMAP_MUT)/kstart_drift
@if $(NASM) -f elf32 -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) -DMK_KERNEL_STACK_BOT=$(KERNEL_STACK_BOT) -DMK_KERNEL_STACK_TOP=0x0009FFFC \
	$(KERNEL_START_ASM) -o $(TEST_MEMMAP_MUT)/kstart_drift/kstart.o > $(TEST_MEMMAP_MUT)/kstart_drift/nasm.log 2>&1; then \
	printf '!!! test-memory-map-mutant FAIL: kstart.asm assembled with a drifted MK_KERNEL_STACK_TOP -- the equate guard is decoration\n'; exit 1; \
fi; \
grep -q 'KERNEL_STACK_TOP MISMATCH' $(TEST_MEMMAP_MUT)/kstart_drift/nasm.log || { cat $(TEST_MEMMAP_MUT)/kstart_drift/nasm.log; printf '!!! test-memory-map-mutant FAIL: kstart_drift RED, but not for the equate mismatch\n'; exit 1; }; \
printf '    kstart_drift: RED -- KERNEL_STACK_TOP MISMATCH\n'
@printf '>>> test-memory-map-mutant: green (5 mutants correctly RED)\n'
```

## test-multiopen

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26142):
Prerequisites: `build/qemu_harness build/multiopen_boot.img build/fat_multiopen.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-multiopen : N concurrent opens + >64 KiB + LSEEK\n'
@printf '  Ref: beads initech-0qh / epic initech-6qy; positioned cluster-chain I/O.\n'
@printf '  Two files open at once, interleaved positioned reads, a >64 KiB round-trip.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(MULTIOPEN_IMG)" "$(FAT_MULTIOPEN_IMG)"
@printf 'Expecting : FILEIO-BIND-OK + MO-A1=Hel / MO-B1=Sec / MO-A2=Hello / MO-BIG=%s + MULTIOPEN-EXIT rc=0 + no triple-fault\n' "$(MO_BIG_SIG)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_MULTIOPEN_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_MULTIOPEN_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_MULTIOPEN_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_MULTIOPEN_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_MULTIOPEN_IMG))),:)
@$(HARNESS_BIN) --disk "$(MULTIOPEN_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(MULTIOPEN_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(MULTIOPEN_REPORT)" || true
@cat "$(MULTIOPEN_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(MULTIOPEN_REPORT)"; then \
	printf '!!! test-multiopen FAIL: TRIPLE FAULT (root-cause the positioned read/LSEEK path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-multiopen [1/5]: no triple-fault\n'
@if [ ! -s "$(MULTIOPEN_SERIAL)" ]; then \
	printf '!!! test-multiopen FAIL: no serial captured at %s\n' "$(MULTIOPEN_SERIAL)"; exit 1; \
fi
@grep -q '^FILEIO-BIND-OK$$' "$(MULTIOPEN_SERIAL)" \
	|| { printf '!!! test-multiopen FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
@if grep -q 'MO-OPENA-FAIL\|MO-OPENB-FAIL\|MO-OPENC-FAIL\|MO-BIG-FAIL' "$(MULTIOPEN_SERIAL)"; then \
	printf '!!! test-multiopen FAIL: a MO-*-FAIL marker -- a concurrent OPEN or the >64 KiB seek failed (root-cause, Rule 3):\n'; \
	grep 'MO-.*-FAIL' "$(MULTIOPEN_SERIAL)"; exit 1; \
fi
@printf '>>> test-multiopen [2/5]: FILEIO-BIND-OK + no MO-*-FAIL (all OPENs succeeded)\n'
@sed -n '/^MULTIOPEN-OUTPUT-BEGIN$$/,/^MULTIOPEN-OUTPUT-END$$/p' "$(MULTIOPEN_SERIAL)" > "$(BUILD)/$(MULTIOPEN_NAME).out"
@grep -qF 'MO-A1=Hel' "$(BUILD)/$(MULTIOPEN_NAME).out" \
	|| { printf '!!! test-multiopen FAIL: MO-A1=Hel missing -- handle A did not read HELLO.TXT at offset 0\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
@grep -qF 'MO-B1=Sec' "$(BUILD)/$(MULTIOPEN_NAME).out" \
	|| { printf '!!! test-multiopen FAIL: MO-B1=Sec missing -- a SECOND file was not open concurrently (the multi-tenant capability)\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
@printf '>>> test-multiopen [3/5]: TWO files open concurrently (MO-A1=Hel + MO-B1=Sec, distinct content)\n'
@grep -qF 'MO-A2=Hello' "$(BUILD)/$(MULTIOPEN_NAME).out" \
	|| { printf '!!! test-multiopen FAIL: MO-A2=Hello missing -- handle A position was corrupted by handle B (cross-talk)\n'; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
@printf '>>> test-multiopen [4/5]: independent per-handle positions (A seeked back to 0 reads "Hello" despite B reads)\n'
@grep -qF 'MO-BIG=$(MO_BIG_SIG)' "$(BUILD)/$(MULTIOPEN_NAME).out" \
	|| { printf '!!! test-multiopen FAIL: MO-BIG=%s missing -- the >64 KiB LSEEK+READ did not return the signature at offset %s (the old 64 KiB buffer limit)\n' "$(MO_BIG_SIG)" "$(MO_BIG_SIG_OFF)"; cat "$(BUILD)/$(MULTIOPEN_NAME).out"; exit 1; }
@grep -q '^MULTIOPEN-EXIT rc=0$$' "$(MULTIOPEN_SERIAL)" \
	|| { printf '!!! test-multiopen FAIL: MULTIOPEN-EXIT rc=0 missing -- the program did not finish cleanly\n'; exit 1; }
@printf '>>> test-multiopen [5/5]: >64 KiB round-trip (MO-BIG=%s read from offset %s) + MULTIOPEN-EXIT rc=0\n' "$(MO_BIG_SIG)" "$(MO_BIG_SIG_OFF)"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechDOS held 2+ files open concurrently with independent\n'
@printf '            positions and round-tripped a >64 KiB file via positioned LSEEK+READ\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-mz

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7500):
Prerequisites: `build/test_mz build/.makefile.stamp`
```make
@printf ">>> test-mz: is_mz + parse (both last-page cases) + foreign-gate + fail-loud + apply_relocs\n"
@$(TEST_MZ)
@printf ">>> test-mz: green\n"
```

## test-mz-extended-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/exec_high.mk', line 120):
Prerequisites: `build/test_mz_ext_length_mutant build/.makefile.stamp`
```make
@if $< >$(BUILD)/mz_ext_length_mutant.log 2>&1; then echo '!!! descriptor length mutant survived'; exit 1; fi
@grep 'FAIL.*DEC-03a: unknown version/length/stack/guard refused' $(BUILD)/mz_ext_length_mutant.log
@printf '>>> test-mz-extended-mutant: green (unchecked descriptor length RED)\n'
```

## test-mz-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7506):
Prerequisites: `build/test_mz_mutant_noadd build/test_mz_mutant_paragraph build/.makefile.stamp`
```make
@printf ">>> test-mz-mutant: confirming both reloc mutants go RED (Rule 6)\n"
@if $(TEST_MZ_MUT_NOADD) >/dev/null 2>&1; then \
	printf '!!! test-mz-mutant FAIL: RELOC_NOADD mutant PASSED -- the reloc oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mz-mutant: green (RELOC_NOADD mutant correctly RED)\n'; \
fi
@if $(TEST_MZ_MUT_PARA) >/dev/null 2>&1; then \
	printf '!!! test-mz-mutant FAIL: RELOC_PARAGRAPH mutant PASSED -- the reloc oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mz-mutant: green (RELOC_PARAGRAPH mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-mzexec

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25693):
Prerequisites: `build/qemu_harness build/mzexec_boot.img build/fat_mzexec.img build/mzforeign_boot.img build/fat_mzforeign.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-mzexec : RUN A REAL InitechMZ .EXE\n'
@printf '  Ref: ADR-0003 DEC-08a; beads initech-dtw/0kiq. The flat-32 relocation\n'
@printf '  must resolve at RUNTIME or the marker never prints.\n'
@printf '======================================================================\n'
@# These self-test kernels do NOT halt after their leg (they continue to
@# IRQ-LIVE/CONIN-LIVE), so the harness times out and its EXIT CODE is non-zero
@# even on success -- exactly why test-exec uses `|| true` + a marker check. We
@# judge on marker_found (set by --expect's LIVE serial read; race-free), NOT
@# the exit code.
@# [1/2] RELOC: a real InitechMZ runs + the flat relocation resolves at runtime.
@$(call fixture-preflight,$(FAT_MZEXEC_IMG))
@$(HARNESS_BIN) --disk "$(MZEXEC_IMG)" --disk2 "$(FAT_MZEXEC_IMG)" \
	--expect "MZEXEC-OK" --name "$(MZEXEC_RUN_NAME)" --out "$(BUILD)" --timeout-ms 30000 \
	2> "$(BUILD)/$(MZEXEC_RUN_NAME).report" || true
@if grep -q 'triple_fault=1' "$(BUILD)/$(MZEXEC_RUN_NAME).report"; then \
	printf '!!! test-mzexec FAIL: TRIPLE FAULT booting/running the InitechMZ\n'; exit 1; fi
@grep -q 'marker_found=1' "$(BUILD)/$(MZEXEC_RUN_NAME).report" \
	&& printf '>>> test-mzexec [1/2]: MZEXEC-OK -- the relocated .EXE ran (reloc resolved at runtime)\n' \
	|| { printf '!!! test-mzexec FAIL: MZEXEC-OK never appeared -- the .EXE did not run OR the reloc did not resolve\n'; cat "$(BUILD)/$(MZEXEC_RUN_NAME).report"; exit 1; }
@# [2/2] FOREIGN: an untagged 16-bit MZ panics fail-loud (DEC-08a.5), never runs.
@$(call fixture-preflight,$(FAT_MZFOREIGN_IMG))
@$(HARNESS_BIN) --disk "$(MZFOREIGN_IMG)" --disk2 "$(FAT_MZFOREIGN_IMG)" \
	--expect "PANIC foreign-mz" --name "$(MZFOREIGN_RUN_NAME)" --out "$(BUILD)" --timeout-ms 30000 \
	2> "$(BUILD)/$(MZFOREIGN_RUN_NAME).report" || true
@grep -q 'marker_found=1' "$(BUILD)/$(MZFOREIGN_RUN_NAME).report" \
	&& printf '>>> test-mzexec [2/2]: untagged 16-bit MZ panicked fail-loud (no run) -- DEC-08a.5\n' \
	|| { printf '!!! test-mzexec FAIL: foreign untagged MZ did NOT panic -- the DEC-08a.5 fail-loud gate is broken\n'; cat "$(BUILD)/$(MZFOREIGN_RUN_NAME).report"; exit 1; }
@printf 'VERDICT   : PASS -- InitechOS loaded + RAN a real InitechMZ .EXE (reloc resolved at runtime);\n'
@printf '            an untagged 16-bit MZ was refused with a fail-loud panic. (QEMU; Bochs -> x0i.)\n'
@printf '======================================================================\n'
```

## test-mzexec-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25726):
Prerequisites: `build/qemu_harness build/mzexec_boot.img build/fat_mzexec_noreloc.img build/.makefile.stamp`
```make
@printf '>>> test-mzexec-mutant: the NO-RELOC InitechMZ must NOT print MZEXEC-OK (Rule 6)\n'
@$(call fixture-preflight,$(FAT_MZEXEC_NORELOC_IMG))
@$(HARNESS_BIN) --disk "$(MZEXEC_IMG)" --disk2 "$(FAT_MZEXEC_NORELOC_IMG)" \
	--expect "MZEXEC-OK" --name "mzexec_noreloc_run" --out "$(BUILD)" --timeout-ms 20000 \
	2> "$(BUILD)/mzexec_noreloc_run.report" || true
@if grep -q 'marker_found=1' "$(BUILD)/mzexec_noreloc_run.report"; then \
	printf '!!! test-mzexec-mutant FAIL: MZEXEC-OK printed WITHOUT the relocation -- the reloc gate is decoration\n'; exit 1; \
else \
	printf '>>> test-mzexec-mutant: green (no-reloc image correctly did NOT print MZEXEC-OK -- the reloc is load-bearing)\n'; \
fi
```

## test-mzload

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7548):
Prerequisites: `build/test_mzload build/.makefile.stamp`
```make
@printf ">>> test-mzload: parse+dispatch + flat-reloc (trailer + header-resident) + place@PROGRAM_IMAGE + entry + arena/minalloc + foreign-fail-loud + .COM-untouched\n"
@$(TEST_MZLOAD)
@printf ">>> test-mzload: green\n"
```

## test-mzload-entry-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 233):
Prerequisites: `build/qemu_harness build/mzexec_boot.img build/fat_mzexec.img build/test_mzload build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/mz_entry_emu.sh $(BUILD) $(MZEXEC_IMG) $(FAT_MZEXEC_IMG) green
```

## test-mzload-entry-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 241):
Prerequisites: `build/bochs_harness build/mzexec_boot.img build/fat_mzexec.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-mzload-entry-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/mzexec_boot.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-mzload-entry-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/fat_mzexec.img $(BUILD)/mzload-entry-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/mzexec_boot.img" --disk2 "$(BUILD)/mzload-entry-emu_bochs_data.img"  --expect MZEXEC-END --quit-after MZEXEC-END --name "mzload-entry-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/mzload-entry-emu_bochs.report.txt" || true
@cat "$(BUILD)/mzload-entry-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-mzload-entry-emu-bochs "$(BUILD)/mzload-entry-emu_bochs.report.txt" "$(BUILD)/mzload-entry-emu_bochs.serial" vbe101 IRQ-LIVE CONIN-LIVE MZEXEC-END !PANIC
```

## test-mzload-entry-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 246):
Prerequisites: `build/qemu_harness build/mz_entry_equal.img build/fat_mzexec.img build/test_mzload build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/mz_entry_emu.sh $(BUILD) $(BUILD)/mz_entry_equal.img $(FAT_MZEXEC_IMG) mutant > $(BUILD)/khygiene-evidence/mutant-mz-entry-emu.log 2>&1; then echo 'FAIL MZ equality emu mutant survived'; exit 1; fi
@grep 'RED MZ entry at end:' $(BUILD)/khygiene-evidence/mutant-mz-entry-emu.log
@echo 'GREEN test-mzload-entry-emu-mutant: LOADER_MUTATE_MZ_ENTRY_EQUAL correctly RED'
```

## test-mzload-entry-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 77):
Prerequisites: `build/test_mzload_entry_equal build/.makefile.stamp | build/khygiene-evidence`
```make
@if $< > $(BUILD)/khygiene-evidence/mutant-mz-entry.log 2>&1; then echo 'FAIL MZ equality mutant survived'; exit 1; fi
@grep 'FAIL.*MZ entry boundary' $(BUILD)/khygiene-evidence/mutant-mz-entry.log
@echo 'GREEN test-mzload-entry-mutant: LOADER_MUTATE_MZ_ENTRY_EQUAL correctly RED'
```

## test-mzload-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7553):
Prerequisites: `build/test_mzload_mutant_noreloc build/.makefile.stamp`
```make
@printf ">>> test-mzload-mutant: confirming the no-reloc mutant goes RED (Rule 6)\n"
@if $(TEST_MZLOAD_MUT) >/dev/null 2>&1; then \
	printf '!!! test-mzload-mutant FAIL: NO_RELOC mutant PASSED -- the reloc oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mzload-mutant: green (NO_RELOC mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-mzxa-integration

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6277):
Prerequisites: `build/test_fileio_subdir build/fat12_nested.img build/.makefile.stamp`
```make
@printf ">>> test-mzxa-integration: INT 21h OPEN/READ of '\\SUB\\NESTED.TXT' + '\\SUB\\DEEP\\DEEP.TXT' through the REAL fat12 backend (beads initech-mzxa)\n"
@$(TEST_FILEIO_SUBDIR) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)"
@printf ">>> test-mzxa-integration: green\n"
```

## test-mzxa-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6235):
Prerequisites: `build/test_fileio_mutant_nodrive build/test_fileio_mutant_notroot build/.makefile.stamp`
```make
@printf ">>> test-mzxa-mutant: confirming both resolve mutants go RED (Rule 6; beads initech-mzxa)\n"
@if $(TEST_FILEIO_MUT_NODRIVE) >/dev/null 2>&1; then \
	printf '!!! test-mzxa-mutant FAIL: RESOLVE-NODRIVE mutant PASSED -- the drive-strip test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mzxa-mutant: green (RESOLVE-NODRIVE mutant correctly RED)\n'; \
fi
@if $(TEST_FILEIO_MUT_NOTROOT) >/dev/null 2>&1; then \
	printf '!!! test-mzxa-mutant FAIL: RESOLVE-NOTROOT mutant PASSED -- the subdir-resolution test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-mzxa-mutant: green (RESOLVE-NOTROOT mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-native

Declared tier: HOST; #  recipe to execute (from 'harness/emu/exec_high.mk', line 45):
Prerequisites: `build/test_native build/.makefile.stamp`
```make
@$<
```

## test-native-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/exec_high.mk', line 97):
Prerequisites: `test-native build/test_native_count16 build/test_native_owner build/.makefile.stamp`
```make
@set -eu; for m in count16 owner; do \
 if $(BUILD)/test_native_$$m >$(BUILD)/native_mut_$$m.log 2>&1; then echo '!!! native mutant survived'; exit 1; fi; \
 grep '^native RED' $(BUILD)/native_mut_$$m.log; done
@printf '>>> test-native-mutant: green (truncated count and foreign owner RED)\n'
```

## test-nmpo

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6431):
Prerequisites: `build/test_fileio_subdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: mtools `mdir` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-nmpo FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-nmpo: AH=5Bh CREATNEW end-to-end (FRESH materialize + COLLISION reject + no-truncate) vs mtools + python3 (beads initech-nmpo)\n"
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@# CREATNEW '\SUB\NEW5B.TXT' (700B, 2 clusters); re-CREATNEW collides 0x0050; no truncation.
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write creatnew
@mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/NEW5B.TXT $(BUILD)/nmpo_mcopy.bin
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\NEW5B.TXT' > $(BUILD)/nmpo_py.bin
@python3 -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); m=open("$(BUILD)/nmpo_mcopy.bin","rb").read(); p=open("$(BUILD)/nmpo_py.bin","rb").read(); sys.exit(0 if (m==exp and p==exp and m==p) else 1)' \
	|| { printf '!!! test-nmpo FAIL: CREATNEW-materialized NEW5B.TXT read-back != written (mcopy/python disagree -- a phantom or truncated create)\n'; exit 1; }
@mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'NEW5B *TXT' \
	|| { printf '!!! test-nmpo FAIL: mdir ::SUB does not list NEW5B.TXT (CREATNEW did not materialize)\n'; exit 1; }
@printf '>>> test-nmpo: green (NEW5B.TXT materialized in \\SUB, 700B byte-exact; re-CREATNEW collided 0x0050; no truncation -- mcopy==python==written)\n'
```

## test-nmpo-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6461):
Prerequisites: `build/test_fileio_subdir_nmpo_mut_noguard build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-nmpo-mutant FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-nmpo-mutant FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-nmpo-mutant: confirming the test-nmpo CREATNEW materialize differential bites a NO_GUARD truncate mutant (Rule 6; beads initech-nmpo)\n"
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@# Run the SAME harness leg the mutant perturbs. The NO_GUARD mutant's second
@# CREATNEW truncates NEW5B.TXT on disk; capture the report (it will fail its
@# own in-process collision/no-truncate CHECKs -- that is part of the bite).
@$(TEST_NMPO_MUT_NOGUARD) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write creatnew > $(BUILD)/nmpo_mut.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/nmpo_mut.report \
	|| { printf '!!! test-nmpo-mutant FAIL: NO_GUARD mutant never reached the harness summary -- crashed, RED meaningless\n'; cat $(BUILD)/nmpo_mut.report; exit 1; }
@# The LOAD-BEARING assertion: run the SAME external mtools + python read-back
@# differential test-nmpo runs and assert it DIVERGES from the 700-byte payload
@# (the second CREATNEW truncated the on-disk file). If it still matches, the
@# differential is decoration.
@mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/NEW5B.TXT $(BUILD)/nmpo_mut_mcopy.bin 2>/dev/null || : > $(BUILD)/nmpo_mut_mcopy.bin
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\NEW5B.TXT' > $(BUILD)/nmpo_mut_py.bin 2>/dev/null || : > $(BUILD)/nmpo_mut_py.bin
@python3 -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); m=open("$(BUILD)/nmpo_mut_mcopy.bin","rb").read(); p=open("$(BUILD)/nmpo_mut_py.bin","rb").read(); sys.exit(0 if (m==exp and p==exp and m==p) else 1)' \
	&& { printf '!!! test-nmpo-mutant FAIL: NO_GUARD mutant PASSED -- the materialized bytes still match the payload; the test-nmpo differential is decoration\n'; exit 1; } || true
@printf '>>> test-nmpo-mutant: green (NO_GUARD ran + the on-disk NEW5B.TXT diverged from the 700B payload -- the mtools/python materialize differential bites)\n'
```

## test-overlay

Declared tier: HOST; #  recipe to execute (from 'harness/exec_overlay.mk', line 82):
Prerequisites: `build/test_overlay test-overlay-dos33-reference build/.makefile.stamp`
```make
@printf '>>> test-overlay: AH=4Bh AL=03h core (factor reloc, footprint, raw, refusals) + dispatch + DEC-19.4 ownership\n'
@$(TEST_OVERLAY)
@printf '>>> test-overlay: green\n'
```

## test-overlay-dos33-reference

Declared tier: outside active vector; #  recipe to execute (from 'harness/exec_overlay.mk', line 48):
Prerequisites: `build/overlay_dos33_check build/overlay_dos33_check_mut build/.makefile.stamp`
```make
@$(BUILD)/overlay_dos33_check $(OVL_DOS33_REF)/factor-different.bin $(OVL_DOS33_REF)/factor-equal.bin
@if $(BUILD)/overlay_dos33_check_mut $(OVL_DOS33_REF)/factor-different.bin $(OVL_DOS33_REF)/factor-equal.bin > $(BUILD)/overlay_dos33_check_mut.log 2>&1; then \
	printf '!!! DOS33 load-based-address mutant survived\n'; exit 1; fi
@grep -qx 'FAIL DOS33 different-factor fixup address' $(BUILD)/overlay_dos33_check_mut.log
@printf '>>> test-overlay-dos33-reference: green; load-based-address mutant RED\n'
```

## test-overlay-memory

Declared tier: outside active vector; #  recipe to execute (from 'harness/exec_overlay.mk', line 249):
Prerequisites: `build/kernel_flairtenants.elf build/.makefile.stamp`
```make
@nm -S --defined-only $< > $(BUILD)/overlay_memory.symbols
@total=0; count=0; while read address bytes kind name; do \
	case "$$name" in g_load_sector|g_load_cluster|g_ovl_stage) \
		total=$$((total + 0x$$bytes)); count=$$((count + 1));; esac; \
	done < $(BUILD)/overlay_memory.symbols; \
	printf '>>> overlay fixed byte buffers: %s B (limit 1024 B)\n' "$$total"; \
	test "$$count" -ge 2 && test "$$total" -le 1024
```

## test-overlay-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/exec_overlay.mk', line 87):
Prerequisites: `test-overlay build/test_overlay_mutant_nofactor build/test_overlay_mutant_obo build/test_overlay_mutant_foreign build/test_overlay_mutant_preflight build/test_overlay_mutant_chain build/.makefile.stamp`
```make
$(call ovl-host-red,nofactor,A1 reloc @4: 0x10 + factor 0x123400 = 0x123410,factor-not-applied)
$(call ovl-host-red,obo,C5 one byte past the block (the next MCB header) refused,bounds-off-by-one)
$(call ovl-host-red,foreign,A5 untagged MZ -> LOADER_ERR_FOREIGN_MZ,untagged-MZ-accepted)
$(call ovl-host-red,preflight,A12 failed validation leaves the WHOLE destination untouched,copy-before-validation)
$(call ovl-host-red,chain,A13 short FAT chain leaves the WHOLE destination untouched,missing-chain-preflight)
@printf '>>> test-overlay-mutant: green (all five mutants RED for their named reasons)\n'
```

## test-panic

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27764):
Prerequisites: `build/qemu_harness build/panic_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-panic : CPU exception caught (no triple-fault)\n'
@printf '  Ref: ground-truth Sec 3.3 (fail-loud panic) / Sec 8 Risk 1 (the\n'
@printf '       minefield: a triple-fault silently reboots). beads initech-a5a.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (self-test #DE fault kernel)\n' "$(PANIC_IMG)"
@printf 'Expecting : serial SELFTEST-FAULT-ARMED + "PANIC vec=00" + triple_fault=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(PANIC_IMG)" \
	--name "$(PANIC_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(PANIC_REPORT)" || true
@cat "$(PANIC_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# 1. We actually reached the deliberate fault.
@if [ ! -s "$(PANIC_SERIAL)" ]; then \
	printf '!!! test-panic FAIL: no serial captured at %s\n' "$(PANIC_SERIAL)"; \
	exit 1; \
fi
@grep -q '^SELFTEST-FAULT-ARMED$$' "$(PANIC_SERIAL)" \
	|| { printf '!!! test-panic FAIL: never reached the deliberate fault (SELFTEST-FAULT-ARMED missing)\n'; exit 1; }
@printf '>>> test-panic [1/3]: reached the deliberate #DE\n'
@# 2. The panic handler ran and dumped the grep-able marker for vector 0 (#DE).
@grep -q 'PANIC vec=00' "$(PANIC_SERIAL)" \
	|| { printf '!!! test-panic FAIL: no "PANIC vec=00" -- the #DE was not caught by the panic handler\n'; exit 1; }
@printf '>>> test-panic [2/3]: #DE caught -- "PANIC vec=00" on serial\n'
@# 3. No triple-fault: the panic halted cleanly (no silent reboot).
@if grep -q 'triple_fault=1' "$(PANIC_REPORT)"; then \
	printf '!!! test-panic FAIL: TRIPLE FAULT -- the exception cascaded to a silent reboot\n'; \
	exit 1; \
fi
@printf '>>> test-panic [3/3]: no triple-fault -- panic halted cleanly\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- deliberate #DE caught by the fail-loud panic, no triple-fault\n'
@printf '======================================================================\n'
```

## test-panic-foreign-screen

Declared tier: outside active vector; #  recipe to execute (from 'harness/emu/panic_reasons.mk', line 19):
Prerequisites: `test-panic-screen build/qemu_harness build/tracer_boot.img build/ppm_panic_check build/.makefile.stamp`
```make
@sh harness/emu/panic_reasons.sh $(BUILD) $(TRACER_IMG) - foreign foreign
```

## test-panic-layout

Declared tier: HOST; #  recipe to execute (from 'harness/emu/panic_screen.mk', line 51):
Prerequisites: `build/test_panic_layout build/.makefile.stamp`
```make
@$<
```

## test-panic-layout-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/panic_screen.mk', line 53):
Prerequisites: `build/test_panic_layout_mut build/.makefile.stamp`
```make
@if $< > $(BUILD)/panic_layout_mut.red 2>&1; then echo 'FAIL PANIC_MUTATE_OFF_CENTRE survived'; exit 1; fi
@grep '^RED panic-layout: off-centre ' $(BUILD)/panic_layout_mut.red
@echo 'GREEN test-panic-layout-mutant: PANIC_MUTATE_OFF_CENTRE rejected'
```

## test-panic-reason-map

Declared tier: HOST; #  recipe to execute (from 'harness/emu/panic_reasons.mk', line 12):
Prerequisites: `build/test_panic_reasons build/.makefile.stamp`
```make
@$<
```

## test-panic-reason-map-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/emu/panic_reasons.mk', line 14):
Prerequisites: `build/test_panic_reasons_mut build/.makefile.stamp`
```make
@if $< > $(BUILD)/panic_reason_map.red 2>&1; then echo 'FAIL PANIC_MUTATE_STARTUP_OOM survived'; exit 1; fi
@grep '^RED panic-reason-map:' $(BUILD)/panic_reason_map.red
@echo 'GREEN test-panic-reason-map-mutant: all-OOM mapping rejected'
```

## test-panic-reasons

Declared tier: EMU; 
Prerequisites: `test-panic-reason-map test-panic-foreign-screen test-film-startup-screen test-flair-live-scene build/.makefile.stamp`
```make
```

## test-panic-reasons-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/panic_reasons.mk', line 37):
Prerequisites: `test-panic-screen build/panic-screen/skipreason.img build/flair_tenants_mut_film_startup_oom.img build/flair_data.img build/ppm_panic_check build/.makefile.stamp`
```make
@mkdir -p $(BUILD)/panic-reasons
@set -e; if sh harness/emu/panic_reasons.sh $(BUILD) $(PANIC_SCREEN_DIR)/skipreason.img - skipreason foreign > $(BUILD)/panic-reasons/skipreason.red 2>&1; then echo 'FAIL PANIC_MUTATE_SKIP_REASON survived'; exit 1; else rc=$$?; test $$rc -eq 1; fi
@grep '^GREEN panic-reason-serial skipreason:' $(BUILD)/panic-reasons/skipreason.red
@grep '^RED panic-pixels:' $(BUILD)/panic-reasons/skipreason.red
@set -e; for kind in nodata noapps nocalc badcalc; do \
	if sh harness/emu/panic_reasons.sh $(BUILD) $(BUILD)/flair_tenants_mut_film_startup_oom.img $(FLAIR_DATA_IMG) oom_$$kind $$kind > $(BUILD)/panic-reasons/oom_$$kind.red 2>&1; then echo 'FAIL PANIC_MUTATE_STARTUP_OOM survived'; exit 1; else rc=$$?; test $$rc -eq 1; fi; \
	grep "^GREEN panic-reason-serial oom_$$kind:" $(BUILD)/panic-reasons/oom_$$kind.red; \
	grep '^RED panic-pixels:' $(BUILD)/panic-reasons/oom_$$kind.red; \
done
@echo 'GREEN test-panic-reasons-mutant: serial retained; skipped painter and all-OOM mapping rejected by screen pixels'
```

## test-panic-screen

Declared tier: EMU; #  recipe to execute (from 'harness/emu/panic_screen.mk', line 57):
Prerequisites: `build/qemu_harness build/tracer_boot.img build/panic-screen/debug32.img build/panic-screen/debug8.img build/panic-screen/debug13.img build/panic-screen/ud.com build/panic-screen/gp.com build/ppm_panic_check build/.makefile.stamp`
```make
@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/ud.com ud32 640 480 6 1
@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug32.img $(PANIC_SCREEN_DIR)/gp.com gp32 640 480 13 1
@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug8.img $(PANIC_SCREEN_DIR)/ud.com ud8 640 480 6 1
@sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/debug13.img $(PANIC_SCREEN_DIR)/ud.com ud13 320 200 6 1
@sh harness/emu/panic_screen.sh $(BUILD) $(TRACER_IMG) $(PANIC_SCREEN_DIR)/gp.com quiet32 640 480 13 0
@echo 'GREEN test-panic-screen: #UD/#GP, 32bpp/VBE8/VGA13 pixels, debug on/off'
```

## test-panic-screen-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/emu/panic_screen.mk', line 76):
Prerequisites: `build/bochs_harness build/panic-screen/debug32.img build/panic-screen/ud.com build/.makefile.stamp`
```make
@dd if=/dev/zero of=$(PANIC_SCREEN_DIR)/bochs-data.img bs=512 count=2880 status=none
@mformat -i $(PANIC_SCREEN_DIR)/bochs-data.img -f 1440 ::
@mcopy -i $(PANIC_SCREEN_DIR)/bochs-data.img $(PANIC_SCREEN_DIR)/ud.com ::PANIC.COM
@printf '@ECHO OFF\r\nPANIC\r\n' > $(PANIC_SCREEN_DIR)/bochs.bat
@mcopy -i $(PANIC_SCREEN_DIR)/bochs-data.img $(PANIC_SCREEN_DIR)/bochs.bat ::AUTOEXEC.BAT
@$(call fixture-check,recipe,$(PANIC_SCREEN_DIR)/bochs-data.img,PANIC.COM AUTOEXEC.BAT)
@$(BOCHS_BIN) --disk $(PANIC_SCREEN_DIR)/debug32.img --disk2 $(PANIC_SCREEN_DIR)/bochs-data.img --name panic_screen_bochs --out $(PANIC_SCREEN_DIR) --timeout-ms 45000 --expect HALTED --quit-after HALTED 2> $(PANIC_SCREEN_DIR)/bochs.report
@grep -q 'rfb_unblocked=1' $(PANIC_SCREEN_DIR)/bochs.report
@grep -q 'triple_fault=0' $(PANIC_SCREEN_DIR)/bochs.report
@tr -d '\r' < $(PANIC_SCREEN_DIR)/panic_screen_bochs.serial > $(PANIC_SCREEN_DIR)/bochs.norm
@grep -qx 'PANIC-PROBE-ARMED' $(PANIC_SCREEN_DIR)/bochs.norm
@grep -q '^PANIC vec=06 err=00000000$$' $(PANIC_SCREEN_DIR)/bochs.norm
@grep -q '^  eax=12345678 ebx=23456789 ecx=3456789A edx=456789AB$$' $(PANIC_SCREEN_DIR)/bochs.norm
@echo 'GREEN test-panic-screen-bochs: real FAT .COM #UD, debug register dump, no triple fault (serial only)'
```

## test-panic-screen-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/panic_screen.mk', line 64):
Prerequisites: `build/qemu_harness build/panic-screen/no8.img build/panic-screen/ud.com build/ppm_panic_check build/.makefile.stamp`
```make
@if sh harness/emu/panic_screen.sh $(BUILD) $(PANIC_SCREEN_DIR)/no8.img $(PANIC_SCREEN_DIR)/ud.com no8 640 480 6 1 > $(PANIC_SCREEN_DIR)/no8.red 2>&1; then echo 'FAIL PANIC_MUTATE_NO_8BPP survived'; exit 1; fi
@grep '^GREEN panic-serial no8:' $(PANIC_SCREEN_DIR)/no8.red
@grep '^RED panic-pixels:' $(PANIC_SCREEN_DIR)/no8.red
@echo 'GREEN test-panic-screen-mutant: PANIC_MUTATE_NO_8BPP rejected by pixels after real fault'
```

## test-partition-spec

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 60):
Prerequisites: `build/test_partition spec/mbr_partition_contract.json build/.makefile.stamp`
```make
@$(BUILD)/test_partition --spec spec/mbr_partition_contract.json
```

## test-partition-spec-mutant

Declared tier: HOST; #  recipe to execute (from 'harness/diff/fat_diff/partition.mk', line 62):
Prerequisites: `build/test_partition spec/mbr_partition_contract.json build/.makefile.stamp`
```make
@sed 's/"table_offset": 446/"table_offset": 445/' spec/mbr_partition_contract.json > $(BUILD)/partition_spec_mut.json
@if $(BUILD)/test_partition --spec $(BUILD)/partition_spec_mut.json > $(BUILD)/partition-spec-mut.log 2>&1; then echo 'FAIL partition spec mutant survived'; exit 1; fi
@grep '^RED partition SPEC contract drift' $(BUILD)/partition-spec-mut.log
@echo 'GREEN test-partition-spec-mutant: wrong table offset rejected'
```

## test-path-command

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 39):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin build/dos_parity_host build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/dos_parity.sh $(if $(filter test-dos-stamp,$@),stamp,$(if $(filter test-dir-args,$@),dir,path)) $(SECONDARY_TRACER_IMG)
```

## test-path-command-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/dos_parity.mk', line 47):
Prerequisites: `build/qemu_harness build/tracer_mut_safety_pathcmd.img build/greet_program.bin build/.makefile.stamp`
```make
@if sh harness/diff/fat_diff/dos_parity.sh path $(BUILD)/tracer_mut_safety_pathcmd.img mutant > $(BUILD)/path_command_mutant.log 2>&1; then echo 'FAIL: PATH mutant survived'; exit 1; fi
@grep 'path RED: bare PATH / PATH ;' $(BUILD)/path_command_mutant.log
@printf 'PASS PATH mutant: CMD_MUTATE_PATH_COMMAND caught RED\n'
```

## test-program

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25479):
Prerequisites: `build/qemu_harness build/demo_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-program : InitechDOS RUNS A PROGRAM\n'
@printf '  Ref: docs/research/psp-loader-ground-truth.md Sec 4/5. beads initech-509.5.\n'
@printf '  Prove load -> run -> INT 21h -> return-to-loader end-to-end on real boot.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (real boot chain -> C kernel -> load_program)\n' "$(DEMO_IMG)"
@printf 'Expecting : PROGRAM-BEGIN + "%s" + PROGRAM-EXIT rc=0 + no triple-fault\n' "$(PROG_OUTPUT)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(DEMO_IMG)" \
	--name "$(PROG_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(PROG_REPORT)" || true
@cat "$(PROG_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@# ---- 1. Triple-fault check (the control transfer did not silently reboot). ----
@if grep -q 'triple_fault=1' "$(PROG_REPORT)"; then \
	printf '!!! test-program FAIL: TRIPLE FAULT -- the control transfer or return crashed\n'; \
	exit 1; \
fi
@printf '>>> test-program [1/3]: no triple-fault\n'
@# ---- 2. Serial: begin marker + program output + clean exit. ----
@if [ ! -s "$(PROG_SERIAL)" ]; then \
	printf '!!! test-program FAIL: no serial captured at %s\n' "$(PROG_SERIAL)"; \
	exit 1; \
fi
@grep -q '^PROGRAM-BEGIN$$' "$(PROG_SERIAL)" \
	|| { printf '!!! test-program FAIL: PROGRAM-BEGIN marker missing (loader never invoked)\n'; exit 1; }
@grep -qF '$(PROG_OUTPUT)' "$(PROG_SERIAL)" \
	|| { printf '!!! test-program FAIL: program output "%s" missing -- the loaded program did not run/print via INT 21h\n' "$(PROG_OUTPUT)"; exit 1; }
@printf '>>> test-program [2/3]: program ran and printed "%s" through INT 21h\n' "$(PROG_OUTPUT)"
@# ---- 3. Return-to-loader: PROGRAM-EXIT rc=0 came back to the kernel. ----
@grep -q '^PROGRAM-EXIT rc=0$$' "$(PROG_SERIAL)" \
	|| { printf '!!! test-program FAIL: PROGRAM-EXIT rc=0 missing -- the program ran but never returned to the loader (Risk 1)\n'; exit 1; }
@printf '>>> test-program [3/3]: return-to-loader brought control back (PROGRAM-EXIT rc=0)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- InitechOS loaded a flat program, it ran + called the OS, and the loader regained control with rc=0\n'
@printf '======================================================================\n'
```

## test-program-window-golden

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28594):
Prerequisites: `build/.makefile.stamp | build`
```make
@awk '$$1=="#define" && ($$2=="PROGRAM_BASE" || $$2=="PROGRAM_IMAGE" || $$2=="ENV_BLOCK" || \
       $$2=="PROGRAM_ALLOC_END" || $$2=="PROGRAM_STACK_TOP" || \
       $$2=="PROGRAM_STACK_BOT") { print $$2, $$3 }' \
	spec/memory_map.h > $(BUILD)/program-window.got
@printf '%s\n' \
	'PROGRAM_BASE 0x00040000u' \
	'PROGRAM_IMAGE 0x00040100u' \
	'ENV_BLOCK 0x0006F000u' \
	'PROGRAM_ALLOC_END 0x00080000u' \
	'PROGRAM_STACK_TOP 0x0007FFFCu' \
	'PROGRAM_STACK_BOT 0x00070000u' \
	> $(BUILD)/program-window.expected
@diff -u $(BUILD)/program-window.expected $(BUILD)/program-window.got \
	|| { printf '!!! test-program-window-golden FAIL: locked PROGRAM_* literals moved\n'; exit 1; }
@printf '>>> test-program-window-golden: green -- program window literals byte-identical\n'
```

## test-psp

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8116):
Prerequisites: `build/test_psp build/.makefile.stamp`
```make
@printf ">>> test-psp: PSP 256-byte construction (int20/segs/jft/int21-entry/cmd-tail) + clamp + no-overflow\n"
@$(TEST_PSP)
@printf ">>> test-psp: green\n"
```

## test-psp-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8122):
Prerequisites: `build/test_psp_mutant_int20 build/test_psp_mutant_tail build/test_psp_mutant_jft build/.makefile.stamp`
```make
@printf ">>> test-psp-mutant: confirming all mutants go RED (Rule 6)\n"
@if $(TEST_PSP_MUT_INT20) >/dev/null 2>&1; then \
	printf '!!! test-psp-mutant FAIL: int20 mutant PASSED -- the int20 test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-psp-mutant: green (int20 mutant correctly RED)\n'; \
fi
@if $(TEST_PSP_MUT_TAIL) >/dev/null 2>&1; then \
	printf '!!! test-psp-mutant FAIL: cmd-tail-len mutant PASSED -- the tail test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-psp-mutant: green (cmd-tail-len mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_PSP_MUT_JFT) >/dev/null 2>&1; then \
	printf '!!! test-psp-mutant FAIL: no-JFT-inherit mutant PASSED -- the child-inherits-parent-JFT test is decoration (bsy.9)\n'; \
	exit 1; \
else \
	printf '>>> test-psp-mutant: green (no-JFT-inherit mutant correctly RED -- the JFT inheritance oracle bites; bsy.9)\n'; \
fi
```

## test-qekc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6675):
Prerequisites: `build/test_fileio_subdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-qekc FAIL: mtools `mdir` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-qekc FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-qekc: AH=57h SET FILE DATE/TIME persistence vs python --stat-path-time + mtools mdir (beads initech-qekc)\n"
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_QEKC_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_QEKC_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@# (1) CREATE '\SUB\TSTAMP.TXT' (0/0 baseline) + AH=57h SET mtime/mdate + FLUSH.
@$(TEST_FILEIO_SUBDIR) "$(FAT12_QEKC_IMG)" "$(FAT12_FIXTURE_DIR)" --write filetime-set
@# python: the on-disk 0x16/0x18 packed words EXACTLY equal what SET wrote
@# (0x4A6B 0x2C8D == 19051 11405). NO normalization -- the INVERSE oracle.
@test "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" \
	|| { printf '!!! test-qekc FAIL [SET]: python --stat-path-time != the written 19051 11405 (0x4A6B 0x2C8D)\n'; \
	     printf '    got: [%s]\n' "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')"; exit 1; }
@# mtools: the SAME packed words rendered as the date 2002-04-13 + time 9:19
@# (an INDEPENDENT decoder agreeing with python -- Rule 5 two-reference rule).
@mdir -i $(FAT12_QEKC_IMG) ::SUB 2>/dev/null | grep -q 'TSTAMP  *TXT.*2002-04-13  *9:19' \
	|| { printf '!!! test-qekc FAIL [SET]: mtools mdir does not render TSTAMP.TXT as 2002-04-13 9:19 (the written 0x4A6B/0x2C8D)\n'; \
	     mdir -i $(FAT12_QEKC_IMG) ::SUB 2>/dev/null | grep -i tstamp; exit 1; }
@printf '>>> test-qekc [SET]: \\SUB\\TSTAMP.TXT on-disk mtime/mdate == 0x4A6B/0x2C8D (python==mtools==written)\n'
@# (2) CONTENT intact: the time-set flush did NOT clobber the file body.
@python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --cat-path 'SUB\TSTAMP.TXT' > $(BUILD)/qekc_body.bin
@python3 -c 'import sys; e=bytes(97+(i%26) for i in range(64)); sys.exit(0 if open("$(BUILD)/qekc_body.bin","rb").read()==e else 1)' \
	|| { printf '!!! test-qekc FAIL [SET]: the file body changed after the time-set flush (data clobbered)\n'; exit 1; }
@printf '>>> test-qekc [SET]: TSTAMP.TXT body reads back byte-exact (the time-set flush left data untouched)\n'
@# (3) PERSISTENCE: a subsequent WRITE must PRESERVE the FILETIME-set stamp
@# (fat12_write_partial patches only size/start_cluster, NOT mtime/mdate).
@$(TEST_FILEIO_SUBDIR) "$(FAT12_QEKC_IMG)" "$(FAT12_FIXTURE_DIR)" --write filetime-persist >/dev/null 2>&1 || true
@test "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" \
	|| { printf '!!! test-qekc FAIL [PERSIST]: a later WRITE clobbered the FILETIME-set stamp (fat12_write_partial did not preserve mtime/mdate)\n'; exit 1; }
@printf '>>> test-qekc [PERSIST]: a later WRITE preserved the FILETIME-set stamp (write_partial leaves 0x16/0x18 alone)\n'
@printf ">>> test-qekc: green\n"
```

## test-qekc-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6715):
Prerequisites: `build/test_fileio_qekc_mut_getswap build/test_fileio_qekc_mut_noalreject build/test_fileio_qekc_mut_rook build/test_fileio_subdir_qekc_mut_dropmdate build/test_fileio_subdir_qekc_mut_noflush build/test_fileio_subdir_qekc_mut_rootdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf ">>> test-qekc-mutant: confirming all SIX AH=57h mutants go RED for the RIGHT reason (Rule 6; beads initech-qekc)\n"
@# --- MUTANT 2 (GET_SWAP): GET returns mdate in CX / mtime in DX -> the host
@# CX/DX GET contract bites. The host oracle must run + report >=1 failure.
@$(TEST_QEKC_MUT_GETSWAP) > $(BUILD)/qekc_mut_getswap.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/qekc_mut_getswap.report \
	|| { printf '!!! test-qekc-mutant FAIL: GET_SWAP mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/qekc_mut_getswap.report \
	|| { printf '!!! test-qekc-mutant FAIL: GET_SWAP mutant PASSED -- the host CX/DX GET contract is decoration\n'; exit 1; }
@printf '>>> test-qekc-mutant: green (GET_SWAP ran + RED -- CX/DX GET contract bites)\n'
@# --- MUTANT 4 (NO_AL_REJECT): AL=2 no longer rejected (falls to SET, CF=0)
@# -> the bad-AL host contract (CF=1/0x0001) bites.
@$(TEST_QEKC_MUT_NOALREJECT) > $(BUILD)/qekc_mut_noalreject.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/qekc_mut_noalreject.report \
	|| { printf '!!! test-qekc-mutant FAIL: NO_AL_REJECT mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/qekc_mut_noalreject.report \
	|| { printf '!!! test-qekc-mutant FAIL: NO_AL_REJECT mutant PASSED -- the bad-AL contract is decoration\n'; exit 1; }
@printf '>>> test-qekc-mutant: green (NO_AL_REJECT ran + RED -- the AL=2 reject bites)\n'
@# --- MUTANT 5 (RO_OK): a SET with no write backend returns CF=0 instead of
@# 0x0005 -> the read-only-SET host assertion bites.
@$(TEST_QEKC_MUT_ROOK) > $(BUILD)/qekc_mut_rook.report 2>&1 || true
@grep -q '^test_fileio: ' $(BUILD)/qekc_mut_rook.report \
	|| { printf '!!! test-qekc-mutant FAIL: RO_OK mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/qekc_mut_rook.report \
	|| { printf '!!! test-qekc-mutant FAIL: RO_OK mutant PASSED -- the read-only-SET access-denied contract is decoration\n'; exit 1; }
@printf '>>> test-qekc-mutant: green (RO_OK ran + RED -- the read-only-SET 0x0005 contract bites)\n'
@# --- MUTANT 1 (DROP_MDATE): SET writes only mtime, drops mdate -> the on-disk
@# DATE differs from the written 0x2C8D. The mtime word still matches, so we
@# assert the FULL pair != written (the date leg bites).
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_QEKC_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_QEKC_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_QEKC_MUT_DROPMDATE) "$(FAT12_QEKC_IMG)" "$(FAT12_FIXTURE_DIR)" --write filetime-set > $(BUILD)/qekc_mut_dropmdate.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/qekc_mut_dropmdate.report \
	|| { printf '!!! test-qekc-mutant FAIL: DROP_MDATE mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@if [ "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" ]; then \
	printf '!!! test-qekc-mutant FAIL: DROP_MDATE mutant PASSED -- the on-disk mdate matched the written value; the date leg is decoration\n'; exit 1; \
else \
	printf '>>> test-qekc-mutant: green (DROP_MDATE ran + RED -- the mdate=DX write bites)\n'; \
fi
@# --- MUTANT 3 (NO_FLUSH): patch the entry but SKIP the write-back -> the SET
@# reports success + a SAME-handle GET stays GREEN, but the on-disk differential
@# (after re-mount) bites. We assert BOTH: (a) the harness reported 0 failures
@# (the same-handle GET the harness does NOT do, but the SET CHECK passes since
@# the dispatcher returns CF=0), proving the flush-skip is SILENT at the API;
@# and (b) the on-disk stamp did NOT land (still the 0/0 baseline).
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_QEKC_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_QEKC_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_QEKC_MUT_NOFLUSH) "$(FAT12_QEKC_IMG)" "$(FAT12_FIXTURE_DIR)" --write filetime-set > $(BUILD)/qekc_mut_noflush.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/qekc_mut_noflush.report \
	|| { printf '!!! test-qekc-mutant FAIL: NO_FLUSH mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, 0 failures' $(BUILD)/qekc_mut_noflush.report \
	|| { printf '!!! test-qekc-mutant FAIL: NO_FLUSH mutant surfaced an API failure -- the SET should report SUCCESS (CF=0) even with the flush skipped; RED for the wrong reason\n'; exit 1; }
@if [ "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" ]; then \
	printf '!!! test-qekc-mutant FAIL: NO_FLUSH mutant PASSED -- the stamp landed on disk despite the skipped flush; the persistence/flush oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-qekc-mutant: green (NO_FLUSH ran, SET reported success BUT the on-disk stamp is absent -- the flush is load-bearing)\n'; \
fi
@# --- MUTANT 6 (ROOTDIR): a subdir file's stamp is forced to a ROOT slot ->
@# the SUBDIR --stat-path-time differential bites (the subdir entry never gets
@# the stamp). The harness still runs the SET at the API level (CF=0).
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_QEKC_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_QEKC_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_QEKC_MUT_ROOTDIR) "$(FAT12_QEKC_IMG)" "$(FAT12_FIXTURE_DIR)" --write filetime-set > $(BUILD)/qekc_mut_rootdir.report 2>&1 || true
@grep -q '^test_fileio_subdir: ' $(BUILD)/qekc_mut_rootdir.report \
	|| { printf '!!! test-qekc-mutant FAIL: ROOTDIR mutant never reached the summary -- crashed, RED meaningless\n'; exit 1; }
@if [ "$$(python3 $(FAT12_REF_PY) $(FAT12_QEKC_IMG) --stat-path-time 'SUB\TSTAMP.TXT')" = "19051 11405" ]; then \
	printf '!!! test-qekc-mutant FAIL: ROOTDIR mutant PASSED -- the subdir file stamp landed despite dir_start being forced to root; the subdir-path leg is decoration\n'; exit 1; \
else \
	printf '>>> test-qekc-mutant: green (ROOTDIR ran + RED -- the subdir TSTAMP.TXT entry never got the stamp; dir_start threading bites)\n'; \
fi
@printf '>>> test-qekc-mutant: green (ALL SIX AH=57h mutants ran + RED for the right reason)\n'
```

## test-readerr-winh

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27100):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_exec.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-readerr-winh : AH=3Fh READ error != a byte count\n'
@printf '  Ref: os/milton/devices.c dev_prn_handler; spec/dos_messages.json (MSG-DOS-0002). Law 2/Rule 2.\n'
@printf '  beads initech-winh (P1). Inject: type prn / copy prn out.txt / type out.txt / exit.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + disk %s (device OPEN-by-name needs no data file)\n' "$(SECONDARY_TRACER_IMG)" "$(FAT_EXEC_IMG)"
@printf 'Expecting : ONE "Bad command or file name" per PRN read, OUT.TXT left empty, clean SHELL-EXIT\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_EXEC_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(WINH_NAME)" --out "$(BUILD)" --timeout-ms 10000 \
	--keys "$(WINH_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(WINH_REPORT)" || true
@cat "$(WINH_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(WINH_REPORT)"; then \
	printf '!!! test-readerr-winh FAIL: TRIPLE FAULT -- the shell boot or a PRN read crashed\n'; exit 1; \
fi
@printf '>>> test-readerr-winh [1/5]: no triple-fault\n'
@if [ ! -s "$(WINH_SERIAL)" ]; then \
	printf '!!! test-readerr-winh FAIL: no serial captured at %s\n' "$(WINH_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(WINH_SERIAL)" \
	|| { printf '!!! test-readerr-winh FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-readerr-winh [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@sed -n '/^SHELL-READY$$/,$$p' "$(WINH_SERIAL)" | tr -d '\r' > "$(BUILD)/$(WINH_NAME).repl"
@# ---- exactly ONE diagnostic from `type prn` (a repeat/garbage stream would
@# mean the read-error loop ran more than once -- the bug is back). ----
@tcnt=$$(awk '/^A:\\>type prn$$/{f=1;next} /^A:\\>/{f=0} f' "$(BUILD)/$(WINH_NAME).repl" | grep -cF 'Bad command or file name'); \
if [ "$$tcnt" != "1" ]; then \
	printf '!!! test-readerr-winh FAIL: `type prn` diagnostic count = %s (want 1) -- a CF=1 read is leaking through as data again\n' "$$tcnt"; \
	cat "$(BUILD)/$(WINH_NAME).repl"; exit 1; \
fi
@printf '>>> test-readerr-winh [3/5]: `type prn` -> exactly ONE "Bad command or file name" (no garbage loop)\n'
@# ---- `copy prn out.txt` ALSO aborts clean, and OUT.TXT is left EMPTY. ----
@ccnt=$$(awk '/^A:\\>copy prn out\.txt$$/{f=1;next} /^A:\\>/{f=0} f' "$(BUILD)/$(WINH_NAME).repl" | grep -cF 'Bad command or file name'); \
if [ "$$ccnt" != "1" ]; then \
	printf '!!! test-readerr-winh FAIL: `copy prn out.txt` diagnostic count = %s (want 1)\n' "$$ccnt"; \
	cat "$(BUILD)/$(WINH_NAME).repl"; exit 1; \
fi
@# builtin_type ALWAYS emits its own trailing "\r\n" trailer (~2301) even for
@# a zero-byte file, so the captured block is one blank line on a clean OUT.TXT
@# -- strip CR/LF and require NO surviving (garbage) characters.
@ochars=$$(awk '/^A:\\>type out\.txt$$/{f=1;next} /^A:\\>/{f=0} f' "$(BUILD)/$(WINH_NAME).repl" | tr -d '\r\n' | wc -c | tr -d ' '); \
if [ "$$ochars" != "0" ]; then \
	printf '!!! test-readerr-winh FAIL: OUT.TXT is NOT empty (%s stray byte(s) beyond TYPE'"'"'s own CRLF trailer) -- builtin_copy wrote garbage before aborting\n' "$$ochars"; \
	cat "$(BUILD)/$(WINH_NAME).repl"; exit 1; \
fi
@printf '>>> test-readerr-winh [4/5]: `copy prn out.txt` -> ONE diagnostic, OUT.TXT left EMPTY (no garbage written to disk)\n'
@# ---- EXIT halted cleanly -- proves neither builtin hung (the defining bug). ----
@grep -q '^SHELL-EXIT$$' "$(WINH_SERIAL)" \
	|| { printf '!!! test-readerr-winh FAIL: SHELL-EXIT missing -- a PRN read hung the shell (the initech-winh symptom)\n'; exit 1; }
@grep -q '^SHELL-DONE$$' "$(WINH_SERIAL)" \
	|| { printf '!!! test-readerr-winh FAIL: SHELL-DONE missing -- the REPL did not return to the halt loop\n'; exit 1; }
@printf '>>> test-readerr-winh [5/5]: SHELL-EXIT + SHELL-DONE (EXIT reached -- neither builtin hung)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- AH=3Fh CF=1 aborts clean in TYPE + COPY, no garbage, no hang (initech-winh)\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-readerr-winh-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27169):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_winh.img build/fat_exec.img build/.makefile.stamp`
```make
@printf '>>> test-readerr-winh-mutant: confirming the no-CF-check mutant HANGS on a PRN read (Rule 6)\n'
@# initech-qed1: --legacy-quit -- this leg's WHOLE POINT is that the mutant
@# HANGS forever (never reaches SHELL-EXIT/SHELL-DONE, by design). No
@# completion marker can be named for a guest that intentionally never
@# finishes; the wall-clock --timeout-ms is the correct, honest backstop
@# here (a host-load delay cannot manufacture a hang that isn't real, so
@# there is no false-RED exposure from the legacy quit on this leg).
@$(call fixture-preflight,$(FAT_EXEC_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_EXEC_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_EXEC_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_EXEC_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_EXEC_IMG))),:)
@$(HARNESS_BIN) --disk "$(WINH_MUT_TRACER_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(WINH_MUT_NAME)" --out "$(BUILD)" --timeout-ms 5000 \
	--keys "$(WINH_KEYS)" --keys-after "SHELL-READY" --legacy-quit \
	2> "$(WINH_MUT_REPORT)" || true
@if grep -q 'triple_fault=1' "$(WINH_MUT_REPORT)"; then \
	printf '!!! test-readerr-winh-mutant FAIL: mutant TRIPLE FAULT -- cannot attribute the hang\n'; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(WINH_MUT_SERIAL)" \
	|| { printf '!!! test-readerr-winh-mutant FAIL: mutant never entered the REPL -- RED is meaningless\n'; exit 1; }
@# The mutant must NOT reach SHELL-EXIT -- `type prn` loops forever on the
@# CF=1-as-byte-count bug, so the `exit` keys are never consumed.
@if grep -q '^SHELL-EXIT$$' "$(WINH_MUT_SERIAL)"; then \
	printf '!!! test-readerr-winh-mutant FAIL: mutant reached SHELL-EXIT -- CMD_MUTATE_NO_READ_CF not effective (or TYPE PRN did not hang); the gate does not bite\n'; \
	cat "$(BUILD)/$(WINH_MUT_NAME).serial" 2>/dev/null || true; exit 1; \
fi
@# Positive evidence the mutant actually ran the buggy loop (not a dead boot):
@# the post-SHELL-READY serial must be substantially larger than the fixed
@# leg's ~40-byte "type prn\r\n" + one diagnostic -- the repeating garbage unit.
@sed -n '/^SHELL-READY$$/,$$p' "$(WINH_MUT_SERIAL)" > "$(BUILD)/$(WINH_MUT_NAME).repl"
@rlen=$$(wc -c < "$(BUILD)/$(WINH_MUT_NAME).repl" | tr -d ' '); \
if [ "$$rlen" -lt "500" ]; then \
	printf '!!! test-readerr-winh-mutant FAIL: post-SHELL-READY serial only %s bytes -- no evidence the read-error loop ran (dead boot?)\n' "$$rlen"; exit 1; \
fi; \
printf '>>> test-readerr-winh-mutant: green (mutant never reached SHELL-EXIT; %s bytes of runaway output -- the CF-blind read loops forever, the gate BITES)\n' "$$rlen"
```

## test-ro6c-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5867):
Prerequisites: `build/test_int21_mutant_ro6c_conwrong build/test_int21_mutant_ro6c_isdevinv build/test_int21_mutant_ro6c_badhandle build/test_int21_mutant_ro6c_minorok build/test_int21_mutant_ro6c_nodispatch build/.makefile.stamp`
```make
@printf ">>> test-ro6c-mutant: confirming all five AH=44h IOCTL mutants go RED (Rule 6; beads initech-ro6c)\n"
@if $(TEST_INT21_MUT_RO6C_CONWRONG) >/dev/null 2>&1; then \
	printf '!!! test-ro6c-mutant FAIL: CON_WRONG mutant PASSED -- the CON device-info word is decoration\n'; exit 1; \
else \
	printf '>>> test-ro6c-mutant: green (CON_WRONG mutant correctly RED -- the 0x80D3 CON word bites)\n'; \
fi
@if $(TEST_INT21_MUT_RO6C_ISDEVINV) >/dev/null 2>&1; then \
	printf '!!! test-ro6c-mutant FAIL: ISDEV_INVERT mutant PASSED -- the device/file fork is decoration\n'; exit 1; \
else \
	printf '>>> test-ro6c-mutant: green (ISDEV_INVERT mutant correctly RED -- the bit15 device/file fork bites)\n'; \
fi
@if $(TEST_INT21_MUT_RO6C_BADHANDLE) >/dev/null 2>&1; then \
	printf '!!! test-ro6c-mutant FAIL: BADHANDLE_OK mutant PASSED -- the invalid-handle guard is decoration\n'; exit 1; \
else \
	printf '>>> test-ro6c-mutant: green (BADHANDLE_OK mutant correctly RED -- the 0x0006 guard bites)\n'; \
fi
@if $(TEST_INT21_MUT_RO6C_MINOROK) >/dev/null 2>&1; then \
	printf '!!! test-ro6c-mutant FAIL: MINOR_OK mutant PASSED -- the deferred-minor guard is decoration\n'; exit 1; \
else \
	printf '>>> test-ro6c-mutant: green (MINOR_OK mutant correctly RED -- the AL!=00 -> 0x0001 guard bites)\n'; \
fi
@if $(TEST_INT21_MUT_RO6C_NODISPATCH) >/dev/null 2>&1; then \
	printf '!!! test-ro6c-mutant FAIL: NO_DISPATCH mutant PASSED -- the 0x44 dispatch wiring is decoration\n'; exit 1; \
else \
	printf '>>> test-ro6c-mutant: green (NO_DISPATCH mutant correctly RED -- the 0x44 case bites)\n'; \
fi
```

## test-rtc

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8243):
Prerequisites: `build/test_rtc build/.makefile.stamp`
```make
@printf ">>> test-rtc: MC146818 BCD/binary/12-24h/century/DOW + SET encode round-trip (initech-yv9)\n"
@$(TEST_RTC)
@printf ">>> test-rtc: green\n"
```

## test-rtc-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8249):
Prerequisites: `build/test_rtc_mutant_bcd build/test_rtc_mutant_month build/.makefile.stamp`
```make
@printf ">>> test-rtc-mutant: confirming both mutants go RED (Rule 6)\n"
@if $(TEST_RTC_MUT_BCD) >/dev/null 2>&1; then \
	printf '!!! test-rtc-mutant FAIL: skip-BCD mutant PASSED -- the BCD-decode assertions are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-rtc-mutant: green (skip-BCD mutant correctly RED)\n'; \
fi
@if $(TEST_RTC_MUT_MONTH) >/dev/null 2>&1; then \
	printf '!!! test-rtc-mutant FAIL: month off-by-one mutant PASSED -- the month assertions are decoration\n'; \
	exit 1; \
else \
	printf '>>> test-rtc-mutant: green (month off-by-one mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-sft

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8184):
Prerequisites: `build/test_sft build/.makefile.stamp`
```make
@printf ">>> test-sft: JFT->SFT handle layer + predefined handles 0-4 + DUP/DUP2 (initech-509.3)\n"
@$(TEST_SFT)
@printf ">>> test-sft: green\n"
```

## test-sft-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 8190):
Prerequisites: `build/test_sft_mutant_dup build/test_sft_mutant_dup2 build/test_sft_mutant_inherit build/.makefile.stamp`
```make
@printf ">>> test-sft-mutant: confirming all mutants go RED (Rule 6)\n"
@if $(TEST_SFT_MUT_DUP) >/dev/null 2>&1; then \
	printf '!!! test-sft-mutant FAIL: DUP-no-refcount mutant PASSED -- the ref_count test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-sft-mutant: green (DUP-no-refcount mutant correctly RED)\n'; \
fi
@if $(TEST_SFT_MUT_DUP2) >/dev/null 2>&1; then \
	printf '!!! test-sft-mutant FAIL: DUP2-no-release mutant PASSED -- the release test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-sft-mutant: green (DUP2-no-release mutant correctly RED -- the oracle bites)\n'; \
fi
@if $(TEST_SFT_MUT_INH) >/dev/null 2>&1; then \
	printf '!!! test-sft-mutant FAIL: no-inherit-refcount mutant PASSED -- the JFT-inheritance refcount test is decoration (bsy.9)\n'; \
	exit 1; \
else \
	printf '>>> test-sft-mutant: green (no-inherit-refcount mutant correctly RED -- the inherit/close symmetry bites; bsy.9)\n'; \
fi
```

## test-spec

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 28360):
Prerequisites: `spec/int21h_register.json spec/int21h_calling_convention.json spec/dos_messages.json spec/dos_structs.h spec/dos_banner.txt spec/absdisk_int2526.json test-partition-spec test-block-device-spec test-drive-letters-spec build/.makefile.stamp | build`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-spec : ADR-0003 spec-data oracle\n'
@printf '  Ref: ADR-0003 Appendices A-D / DEC-13 (controlled vocabulary).\n'
@printf '  CLAUDE.md Rule 8 (specs-as-data) / Law 2 (oracle is the truth).\n'
@printf '======================================================================\n'
@printf '>>> test-spec [1/6]: INT 21h register JSON (Appendix A)\n'
@python3 -c "import json,sys; \
```

## test-spurious

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 28018):
Prerequisites: `build/qemu_harness build/spurious_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-spurious : stray vector resumes (no wedge)\n'
@printf '  Ref: bcg.6 (audit 2026-06-13); isr_dispatch_c resumes for vector>31.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (self-test stray-int kernel)\n' "$(SPURIOUS_IMG)"
@printf 'Expecting : SPURIOUS-ARMED + "SPURIOUS vec=FF" + SPURIOUS-RESUMED + no PANIC + triple_fault=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(SPURIOUS_IMG)" \
	--name "$(SPURIOUS_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(SPURIOUS_REPORT)" || true
@cat "$(SPURIOUS_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if [ ! -s "$(SPURIOUS_SERIAL)" ]; then \
	printf '!!! test-spurious FAIL: no serial captured at %s\n' "$(SPURIOUS_SERIAL)"; \
	exit 1; \
fi
@grep -q '^SPURIOUS-ARMED$$' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: never reached the stray int (SPURIOUS-ARMED missing)\n'; exit 1; }
@printf '>>> test-spurious [1/4]: reached the deliberate stray int $$0x70\n'
@grep -q 'SPURIOUS vec=FF' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: no "SPURIOUS vec=FF" -- spurious handler did not run\n'; exit 1; }
@# bcg.7: the dedicated 8259A stubs push their REAL vector (0x2F/0x37), not
@# the 0xFF sentinel -- proving idt routes IRQ7/IRQ15 spurious to the
@# EOI-aware handlers rather than the generic isr_spurious.
@grep -q 'SPURIOUS vec=2F' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: no "SPURIOUS vec=2F" -- master spurious not on its dedicated stub (bcg.7)\n'; exit 1; }
@grep -q 'SPURIOUS vec=37' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: no "SPURIOUS vec=37" -- slave spurious not on its dedicated stub (bcg.7)\n'; exit 1; }
@printf '>>> test-spurious [2/4]: spurious handlers ran -- vec=FF (generic) + vec=2F/37 (dedicated 8259A stubs)\n'
@grep -q '^SPURIOUS-RESUMED$$' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: SPURIOUS-RESUMED missing -- the stray vector WEDGED the machine\n'; exit 1; }
@# bcg.7: the two 8259A spurious-IRQ vectors (master IRQ7 0x2F, slave IRQ15
@# 0x37) must also resume (the slave path runs a master-only EOI first).
@grep -q '^SPURIOUS-IRQ7-RESUMED$$' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: SPURIOUS-IRQ7-RESUMED missing -- master spurious (0x2F) wedged\n'; exit 1; }
@grep -q '^SPURIOUS-IRQ15-RESUMED$$' "$(SPURIOUS_SERIAL)" \
	|| { printf '!!! test-spurious FAIL: SPURIOUS-IRQ15-RESUMED missing -- slave spurious (0x37) wedged\n'; exit 1; }
@if grep -q 'PANIC' "$(SPURIOUS_SERIAL)"; then \
	printf '!!! test-spurious FAIL: a PANIC fired -- a stray vector was treated as a fatal exception\n'; \
	exit 1; \
fi
@printf '>>> test-spurious [3/4]: execution RESUMED past the stray ints incl. 8259A IRQ7/IRQ15 (no wedge, no PANIC)\n'
@if grep -q 'triple_fault=1' "$(SPURIOUS_REPORT)"; then \
	printf '!!! test-spurious FAIL: TRIPLE FAULT -- the stray int cascaded to a silent reboot\n'; \
	exit 1; \
fi
@printf '>>> test-spurious [4/4]: no triple-fault\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- stray/unhandled vector resumed cleanly, machine not wedged\n'
@printf '======================================================================\n'
```

## test-sysinit

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26588):
Prerequisites: `build/qemu_harness build/sysi_boot.img build/fat_sysi.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-sysinit : SYSINIT reads CONFIG.SYS, FILES= has teeth\n'
@printf '  Ref: beads initech-509.2; sysinit_apply_config -> sft_set_files_limit; CONFIG.SYS FILES=8.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + CONFIG.SYS disk %s (primary slave)\n' "$(SYSI_IMG)" "$(FAT_SYSI_IMG)"
@printf 'Expecting : SYSINIT: source=CONFIG.SYS FILES=8 ... + SYSINIT-PROG-OPENS=%s + no triple-fault\n' "$(SYSI_EXPECT_OPENS)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_SYSI_IMG))
@$(HARNESS_BIN) --disk "$(SYSI_IMG)" --disk2 "$(FAT_SYSI_IMG)" \
	--name "$(SYSI_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(SYSI_REPORT)" || true
@cat "$(SYSI_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SYSI_REPORT)"; then \
	printf '!!! test-sysinit FAIL: TRIPLE FAULT (root-cause the SYSINIT path, Rule 3)\n'; exit 1; \
fi
@printf '>>> test-sysinit [1/4]: no triple-fault\n'
@if [ ! -s "$(SYSI_SERIAL)" ]; then \
	printf '!!! test-sysinit FAIL: no serial captured at %s\n' "$(SYSI_SERIAL)"; exit 1; \
fi
@grep -q '^SYSINIT: source=CONFIG.SYS FILES=8 cap=8 ' "$(SYSI_SERIAL)" \
	|| { printf '!!! test-sysinit FAIL: SYSINIT summary (source=CONFIG.SYS FILES=8 cap=8) missing -- CONFIG.SYS not read+parsed+applied\n'; \
	     grep -n 'SYSINIT' "$(SYSI_SERIAL)" || true; exit 1; }
@printf '>>> test-sysinit [2/4]: SYSINIT read+parsed CONFIG.SYS (FILES=8 cap=8 summary present)\n'
@grep -q '^SYSINIT: DEVICE=ANSI.SYS ansi-enabled$$' "$(SYSI_SERIAL)" \
	|| { printf '!!! test-sysinit FAIL: DEVICE=ANSI.SYS ansi-enabled line missing -- the device chain wiring (beads 6zd9) must RECORD ANSI.SYS + set the g_ansi_enabled flag for p96i, not drop it\n'; \
	     grep -n 'SYSINIT' "$(SYSI_SERIAL)" || true; exit 1; }
@printf '>>> test-sysinit [3/4]: DEVICE=ANSI.SYS recorded ansi-enabled (g_ansi_enabled set for the ANSI interpreter, beads 6zd9/p96i)\n'
@tr -d '\r' < "$(SYSI_SERIAL)" | grep -q '^SYSINIT-PROG-OPENS=$(SYSI_EXPECT_OPENS)$$' \
	|| { printf '!!! test-sysinit FAIL: SYSINIT-PROG-OPENS=%s missing -- the FILES= cap did NOT bite at exactly the limit:\n' "$(SYSI_EXPECT_OPENS)"; \
	     grep -n 'SYSINIT-PROG-OPENS' "$(SYSI_SERIAL)" || true; exit 1; }
@printf '>>> test-sysinit [4/4]: SYSINIT-PROG-OPENS=%s -- FILES=8 cap ENFORCED at exactly the limit (4 file slots)\n' "$(SYSI_EXPECT_OPENS)"
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- SYSINIT reads CONFIG.SYS off the volume, the FILES= directive\n'
@printf '            has REAL teeth (the SFT file-slot cap), and it bites at exactly the limit\n'
@printf '            (QEMU only; tri-emulator pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-sysinit-oversize

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 26645):
Prerequisites: `build/qemu_harness build/sysi_boot.img build/fat_sysi_big.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-sysinit-oversize : >1KB CONFIG.SYS honored (bcg.9)\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + oversize CONFIG.SYS disk %s\n' "$(SYSI_IMG)" "$(FAT_SYSI_BIG_IMG)"
@printf 'Expecting : SYSINIT: source=CONFIG.SYS(truncated@1024) FILES=8 ... (NOT baseline)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_SYSI_BIG_IMG))
@$(HARNESS_BIN) --disk "$(SYSI_IMG)" --disk2 "$(FAT_SYSI_BIG_IMG)" \
	--name "$(SYSIBIG_NAME)" --out "$(BUILD)" --timeout-ms 8000 \
	2> "$(SYSIBIG_REPORT)" || true
@cat "$(SYSIBIG_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(SYSIBIG_REPORT)"; then \
	printf '!!! test-sysinit-oversize FAIL: TRIPLE FAULT\n'; exit 1; \
fi
@printf '>>> test-sysinit-oversize [1/3]: no triple-fault\n'
@if [ ! -s "$(SYSIBIG_SERIAL)" ]; then \
	printf '!!! test-sysinit-oversize FAIL: no serial captured\n'; exit 1; \
fi
@grep -q '^SYSINIT: source=CONFIG.SYS(truncated@1024) FILES=8 cap=8 ' "$(SYSIBIG_SERIAL)" \
	|| { printf '!!! test-sysinit-oversize FAIL: oversize CONFIG.SYS not honored (no truncated@1024 FILES=8 marker)\n'; \
	     grep -n 'SYSINIT: source=' "$(SYSIBIG_SERIAL)" || true; exit 1; }
@printf '>>> test-sysinit-oversize [2/3]: first 1KB honored -- FILES=8 applied + truncation flagged\n'
@if grep -q '^SYSINIT: source=baseline' "$(SYSIBIG_SERIAL)"; then \
	printf '!!! test-sysinit-oversize FAIL: fell back to baseline -- the oversize file was discarded\n'; exit 1; \
fi
@printf '>>> test-sysinit-oversize [3/3]: did NOT discard to baseline\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a CONFIG.SYS larger than the scratch is honored (first 1KB) and flagged\n'
@printf '======================================================================\n'
```

## test-trash-hidden-emu

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 185):
Prerequisites: `build/qemu_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@sh harness/diff/fat_diff/trash_hidden_emu.sh $(BUILD) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) green
```

## test-trash-hidden-emu-bochs

Declared tier: EMU; #  recipe to execute (from 'harness/bochs_legs.mk', line 238):
Prerequisites: `build/bochs_harness build/flair_tenants.img build/flair_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-trash-hidden-emu-bochs : BOCHS boot leg of %s (%s video path)\n' "build/flair_tenants.img" "vbe101"
@printf '  Ref: PRD Sec 8 / Rule 5 (tri-emulator); harness/bochs_legs.mk; initech-cqej; WL-0170.\n'
@printf '======================================================================\n'
@command -v $(BOCHS) >/dev/null 2>&1 || { printf '!!! test-trash-hidden-emu-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2); SKIP_BOCHS=1 is the only documented opt-out.\n'; exit 1; }
@true
cp -f build/flair_data.img $(BUILD)/trash-hidden-emu_bochs_data.img
@$(BOCHS_BIN) --disk "build/flair_tenants.img" --disk2 "$(BUILD)/trash-hidden-emu_bochs_data.img"  --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY --name "trash-hidden-emu_bochs" --out "$(BUILD)" --timeout-ms 45000 2> "$(BUILD)/trash-hidden-emu_bochs.report.txt" || true
@cat "$(BUILD)/trash-hidden-emu_bochs.report.txt"
@sh harness/emu/bochs_leg_check.sh test-trash-hidden-emu-bochs "$(BUILD)/trash-hidden-emu_bochs.report.txt" "$(BUILD)/trash-hidden-emu_bochs.serial" vbe101 FLAIR-HEAP-OK FLAIR-FAT-MOUNT-OK FLAIR-DESKTOP FLAIR-LIVE-READY !PANIC
```

## test-trash-hidden-emu-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/diff/fat_diff/kernel_hygiene.mk', line 201):
Prerequisites: `build/qemu_harness build/trash_visible.img build/flair_data.img build/.makefile.stamp | build/khygiene-evidence`
```make
@if sh harness/diff/fat_diff/trash_hidden_emu.sh $(BUILD) $(BUILD)/trash_visible.img $(FLAIR_DATA_IMG) mutant > $(BUILD)/khygiene-evidence/mutant-trash-hidden.log 2>&1; then echo 'FAIL visible TRASH emu mutant survived'; exit 1; fi
@grep 'RED TRASH hidden attribute' $(BUILD)/khygiene-evidence/mutant-trash-hidden.log
@echo 'GREEN test-trash-hidden-emu-mutant: FAT12_MUTATE_HIDE_DIR_NO_WRITE correctly RED'
```

## test-tsr

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35605):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_tsr.img build/.makefile.stamp`
```make
$(call tsr-boot,$(SECONDARY_TRACER_IMG),tsr)
@! grep -q 'triple_fault=1' $(BUILD)/tsr.report || { cat $(BUILD)/tsr.report; printf '!!! test-tsr RED: triple fault\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' $(BUILD)/tsr.serial | tr -d '\r' > $(BUILD)/tsr.repl
@grep -q '^SK-INSTALLED$$' $(BUILD)/tsr.repl || { cat $(BUILD)/tsr.repl; printf '!!! test-tsr RED: TSR.COM did not install\n'; exit 1; }
@grep -qF 'TSR      COM' $(BUILD)/tsr.repl && grep -qF 'GREET    COM' $(BUILD)/tsr.repl || { cat $(BUILD)/tsr.repl; printf '!!! test-tsr RED: DIR did not run after the resident\n'; exit 1; }
@grep -q '^GREETINGS FROM A:GREET.COM$$' $(BUILD)/tsr.repl || { cat $(BUILD)/tsr.repl; printf '!!! test-tsr RED: GREET.COM did not run above the resident\n'; exit 1; }
@$(call tsr-popup-ok,$(BUILD)/tsr.repl) || { cat $(BUILD)/tsr.repl; printf '!!! test-tsr RED: no SK-POPUP INDOS=1 SCREEN=SAME between WAITKEY-WAITING and WAITKEY-GOT=x\n'; exit 1; }
@grep -q '^SHELL-DONE$$' $(BUILD)/tsr.serial || { printf '!!! test-tsr RED: the shell did not exit cleanly\n'; exit 1; }
@printf '>>> test-tsr: green (TSR.COM resident via AH=31h; DIR + GREET.COM ran above it; Ctrl+Alt popped it up inside WAITKEY'"'"'s AH=08h wait, InDOS=1, page restored; WAITKEY then got x)\n'
```

## test-tsr-exec

Declared tier: EMU; #  recipe to execute (from 'harness/emu/tsr_exec.mk', line 25):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/fat_tsr_exec.img build/.makefile.stamp`
```make
$(call tsr-exec-boot,$(SECONDARY_TRACER_IMG),tsr_exec)
@sh harness/emu/tsr_exec_check.sh $(BUILD)/tsr_exec
@printf '>>> test-tsr-exec: green (EDLIN fixed-base before KEEP; named refusal after KEEP; MZ relocates twice; resident popup survives)\n'
```

## test-tsr-exec-mutant

Declared tier: EMU; #  recipe to execute (from 'harness/emu/tsr_exec.mk', line 50):
Prerequisites: `test-tsr-exec build/tsr_exec_fixed.img build/tsr_exec_flat.img build/tsr_exec_overlap.img build/.makefile.stamp`
```make
$(call tsr-exec-boot,$(BUILD)/tsr_exec_fixed.img,tsr_exec_fixed)
$(call tsr-exec-boot,$(BUILD)/tsr_exec_flat.img,tsr_exec_flat)
$(call tsr-exec-boot,$(BUILD)/tsr_exec_overlap.img,tsr_exec_overlap)
@set -eu; for m in fixed flat overlap; do \
 s=$(BUILD)/tsr_exec_$$m; \
 if sh harness/emu/tsr_exec_check.sh $$s >$$s.check 2>&1; then \
  printf '!!! test-tsr-exec-mutant FAIL: %s survived\n' $$m; exit 1; fi; \
 grep -qx 'SK-INSTALLED' $$s.repl; \
 grep -qx 'Cannot edit .BAK file--rename file' $$s.repl; \
 grep '^!!! test-tsr-exec RED:' $$s.check; \
 printf '>>> test-tsr-exec-mutant: %s RED after successful installer\n' $$m; \
done
@printf '>>> test-tsr-exec-mutant: green (fixed relocation base, unchecked raw flat image, overlapping MCB all RED)\n'
```

## test-tsr-io

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7851):
Prerequisites: `build/test_tsr_io build/.makefile.stamp`
```make
@$(TEST_TSR_IO)
@printf '>>> test-tsr-io: green (BIOS flag byte Ctrl+Alt=0Ch; text page save/restore == pixel snapshot + cursor)\n'
```

## test-tsr-io-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7854):
Prerequisites: `build/test_tsr_io_KBD_MUT_NO_ALT build/test_tsr_io_CONSOLE_MUT_RESTORE_NO_DRAW build/.makefile.stamp`
```make
@for m in $(TEST_TSR_IO_MUTANTS); do \
	if $(BUILD)/test_tsr_io_$$m > $(BUILD)/test_tsr_io_$$m.log 2>&1; then \
		printf '!!! test-tsr-io-mutant FAIL: %s survived\n' $$m; exit 1; fi; \
	printf '    %s RED: %s\n' $$m "$$(grep -m1 FAIL $(BUILD)/test_tsr_io_$$m.log)"; \
done
@printf '>>> test-tsr-io-mutant: green (KBD_MUT_NO_ALT, CONSOLE_MUT_RESTORE_NO_DRAW both RED)\n'
```

## test-tsr-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 35615):
Prerequisites: `test-tsr build/tsr_nokeep.img build/tsr_noidle.img build/.makefile.stamp`
```make
$(call tsr-boot,$(BUILD)/tsr_nokeep.img,tsr_nokeep)
$(call tsr-boot,$(BUILD)/tsr_noidle.img,tsr_noidle)
@tr -d '\r' < $(BUILD)/tsr_nokeep.serial > $(BUILD)/tsr_nokeep.txt; tr -d '\r' < $(BUILD)/tsr_noidle.serial > $(BUILD)/tsr_noidle.txt
@grep -q '^SK-INSTALLED$$' $(BUILD)/tsr_nokeep.txt || { cat $(BUILD)/tsr_nokeep.txt; printf '!!! test-tsr-mutant: TSR_MUT_NO_KEEP session never installed (infrastructure)\n'; exit 1; }
@if $(call tsr-popup-ok,$(BUILD)/tsr_nokeep.txt); then printf '!!! test-tsr-mutant FAIL: TSR_MUT_NO_KEEP survived\n'; exit 1; fi
@grep -q '^WAITKEY-GOT=x$$' $(BUILD)/tsr_noidle.txt || { cat $(BUILD)/tsr_noidle.txt; printf '!!! test-tsr-mutant: TSR_MUT_NO_IDLE session broke (infrastructure)\n'; exit 1; }
@if $(call tsr-popup-ok,$(BUILD)/tsr_noidle.txt); then printf '!!! test-tsr-mutant FAIL: TSR_MUT_NO_IDLE survived\n'; exit 1; fi
@printf '>>> test-tsr-mutant: green (TSR_MUT_NO_KEEP RED: GREET loaded over the resident, %s; TSR_MUT_NO_IDLE RED: WAITKEY got x with no pop-up)\n' "$$(grep -m1 '^SK-POPUP' $(BUILD)/tsr_nokeep.txt || echo 'no SK-POPUP')"
```

## test-tsr-unit

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7828):
Prerequisites: `build/test_tsr build/.makefile.stamp`
```make
@$(TEST_TSR)
@printf '>>> test-tsr-unit: green (KEEP top-level + MCB-hosted, files/blocks kept, next EXEC above, 31h twice, INT 27h, InDOS, IRQ gate type)\n'
```

## test-tsr-unit-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 7831):
Prerequisites: `build/test_tsr_TSR_MUT_NO_KEEP build/test_tsr_INT21_MUT_INDOS_EXEC build/test_tsr_INT21_MUT_SETVECT_TRAP build/.makefile.stamp`
```make
@for m in $(TEST_TSR_MUTANTS); do \
	if $(BUILD)/test_tsr_$$m > $(BUILD)/test_tsr_$$m.log 2>&1; then \
		printf '!!! test-tsr-unit-mutant FAIL: %s survived\n' $$m; exit 1; fi; \
	printf '    %s RED: %s\n' $$m "$$(grep -m1 FAIL $(BUILD)/test_tsr_$$m.log)"; \
done
@printf '>>> test-tsr-unit-mutant: green (TSR_MUT_NO_KEEP, INT21_MUT_INDOS_EXEC, INT21_MUT_SETVECT_TRAP all RED)\n'
```

## test-type

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 25633):
Prerequisites: `build/qemu_harness build/demo_boot.img build/fat_data.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-type : a PROGRAM TYPEs a real file\n'
@printf '  Ref: docs/research/fs-mount-sft-ground-truth.md Sec 5.2. beads initech-509.5.\n'
@printf '  Prove OPEN(3Dh)+READ(3Fh)+WRITE(40h)+CLOSE(3Eh) over mounted FAT12.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s + data disk %s (primary slave)\n' "$(DEMO_IMG)" "$(FAT_DATA_IMG)"
@printf 'Expecting : FILEIO-BIND-OK + "%s" between TYPE-OUTPUT-BEGIN/END + TYPE-EXIT rc=0\n' "$(TYPE_CONTENT)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(FAT_DATA_IMG))
@# initech-1x6x2.1: boot a private copy -- QEMU write-locks a raw --disk2,
@# so legs sharing $(FAT_DATA_IMG) failed to start under make -j (exit=1, serial_len=0).
cp -f $(FAT_DATA_IMG) $(FLAIR_GATE_DATA)
@$(if $(call fixture-spec,$(FAT_DATA_IMG)),$(call fixture-check,harness,$(FLAIR_GATE_DATA),$(call fixture-spec,$(FAT_DATA_IMG))),:)
@$(HARNESS_BIN) --disk "$(DEMO_IMG)" --disk2 "$(FLAIR_GATE_DATA)" \
	--name "$(TYPE_NAME)" --out "$(BUILD)" --timeout-ms 6000 \
	2> "$(TYPE_REPORT)" || true
@cat "$(TYPE_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(TYPE_REPORT)"; then \
	printf '!!! test-type FAIL: TRIPLE FAULT\n'; exit 1; \
fi
@printf '>>> test-type [1/4]: no triple-fault\n'
@if [ ! -s "$(TYPE_SERIAL)" ]; then \
	printf '!!! test-type FAIL: no serial captured at %s\n' "$(TYPE_SERIAL)"; exit 1; \
fi
@grep -q '^FILEIO-BIND-OK$$' "$(TYPE_SERIAL)" \
	|| { printf '!!! test-type FAIL: FILEIO-BIND-OK missing -- the FAT file backend never bound\n'; exit 1; }
@printf '>>> test-type [2/4]: FILEIO-BIND-OK (FAT12 file backend bound to int21)\n'
@if grep -q 'TYPE-OPEN-FAIL' "$(TYPE_SERIAL)"; then \
	printf '!!! test-type FAIL: TYPE-OPEN-FAIL -- AH=3Dh OPEN of HELLO.TXT failed (root-cause the OPEN path, Rule 3)\n'; exit 1; \
fi
@grep -qF '$(TYPE_CONTENT)' "$(TYPE_SERIAL)" \
	|| { printf '!!! test-type FAIL: HELLO.TXT contents "%s" missing -- OPEN/READ/WRITE did not deliver the file to stdout\n' "$(TYPE_CONTENT)"; exit 1; }
@printf '>>> test-type [3/4]: the program OPENed+READ HELLO.TXT and WROTE its contents to stdout\n'
@grep -q '^TYPE-EXIT rc=0$$' "$(TYPE_SERIAL)" \
	|| { printf '!!! test-type FAIL: TYPE-EXIT rc=0 missing -- the TYPE program did not CLOSE+EXIT cleanly\n'; exit 1; }
@printf '>>> test-type [4/4]: TYPE-EXIT rc=0 (CLOSE + return-to-loader)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- a flat program OPENed, READ, and TYPEd a real FAT12 file via INT 21h\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-u6wa-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6312):
Prerequisites: `build/test_fileio_mutant_chdir_noattr build/test_fileio_subdir_mut_noroot build/fat12_nested.img build/.makefile.stamp`
```make
@printf ">>> test-u6wa-mutant: confirming both CHDIR mutants go RED (Rule 6; beads initech-u6wa)\n"
@if $(TEST_FILEIO_MUT_CHDIR_NOATTR) >/dev/null 2>&1; then \
	printf '!!! test-u6wa-mutant FAIL: CHDIR-NOATTR mutant PASSED -- the CD-into-a-file gate is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-u6wa-mutant: green (CHDIR-NOATTR mutant correctly RED)\n'; \
fi
@if $(TEST_FILEIO_SUBDIR_MUT_NOROOT) "$(FAT12_NESTED_IMG)" "$(FAT12_FIXTURE_DIR)" >/dev/null 2>&1; then \
	printf '!!! test-u6wa-mutant FAIL: CWD-NOROOT mutant PASSED -- the relative-CWD seed test is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-u6wa-mutant: green (CWD-NOROOT mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-ut6d

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27327):
Prerequisites: `build/qemu_harness build/tracer_secondary.img harness/diff/fat_diff/fixtures/hello.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-ut6d : COMMAND.COM MD/RD/CD subdir cycle\n'
@printf '  Ref: ADR-0003 DEC-11/DEC-12; spec/int21h_calling_convention.json (AH=39h/3Ah/3Bh/47h).\n'
@printf '  beads initech-ut6d. Inject: md sub / cd sub / dir / cd .. / rd sub / cd sub / exit.\n'
@printf '======================================================================\n'
@# Re-mint a FRESH writable FAT12 disk so MD SUB never collides with a stale SUB.
@dd if=/dev/zero of=$(UT6D_IMG) bs=512 count=2880 status=none
@mformat -i $(UT6D_IMG) -f 1440 ::
@mcopy -i $(UT6D_IMG) $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
@$(call fixture-check,recipe,$(UT6D_IMG),HELLO.TXT)
@printf 'Booting   : %s + FRESH WRITABLE disk %s (primary slave)\n' "$(SECONDARY_TRACER_IMG)" "$(UT6D_IMG)"
@printf 'Expecting : SHELL-READY + prompt "A:\\SUB>" after CD SUB + clean SHELL-EXIT/SHELL-DONE\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(UT6D_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(UT6D_IMG)" \
	--name "$(UT6D_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(UT6D_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(UT6D_REPORT)" || true
@cat "$(UT6D_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(UT6D_REPORT)"; then \
	printf '!!! test-ut6d FAIL: TRIPLE FAULT -- the shell boot or a directory command crashed\n'; exit 1; \
fi
@printf '>>> test-ut6d [1/6]: no triple-fault\n'
@if [ ! -s "$(UT6D_SERIAL)" ]; then \
	printf '!!! test-ut6d FAIL: no serial captured at %s\n' "$(UT6D_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(UT6D_SERIAL)" \
	|| { printf '!!! test-ut6d FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-ut6d [2/6]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to the post-SHELL-READY REPL transcript (the boot demos print earlier).
@sed -n '/^SHELL-READY$$/,$$p' "$(UT6D_SERIAL)" > "$(BUILD)/$(UT6D_NAME).repl"
@# ---- CENTERPIECE: after `CD SUB` the GETCWD-composed prompt shows "A:\SUB>". ----
@grep -qF 'A:\SUB>' "$(BUILD)/$(UT6D_NAME).repl" \
	|| { printf '!!! test-ut6d FAIL: the prompt never showed "A:\\SUB>" after CD SUB -- the GETCWD-composed $$P$$G prompt did not reflect the CWD (root-cause the prompt compose / do_chdir wiring, Rule 3)\n'; exit 1; }
@printf '>>> test-ut6d [3/6]: prompt showed "A:\\SUB>" after CD SUB (GETCWD-composed $$P$$G reflects the live CWD)\n'
@# ---- DIR inside SUB lists the subdir dot-dirs '.' and '..' (Fix 3). The DIR
@# formatter (cmd_format_dir_line) splits the 8.3 columns at 0 and 9,
@# then puts "<DIR>" in the size field at columns 16..20, so a dot-dir line begins "<name>" and contains
@# "<DIR>". Anchor each entry name at line start AND require the <DIR> marker so
@# we match a real directory row, not an incidental '.' elsewhere. ----
@grep -Eq '^\.[[:space:]]+<DIR>' "$(BUILD)/$(UT6D_NAME).repl" \
	|| { printf '!!! test-ut6d FAIL: DIR inside SUB did not list the "." dot-dir (subdir . entry missing -- MD did not create a real directory with dot-dirs)\n'; exit 1; }
@grep -Eq '^\.\.[[:space:]]+<DIR>' "$(BUILD)/$(UT6D_NAME).repl" \
	|| { printf '!!! test-ut6d FAIL: DIR inside SUB did not list the ".." parent dot-dir (subdir .. entry missing)\n'; exit 1; }
@printf '>>> test-ut6d [4/6]: DIR inside SUB listed the "." and ".." dot-dirs (MD created a real subdirectory)\n'
@# ---- RD-REMOVAL PROOF (Fix 1): the SECOND `cd sub` is injected AFTER `rd sub`.
@# If RD truly removed SUB, the re-CD FAILS -> builtin_cd prints the controlled
@# "Invalid directory" (MSG-DOS-0018) and the prompt stays "A:\>". A silent RD
@# failure would instead let the re-CD SUCCEED, re-show "A:\SUB>", and SUPPRESS
@# this diagnostic. Asserting the diagnostic is present is what proves removal.
@# (Mutation-proven by the CMD_MUTATE_RD_NOOP leg of test-ut6d-mutant.) ----
@grep -qF 'Invalid directory' "$(BUILD)/$(UT6D_NAME).repl" \
	|| { printf '!!! test-ut6d FAIL: the post-RD `cd sub` did NOT print "Invalid directory" -- SUB still resolves, so RD did not actually remove it (a SILENT RD failure; root-cause builtin_rd / dos_rmdir, Rule 3)\n'; exit 1; }
@printf '>>> test-ut6d [5/6]: post-RD `cd sub` failed with "Invalid directory" (RD genuinely removed SUB)\n'
@# ---- The cycle completes cleanly (MD created SUB; CD/RD round-tripped; EXIT). ----
@grep -q '^SHELL-EXIT$$' "$(UT6D_SERIAL)" \
	|| { printf '!!! test-ut6d FAIL: SHELL-EXIT missing -- the MD/CD/RD cycle did not reach EXIT cleanly\n'; exit 1; }
@grep -q '^SHELL-DONE$$' "$(UT6D_SERIAL)" \
	|| { printf '!!! test-ut6d FAIL: SHELL-DONE missing -- the REPL did not return to the halt loop\n'; exit 1; }
@printf '>>> test-ut6d [6/6]: MD/CD/DIR/CD../RD/CD cycle completed; EXIT halted cleanly (SHELL-EXIT + SHELL-DONE)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- MD created a real dir (DIR shows ./..), CD entered it (prompt "A:\\SUB>"),\n'
@printf '            and RD removed it -- proven because the post-RD `cd sub` failed "Invalid directory"\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-ut6d-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27409):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_ut6d.img build/tracer_boot_rdnoop_ut6d.img harness/diff/fat_diff/fixtures/hello.txt build/.makefile.stamp`
```make
@printf ">>> test-ut6d-mutant: confirming BOTH ut6d mutants go RED for the RIGHT reason (Rule 6; beads initech-ut6d)\n"
@# ---------------- Leg A: NO_CWD_PROMPT (subdir-prompt assertion) ----------------
@printf '%s\n' '---- leg A: root-pinned-prompt mutant (-DCMD_MUTATE_NO_CWD_PROMPT) ----'
@dd if=/dev/zero of=$(UT6D_MUT_IMG) bs=512 count=2880 status=none
@mformat -i $(UT6D_MUT_IMG) -f 1440 ::
@mcopy -i $(UT6D_MUT_IMG) $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
@$(call fixture-check,recipe,$(UT6D_MUT_IMG),HELLO.TXT)
@$(call fixture-preflight,$(UT6D_MUT_IMG))
@$(HARNESS_BIN) --disk "$(UT6D_TRACER_MUT_IMG)" --disk2 "$(UT6D_MUT_IMG)" \
	--name "$(UT6D_MUT_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(UT6D_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(UT6D_MUT_REPORT)" || true
@# Hard-FAIL on a missing/empty serial -- a dead boot must not pass for absence.
@if [ ! -s "$(UT6D_MUT_SERIAL)" ]; then \
	printf '!!! test-ut6d-mutant FAIL (leg A): no serial captured at %s -- mutant boot is dead, RED is meaningless\n' "$(UT6D_MUT_SERIAL)"; exit 1; \
fi
@# CONFIRM the mutant genuinely booted and ran the cycle before trusting the RED.
@grep -q '^SHELL-READY$$' "$(UT6D_MUT_SERIAL)" \
	|| { printf '!!! test-ut6d-mutant FAIL (leg A): SHELL-READY missing -- mutant never reached the REPL, RED is meaningless\n'; exit 1; }
@grep -Eq '^SHELL-EXIT$$|^SHELL-DONE$$' "$(UT6D_MUT_SERIAL)" \
	|| { printf '!!! test-ut6d-mutant FAIL (leg A): neither SHELL-EXIT nor SHELL-DONE -- mutant did not run the cycle, RED is meaningless\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' "$(UT6D_MUT_SERIAL)" > "$(BUILD)/$(UT6D_MUT_NAME).repl"
@if grep -qF 'A:\SUB>' "$(BUILD)/$(UT6D_MUT_NAME).repl"; then \
	printf '!!! test-ut6d-mutant FAIL (leg A): the root-pinned-prompt mutant STILL showed "A:\\SUB>" -- the subdir-prompt assertion is decoration\n'; \
	exit 1; \
fi
@printf '>>> test-ut6d-mutant leg A: green (mutant booted + ran the cycle; "A:\\SUB>" correctly ABSENT -- the subdir-prompt assertion bites)\n'
@# ---------------- Leg B: RD_NOOP (RD-removal assertion) ----------------
@printf '%s\n' '---- leg B: silent-RD-no-op mutant (-DCMD_MUTATE_RD_NOOP) ----'
@dd if=/dev/zero of=$(UT6D_RDNOOP_IMG) bs=512 count=2880 status=none
@mformat -i $(UT6D_RDNOOP_IMG) -f 1440 ::
@mcopy -i $(UT6D_RDNOOP_IMG) $(FAT12_FIXTURE_DIR)/hello.txt ::HELLO.TXT
@$(call fixture-check,recipe,$(UT6D_RDNOOP_IMG),HELLO.TXT)
@$(call fixture-preflight,$(UT6D_RDNOOP_IMG))
@$(HARNESS_BIN) --disk "$(UT6D_TRACER_RDNOOP_IMG)" --disk2 "$(UT6D_RDNOOP_IMG)" \
	--name "$(UT6D_RDNOOP_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(UT6D_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(UT6D_RDNOOP_REPORT)" || true
@if [ ! -s "$(UT6D_RDNOOP_SERIAL)" ]; then \
	printf '!!! test-ut6d-mutant FAIL (leg B): no serial captured at %s -- mutant boot is dead, RED is meaningless\n' "$(UT6D_RDNOOP_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(UT6D_RDNOOP_SERIAL)" \
	|| { printf '!!! test-ut6d-mutant FAIL (leg B): SHELL-READY missing -- mutant never reached the REPL, RED is meaningless\n'; exit 1; }
@grep -Eq '^SHELL-EXIT$$|^SHELL-DONE$$' "$(UT6D_RDNOOP_SERIAL)" \
	|| { printf '!!! test-ut6d-mutant FAIL (leg B): neither SHELL-EXIT nor SHELL-DONE -- mutant did not run the cycle, RED is meaningless\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' "$(UT6D_RDNOOP_SERIAL)" > "$(BUILD)/$(UT6D_RDNOOP_NAME).repl"
@# Under RD_NOOP, SUB persists -> the post-RD `cd sub` SUCCEEDS, so the controlled
@# "Invalid directory" diagnostic is ABSENT and the RD-removal assertion goes RED.
@if grep -qF 'Invalid directory' "$(BUILD)/$(UT6D_RDNOOP_NAME).repl"; then \
	printf '!!! test-ut6d-mutant FAIL (leg B): the silent-RD-no-op mutant STILL printed "Invalid directory" -- the RD-removal assertion is not isolating RD (it fired for another reason)\n'; \
	exit 1; \
fi
@# And the persisted SUB means the post-RD re-CD re-showed "A:\SUB>" (sanity: the
@# re-CD really did succeed, confirming SUB was never removed).
@grep -qF 'A:\SUB>' "$(BUILD)/$(UT6D_RDNOOP_NAME).repl" \
	|| { printf '!!! test-ut6d-mutant FAIL (leg B): SUB did not persist (no "A:\\SUB>") -- the RD_NOOP mutant did not behave as a silent RD no-op; the RED is for the wrong reason\n'; exit 1; }
@printf '>>> test-ut6d-mutant leg B: green (mutant booted + ran the cycle; "Invalid directory" correctly ABSENT, SUB persisted -- the RD-removal assertion bites)\n'
@printf '>>> test-ut6d-mutant: green (BOTH legs RED for the right reason; the oracle bites on subdir-prompt AND RD-removal)\n'
```

## test-vect

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 28176):
Prerequisites: `build/qemu_harness build/vect_boot.img build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-vect : INT 24h crit-error + vector save/restore\n'
@printf '  Ref: ADR-0003 DEC-10; App C MSG-DOS-0001. beads initech-509.8. Law 2 / Rule 5.\n'
@printf '======================================================================\n'
@printf 'Booting   : %s (vect self-test kernel), injecting --keys "a" after VECT-PROG-READY\n' "$(VECT_IMG)"
@printf 'Expecting : V24PRE=N + MSG-DOS-0001 + CRIT-AL=1 + V24POST==V24PRE + triple_fault=0\n'
@printf '%s\n' '----------------------------------------------------------------------'
@$(HARNESS_BIN) --disk "$(VECT_IMG)" \
	--name "$(VECT_NAME)" --out "$(BUILD)" --timeout-ms 9000 \
	--keys "a" --keys-after "VECT-PROG-READY" \
	--quit-after "V24POST=" \
	2> "$(VECT_REPORT)" || true
@cat "$(VECT_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if [ ! -s "$(VECT_SERIAL)" ]; then \
	printf '!!! test-vect FAIL: no serial captured at %s\n' "$(VECT_SERIAL)"; \
	exit 1; \
fi
@# [1] MSG-DOS-0001 must appear (the critical error was presented A/R/F).
@grep -q 'Abort, Retry, Fail?' "$(VECT_SERIAL)" \
	|| { printf '!!! test-vect FAIL: MSG-DOS-0001 ("Abort, Retry, Fail?") missing -- INT 24h did not present the critical error\n'; exit 1; }
@printf '>>> test-vect [1/4]: MSG-DOS-0001 presented (critical error -> A/R/F prompt)\n'
@# [2] CRIT-AL=1: the injected 'a' was processed as Abort (the A/R/F action).
@tr -d '\r' < "$(VECT_SERIAL)" | grep -q '^CRIT-AL=1$$' \
	|| { printf '!!! test-vect FAIL: CRIT-AL=1 missing -- INT 24h did not process the injected key as Abort (root-cause the crit-error path, Rule 3)\n'; exit 1; }
@printf '>>> test-vect [2/4]: CRIT-AL=1 (injected '\''a'\'' processed -> Abort)\n'
@# [3] V24POST == V24PRE: the loader RESTORED the parent's 0x24 vector on EXIT,
@# despite the child's SETVECT to a bogus handler. Extract both decimals and
@# compare; they MUST be equal AND non-empty.
@PRE=$$(tr -d '\r' < "$(VECT_SERIAL)" | sed -n 's/^V24PRE=//p' | head -1); \
 POST=$$(tr -d '\r' < "$(VECT_SERIAL)" | sed -n 's/^V24POST=//p' | head -1); \
 if [ -z "$$PRE" ] || [ -z "$$POST" ]; then \
	printf '!!! test-vect FAIL: V24PRE (%s) or V24POST (%s) missing from serial\n' "$$PRE" "$$POST"; \
	exit 1; \
 fi; \
 if [ "$$PRE" != "$$POST" ]; then \
	printf '!!! test-vect FAIL: V24POST=%s != V24PRE=%s -- the loader did NOT restore the parent 0x24 vector on EXIT (the child SETVECT leaked)\n' "$$POST" "$$PRE"; \
	exit 1; \
 fi; \
 printf '>>> test-vect [3/4]: V24POST=%s == V24PRE=%s (loader restored 0x24 across EXEC/EXIT)\n' "$$POST" "$$PRE"
@# [4] clean run: no triple fault + the key was injected.
@if grep -q 'triple_fault=1' "$(VECT_REPORT)"; then \
	printf '!!! test-vect FAIL: TRIPLE FAULT during the vect run\n'; \
	exit 1; \
fi
@grep -q 'keys_sent=1' "$(VECT_REPORT)" \
	|| { printf '!!! test-vect FAIL: harness did not report keys_sent=1 (a)\n'; exit 1; }
@printf '>>> test-vect [4/4]: triple_fault=0 + keys_sent=1 (clean run, key injected)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- INT 24h presented MSG-DOS-0001 + processed Abort; the loader\n'
@printf '            saved/restored the 0x24 vector across EXEC/EXIT (V24POST==V24PRE)\n'
@printf '======================================================================\n'
```

## test-x8fs

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5712):
Prerequisites: `build/test_conin build/.makefile.stamp`
```make
@printf ">>> test-x8fs: AH=3Fh on CON (handle 0) cooked line-read (line+CR+LF, inclusive count)\n"
@$(TEST_CONIN)
@printf ">>> test-x8fs: green\n"
```

## test-x8fs-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 5717):
Prerequisites: `build/test_conin_mutant_nocooked build/.makefile.stamp`
```make
@printf ">>> test-x8fs-mutant: confirming the no-cooked mutant goes RED (Rule 6)\n"
@if $(TEST_CONIN_MUT_NOCOOKED) >/dev/null 2>&1; then \
	printf '!!! test-x8fs-mutant FAIL: no-cooked mutant PASSED -- the AH=3Fh CON-read oracle is decoration\n'; \
	exit 1; \
else \
	printf '>>> test-x8fs-mutant: green (no-cooked mutant correctly RED -- the oracle bites)\n'; \
fi
```

## test-zs24

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6340):
Prerequisites: `build/test_fileio_subdir build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: mtools `mcopy` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mdir    >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: mtools `mdir` not found.\n'; exit 1; }
@command -v python3 >/dev/null 2>&1 || { printf '!!! test-zs24 FAIL: python3 not found (independent reference).\n'; exit 1; }
@printf ">>> test-zs24: subdir file CREATE/WRITE/LSEEK-WRITE/UNLINK vs mtools + python3 (beads initech-zs24)\n"
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@# (1) CREATE+WRITE '\SUB\NEW.TXT' (700 bytes, spans 2 clusters).
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write create-write
@mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/NEW.TXT $(BUILD)/zs24_mcopy.bin
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\NEW.TXT' > $(BUILD)/zs24_py.bin
@python3 -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); m=open("$(BUILD)/zs24_mcopy.bin","rb").read(); p=open("$(BUILD)/zs24_py.bin","rb").read(); sys.exit(0 if (m==exp and p==exp and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [CREATE+WRITE]: mcopy/python read-back != written bytes\n'; exit 1; }
@mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'NEW *TXT' \
	|| { printf '!!! test-zs24 FAIL [CREATE+WRITE]: mdir ::SUB does not list NEW.TXT\n'; exit 1; }
@printf '>>> test-zs24 [CREATE+WRITE]: NEW.TXT (700B, 2 clusters) lands in \\SUB; mcopy==python==written\n'
@# (2) Positioned LSEEK+WRITE: splice 50 bytes at offset 600.
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write seek-write
@mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/NEW.TXT $(BUILD)/zs24_mcopy2.bin
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\NEW.TXT' > $(BUILD)/zs24_py2.bin
@python3 -c 'import sys; e=bytearray(65+(i%26) for i in range(700)); e[600:650]=bytes(48+(i%10) for i in range(50)); e=bytes(e); m=open("$(BUILD)/zs24_mcopy2.bin","rb").read(); p=open("$(BUILD)/zs24_py2.bin","rb").read(); sys.exit(0 if (m==e and p==e and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [LSEEK+WRITE]: spliced read-back != expected\n'; exit 1; }
@printf '>>> test-zs24 [LSEEK+WRITE]: positioned splice at offset 600 byte-exact (mcopy==python)\n'
@# (3) UNLINK '\SUB\NEW.TXT' -> mdir ::SUB no longer lists it; python list == only NESTED.TXT.
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write unlink
@if mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'NEW *TXT'; then \
	printf '!!! test-zs24 FAIL [UNLINK]: mdir ::SUB STILL lists NEW.TXT after UNLINK\n'; exit 1; fi
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --list-path 'SUB' | grep -q '^NESTED.TXT ' \
	|| { printf '!!! test-zs24 FAIL [UNLINK]: python --list-path SUB lost the original NESTED.TXT\n'; exit 1; }
@if python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --list-path 'SUB' | grep -q '^NEW.TXT '; then \
	printf '!!! test-zs24 FAIL [UNLINK]: python --list-path SUB STILL lists NEW.TXT\n'; exit 1; fi
@printf '>>> test-zs24 [UNLINK]: NEW.TXT removed from \\SUB; NESTED.TXT intact (mtools + python agree)\n'
@# (4) ROOT regression: a root CREATE+WRITE still works under the generalized primitives.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write root-regress
@mcopy -n -i $(FAT12_ZS24_IMG) ::ROOTNEW.TXT $(BUILD)/zs24_root.bin
@python3 -c 'import sys; e=bytes(97+(i%26) for i in range(300)); m=open("$(BUILD)/zs24_root.bin","rb").read(); sys.exit(0 if m==e else 1)' \
	|| { printf '!!! test-zs24 FAIL [ROOT-REGRESS]: root CREATE+WRITE read-back != written\n'; exit 1; }
@printf '>>> test-zs24 [ROOT-REGRESS]: a ROOT file write still works (dir_start==0 path intact)\n'
@# (5) DIRECTORY GROW (Fix 1): the riskiest new function fat12_grow_dir. \SUB
@# holds '.'/'..'/NESTED.TXT/DEEP = 4 entries; spc==1 => 16 entries/cluster, so
@# GROW00..GROW11 fill slots 4..15 (the first cluster) and GROW12 (slot 16)
@# FORCES fat12_grow_dir to append a 2nd cluster. Diff with BOTH independent
@# references: EVERY GROW file is listed AND the boundary file (GROW12) reads
@# back byte-exact from the GROWN cluster (proving FAT relink + slot map +
@# zero-fill). The boundary file content is 40 bytes of 'A'+12 == 'M'.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_FILEIO_SUBDIR) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write grow
@# mtools: ALL 13 GROW files are listed in ::SUB (rendered 8.3 "GROWnn   TXT").
@for n in 00 01 02 03 04 05 06 07 08 09 10 11 12; do \
	mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q "GROW$$n  *TXT" \
		|| { printf '!!! test-zs24 FAIL [GROW]: mdir ::SUB does not list GROW%s.TXT\n' "$$n"; exit 1; }; \
done
@# python: ALL 13 GROW files listed (anchored, rendered 8.3 form).
@for n in 00 01 02 03 04 05 06 07 08 09 10 11 12; do \
	python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --list-path 'SUB' | grep -q "^GROW$$n.TXT " \
		|| { printf '!!! test-zs24 FAIL [GROW]: python --list-path SUB does not list GROW%s.TXT\n' "$$n"; exit 1; }; \
done
@# Boundary file (GROW12, slot 16 -- the FIRST slot of the appended 2nd cluster)
@# reads back byte-exact via BOTH references (mtools AND python agree == written).
@mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/GROW12.TXT $(BUILD)/zs24_grow_mcopy.bin
@python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\GROW12.TXT' > $(BUILD)/zs24_grow_py.bin
@python3 -c 'import sys; e=bytes([ord("A")+12]*40); m=open("$(BUILD)/zs24_grow_mcopy.bin","rb").read(); p=open("$(BUILD)/zs24_grow_py.bin","rb").read(); sys.exit(0 if (m==e and p==e and m==p) else 1)' \
	|| { printf '!!! test-zs24 FAIL [GROW]: boundary file GROW12 read-back from the grown cluster != written (mcopy/python disagree)\n'; exit 1; }
@# Confirm the directory ACTUALLY grew to 2 clusters (the boundary file lives in
@# the appended cluster, not an incidental free slot of the first).
@python3 -c 'import importlib.util,sys; s=importlib.util.spec_from_file_location("r","$(FAT12_REF_PY)"); m=importlib.util.module_from_spec(s); s.loader.exec_module(m); fs=m.Fat12(open("$(FAT12_ZS24_IMG)","rb").read()); _,st=fs.resolve_dir("SUB"); ch=fs.walk_chain(st); sys.exit(0 if len(ch)==2 else 1)' \
	|| { printf '!!! test-zs24 FAIL [GROW]: \\SUB did not grow to 2 clusters -- fat12_grow_dir was not exercised\n'; exit 1; }
@printf '>>> test-zs24 [GROW]: GROW12 (slot 16) landed in the appended 2nd cluster; all 13 files listed; mcopy==python==written\n'
@printf ">>> test-zs24: green\n"
```

## test-zs24-exec

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27527):
Prerequisites: `build/qemu_harness build/tracer_secondary.img build/greet_program.bin harness/diff/fat_diff/fixtures/hello.txt build/.makefile.stamp`
```make
@printf '======================================================================\n'
@printf 'InitechOS (STAPLER) -- make test-zs24-exec : AH=4Bh EXEC a .COM FROM A SUBDIR\n'
@printf '  Ref: DOS 3.3 PRM AH=4Bh; ADR-0003 DEC-08; psp-loader-ground-truth.md Sec 4/5.\n'
@printf '  beads initech-zs24 (Landing 2). Inject: \\SUB\\GREET / cd sub / greet / exit.\n'
@printf '  GREET.COM lives ONLY in ::SUB -- a root-only loader/dispatch cannot find it.\n'
@printf '======================================================================\n'
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-zs24-exec FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mmd     >/dev/null 2>&1 || { printf '!!! test-zs24-exec FAIL: mtools `mmd` not found.\n'; exit 1; }
$(call zs24exec-mint-disk,$(ZS24EXEC_IMG))
@printf 'Booting   : %s + FRESH writable disk %s (GREET.COM only in ::SUB)\n' "$(SECONDARY_TRACER_IMG)" "$(ZS24EXEC_IMG)"
@printf 'Expecting : SHELL-READY + "%s" x2 + prompt "A:\\SUB>" + clean SHELL-EXIT/SHELL-DONE\n' "$(ZS24EXEC_MARKER)"
@printf '%s\n' '----------------------------------------------------------------------'
@$(call fixture-preflight,$(ZS24EXEC_IMG))
@$(HARNESS_BIN) --disk "$(SECONDARY_TRACER_IMG)" --disk2 "$(ZS24EXEC_IMG)" \
	--name "$(ZS24EXEC_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(ZS24EXEC_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(ZS24EXEC_REPORT)" || true
@cat "$(ZS24EXEC_REPORT)"
@printf '%s\n' '----------------------------------------------------------------------'
@if grep -q 'triple_fault=1' "$(ZS24EXEC_REPORT)"; then \
	printf '!!! test-zs24-exec FAIL: TRIPLE FAULT -- the subdir FAT load or control transfer crashed\n'; exit 1; \
fi
@printf '>>> test-zs24-exec [1/5]: no triple-fault\n'
@if [ ! -s "$(ZS24EXEC_SERIAL)" ]; then \
	printf '!!! test-zs24-exec FAIL: no serial captured at %s\n' "$(ZS24EXEC_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(ZS24EXEC_SERIAL)" \
	|| { printf '!!! test-zs24-exec FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@printf '>>> test-zs24-exec [2/5]: SHELL-READY (COMMAND.COM REPL entered)\n'
@# Scope to the post-SHELL-READY REPL transcript (the boot demos print earlier).
@sed -n '/^SHELL-READY$$/,$$p' "$(ZS24EXEC_SERIAL)" | tr -d '\r' > "$(BUILD)/$(ZS24EXEC_NAME).repl"
@# ---- CENTERPIECE: the GREET marker appears AT LEAST TWICE -- once for the
@# ABSOLUTE `\SUB\GREET` (root CWD) and once for the CWD-RELATIVE bare `GREET`
@# (CWD=\SUB). GREET.COM is ONLY in ::SUB, so EACH occurrence proves a subdir
@# load (a root-only path would find nothing and print "Bad command", not this). ----
@n=$$(grep -cF '$(ZS24EXEC_MARKER)' "$(BUILD)/$(ZS24EXEC_NAME).repl"); \
if [ "$$n" -lt 2 ]; then \
	printf '!!! test-zs24-exec FAIL: expected the GREET marker >=2x (absolute + CWD-relative subdir EXEC), saw %s -- a subdir .COM did not load+run (root-cause do_exec resolve / loader subdir find, Rule 3)\n' "$$n"; \
	exit 1; \
fi
@printf '>>> test-zs24-exec [3/5]: GREET.COM (in ::SUB) ran TWICE -- absolute "\\SUB\\GREET" AND CWD-relative "greet" after CD SUB\n'
@# ---- The CWD-relative leg requires CD SUB to have entered the subdir: the
@# GETCWD-composed $P$G prompt must show "A:\SUB>" (proving the bare `greet`
@# resolved against g_cwd=\SUB, not the root). ----
@grep -qF 'A:\SUB>' "$(BUILD)/$(ZS24EXEC_NAME).repl" \
	|| { printf '!!! test-zs24-exec FAIL: the prompt never showed "A:\\SUB>" -- CD SUB did not enter the subdir, so the bare `greet` did not run CWD-relative\n'; exit 1; }
@printf '>>> test-zs24-exec [4/5]: prompt showed "A:\\SUB>" -- the bare `greet` ran CWD-relative to \\SUB (CWD not corrupted by the absolute EXEC)\n'
@# ---- The cycle completes cleanly (both subdir EXECs returned to the REPL). ----
@grep -q '^SHELL-EXIT$$' "$(ZS24EXEC_SERIAL)" \
	|| { printf '!!! test-zs24-exec FAIL: SHELL-EXIT missing -- the subdir EXEC cycle did not reach EXIT cleanly\n'; exit 1; }
@grep -q '^SHELL-DONE$$' "$(ZS24EXEC_SERIAL)" \
	|| { printf '!!! test-zs24-exec FAIL: SHELL-DONE missing -- the REPL did not return to the halt loop\n'; exit 1; }
@printf '>>> test-zs24-exec [5/5]: both subdir EXECs returned to the REPL; EXIT halted cleanly (SHELL-EXIT + SHELL-DONE)\n'
@printf '%s\n' '----------------------------------------------------------------------'
@printf 'VERDICT   : PASS -- AH=4Bh EXEC loaded + ran a .COM from a SUBDIRECTORY two ways\n'
@printf '            (absolute "\\SUB\\GREET" and CWD-relative "greet" after CD SUB); root EXEC unchanged\n'
@printf '            (QEMU only; tri-emulator agreement pending beads initech-x0i)\n'
@printf '======================================================================\n'
```

## test-zs24-exec-mutant

Declared tier: EMU; #  recipe to execute (from 'Makefile', line 27672):
Prerequisites: `build/qemu_harness build/tracer_boot_mut_zs24reject.img build/tracer_boot_mut_zs24rootonly.img build/greet_program.bin harness/diff/fat_diff/fixtures/hello.txt build/.makefile.stamp`
```make
@printf ">>> test-zs24-exec-mutant: confirming BOTH subdir-EXEC mutants go RED for the RIGHT reason (Rule 6; beads initech-zs24)\n"
@# ---------------- Leg A: REJECT (do_exec restores the subdir reject) ----------------
@printf '%s\n' '---- leg A: do_exec subdir-reject mutant (-DINT21_MUTATE_EXEC_ROOTREJECT) ----'
$(call zs24exec-mint-disk,$(ZS24EXEC_REJECT_IMG))
@$(call fixture-preflight,$(ZS24EXEC_REJECT_IMG))
@$(HARNESS_BIN) --disk "$(ZS24EXEC_TRACER_REJECT_IMG)" --disk2 "$(ZS24EXEC_REJECT_IMG)" \
	--name "$(ZS24EXEC_REJECT_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(ZS24EXEC_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(ZS24EXEC_REJECT_REPORT)" || true
@if [ ! -s "$(ZS24EXEC_REJECT_SERIAL)" ]; then \
	printf '!!! test-zs24-exec-mutant FAIL (leg A): no serial captured at %s -- mutant boot is dead, RED is meaningless\n' "$(ZS24EXEC_REJECT_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(ZS24EXEC_REJECT_SERIAL)" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg A): SHELL-READY missing -- mutant never reached the REPL, RED is meaningless\n'; exit 1; }
@grep -Eq '^SHELL-EXIT$$|^SHELL-DONE$$' "$(ZS24EXEC_REJECT_SERIAL)" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg A): neither SHELL-EXIT nor SHELL-DONE -- mutant did not run the cycle, RED is meaningless\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' "$(ZS24EXEC_REJECT_SERIAL)" | tr -d '\r' > "$(BUILD)/$(ZS24EXEC_REJECT_NAME).repl"
@# Under REJECT, do_exec rejects ONLY a path that LEXICALLY carries '\'/':' --
@# so the ABSOLUTE `\SUB\GREET` is blocked ("Bad command"), but the bare `greet`
@# (no '\') after CD SUB still resolves CWD-relative and runs. The marker thus
@# appears at MOST ONCE, so the real gate's centerpiece (>=2) goes RED. Assert
@# exactly that inversion (count < 2). A full-absent assertion would WRONGLY pass
@# the gate's later steps; biting the >=2 centerpiece is the precise proof.
@n=$$(grep -cF '$(ZS24EXEC_MARKER)' "$(BUILD)/$(ZS24EXEC_REJECT_NAME).repl"); \
if [ "$$n" -ge 2 ]; then \
	printf '!!! test-zs24-exec-mutant FAIL (leg A): the subdir-reject mutant STILL ran GREET.COM %sx (>=2) -- the absolute-subdir-EXEC half of the centerpiece is decoration\n' "$$n"; \
	exit 1; \
fi
@# Confirm the absolute leg specifically failed (Bad command), so the RED is the
@# absolute subdir reject biting -- not some unrelated early abort.
@grep -qF 'Bad command or file name' "$(BUILD)/$(ZS24EXEC_REJECT_NAME).repl" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg A): no "Bad command or file name" -- the absolute `\\SUB\\GREET` did not reach the reject; the RED is for the wrong reason\n'; exit 1; }
@printf '>>> test-zs24-exec-mutant leg A: green (mutant booted + ran the cycle; absolute "\\SUB\\GREET" rejected -> marker <2x -- do_exec subdir-resolve bites the centerpiece)\n'
@# ---------------- Leg B: ROOTONLY (loader ignores dir_start) ----------------
@printf '%s\n' '---- leg B: loader root-only mutant (-DLOADER_MUTATE_EXEC_ROOTONLY) ----'
$(call zs24exec-mint-disk,$(ZS24EXEC_ROOTONLY_IMG))
@$(call fixture-preflight,$(ZS24EXEC_ROOTONLY_IMG))
@$(HARNESS_BIN) --disk "$(ZS24EXEC_TRACER_ROOTONLY_IMG)" --disk2 "$(ZS24EXEC_ROOTONLY_IMG)" \
	--name "$(ZS24EXEC_ROOTONLY_NAME)" --out "$(BUILD)" --timeout-ms 14000 \
	--keys "$(ZS24EXEC_KEYS)" --keys-after "SHELL-READY" \
	$(SHELL_OS_COMPLETION) \
	2> "$(ZS24EXEC_ROOTONLY_REPORT)" || true
@if [ ! -s "$(ZS24EXEC_ROOTONLY_SERIAL)" ]; then \
	printf '!!! test-zs24-exec-mutant FAIL (leg B): no serial captured at %s -- mutant boot is dead, RED is meaningless\n' "$(ZS24EXEC_ROOTONLY_SERIAL)"; exit 1; \
fi
@grep -q '^SHELL-READY$$' "$(ZS24EXEC_ROOTONLY_SERIAL)" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg B): SHELL-READY missing -- mutant never reached the REPL, RED is meaningless\n'; exit 1; }
@grep -Eq '^SHELL-EXIT$$|^SHELL-DONE$$' "$(ZS24EXEC_ROOTONLY_SERIAL)" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg B): neither SHELL-EXIT nor SHELL-DONE -- mutant did not run the cycle, RED is meaningless\n'; exit 1; }
@sed -n '/^SHELL-READY$$/,$$p' "$(ZS24EXEC_ROOTONLY_SERIAL)" | tr -d '\r' > "$(BUILD)/$(ZS24EXEC_ROOTONLY_NAME).repl"
@# Under ROOTONLY the loader ignores dir_start for EVERY EXEC, so BOTH the
@# absolute AND the CWD-relative subdir loads look in root, find nothing, and
@# fail -- the GREET marker is FULLY ABSENT (count 0, well below the gate's >=2).
@n=$$(grep -cF '$(ZS24EXEC_MARKER)' "$(BUILD)/$(ZS24EXEC_ROOTONLY_NAME).repl"); \
if [ "$$n" -ge 2 ]; then \
	printf '!!! test-zs24-exec-mutant FAIL (leg B): the root-only loader mutant STILL ran GREET.COM %sx (>=2) -- the loader did NOT honor dir_start yet found GREET; the subdir-find assertion is decoration\n' "$$n"; \
	exit 1; \
fi
@# Sanity: under ROOTONLY the path RESOLVE is unchanged (it is the LOADER that is
@# mutated), so CD SUB still works and the prompt "A:\SUB>" still appears -- the
@# mutant DID enter the subdir and attempt the subdir LOAD, which then failed.
@# This isolates the RED to the loader subdir-find, not a resolve/CD failure.
@grep -qF 'A:\SUB>' "$(BUILD)/$(ZS24EXEC_ROOTONLY_NAME).repl" \
	|| { printf '!!! test-zs24-exec-mutant FAIL (leg B): "A:\\SUB>" missing -- CD SUB did not run, so the mutant did not exercise the subdir LOAD path; the RED is for the wrong reason\n'; exit 1; }
@printf '>>> test-zs24-exec-mutant leg B: green (mutant booted + entered \\SUB; the GREET marker correctly ABSENT (0x) -- loader subdir-find bites)\n'
@printf '>>> test-zs24-exec-mutant: green (BOTH legs RED for the right reason; the oracle bites on do_exec resolve AND loader subdir-find)\n'
```

## test-zs24-mutant

Declared tier: HOST; #  recipe to execute (from 'Makefile', line 6513):
Prerequisites: `build/test_fileio_subdir_mut_rootslot build/test_fileio_subdir_mut_createroot build/test_fileio_subdir_mut_unlinknoop build/test_fileio_subdir_mut_grownoeoc build/test_fileio_subdir_mut_grownoop build/fat12_nested.img harness/diff/fat_diff/fat12_ref.py build/.makefile.stamp`
```make
@printf ">>> test-zs24-mutant: confirming all FIVE subdir-WRITE mutants go RED for the RIGHT reason (Rule 6; beads initech-zs24)\n"
@# Each mutant leg first CONFIRMS the harness actually BOOTED/RAN (it emits a
@# "test_fileio_subdir: N checks, M failures" summary line) before trusting the
@# RED -- a crash before the side-effect would otherwise false-green (Fix 3,
@# mirroring the test-ut6d-mutant boot-confirmation discipline). The harness
@# stdout is captured to a report; a missing/empty report is a HARD FAIL.
@# ROOTSLOT: a subdir CREATE+WRITE write-backs to a ROOT slot -> mcopy read-back is wrong/absent.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_ZS24_MUT_ROOTSLOT) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write create-write > $(BUILD)/zs24_mut_rootslot.report 2>&1 || true
@if [ ! -s $(BUILD)/zs24_mut_rootslot.report ]; then \
	printf '!!! test-zs24-mutant FAIL: ROOTSLOT mutant produced no output -- harness is dead, RED is meaningless\n'; exit 1; fi
@grep -q '^test_fileio_subdir: ' $(BUILD)/zs24_mut_rootslot.report \
	|| { printf '!!! test-zs24-mutant FAIL: ROOTSLOT mutant never reached the TEST_SUMMARY -- it crashed before the create, RED is meaningless\n'; exit 1; }
@if mcopy -n -i $(FAT12_ZS24_IMG) ::SUB/NEW.TXT $(BUILD)/zs24_mut.bin >/dev/null 2>&1 && \
	python3 -c 'import sys; exp=bytes(65+(i%26) for i in range(700)); sys.exit(0 if open("$(BUILD)/zs24_mut.bin","rb").read()==exp else 1)' >/dev/null 2>&1; then \
	printf '!!! test-zs24-mutant FAIL: ROOTSLOT mutant PASSED -- the subdir-WRITE write-back diff is decoration\n'; exit 1; \
else \
	printf '>>> test-zs24-mutant: green (ROOTSLOT mutant ran + correctly RED -- subdir write-back bites)\n'; \
fi
@# CREATEROOT: the kept root-only CREATE guard rejects the subdir CREATE (the
@# backend maps it to 0x0003 PATH_NOT_FOUND -> CF set -> creat_write_close
@# returns -1 -> the harness records a FAILURE on the create CHECK). We assert
@# BOTH the rejection was SURFACED (the harness reported >=1 failure -- it RAN
@# the create and the guard rejected it, not merely that the file is absent)
@# AND the file did not land in \SUB (no exit-code swallow that could mask a crash).
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_ZS24_MUT_CREATEROOT) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write create-write > $(BUILD)/zs24_mut_createroot.report 2>&1; rc=$$?; \
	if [ ! -s $(BUILD)/zs24_mut_createroot.report ]; then \
		printf '!!! test-zs24-mutant FAIL: CREATEROOT mutant produced no output -- harness is dead, RED is meaningless\n'; exit 1; fi
@grep -q '^test_fileio_subdir: ' $(BUILD)/zs24_mut_createroot.report \
	|| { printf '!!! test-zs24-mutant FAIL: CREATEROOT mutant never reached the TEST_SUMMARY -- it crashed before the create, RED is meaningless\n'; exit 1; }
@# The CREATE was REJECTED by the kept root-only guard: the harness recorded a
@# non-zero failure count (the create CHECK bit), proving the subdir CREATE was
@# surfaced as a CF/0x0003 rejection rather than silently skipped.
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/zs24_mut_createroot.report \
	|| { printf '!!! test-zs24-mutant FAIL: CREATEROOT mutant did NOT surface a CREATE rejection -- the harness reported 0 failures, so the root-only guard did not bite (RED for the wrong reason)\n'; exit 1; }
@if mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'NEW *TXT'; then \
	printf '!!! test-zs24-mutant FAIL: CREATEROOT mutant PASSED -- the subdir CREATE gate is decoration (file landed despite the guard)\n'; exit 1; \
else \
	printf '>>> test-zs24-mutant: green (CREATEROOT mutant ran, the CREATE was rejected + the file is absent -- subdir CREATE bites)\n'; \
fi
@# UNLINKNOOP: a subdir UNLINK is a no-op -> mdir ::SUB still lists the file.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_ZS24_MUT_UNLINKNOOP) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write create-write > $(BUILD)/zs24_mut_unlinknoop.report 2>&1 || true
@$(TEST_ZS24_MUT_UNLINKNOOP) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write unlink >> $(BUILD)/zs24_mut_unlinknoop.report 2>&1 || true
@if [ ! -s $(BUILD)/zs24_mut_unlinknoop.report ]; then \
	printf '!!! test-zs24-mutant FAIL: UNLINKNOOP mutant produced no output -- harness is dead, RED is meaningless\n'; exit 1; fi
@grep -q '^test_fileio_subdir: ' $(BUILD)/zs24_mut_unlinknoop.report \
	|| { printf '!!! test-zs24-mutant FAIL: UNLINKNOOP mutant never reached the TEST_SUMMARY -- it crashed, RED is meaningless\n'; exit 1; }
@if mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'NEW *TXT'; then \
	printf '>>> test-zs24-mutant: green (UNLINKNOOP mutant ran + correctly RED -- subdir UNLINK bites)\n'; \
else \
	printf '!!! test-zs24-mutant FAIL: UNLINKNOOP mutant PASSED -- the subdir UNLINK oracle is decoration\n'; exit 1; \
fi
@# GROW_NO_EOC: fat12_grow_dir skips the new-cluster EOC mark, so the appended
@# 2nd cluster is unreachable / the chain is broken at the join -> the boundary
@# file (GROW12, slot 16) is UNREADABLE via mtools/python. The harness still
@# RUNS (the create succeeds at the artifact level; the corruption is on disk),
@# so we confirm it reached TEST_SUMMARY, then assert GROW12 is unreadable. This
@# proves the chain RELINK + EOC-terminate is what makes the grown slots reachable.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_ZS24_MUT_GROWNOEOC) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write grow > $(BUILD)/zs24_mut_grownoeoc.report 2>&1 || true
@if [ ! -s $(BUILD)/zs24_mut_grownoeoc.report ]; then \
	printf '!!! test-zs24-mutant FAIL: GROW_NO_EOC mutant produced no output -- harness is dead, RED is meaningless\n'; exit 1; fi
@grep -q '^test_fileio_subdir: ' $(BUILD)/zs24_mut_grownoeoc.report \
	|| { printf '!!! test-zs24-mutant FAIL: GROW_NO_EOC mutant never reached the TEST_SUMMARY -- it crashed before the grow, RED is meaningless\n'; exit 1; }
@if python3 $(FAT12_REF_PY) $(FAT12_ZS24_IMG) --cat-path 'SUB\GROW12.TXT' > $(BUILD)/zs24_mut_grow.bin 2>/dev/null && \
	python3 -c 'import sys; e=bytes([ord("A")+12]*40); sys.exit(0 if open("$(BUILD)/zs24_mut_grow.bin","rb").read()==e else 1)' >/dev/null 2>&1; then \
	printf '!!! test-zs24-mutant FAIL: GROW_NO_EOC mutant PASSED -- GROW12 read back correct despite the skipped EOC; the grow chain-relink/EOC oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-zs24-mutant: green (GROW_NO_EOC mutant ran + correctly RED -- the boundary file is unreachable; grow chain-relink/EOC bites)\n'; \
fi
@# GROW_NOOP: fat12_grow_dir refuses to grow (DIR_FULL), so the boundary CREATE
@# fails outright -> the harness records a failure AND GROW12 never lands.
@$(call fixture-preflight,$(FAT12_NESTED_IMG))
@cp $(FAT12_NESTED_IMG) $(FAT12_ZS24_IMG)
@$(if $(call fixture-spec,$(FAT12_NESTED_IMG)),$(call fixture-check,harness,$(FAT12_ZS24_IMG),$(call fixture-spec,$(FAT12_NESTED_IMG))),:)
@$(TEST_ZS24_MUT_GROWNOOP) "$(FAT12_ZS24_IMG)" "$(FAT12_FIXTURE_DIR)" --write grow > $(BUILD)/zs24_mut_grownoop.report 2>&1 || true
@if [ ! -s $(BUILD)/zs24_mut_grownoop.report ]; then \
	printf '!!! test-zs24-mutant FAIL: GROW_NOOP mutant produced no output -- harness is dead, RED is meaningless\n'; exit 1; fi
@grep -q '^test_fileio_subdir: ' $(BUILD)/zs24_mut_grownoop.report \
	|| { printf '!!! test-zs24-mutant FAIL: GROW_NOOP mutant never reached the TEST_SUMMARY -- it crashed before the grow, RED is meaningless\n'; exit 1; }
@# The boundary CREATE was rejected (grow refused) -> the harness recorded a failure.
@grep -Eq 'test_fileio_subdir: [0-9]+ checks, [1-9][0-9]* failures' $(BUILD)/zs24_mut_grownoop.report \
	|| { printf '!!! test-zs24-mutant FAIL: GROW_NOOP mutant did NOT surface a CREATE failure -- the boundary create was not rejected (RED for the wrong reason)\n'; exit 1; }
@if mdir -i $(FAT12_ZS24_IMG) ::SUB 2>/dev/null | grep -q 'GROW12  *TXT'; then \
	printf '!!! test-zs24-mutant FAIL: GROW_NOOP mutant PASSED -- GROW12 landed despite the refused grow; the grow oracle is decoration\n'; exit 1; \
else \
	printf '>>> test-zs24-mutant: green (GROW_NOOP mutant ran, the boundary CREATE was rejected + GROW12 is absent -- grow bites)\n'; \
fi
@printf '>>> test-zs24-mutant: green (ALL FIVE mutants ran + RED for the right reason -- write-back, CREATE gate, UNLINK, grow relink/EOC, grow-refuse all bite)\n'
```

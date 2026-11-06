# ADR-0009 DEC-03a.2-.8; initech-jarzu. Hand-authored v1, no emitter dependency.
$(BUILD)/exec_high.exe: os/milton/exec_high_fixture.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/exec_high_big.exe: os/milton/exec_high_fixture.asm | $(BUILD)
	$(NASM) -f bin -DHIGH_OVERSIZE $< -o $@
$(BUILD)/exec_high_low.com: os/milton/exec_high_low.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/exec_high_legacy.bin: os/milton/mzexec_fixture.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/exec_high_legacy.exe: $(BUILD)/exec_high_legacy.bin $(MZLINK_BIN)
	$(MZLINK_BIN) $< $@ --reloc 1
$(BUILD)/fat_exec_high.img: $(BUILD)/fat_tsr_exec.img $(BUILD)/exec_high_legacy.exe $(BUILD)/exec_high.exe $(BUILD)/exec_high_big.exe $(BUILD)/exec_high_low.com
	cp -f $< $@
	mcopy -o -i $@ $(BUILD)/exec_high_legacy.exe ::MZEXEC.EXE
	mcopy -o -i $@ $(BUILD)/exec_high.exe ::HIGH.EXE
	mcopy -o -i $@ $(BUILD)/exec_high_big.exe ::BIG.EXE
	mcopy -o -i $@ $(BUILD)/exec_high_low.com ::LOW.COM
EXEC_HIGH_KEYS := kh,ki,kg,kh,kdot,ke,kx,ke,kret,kh,ki,kg,kh,kdot,ke,kx,ke,kret,kb,ki,kg,kdot,ke,kx,ke,kret,km,kz,ke,kx,ke,kc,kret,ke,kx,ki,kt,kret
# Local profiles only. General emulator-profile work belongs to initech-r2lqd.
EXEC_HIGH_RAM_MB ?= 8
EXEC_HIGH_LOW_RAM_MB ?= 5
EXEC_HIGH_HOLE_RAM_MB ?= 16
define exec-high-boot
	@cp -f $(BUILD)/fat_exec_high.img $(BUILD)/$(2)_data.img
	@INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=$(EXEC_HIGH_RAM_MB) $(HARNESS_BIN) --disk $(1) --disk2 $(BUILD)/$(2)_data.img --name $(2) --out $(BUILD) --mouse '$(EXEC_HIGH_KEYS)' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 45000 2>$(BUILD)/$(2).report || true
endef
.PHONY: test-exec-high test-exec-high-mutant test-exec-high-bochs test-exec-high-lowmem
test-exec-high: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/fat_exec_high.img
	$(call exec-high-boot,$(SECONDARY_TRACER_IMG),exec_high)
	@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high
	@printf '>>> test-exec-high: green (8 MiB, high fixup/entry/heap/stack, KEEP and second-high refusals, conventional child, reclaim, legacy low, oversize)\n'
test-exec-high-lowmem: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@INITECH_TEST_RAM_MB=$(EXEC_HIGH_LOW_RAM_MB) $(HARNESS_BIN) --disk $(SECONDARY_TRACER_IMG) --name exec_high_lowmem --out $(BUILD) --quit-after HALTED --timeout-ms 10000 2>$(BUILD)/exec_high_lowmem.report || true
	@grep -q 'PC LOAD LETTER.*Insufficient memory' $(BUILD)/exec_high_lowmem.serial || { printf '!!! test-exec-high-lowmem RED: missing MSG-DOS-0035 before high copy\n'; exit 1; }
	@grep -q 'ext_mem_kb=4096 required_kb=7168' $(BUILD)/exec_high_lowmem.serial
	@! grep -q 'FLAIR-HEAP-OK\|SHELL-READY' $(BUILD)/exec_high_lowmem.serial
	@printf '>>> test-exec-high-lowmem: green (5 MiB refuses with MSG-DOS-0035 before desktop/high writes)\n'
TEST_EMU_GATES += test-exec-high test-exec-high-mutant test-exec-high-lowmem
# DEC-03a.8: high-model acceptance belongs to the SAMIR merge vector too.
MERGE_SAMIR_CANDIDATES += test-exec-high test-exec-high-mutant test-exec-high-lowmem
$(BUILD)/test_native: os/milton/test_native.c os/milton/native.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: test-native
test-native: $(BUILD)/test_native
	@$<
TEST_UNIT_GATES += test-native
# Rule 6: perturb production dispatch, owner refusal and teardown separately.
$(eval $(call tsr-exec-mutant,high_low,LOADER_MUT_HIGH_LOW))
$(eval $(call tsr-exec-mutant,high_owner,LOADER_MUT_HIGH_OWNER))
$(eval $(call tsr-exec-mutant,high_reclaim,LOADER_MUT_HIGH_RECLAIM))
test-exec-high-mutant: test-exec-high $(BUILD)/tsr_exec_high_low.img $(BUILD)/tsr_exec_high_owner.img $(BUILD)/tsr_exec_high_reclaim.img
	$(call exec-high-boot,$(BUILD)/tsr_exec_high_low.img,exec_high_mut_low)
	$(call exec-high-boot,$(BUILD)/tsr_exec_high_owner.img,exec_high_mut_owner)
	$(call exec-high-boot,$(BUILD)/tsr_exec_high_reclaim.img,exec_high_mut_reclaim)
	@set -eu; for m in low owner reclaim; do s=$(BUILD)/exec_high_mut_$$m; \
	 if sh harness/emu/exec_high_check.sh $$s > $$s.check 2>&1; then printf '!!! high mutant %s survived\n' $$m; exit 1; fi; \
	 grep -q '^SHELL-READY' $$s.serial; grep '^!!! test-exec-high RED:' $$s.check; \
	 printf '>>> test-exec-high-mutant: %s RED after successful boot\n' $$m; done
	@printf '>>> test-exec-high-mutant: green (dispatch low, no single-owner refusal, skipped reclaim all RED)\n'
EXEC_HIGH_BOCHS_KEYS := h,i,g,h,dot,e,x,e,ret,h,i,g,h,dot,e,x,e,ret,b,i,g,dot,e,x,e,ret,m,z,e,x,e,c,ret,e,x,i,t,ret
test-exec-high-bochs: $(BOCHS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/fat_exec_high.img
	@cp -f $(BUILD)/fat_exec_high.img $(BUILD)/exec_high_bochs_data.img
	@INITECH_TEST_RAM_MB=$(EXEC_HIGH_RAM_MB) $(BOCHS_BIN) --disk $(SECONDARY_TRACER_IMG) --disk2 $(BUILD)/exec_high_bochs_data.img --name exec_high_bochs --out $(BUILD) --keys '$(EXEC_HIGH_BOCHS_KEYS)' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 60000 2>$(BUILD)/exec_high_bochs.report || true
	@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_bochs
	@printf '>>> test-exec-high-bochs: green (8 MiB; same high allocation/return oracle as QEMU)\n'
# DEC-03a.6: exact reported-memory boundary and known-hole refusal. Physical
# RAM is 8 MiB; only these explicitly named IO.SYS fixtures inject reports.
define exec-high-ram-fixture
$(BUILD)/exec_high_ram_$(1).sys: os/boot/io_sys.asm $(DOS_MESSAGES_INC) | $(BUILD)
	$(NASM) -f bin -I$(BUILD)/ -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) $(2) $$< -o $$@
$(BUILD)/exec_high_ram_$(1).img: $(BUILD)/exec_high_ram_$(1).sys $(SECONDARY_TRACER_IMG) $(BOOT_SYSTEM_MINT) $(BOOT_SYSTEM_TOOL)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $$< $(BUILD)/kernel_shell_secondary.bin $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call exec-high-ram-fixture,below,-DDEC03A_REPORT_KB=7167))
$(eval $(call exec-high-ram-fixture,exact,-DDEC03A_REPORT_KB=7168))
$(eval $(call exec-high-ram-fixture,hole,-DDEC03A_KNOWN_HOLE))
$(eval $(call exec-high-ram-fixture,lowhole,-DDEC03A_LOW_HOLE))
.PHONY: test-exec-high-ram
test-exec-high-ram: $(HARNESS_BIN) $(BUILD)/exec_high_ram_below.img $(BUILD)/exec_high_ram_exact.img $(BUILD)/exec_high_ram_hole.img $(BUILD)/exec_high_ram_lowhole.img
	@set -eu; for m in below hole lowhole; do ram=8; test $$m != hole || ram=$(EXEC_HIGH_HOLE_RAM_MB); \
	 INITECH_TEST_RAM_MB=$$ram $(HARNESS_BIN) --disk $(BUILD)/exec_high_ram_$$m.img --name exec_high_ram_$$m --out $(BUILD) --quit-after HALTED --timeout-ms 5000 2>$(BUILD)/exec_high_ram_$$m.report || true; \
	 grep -q 'PC LOAD LETTER.*Insufficient memory' $(BUILD)/exec_high_ram_$$m.serial; \
	 ! grep -q '^KHI\|^FLAIR-HEAP-OK' $(BUILD)/exec_high_ram_$$m.serial; done
	@grep -q 'ext_mem_kb=7167 required_kb=7168' $(BUILD)/exec_high_ram_below.serial
	@INITECH_TEST_RAM_MB=8 $(HARNESS_BIN) --disk $(BUILD)/exec_high_ram_exact.img --name exec_high_ram_exact --out $(BUILD) --expect SHELL-DONE --keys e,x,i,t,ret --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 5000 2>$(BUILD)/exec_high_ram_exact.report
	@grep -q '^SHELL-READY' $(BUILD)/exec_high_ram_exact.serial
	@printf '>>> test-exec-high-ram: green (7167 RED, 7168 GREEN, known E820 hole RED before copy to high kernel)\n'
TEST_EMU_GATES += test-exec-high-ram
MERGE_SAMIR_CANDIDATES += test-exec-high-ram
# Unit counterparts to the full-width and owner mutation vectors.
$(BUILD)/test_native_count16: os/milton/test_native.c os/milton/native.c | $(BUILD)
	$(CC) $(CFLAGS) -DNATIVE_MUT_COUNT16 -o $@ $<
$(BUILD)/test_native_owner: os/milton/test_native.c os/milton/native.c | $(BUILD)
	$(CC) $(CFLAGS) -DNATIVE_MUT_OWNER -o $@ $<
.PHONY: test-native-mutant
test-native-mutant: test-native $(BUILD)/test_native_count16 $(BUILD)/test_native_owner
	@set -eu; for m in count16 owner; do \
	 if $(BUILD)/test_native_$$m >$(BUILD)/native_mut_$$m.log 2>&1; then echo '!!! native mutant survived'; exit 1; fi; \
	 grep '^native RED' $(BUILD)/native_mut_$$m.log; done
	@printf '>>> test-native-mutant: green (truncated count and foreign owner RED)\n'
TEST_UNIT_GATES += test-native-mutant
# Same-TU cold/native sources and ABI headers are real dependencies.
$(TEST_MZLOAD) $(TEST_MZLOAD_MUT) $(TEST_LOADER) $(TEST_LOADER_MUT) $(TEST_LOADER_MUT_ENV) $(TEST_LOADER_BIG) $(TEST_LOADER_BIG_MUT) $(TEST_ARENADJ) $(TEST_ARENADJ_MUT) $(KERNEL_LOADER_OBJ) $(KERNEL_TENANT_LOADER_OBJ): os/milton/loader_cold.c os/milton/loader_cold.h spec/initechmz_extended.h
$(TEST_ARENADJ) $(TEST_ARENADJ_MUT) $(KERNEL_INT21_OBJ): os/milton/native.c
# DEC-03a.5 / Rule 14: launch, LIST real data, quit and show the preserved
# desktop using the established record-flair lockstep/repro machinery.
RECORD_SCRIPTS += samir_high
RECORD_SPEC_samir_high = w300,kctrl-alt-bsl,ku,ks,ke,kspc,kc,kl,ki,ke,kn,kt,ks,kdot,kd,kb,kf,kret,kl,ki,ks,kt,kret,w300,kq,ku,ki,kt,kret,WFLAIR-RESUME,w300
RECORD_MARKER_samir_high = FLAIR-RESUME
RECORD_IMAGE_samir_high = $(FLAIRTENANTS_RECORDDBL_IMG)
RECORD_DATA_samir_high = $(SAMIR_LIST_IMG)
RECORD_SETTLE_samir_high = 200
ifeq ($(SCRIPT),samir_high)
record-flair: $(SAMIR_LIST_IMG)
endif
$(BUILD)/test_mz_ext_length_mutant: os/milton/test_mz.c os/milton/mz.c os/milton/mz.h spec/initechmz_extended.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DMZ_MUT_EXT_LENGTH -Ispec -Ios/milton -Iseed -o $@ $<
.PHONY: test-mz-extended-mutant
test-mz-extended-mutant: $(BUILD)/test_mz_ext_length_mutant
	@if $< >$(BUILD)/mz_ext_length_mutant.log 2>&1; then echo '!!! descriptor length mutant survived'; exit 1; fi
	@grep 'FAIL.*DEC-03a: unknown version/length/stack/guard refused' $(BUILD)/mz_ext_length_mutant.log
	@printf '>>> test-mz-extended-mutant: green (unchecked descriptor length RED)\n'
TEST_UNIT_GATES += test-mz-extended-mutant
# DEC-03a.5: desktop hotkey -> resume -> Exit to InitechDOS -> high prompt
# launch twice -> legacy MZ. The final marker requires both high teardowns.
SAMIR_HIGH_RETURN_KEYS = kctrl-alt-bsl,kq,ku,ki,kt,kret,WFLAIR-RESUME,$(FLAIR_EXIT_DOS_SPEC),ks,ka,km,ki,kr,kret,kq,ku,ki,kt,kret,ks,ka,km,ki,kr,kret,kq,ku,ki,kt,kret,km,kz,ke,kx,ke,kc,kret,WMZEXEC-OK
.PHONY: test-flair-samir-high-return
test-flair-samir-high-return: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(SAMIR_LIST_IMG) $(BUILD)/exec_high_legacy.exe
	@cp -f $(SAMIR_LIST_IMG) $(BUILD)/samir_high_return_data.img
	@mcopy -o -i $(BUILD)/samir_high_return_data.img $(BUILD)/exec_high_legacy.exe ::MZEXEC.EXE
	@printf '@echo off\r\n' >$(BUILD)/samir_high_return_autoexec.bat
	@mcopy -o -i $(BUILD)/samir_high_return_data.img $(BUILD)/samir_high_return_autoexec.bat ::AUTOEXEC.BAT
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(FLAIRTENANTS_IMG) --disk2 $(BUILD)/samir_high_return_data.img --name samir_high_return --out $(BUILD) --mouse '$(SAMIR_HIGH_RETURN_KEYS)' --keys-after FLAIR-LIVE-READY --quit-after MZEXEC-OK --timeout-ms 60000 2>$(BUILD)/samir_high_return.report || true
	@tr -d '\r' <$(BUILD)/samir_high_return.serial >$(BUILD)/samir_high_return.repl
	@! grep -q '^PANIC' $(BUILD)/samir_high_return.repl
	@grep -q 'triple_fault=0' $(BUILD)/samir_high_return.report
	@grep -qx 'FLAIR-RESUME' $(BUILD)/samir_high_return.repl
	@grep -qx 'FLAIR-EXIT-TO-DOS' $(BUILD)/samir_high_return.repl
	@test $$(grep -c '^\. quit$$' $(BUILD)/samir_high_return.repl) -ge 3
	@grep -qx 'MZEXEC-OK' $(BUILD)/samir_high_return.repl
	@grep -q 'quit_after_found=1' $(BUILD)/samir_high_return.report
	@printf '>>> test-flair-samir-high-return: green (hotkey/resume, Exit to InitechDOS, two high prompt launches/reclaims, legacy MZ return)\n'
TEST_EMU_GATES += test-flair-samir-high-return
# DEC-03a.2/.6: independent locked-map mutations must fail at compile time.
.PHONY: test-memory-map-high-mutant
test-memory-map-high-mutant:
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
test-memory-map-mutant: test-memory-map-high-mutant
TEST_UNIT_GATES += test-memory-map-high-mutant
# DEC-03a.8 KEEP: explicit legacy fallback profile; all existing Bochs
# ENOMODE/VGA13/guard assertions stay. No production video rule changes.
$(BUILD)/samir_suspend_bochs_fallback.sys: os/boot/io_sys.asm $(DOS_MESSAGES_INC) | $(BUILD)
	$(NASM) -f bin -I$(BUILD)/ -DVBE_MUT_NO_101 -DMK_KERNEL_SECTORS=$(KERNEL_SECTORS) $< -o $@
$(BUILD)/samir_suspend_bochs_fallback.img: $(BUILD)/samir_suspend_bochs_fallback.sys $(FLAIRTENANTS_IMG)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $< $(BUILD)/flairovl_kernel_flairtenants.sys $(BOOT_SYSTEM_TOOL) $(BUILD)/transient_flairtenants.bin $(BUILD)/flairovl_kernel_flairtenants.ovl
test-flair-samir-suspend-bochs: $(BUILD)/samir_suspend_bochs_fallback.img
# DEC-03a.3/.5: deterministic fault/dirty-RAM fixtures, same production cleanup
# and serial oracle. No production flag or artifact behavior changes.
$(eval $(call tsr-exec-mutant,high_io,LOADER_TEST_HIGH_IO_FAIL))
$(eval $(call tsr-exec-mutant,high_dirty,LOADER_TEST_HIGH_DIRTY))
.PHONY: test-exec-high-io test-exec-high-dirty
test-exec-high-io: EXEC_HIGH_KEYS := kh,ki,kg,kh,kdot,ke,kx,ke,kret,kh,ki,kg,kh,kdot,ke,kx,ke,kret,kh,ki,kg,kh,kdot,ke,kx,ke,kret,kb,ki,kg,kdot,ke,kx,ke,kret,km,kz,ke,kx,ke,kc,kret,ke,kx,ki,kt,kret
test-exec-high-io: $(HARNESS_BIN) $(BUILD)/tsr_exec_high_io.img $(BUILD)/fat_exec_high.img
	$(call exec-high-boot,$(BUILD)/tsr_exec_high_io.img,exec_high_io)
	@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_io
	@printf '>>> test-exec-high-io: green (one failed read after high writes never transfers; subsequent two launches reclaim low/native resources)\n'
test-exec-high-dirty: $(HARNESS_BIN) $(BUILD)/tsr_exec_high_dirty.img $(BUILD)/fat_exec_high.img
	$(call exec-high-boot,$(BUILD)/tsr_exec_high_dirty.img,exec_high_dirty)
	@sh harness/emu/exec_high_check.sh $(BUILD)/exec_high_dirty
	@printf '>>> test-exec-high-dirty: green (A5/5A dirty high RAM zeroed before load; heap/reserve/stack and guard verified on both launches)\n'
TEST_EMU_GATES += test-exec-high-io test-exec-high-dirty
MERGE_SAMIR_CANDIDATES += test-exec-high-io test-exec-high-dirty
$(MZLINK_BIN): spec/initechmz_extended.h spec/samir/memory_contract.h
# DEC-03a.8: new acceptance entries stay in the merge vector; Bochs is an
# explicitly reported per-session omission through the existing suffix rule.
MERGE_SAMIR_CANDIDATES += test-native test-native-mutant test-mz-extended-mutant test-memory-map-high-mutant test-exec-high-bochs
$(BUILD)/exec_high_exit00.exe: os/milton/exec_high_fixture.asm | $(BUILD)
	$(NASM) -f bin -DHIGH_EXIT_00 $< -o $@
$(BUILD)/exec_high_exit20.exe: os/milton/exec_high_fixture.asm | $(BUILD)
	$(NASM) -f bin -DHIGH_EXIT_20 $< -o $@
# DEC-03a.3/.5: alternate termination services and content-based filename alias.
EXEC_HIGH_ALIAS_KEYS := kh,ki,kg,kh,kdot,kc,ko,km,kret,kh,ki,kg,kh,kdot,kc,ko,km,kret,kb,ki,kg,kdot,ke,kx,ke,kret,km,kz,ke,kx,ke,kc,kret,ke,kx,ki,kt,kret
.PHONY: test-exec-high-exits
test-exec-high-exits: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/fat_exec_high.img $(BUILD)/exec_high_exit00.exe $(BUILD)/exec_high_exit20.exe
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
TEST_EMU_GATES += test-exec-high-exits
MERGE_SAMIR_CANDIDATES += test-exec-high-exits
# DEC-03a.5, Rule 6: exact reference output alone cannot catch a C-stack
# overflow. Reintroduce the old WITH workspace and require the exit guard RED.
$(BUILD)/SAMIR_STACK_ARGS.EXE: $(SAMIR_HEADERS) $(SAMIR_EXE_CSRCS) $(SAMIR_CRT0_ASM) $(SAMIR_LD_SCRIPT) $(SAMIR_PACK_DEPS) | $(BUILD)
	@mkdir -p $(BUILD)/samir_stack_args
	@$(NASM) -f elf32 $(SAMIR_CRT0_ASM) -o $(BUILD)/samir_stack_args/crt0.o
	@set -e; for src in $(SAMIR_EXE_CSRCS); do obj=$(BUILD)/samir_stack_args/$$(echo $$src | tr / _ | sed 's/\.c$$/.o/'); $(CC) $(SAMIR_EXE_PROFILE) -DPROC_MUTATE_STACK_ARGS -c $$src -o $$obj; done
	@$(SAMIR_LINK) -o $(BUILD)/samir_stack_args/SAMIR.elf $(BUILD)/samir_stack_args/crt0.o $$(ls $(BUILD)/samir_stack_args/*.o | grep -v '/crt0.o')
	@$(call samir_pack,$(BUILD)/samir_stack_args/SAMIR.elf,$@)
.PHONY: test-samir-high-stack test-samir-high-stack-mutant
test-samir-high-stack: $(SAMIR_EXE) $(SAMIR_OVL) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/emu/samir_high_stack.sh $(BUILD) $(SAMIR_EXE) clean
test-samir-high-stack-mutant: test-samir-high-stack $(BUILD)/SAMIR_STACK_ARGS.EXE
	@sh harness/emu/samir_high_stack.sh $(BUILD) $(BUILD)/SAMIR_STACK_ARGS.EXE mutant
TEST_EMU_GATES += test-samir-high-stack test-samir-high-stack-mutant

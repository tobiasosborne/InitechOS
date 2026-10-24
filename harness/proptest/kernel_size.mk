# initech-tode: tests precede the kernel-only GC policy.
# Ref: PRD Sec 5/6.1/8; round2 size-policy Sec 4 lever C; Rule 6/11.
KGC_DIR := $(BUILD)/kernel_gc
KGC_SRC := harness/proptest/kernel_gc_fixture.c

$(KGC_DIR): | $(BUILD)
	@mkdir -p $@

$(KGC_DIR)/probe.o: $(KGC_SRC) | $(KGC_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Os -c $< -o $@
$(KGC_DIR)/probe.elf: $(KGC_DIR)/probe.o $(KERNEL_LD)
	$(LD) $(KERNEL_GC_LDFLAGS) -m elf_i386 -T $(KERNEL_LD) -o $@ $<

# Grade symbol reachability independently of the compiler/linker flags.
define kernel-gc-grade
symbols=$(KGC_DIR)/$(notdir $(1)).symbols; \
nm --defined-only $(1) > "$$symbols"; \
for s in _start gc_live_function gc_live_data; do \
	grep -q " $$s$$" "$$symbols" || { printf '!!! kernel-gc FAIL: live %s removed\n' "$$s"; exit 1; }; \
done; \
for s in gc_dead_function gc_dead_data; do \
	if grep -q " $$s$$" "$$symbols"; then printf '!!! kernel-gc FAIL: dead %s retained\n' "$$s"; exit 1; fi; \
done; \
grep -q '^00500000 .* _start$$' "$$symbols" || { printf '!!! kernel-gc FAIL: entry moved from KERNEL_BASE\n'; exit 1; }
endef

.PHONY: test-kernel-gc test-kernel-gc-mutant test-kernel-gc-emu
test-kernel-gc: $(KGC_DIR)/probe.elf test-kernel-gc-lto
	@$(call kernel-gc-grade,$<)
	@printf '>>> test-kernel-gc: green (entry/live code/data retained; dead code/data removed)\n'

test-kernel-gc-mutant: test-kernel-gc
	@set -eu; for mutant in no_gc no_sections; do \
		case $$mutant in \
			no_gc) flags='$(KERNEL_SECTION_FLAGS)'; ldflags=;; \
			no_sections) flags=; ldflags='$(KERNEL_GC_LDFLAGS)';; \
		esac; \
		$(KERNEL_CC) $(KERNEL_CFLAGS) $$flags -Os -c $(KGC_SRC) -o $(KGC_DIR)/$$mutant.o; \
		$(LD) $$ldflags -m elf_i386 -T $(KERNEL_LD) -o $(KGC_DIR)/$$mutant.elf $(KGC_DIR)/$$mutant.o 2>$(KGC_DIR)/$$mutant.link; \
		if ( $(call kernel-gc-grade,$(KGC_DIR)/$$mutant.elf) ) >$(KGC_DIR)/$$mutant.log 2>&1; then \
			printf '!!! test-kernel-gc-mutant FAIL: %s survived\n' "$$mutant"; exit 1; \
		fi; \
		grep -q 'kernel-gc FAIL: dead gc_dead_function retained' $(KGC_DIR)/$$mutant.log; \
		printf '    %s: ' "$$mutant"; cat $(KGC_DIR)/$$mutant.log; \
	done
	@printf '>>> test-kernel-gc-mutant: green (no_gc/no_sections both RED for dead code)\n'

# Existing independently graded, injected QEMU traces exercise the real entry,
# IRQ/INT gates, FCB/FAT results (mtools on identical images), desktop pixels,
# and tenant calls. No new DOS behaviour or new reference output is invented.
# Ref: ADR-0003 Appendix A; IBM DOS 3.30 Technical Reference 6-200 (PDF 323,
# inspected scan: EXIT closes files/returns to parent); fcb_tps.mk goldens.
test-kernel-gc-emu:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-boot test-flair-appswitch test-fcb-api
	@printf '>>> test-kernel-gc-emu: green (boot/desktop pixels/FCB transcript and FAT differential)\n'

TEST_UNIT_GATES += test-kernel-gc test-kernel-gc-mutant
TEST_EMU_GATES += test-kernel-gc-emu

.PHONY: test-kernel-gc-roots test-kernel-gc-emu-mutant test-kernel-flair-repro
test-kernel-gc-roots: $(KERNEL_FLAIRTENANTS_ELF) | $(KGC_DIR)
	@nm --defined-only $< > $(KGC_DIR)/roots
	@set -eu; for s in _start kernel_main g_idt g_idt_ptr g_exc_stubs \
		int20_entry int21_entry int22_entry int23_entry int24_entry int25_entry int26_entry \
		irq0_entry irq1_entry irq12_entry isr_spurious isr_irq7_spurious isr_irq15_spurious \
		isr_selftest tbx_gate_entry $$(seq 0 31 | sed 's/^/isr/'); do \
		grep -q " $$s$$" $(KGC_DIR)/roots || { printf '!!! kernel-gc-roots FAIL: %s removed\n' "$$s"; exit 1; }; \
	done
	@grep -q '^00500000 .* _start$$' $(KGC_DIR)/roots
	@printf '>>> test-kernel-gc-roots: green (entry, IDT, all exception/IRQ/DOS/Toolbox stubs retained)\n'

test-kernel-gc-emu-mutant:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-flair-appswitch-mutant test-fcb-api-mutant
	@printf '>>> test-kernel-gc-emu-mutant: green (desktop and FCB behaviour mutants rejected)\n'

# Two complete flagship object rebuilds, not just the smaller DOS kernel.bin.
# Ref: CLAUDE.md Rule 11 and PRD Sec 7; no source/host-path-dependent bytes.
test-kernel-flair-repro: | $(BUILD)
	@set -eu; for pass in 1 2; do \
		rm -f $(KERNEL_FLAIRTENANTS_OBJS) $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_FLAIRTENANTS_BIN); \
		$(MAKE) --no-print-directory $(KERNEL_FLAIRTENANTS_BIN) >$(BUILD)/kernel-flair-repro-$$pass.log 2>&1; \
		cp -f $(KERNEL_FLAIRTENANTS_BIN) $(BUILD)/kernel-flair-repro-$$pass.bin; \
	done
	@cmp -s $(BUILD)/kernel-flair-repro-1.bin $(BUILD)/kernel-flair-repro-2.bin || { printf '!!! test-kernel-flair-repro FAIL: clean flagship rebuilds differ\n'; exit 1; }
	@printf '>>> test-kernel-flair-repro: green (two clean builds byte-identical; sha256 %s)\n' "$$(sha256sum < $(BUILD)/kernel-flair-repro-1.bin | cut -d' ' -f1)"

TEST_UNIT_GATES += test-kernel-gc-roots test-kernel-flair-repro
TEST_EMU_GATES += test-kernel-gc-emu-mutant

# Round 3 part 2, initech-tode. Independent acceptance bound: the arrival
# binding kernel loaded 182732 bytes; recover at least 10240 more, without
# changing the 196608-byte boot window or its 4096-byte safety floor.
# Ref: WL-0127; PRD Sec 5/6.1/6.3/8; ADR-0003 Appendix A.
KERNEL_R3_LOADED_MAX := 172492
.PHONY: test-kernel-size-r3 test-kernel-size-r3-mutant
.PHONY: test-kernel-size-r3-emu test-kernel-size-r3-emu-mutant
define kernel-r3-grade
$(OBJCOPY) -O binary "$(1)" $(KGC_DIR)/r3-loaded.bin; \
loaded=$$(wc -c < $(KGC_DIR)/r3-loaded.bin); \
if [ "$$loaded" -gt "$(KERNEL_R3_LOADED_MAX)" ]; then \
	printf '!!! kernel-size-r3 FAIL: %s loaded %s B > %s B (10 KiB reclaim required)\n' "$(1)" "$$loaded" "$(KERNEL_R3_LOADED_MAX)"; exit 1; \
fi
endef
test-kernel-size-r3: test-kernel-headroom | $(KGC_DIR)
	@set -eu; $(kernel-headroom-enum); \
	for b in $$bins; do \
		$(call kernel-r3-grade,$${b%.bin}.elf); \
	done
	@printf '>>> test-kernel-size-r3: green (all kernels reclaim >= 10240 B against arrival binding)\n'

# size_pad_one adds real, explicitly rooted loaded data to the flagship until
# it exceeds the independent acceptance bound by exactly one byte. The gate
# must reject that ELF; changing the oracle's bound would not prove this.
# Round 8: retain the flagship's real transient imports, as the r4 mutant does.
# Ref: CODEX-OUT.md, oracle audit; dropping those roots changes the image under
# test and can discard services instead of testing the independent byte cap.
test-kernel-size-r3-mutant: test-kernel-size-r3
	@set -eu; $(call kernel-r3-grade,$(KERNEL_FLAIRTENANTS_ELF)); \
	pad=$$(( $(KERNEL_R3_LOADED_MAX) + 1 - loaded )); \
	printf 'bits 32\nsection .data align=1\nglobal r3_size_pad\nr3_size_pad: times %s db 0xA5\n' "$$pad" >$(KGC_DIR)/r3-pad.asm; \
	$(NASM) -f elf32 $(KGC_DIR)/r3-pad.asm -o $(KGC_DIR)/r3-pad.o
	$(KERNEL_LINK) -u r3_size_pad -o $(KGC_DIR)/r3-pad.elf $(KERNEL_FLAIRTENANTS_OBJS) $(KGC_DIR)/r3-pad.o @$(TRANSIENT_IMPORTS_LD)
	@$(OBJCOPY) -O binary $(KGC_DIR)/r3-pad.elf $(KGC_DIR)/r3-loaded.bin
	@test "$$(wc -c < $(KGC_DIR)/r3-loaded.bin)" -eq $$(( $(KERNEL_R3_LOADED_MAX) + 1 ))
	@if ( $(call kernel-r3-grade,$(KGC_DIR)/r3-pad.elf) ) >$(KGC_DIR)/r3-pad.log 2>&1; then \
		printf '!!! test-kernel-size-r3-mutant FAIL: size_pad_one survived\n'; exit 1; fi
	@grep '!!! kernel-size-r3 FAIL:' $(KGC_DIR)/r3-pad.log
	@printf '>>> test-kernel-size-r3-mutant: green (size_pad_one RED)\n'

# Reuse independent injected traces and their mutation-proven pixel / real
# DOS transcript / mtools identical-image FAT oracles; no new DOS behaviour.
test-kernel-size-r3-emu:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-kernel-gc-emu test-flair-itext
	@printf '>>> test-kernel-size-r3-emu: green (desktop pixels, text trace, DOS/FCB/FAT differential)\n'
test-kernel-size-r3-emu-mutant:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-kernel-gc-emu-mutant
	@printf '>>> test-kernel-size-r3-emu-mutant: green (desktop/FCB behaviour mutants RED)\n'
TEST_UNIT_GATES += test-kernel-size-r3 test-kernel-size-r3-mutant
TEST_EMU_GATES += test-kernel-size-r3-emu test-kernel-size-r3-emu-mutant

# LTO stays confined to selected cold TUs. Optimise across their boundary,
# leaving native hot objects (blitter, region and text) outside the optimiser.
.PHONY: test-kernel-cold-lto test-kernel-cold-lto-mutant
define kernel-cold-grade
if nm --defined-only $(1) | grep -q ' cold_answer$$'; then \
	printf '!!! kernel-cold-lto FAIL: cross-object callee retained\n'; exit 1; \
fi; \
objdump -d $(1) | grep -q '\$$0x16' || { printf '!!! kernel-cold-lto FAIL: constant answer 22 absent\n'; exit 1; }; \
nm --defined-only $(1) | grep -q '^00500000 .* _start$$' || { printf '!!! kernel-cold-lto FAIL: entry moved\n'; exit 1; }
endef
test-kernel-cold-lto: | $(KGC_DIR)
	@set -eu; for unit in entry answer; do \
		$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_COLD_LTO_FLAGS) -c harness/proptest/kernel_cold_$$unit.c -o $(KGC_DIR)/cold_$$unit.o; \
	done
	$(KERNEL_LINK) -o $(KGC_DIR)/cold.elf $(KGC_DIR)/cold_entry.o $(KGC_DIR)/cold_answer.o
	@$(call kernel-cold-grade,$(KGC_DIR)/cold.elf)
	@printf '>>> test-kernel-cold-lto: green (cross-TU call folded to 22; entry fixed)\n'
test-kernel-cold-lto-mutant: test-kernel-cold-lto
	@set -eu; for unit in entry answer; do \
		$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Oz -c harness/proptest/kernel_cold_$$unit.c -o $(KGC_DIR)/cold_native_$$unit.o; \
	done
	$(KERNEL_LINK) -o $(KGC_DIR)/cold_no_lto.elf $(KGC_DIR)/cold_native_entry.o $(KGC_DIR)/cold_native_answer.o
	@if ( $(call kernel-cold-grade,$(KGC_DIR)/cold_no_lto.elf) ) >$(KGC_DIR)/cold_no_lto.log 2>&1; then \
		printf '!!! test-kernel-cold-lto-mutant FAIL: no_lto survived\n'; exit 1; fi
	@grep '!!! kernel-cold-lto FAIL: cross-object callee retained' $(KGC_DIR)/cold_no_lto.log
	@printf '>>> test-kernel-cold-lto-mutant: green (no_lto RED)\n'
TEST_UNIT_GATES += test-kernel-cold-lto test-kernel-cold-lto-mutant

# Round 8: exercise the real LTO link as well as the raw-ld fixture above.
# Ref: CODEX-OUT.md, LTO section probe; round-2 policy Sec 4 lever C.
# A reference in a dead native section makes the plugin emit the callee;
# final section GC must still remove it, and KEEP must place _start first.
$(KGC_DIR)/lto-probe.o: $(KGC_SRC) | $(KGC_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_COLD_LTO_FLAGS) -c $< -o $@
$(KGC_DIR)/native-dead.o: harness/proptest/kernel_gc_native.c | $(KGC_DIR)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -Oz -c $< -o $@
$(KGC_DIR)/lto-probe.elf: $(KGC_DIR)/lto-probe.o $(KGC_DIR)/native-dead.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(KGC_DIR)/lto-probe.o $(KGC_DIR)/native-dead.o

.PHONY: test-kernel-gc-lto test-kernel-gc-lto-mutant
test-kernel-gc-lto: $(KGC_DIR)/lto-probe.elf
	@$(call kernel-gc-grade,$<)
	@printf '>>> test-kernel-gc-lto: green (dead native reference discarded after LTO; entry fixed)\n'

test-kernel-gc-lto-mutant: test-kernel-gc-lto
	$(filter-out $(KERNEL_LTO_SECTION_FLAGS),$(KERNEL_LINK)) -o $(KGC_DIR)/lto-no-sections.elf $(KGC_DIR)/lto-probe.o $(KGC_DIR)/native-dead.o
	@if ( $(call kernel-gc-grade,$(KGC_DIR)/lto-no-sections.elf) ) >$(KGC_DIR)/lto-no-sections.log 2>&1; then \
		printf '!!! test-kernel-gc-lto-mutant FAIL: no_lto_sections survived\n'; exit 1; fi
	@grep '!!! kernel-gc FAIL: dead gc_dead_function retained' $(KGC_DIR)/lto-no-sections.log
	@printf '>>> test-kernel-gc-lto-mutant: green (no_lto_sections RED)\n'
TEST_UNIT_GATES += test-kernel-gc-lto-mutant

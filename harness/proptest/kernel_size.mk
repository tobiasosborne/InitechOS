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
nm --defined-only $(1) > $(KGC_DIR)/symbols; \
for s in _start gc_live_function gc_live_data; do \
	grep -q " $$s$$" $(KGC_DIR)/symbols || { printf '!!! kernel-gc FAIL: live %s removed\n' "$$s"; exit 1; }; \
done; \
for s in gc_dead_function gc_dead_data; do \
	if grep -q " $$s$$" $(KGC_DIR)/symbols; then printf '!!! kernel-gc FAIL: dead %s retained\n' "$$s"; exit 1; fi; \
done; \
grep -q '^00500000 .* _start$$' $(KGC_DIR)/symbols || { printf '!!! kernel-gc FAIL: entry moved from KERNEL_BASE\n'; exit 1; }
endef

.PHONY: test-kernel-gc test-kernel-gc-mutant test-kernel-gc-emu
test-kernel-gc: $(KGC_DIR)/probe.elf
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

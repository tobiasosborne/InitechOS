# initech-tode round 4: fixed arrival measurement, independent of codegen.
# Ref: PRD Sec 5/6.1/6.3/8; ADR-0003 DEC-08 / Appendix A; Rule 6/11.
# Arrival raw flagship = 188552 B; the round reclaimed 16,564 B (WL-0152).
# PM 2026-10-17: the guard is a HEADROOM floor for the flagship -- 16 KiB above
# the 4 KiB global floor of test-kernel-headroom in the 196,608 B window -- so
# later lanes may spend bytes until the next round is due, instead of a fixed
# arrival cap that every merged feature would break by construction.
KERNEL_R4_MIN_HEADROOM := 16384
KERNEL_R4_LOADED_MAX := $(shell echo $$(( $(KERNEL_SECTORS) * 512 - 16384 )))
KERNEL_R4_DIR := $(BUILD)/ksize4
$(KERNEL_R4_DIR): | $(BUILD)
	@mkdir -p $@

define kernel-r4-grade
$(OBJCOPY) -O binary $(1) $(KERNEL_R4_DIR)/loaded.bin; \
loaded=$$(wc -c < $(KERNEL_R4_DIR)/loaded.bin); \
if [ "$$loaded" -gt "$(KERNEL_R4_LOADED_MAX)" ]; then \
	printf '!!! kernel-size-r4 FAIL: loaded %s B > %s B (the flagship must keep 16 KiB headroom -- size round 5 is due)\n' "$$loaded" '$(KERNEL_R4_LOADED_MAX)'; exit 1; \
fi
endef

.PHONY: test-kernel-size-r4 test-kernel-size-r4-mutant
.PHONY: test-kernel-size-r4-emu test-kernel-size-r4-emu-mutant
test-kernel-size-r4: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R4_DIR)
	@$(call kernel-r4-grade,$<)
	@printf '>>> test-kernel-size-r4: green (flagship keeps >= 16 KiB headroom in the 384-sector window)\n'

# Retain the actual flagship imports when making the budget+1 mutant: omitting
# them would silently discard transient services rather than test the budget.
test-kernel-size-r4-mutant: test-kernel-size-r4
	@set -eu; $(call kernel-r4-grade,$(KERNEL_FLAIRTENANTS_ELF)); \
	pad=$$(( $(KERNEL_R4_LOADED_MAX) + 1 - loaded )); \
	printf 'bits 32\nsection .data align=1\nglobal r4_size_pad\nr4_size_pad: times %s db 0xA5\n' "$$pad" >$(KERNEL_R4_DIR)/pad.asm; \
	$(NASM) -f elf32 $(KERNEL_R4_DIR)/pad.asm -o $(KERNEL_R4_DIR)/pad.o
	$(KERNEL_LINK) -u r4_size_pad -o $(KERNEL_R4_DIR)/pad.elf $(KERNEL_FLAIRTENANTS_OBJS) $(KERNEL_R4_DIR)/pad.o @$(TRANSIENT_IMPORTS_LD)
	@$(OBJCOPY) -O binary $(KERNEL_R4_DIR)/pad.elf $(KERNEL_R4_DIR)/pad.bin
	@test "$$(wc -c < $(KERNEL_R4_DIR)/pad.bin)" -eq $$(( $(KERNEL_R4_LOADED_MAX) + 1 ))
	@if ( $(call kernel-r4-grade,$(KERNEL_R4_DIR)/pad.elf) ) >$(KERNEL_R4_DIR)/pad.log 2>&1; then \
		printf '!!! test-kernel-size-r4-mutant FAIL: size_pad_one survived\n'; exit 1; fi
	@grep '!!! kernel-size-r4 FAIL:' $(KERNEL_R4_DIR)/pad.log
	@printf '>>> test-kernel-size-r4-mutant: green (size_pad_one RED)\n'

# Independent DOS transcript, mtools/fsck on the guest-written images, injected
# tenant trace and pixel graders. No new DOS behaviour or self-derived golden.
# IBM DOS 3.30 Technical Reference 6-200 / PDF 323: EXIT closes files and
# returns to parent; MS-DOS 3.3 User's Guide 28 / PDF 41: DIR / COMMAND.COM.
test-kernel-size-r4-emu:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-fcb-api test-flair-appswitch test-flair-itext
	@printf '>>> test-kernel-size-r4-emu: green (DOS/FAT differential; injected desktop/text traces and pixels)\n'
test-kernel-size-r4-emu-mutant:
	+@INITECH_QMP_STDIO=1 $(MAKE) --no-print-directory test-fcb-api-mutant test-flair-appswitch-mutant
	@printf '>>> test-kernel-size-r4-emu-mutant: green (FCB and desktop behaviour mutants RED)\n'

TEST_UNIT_GATES += test-kernel-size-r4 test-kernel-size-r4-mutant
TEST_EMU_GATES += test-kernel-size-r4-emu test-kernel-size-r4-emu-mutant

# Source-to-table equality uses the host C compiler as the independent oracle.
# The original formatter calls and the packed form must emit identical bytes.
$(SERIAL_TRACE_PACK): $(SERIAL_TRACE_PACK_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(MAKEFILE_STAMP): $(SERIAL_TRACE_PACK_SRC)

.PHONY: test-serial-trace-pack test-serial-trace-pack-mutant
test-serial-trace-pack: $(SERIAL_TRACE_PACK) | $(KERNEL_R4_DIR)
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

test-serial-trace-pack-mutant: test-serial-trace-pack
	$(CC) $(CFLAGS) -DSERIAL_TRACE_MUT_BYTE $(KERNEL_R4_DIR)/trace.c -o $(KERNEL_R4_DIR)/trace-mutant
	@$(KERNEL_R4_DIR)/trace-mutant >$(KERNEL_R4_DIR)/trace-mutant.txt
	@if cmp -s $(KERNEL_R4_DIR)/trace-original.txt $(KERNEL_R4_DIR)/trace-mutant.txt; then \
		printf '!!! test-serial-trace-pack-mutant FAIL: trace_byte survived\n'; exit 1; fi
	@printf '!!! serial-trace-pack FAIL: trace_byte changes the original transcript\n'
	@printf '>>> test-serial-trace-pack-mutant: green (trace_byte RED)\n'

TEST_UNIT_GATES += test-serial-trace-pack test-serial-trace-pack-mutant

# Storage equality against the unchanged locked font rows. The independent
# historical font/pixel gates still grade the glyphs themselves.
$(BUILD)/chicago_pack: tools/chicago_pack.c spec/assets/chicago12.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec/assets -o $@ $<
$(CHICAGO_PACK_H): $(BUILD)/chicago_pack spec/assets/chicago12.h
	@$< >$@.tmp && mv -f $@.tmp $@
.PHONY: test-chicago-pack test-chicago-pack-mutant
test-chicago-pack: $(CHICAGO_PACK_H) | $(KERNEL_R4_DIR)
	$(CC) $(CFLAGS) -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_chicago_pack.c -o $(KERNEL_R4_DIR)/chicago-pack
	@$(KERNEL_R4_DIR)/chicago-pack
test-chicago-pack-mutant: test-chicago-pack
	$(CC) $(CFLAGS) -DCHICAGO_PACK_MUT_ROW -I$(BUILD) -Ispec/assets -Ios/flair harness/proptest/test_chicago_pack.c -o $(KERNEL_R4_DIR)/chicago-pack-mut
	@if $(KERNEL_R4_DIR)/chicago-pack-mut >$(KERNEL_R4_DIR)/chicago-mut.log 2>&1; then \
		printf '!!! test-chicago-pack-mutant FAIL: packed_row survived\n'; exit 1; fi
	@grep '^!!! chicago-pack FAIL:' $(KERNEL_R4_DIR)/chicago-mut.log
	@printf '>>> test-chicago-pack-mutant: green (packed_row RED)\n'
TEST_UNIT_GATES += test-chicago-pack test-chicago-pack-mutant

# Compile a reference with the audited native policy, then compare the
# load-bearing producer/consumer sections to the actual kernel objects.
# Round 8 part 2: event matches native -Oz; keyboard matches native -Os.
# The loader joins cold LTO after its context/clobber audit; check the actual
# final ELF save/restore/stack-switch sequence instead of assuming -O0 bytes.
# This is a boundary check, not the semantic oracle (the injected gates and
# independent DOS/FAT/pixel checks above remain mandatory).
define kernel-native-boundary-grade
for pair in 'event flair_raw_post drain_one GetNextEvent WaitNextEvent' 'kbd kbd_ring_put kbd_ring_get kbd_irq_handler kbd_haschar kbd_getchar'; do \
	set -- $$pair; unit=$$1; shift; \
	for fn; do \
		rm -f $(KERNEL_R4_DIR)/base-section $(KERNEL_R4_DIR)/test-section; \
		$(OBJCOPY) --dump-section .text.$$fn=$(KERNEL_R4_DIR)/base-section $(KERNEL_R4_DIR)/native-$$unit.o || exit 1; \
		$(OBJCOPY) --dump-section .text.$$fn=$(KERNEL_R4_DIR)/test-section $(KERNEL_R4_DIR)/$(1)-$$unit.o || exit 1; \
		test -s $(KERNEL_R4_DIR)/base-section && test -s $(KERNEL_R4_DIR)/test-section || exit 1; \
		cmp -s $(KERNEL_R4_DIR)/base-section $(KERNEL_R4_DIR)/test-section || { \
			printf '!!! kernel-native-boundary FAIL: %s changed\n' "$$fn"; exit 1; }; \
	done; \
done
endef
.PHONY: test-kernel-native-boundaries test-kernel-native-boundaries-mutant
test-kernel-native-boundaries: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R4_DIR)
	@set -eu; for unit in kbd event; do \
		case $$unit in event) src=os/flair/event.c;; *) src=os/milton/$$unit.c;; esac; \
		flags=; case $$unit in event) flags='$(KERNEL_EVENT_OPT)';; kbd) flags='$(KERNEL_KBD_OPT) $(KERNEL_PURE_DECODE_FLAGS)';; esac; \
		$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $$flags -Ispec -Ios/milton -Ios/flair -c $$src -o $(KERNEL_R4_DIR)/native-$$unit.o; \
		cp -f $(BUILD)/$$unit.o $(KERNEL_R4_DIR)/selected-$$unit.o; \
	done
	@set -eu; $(call kernel-native-boundary-grade,selected)
	@set -eu; for src in $(TEST_LOADER_SRC) $(TEST_MZLOAD_SRC) $(TEST_TENANT_LOAD_SRC); do \
		$(CC) $(CFLAGS) -Ispec -Ios/milton -Iseed $$src $(KERNEL_LOADER_C) $(KERNEL_PSP_C) -o $(KERNEL_R4_DIR)/validation-model; \
		$(KERNEL_R4_DIR)/validation-model; \
	done
	$(CC) $(CFLAGS) $(KERNEL_EVENT_OPT) $(EVENT_INC) $(TEST_EVENT_SRC) os/flair/event.c -o $(KERNEL_R4_DIR)/event-model
	@$(KERNEL_R4_DIR)/event-model
	$(CC) $(CFLAGS) $(KERNEL_KBD_OPT) $(KERNEL_PURE_DECODE_FLAGS) -Ios/milton -Iseed $(TEST_KBD_SRC) $(TEST_KBD_DEPS) -o $(KERNEL_R4_DIR)/kbd-model
	@$(KERNEL_R4_DIR)/kbd-model
	@sh harness/proptest/kernel_transfer_check.sh $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/transfer.dis
	@printf '>>> test-kernel-native-boundaries: green (native event/keyboard policy; final ELF loader context and stack switch; host models)\n'
test-kernel-native-boundaries-mutant: test-kernel-native-boundaries
	@set -eu; for unit in kbd event; do \
		case $$unit in event) src=os/flair/event.c;; *) src=os/milton/$$unit.c;; esac; \
		$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -O0 -Ispec -Ios/milton -Ios/flair -c $$src -o $(KERNEL_R4_DIR)/wrong_native-$$unit.o; \
	done
	@if ( $(call kernel-native-boundary-grade,wrong_native) ) >$(KERNEL_R4_DIR)/wrong-native.log 2>&1; then \
		printf '!!! test-kernel-native-boundaries-mutant FAIL: wrong_native_policy survived\n'; exit 1; fi
	@grep '^!!! kernel-native-boundary FAIL:' $(KERNEL_R4_DIR)/wrong-native.log
	@cp -f $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/bad-restore.elf
	@set -eu; pos=$$(LC_ALL=C grep -abo "$$(printf '\213\130\010\213\150\014\213\160\020\213\170\024\213\040\377\140\004')" $(KERNEL_R4_DIR)/bad-restore.elf | cut -d: -f1); \
		test "$$(printf '%s\n' "$$pos" | wc -l)" -eq 1; test -n "$$pos"; \
		printf '\020' | dd of=$(KERNEL_R4_DIR)/bad-restore.elf bs=1 seek=$$(( pos + 11 )) conv=notrunc status=none
	@if sh harness/proptest/kernel_transfer_check.sh $(KERNEL_R4_DIR)/bad-restore.elf $(KERNEL_R4_DIR)/bad-restore.dis >$(KERNEL_R4_DIR)/bad-restore.log 2>&1; then \
		printf '!!! test-kernel-native-boundaries-mutant FAIL: wrong_edi_restore survived\n'; exit 1; fi
	@grep '^!!! kernel-transfer FAIL: restore step 4' $(KERNEL_R4_DIR)/bad-restore.log
	@printf '>>> test-kernel-native-boundaries-mutant: green (wrong_native_policy and wrong_edi_restore RED)\n'
TEST_UNIT_GATES += test-kernel-native-boundaries test-kernel-native-boundaries-mutant

# Round 8 part 2: data placement may lose cache-oriented padding, while ABI
# layouts and explicitly requested alignment must remain unchanged. A real
# packing flag is the counterexample, not a modified expected byte vector.
KERNEL_LAYOUT_SRC := harness/proptest/kernel_data_layout.c
KERNEL_LAYOUT_INC := -Ispec -Ios/milton -Ios/flair
.PHONY: test-kernel-data-layout test-kernel-data-layout-mutant
test-kernel-data-layout: | $(KERNEL_R4_DIR)
	@case '$(KERNEL_CFLAGS) $(TENANT_CFLAGS)' in *-malign-data=abi*) \
		printf '!!! kernel-data-layout FAIL: cold flag leaked to shared/tenant flags\n'; exit 1;; esac
	@case '$(KERNEL_COLD_DATA_FLAGS)' in -malign-data=abi) :;; *) \
		printf '!!! kernel-data-layout FAIL: cold ABI alignment policy absent\n'; exit 1;; esac
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_LAYOUT_INC) -c $(KERNEL_LAYOUT_SRC) -o $(KERNEL_R4_DIR)/layout-default.o
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_COLD_DATA_FLAGS) $(KERNEL_LAYOUT_INC) -c $(KERNEL_LAYOUT_SRC) -o $(KERNEL_R4_DIR)/layout-abi.o
	@set -eu; for policy in default abi; do \
		$(OBJCOPY) --dump-section .layout=$(KERNEL_R4_DIR)/layout-$$policy.bin $(KERNEL_R4_DIR)/layout-$$policy.o; \
		readelf -SW $(KERNEL_R4_DIR)/layout-$$policy.o | awk '/\.explicit_alignment / && $$NF==16 {found=1} END {exit !found}'; \
	done
	@cmp $(KERNEL_R4_DIR)/layout-default.bin $(KERNEL_R4_DIR)/layout-abi.bin
	@printf '>>> test-kernel-data-layout: green (22 sizeof/alignment pairs equal; explicit align 16 retained; tenant flags isolated)\n'
test-kernel-data-layout-mutant: test-kernel-data-layout
	@if LC_ALL=C $(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -fpack-struct=1 $(KERNEL_LAYOUT_INC) -c $(KERNEL_LAYOUT_SRC) -o $(KERNEL_R4_DIR)/layout-packed.o >$(KERNEL_R4_DIR)/layout-packed.log 2>&1; then \
		$(OBJCOPY) --dump-section .layout=$(KERNEL_R4_DIR)/layout-packed.bin $(KERNEL_R4_DIR)/layout-packed.o; \
		if cmp -s $(KERNEL_R4_DIR)/layout-default.bin $(KERNEL_R4_DIR)/layout-packed.bin; then \
			printf '!!! test-kernel-data-layout-mutant FAIL: packed_layout survived\n'; exit 1; fi; \
	else grep -q 'static assertion failed' $(KERNEL_R4_DIR)/layout-packed.log || { cat $(KERNEL_R4_DIR)/layout-packed.log; exit 1; }; fi
	@printf '>>> test-kernel-data-layout-mutant: green (packed_layout RED)\n'
TEST_UNIT_GATES += test-kernel-data-layout test-kernel-data-layout-mutant

$(KERNEL_KMAIN_COLD_OBJ): os/milton/kmain_cold.c os/milton/kmain_cold.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_TENANTS_OPT) -Ios/milton -c $< -o $@
.PHONY: test-kernel-cold-about test-kernel-cold-about-mutant
test-kernel-cold-about: | $(KERNEL_R4_DIR)
	$(CC) $(CFLAGS) -Ios/milton harness/proptest/test_kmain_cold.c os/milton/kmain_cold.c -o $(KERNEL_R4_DIR)/cold-about
	@$(KERNEL_R4_DIR)/cold-about
test-kernel-cold-about-mutant: test-kernel-cold-about
	$(CC) $(CFLAGS) -DKMAIN_ABOUT_MUT_NONE -Ios/milton harness/proptest/test_kmain_cold.c os/milton/kmain_cold.c -o $(KERNEL_R4_DIR)/cold-about-mut
	@if $(KERNEL_R4_DIR)/cold-about-mut >$(KERNEL_R4_DIR)/cold-about-mut.log 2>&1; then \
		printf '!!! test-kernel-cold-about-mutant FAIL: about_none survived\n'; exit 1; fi
	@grep '^!!! cold-about FAIL:' $(KERNEL_R4_DIR)/cold-about-mut.log
	@printf '>>> test-kernel-cold-about-mutant: green (about_none RED)\n'
TEST_UNIT_GATES += test-kernel-cold-about test-kernel-cold-about-mutant
$(MAKEFILE_STAMP): os/milton/kmain_cold.h os/flair/chicago_packed.h

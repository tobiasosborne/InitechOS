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
# initech-1x6x2.1: the re-graded gates are PREREQUISITES, not a nested
# $(MAKE) -- a nested run re-ran gates that are in the same parallel vector
# (test-par) and raced them on their fixed build/ output names.
test-kernel-size-r4-emu: test-fcb-api test-flair-appswitch test-flair-itext
	@printf '>>> test-kernel-size-r4-emu: green (DOS/FAT differential; injected desktop/text traces and pixels)\n'
test-kernel-size-r4-emu-mutant: test-fcb-api-mutant test-flair-appswitch-mutant
	@printf '>>> test-kernel-size-r4-emu-mutant: green (FCB and desktop behaviour mutants RED)\n'

TEST_UNIT_GATES += test-kernel-size-r4 test-kernel-size-r4-mutant
TEST_EMU_GATES += test-kernel-size-r4-emu test-kernel-size-r4-emu-mutant

# Static DF contract on the actual flagship, including the non-local resume.
.PHONY: test-kernel-df test-kernel-df-mutant
test-kernel-df: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R4_DIR)
	@sh harness/proptest/kernel_df_check.sh $< $(KERNEL_R4_DIR)/df.dis
	@printf '>>> test-kernel-df: green (linked entry/resume CLD contract; static proof only)\n'
test-kernel-df-mutant: test-kernel-df
	@cp -f $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/df-mutant.elf
	@set -eu; addr=$$(awk '/<irq1_entry>:/ {active=1; next} active && /cld/ {sub(/:$$/,"",$$1); print $$1; exit}' $(KERNEL_R4_DIR)/df.dis); \
		set -- $$(objdump -h $(KERNEL_R4_DIR)/df-mutant.elf | awk '$$2==".text" {print $$4, $$6}'); \
		test -n "$$addr"; test "$$#" -eq 2; \
		printf '\220' | dd of=$(KERNEL_R4_DIR)/df-mutant.elf bs=1 seek=$$(( 0x$$addr - 0x$$1 + 0x$$2 )) conv=notrunc status=none
	@if sh harness/proptest/kernel_df_check.sh $(KERNEL_R4_DIR)/df-mutant.elf $(KERNEL_R4_DIR)/df-mutant.dis >$(KERNEL_R4_DIR)/df-mutant.log 2>&1; then \
		printf '!!! test-kernel-df-mutant FAIL: missing_irq1_cld survived\n'; exit 1; fi
	@grep '^!!! kernel-df FAIL: irq1_entry calls C before cld' $(KERNEL_R4_DIR)/df-mutant.log
	@printf '>>> test-kernel-df-mutant: green (real ELF missing_irq1_cld RED)\n'
TEST_UNIT_GATES += test-kernel-df test-kernel-df-mutant

.PHONY: test-kernel-transfer test-kernel-transfer-mutant
test-kernel-transfer: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R4_DIR)
	@sh harness/proptest/kernel_transfer_check.sh $< $(KERNEL_R4_DIR)/transfer.dis
	@printf '>>> test-kernel-transfer: green (unique linked transfer; ESP/EBP-based stack operand)\n'
test-kernel-transfer-mutant: test-kernel-transfer
	@sh harness/proptest/kernel_transfer_mutants.sh $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)
TEST_UNIT_GATES += test-kernel-transfer test-kernel-transfer-mutant

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

# Round-8 review F3: same-source/same-flag byte equality cannot prove barrier
# placement. Keep the independent host models and linked loader contract;
# ring barriers are review-proven, not gate-proven (see the design document).
.PHONY: test-kernel-native-boundaries test-kernel-native-boundaries-mutant
test-kernel-native-boundaries: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R4_DIR)
	@set -eu; for src in $(TEST_LOADER_SRC) $(TEST_MZLOAD_SRC) $(TEST_TENANT_LOAD_SRC); do \
		$(CC) $(CFLAGS) -Ispec -Ios/milton -Iseed $$src $(KERNEL_LOADER_C) $(KERNEL_PSP_C) -o $(KERNEL_R4_DIR)/validation-model; \
		$(KERNEL_R4_DIR)/validation-model; \
	done
	$(CC) $(CFLAGS) $(KERNEL_EVENT_OPT) $(EVENT_INC) $(TEST_EVENT_SRC) os/flair/event.c -o $(KERNEL_R4_DIR)/event-model
	@$(KERNEL_R4_DIR)/event-model
	$(CC) $(CFLAGS) $(KERNEL_KBD_OPT) $(KERNEL_PURE_DECODE_FLAGS) -Ios/milton -Iseed $(TEST_KBD_SRC) $(TEST_KBD_DEPS) -o $(KERNEL_R4_DIR)/kbd-model
	@$(KERNEL_R4_DIR)/kbd-model
	@sh harness/proptest/kernel_transfer_check.sh $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/transfer.dis
	@printf '>>> test-kernel-native-boundaries: green (linked loader transfer; host models; ring barriers review-proven only)\n'
test-kernel-native-boundaries-mutant: test-kernel-native-boundaries
	@cp -f $(KERNEL_FLAIRTENANTS_ELF) $(KERNEL_R4_DIR)/bad-restore.elf
	@set -eu; pos=$$(LC_ALL=C grep -abo "$$(printf '\213\130\010\213\150\014\213\160\020\213\170\024\213\040\377\140\004')" $(KERNEL_R4_DIR)/bad-restore.elf | cut -d: -f1); \
		test "$$(printf '%s\n' "$$pos" | wc -l)" -eq 1; test -n "$$pos"; \
		printf '\020' | dd of=$(KERNEL_R4_DIR)/bad-restore.elf bs=1 seek=$$(( pos + 11 )) conv=notrunc status=none
	@if sh harness/proptest/kernel_transfer_check.sh $(KERNEL_R4_DIR)/bad-restore.elf $(KERNEL_R4_DIR)/bad-restore.dis >$(KERNEL_R4_DIR)/bad-restore.log 2>&1; then \
		printf '!!! test-kernel-native-boundaries-mutant FAIL: wrong_edi_restore survived\n'; exit 1; fi
	@grep '^!!! kernel-transfer FAIL: restore step 4' $(KERNEL_R4_DIR)/bad-restore.log
	@printf '>>> test-kernel-native-boundaries-mutant: green (real ELF wrong_edi_restore RED)\n'
TEST_UNIT_GATES += test-kernel-native-boundaries test-kernel-native-boundaries-mutant

# F3: removed test-kernel-data-layout and its packing mutant. sizeof/_Alignof
# do not measure -malign-data=abi global placement; no policy oracle is claimed.

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

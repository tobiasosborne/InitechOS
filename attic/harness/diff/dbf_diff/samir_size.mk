# DEC-03a.8, initech-fxolx: size policy retargeted; behavioral gates retained.
# ADR-0009 DEC-03/04/05. Factory-only diagnostics never relax samir.ld.
# Overlays Phase B (initech-5etom.1; docs/design/samir-overlays.md Sec 5):
# the oracle now reads SAMIR.EXE (resident module) and its sibling SAMIR.OVL
# (slot = largest overlay). DEC-03a.8 retires the old window/floor.
SAMIR_SIZE_TEST := $(BUILD)/test_samir_size
SAMIR_SIZE_IMAGE ?= $(SAMIR_EXE)
$(SAMIR_SIZE_TEST): $(DBF_DIFF_DIR)/test_samir_size.c spec/memory_map.h spec/samir/ovl_format.h spec/samir/memory_contract.h spec/initechmz_extended.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec -o $@ $<
.PHONY: test-samir-size test-samir-size-mutant test-samir-size-repro test-samir-size-emu test-samir-size-emu-mutant
test-samir-size: $(SAMIR_SIZE_TEST) $(SAMIR_SIZE_IMAGE) $(SAMIR_OVL)
	@$(SAMIR_SIZE_TEST) $(SAMIR_SIZE_IMAGE)
	@printf '>>> test-samir-size: green (DEC-03a.8: module <=256 KiB, declared high footprint/heap; slot == largest overlay)\n'

# DEC-03a.8 RETARGET: ceiling+1 RED / ceiling GREEN, including >old-window
# images. KEEP SLOT mutant, byte reproducibility, all behavior aliases below.
SAMIR_SIZE_RES := $(BUILD)/samir_exe/SAMIR.elf.res
test-samir-size-mutant: test-samir-size $(MZLINK_BIN)
	@mkdir -p $(BUILD)/samir-size
	@set -e; for v in red:262145 floor:262144 above-old:200000; do n=$${v%%:*}; bytes=$${v#*:}; \
	   cp -f $(SAMIR_SIZE_RES) $(BUILD)/samir-size/$$n.res; \
	   truncate -s $$bytes $(BUILD)/samir-size/$$n.res; \
	   $(MZLINK_BIN) $(BUILD)/samir-size/$$n.res $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.EXE --header-relocs --samir 2>/dev/null; \
	   cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.OVL; \
	 done; \
	 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_red.EXE > $(BUILD)/samir-size/pad-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED SAMIR_SIZE:' $(BUILD)/samir-size/pad-red.log || { cat $(BUILD)/samir-size/pad-red.log; exit 1; }; \
	 cat $(BUILD)/samir-size/pad-red.log
	@$(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_above-old.EXE
	@$(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_floor.EXE
	@set -e; cp -f $(SAMIR_EXE) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE; \
	 cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL; \
	 slot=$$(od -An -tu4 -j16 -N4 $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL | tr -d ' '); slot=$$(( slot + 16 )); \
	 printf "$$(printf '\\%03o\\%03o\\%03o\\%03o' $$(( slot & 255 )) $$(( (slot >> 8) & 255 )) $$(( (slot >> 16) & 255 )) $$(( slot >> 24 )))" | \
	   dd of=$(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL bs=1 seek=16 conv=notrunc status=none; \
	 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE > $(BUILD)/samir-size/slot-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED SAMIR_SLOT:' $(BUILD)/samir-size/slot-red.log || { cat $(BUILD)/samir-size/slot-red.log; exit 1; }; \
	 cat $(BUILD)/samir-size/slot-red.log
	@printf '>>> test-samir-size-mutant: green (DEC-03a.8 ceiling+1 RED; ceiling and above-old GREEN; SAMIR_SIZE_SLOT RED)\n'
# Reproducible (Rule 11): two forced rebuilds give byte-identical SAMIR.EXE AND
# SAMIR.OVL (the stamp in each ties the pair).
test-samir-size-repro: $(SAMIR_EXE)
	@mkdir -p $(BUILD)/samir-size
	@set -e; root=$$(mktemp -d $(abspath $(BUILD))/samir-size/repro-XXXXXX); \
	 $(MAKE) --no-print-directory -B samir-exe BUILD=$$root/first > $$root/first.log 2>&1; \
	 $(MAKE) --no-print-directory -B samir-exe BUILD=$$root/second > $$root/second.log 2>&1; \
	 cmp $$root/first/SAMIR.EXE $$root/second/SAMIR.EXE; \
	 cmp $$root/first/SAMIR.OVL $$root/second/SAMIR.OVL; \
	 cmp $(SAMIR_EXE) $$root/first/SAMIR.EXE; \
	 cmp $(basename $(SAMIR_EXE)).OVL $$root/first/SAMIR.OVL
	@printf '>>> test-samir-size-repro: green (two clean builds byte-identical): '; sha256sum $(SAMIR_EXE) $(SAMIR_OVL)

# Locked injected traces, independent III+ transcripts/DBF bytes, mtools and
# dosfstools are the existing boot/write gates (IBM DOS 3.30 TR 6-200, EXIT).
test-samir-size-emu: test-samir-boot test-samir-write test-samir-canon-y2k
	@printf '>>> test-samir-size-emu: green (boot, persisted writes, canon replay)\n'
test-samir-size-emu-mutant: test-samir-boot-mutant test-samir-write-mutant test-samir-canon-y2k-mutant
	@printf '>>> test-samir-size-emu-mutant: green (PAL short-read, dropped write, DO-file truncation RED)\n'
TEST_UNIT_GATES += test-samir-size test-samir-size-mutant test-samir-size-repro
TEST_EMU_GATES += test-samir-size-emu test-samir-size-emu-mutant

SAMIR_MSG_GEN := $(BUILD)/samir_msg_pack
SAMIR_MSG_HEADER := $(SAMIR_INC_DIR)/samir/messages_packed.h
$(SAMIR_MSG_GEN): tools/samir_msg_pack.c | $(BUILD)
	$(CC) $(CFLAGS) -O2 -o $@ $<
$(BUILD)/test_samir_messages: $(DBF_DIFF_DIR)/test_samir_messages.c $(SAMIR_INC_DIR)/samir/messages.h $(SAMIR_MSG_HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -I$(SAMIR_INC_DIR) -o $@ $<
$(BUILD)/test_samir_messages_mutant: $(DBF_DIFF_DIR)/test_samir_messages.c $(SAMIR_INC_DIR)/samir/messages.h $(SAMIR_MSG_HEADER) | $(BUILD)
	$(CC) $(CFLAGS) -DSAMIR_SIZE_MUTATE_MSG_PAIR -I$(SAMIR_INC_DIR) -o $@ $<
.PHONY: test-samir-messages test-samir-messages-mutant
test-samir-messages: $(BUILD)/test_samir_messages $(SAMIR_MSG_GEN)
	@mkdir -p $(BUILD)/samir-size
	@$(SAMIR_MSG_GEN) spec/samir/dbase_msg_codes.tsv > $(BUILD)/samir-size/messages-regenerated.h
	@cmp $(SAMIR_MSG_HEADER) $(BUILD)/samir-size/messages-regenerated.h
	@$(BUILD)/test_samir_messages spec/samir/dbase_msg_codes.tsv
	@printf '>>> test-samir-messages: green (deterministic packing; independent catalogue decode)\n'
test-samir-messages-mutant: test-samir-messages $(BUILD)/test_samir_messages_mutant
	@set +e; $(BUILD)/test_samir_messages_mutant spec/samir/dbase_msg_codes.tsv > $(BUILD)/samir-size/msg-pair-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED SAMIR_MSG_PAIR:' $(BUILD)/samir-size/msg-pair-red.log || { cat $(BUILD)/samir-size/msg-pair-red.log; exit 1; }; \
	 head -1 $(BUILD)/samir-size/msg-pair-red.log
	@printf '>>> test-samir-messages-mutant: green (SAMIR_SIZE_MUTATE_MSG_PAIR RED)\n'
TEST_UNIT_GATES += test-samir-messages test-samir-messages-mutant

# Regression exposed by the acceptance run: numeric memvar presentation was
# accidentally normalized only in the MEMVAR_RECLAIM mutant branch. PROG-04/05
# and DML-02 are independent real-III+ captures, including the spaces.
$(BUILD)/prog_diff_memwidth_mutant: $(PROG_DIFF_DIR)/prog_diff.c $(PROG_DIFF_ENG) $(SAMIR_PAL_HOST_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_SIZE_MUTATE_MEMVAR_WIDTH -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $^
.PHONY: test-samir-memwidth test-samir-memwidth-mutant test-samir-memwidth-emu test-samir-memwidth-emu-mutant
test-samir-memwidth: test-dbase-diff test-samir-dml-macro test-samir-repl samir-size-goldens $(SAMIR_FNS_TEST) $(CLIENTS_DBF)
	@$(SAMIR_FNS_TEST) memwidth host
	@printf '>>> test-samir-memwidth: green (PROG-04/05, DML-02 numeric memvar columns)\n'
test-samir-memwidth-mutant: test-samir-memwidth $(BUILD)/prog_diff_memwidth_mutant
	@mkdir -p $(BUILD)/samir-size
	@set +e; $(BUILD)/prog_diff_memwidth_mutant $(DBASE3_DECOMP) $(PROG_DIFF_DIR) > $(BUILD)/samir-size/memwidth-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^FAIL: flow: stdout matches real III+ golden' $(BUILD)/samir-size/memwidth-red.log || { cat $(BUILD)/samir-size/memwidth-red.log; exit 1; }; \
	 printf 'SAMIR_SIZE_MUTATE_MEMVAR_WIDTH RED: '; grep -m1 '^FAIL:' $(BUILD)/samir-size/memwidth-red.log
	@printf '>>> test-samir-memwidth-mutant: green\n'
test-samir-memwidth-emu: test-samir-dml-macro-emu samir-size-goldens $(SAMIR_FNS_TEST) $(CLIENTS_DBF)
	@INITECH_QMP_STDIO=1 $(SAMIR_FNS_TEST) memwidth emu
	@printf '>>> test-samir-memwidth-emu: green (real III+ DML-02 in QEMU)\n'
test-samir-memwidth-emu-mutant: test-samir-memwidth-emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MEMVAR_WIDTH.EXE $(CLIENTS_DBF)
	@mkdir -p $(BUILD)/samir-size   # own scratch dir: FAIL on a fresh parallel worker (initech-27clv)
	@set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) macro emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MEMVAR_WIDTH.EXE > $(BUILD)/samir-size/memwidth-emu-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q 'FAIL.*DML transcript byte-identical' $(BUILD)/samir-size/memwidth-emu-red.log && \
	 ! grep -q 'FAIL.*locked QEMU trace' $(BUILD)/samir-size/memwidth-emu-red.log || { cat $(BUILD)/samir-size/memwidth-emu-red.log; exit 1; }; \
	 printf 'SAMIR_SIZE_MUTATE_MEMVAR_WIDTH emu RED: '; grep -m1 'FAIL.*DML transcript' $(BUILD)/samir-size/memwidth-emu-red.log
	@printf '>>> test-samir-memwidth-emu-mutant: green\n'
TEST_UNIT_GATES += test-samir-memwidth test-samir-memwidth-mutant
TEST_EMU_GATES += test-samir-memwidth-emu test-samir-memwidth-emu-mutant

.PHONY: mint-samir-memwidth test-samir-messages-emu test-samir-messages-emu-mutant
mint-samir-memwidth: $(CLIENTS_DBF)
	@sh $(DBF_DIFF_DIR)/fns_mint.sh $(abspath $(DBASE3_DECOMP)) memwidth
test-samir-messages-emu: test-samir-core-emu
	@printf '>>> test-samir-messages-emu: green (exact catalogue messages 76/77 in QEMU)\n'
test-samir-messages-emu-mutant: test-samir-messages-emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MSG_PAIR.EXE
	@set +e; INITECH_QMP_STDIO=1 $(BUILD)/test_samir_core_emu $(BUILD)/SAMIR_SCREEN_SAMIR_SIZE_MUTATE_MSG_PAIR.EXE > $(BUILD)/samir-size/msg-pair-emu-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q 'FAIL.*guest exact manual error' $(BUILD)/samir-size/msg-pair-emu-red.log && \
	 ! grep -q 'FAIL.*guest did not panic' $(BUILD)/samir-size/msg-pair-emu-red.log || { cat $(BUILD)/samir-size/msg-pair-emu-red.log; exit 1; }; \
	 printf 'SAMIR_SIZE_MUTATE_MSG_PAIR emu RED: '; grep -m1 'FAIL.*guest exact manual error' $(BUILD)/samir-size/msg-pair-emu-red.log
	@printf '>>> test-samir-messages-emu-mutant: green\n'
TEST_EMU_GATES += test-samir-messages-emu test-samir-messages-emu-mutant

.PHONY: samir-size-goldens
samir-size-goldens: $(SAMIR_FNS_PROMOTED)
	@sha256sum --quiet -c $(PROG_DIFF_DIR)/samir-size-real.sha256
	@printf '>>> samir-size-goldens: green (SF-memwidth real III+ capture pinned)\n'
TEST_UNIT_GATES += samir-size-goldens
# DEC-03a.8, initech-fxolx: link reserve cannot outrun the declared loader
# reserve. A synthetic link tests the ASSERT itself, independently of packer.
$(BUILD)/samir_link_reserve.o: harness/emu/samir_link_reserve.asm | $(BUILD)
	$(NASM) -f elf32 $< -o $@
$(BUILD)/samir_link_reserve_big.o: harness/emu/samir_link_reserve.asm | $(BUILD)
	$(NASM) -f elf32 -DRESERVE_OVERSIZE $< -o $@
.PHONY: test-samir-link-reserve-mutant
test-samir-link-reserve-mutant: $(SAMIR_LD_SCRIPT) $(BUILD)/samir_link_reserve.o $(BUILD)/samir_link_reserve_big.o
	@$(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) --defsym=__samir_link_reserve=65536 -o $(BUILD)/samir_link_reserve.elf $(BUILD)/samir_link_reserve.o
	@if $(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) --defsym=__samir_link_reserve=65552 -o $(BUILD)/samir_link_reserve_bad.elf $(BUILD)/samir_link_reserve.o >$(BUILD)/samir_link_reserve.log 2>&1; then echo '!!! DEC-03a link reserve above declaration survived'; exit 1; fi
	@grep 'link reserve exceeds declared loader reserve/window' $(BUILD)/samir_link_reserve.log
	@if $(LD) -m elf_i386 -T $(SAMIR_LD_SCRIPT) -o $(BUILD)/samir_link_actual_bad.elf $(BUILD)/samir_link_reserve_big.o >$(BUILD)/samir_link_actual.log 2>&1; then echo '!!! DEC-03a actual reserve overflow survived'; exit 1; fi
	@grep 'slot + BSS + PAL tail exceed declared loader reserve' $(BUILD)/samir_link_actual.log
	@printf '>>> test-samir-link-reserve-mutant: green (link bound > declaration RED, actual reserve overflow RED, equal bound GREEN)\n'
TEST_UNIT_GATES += test-samir-link-reserve-mutant

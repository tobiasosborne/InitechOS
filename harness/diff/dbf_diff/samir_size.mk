# ADR-0009 DEC-03/04/05. Factory-only diagnostics never relax samir.ld.
# Overlays Phase B (initech-5etom.1; docs/design/samir-overlays.md Sec 5):
# the oracle now reads SAMIR.EXE (resident module) and its sibling SAMIR.OVL
# (slot = largest overlay); same physical window, same 4096 B floor.
SAMIR_SIZE_TEST := $(BUILD)/test_samir_size
SAMIR_SIZE_IMAGE ?= $(SAMIR_EXE)
$(SAMIR_SIZE_TEST): $(DBF_DIFF_DIR)/test_samir_size.c spec/memory_map.h spec/samir/ovl_format.h | $(BUILD)
	$(CC) $(CFLAGS) -Ispec -o $@ $<
.PHONY: test-samir-size test-samir-size-mutant test-samir-size-repro test-samir-size-emu test-samir-size-emu-mutant
test-samir-size: $(SAMIR_SIZE_TEST) $(SAMIR_SIZE_IMAGE)
	@$(SAMIR_SIZE_TEST) $(SAMIR_SIZE_IMAGE)
	@printf '>>> test-samir-size: green (resident >=4096 B headroom against the samir.ld PAL-memory budget; slot == largest overlay)\n'

# Rule 6, two size mutants built from the real artifacts:
#  SAMIR_SIZE_PAD_4096: the real resident module zero-padded to 4095 B of
#    headroom and re-wrapped by mzlink (RED), then to exactly 4096 B (GREEN);
#  SAMIR_SIZE_SLOT: the real SAMIR.OVL with its slot length 16 B too large,
#    so the slot is no longer the largest overlay (RED).
# File-size mutants, not behaviour mutants; QEMU behaviour is separate.
SAMIR_SIZE_RES := $(BUILD)/samir_exe/SAMIR.elf.res
test-samir-size-mutant: test-samir-size $(MZLINK_BIN)
	@mkdir -p $(BUILD)/samir-size
	@set -e; budget=$$($(SAMIR_SIZE_TEST) $(SAMIR_EXE) | sed -n 's/.*(budget \([0-9][0-9]*\) B).*/\1/p'); \
	 test -n "$$budget" || { printf 'samir-size-mutant: missing PAL-memory budget\n'; exit 1; }; \
	 for v in red:1 floor:0; do n=$${v%%:*}; extra=$${v#*:}; \
	   cp -f $(SAMIR_SIZE_RES) $(BUILD)/samir-size/$$n.res; \
	   truncate -s $$(( budget - 4096 + extra )) $(BUILD)/samir-size/$$n.res; \
	   $(MZLINK_BIN) $(BUILD)/samir-size/$$n.res $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.EXE --header-relocs --minalloc $(SAMIR_MINALLOC) 2>/dev/null; \
	   cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_PAD_$$n.OVL; \
	 done; \
	 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_red.EXE > $(BUILD)/samir-size/pad-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED SAMIR_SIZE:' $(BUILD)/samir-size/pad-red.log || { cat $(BUILD)/samir-size/pad-red.log; exit 1; }; \
	 cat $(BUILD)/samir-size/pad-red.log
	@$(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_PAD_floor.EXE
	@set -e; cp -f $(SAMIR_EXE) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE; \
	 cp -f $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL; \
	 slot=$$(od -An -tu4 -j16 -N4 $(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL | tr -d ' '); slot=$$(( slot + 16 )); \
	 printf "$$(printf '\\%03o\\%03o\\%03o\\%03o' $$(( slot & 255 )) $$(( (slot >> 8) & 255 )) $$(( (slot >> 16) & 255 )) $$(( slot >> 24 )))" | \
	   dd of=$(BUILD)/samir-size/SAMIR_SIZE_SLOT.OVL bs=1 seek=16 conv=notrunc status=none; \
	 set +e; $(SAMIR_SIZE_TEST) $(BUILD)/samir-size/SAMIR_SIZE_SLOT.EXE > $(BUILD)/samir-size/slot-red.log 2>&1; rc=$$?; set -e; \
	 test $$rc = 1 && grep -q '^RED SAMIR_SLOT:' $(BUILD)/samir-size/slot-red.log || { cat $(BUILD)/samir-size/slot-red.log; exit 1; }; \
	 cat $(BUILD)/samir-size/slot-red.log
	@printf '>>> test-samir-size-mutant: green (SAMIR_SIZE_PAD floor-1 RED; floor GREEN; SAMIR_SIZE_SLOT RED)\n'
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
	@printf '>>> test-samir-size-repro: green (two clean builds byte-identical): '; sha256sum $(SAMIR_EXE)

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

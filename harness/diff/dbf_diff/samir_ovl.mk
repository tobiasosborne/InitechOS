# SAMIR overlays Phase B gates (initech-5etom.1; docs/design/samir-overlays.md).
# Included after samir_size.mk. Every gate here is RED-first: each named
# mutant must fail for its own reason.
#
#   test-samir-ovl              host: CRC check value, locked layout, header /
#                               directory / relocation on literal bytes, and the
#                               shipped SAMIR.EXE + SAMIR.OVL pair walked
#   test-samir-ovl-mutant       NO_RELOC and NO_STAMP each RED on their check
#   test-samir-ovl-static       the shipped link re-packed byte-identically
#                               (all packer checks pass, deterministic)
#   test-samir-ovl-static-mutant  overlay->overlay (CROSS) and resident->overlay
#                               (RESREF) refused by the linker; overlay code
#                               calling samir_do (REENTER), which reaches the
#                               manager through exec_command, refused by the
#                               packer's call-graph rule
#   test-samir-ovl-emu          QEMU: right SAMIR.OVL runs; missing / stale /
#                               corrupt SAMIR.OVL -> the Insert System Disk
#                               prompt, ENTER retries, Ctrl-C exits with 1
#   test-samir-ovl-emu-mutant   SKIP_OPEN (missing), NO_STAMP (stale), NO_CRC
#                               (corrupt) each RED
#   test-samir-ovl-swap-emu     QEMU: the real-III+ DML-status program, which
#                               alternates the SET, STATUS and FILE overlays,
#                               byte-identical to real dBASE III PLUS
#   test-samir-ovl-swap-emu-mutant  NO_SWAP (first overlay answers every call)
#                               and NO_RELOC (overlay not relocated) each RED
TEST_SAMIR_OVL     := $(BUILD)/test_samir_ovl
TEST_SAMIR_OVL_EMU := $(BUILD)/test_samir_ovl_emu
SAMIR_OVL_TEST_DEPS := $(DBF_DIFF_DIR)/test_samir_ovl.c $(SAMIR_OVL_SRC) $(SAMIR_RT_SRC) $(SAMIR_INC_DIR)/samir/ovl.h spec/samir/ovl_format.h $(DBF_DIFF_DIR)/samir_install.h
$(TEST_SAMIR_OVL): $(SAMIR_OVL_TEST_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $(DBF_DIFF_DIR)/test_samir_ovl.c $(SAMIR_OVL_SRC) $(SAMIR_RT_SRC)
$(BUILD)/test_samir_ovl_%: $(SAMIR_OVL_TEST_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DSAMIR_OVL_MUTATE_$* -Iseed -I$(SAMIR_INC_DIR) -Ispec -o $@ $(DBF_DIFF_DIR)/test_samir_ovl.c $(SAMIR_OVL_SRC) $(SAMIR_RT_SRC)
$(TEST_SAMIR_OVL_EMU): $(DBF_DIFF_DIR)/test_samir_ovl_emu.c $(DBF_DIFF_DIR)/samir_install.h spec/samir/ovl_format.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Iseed -Ispec -o $@ $<

.PHONY: test-samir-ovl test-samir-ovl-mutant test-samir-ovl-static test-samir-ovl-static-mutant \
        test-samir-ovl-emu test-samir-ovl-emu-mutant test-samir-ovl-swap-emu test-samir-ovl-swap-emu-mutant
test-samir-ovl: $(TEST_SAMIR_OVL) $(SAMIR_EXE)
	@$(TEST_SAMIR_OVL) $(SAMIR_EXE)
	@printf '>>> test-samir-ovl: green (manager pure parts on literal bytes; shipped SAMIR.EXE/SAMIR.OVL pair consistent)\n'
test-samir-ovl-mutant: test-samir-ovl $(BUILD)/test_samir_ovl_NO_RELOC $(BUILD)/test_samir_ovl_NO_STAMP
	@mkdir -p $(BUILD)/samir-ovl
	@set -e; for m in NO_RELOC:'B2 site 0' NO_STAMP:'B3 STAMP'; do k=$${m%%:*}; want=$${m#*:}; \
	  set +e; $(BUILD)/test_samir_ovl_$$k $(SAMIR_EXE) > $(BUILD)/samir-ovl/mut-$$k.log 2>&1; rc=$$?; set -e; \
	  test $$rc -ne 0 && grep -q "FAIL.*$$want" $(BUILD)/samir-ovl/mut-$$k.log || { cat $(BUILD)/samir-ovl/mut-$$k.log; exit 1; }; \
	  printf 'SAMIR_OVL_MUTATE_%s RED: ' $$k; grep -m1 "FAIL.*$$want" $(BUILD)/samir-ovl/mut-$$k.log; done
	@printf '>>> test-samir-ovl-mutant: green (NO_RELOC, NO_STAMP RED)\n'

# The shipped link, re-packed: every packer check passes again and the
# outputs are byte-identical to the shipped pair (Rule 11).
test-samir-ovl-static: $(SAMIR_EXE) $(SAMIR_PACK_DEPS)
	@mkdir -p $(BUILD)/samir-ovl
	@cp -f $(BUILD)/samir_exe/SAMIR.elf $(BUILD)/samir-ovl/SAMIR.elf
	@$(call samir_pack,$(BUILD)/samir-ovl/SAMIR.elf,$(BUILD)/samir-ovl/REPACK.EXE)
	@cmp $(SAMIR_EXE) $(BUILD)/samir-ovl/REPACK.EXE
	@cmp $(basename $(SAMIR_EXE)).OVL $(BUILD)/samir-ovl/REPACK.OVL
	@printf '>>> test-samir-ovl-static: green (relocation kinds, slot, headers, call-graph rule; re-pack byte-identical)\n'
# Each mutant build must FAIL, for its named reason.
test-samir-ovl-static-mutant: test-samir-ovl-static
	@mkdir -p $(BUILD)/samir-ovl
	@set -e; for m in CROSS:'prohibited cross reference from .ovl_report to' \
	                  RESREF:'prohibited cross reference from .text to' \
	                  REENTER:'<- samir_do <- report_ovl_entry'; do \
	  k=$${m%%:*}; want=$${m#*:}; log=$(BUILD)/samir-ovl/static-$$k.log; \
	  rm -f $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE; \
	  if $(MAKE) --no-print-directory $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE > $$log 2>&1; then \
	    printf '!!! test-samir-ovl-static-mutant: %s built -- the check is decoration\n' $$k; exit 1; fi; \
	  grep -q -- "$$want" $$log || { cat $$log; exit 1; }; \
	  printf 'SAMIR_OVL_MUTATE_%s RED: ' $$k; grep -m1 -- "$$want" $$log; done
	@printf '>>> test-samir-ovl-static-mutant: green (CROSS, RESREF refused by the link; REENTER refused by the call-graph rule)\n'

# The stale overlay file is another real build's: the REPORT FORM subtotal
# mutant's SAMIR.OVL (self-consistent, differently stamped).
SAMIR_OVL_STALE := $(basename $(SAMIR_REPORT_MUT_EXE)).OVL
SAMIR_OVL_EMU_DEPS := $(TEST_SAMIR_OVL_EMU) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_REPORT_MUT_EXE)
test-samir-ovl-emu: $(SAMIR_OVL_EMU_DEPS) $(SAMIR_EXE)
	@set -e; for m in ok missing stale corrupt; do \
	  INITECH_QMP_STDIO=1 $(TEST_SAMIR_OVL_EMU) $$m $(SAMIR_EXE) $(SAMIR_OVL_STALE); done
	@printf '>>> test-samir-ovl-emu: green (right SAMIR.OVL runs; missing/stale/corrupt -> Insert System Disk, ENTER retries, Ctrl-C exits 1)\n'
test-samir-ovl-emu-mutant: test-samir-ovl-emu $(SAMIR_OVL_EMU_DEPS) \
		$(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_SKIP_OPEN.EXE $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_STAMP.EXE \
		$(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_CRC.EXE
	@mkdir -p $(BUILD)/samir-ovl
	@set -e; for m in SKIP_OPEN:missing NO_STAMP:stale NO_CRC:corrupt; do k=$${m%%:*}; mode=$${m#*:}; \
	  log=$(BUILD)/samir-ovl/emu-mut-$$k.log; \
	  set +e; INITECH_QMP_STDIO=1 $(TEST_SAMIR_OVL_EMU) $$mode $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE $(SAMIR_OVL_STALE) > $$log 2>&1; rc=$$?; set -e; \
	  test $$rc -ne 0 && grep -q "FAIL.*$$mode: Insert System Disk prompt shown" $$log || { cat $$log; exit 1; }; \
	  printf 'SAMIR_OVL_MUTATE_%s (%s) emu RED: ' $$k $$mode; grep -m1 "FAIL.*$$mode: Insert System Disk" $$log; done
	@printf '>>> test-samir-ovl-emu-mutant: green (SKIP_OPEN, NO_STAMP, NO_CRC RED)\n'

test-samir-ovl-swap-emu: $(SAMIR_DML_TEST) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(SAMIR_EXE) $(CLIENTS_DBF) samir-dml-goldens
	@INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(SAMIR_EXE)
	@printf '>>> test-samir-ovl-swap-emu: green (SET/STATUS/FILE overlays alternate; transcript byte-identical to real III+)\n'
test-samir-ovl-swap-emu-mutant: test-samir-ovl-swap-emu \
		$(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_SWAP.EXE $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_NO_RELOC.EXE
	@mkdir -p $(BUILD)/samir-ovl
	@set -e; for k in NO_SWAP NO_RELOC; do log=$(BUILD)/samir-ovl/swap-mut-$$k.log; \
	  set +e; INITECH_QMP_STDIO=1 $(SAMIR_DML_TEST) status emu $(BUILD)/SAMIR_SCREEN_SAMIR_OVL_MUTATE_$$k.EXE > $$log 2>&1; rc=$$?; set -e; \
	  test $$rc -ne 0 && grep -q 'FAIL' $$log || { cat $$log; exit 1; }; \
	  printf 'SAMIR_OVL_MUTATE_%s swap RED: ' $$k; grep -m1 'FAIL' $$log; done
	@printf '>>> test-samir-ovl-swap-emu-mutant: green (NO_SWAP, NO_RELOC RED)\n'

TEST_UNIT_GATES += test-samir-ovl test-samir-ovl-mutant test-samir-ovl-static test-samir-ovl-static-mutant
TEST_EMU_GATES += test-samir-ovl-emu test-samir-ovl-emu-mutant test-samir-ovl-swap-emu test-samir-ovl-swap-emu-mutant

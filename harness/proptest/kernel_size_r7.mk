# Kernel size round 7 (initech-5qsm): the whole-image struct-return convention.
# Ref: PRD Sec 5/6.1/8; CLAUDE.md Rule 2/6/11; Makefile KERNEL_ABI_FLAGS note;
# the WL-0168 -mregparm/-mrtd audit this gate turns into a standing check.
#   test-kernel-abi-struct-return: (1) every -m32 compile and the LTO link of
#   the flagship, as `make -n -B` prints them, resolves to -freg-struct-return
#   (the LAST of -freg-/-fpcc-struct-return on the line wins, as in GCC), so no
#   kernel object can disagree with its callers about where an 8-byte
#   rgn_rect_t comes back; (2) the flagship holds no dynamic stack realignment
#   (`and esp,0xfffffff8`) -- the cost the convention removed.
#   test-kernel-abi-struct-return-mutant: the region engine alone compiled
#   -fpcc-struct-return (a mixed-ABI image) -> (1) RED naming region.c.
KERNEL_R7_DIR := $(BUILD)/ksize7
$(KERNEL_R7_DIR): | $(BUILD)
	@mkdir -p $@

# $(1) = extra make arguments, $(2) = output stem. Fails loud on any line.
define kernel-abi-struct-audit
$(MAKE) --no-print-directory -n -B $(1) $(KERNEL_FLAIRTENANTS_ELF) 2>/dev/null \
	| grep -E '^gcc -m32 ' > $(KERNEL_R7_DIR)/$(2).lines; \
n=$$(wc -l < $(KERNEL_R7_DIR)/$(2).lines); [ "$$n" -gt 40 ] || { printf '!!! kernel-abi-struct-return FAIL: only %s compile/link lines captured\n' "$$n"; exit 1; }; \
bad=$$(awk '{ w = ""; for (i = 1; i <= NF; i++) if ($$i == "-freg-struct-return" || $$i == "-fpcc-struct-return") w = $$i; \
	if (w != "-freg-struct-return") { src = "link"; for (i = 1; i <= NF; i++) if ($$i == "-c") src = $$(i+1); print src } }' $(KERNEL_R7_DIR)/$(2).lines); \
if [ -n "$$bad" ]; then printf '!!! kernel-abi-struct-return FAIL: not -freg-struct-return: %s\n' $$bad; exit 1; fi; \
printf '    %s kernel compile/link lines, every one -freg-struct-return\n' "$$n"
endef

.PHONY: test-kernel-abi-struct-return test-kernel-abi-struct-return-mutant
test-kernel-abi-struct-return: $(KERNEL_FLAIRTENANTS_ELF) | $(KERNEL_R7_DIR)
	@set -eu; $(call kernel-abi-struct-audit,,real)
	@n=$$(objdump -d -M intel $(KERNEL_FLAIRTENANTS_ELF) | grep -c 'and    esp,0xfffffff8' || true); \
	if [ "$$n" -ne 0 ]; then printf '!!! kernel-abi-struct-return FAIL: %s dynamic stack realignments in the flagship\n' "$$n"; exit 1; fi
	@printf '>>> test-kernel-abi-struct-return: green (one struct-return convention on every kernel object + the link; 0 stack realignments)\n'

test-kernel-abi-struct-return-mutant: test-kernel-abi-struct-return
	@if ( set -eu; $(call kernel-abi-struct-audit,FLAIR_REGION_OPT='-Oz -fomit-frame-pointer -fpcc-struct-return',mutant) ) > $(KERNEL_R7_DIR)/mutant.log 2>&1; then \
		printf '!!! test-kernel-abi-struct-return-mutant FAIL: region_pcc survived\n'; exit 1; fi
	@grep -x '!!! kernel-abi-struct-return FAIL: not -freg-struct-return: os/flair/atkinson/region.c' $(KERNEL_R7_DIR)/mutant.log
	@printf '>>> test-kernel-abi-struct-return-mutant: green (region_pcc RED)\n'
TEST_UNIT_GATES += test-kernel-abi-struct-return test-kernel-abi-struct-return-mutant

# The FAT walkers' common guard as one body (os/milton/fat12.c
# chain_link_unusable, twelve sites). Its load-bearing clause is the data-range
# bound (free 0x000 and bad 0xFF7/0xFFF7 are themselves outside the range, so
# dropping those two clauses is an equivalent mutant no oracle can see). The
# corrupt-input fuzzer must go RED when the range bound is dropped.
$(KERNEL_R7_DIR)/fat12_corrupt_fuzz_mut_chain: $(FAT_DIFF_DIR)/fat12_corrupt_fuzz.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(KERNEL_R7_DIR)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DFAT12_MUT_CHAIN_GUARD -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) \
		-o $@ $(FAT_DIFF_DIR)/fat12_corrupt_fuzz.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
.PHONY: test-fat-corrupt-fuzz-chain-mutant
test-fat-corrupt-fuzz-chain-mutant: test-fat-corrupt-fuzz $(KERNEL_R7_DIR)/fat12_corrupt_fuzz_mut_chain $(FAT12_FUZZ_BLANK_IMG)
	@if timeout 120 $(KERNEL_R7_DIR)/fat12_corrupt_fuzz_mut_chain --sweep $(FAT12_CORRUPT_SEEDS_LO) $(FAT12_CORRUPT_SEEDS_HI) --ops $(FAT12_CORRUPT_OPS) "$(FAT12_FUZZ_BLANK_IMG)" >$(KERNEL_R7_DIR)/chain_mutant.log 2>&1; then \
		rm -f "$(FAT12_FUZZ_BLANK_IMG)".cseed*.scratch; printf '!!! test-fat-corrupt-fuzz-chain-mutant FAIL: FAT12_MUT_CHAIN_GUARD survived\n'; exit 1; fi
	@rm -f "$(FAT12_FUZZ_BLANK_IMG)".cseed*.scratch
	@grep -m1 'VIOLATION.*out-of-range' $(KERNEL_R7_DIR)/chain_mutant.log
	@printf '>>> test-fat-corrupt-fuzz-chain-mutant: green (FAT12_MUT_CHAIN_GUARD RED)\n'
TEST_UNIT_GATES += test-fat-corrupt-fuzz-chain-mutant

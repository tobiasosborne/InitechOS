# Kernel size round 5 (WL-0168): storage-only reclaims, each with a host
# equality tooth and a named Rule-6 mutant. Ref: PRD Sec 5/6.4/8; CLAUDE.md
# Rule 6/8/11; the round-4 chicago_pack shape (kernel_size_r4.mk, WL-0152).
KERNEL_R5_DIR := $(BUILD)/ksize5
$(KERNEL_R5_DIR): | $(BUILD)
	@mkdir -p $@

# ---------------------------------------------------------------------------
# Packed strike storage (os/flair/asset_lz.h + the COMMITTED, generated
# os/flair/asset_lz_data.h). The locked spec/assets headers stay the only
# source of every byte; the freestanding kernel reads the packed copy.
#   test-asset-lz: (1) DRIFT -- the packer regenerates the committed header
#   from the locked headers, byte for byte; (2) EQUALITY -- the kernel's own
#   decoder unpacks every block == the unchanged locked tables.
#   test-asset-lz-mutant: ASSET_LZ_MUT_BYTE flips one decoded byte -> RED.
# A locked strike edit (Rule 8) makes (1) RED until `make gen-asset-lz`.
ASSET_LZ_TOOL := $(BUILD)/asset_lz
ASSET_LZ_DATA := os/flair/asset_lz_data.h
ASSET_LZ_LOCKED := spec/assets/desk_icons.h spec/assets/finder_icons.h \
                   spec/assets/alert_icons.h spec/assets/finder_small_icons.h \
                   spec/assets/boot_logo.h
$(ASSET_LZ_TOOL): tools/asset_lz.c $(ASSET_LZ_LOCKED) | $(BUILD)
	$(CC) $(CFLAGS) -O2 -Ispec/assets -o $@ tools/asset_lz.c

.PHONY: gen-asset-lz test-asset-lz test-asset-lz-mutant
gen-asset-lz: $(ASSET_LZ_TOOL)
	$(ASSET_LZ_TOOL) >$(ASSET_LZ_DATA).tmp && mv -f $(ASSET_LZ_DATA).tmp $(ASSET_LZ_DATA)

test-asset-lz: $(ASSET_LZ_TOOL) | $(KERNEL_R5_DIR)
	@$(ASSET_LZ_TOOL) >$(KERNEL_R5_DIR)/asset_lz_data.h 2>$(KERNEL_R5_DIR)/asset_lz.sizes
	@cmp -s $(KERNEL_R5_DIR)/asset_lz_data.h $(ASSET_LZ_DATA) || { \
		printf '!!! asset-lz FAIL: %s is stale against the locked strikes -- run make gen-asset-lz\n' '$(ASSET_LZ_DATA)'; exit 1; }
	@cat $(KERNEL_R5_DIR)/asset_lz.sizes
	$(CC) $(CFLAGS) -Ispec/assets -Ios/flair harness/proptest/test_asset_lz.c -o $(KERNEL_R5_DIR)/asset-lz
	@$(KERNEL_R5_DIR)/asset-lz

test-asset-lz-mutant: test-asset-lz
	$(CC) $(CFLAGS) -DASSET_LZ_MUT_BYTE -Ispec/assets -Ios/flair harness/proptest/test_asset_lz.c -o $(KERNEL_R5_DIR)/asset-lz-mut
	@if $(KERNEL_R5_DIR)/asset-lz-mut >$(KERNEL_R5_DIR)/asset-lz-mut.log 2>&1; then \
		printf '!!! test-asset-lz-mutant FAIL: asset_lz_byte survived\n'; exit 1; fi
	@grep -m1 '^!!! asset-lz FAIL:' $(KERNEL_R5_DIR)/asset-lz-mut.log
	@printf '>>> test-asset-lz-mutant: green (asset_lz_byte RED)\n'

TEST_UNIT_GATES += test-asset-lz test-asset-lz-mutant

# The packed headers are prerequisites of every object that compiles a
# packed reader (incl. the -D mutant and freestanding-check objects), the
# line-10475 .EXTRA_PREREQS idiom for kmain variants (boot_screen.h).
ASSET_LZ_HDRS := os/flair/asset_lz.h $(ASSET_LZ_DATA)
$(BUILD)/finder_desktop%.o $(BUILD)/dialog%.o $(BUILD)/finder_icon%.o $(BUILD)/kmain%.o: private .EXTRA_PREREQS += $(ASSET_LZ_HDRS)

# ---------------------------------------------------------------------------
# flair_part_map compaction: the authored derived colours live once in
# flair_derived_rgb[] (named by a row ordinal) instead of a 4-byte field in
# all 79 rows. test-flair-part-map diffs every PART x depth x skin resolution
# against fixtures/flair_part_map.golden, captured ONCE from the pre-change
# implementation (commit 4d907b1f) -- a port-and-verify differential.
#   test-flair-part-map-mutant: FLAIR_PART_MAP_MUT_DERIVED names the wrong
#   derived ordinal for one row -> RED.
PART_MAP_LINK := harness/proptest/test_flair_part_map.c $(FLAIRLOOK_C) os/flair/surface.c
PART_MAP_INC  := -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson
.PHONY: test-flair-part-map test-flair-part-map-mutant
test-flair-part-map: | $(KERNEL_R5_DIR)
	$(CC) $(CFLAGS) $(PART_MAP_INC) $(PART_MAP_LINK) -o $(KERNEL_R5_DIR)/part-map
	@$(KERNEL_R5_DIR)/part-map >$(KERNEL_R5_DIR)/part-map.txt
	@diff -u harness/proptest/fixtures/flair_part_map.golden $(KERNEL_R5_DIR)/part-map.txt >$(KERNEL_R5_DIR)/part-map.diff || { \
		head -20 $(KERNEL_R5_DIR)/part-map.diff; printf '!!! flair-part-map FAIL: PART resolution differs from the pre-change table\n'; exit 1; }
	@printf '>>> test-flair-part-map: green (%s PART x depth x skin resolutions == the pre-change table)\n' "$$(wc -l < $(KERNEL_R5_DIR)/part-map.txt)"
test-flair-part-map-mutant: test-flair-part-map
	$(CC) $(CFLAGS) -DFLAIR_PART_MAP_MUT_DERIVED $(PART_MAP_INC) $(PART_MAP_LINK) -o $(KERNEL_R5_DIR)/part-map-mut
	@$(KERNEL_R5_DIR)/part-map-mut >$(KERNEL_R5_DIR)/part-map-mut.txt
	@if cmp -s harness/proptest/fixtures/flair_part_map.golden $(KERNEL_R5_DIR)/part-map-mut.txt; then \
		printf '!!! test-flair-part-map-mutant FAIL: part_map_derived survived\n'; exit 1; fi
	@printf '!!! flair-part-map FAIL: part_map_derived changes the resolution table\n'
	@printf '>>> test-flair-part-map-mutant: green (part_map_derived RED)\n'

TEST_UNIT_GATES += test-flair-part-map test-flair-part-map-mutant

# initech-qqrz: host differential and live injected-key gates, before the fix.
COPY_STAMP_DIR := harness/diff/fat_diff
$(MAKEFILE_STAMP): $(COPY_STAMP_DIR)/copy_stamp.mk
$(BUILD)/dos_copy_stamp_host_mut: $(DOS_PARITY_SRC) $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -DINT21_MUTATE_FILETIME_GET_SWAP -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -Ibuild -o $@ $< $(TEST_FILEIO_SUBDIR_DEPS)
$(eval $(call dos-safety-shell-mutant,copytime,CMD_MUTATE_COPY_TIME))
$(eval $(call dos-safety-shell-mutant,copydate,CMD_MUTATE_COPY_DATE))
$(eval $(call zuvo-com,xcopy_stamp_mut,-DDU_MUT_TIME,))

.PHONY: test-dos-stamp-copy-host test-dos-stamp-copy-host-mutant test-dos-stamp-copy test-dos-stamp-copy-emu-mutant
test-dos-stamp-copy-host: $(BUILD)/dos_parity_host $(ZUVO_HOST)
	@sh $(COPY_STAMP_DIR)/copy_stamp.sh host
test-dos-stamp-copy-host-mutant: $(BUILD)/dos_parity_host $(BUILD)/dos_copy_stamp_host_mut $(ZUVO_HOST) $(BUILD)/test_xcopy_tree_mut_TIME
	@set -eu; for m in GET_SWAP XCOPY_TIME; do \
	  h=$(BUILD)/dos_parity_host; x=$(ZUVO_HOST); \
	  case $$m in GET_SWAP) h=$(BUILD)/dos_copy_stamp_host_mut;; XCOPY_TIME) x=$(BUILD)/test_xcopy_tree_mut_TIME;; esac; \
	  log=$(BUILD)/copy_stamp_host_$$m.log; \
	  if COPY_STAMP_HOST=$$h COPY_STAMP_XHOST=$$x sh $(COPY_STAMP_DIR)/copy_stamp.sh host unused unused $$m > $$log 2>&1; then echo "FAIL copy stamp: $$m survived"; exit 1; fi; \
	  grep '^copy-stamp RED:.*stamp' $$log || { cat $$log; exit 1; }; \
	  echo "PASS copy-stamp host mutant: $$m caught RED"; \
	done
test-dos-stamp-copy: $(BUILD)/dos_parity_host $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN)
	@sh $(COPY_STAMP_DIR)/copy_stamp.sh emu $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN)
test-dos-stamp-copy-emu-mutant: $(BUILD)/dos_parity_host $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN) $(BUILD)/tracer_mut_safety_copytime.img $(BUILD)/tracer_mut_safety_copydate.img $(BUILD)/xcopy_stamp_mut_program.bin
	@set -eu; for m in COPY_TIME COPY_DATE XCOPY_TIME; do \
	  boot=$(SECONDARY_TRACER_IMG); x=$(XCOPY_PROG_BIN); \
	  case $$m in COPY_TIME) boot=$(BUILD)/tracer_mut_safety_copytime.img;; COPY_DATE) boot=$(BUILD)/tracer_mut_safety_copydate.img;; XCOPY_TIME) x=$(BUILD)/xcopy_stamp_mut_program.bin;; esac; \
	  log=$(BUILD)/copy_stamp_emu_$$m.log; \
	  if sh $(COPY_STAMP_DIR)/copy_stamp.sh emu $$boot $$x $$m > $$log 2>&1; then echo "FAIL copy stamp: $$m survived"; exit 1; fi; \
	  grep '^copy-stamp RED:.*stamp' $$log || { cat $$log; exit 1; }; \
	  echo "PASS copy-stamp emulator mutant: $$m booted, completed and caught RED"; \
	done
TEST_UNIT_GATES += test-dos-stamp-copy-host test-dos-stamp-copy-host-mutant
TEST_EMU_GATES += test-dos-stamp-copy test-dos-stamp-copy-emu-mutant

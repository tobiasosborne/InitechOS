# Four-bug DOS lane. All gates and named mutants join the PM vectors.
BATCH2_SRC := harness/diff/fat_diff/batch2_host.c
$(BUILD)/batch2_host: $(BATCH2_SRC) $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) $(MILTON_DIR)/batch.c | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -Ibuild -o $@ $< $(TEST_FILEIO_SUBDIR_DEPS) $(MILTON_DIR)/batch.c

$(eval $(call dos-safety-shell-mutant,batch2body,CMD_MUTATE_BATCH_BODY))
$(eval $(call dos-safety-shell-mutant,batch2shift,CMD_MUTATE_SHIFT_ZERO))
$(eval $(call dos-safety-shell-mutant,batch2date,CMD_MUTATE_PROMPT_DATE))
$(eval $(call dos-safety-shell-mutant,batch2dollar,CMD_MUTATE_PROMPT_DOLLAR))
$(eval $(call dos-safety-shell-mutant,batch2for,CMD_MUTATE_FOR_PROMPT))
$(eval $(call dos-safety-shell-mutant,batch2wild,CMD_MUTATE_FOR_WILDCARD))
BATCH2_INT_KNOB_cwd := INT21_MUTATE_CD_ROOT
BATCH2_INT_KNOB_ctrlz := INT21_MUTATE_CARET_Z
BATCH2_INT_KNOB_ctrlc := INT21_MUTATE_CARET_C
$(BUILD)/int21_batch2_%.o: $(KERNEL_INT21_C) $(TEST_DEVWIRE_HDRS) | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_INT21_OPT) -D$(BATCH2_INT_KNOB_$*) -Ispec -I$(KERNEL_DIR) -I$(BUILD) -c $< -o $@
$(foreach m,cwd ctrlz ctrlc,$(eval $(call j1lu-emu-variant,batch2$(m),$(KERNEL_INT21_OBJ),$(BUILD)/int21_batch2_$(m).o)))

.PHONY: test-dos-batch2-host test-dos-batch2-body test-dos-batch2-cwd test-dos-batch2-prompt test-dos-batch2-for test-dos-batch2-console test-dos-batch2-mutant
test-dos-batch2-host: $(BUILD)/batch2_host
	@sh harness/diff/fat_diff/batch2.sh host
test-dos-batch2-body test-dos-batch2-cwd test-dos-batch2-prompt test-dos-batch2-for test-dos-batch2-console: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/console_text
	@sh harness/diff/fat_diff/batch2.sh $(patsubst test-dos-batch2-%,%,$@) $(SECONDARY_TRACER_IMG)

test-dos-batch2-mutant: $(HARNESS_BIN) $(BUILD)/console_text $(addprefix $(BUILD)/tracer_mut_safety_batch2,body.img shift.img date.img dollar.img for.img wild.img) $(addprefix $(BUILD)/j1lu_batch2,cwd.img ctrlz.img ctrlc.img)
	@set -e; for pair in body:body shift:body date:prompt dollar:prompt for:for wild:for cwd:cwd ctrlz:console ctrlc:console; do \
	  m=$${pair%:*}; mode=$${pair#*:}; boot=$(BUILD)/tracer_mut_safety_batch2$$m.img; \
	  case $$m in cwd|ctrlz|ctrlc) boot=$(BUILD)/j1lu_batch2$$m.img;; esac; \
	  log=$(BUILD)/batch2_mutant_$$m.log; \
	  if sh harness/diff/fat_diff/batch2.sh $$mode $$boot $$m >$$log 2>&1; then echo "FAIL batch2 mutant survived: $$m"; exit 1; fi; \
	  case $$m in \
	    body) why='IF/FOR body bypassed batch dispatcher';; shift) why='missing SHIFT-first-second';; \
	    date) why='prompt date did not read clock';; dollar) why='literal dollar truncated prompt';; \
	    for) why='interactive FOR differs from mdir';; wild) why='batch wildcard set differs from mdir';; \
	    cwd) why='CD root/parent/absolute/getcwd';; ctrlz) why='console contains an undecodable control glyph';; \
	    ctrlc) why='Ctrl-C did not cancel input with ^C';; esac; \
	  grep -F "batch2-$$mode RED: $$why" $$log; \
	  printf 'PASS batch2 mutant %s caught RED\n' "$$m"; \
	done
TEST_UNIT_GATES += test-dos-batch2-host
TEST_EMU_GATES += test-dos-batch2-body test-dos-batch2-cwd test-dos-batch2-prompt test-dos-batch2-for test-dos-batch2-console test-dos-batch2-mutant

$(BUILD)/batch2_host_%: $(BATCH2_SRC) $(TEST_FILEIO_SUBDIR_DEPS) $(TEST_FILEIO_SUBDIR_HDRS) $(MILTON_DIR)/batch.c | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -no-pie -D$(BATCH2_INT_KNOB_$*) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -Ibuild -o $@ $< $(TEST_FILEIO_SUBDIR_DEPS) $(MILTON_DIR)/batch.c
.PHONY: test-dos-batch2-host-mutant
test-dos-batch2-host-mutant: $(addprefix $(BUILD)/batch2_host_,cwd ctrlz ctrlc)
	@set -e; for m in cwd ctrlz ctrlc; do \
	  log=$(BUILD)/batch2_host_mutant_$$m.log; \
	  if BATCH2_HOST=$(BUILD)/batch2_host_$$m sh harness/diff/fat_diff/batch2.sh host unused $$m >$$log 2>&1; then echo "FAIL batch2 host mutant survived: $$m"; exit 1; fi; \
	  grep 'batch2-host RED: \(CD root\|Ctrl-Z caret\|Ctrl-C caret\)' $$log; \
	  printf 'PASS batch2 host mutant %s caught RED\n' "$$m"; \
	done
TEST_UNIT_GATES += test-dos-batch2-host-mutant

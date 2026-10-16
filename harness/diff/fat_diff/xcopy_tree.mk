# initech-zuvo: gates authored before implementation (Rule 1).
ZUVO_DIR := harness/diff/fat_diff
ZUVO_SRC := os/milton/xcopy_tree.c
ZUVO_HOST_SRC := $(ZUVO_DIR)/test_xcopy_tree.c $(ZUVO_SRC) $(TEST_FILEIO_SUBDIR_DEPS)
ZUVO_INC := -Ispec -Ios/milton -Ibuild -I$(ZUVO_DIR)
ZUVO_MUTANTS := S E A M D P V W TARGET COUNT TREE_F TREE_LAYOUT DEFAULT WILD DESCEND TIME HIDDEN E_REQUIRES_S CYCLIC SELF LABEL OMIT CTRL_C DATE_RANGE
ZUVO_HOST := $(BUILD)/test_xcopy_tree
$(MAKEFILE_STAMP): $(ZUVO_DIR)/xcopy_tree.mk
$(ZUVO_HOST): $(ZUVO_HOST_SRC) os/milton/xcopy_tree.h $(DOS_MESSAGES_H)
	$(CC) $(CFLAGS) -no-pie $(ZUVO_INC) -o $@ $(ZUVO_HOST_SRC)

define zuvo-mutant
$(BUILD)/test_xcopy_tree_mut_$(1): $(ZUVO_HOST_SRC) os/milton/xcopy_tree.h $(DOS_MESSAGES_H)
	$(CC) $(CFLAGS) -no-pie $(ZUVO_INC) -DDU_MUT_$(1) -o $$@ $(ZUVO_HOST_SRC)
endef
$(foreach m,$(ZUVO_MUTANTS),$(eval $(call zuvo-mutant,$(m))))

ZUVO_COM_FLAGS := $(KERNEL_CFLAGS) -Os -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -Ios/milton -Ispec
define zuvo-com
$(BUILD)/$(1)_program.o: $(ZUVO_SRC) os/milton/xcopy_tree.h
	$(KERNEL_CC) $(ZUVO_COM_FLAGS) $(2) -c $$< -o $$@
$(BUILD)/$(1)_entry.o: os/milton/xcopy_tree_entry.asm
	$(NASM) -f elf32 $(3) $$< -o $$@
$(BUILD)/$(1)_program.elf: $(BUILD)/$(1)_program.o $(BUILD)/$(1)_entry.o os/milton/xcopy_tree.ld
	$(LD) -m elf_i386 --no-warn-rwx-segments -T os/milton/xcopy_tree.ld -o $$@ $(BUILD)/$(1)_entry.o $(BUILD)/$(1)_program.o
$(BUILD)/$(1)_program.bin: $(BUILD)/$(1)_program.elf
	$(OBJCOPY) -O binary $$< $$@
endef
$(eval $(call zuvo-com,xcopy,,))
$(eval $(call zuvo-com,tree,,-DTREE_PROGRAM))
$(eval $(call zuvo-com,xcopy_mut,-DDU_MUT_S,))
$(eval $(call zuvo-com,tree_mut,-DDU_MUT_TREE_LAYOUT,-DTREE_PROGRAM))
$(eval $(call zuvo-com,xcopy_ctrlc,-DDU_MUT_CTRL_C,))

.PHONY: test-xcopy-tree test-xcopy-tree-mutant test-xcopy-tree-emu test-xcopy-tree-emu-mutant test-xcopy-tree-emu-cancel
test-xcopy-tree: $(ZUVO_HOST)
	@sh $(ZUVO_DIR)/xcopy_tree.sh host $(ZUVO_HOST) $(BUILD)/zuvo_host
test-xcopy-tree-mutant: $(foreach m,$(ZUVO_MUTANTS),$(BUILD)/test_xcopy_tree_mut_$(m))
	@set -e; for m in $(ZUVO_MUTANTS); do \
	  log=$(BUILD)/zuvo_mut_$$m.log; \
	  if sh $(ZUVO_DIR)/xcopy_tree.sh host $(BUILD)/test_xcopy_tree_mut_$$m $(BUILD)/zuvo_mut_$$m > $$log 2>&1; then echo "FAIL: $$m mutant survived"; exit 1; fi; \
	  grep '^XCOPY/TREE RED:' $$log; \
	  echo "XCOPY/TREE MUTANT GREEN: $$m caught"; \
	done
	@echo 'XCOPY/TREE MUTANTS GREEN: 24 named mutants caught'
test-xcopy-tree-emu: $(ZUVO_HOST) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
	@sh $(ZUVO_DIR)/xcopy_tree.sh emu $(ZUVO_HOST) $(BUILD)/zuvo_emu $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
test-xcopy-tree-emu-cancel: $(ZUVO_HOST) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
	@sh $(ZUVO_DIR)/xcopy_tree.sh cancel $(ZUVO_HOST) $(BUILD)/zuvo_cancel $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
test-xcopy-tree-emu-mutant: $(ZUVO_HOST) $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/xcopy_mut_program.bin $(BUILD)/tree_mut_program.bin $(BUILD)/xcopy_ctrlc_program.bin $(XCOPY_PROG_BIN) $(TREE_PROG_BIN)
	@set -e; for m in xcopy tree ctrlc; do \
	  x=$(XCOPY_PROG_BIN); t=$(TREE_PROG_BIN); \
	  mode=emu; \
	  case $$m in xcopy) x=$(BUILD)/xcopy_mut_program.bin;; tree) t=$(BUILD)/tree_mut_program.bin;; ctrlc) x=$(BUILD)/xcopy_ctrlc_program.bin; mode=cancel;; esac; \
	  log=$(BUILD)/zuvo_emu_mut_$$m.log; work=$(BUILD)/zuvo_emu_mut_$$m; \
	  if sh $(ZUVO_DIR)/xcopy_tree.sh $$mode $(ZUVO_HOST) $$work $(SECONDARY_TRACER_IMG) $(HARNESS_BIN) $$x $$t > $$log 2>&1; then echo "FAIL: $$m live mutant survived"; exit 1; fi; \
	  grep -q '^ZUVO-DONE' $$work/zuvo_emu_mut_$$m.serial; \
	  grep '^XCOPY/TREE RED:.*\(directory membership\|TREE manual layout\|Ctrl-C exit\)' $$log; \
	  echo "XCOPY/TREE EMU MUTANT GREEN: $$m booted and caught"; \
	done
	@echo 'XCOPY/TREE EMU MUTANTS GREEN: S TREE_LAYOUT CTRL_C booted and caught'
TEST_UNIT_GATES += test-xcopy-tree test-xcopy-tree-mutant
TEST_EMU_GATES += test-xcopy-tree-emu test-xcopy-tree-emu-mutant test-xcopy-tree-emu-cancel

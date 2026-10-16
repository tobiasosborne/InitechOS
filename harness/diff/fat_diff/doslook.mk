# LOOK pass 6 DOS lane: independent manual + mdir + live LFB gates.
DOSLOOK_DIR := harness/diff/fat_diff
DOSLOOK_HOST_SRC := $(DOSLOOK_DIR)/test_dir_look.c os/milton/command.c os/milton/env.c
DOSLOOK_DISK_SRC := $(DOSLOOK_DIR)/test_dir_disk.c $(DOSLOOK_DIR)/blockdev_file.c os/milton/fat12.c os/milton/fileio_fat.c
DOSLOOK_INC := -Ispec -Ios/milton -Ibuild -I$(DOSLOOK_DIR)
$(MAKEFILE_STAMP): $(DOSLOOK_DIR)/doslook.mk

$(BUILD)/test_dir_look: $(DOSLOOK_HOST_SRC) $(DOS_MESSAGES_H) os/milton/command.h
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -o $@ $(DOSLOOK_HOST_SRC)
$(BUILD)/test_dir_disk: $(DOSLOOK_DISK_SRC) os/milton/fileio_fat.h
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -o $@ $(DOSLOOK_DISK_SRC)
$(BUILD)/test_console_look: $(DOSLOOK_DIR)/test_console_look.c os/milton/console.c os/milton/console.h os/flair/surface.c
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -o $@ $(DOSLOOK_DIR)/test_console_look.c os/milton/console.c os/flair/surface.c

.PHONY: test-dir-look test-dir-look-mutant test-dir-free-space test-dir-free-space-mutant test-shell-dir-look test-shell-dir-look-mutant
test-dir-look: $(BUILD)/test_dir_look
	@$(BUILD)/test_dir_look $(DOSLOOK_DIR)/dos33_dir.golden
test-console-look: $(BUILD)/test_console_look
	@$(BUILD)/test_console_look
test-dir-free-space: $(BUILD)/test_dir_disk $(FLAIR_DATA_IMG)
	@LC_ALL=C TZ=UTC mdir -i $(FLAIR_DATA_IMG) :: > $(BUILD)/dir_space.mdir
	@$(BUILD)/test_dir_disk $(FLAIR_DATA_IMG) $(BUILD)/dir_space.mdir $(BUILD)/dir_space.cells
# qskz: these traces grade DIR/CLS/cursor in a secondary processor. The primary
# without AUTOEXEC now asks for date/time; test-dos-startup grades that flow.
test-shell-dir-look: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(FLAIR_DATA_IMG) $(PPM_TEXT_CHECK_BIN) $(BUILD)/test_dir_disk
	@BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(SECONDARY_TRACER_IMG) $(FLAIR_DATA_IMG) shell_dir_look
	@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/shell_dir_unlabelled.img none
	@BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(SECONDARY_TRACER_IMG) $(BUILD)/shell_dir_unlabelled.img shell_dir_unlabelled none

TEST_UNIT_GATES += test-console-look test-dir-look test-dir-look-mutant test-dir-free-space test-dir-free-space-mutant
TEST_EMU_GATES += test-shell-dir-look test-shell-dir-look-mutant

.PHONY: test-dir-volume test-dir-volume-mutant test-console-look
test-dir-volume: $(BUILD)/test_dir_disk $(FLAIR_DATA_IMG)
	@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_root.img root
	@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_boot.img boot
	@$(BUILD)/test_dir_disk --fixture $(FLAIR_DATA_IMG) $(BUILD)/dir_label_none.img none
	@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_root.img 'ROOT LABEL'
	@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_boot.img 'BOOT LABEL'
	@$(BUILD)/test_dir_disk --label $(BUILD)/dir_label_none.img ''
$(BUILD)/test_dir_disk_mut_label: $(DOSLOOK_DISK_SRC) os/milton/fileio_fat.h
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -DFILEIO_MUTATE_DIR_BOOT_LABEL -o $@ $(DOSLOOK_DISK_SRC)
test-dir-volume-mutant: test-dir-volume $(BUILD)/test_dir_disk_mut_label
	@if $(BUILD)/test_dir_disk_mut_label --label $(BUILD)/dir_label_root.img 'ROOT LABEL' > $(BUILD)/dir_label_mut.log 2>&1; then echo 'FAIL: BPB-only label mutant passed'; exit 1; fi
	@grep 'DIR LABEL RED:' $(BUILD)/dir_label_mut.log
	@echo 'DIR LABEL MUTANT PASS: BPB-only label RED on conflicting root label'
TEST_UNIT_GATES += test-dir-volume test-dir-volume-mutant

define doslook-host-mutant
$(BUILD)/test_dir_look_mut_$(1): $(DOSLOOK_HOST_SRC) $(DOS_MESSAGES_H) os/milton/command.h
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -D$(2) -o $$@ $(DOSLOOK_HOST_SRC)
endef
$(eval $(call doslook-host-mutant,date,CMD_MUTATE_DIR_NO_DATE))
$(eval $(call doslook-host-mutant,joined,CMD_MUTATE_DIR_JOINED))
$(eval $(call doslook-host-mutant,lower,CMD_MUTATE_DIR_LOWERCASE))
test-dir-look-mutant: $(BUILD)/test_dir_look_mut_date $(BUILD)/test_dir_look_mut_joined $(BUILD)/test_dir_look_mut_lower
	@set -e; for tag in date joined lower; do \
	  log=$(BUILD)/dir_look_mut_$$tag.log; \
	  if $(BUILD)/test_dir_look_mut_$$tag $(DOSLOOK_DIR)/dos33_dir.golden > $$log 2>&1; then cat $$log; echo "FAIL: $$tag mutant passed"; exit 1; fi; \
	  grep -q 'DIR LOOK RED:' $$log; \
	  printf 'DIR LOOK MUTANT PASS: %s RED against independent manual golden\n' $$tag; \
	done

$(BUILD)/test_dir_disk_mut_free: $(DOSLOOK_DISK_SRC) os/milton/fileio_fat.h
	$(CC) $(CFLAGS) $(DOSLOOK_INC) -DFILEIO_MUTATE_DIR_FREE -o $@ $(DOSLOOK_DISK_SRC)
test-dir-free-space-mutant: $(BUILD)/test_dir_disk_mut_free $(FLAIR_DATA_IMG)
	@LC_ALL=C TZ=UTC mdir -i $(FLAIR_DATA_IMG) :: > $(BUILD)/dir_space_mut.mdir
	@if $(BUILD)/test_dir_disk_mut_free $(FLAIR_DATA_IMG) $(BUILD)/dir_space_mut.mdir $(BUILD)/dir_space_mut.cells > $(BUILD)/dir_space_mut.log 2>&1; then echo 'FAIL: free-space mutant passed'; exit 1; fi
	@grep 'DIR SPACE RED:' $(BUILD)/dir_space_mut.log
	@echo 'DIR SPACE MUTANT PASS: zero-free-count RED against mdir'

# Each live mutant reuses the real shell objects except one perturbed TU.
# Images obey the same boot window and linker placement guard as tracer.
define doslook-emu-mutant
$(BUILD)/doslook_$(1).o: os/milton/$(2).c $(DOS_MESSAGES_H) os/milton/console.h os/milton/command.h
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DCOMMAND_KERNEL_REPL -D$(3) -Ispec -Ios/milton -Ibuild -c $$< -o $$@
$(BUILD)/kernel_doslook_mut_$(1).elf: $(filter-out $(BUILD)/$(2).o,$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/doslook_$(1).o $(KERNEL_LD)
	$(KERNEL_LINK) -o $$@ $(filter-out $(BUILD)/$(2).o,$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/doslook_$(1).o
$(BUILD)/kernel_doslook_mut_$(1).bin: $(BUILD)/kernel_doslook_mut_$(1).elf
	$(OBJCOPY) -O binary $$< $$@
	@set -eu; sz=$$$$(wc -c < $$@); max=$$$$(( $(KERNEL_SECTORS)*512 )); test "$$$$sz" -le "$$$$max"; \
	dd if=/dev/zero of=$$@ bs=1 seek="$$$$sz" count="$$$$(( max - sz ))" conv=notrunc status=none
$(BUILD)/doslook_mut_$(1).img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_doslook_mut_$(1).bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $$@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_doslook_mut_$(1).bin $(BOOT_SYSTEM_TOOL)
endef
$(eval $(call doslook-emu-mutant,volume,command,CMD_MUTATE_DIR_NO_VOLUME))
$(eval $(call doslook-emu-mutant,date,command,CMD_MUTATE_DIR_NO_DATE))
$(eval $(call doslook-emu-mutant,joined,command,CMD_MUTATE_DIR_JOINED))
$(eval $(call doslook-emu-mutant,lower,command,CMD_MUTATE_DIR_LOWERCASE))
$(eval $(call doslook-emu-mutant,home,console,CONSOLE_MUTATE_CLS_NO_HOME))
$(eval $(call doslook-emu-mutant,cursor,console,CONSOLE_MUTATE_CURSOR_OFF))
DOSLOOK_MUT_IMGS := $(foreach tag,volume date joined lower home cursor,$(BUILD)/doslook_mut_$(tag).img)
test-shell-dir-look-mutant: $(HARNESS_BIN) $(DOSLOOK_MUT_IMGS) $(FLAIR_DATA_IMG) $(PPM_TEXT_CHECK_BIN) $(BUILD)/test_dir_disk
	@set -e; for tag in volume date joined lower home cursor; do for attempt in 1 2; do \
	  name=doslook_mut_$$tag; log=$(BUILD)/$$name.log; \
	  if BUILD=$(BUILD) sh $(DOSLOOK_DIR)/shell_dir_look.sh $(BUILD)/$$name.img $(FLAIR_DATA_IMG) $$name > $$log 2>&1; then echo "FAIL: $$tag live mutant passed"; exit 1; fi; \
	  grep -q '^SHELL-READY$$' $(BUILD)/$$name.serial; \
	  grep -qE '(DIR LOOK RED:|CELL RED|CURSOR RED)' $$log || { cat $$log; echo 'FAIL: mutant RED for wrong reason'; exit 1; }; \
	  printf 'SHELL DIR LOOK MUTANT PASS: %s attempt %s booted, named LOOK assertion RED\n' $$tag $$attempt; \
	done; done

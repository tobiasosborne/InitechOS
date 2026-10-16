# initech-e9sx: oracle + emulator gates BEFORE implementation (Rule 1/6).
COMMAND_FILE_DIR := harness/diff/fat_diff
COMMAND_FILE_TRACE := $(FLAIR_EXIT_DOS_PREFIX),m100:2,m100:2,m94:1,l1,l0
COMMAND_FILE_IMG := $(BUILD)/command-file
$(BUILD)/command_file: $(COMMAND_FILE_DIR)/command_file.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -Ispec -Ios/milton -I$(COMMAND_FILE_DIR) $^ -o $@

.PHONY: test-command-file test-command-file-mutant test-command-file-emu test-command-file-negative test-command-file-emu-mutant
test-command-file: $(BUILD)/command_file $(FLAIRTENANTS_IMG) $(FLAIRTENANTS_INTERACTIVE_IMG) $(FLAIRTENANTS_RECORDDBL_EXITDOS_IMG) $(BUILD)/flair_tenants_mut_record.img
	@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_IMG) $(BUILD)/transient_flairtenants.bin $(BUILD) shipped
	@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_INTERACTIVE_IMG) $(BUILD)/transient_flairtenants_int.bin $(BUILD) interactive
	@sh $(COMMAND_FILE_DIR)/command_file.sh $(FLAIRTENANTS_RECORDDBL_EXITDOS_IMG) $(BUILD)/transient_recorddbl.bin $(BUILD) clip
	@sh $(COMMAND_FILE_DIR)/command_file.sh $(BUILD)/flair_tenants_mut_record.img $(BUILD)/transient_kernel_flairtenants_mut_record.bin $(BUILD) desktop-family

test-command-file-mutant: export TZ := UTC
test-command-file-mutant: test-command-file
	@set -eu; for m in NAME ATTR BYTES GEOMETRY DATE; do \
	 img=$(COMMAND_FILE_IMG)/host-$$m.img; cp -f $(FLAIRTENANTS_IMG) $$img; \
	 case $$m in \
	 NAME) mdel -i $$img@@32256 ::COMMAND.COM;; \
	 ATTR) mattrib -i $$img@@32256 +h ::COMMAND.COM;; \
	 BYTES) cp -f $(BUILD)/transient_flairtenants.bin $(COMMAND_FILE_IMG)/bad.bin; \
	 printf X | dd of=$(COMMAND_FILE_IMG)/bad.bin bs=1 conv=notrunc status=none; \
	 touch -d '@315532800' $(COMMAND_FILE_IMG)/bad.bin; \
	 mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/bad.bin ::COMMAND.COM;; \
	 GEOMETRY) truncate -s 1572864 $$img;; \
	 DATE) cp -f $(BUILD)/transient_flairtenants.bin $(COMMAND_FILE_IMG)/bad.bin; \
	 touch -d '@315619200' $(COMMAND_FILE_IMG)/bad.bin; mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/bad.bin ::COMMAND.COM;; esac; \
	 if $(BUILD)/command_file $$img $(BUILD)/transient_flairtenants.bin check >$(COMMAND_FILE_IMG)/host-$$m.log 2>&1; then echo "FAIL $$m survived"; exit 1; fi; \
	 grep "^RED command-file $$m " $(COMMAND_FILE_IMG)/host-$$m.log; \
	 echo "GREEN test-command-file-mutant $$m rejected"; done

$(COMMAND_FILE_IMG)/fragment.img: $(FLAIRTENANTS_IMG) $(BUILD)/command_file
	@mkdir -p $(COMMAND_FILE_IMG)
	cp -f $< $@
	$(BUILD)/command_file $@ $(BUILD)/transient_flairtenants.bin fragment
$(COMMAND_FILE_IMG)/missing.img: $(FLAIRTENANTS_IMG)
	@mkdir -p $(COMMAND_FILE_IMG)
	cp -f $< $@
	mdel -i $@@@32256 ::COMMAND.COM
$(COMMAND_FILE_IMG)/magic.img: $(FLAIRTENANTS_IMG)
	@mkdir -p $(COMMAND_FILE_IMG)
	cp -f $< $@
	cp -f $(BUILD)/transient_flairtenants.bin $(COMMAND_FILE_IMG)/magic.bin
	@printf X | dd of=$(COMMAND_FILE_IMG)/magic.bin bs=1 conv=notrunc status=none
	@touch -d '@315532800' $(COMMAND_FILE_IMG)/magic.bin
	TZ=UTC mcopy -o -m -i $@@@32256 $(COMMAND_FILE_IMG)/magic.bin ::COMMAND.COM
	@$(call fixture-check,recipe,$@)

test-command-file-emu: $(HARNESS_BIN) $(BUILD)/command_file $(COMMAND_FILE_IMG)/fragment.img $(FLAIR_DATA_IMG) $(PPM_TEXT_CHECK_BIN)
	@sh $(COMMAND_FILE_DIR)/command_file.sh $(COMMAND_FILE_IMG)/fragment.img $(BUILD)/transient_flairtenants.bin $(BUILD) fragmented
	$(call exitdos-boot,$(COMMAND_FILE_IMG)/fragment.img,command_file_fragment,$(FLAIR_DATA_IMG),$(FLAIR_EXIT_DOS_SPEC),$(EXITDOS_DONE),1,40000)
	@( $(call exitdos-exit-check,command_file_fragment) )
	@echo 'GREEN test-command-file-emu: fragmented COMMAND.COM -> transient -> banner -> SHELL-READY -> injected DIR; text console'

test-command-file-negative: $(HARNESS_BIN) $(COMMAND_FILE_IMG)/missing.img $(COMMAND_FILE_IMG)/magic.img $(FLAIR_DATA_IMG)
	@sh harness/emu/command_file.sh $(BUILD) $(COMMAND_FILE_IMG)/missing.img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' missing 'COMMAND.COM not found'
	@sh harness/emu/command_file.sh $(BUILD) $(COMMAND_FILE_IMG)/magic.img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' magic 'no COMMAND3 transient'

# Three real kernel mutants: wrong physical volume, ignored magic, wrong message.
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_COMMAND_VOLUME,command_volume))
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_COMMAND_MAGIC,command_magic))
$(eval $(call flair-tenants-kmain-mutant-rules,KMAIN_MUT_COMMAND_MESSAGE,command_message))
$(foreach tag,command_volume command_magic command_message,$(eval $(call exitdos-transient-rules,$(tag),$(BUILD)/kernel_flairtenants_mut_$(tag).elf)))
define command-file-mutant-image
$(COMMAND_FILE_IMG)/$(1).img: $(BUILD)/kernel_flairtenants_mut_$(1).bin $(BUILD)/transient_$(1).bin $(BOOT_SYSTEM_MINT) $(BOOT_SYSTEM_TOOL) $(IO_SYS_BIN) $(MBR_BIN) $(STAGE2_BIN)
	@mkdir -p $(COMMAND_FILE_IMG)
	$$(call exitdos-image-recipe,$(BUILD)/kernel_flairtenants_mut_$(1).bin,$(BUILD)/transient_$(1).bin)
endef
$(foreach tag,command_volume command_magic command_message,$(eval $(call command-file-mutant-image,$(tag))))

test-command-file-emu-mutant: $(HARNESS_BIN) $(PPM_TEXT_CHECK_BIN) $(FLAIR_DATA_IMG) $(foreach tag,command_volume command_magic command_message,$(COMMAND_FILE_IMG)/$(tag).img)
	$(call exitdos-boot,$(COMMAND_FILE_IMG)/command_volume.img,command_file_mut_volume,$(FLAIR_DATA_IMG),$(COMMAND_FILE_TRACE),HALTED,0,40000)
	@out=$$( ( $(call exitdos-exit-check,command_file_mut_volume) ) 2>&1 ); rc=$$?; \
	 test $$rc -ne 0 && echo "$$out" | grep -q 'did not run alert -> put away' || { echo "FAIL COMMAND_VOLUME: $$out"; exit 1; }; \
	 grep -q '^PANIC exit-to-dos: COMMAND.COM not found' $(BUILD)/command_file_mut_volume.serial; \
	 echo "RED command-file COMMAND_VOLUME: $$out"; echo 'GREEN test-command-file-emu-mutant COMMAND_VOLUME rejected'
	@set -eu; img=$(COMMAND_FILE_IMG)/run-command_magic.img; cp -f $(COMMAND_FILE_IMG)/command_magic.img $$img; \
	 cp -f $(BUILD)/transient_command_magic.bin $(COMMAND_FILE_IMG)/skip-magic.bin; \
	 printf X | dd of=$(COMMAND_FILE_IMG)/skip-magic.bin bs=1 conv=notrunc status=none; \
	 touch -d '@315532800' $(COMMAND_FILE_IMG)/skip-magic.bin; \
	 TZ=UTC mcopy -o -m -i $$img@@32256 $(COMMAND_FILE_IMG)/skip-magic.bin ::COMMAND.COM; \
	 if sh harness/emu/command_file.sh $(BUILD) $$img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' skip-magic 'no COMMAND3 transient' >$(COMMAND_FILE_IMG)/skip-magic.log 2>&1; then echo 'FAIL COMMAND_MAGIC survived'; exit 1; fi; \
	 grep '^RED command-file-emu skip-magic missing exact MSG-DOS-0014' $(COMMAND_FILE_IMG)/skip-magic.log; \
	 grep -qx SHELL-READY $(COMMAND_FILE_IMG)/skip-magic.serial; echo 'GREEN test-command-file-emu-mutant COMMAND_MAGIC rejected'
	@set -eu; img=$(COMMAND_FILE_IMG)/run-command_message.img; cp -f $(COMMAND_FILE_IMG)/command_message.img $$img; mdel -i $$img@@32256 ::COMMAND.COM; \
	 if sh harness/emu/command_file.sh $(BUILD) $$img $(FLAIR_DATA_IMG) '$(COMMAND_FILE_TRACE)' wrong-message 'COMMAND.COM not found' >$(COMMAND_FILE_IMG)/wrong-message.log 2>&1; then echo 'FAIL COMMAND_MESSAGE survived'; exit 1; fi; \
	 grep '^RED command-file-emu wrong-message missing exact MSG-DOS-0014' $(COMMAND_FILE_IMG)/wrong-message.log; \
	 grep -qx HALTED $(COMMAND_FILE_IMG)/wrong-message.serial; echo 'GREEN test-command-file-emu-mutant COMMAND_MESSAGE rejected'

TEST_UNIT_GATES += test-command-file test-command-file-mutant
TEST_EMU_GATES += test-command-file-emu test-command-file-negative test-command-file-emu-mutant

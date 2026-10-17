# Small, isolated MILTON hygiene gates. Rules 1/6; see the source citations.
KHY_SRC := harness/diff/fat_diff/test_kernel_hygiene.c
KHY_DEPS := $(KHY_SRC) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC) $(KERNEL_DESKTOP_DB_C)
$(BUILD)/test_kernel_hygiene: $(KHY_DEPS) | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -Ispec -I$(MILTON_DIR) -Iseed -I$(FAT_DIFF_DIR) -o $@ $(KHY_DEPS)

.PHONY: test-fat-hygiene-rename test-fat-hygiene-grow test-fat-hygiene-hidden
test-fat-hygiene-rename test-fat-hygiene-grow test-fat-hygiene-hidden: $(BUILD)/test_kernel_hygiene
	@sh harness/diff/fat_diff/kernel_hygiene.sh $(patsubst test-fat-hygiene-%,%,$@) $(BUILD) $<

TEST_UNIT_GATES += test-fat-hygiene-rename test-fat-hygiene-grow test-fat-hygiene-hidden

$(BUILD)/hygiene_probe.bin: os/milton/hygiene_probe.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/baked_exec_probe.bin: os/milton/baked_exec_probe.asm | $(BUILD)
	$(NASM) -f bin $< -o $@
$(BUILD)/baked_exec_blob.c: $(BUILD)/baked_exec_probe.bin $(BIN2C_BIN)
	$(BIN2C_BIN) $< g_test_prog_image > $@
$(BUILD)/baked_exec_blob.o: $(BUILD)/baked_exec_blob.c
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -c $< -o $@
$(BUILD)/kernel_baked_guard.elf: $(filter-out $(KERNEL_TEST_PROG_OBJ),$(KERNEL_EXEC_OBJS)) $(BUILD)/baked_exec_blob.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_baked_guard.bin: $(BUILD)/kernel_baked_guard.elf Makefile
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,baked-guard)
$(BUILD)/baked_guard.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_baked_guard.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_baked_guard.bin $(BOOT_SYSTEM_TOOL)

.PHONY: test-baked-exec-guard test-fat-hygiene-emu
test-baked-exec-guard: $(HARNESS_BIN) $(BUILD)/baked_guard.img $(FAT_EXEC_IMG)
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/baked_guard.img --disk2 $(FAT_EXEC_IMG) --name baked_guard --out $(BUILD) --timeout-ms 8000 --quit-after EXEC-4B-END > $(BUILD)/baked_guard.stdout 2> $(BUILD)/baked_guard.report || true
	@! grep -q 'triple_fault=1' $(BUILD)/baked_guard.report
	@tr -d '\r' < $(BUILD)/baked_guard.serial > $(BUILD)/baked_guard.normalized
	@grep -qx 'KHY-BAKED-BUSY-OK' $(BUILD)/baked_guard.normalized || { echo 'RED baked EXEC guard: nested load did not return BUSY'; exit 1; }
	@grep -qx 'PROGRAM-EXIT rc=7' $(BUILD)/baked_guard.serial
	@grep -qx 'EXEC-4B-RC=7' $(BUILD)/baked_guard.serial
	@echo 'GREEN test-baked-exec-guard: nested BUSY, outer exit 7, subsequent EXEC 7'

test-fat-hygiene-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/hygiene_probe.bin
	@sh harness/diff/fat_diff/kernel_hygiene_emu.sh $(BUILD) $(SECONDARY_TRACER_IMG)
TEST_EMU_GATES += test-baked-exec-guard test-fat-hygiene-emu

$(BUILD)/test_ata: os/milton/test_ata.c os/milton/ata.c os/milton/ata.h | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DATA_HOST_TEST -DATA_POLL_LIMIT=16 -Ispec -I$(MILTON_DIR) -Iseed -o $@ os/milton/test_ata.c os/milton/ata.c
.PHONY: test-ata
test-ata: $(BUILD)/test_ata
	@$<
TEST_UNIT_GATES += test-ata

KHY_FLAG_rename := FAT12_MUTATE_RENAME_REJECT_DIR
KHY_FLAG_grow := FAT12_MUTATE_GROW_NO_WRAP
KHY_FLAG_hidden := FAT12_MUTATE_HIDE_DIR_NO_WRITE
define khy-fat-mutant
$(BUILD)/test_kernel_hygiene_$(1): $(KHY_DEPS) | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -D$$(KHY_FLAG_$(1)) -Ispec -I$$(MILTON_DIR) -Iseed -I$$(FAT_DIFF_DIR) -o $$@ $$(KHY_DEPS)
.PHONY: test-fat-hygiene-$(1)-mutant
test-fat-hygiene-$(1)-mutant: $(BUILD)/test_kernel_hygiene_$(1)
	@if sh harness/diff/fat_diff/kernel_hygiene.sh $(1) $$(BUILD) $$< > $$(BUILD)/khygiene-evidence/mutant-fat-$(1).log 2>&1; then echo 'FAIL $(1) mutant survived'; exit 1; fi
	@grep 'FAIL.*$(2)' $$(BUILD)/khygiene-evidence/mutant-fat-$(1).log
	@echo 'GREEN test-fat-hygiene-$(1)-mutant: $$(KHY_FLAG_$(1)) correctly RED'
TEST_UNIT_GATES += test-fat-hygiene-$(1)-mutant
endef
$(eval $(call khy-fat-mutant,rename,f0hh))
$(eval $(call khy-fat-mutant,grow,fov8))
$(eval $(call khy-fat-mutant,hidden,tdnl.30))

$(BUILD)/test_mzload_entry_equal: $(TEST_MZLOAD_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C) os/milton/mz.c | $(BUILD)
	$(CC) $(CFLAGS) $(SEED_TEST_CFLAGS) -DLOADER_MUTATE_MZ_ENTRY_EQUAL -Ispec -I$(MILTON_DIR) -Iseed -o $@ $(TEST_MZLOAD_SRC) $(KERNEL_LOADER_C) $(KERNEL_PSP_C)
.PHONY: test-mzload-entry-mutant
test-mzload-entry-mutant: $(BUILD)/test_mzload_entry_equal
	@if $< > $(BUILD)/khygiene-evidence/mutant-mz-entry.log 2>&1; then echo 'FAIL MZ equality mutant survived'; exit 1; fi
	@grep 'FAIL.*MZ entry boundary' $(BUILD)/khygiene-evidence/mutant-mz-entry.log
	@echo 'GREEN test-mzload-entry-mutant: LOADER_MUTATE_MZ_ENTRY_EQUAL correctly RED'
TEST_UNIT_GATES += test-mzload-entry-mutant

$(BUILD)/loader_baked_unguarded.o: os/milton/loader.c os/milton/loader.h os/milton/mz.c | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DLOADER_MUTATE_BAKED_UNGUARDED -Ispec -Ios/milton -c $< -o $@
$(BUILD)/kernel_baked_unguarded.elf: $(filter-out $(KERNEL_TEST_PROG_OBJ) $(KERNEL_LOADER_OBJ),$(KERNEL_EXEC_OBJS)) $(BUILD)/loader_baked_unguarded.o $(BUILD)/baked_exec_blob.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_baked_unguarded.bin: $(BUILD)/kernel_baked_unguarded.elf Makefile
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,baked-unguarded)
$(BUILD)/baked_unguarded.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_baked_unguarded.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_baked_unguarded.bin $(BOOT_SYSTEM_TOOL)
.PHONY: test-baked-exec-guard-mutant
test-baked-exec-guard-mutant: $(HARNESS_BIN) $(BUILD)/baked_unguarded.img $(FAT_EXEC_IMG)
	@INITECH_QMP_STDIO=1 $(HARNESS_BIN) --disk $(BUILD)/baked_unguarded.img --disk2 $(FAT_EXEC_IMG) --name baked_unguarded --out $(BUILD) --timeout-ms 6000 > $(BUILD)/baked_unguarded.stdout 2> $(BUILD)/baked_unguarded.report || true
	@grep -q 'PROGRAM-BEGIN' $(BUILD)/baked_unguarded.serial
	@grep -q '^GREETINGS FROM A:GREET.COM' $(BUILD)/baked_unguarded.serial
	@! grep -q 'PROGRAM-EXIT rc=7' $(BUILD)/baked_unguarded.serial || { echo 'FAIL unguarded baked mutant survived'; exit 1; }
	@echo 'RED LOADER_MUTATE_BAKED_UNGUARDED: outer PROGRAM-EXIT rc=7 missing'
	@echo 'GREEN test-baked-exec-guard-mutant: nested load clobber caught'
TEST_EMU_GATES += test-baked-exec-guard-mutant

# Fixed-map dependencies apply to both green and existing semantic mutants.
$(TEST_KEEP) $(TEST_KEEP_MUT) $(TEST_MCBI21) $(TEST_MCBI21_MUT_SEGBASE) $(TEST_ARENADJ) $(TEST_ARENADJ_MUT): os/milton/test_low_mapping.h
define khy-mapping-mutant
$(BUILD)/test_mapping_$(1): os/milton/test_$(1).c $$(TEST_ARENADJ_DEPS) os/milton/test_low_mapping.h | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -DHOST_LOW_MUTATE_ZERO_OWNER -Ispec -Ios/milton -Iseed -Ibuild -o $$@ os/milton/test_$(1).c $$(TEST_ARENADJ_DEPS)
.PHONY: test-host-mapping-$(1)-mutant
test-host-mapping-$(1)-mutant: $(BUILD)/test_mapping_$(1)
	@if $$< > $$(BUILD)/khygiene-evidence/mutant-mapping-$(1).log 2>&1; then echo 'FAIL zero-owner mapping mutant survived'; exit 1; fi
	@grep 'FAIL.*deterministic PSP' $$(BUILD)/khygiene-evidence/mutant-mapping-$(1).log
	@echo 'GREEN test-host-mapping-$(1)-mutant: HOST_LOW_MUTATE_ZERO_OWNER correctly RED'
TEST_UNIT_GATES += test-host-mapping-$(1)-mutant
endef
$(eval $(call khy-mapping-mutant,keep))
$(eval $(call khy-mapping-mutant,arena_disjoint))
$(eval $(call khy-mapping-mutant,mcb_int21))

define khy-ata-mutant
$(BUILD)/test_ata_$(1): os/milton/test_ata.c os/milton/ata.c os/milton/ata.h | $(BUILD)
	$$(CC) $$(CFLAGS) $$(SEED_TEST_CFLAGS) -DATA_HOST_TEST -DATA_POLL_LIMIT=16 -D$(2) -Ispec -Ios/milton -Iseed -o $$@ os/milton/test_ata.c os/milton/ata.c
.PHONY: test-ata-$(1)-mutant
test-ata-$(1)-mutant: $(BUILD)/test_ata_$(1)
	@if $$< > $$(BUILD)/khygiene-evidence/mutant-ata-$(1).log 2>&1; then echo 'FAIL $(2) survived'; exit 1; fi
	@grep 'FAIL.*$(3)' $$(BUILD)/khygiene-evidence/mutant-ata-$(1).log
	@echo 'GREEN test-ata-$(1)-mutant: $(2) correctly RED'
TEST_UNIT_GATES += test-ata-$(1)-mutant
endef
$(eval $(call khy-ata-mutant,prewait,ATA_MUTATE_NO_PREWAIT,BSY=0 DRDY=1))
$(eval $(call khy-ata-mutant,flush,ATA_MUTATE_FLUSH_EARLY,final WRITE completion))
$(eval $(call khy-ata-mutant,lba,ATA_MUTATE_LBA_WRAP,LBA28 transfer end))
$(eval $(call khy-ata-mutant,errors,ATA_MUTATE_ERROR_COLLAPSE,distinct post-command error))

.PHONY: test-kernel-repro-flagship
test-kernel-repro-flagship: test-kernel-repro
	@test -s $(KERNEL_FLAIRTENANTS_BIN).repro.1 -a -s $(KERNEL_FLAIRTENANTS_BIN).repro.2 || { echo 'RED flagship repro: flagship rebuild comparison missing'; exit 1; }
	@cmp $(KERNEL_FLAIRTENANTS_BIN).repro.1 $(KERNEL_FLAIRTENANTS_BIN).repro.2
	@echo 'GREEN test-kernel-repro-flagship: both flagship builds byte-identical'
TEST_UNIT_GATES += test-kernel-repro-flagship

.PHONY: test-kernel-repro-flagship-mutant
test-kernel-repro-flagship-mutant: | $(BUILD)/khygiene-evidence
	@if $(MAKE) --no-print-directory test-kernel-repro KERNEL_REPRO_MUTATE_FLAGSHIP=1 > $(BUILD)/khygiene-evidence/mutant-flagship-repro.log 2>&1; then echo 'FAIL flagship second-build byte mutant survived'; exit 1; fi
	@grep 'RED test-kernel-repro: $(KERNEL_FLAIRTENANTS_BIN) is NOT reproducible' $(BUILD)/khygiene-evidence/mutant-flagship-repro.log
	@echo 'GREEN test-kernel-repro-flagship-mutant: KERNEL_REPRO_MUTATE_FLAGSHIP correctly RED'
TEST_UNIT_GATES += test-kernel-repro-flagship-mutant

$(BUILD)/khygiene-evidence: | $(BUILD)
	mkdir -p $@
$(filter test-fat-hygiene-% test-ata% test-host-mapping-% test-mzload-entry-mutant test-baked-exec-guard%,$(TEST_UNIT_GATES) $(TEST_EMU_GATES)): | $(BUILD)/khygiene-evidence
test-fat-hygiene-emu: test-fat-hygiene-grow

define khy-emu-mutant
$(BUILD)/fat12_hygiene_$(1).o: $(FAT12_SRC) os/milton/fat12.h | $(BUILD)
	$$(KERNEL_CC) $$(KERNEL_CFLAGS) $$(KERNEL_SECTION_FLAGS) $$(KERNEL_FAT12_OPT) -D$$(KHY_FLAG_$(1)) -Ispec -Ios/milton -c $$< -o $$@
$(BUILD)/kernel_hygiene_$(1).elf: $$(filter-out $$(KERNEL_FAT12_OBJ),$$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fat12_hygiene_$(1).o $$(KERNEL_LD)
	$$(KERNEL_LINK) -o $$@ $$(filter %.o,$$^)
$(BUILD)/kernel_hygiene_$(1).bin: $(BUILD)/kernel_hygiene_$(1).elf Makefile
	$$(OBJCOPY) -O binary $$< $$@
	@set -eu; sz=$$$$(wc -c < $$@); max=$$$$(( $$(KERNEL_SECTORS)*512 )); test $$$$sz -le $$$$max; \
	dd if=/dev/zero of=$$@ bs=1 seek=$$$$sz count=$$$$((max-sz)) conv=notrunc status=none
	$$(call kernel-end-guard,$$<,hygiene-$(1))
$(BUILD)/hygiene_$(1).img: $$(MBR_BIN) $$(STAGE2_BIN) $(BUILD)/kernel_hygiene_$(1).bin $$(BOOT_SYSTEM_TOOL) $$(BOOT_SYSTEM_MINT)
	@sh $$(BOOT_SYSTEM_MINT) $$@ $$(MBR_BIN) $$(STAGE2_BIN) $$(IO_SYS_BIN) $(BUILD)/kernel_hygiene_$(1).bin $$(BOOT_SYSTEM_TOOL)
.PHONY: test-fat-hygiene-$(1)-emu-mutant
test-fat-hygiene-$(1)-emu-mutant: $$(HARNESS_BIN) $(BUILD)/hygiene_$(1).img $(BUILD)/hygiene_probe.bin test-fat-hygiene-grow | $(BUILD)/khygiene-evidence
	@if sh harness/diff/fat_diff/kernel_hygiene_emu.sh $$(BUILD) $(BUILD)/hygiene_$(1).img -$(1) > $(BUILD)/khygiene-evidence/mutant-emu-$(1).log 2>&1; then echo 'FAIL FAT $(1) emu mutant survived'; exit 1; fi
	@grep 'RED FAT hygiene emu: missing $(2)' $(BUILD)/khygiene-evidence/mutant-emu-$(1).log
	@grep -qx KHY-FAIL $(BUILD)/khygiene-emu-$(1)/serial.txt
	@grep -qx SHELL-DONE $(BUILD)/khygiene-emu-$(1)/serial.txt
	@echo 'GREEN test-fat-hygiene-$(1)-emu-mutant: $$(KHY_FLAG_$(1)) correctly RED'
TEST_EMU_GATES += test-fat-hygiene-$(1)-emu-mutant
endef
$(eval $(call khy-emu-mutant,rename,KHY-RENAME-OK))
$(eval $(call khy-emu-mutant,grow,KHY-GROW-OK))

.PHONY: test-trash-hidden-emu test-trash-hidden-emu-mutant
test-trash-hidden-emu: $(HARNESS_BIN) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG)
	@sh harness/diff/fat_diff/trash_hidden_emu.sh $(BUILD) $(FLAIRTENANTS_IMG) $(FLAIR_DATA_IMG) green
$(BUILD)/fat12_trash_visible.o: $(FAT12_SRC) os/milton/fat12.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FAT12_OPT) -DFAT12_MUTATE_HIDE_DIR_NO_WRITE -Ispec -Ios/milton -c $< -o $@
$(BUILD)/kernel_trash_visible.elf: $(filter-out $(KERNEL_FAT12_OBJ),$(KERNEL_FLAIRTENANTS_OBJS)) $(BUILD)/fat12_trash_visible.o $(KERNEL_LD) $(TRANSIENT_IMPORTS_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^) @$(TRANSIENT_IMPORTS_LD)
$(BUILD)/kernel_trash_visible.bin: $(BUILD)/kernel_trash_visible.elf Makefile
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,trash-visible)
$(BUILD)/trash_visible.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_trash_visible.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_trash_visible.bin $(BOOT_SYSTEM_TOOL)
test-trash-hidden-emu-mutant: $(HARNESS_BIN) $(BUILD)/trash_visible.img $(FLAIR_DATA_IMG) | $(BUILD)/khygiene-evidence
	@if sh harness/diff/fat_diff/trash_hidden_emu.sh $(BUILD) $(BUILD)/trash_visible.img $(FLAIR_DATA_IMG) mutant > $(BUILD)/khygiene-evidence/mutant-trash-hidden.log 2>&1; then echo 'FAIL visible TRASH emu mutant survived'; exit 1; fi
	@grep 'RED TRASH hidden attribute' $(BUILD)/khygiene-evidence/mutant-trash-hidden.log
	@echo 'GREEN test-trash-hidden-emu-mutant: FAT12_MUTATE_HIDE_DIR_NO_WRITE correctly RED'
TEST_EMU_GATES += test-trash-hidden-emu test-trash-hidden-emu-mutant

$(BUILD)/fat12_hygiene_rename.o $(BUILD)/fat12_hygiene_grow.o $(BUILD)/fat12_trash_visible.o: Makefile harness/diff/fat_diff/kernel_hygiene.mk
$(BUILD)/kernel_baked_guard.bin $(BUILD)/kernel_baked_unguarded.bin $(BUILD)/kernel_hygiene_rename.bin $(BUILD)/kernel_hygiene_grow.bin $(BUILD)/kernel_trash_visible.bin: harness/diff/fat_diff/kernel_hygiene.mk

.PHONY: test-ata-no-drive-emu test-ata-no-drive-emu-mutant
test-ata-no-drive-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG)
	@sh harness/diff/fat_diff/ata_no_drive.sh $(BUILD) $(SECONDARY_TRACER_IMG) green
$(BUILD)/ata_no_drive_success.o: os/milton/ata.c os/milton/ata.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DATA_MUTATE_NO_DRIVE_SUCCESS -Ispec -Ios/milton -c $< -o $@
$(BUILD)/kernel_ata_no_drive_success.elf: $(filter-out $(KERNEL_ATA_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/ata_no_drive_success.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_ata_no_drive_success.bin: $(BUILD)/kernel_ata_no_drive_success.elf Makefile harness/diff/fat_diff/kernel_hygiene.mk
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,ata-no-drive-success)
$(BUILD)/ata_no_drive_success.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_ata_no_drive_success.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_ata_no_drive_success.bin $(BOOT_SYSTEM_TOOL)
test-ata-no-drive-emu-mutant: $(HARNESS_BIN) $(BUILD)/ata_no_drive_success.img | $(BUILD)/khygiene-evidence
	@if sh harness/diff/fat_diff/ata_no_drive.sh $(BUILD) $(BUILD)/ata_no_drive_success.img mutant > $(BUILD)/khygiene-evidence/mutant-ata-nodrive.log 2>&1; then echo 'FAIL no-drive success mutant survived'; exit 1; fi
	@grep 'RED ATA no-drive:' $(BUILD)/khygiene-evidence/mutant-ata-nodrive.log
	@grep -qx 'FAT-MOUNT-FAIL rc=-2 base_lba=0 hidden_sectors=0' $(BUILD)/khygiene-ata-mutant/nodrive.serial
	@grep -qx SHELL-DONE $(BUILD)/khygiene-ata-mutant/nodrive.serial
	@echo 'GREEN test-ata-no-drive-emu-mutant: ATA_MUTATE_NO_DRIVE_SUCCESS correctly RED'
TEST_EMU_GATES += test-ata-no-drive-emu test-ata-no-drive-emu-mutant

.PHONY: test-mzload-entry-emu test-mzload-entry-emu-mutant
test-mzload-entry-emu: $(HARNESS_BIN) $(MZEXEC_IMG) $(FAT_MZEXEC_IMG) $(TEST_MZLOAD)
	@sh harness/diff/fat_diff/mz_entry_emu.sh $(BUILD) $(MZEXEC_IMG) $(FAT_MZEXEC_IMG) green
$(BUILD)/loader_mz_entry_equal.o: os/milton/loader.c os/milton/loader.h os/milton/mz.c | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) -DLOADER_MUTATE_MZ_ENTRY_EQUAL -Ispec -Ios/milton -c $< -o $@
$(BUILD)/kernel_mz_entry_equal.elf: $(filter-out $(KERNEL_LOADER_OBJ),$(KERNEL_MZEXEC_OBJS)) $(BUILD)/loader_mz_entry_equal.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter %.o,$^)
$(BUILD)/kernel_mz_entry_equal.bin: $(BUILD)/kernel_mz_entry_equal.elf Makefile harness/diff/fat_diff/kernel_hygiene.mk
	$(OBJCOPY) -O binary $< $@
	@set -eu; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS)*512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek=$$sz count=$$((max-sz)) conv=notrunc status=none
	$(call kernel-end-guard,$<,mz-entry-equal)
$(BUILD)/mz_entry_equal.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/kernel_mz_entry_equal.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/kernel_mz_entry_equal.bin $(BOOT_SYSTEM_TOOL)
test-mzload-entry-emu-mutant: $(HARNESS_BIN) $(BUILD)/mz_entry_equal.img $(FAT_MZEXEC_IMG) $(TEST_MZLOAD) | $(BUILD)/khygiene-evidence
	@if sh harness/diff/fat_diff/mz_entry_emu.sh $(BUILD) $(BUILD)/mz_entry_equal.img $(FAT_MZEXEC_IMG) mutant > $(BUILD)/khygiene-evidence/mutant-mz-entry-emu.log 2>&1; then echo 'FAIL MZ equality emu mutant survived'; exit 1; fi
	@grep 'RED MZ entry at end:' $(BUILD)/khygiene-evidence/mutant-mz-entry-emu.log
	@echo 'GREEN test-mzload-entry-emu-mutant: LOADER_MUTATE_MZ_ENTRY_EQUAL correctly RED'
TEST_EMU_GATES += test-mzload-entry-emu test-mzload-entry-emu-mutant

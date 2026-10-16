# ATTRIB / LABEL / VOL: initech-gyvx + initech-f88h. Standalone COMs,
# host differential + injected QEMU trace + named implementation mutants.
DOSUTIL_SRC := os/milton/dosutil.c
DOSUTIL_HDR := os/milton/dosutil.h
ATTRIB_PROG_BIN := $(BUILD)/attrib_program.bin
LABEL_PROG_BIN := $(BUILD)/label_program.bin
DOSUTIL_HOST := $(BUILD)/dosutil_host
DOSUTIL_HOST_DEPS := $(DOSUTIL_SRC) $(DOSUTIL_HDR) harness/diff/fat_diff/dosutil_host.c $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
DOSUTIL_HOST_FLAGS := $(CFLAGS) -Ios/milton -Ispec -Iharness/diff/fat_diff
DOSUTIL_COM_FLAGS := $(KERNEL_CFLAGS) -Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -Ios/milton -Ispec
DOSUTIL_ATTR_MUTANTS := R A PRESERVE WILD RECURSE DISPLAY SYNTAX
DOSUTIL_LABEL_MUTANTS := ROOT BPB CONFIRM TRUNCATE PROMPT INVALID ROOM

$(DOSUTIL_HOST): $(DOSUTIL_HOST_DEPS) | $(BUILD)
	$(CC) $(DOSUTIL_HOST_FLAGS) -o $@ harness/diff/fat_diff/dosutil_host.c $(DOSUTIL_SRC) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
define dosutil-host-mutant
$(BUILD)/dosutil_host_$(1): $(DOSUTIL_HOST_DEPS) | $(BUILD)
	$(CC) $(DOSUTIL_HOST_FLAGS) -DDU_MUTATE_$(1) -o $$@ harness/diff/fat_diff/dosutil_host.c $(DOSUTIL_SRC) $(FAT12_SRC) $(BLOCKDEV_FILE_SRC)
endef
$(foreach m,$(DOSUTIL_ATTR_MUTANTS) $(DOSUTIL_LABEL_MUTANTS),$(eval $(call dosutil-host-mutant,$(m))))

define dosutil-com
$(BUILD)/$(1)_core.o: $(DOSUTIL_SRC) $(DOSUTIL_HDR) | $(BUILD)
	$(KERNEL_CC) $(DOSUTIL_COM_FLAGS) $(3) -c $$< -o $$@
$(BUILD)/$(1)_target.o: os/milton/dosutil_target.c $(DOSUTIL_HDR) spec/find_data.h | $(BUILD)
	$(KERNEL_CC) $(DOSUTIL_COM_FLAGS) $(2) -c $$< -o $$@
$(BUILD)/$(1)_program.elf: $(BUILD)/$(1)_core.o $(BUILD)/$(1)_target.o os/milton/dosutil.ld
	$(LD) -m elf_i386 --gc-sections -T os/milton/dosutil.ld -o $$@ $(BUILD)/$(1)_target.o $(BUILD)/$(1)_core.o
$(BUILD)/$(1)_program.bin: $(BUILD)/$(1)_program.elf
	$(OBJCOPY) -O binary $$< $$@
endef
$(eval $(call dosutil-com,attrib,-DDOSUTIL_ATTRIB,))
$(eval $(call dosutil-com,label,,))
$(eval $(call dosutil-com,attrib_mut,-DDOSUTIL_ATTRIB,-DDU_MUTATE_RECURSE))
$(eval $(call dosutil-com,label_mut,,-DDU_MUTATE_BPB))

$(BUILD)/dosutil_vol: harness/diff/fat_diff/dosutil_vol.c os/milton/command.c os/milton/command.h $(DOS_MESSAGES_H)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -Wl,--gc-sections -Ios/milton -Ispec -Ibuild -o $@ $< os/milton/command.c
$(BUILD)/dosutil_vol_mut: harness/diff/fat_diff/dosutil_vol.c os/milton/command.c os/milton/command.h $(DOS_MESSAGES_H)
	$(CC) $(CFLAGS) -DDU_MUTATE_VOL -ffunction-sections -fdata-sections -Wl,--gc-sections -Ios/milton -Ispec -Ibuild -o $@ $< os/milton/command.c

# V2: a native LABEL deletion must invalidate the old mounted BPB fallback.
$(BUILD)/fileio_fat_mut_label_cache.o: os/milton/fileio_fat.c os/milton/fileio_fat.h | $(BUILD)
	$(KERNEL_CC) $(KERNEL_CFLAGS) $(KERNEL_SECTION_FLAGS) $(KERNEL_FILEIO_FAT_OPT) -DDU_MUTATE_LABEL_CACHE -Ispec -Ios/milton -c $< -o $@
$(BUILD)/dosutil_cache.elf: $(filter-out $(KERNEL_FILEIO_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fileio_fat_mut_label_cache.o $(KERNEL_LD)
	$(KERNEL_LINK) -o $@ $(filter-out $(KERNEL_FILEIO_OBJ),$(KERNEL_SECONDARY_SHELL_OBJS)) $(BUILD)/fileio_fat_mut_label_cache.o
$(BUILD)/dosutil_cache.bin: $(BUILD)/dosutil_cache.elf
	$(OBJCOPY) -O binary $< $@
	@set -e; sz=$$(wc -c < $@); max=$$(( $(KERNEL_SECTORS) * 512 )); test $$sz -le $$max; \
	dd if=/dev/zero of=$@ bs=1 seek="$$sz" count="$$((max-sz))" conv=notrunc status=none
	$(call kernel-end-guard,$<,dosutil-cache-mutant)
$(BUILD)/dosutil_cache.img: $(MBR_BIN) $(STAGE2_BIN) $(BUILD)/dosutil_cache.bin $(BOOT_SYSTEM_TOOL) $(BOOT_SYSTEM_MINT)
	@sh $(BOOT_SYSTEM_MINT) $@ $(MBR_BIN) $(STAGE2_BIN) $(IO_SYS_BIN) $(BUILD)/dosutil_cache.bin $(BOOT_SYSTEM_TOOL)

.PHONY: test-attrib test-attrib-mutant test-label-vol test-label-vol-mutant test-dosutil-emu test-dosutil-emu-mutant
test-attrib: $(DOSUTIL_HOST)
	@sh harness/diff/fat_diff/dosutil_diff.sh attrib $(DOSUTIL_HOST) attrib
test-label-vol: $(DOSUTIL_HOST) $(BUILD)/dosutil_vol
	@$(BUILD)/dosutil_vol
	@sh harness/diff/fat_diff/dosutil_diff.sh label $(DOSUTIL_HOST) label
test-attrib-mutant: $(foreach m,$(DOSUTIL_ATTR_MUTANTS),$(BUILD)/dosutil_host_$(m))
	@set -e; for m in $(DOSUTIL_ATTR_MUTANTS); do \
	  log=$(BUILD)/dosutil_mut_$$m.log; \
	  if sh harness/diff/fat_diff/dosutil_diff.sh attrib $(BUILD)/dosutil_host_$$m mut_$$m >$$log 2>&1; then echo "FAIL: $$m survived"; exit 1; fi; \
	  case $$m in R) rule=A1;; A) rule=A2;; PRESERVE) rule=A3;; WILD) rule=A4;; RECURSE) rule=A5;; DISPLAY) rule=A6;; SYNTAX) rule=A7;; esac; \
	  grep "^FAIL: $$rule " $$log || { cat $$log; exit 1; }; \
	  printf 'PASS: DU_MUTATE_%s caught RED\n' "$$m"; \
	done
test-label-vol-mutant: $(foreach m,$(DOSUTIL_LABEL_MUTANTS),$(BUILD)/dosutil_host_$(m)) $(BUILD)/dosutil_vol_mut
	@set -e; for m in $(DOSUTIL_LABEL_MUTANTS); do \
	  log=$(BUILD)/dosutil_mut_$$m.log; \
	  if sh harness/diff/fat_diff/dosutil_diff.sh label $(BUILD)/dosutil_host_$$m mut_$$m >$$log 2>&1; then echo "FAIL: $$m survived"; exit 1; fi; \
	  case $$m in ROOT) rule=L1;; BPB) rule=L2;; CONFIRM) rule=L3;; TRUNCATE) rule=L4;; PROMPT) rule=L5;; INVALID) rule=L6;; ROOM) rule=L7;; esac; \
	  grep "^FAIL: $$rule " $$log || { cat $$log; exit 1; }; \
	  printf 'PASS: DU_MUTATE_%s caught RED\n' "$$m"; \
	done
	@if $(BUILD)/dosutil_vol_mut >$(BUILD)/dosutil_mut_VOL.log; then echo 'FAIL: VOL survived'; exit 1; fi
	@grep '^FAIL: V1' $(BUILD)/dosutil_mut_VOL.log
	@echo 'PASS: DU_MUTATE_VOL caught RED'
test-dosutil-emu: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN)
	@INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/dosutil_emu.sh $(SECONDARY_TRACER_IMG) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) normal
test-dosutil-emu-mutant: $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) $(BUILD)/attrib_mut_program.bin $(BUILD)/label_mut_program.bin $(BUILD)/dosutil_cache.img
	@set -e; for m in attrib label cache; do \
	  a=$(ATTRIB_PROG_BIN); l=$(LABEL_PROG_BIN); boot=$(SECONDARY_TRACER_IMG); \
	  case $$m in attrib) a=$(BUILD)/attrib_mut_program.bin; why='E1 ATTRIB';; \
	    label) l=$(BUILD)/label_mut_program.bin; why='E4 VOL no-label after Y';; \
	    cache) boot=$(BUILD)/dosutil_cache.img; why='E4 VOL no-label after Y';; esac; \
	  log=$(BUILD)/dosutil_emu_mut_$$m.log; \
	  if INITECH_QMP_STDIO=1 sh harness/diff/fat_diff/dosutil_emu.sh $$boot $$a $$l mut_$$m >$$log 2>&1; then echo "FAIL: emu $$m survived"; exit 1; fi; \
	  grep "^FAIL: $$why" $$log || { cat $$log; exit 1; }; \
	  echo "PASS: $$m QEMU mutant caught RED"; \
	done
TEST_UNIT_GATES += test-attrib test-attrib-mutant test-label-vol test-label-vol-mutant
TEST_EMU_GATES += test-dosutil-emu test-dosutil-emu-mutant

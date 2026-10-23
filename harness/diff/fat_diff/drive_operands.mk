# initech-q45q: CD and the external commands resolve drive operands against
# the DEC-18 mounted volumes (audit 2026-10-23-dos L011-L015, L028, L032).
# One emulator trace on a two-volume boot disk; mtools/fsck.fat judge it.
DRIVE_OPS := harness/diff/fat_diff/drive_operands.sh
DRIVE_OPS_EXT := $(XCOPY_PROG_BIN) $(TREE_PROG_BIN) $(ATTRIB_PROG_BIN) $(LABEL_PROG_BIN) $(CHKDSK_PROG_BIN) $(FDISK_PROG_BIN)
DRIVE_OPS_DEPS := $(HARNESS_BIN) $(SECONDARY_TRACER_IMG) $(BUILD)/transient_flairtenants.bin $(ZUVO_HOST) $(DRIVE_OPS_EXT)
.PHONY: test-drive-operands
test-drive-operands: $(DRIVE_OPS_DEPS)
	@sh $(DRIVE_OPS) emu $(SECONDARY_TRACER_IMG) green $(DRIVE_OPS_EXT)
	@sh $(DRIVE_OPS) fdisk $(SECONDARY_TRACER_IMG) green $(DRIVE_OPS_EXT)
TEST_EMU_GATES += test-drive-operands

# Rule 6: each fix reverted in isolation must turn its own finding RED.
$(eval $(call zuvo-com,xcopy_drive_mut,-DDU_MUT_DRIVE,))
$(eval $(call zuvo-com,tree_drive_mut,-DDU_MUT_DRIVE,-DTREE_PROGRAM))
$(eval $(call dosutil-com,attrib_drive_mut,-DDOSUTIL_ATTRIB,-DDU_MUTATE_DRIVE))
$(eval $(call fdisk-emu-mutant,LETTER))
$(eval $(call dos-safety-shell-mutant,cd_drive,CMD_MUTATE_CD_DRIVE))
DRIVE_OPS_MUTANTS := xcopy tree attrib fdisk cd
.PHONY: test-drive-operands-mutant
test-drive-operands-mutant: $(DRIVE_OPS_DEPS) $(BUILD)/xcopy_drive_mut_program.bin $(BUILD)/tree_drive_mut_program.bin $(BUILD)/attrib_drive_mut_program.bin $(BUILD)/fdisk_LETTER.bin $(BUILD)/tracer_mut_safety_cd_drive.img
	@set -eu; mkdir -p $(BUILD)/drive-operands; for m in $(DRIVE_OPS_MUTANTS); do \
	 x=$(XCOPY_PROG_BIN); t=$(TREE_PROG_BIN); a=$(ATTRIB_PROG_BIN); l=$(LABEL_PROG_BIN); c=$(CHKDSK_PROG_BIN); f=$(FDISK_PROG_BIN); \
	 boot=$(SECONDARY_TRACER_IMG); mode=emu; \
	 case $$m in \
	  xcopy) x=$(BUILD)/xcopy_drive_mut_program.bin; why='L032 XCOPY overwrote A:\DHOME\ONE.TXT';; \
	  tree) t=$(BUILD)/tree_drive_mut_program.bin; why='L014 TREE D: walked the wrong drive/volume';; \
	  attrib) a=$(BUILD)/attrib_drive_mut_program.bin; why='L012 ATTRIB +R did not reach D: (mattrib)';; \
	  fdisk) f=$(BUILD)/fdisk_LETTER.bin; mode=fdisk; why='L028 FDISK second primary not D:';; \
	  cd) boot=$(BUILD)/tracer_mut_safety_cd_drive.img; why='L011 CD D: did not display D:\DHOME';; \
	 esac; log=$(BUILD)/drive-operands/mut-$$m.log; \
	 if sh $(DRIVE_OPS) $$mode $$boot mut-$$m $$x $$t $$a $$l $$c $$f > $$log 2>&1; then echo "FAIL drive-operands $$m mutant survived"; exit 1; fi; \
	 grep -Fx "RED drive-operands $$why" $$log || { cat $$log; exit 1; }; \
	 echo "GREEN test-drive-operands-mutant: $$m caught"; done
TEST_EMU_GATES += test-drive-operands-mutant

TOP := ..
COMPONENT := dos
include $(TOP)/mk/config.mk
TARGET_INCLUDES := -I$(TOP)/kernel/include -I$(TOP)/build/kernel -Icommand -Icommon -Ichkdsk -Idiskutil -Ifdisk -Iedlin -Ixcopy
include $(TOP)/mk/target.mk
PROGRAMS := MORE FIND SORT TPSREP CHKDSK EDLIN FDISK FORMAT SYS DISKCOPY DISKCOMP XCOPY TREE ATTRIB LABEL
all: $(TOP)/build/kernel/dos_messages.h resident $(addprefix $(OUT)/,$(addsuffix .COM,$(PROGRAMS)))
resident: $(OUT)/command-resident.o $(OUT)/batch.o $(OUT)/env.o
$(OUT)/command-resident.o: command/command.c FORCE | $(OUT)
	$(call run-command,$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -DCOMMAND_KERNEL_REPL -MMD -MP -c $< -o $@)
$(OUT)/batch.o: command/batch.c $(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/env.o: command/env.c $(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -MMD -MP -c $< -o $@
$(OUT)/MORE.COM: more/more_program.asm | $(OUT)
	$(NASM) -f bin $< -o $@
$(OUT)/FIND.COM: find/find_program.asm $(TOP)/build/kernel/dos_messages.inc | $(OUT)
	$(NASM) -I$(TOP)/build/kernel/ -f bin $< -o $@
$(OUT)/SORT.COM: sort/sort_program.asm $(TOP)/build/kernel/dos_messages.inc | $(OUT)
	$(NASM) -I$(TOP)/build/kernel/ -f bin $< -o $@
$(OUT)/TPSREP.COM: tpsrep/tpsrep_program.asm | $(OUT)
	$(NASM) -f bin $< -o $@
define c_program
$(OUT)/$(1).elf: $(addprefix $(OUT)/,$(2)) $(3)
	$$(TARGET_LD) $$(TARGET_LDFLAGS) -T $(3) -o $$@ $(addprefix $(OUT)/,$(2))
$(OUT)/$(1).COM: $(OUT)/$(1).elf
	objcopy -O binary $$< $$@
endef
$(eval $(call c_program,CHKDSK,chkdsk/chkdsk_start.o chkdsk/chkdsk.o chkdsk/chkdsk_program.o chkdsk/mcb.o,chkdsk/chkdsk.ld))
$(eval $(call c_program,EDLIN,edlin/edlin_start.o edlin/edlin.o edlin/edlin_program.o,edlin/edlin.ld))
$(eval $(call c_program,FDISK,fdisk/fdisk_start.o fdisk/fdisk.o fdisk/fdisk_program.o,fdisk/fdisk.ld))
$(OUT)/%_start.o: %_start.asm | $(OUT)
	@mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@
$(OUT)/xcopy/xcopy_tree_entry.o: xcopy/xcopy_tree_entry.asm | $(OUT)
	@mkdir -p $(@D)
	$(NASM) -f elf32 $< -o $@
$(OUT)/xcopy/tree_entry.o: xcopy/xcopy_tree_entry.asm | $(OUT)
	@mkdir -p $(@D)
	$(NASM) -DTREE_PROGRAM -f elf32 $< -o $@
$(eval $(call c_program,XCOPY,xcopy/xcopy_tree_entry.o xcopy/xcopy_tree.o,xcopy/xcopy_tree.ld))
$(eval $(call c_program,TREE,xcopy/tree_entry.o xcopy/xcopy_tree.o,xcopy/xcopy_tree.ld))
define disk_program
$(OUT)/diskutil/$(1).o: diskutil/diskutil_program.c $$(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	@mkdir -p $$(@D)
	$$(TARGET_CC) $$(TARGET_CFLAGS) $$(TARGET_INCLUDES) -DDU_KIND=$(2) -MMD -MP -c $$< -o $$@
$(eval $(call c_program,$(1),diskutil/diskutil_start.o diskutil/diskutil.o diskutil/$(1).o,chkdsk/chkdsk.ld))
endef
$(eval $(call disk_program,FORMAT,0))
$(eval $(call disk_program,SYS,1))
$(eval $(call disk_program,DISKCOPY,2))
$(eval $(call disk_program,DISKCOMP,3))
$(OUT)/common/attrib.o: common/dosutil_target.c $(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -DDOSUTIL_ATTRIB -MMD -MP -c $< -o $@
$(OUT)/common/label.o: common/dosutil_target.c $(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -MMD -MP -c $< -o $@
$(eval $(call c_program,ATTRIB,common/attrib.o common/dosutil.o,common/dosutil.ld))
$(eval $(call c_program,LABEL,common/label.o common/dosutil.o,common/dosutil.ld))
smoke:
	$(MAKE) -C $(TOP) smoke
$(OUT)/chkdsk/mcb.o: $(TOP)/kernel/mcb.c $(filter-out %.d,$(MAKEFILE_LIST)) | $(OUT)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_INCLUDES) -MMD -MP -c $< -o $@
all: $(OUT)/COMMAND.COM
$(OUT)/COMMAND.COM: command/forward.asm | $(OUT)
	$(NASM) -f bin $< -o $@

$(TOP)/build/kernel/dos_messages.h: $(TOP)/kernel/include/dos_messages.json $(TOP)/kernel/gen_messages.c
	$(MAKE) -C $(TOP)/kernel $@
$(TOP)/build/kernel/dos_messages.inc: $(TOP)/build/kernel/dos_messages.h
	$(MAKE) -C $(TOP)/kernel $@
$(OUT)/command-resident.o $(OUT)/common/dosutil.o $(OUT)/edlin/edlin.o $(OUT)/chkdsk/chkdsk.o $(OUT)/xcopy/xcopy_tree.o: $(TOP)/build/kernel/dos_messages.h

include tests-programs.mk

# Host test wiring (separate from target build rules).
include tests.mk

TARGET_ARTIFACTS := $(addprefix $(OUT)/,$(addsuffix .COM,$(PROGRAMS))) $(OUT)/COMMAND.COM
include $(TOP)/mk/import-check.mk

.PHONY: smoke-errors
smoke-errors:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/copy-state.sh "$(TOP)" "$(TOP)/build/image/boot.img" "$(OUT)" "$(OUT)/smoke-errors"

.PHONY: smoke-batch-return
smoke-batch-return:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/batch-return.sh "$(TOP)" "$(OUT)/smoke-batch-return"

.PHONY: smoke-unknown-redirect
smoke-unknown-redirect:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/unknown-redirect.sh "$(TOP)" "$(OUT)/smoke-unknown-redirect"

.PHONY: smoke-dir-wide
smoke-dir-wide:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/dir-wide.sh "$(TOP)" "$(OUT)/smoke-dir-wide"

.PHONY: smoke-pipe-names
smoke-pipe-names:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/pipe-names.sh "$(TOP)" "$(OUT)/smoke-pipe-names"

.PHONY: smoke-control-syntax
smoke-control-syntax:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/control-syntax.sh "$(TOP)" "$(OUT)/smoke-control-syntax"

.PHONY: smoke-diagnostics
smoke-diagnostics:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/diagnostics.sh "$(TOP)" ALL "$(OUT)/smoke-diagnostics"

.PHONY: smoke-utility-errors
smoke-utility-errors:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/utility-errors.sh "$(TOP)" ALL "$(OUT)/smoke-utility-errors"

.PHONY: smoke-fat16
smoke: smoke-fat16
smoke-fat16:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/fat16-policy.sh "$(TOP)" "$(OUT)/smoke-fat16"

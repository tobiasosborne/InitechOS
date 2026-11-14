# Tier 0 import wiring; target rules remain in Makefile.
HOST_INCLUDES := -I. -Iinclude -Itests -Itests/disk -I$(TOP)/kernel/tests/disk -I$(TOP)/kernel/include -I$(TOP)/factory/testlib -I$(TOP)/build/kernel
HOST_INCLUDES += -Icommand -Icommand/tests -Icommon -Ichkdsk -Idiskutil -Ifdisk -Iedlin -Ixcopy
TEST_HDRS := $(shell find . -name '*.h') $(wildcard $(TOP)/kernel/include/*.h) $(wildcard $(TOP)/factory/testlib/*.h)
PYTHON_FAT_REFERENCE ?= python3
PYTHON_MESSAGE_REFERENCE ?= python3
THOROUGH ?= 0
export PYTHON_FAT_REFERENCE PYTHON_MESSAGE_REFERENCE THOROUGH
HOST_TESTS :=
EXTRA_HOST_TESTS :=
EXTRA_HOST_TESTS += tool_dosutil_host
tool_dosutil_host_SRCS := common/tests/dosutil_host.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_dosutil_host_OBJS := $(OUT)/host-objects/dosutil-2bbce255b87c.o $(OUT)/host-objects/fat12-c53876edf94b.o
tool_dosutil_host_CFLAGS :=
tool_dosutil_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_cmdline_fuzz
tool_test_cmdline_fuzz_SRCS := command/tests/test_cmdline_fuzz.c
tool_test_cmdline_fuzz_OBJS := $(OUT)/host-objects/command-6f7da0e1144b.o $(OUT)/host-objects/psp-eb3f1db49d78.o
tool_test_cmdline_fuzz_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_cmdline_fuzz_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_command
tool_test_command_SRCS := command/tests/test_command.c
tool_test_command_OBJS := $(OUT)/host-objects/command-6f7da0e1144b.o $(OUT)/host-objects/env-661bafd32e3d.o
tool_test_command_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_command_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_shell_files
tool_shell_files_SRCS := command/tests/shell_files.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_shell_files_OBJS := $(OUT)/host-objects/int21-41b0a9e2801e.o $(OUT)/host-objects/devices-e6c3495171d1.o $(OUT)/host-objects/fileio_fat-896fc6a876ce.o $(OUT)/host-objects/fat12-7576844d4c54.o $(OUT)/host-objects/sft-5b6257e27af5.o $(OUT)/host-objects/psp-d97b019f089d.o $(OUT)/host-objects/mcb-9a3c873f4284.o $(OUT)/host-objects/irq-c786d838cf55.o
tool_shell_files_CFLAGS := -Wno-unused-function -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections
tool_shell_files_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_batch_fat
tool_batch_fat_SRCS := command/tests/batch_fat.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_batch_fat_OBJS := $(tool_shell_files_OBJS)
tool_batch_fat_CFLAGS := $(tool_shell_files_CFLAGS)
tool_batch_fat_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_dir_look
tool_test_dir_look_SRCS := command/tests/test_dir_look.c
tool_test_dir_look_OBJS := $(OUT)/host-objects/command-088235e8725b.o $(OUT)/host-objects/env-1eaae495f78b.o
tool_test_dir_look_CFLAGS :=
tool_test_dir_look_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_dir_wide
tool_dir_wide_SRCS := command/tests/dir_wide.c
tool_dir_wide_OBJS := $(tool_test_dir_look_OBJS)
tool_dir_wide_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_dos_parity_host
tool_dos_parity_host_SRCS := command/tests/dos_parity_host.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_dos_parity_host_OBJS := $(OUT)/host-objects/int21-6dcaff2f5d71.o $(OUT)/host-objects/devices-70b1d0cc3edd.o $(OUT)/host-objects/fileio_fat-314c925ae35c.o $(OUT)/host-objects/fat12-c8c444f0ab12.o $(OUT)/host-objects/sft-3063b0aa3f1c.o $(OUT)/host-objects/psp-76d4b3d645be.o $(OUT)/host-objects/mcb-10b377485f71.o $(OUT)/host-objects/irq-c14035804001.o
tool_dos_parity_host_CFLAGS := -D_POSIX_C_SOURCE=200809L -no-pie
tool_dos_parity_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_xcopy_tree
tool_test_xcopy_tree_SRCS := xcopy/tests/test_xcopy_tree.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_test_xcopy_tree_OBJS := $(OUT)/host-objects/xcopy_tree-48560c59d877.o $(OUT)/host-objects/int21-2e46c3106dd4.o $(OUT)/host-objects/devices-3389831f3340.o $(OUT)/host-objects/fileio_fat-74a0e82213dc.o $(OUT)/host-objects/fat12-b333e07e23d8.o $(OUT)/host-objects/sft-48a52177ceda.o $(OUT)/host-objects/psp-e10fbc568cee.o $(OUT)/host-objects/mcb-39d20775e2b5.o $(OUT)/host-objects/irq-695258041d4e.o
tool_test_xcopy_tree_CFLAGS := -no-pie
tool_test_xcopy_tree_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_edlin_model
tool_edlin_model_SRCS := edlin/tests/model.c
tool_edlin_model_CFLAGS :=
tool_edlin_model_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_edlin_host
tool_edlin_host_SRCS := edlin/tests/host.c
tool_edlin_host_OBJS := $(OUT)/host-objects/edlin-5237c1351320.o
tool_edlin_host_CFLAGS :=
tool_edlin_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_env
tool_test_env_SRCS := command/tests/test_env.c
tool_test_env_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_env_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_fat_dump
tool_fat_dump_SRCS := $(TOP)/kernel/tests/disk/fat_dump.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_fat_dump_OBJS := $(OUT)/host-objects/fat12-e1f584162def.o
tool_fat_dump_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_fat_dump_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_dosutil_vol
tool_dosutil_vol_SRCS := command/tests/dosutil_vol.c
tool_dosutil_vol_OBJS := $(OUT)/host-objects/command-bc9776434d5f.o
tool_dosutil_vol_CFLAGS := -ffunction-sections -fdata-sections -Wl,--gc-sections
tool_dosutil_vol_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_batch
tool_test_batch_SRCS := command/tests/test_batch.c
tool_test_batch_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_batch_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_batch_break
tool_batch_break_SRCS := command/tests/batch_break.c
tool_batch_break_CFLAGS := -Wno-unused-function
tool_batch_break_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_batch_exec
tool_test_batch_exec_SRCS := command/tests/test_batch_exec.c
tool_test_batch_exec_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_batch_exec_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_chkdsk_host
tool_chkdsk_host_SRCS := chkdsk/tests/chkdsk_host.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_chkdsk_host_OBJS := $(OUT)/host-objects/chkdsk-ef8dae9bc4da.o $(OUT)/host-objects/mcb-e056f992b120.o
tool_chkdsk_host_CFLAGS :=
tool_chkdsk_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_chkdsk_fixture
tool_chkdsk_fixture_SRCS := chkdsk/tests/chkdsk_fixture.c
tool_chkdsk_fixture_CFLAGS := -Wno-misleading-indentation
tool_chkdsk_fixture_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_batch2_host
tool_batch2_host_SRCS := command/tests/batch2_host.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_batch2_host_OBJS := $(OUT)/host-objects/int21-6dcaff2f5d71.o $(OUT)/host-objects/devices-70b1d0cc3edd.o $(OUT)/host-objects/fileio_fat-314c925ae35c.o $(OUT)/host-objects/fat12-c8c444f0ab12.o $(OUT)/host-objects/sft-3063b0aa3f1c.o $(OUT)/host-objects/psp-76d4b3d645be.o $(OUT)/host-objects/mcb-10b377485f71.o $(OUT)/host-objects/irq-c14035804001.o $(OUT)/host-objects/batch-b9734ff94f6a.o
tool_batch2_host_CFLAGS := -D_POSIX_C_SOURCE=200809L -no-pie
tool_batch2_host_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_dos_safety_fixture
tool_dos_safety_fixture_SRCS := $(TOP)/kernel/tests/disk/dos_safety_fixture.c
tool_dos_safety_fixture_CFLAGS :=
tool_dos_safety_fixture_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_dos_safety
tool_test_dos_safety_SRCS := $(TOP)/kernel/tests/test_dos_safety.c $(TOP)/kernel/tests/disk/blockdev_file.c
tool_test_dos_safety_OBJS := $(OUT)/host-objects/int21-1f197a3d70b9.o $(OUT)/host-objects/devices-da2de61aace1.o $(OUT)/host-objects/fileio_fat-e6b6f9bfc956.o $(OUT)/host-objects/fat12-e1f584162def.o $(OUT)/host-objects/sft-8428bf753c51.o $(OUT)/host-objects/psp-eb3f1db49d78.o $(OUT)/host-objects/mcb-32a259fe9985.o $(OUT)/host-objects/irq-61e6a6971d43.o
tool_test_dos_safety_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_dos_safety_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_fdisk
tool_test_fdisk_SRCS := fdisk/tests/test_fdisk.c
tool_test_fdisk_OBJS := $(OUT)/host-objects/fdisk-f1b04e32887e.o
tool_test_fdisk_CFLAGS :=
tool_test_fdisk_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_redir
tool_test_redir_SRCS := $(TOP)/kernel/tests/test_redir.c
tool_test_redir_OBJS := $(OUT)/host-objects/int21-1f197a3d70b9.o $(OUT)/host-objects/devices-da2de61aace1.o $(OUT)/host-objects/mcb-32a259fe9985.o $(OUT)/host-objects/sft-8428bf753c51.o $(OUT)/host-objects/psp-eb3f1db49d78.o $(OUT)/host-objects/irq-61e6a6971d43.o
tool_test_redir_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_redir_DEPS := $(TOP)/build/kernel/dos_messages.h
EXTRA_HOST_TESTS += tool_test_redir_parse
tool_test_redir_parse_SRCS := command/tests/test_redir_parse.c
tool_test_redir_parse_OBJS := $(OUT)/host-objects/command-6f7da0e1144b.o $(OUT)/host-objects/env-661bafd32e3d.o
tool_test_redir_parse_CFLAGS := -D_POSIX_C_SOURCE=200809L
tool_test_redir_parse_DEPS := $(TOP)/build/kernel/dos_messages.h
export HOST_TEST_OUTPUT := $(abspath $(OUT))/host

# Preserve the untuned legacy host compilation (tests include large bounded models).
$(foreach t,$(EXTRA_HOST_TESTS),$(eval $(t)_CFLAGS += -O0))

# The independent EDLIN model intentionally bounds replacement output.
tool_edlin_model_CFLAGS += -Wno-format-truncation
$(OUT)/host-objects/dosutil-2bbce255b87c.o: common/dosutil.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-c53876edf94b.o: $(TOP)/kernel/fat12.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/command-6f7da0e1144b.o: command/command.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-eb3f1db49d78.o: $(TOP)/kernel/psp.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/env-661bafd32e3d.o: command/env.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-41b0a9e2801e.o: $(TOP)/kernel/int21.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-e6c3495171d1.o: $(TOP)/kernel/devices.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-896fc6a876ce.o: $(TOP)/kernel/fileio_fat.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-7576844d4c54.o: $(TOP)/kernel/fat12.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-5b6257e27af5.o: $(TOP)/kernel/sft.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-d97b019f089d.o: $(TOP)/kernel/psp.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-9a3c873f4284.o: $(TOP)/kernel/mcb.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-c786d838cf55.o: $(TOP)/kernel/irq.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -Wno-unused-function -fdata-sections -ffunction-sections -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/command-088235e8725b.o: command/command.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/env-1eaae495f78b.o: command/env.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-6dcaff2f5d71.o: $(TOP)/kernel/int21.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-70b1d0cc3edd.o: $(TOP)/kernel/devices.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-314c925ae35c.o: $(TOP)/kernel/fileio_fat.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-c8c444f0ab12.o: $(TOP)/kernel/fat12.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-3063b0aa3f1c.o: $(TOP)/kernel/sft.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-76d4b3d645be.o: $(TOP)/kernel/psp.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-10b377485f71.o: $(TOP)/kernel/mcb.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-c14035804001.o: $(TOP)/kernel/irq.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/xcopy_tree-48560c59d877.o: xcopy/xcopy_tree.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-2e46c3106dd4.o: $(TOP)/kernel/int21.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-3389831f3340.o: $(TOP)/kernel/devices.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-74a0e82213dc.o: $(TOP)/kernel/fileio_fat.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-b333e07e23d8.o: $(TOP)/kernel/fat12.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-48a52177ceda.o: $(TOP)/kernel/sft.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/psp-e10fbc568cee.o: $(TOP)/kernel/psp.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-39d20775e2b5.o: $(TOP)/kernel/mcb.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-695258041d4e.o: $(TOP)/kernel/irq.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/edlin-5237c1351320.o: edlin/edlin.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fat12-e1f584162def.o: $(TOP)/kernel/fat12.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/command-bc9776434d5f.o: command/command.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -Wl,--gc-sections -fdata-sections -ffunction-sections -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/chkdsk-ef8dae9bc4da.o: chkdsk/chkdsk.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-e056f992b120.o: $(TOP)/kernel/mcb.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/batch-b9734ff94f6a.o: command/batch.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -no-pie -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/int21-1f197a3d70b9.o: $(TOP)/kernel/int21.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/devices-da2de61aace1.o: $(TOP)/kernel/devices.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fileio_fat-e6b6f9bfc956.o: $(TOP)/kernel/fileio_fat.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/sft-8428bf753c51.o: $(TOP)/kernel/sft.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/mcb-32a259fe9985.o: $(TOP)/kernel/mcb.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/irq-61e6a6971d43.o: $(TOP)/kernel/irq.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES) -D_POSIX_C_SOURCE=200809L -O0 -MMD -MP -c $< -o $@
$(OUT)/host-objects/fdisk-f1b04e32887e.o: fdisk/fdisk.c $(TEST_HDRS) $(filter-out %.d,$(MAKEFILE_LIST)) $(TOP)/build/kernel/dos_messages.h
	@mkdir -p $(@D)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_INCLUDES)  -O0 -MMD -MP -c $< -o $@
-include $(wildcard $(OUT)/host-objects/*.d)
TEST_HDRS += $(wildcard *.c) $(wildcard $(TOP)/kernel/*.c) $(wildcard command/*.c) $(wildcard $(TOP)/dos/command/*.c)

export HOST_TEST_SOURCE_ROOT := $(abspath $(TOP))
export HOST_TEST_COMPONENT := dos

EXTRA_HOST_TESTS += tool_formatsys_host
tool_formatsys_host_SRCS := diskutil/tests/formatsys_host.c diskutil/diskutil.c $(TOP)/kernel/block_device.c
tool_formatsys_host_CFLAGS := -DBLOCK_DEVICE_HOST -O0

EXTRA_HOST_TESTS += tool_formatsys_check
tool_formatsys_check_SRCS := diskutil/tests/formatsys_check.c
tool_formatsys_check_CFLAGS :=  -O0

EXTRA_HOST_TESTS += tool_dir_match
tool_dir_match_SRCS := command/tests/dir_match.c $(TOP)/kernel/int21.c $(TOP)/kernel/devices.c $(TOP)/kernel/mcb.c $(TOP)/kernel/sft.c $(TOP)/kernel/psp.c $(TOP)/kernel/irq.c $(TOP)/kernel/config_sys.c
tool_dir_match_CFLAGS := -DINT21_WILDCARD_TESTSEAM -O0

EXTRA_HOST_TESTS += tool_test_dir_disk
tool_test_dir_disk_SRCS := command/tests/test_dir_disk.c $(TOP)/kernel/tests/disk/blockdev_file.c $(TOP)/kernel/fat12.c $(TOP)/kernel/fileio_fat.c
tool_test_dir_disk_CFLAGS :=  -O0

EXTRA_HOST_TESTS += tool_dosmsg_scan
tool_dosmsg_scan_SRCS := $(TOP)/kernel/tests/dosmsg_scan.c

# Generated catalogue must exist before any parallel host compilation.
$(foreach t,$(EXTRA_HOST_TESTS),$(eval $(t)_DEPS += $(TOP)/build/kernel/dos_messages.h))

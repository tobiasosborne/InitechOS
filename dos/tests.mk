include tests/tools.mk
HOST_TESTS += batch-capacity
batch-capacity_SRCS := command/tests/batch_capacity.c
batch-capacity_CFLAGS := $(tool_batch_break_CFLAGS)
batch-capacity_DEPS := $(TOP)/build/kernel/dos_messages.h
HOST_TESTS += test-attrib
test-attrib_SRCS := tests/runner.c
test-attrib_ARGS := tests/run.sh test-attrib
test-attrib_DEPS := $(OUT)/host/tool_dosutil_host tests/run.sh tests/gates/test-attrib.sh
HOST_TESTS += test-cmdline-fuzz
test-cmdline-fuzz_SRCS := tests/runner.c
test-cmdline-fuzz_ARGS := tests/run.sh test-cmdline-fuzz
test-cmdline-fuzz_DEPS := $(OUT)/host/tool_test_cmdline_fuzz tests/run.sh tests/gates/test-cmdline-fuzz.sh
HOST_TESTS += test-command
test-command_SRCS := tests/runner.c
test-command_ARGS := tests/run.sh test-command
test-command_DEPS := $(OUT)/host/tool_test_command tests/run.sh tests/gates/test-command.sh
HOST_TESTS += test-del-directory-host
test-del-directory-host_SRCS := tests/runner.c
test-del-directory-host_ARGS := tests/run.sh test-del-directory-host
test-del-directory-host_DEPS := $(OUT)/host/tool_shell_files tests/run.sh tests/gates/test-del-directory-host.sh
HOST_TESTS += test-dir-look
test-dir-look_SRCS := tests/runner.c
test-dir-look_ARGS := tests/run.sh test-dir-look
test-dir-look_DEPS := $(OUT)/host/tool_test_dir_look tests/run.sh tests/gates/test-dir-look.sh
HOST_TESTS += test-dos-stamp-copy-host
test-dos-stamp-copy-host_SRCS := tests/runner.c
test-dos-stamp-copy-host_ARGS := tests/run.sh test-dos-stamp-copy-host
test-dos-stamp-copy-host_DEPS := $(OUT)/host/tool_dos_parity_host $(OUT)/host/tool_test_xcopy_tree tests/run.sh tests/gates/test-dos-stamp-copy-host.sh
HOST_TESTS += test-dos-stamp-host
test-dos-stamp-host_SRCS := tests/runner.c
test-dos-stamp-host_ARGS := tests/run.sh test-dos-stamp-host
test-dos-stamp-host_DEPS := $(OUT)/host/tool_dos_parity_host tests/run.sh tests/gates/test-dos-stamp-host.sh
HOST_TESTS += test-edlin-host
test-edlin-host_SRCS := tests/runner.c
test-edlin-host_ARGS := tests/run.sh test-edlin-host
test-edlin-host_DEPS := $(OUT)/host/tool_edlin_model $(OUT)/host/tool_edlin_host tests/run.sh tests/gates/test-edlin-host.sh
HOST_TESTS += test-env
test-env_SRCS := tests/runner.c
test-env_ARGS := tests/run.sh test-env
test-env_DEPS := $(OUT)/host/tool_test_env tests/run.sh tests/gates/test-env.sh
HOST_TESTS += test-label-vol
test-label-vol_SRCS := tests/runner.c
test-label-vol_ARGS := tests/run.sh test-label-vol
test-label-vol_DEPS := $(OUT)/host/tool_dosutil_host $(OUT)/host/tool_dosutil_vol tests/run.sh tests/gates/test-label-vol.sh
HOST_TESTS += test-batch
test-batch_SRCS := tests/runner.c
test-batch_ARGS := tests/run.sh test-batch
test-batch_DEPS := $(OUT)/host/tool_test_batch tests/run.sh tests/gates/test-batch.sh
HOST_TESTS += test-batch-break-host
test-batch-break-host_SRCS := tests/runner.c
test-batch-break-host_ARGS := tests/run.sh test-batch-break-host
test-batch-break-host_DEPS := $(OUT)/host/tool_batch_break tests/run.sh tests/gates/test-batch-break-host.sh
HOST_TESTS += test-batch-exec
test-batch-exec_SRCS := tests/runner.c
test-batch-exec_ARGS := tests/run.sh test-batch-exec
test-batch-exec_DEPS := $(OUT)/host/tool_test_batch_exec tests/run.sh tests/gates/test-batch-exec.sh
HOST_TESTS += test-chkdsk
test-chkdsk_SRCS := tests/runner.c
test-chkdsk_ARGS := tests/run.sh test-chkdsk
test-chkdsk_DEPS := $(OUT)/host/tool_chkdsk_host $(OUT)/host/tool_chkdsk_fixture tests/run.sh tests/gates/test-chkdsk.sh
HOST_TESTS += test-dos-batch2-host
test-dos-batch2-host_SRCS := tests/runner.c
test-dos-batch2-host_ARGS := tests/run.sh test-dos-batch2-host
test-dos-batch2-host_DEPS := $(OUT)/host/tool_batch2_host tests/run.sh tests/gates/test-dos-batch2-host.sh
HOST_TESTS += test-dos-safety-create
test-dos-safety-create_SRCS := tests/runner.c
test-dos-safety-create_ARGS := tests/run.sh test-dos-safety-create
test-dos-safety-create_DEPS := $(OUT)/host/tool_dos_safety_fixture $(OUT)/host/tool_test_dos_safety tests/run.sh tests/gates/test-dos-safety-create.sh
HOST_TESTS += test-dos-safety-create-ro
test-dos-safety-create-ro_SRCS := tests/runner.c
test-dos-safety-create-ro_ARGS := tests/run.sh test-dos-safety-create-ro
test-dos-safety-create-ro_DEPS := $(OUT)/host/tool_dos_safety_fixture $(OUT)/host/tool_test_dos_safety tests/run.sh tests/gates/test-dos-safety-create-ro.sh
HOST_TESTS += test-dos-safety-identity
test-dos-safety-identity_SRCS := tests/runner.c
test-dos-safety-identity_ARGS := tests/run.sh test-dos-safety-identity
test-dos-safety-identity_DEPS := $(OUT)/host/tool_dos_safety_fixture $(OUT)/host/tool_test_dos_safety tests/run.sh tests/gates/test-dos-safety-identity.sh
HOST_TESTS += test-dos-safety-unlink-ro
test-dos-safety-unlink-ro_SRCS := tests/runner.c
test-dos-safety-unlink-ro_ARGS := tests/run.sh test-dos-safety-unlink-ro
test-dos-safety-unlink-ro_DEPS := $(OUT)/host/tool_dos_safety_fixture $(OUT)/host/tool_test_dos_safety tests/run.sh tests/gates/test-dos-safety-unlink-ro.sh
HOST_TESTS += test-dos-safety-write-ro
test-dos-safety-write-ro_SRCS := tests/runner.c
test-dos-safety-write-ro_ARGS := tests/run.sh test-dos-safety-write-ro
test-dos-safety-write-ro_DEPS := $(OUT)/host/tool_dos_safety_fixture $(OUT)/host/tool_test_dos_safety tests/run.sh tests/gates/test-dos-safety-write-ro.sh
HOST_TESTS += test-fdisk
test-fdisk_SRCS := tests/runner.c
test-fdisk_ARGS := tests/run.sh test-fdisk
test-fdisk_DEPS := $(OUT)/host/tool_test_fdisk $(OUT)/host/tool_fat_dump tests/run.sh tests/gates/test-fdisk.sh
HOST_TESTS += test-redir
test-redir_SRCS := tests/runner.c
test-redir_ARGS := tests/run.sh test-redir
test-redir_DEPS := $(OUT)/host/tool_test_redir tests/run.sh tests/gates/test-redir.sh
HOST_TESTS += test-redir-parse
test-redir-parse_SRCS := tests/runner.c
test-redir-parse_ARGS := tests/run.sh test-redir-parse
test-redir-parse_DEPS := $(OUT)/host/tool_test_redir_parse tests/run.sh tests/gates/test-redir-parse.sh
HOST_TESTS += test-shell-files-host
test-shell-files-host_SRCS := tests/runner.c
test-shell-files-host_ARGS := tests/run.sh test-shell-files-host
test-shell-files-host_DEPS := $(OUT)/host/tool_shell_files tests/run.sh tests/gates/test-shell-files-host.sh
HOST_TESTS += test-xcopy-cwd-host
test-xcopy-cwd-host_SRCS := tests/runner.c
test-xcopy-cwd-host_ARGS := tests/run.sh test-xcopy-cwd-host
test-xcopy-cwd-host_DEPS := $(OUT)/host/tool_test_xcopy_tree tests/run.sh tests/gates/test-xcopy-cwd-host.sh
HOST_TESTS += test-xcopy-tree
test-xcopy-tree_SRCS := tests/runner.c
test-xcopy-tree_ARGS := tests/run.sh test-xcopy-tree
test-xcopy-tree_DEPS := $(OUT)/host/tool_test_xcopy_tree tests/run.sh tests/gates/test-xcopy-tree.sh
test-build: $(TOP)/build/kernel/dos_messages.h

MUTANT_PATCHES := $(wildcard */mutants/*.patch)

HOST_TESTS += test-formatsys
test-formatsys_SRCS := tests/runner.c
test-formatsys_ARGS := tests/run.sh test-formatsys
test-formatsys_DEPS := $(OUT)/host/tool_formatsys_host $(OUT)/host/tool_formatsys_check tests/gates/test-formatsys.sh tests/run.sh

HOST_TESTS += test-dir-match-host
test-dir-match-host_SRCS := tests/runner.c
test-dir-match-host_ARGS := tests/run.sh test-dir-match-host
test-dir-match-host_DEPS := $(OUT)/host/tool_dir_match tests/gates/test-dir-match-host.sh tests/run.sh

HOST_TESTS += test-dir-free-space
test-dir-free-space_SRCS := tests/runner.c
test-dir-free-space_ARGS := tests/run.sh test-dir-free-space
test-dir-free-space_DEPS := $(OUT)/host/tool_test_dir_disk tests/gates/test-dir-free-space.sh tests/run.sh

HOST_TESTS += test-dir-volume
test-dir-volume_SRCS := tests/runner.c
test-dir-volume_ARGS := tests/run.sh test-dir-volume
test-dir-volume_DEPS := $(OUT)/host/tool_test_dir_disk tests/gates/test-dir-volume.sh tests/run.sh





HOST_TESTS += test-dosmsg
test-dosmsg_SRCS := tests/runner.c
test-dosmsg_ARGS := tests/run.sh test-dosmsg
test-dosmsg_DEPS := $(OUT)/host/tool_dosmsg_scan $(addprefix $(OUT)/,$(addsuffix .COM,$(PROGRAMS))) tests/gates/test-dosmsg.sh
include $(TOP)/mk/host-test.mk
include $(TOP)/mk/mutants.mk

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
test-build: dos33-reference
.PHONY: dos33-reference
dos33-reference:
	@cd command/tests/dos33/reference && sha256sum -c SHA256SUMS >/dev/null

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
HOST_TESTS += test-copy-full
test-copy-full_SRCS := tests/runner.c
test-copy-full_ARGS := tests/run.sh test-copy-full
test-copy-full_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-copy-full.sh
HOST_TESTS += batch-physical
batch-physical_SRCS := command/tests/batch_physical.c
batch-physical_CFLAGS := $(tool_batch_break_CFLAGS)
batch-physical_DEPS := $(TOP)/build/kernel/dos_messages.h
HOST_TESTS += fdisk-session
fdisk-session_SRCS := fdisk/tests/session.c fdisk/fdisk.c
HOST_TESTS += fdisk-running-disk
fdisk-running-disk_SRCS := fdisk/tests/running_disk.c fdisk/fdisk_program.c fdisk/fdisk.c
fdisk-running-disk_DEPS := $(TOP)/build/kernel/dos_messages.h
HOST_TESTS += test-explicit-drive
test-explicit-drive_SRCS := tests/runner.c
test-explicit-drive_ARGS := tests/run.sh test-explicit-drive
test-explicit-drive_DEPS := $(OUT)/host/tool_dosutil_host tests/gates/test-explicit-drive.sh
HOST_TESTS += test-shell-failures
test-shell-failures_SRCS := tests/runner.c
test-shell-failures_ARGS := tests/run.sh test-shell-failures
test-shell-failures_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-shell-failures.sh
HOST_TESTS += test-xcopy-drive
test-xcopy-drive_SRCS := tests/runner.c
test-xcopy-drive_ARGS := tests/run.sh test-xcopy-drive
test-xcopy-drive_DEPS := $(OUT)/host/tool_test_xcopy_tree tests/gates/test-xcopy-drive.sh
HOST_TESTS += test-runtime-verdict
test-runtime-verdict_SRCS := tests/runner.c
test-runtime-verdict_ARGS := tests/run.sh test-runtime-verdict
test-runtime-verdict_DEPS := tests/gates/test-runtime-verdict.sh tests/runtime/classify.sh
HOST_TESTS += test-compat33-evidence
test-compat33-evidence_SRCS := tests/runner.c
test-compat33-evidence_ARGS := tests/run.sh test-compat33-evidence
test-compat33-evidence_DEPS := tests/gates/test-compat33-evidence.sh command/tests/compat33/EVIDENCE.sha256 find/tests/compat33/EVIDENCE.sha256 more/tests/compat33/EVIDENCE.sha256
HOST_TESTS += test-del-invalid-switch
test-del-invalid-switch_SRCS := tests/runner.c
test-del-invalid-switch_ARGS := tests/run.sh test-del-invalid-switch
test-del-invalid-switch_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-del-invalid-switch.sh
HOST_TESTS += test-ren-invalid-switch
test-ren-invalid-switch_SRCS := tests/runner.c
test-ren-invalid-switch_ARGS := tests/run.sh test-ren-invalid-switch
test-ren-invalid-switch_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-ren-invalid-switch.sh
HOST_TESTS += test-for-compat33
test-for-compat33_SRCS := tests/runner.c
test-for-compat33_ARGS := tests/run.sh test-for-compat33
test-for-compat33_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-for-compat33.sh
HOST_TESTS += test-if-exist-compat33
test-if-exist-compat33_SRCS := tests/runner.c
test-if-exist-compat33_ARGS := tests/run.sh test-if-exist-compat33
test-if-exist-compat33_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-if-exist-compat33.sh
HOST_TESTS += test-batch-name-compat33
test-batch-name-compat33_SRCS := tests/runner.c
test-batch-name-compat33_ARGS := tests/run.sh test-batch-name-compat33
test-batch-name-compat33_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-batch-name-compat33.sh
HOST_TESTS += test-command-batch-return
test-command-batch-return_SRCS := tests/runner.c
test-command-batch-return_ARGS := tests/run.sh test-command-batch-return
test-command-batch-return_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-command-batch-return.sh
HOST_TESTS += test-copy-name-compat33
test-copy-name-compat33_SRCS := tests/runner.c
test-copy-name-compat33_ARGS := tests/run.sh test-copy-name-compat33
test-copy-name-compat33_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-copy-name-compat33.sh
HOST_TESTS += test-copy-wildcard-compat33
test-copy-wildcard-compat33_COUNT_PATTERN := ^INITECH_CHECKS ([0-9]+) failures=[0-9]+$$
test-copy-wildcard-compat33_SRCS := tests/runner.c
test-copy-wildcard-compat33_ARGS := tests/run.sh test-copy-wildcard-compat33
test-copy-wildcard-compat33_DEPS := $(OUT)/host/tool_shell_files tests/gates/test-copy-wildcard-compat33.sh
HOST_TESTS += test-dir-wide-compat33
test-dir-wide-compat33_SRCS := tests/runner.c
test-dir-wide-compat33_ARGS := tests/run.sh test-dir-wide-compat33
test-dir-wide-compat33_DEPS := $(OUT)/host/tool_dir_wide tests/gates/test-dir-wide-compat33.sh
HOST_TESTS += test-utility-errors-compat33
test-utility-errors-compat33_SRCS := tests/runner.c
test-utility-errors-compat33_ARGS := tests/run.sh test-utility-errors-compat33
test-utility-errors-compat33_DEPS := $(OUT)/host/tool_chkdsk_host $(OUT)/host/tool_formatsys_host $(OUT)/host/tool_dosutil_host tests/gates/test-utility-errors-compat33.sh
SESSION_MUTANT_PATCHES := $(wildcard */mutants/runtime/*.patch)
.PHONY: mutants mutants-host mutants-sessions
mutants: mutants-host mutants-sessions
# Both sweeps reuse the repository catalogue in recursive builds.
mutants-host mutants-sessions: $(TOP)/build/kernel/dos_messages.inc
mutants-host:
	+@sh $(TOP)/factory/testlib/run-mutants.sh "$(abspath $(TOP))" "$(COMPONENT)" \
	  "$(abspath $(OUT))" "$(MAKE)" $(MUTANT_PATCHES)
mutants-sessions: $(OUT)/host/tool_dos_parity_host
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/controls.sh "$(TOP)" "$(OUT)/runtime-controls"
	+sh tests/runtime/mutants.sh "$(TOP)" $(SESSION_MUTANT_PATCHES)

HOST_TESTS += test-fat16-utilities
test-fat16-utilities_SRCS := tests/runner.c
test-fat16-utilities_COUNT_PATTERN := ^INITECH_CHECKS ([0-9]+) failures=[0-9]+$$
test-fat16-utilities_ARGS := tests/run.sh test-fat16-utilities
test-fat16-utilities_DEPS := $(OUT)/host/tool_fat16_host $(OUT)/host/tool_formatsys_host tests/gates/test-fat16-utilities.sh


HOST_TESTS += test-batch-transfer
test-batch-transfer_COUNT_PATTERN := ^INITECH_CHECKS ([0-9]+) failures=[0-9]+$$
test-batch-transfer_SRCS := tests/runner.c
test-batch-transfer_ARGS := tests/run.sh test-batch-transfer
test-batch-transfer_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-batch-transfer.sh
HOST_TESTS += test-batch-expand-compat33
test-batch-expand-compat33_COUNT_PATTERN := ^INITECH_CHECKS ([0-9]+) failures=[0-9]+$$
test-batch-expand-compat33_SRCS := tests/runner.c
test-batch-expand-compat33_ARGS := tests/run.sh test-batch-expand-compat33
test-batch-expand-compat33_DEPS := $(OUT)/host/tool_batch_fat tests/gates/test-batch-expand-compat33.sh command/tests/compat33/expected/PARAMS/files/TRACE.TXT command/tests/compat33/expected/BATCH/files/TRACE.TXT $(wildcard command/tests/dos33/reference/shift-window/*.txt)

.PHONY: smoke-sort-limits
smoke-sort-limits:
	+$(MAKE) -C $(TOP) image
	FILTER_BOUNDARIES=1 sh tests/runtime/sortlimits.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/sort-limits"

.PHONY: smoke-find-limits
smoke-find-limits:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/findlimits.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/find-limits"

.PHONY: smoke-filter-full
smoke-filter-full:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/filterfull.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/filter-full"

.PHONY: smoke-find-args
smoke: smoke-find-args
smoke-find-args:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/findargs.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/find-args"

.PHONY: smoke-pipe-cwd
smoke-pipe-cwd:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/pipecwd.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/pipe-cwd"

.PHONY: smoke-sort-columns
smoke-sort-columns:
	+$(MAKE) -C $(TOP) image
	sh tests/runtime/sortcolumns.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$(abspath $(OUT))/sort-columns"


HOST_TESTS += test-verdict-controls
test-verdict-controls_SRCS := tests/runner.c
test-verdict-controls_COUNT_PATTERN := ^INITECH_CHECKS ([0-9]+) failures=[0-9]+$$
test-verdict-controls_ARGS := tests/run.sh test-verdict-controls
test-verdict-controls_DEPS := $(OUT)/host/tool_dos_parity_host tests/gates/test-verdict-controls.sh tests/assertions.sh


HOST_TESTS += test-summary-controls
test-summary-controls_SRCS := tests/runner.c
test-summary-controls_ARGS := tests/run.sh test-summary-controls
test-summary-controls_DEPS := $(OUT)/host/tool_edlin_model $(OUT)/host/tool_dosutil_host $(OUT)/host/batch-capacity $(OUT)/host/fdisk-session $(OUT)/host/tool_test_dir_look $(OUT)/host/tool_test_cmdline_fuzz $(OUT)/host/tool_batch_fat tests/gates/test-summary-controls.sh

include $(TOP)/mk/host-test.mk

.PHONY: smoke-more-page
smoke: smoke-more-page
smoke-more-page:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/morepage.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$$(mktemp -d "$(abspath $(OUT))/more-page.XXXXXX")"

.PHONY: smoke-type-eof
smoke: smoke-type-eof
smoke-type-eof:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/type-eof.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$$(mktemp -d "$(abspath $(OUT))/type-eof.XXXXXX")"

.PHONY: smoke-shift-window
smoke: smoke-shift-window
smoke-shift-window:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/shift-window.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$(abspath $(OUT))" "$$(mktemp -d "$(abspath $(OUT))/shift-window.XXXXXX")"

.PHONY: smoke-shell-data-guards
smoke: smoke-shell-data-guards
smoke-shell-data-guards:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/shell-data-guards.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$$(mktemp -d "$(abspath $(OUT))/shell-data-guards.XXXXXX")"

.PHONY: smoke-builtin-switches
smoke: smoke-builtin-switches
smoke-builtin-switches:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/builtin-switches.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$$(mktemp -d "$(abspath $(OUT))/builtin-switches.XXXXXX")"

.PHONY: smoke-copy-omitted-binary
smoke: smoke-copy-omitted-binary
smoke-copy-omitted-binary:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/copy-omitted-binary.sh "$(abspath $(TOP))" "$(abspath $(TOP))/build/image/boot.img" "$$(mktemp -d "$(abspath $(OUT))/copy-omitted-binary.XXXXXX")"

.PHONY: smoke-c-command-startup
smoke: smoke-c-command-startup
smoke-c-command-startup:
	+$(MAKE) -C $(TOP) image
	+sh tests/runtime/c-command-startup.sh "$(abspath $(TOP))" "$$(mktemp -d "$(abspath $(OUT))/c-command-startup.XXXXXX")"

.PHONY: smoke-shell-c-sessions
smoke-shell-c-sessions:
	+$(MAKE) -C $(TOP) image
	@set -eu; root="$(abspath $(TOP))"; boot="$$root/build/image/c.img"; \
	out=$$(mktemp -d "$(abspath $(OUT))/shell-c-sessions.XXXXXX"); \
	for emulator in qemu bochs; do \
	    export DOS_SESSION_EMULATOR=$$emulator; \
	    for scenario in startup auto permanent basic; do \
	        sh "tests/runtime/$$scenario.sh" "$$root" "$$boot" "$(abspath $(OUT))" "$$out/$$emulator-$$scenario"; \
	    done; \
	    DOS_SESSION_BOOT="$$boot" sh tests/runtime/batch-return.sh "$$root" "$$out/$$emulator-batch-return"; \
	done

.PHONY: smoke-shell-c-files
smoke-shell-c-files:
	+$(MAKE) -C $(TOP) image
	@set -eu; root="$(abspath $(TOP))"; boot="$$root/build/image/c.img"; \
	out=$$(mktemp -d "$(abspath $(OUT))/shell-c-files.XXXXXX"); \
	export DOS_SESSION_BOOT="$$boot"; \
	for emulator in qemu bochs; do \
	    export DOS_SESSION_EMULATOR=$$emulator; \
	    for scenario in break callredir copy-state full pipecwd readerror shift-window type-eof volume; do \
	        sh "tests/runtime/$$scenario.sh" "$$root" "$$boot" "$(abspath $(OUT))" "$$out/$$emulator-$$scenario"; \
	    done; \
	    for scenario in control-syntax dir-wide pipe-names unknown-redirect; do \
	        sh "tests/runtime/$$scenario.sh" "$$root" "$$out/$$emulator-$$scenario"; \
	    done; \
	    sh tests/runtime/diagnostics.sh "$$root" ALL "$$out/$$emulator-diagnostics"; \
	done; \
	export DOS_SESSION_EMULATOR=qemu; \
	for scenario in datetime dirbreak interact; do \
	    sh "tests/runtime/$$scenario.sh" "$$root" "$$boot" "$(abspath $(OUT))" "$$out/qemu-$$scenario"; \
	done

.PHONY: mutants-shell-c
mutants-shell-c:
	+$(MAKE) -C $(TOP) image
	@set -eu; root="$(abspath $(TOP))"; export DOS_SESSION_BOOT="$$root/build/image/c.img"; \
	patches="$(filter command/%,$(SESSION_MUTANT_PATCHES))"; \
	DOS_SESSION_EMULATOR=qemu sh tests/runtime/mutants.sh "$$root" $$patches; \
	set --; \
	for patch in $$patches; do \
	    case "$$(sed -n 's/^# session: //p' "$$patch")" in \
	        datetime|dirbreak|interact) continue ;; \
	    esac; \
	    set -- "$$@" "$$patch"; \
	done; \
	DOS_SESSION_EMULATOR=bochs sh tests/runtime/mutants.sh "$$root" "$$@"

set -eu
check_suite=verdict-controls
. dos/tests/assertions.sh
mkdir -p build/fault-bin build/verdict-controls
printf '#!/bin/sh\nexit 1\n' > build/fault-bin/mformat
chmod +x build/fault-bin/mformat
for driver in tests/gates/test-explicit-drive.sh tests/gates/test-shell-failures.sh tests/gates/test-xcopy-drive.sh command/tests/shell_files.sh command/tests/del_directory.sh xcopy/tests/xcopy_cwd.sh tests/gates/test-dos-stamp-copy-host.sh; do
 name=$(basename "$driver" .sh)
 rc=0
 PATH="$PWD/build/fault-bin:$PATH" sh "${DOS_VERDICT_COMPONENT:-dos}/$driver" host build/unused > "build/verdict-controls/$name.log" 2>&1 || rc=$?
 [ "$rc" -ne 0 ] || fail "$name accepted failed mformat setup"
 if grep -E '^FAIL[ :]|^INITECH_CHECKS .*failures=[1-9]' "build/verdict-controls/$name.log"; then
  fail "$name turned mformat setup failure into semantic evidence"
 fi
 checks=$((checks+1))
done
# Check the assertion adapter itself: tool failure must retain error status,
# even if a previous actual property already passed.
rc=0
(sh -c '. dos/tests/assertions.sh; check_suite=control; check true test 1 = 1; check io cmp build/no-such-file build/no-other-file') > build/verdict-controls/io.log 2>&1 || rc=$?
check 'cmp I/O failure remains exit 2' test "$rc" = 2
if grep '^FAIL' build/verdict-controls/io.log; then fail 'cmp I/O error emitted semantic assertion'; fi
checks=$((checks+1))
rc=0
(sh -c '. dos/tests/assertions.sh; check_suite=control; check_log logged build/verdict-controls/tool.log sh -c "exit 2"') > build/verdict-controls/log-error.log 2>&1 || rc=$?
check 'captured tool error remains exit 2' test "$rc" = 2
if grep '^FAIL' build/verdict-controls/log-error.log; then fail 'captured tool error emitted semantic assertion'; fi
checks=$((checks+1))
complete

# Exercise the real assertion helper: an I/O/tool failure is never a kill.
set -eu
. kernel/tests/assertions.sh
mkdir -p build/verdict
run_case() {
 expected=$1; semantic=$2; script=$3
 rc=0
 sh -c ". kernel/tests/assertions.sh; $script" > build/verdict/log 2>&1 || rc=$?
 assert test "$rc" -eq "$expected"
 verdict=0
 awk -f factory/testlib/host-verdict.awk build/verdict/log > build/verdict/assertions || verdict=$?
 if [ "$semantic" = yes ]; then
  assert test "$verdict" -eq 0
  assert test -s build/verdict/assertions
 else
  assert test "$verdict" -ne 0
 fi
}
run_case 0 no 'assert test 1 -eq 1; finish'
run_case 1 yes 'assert test 1 -eq 2'
run_case 0 no 'assert_not grep -q x /dev/null; finish'
run_case 1 yes 'assert_not test 1 -eq 1'
run_case 2 no 'assert sh -c "exit 2"'
run_case 2 no 'assert_not sh -c "exit 2"'
run_case 2 no 'assert cmp build/verdict/absent /dev/null'
run_case 2 no 'assert command-that-does-not-exist'
run_case 1 no 'exit 1'
finish

# Kill contract for POSIX shell tests (testlib/README.md). Source this file,
# call `ok` after each passed check, `fail "script: message"` at a failed
# assertion and `summary` at the end. Setup failures should exit 2, not 1.
initech_checks=0
ok() { initech_checks=$((initech_checks+1)); }
fail() {
    echo "  FAIL $*"
    echo "INITECH_CHECKS $((initech_checks+1)) failures=1"
    exit 1
}
summary() { echo "INITECH_CHECKS $initech_checks failures=0"; }

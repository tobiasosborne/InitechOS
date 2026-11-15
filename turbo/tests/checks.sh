# Explicit assertion sites for Turbo factory shell tests (initech-kkqbw).
# Setup failures never emit a completed assertion summary. Every comparison
# names its independent obligation. Call finish only after all work completed.
checks=0
setup() {
    setup_rc=0
    "$@" || setup_rc=$?
    if [ "$setup_rc" -ne 0 ]; then
        echo "ERROR setup (exit $setup_rc): $*" >&2
        exit 2
    fi
}
assert() {
    assertion_name=$1; shift
    checks=$((checks+1)); assertion_rc=0
    "$@" || assertion_rc=$?
    if [ "$assertion_rc" -eq 0 ]; then :
    elif [ "$assertion_rc" -eq 1 ]; then
        echo "FAIL $assertion_name [check $checks]" >&2
        echo "$checks checks, 1 failures" >&2
        exit 1
    else
        echo "ERROR assertion reader (exit $assertion_rc): $assertion_name" >&2
        exit 2
    fi
}
assert_not() {
    assertion_name=$1; shift
    checks=$((checks+1)); assertion_rc=0
    "$@" || assertion_rc=$?
    if [ "$assertion_rc" -eq 1 ]; then :
    elif [ "$assertion_rc" -eq 0 ]; then
        echo "FAIL $assertion_name [check $checks]" >&2
        echo "$checks checks, 1 failures" >&2
        exit 1
    else
        echo "ERROR assertion reader (exit $assertion_rc): $assertion_name" >&2
        exit 2
    fi
}
finish() {
    [ "$checks" -gt 0 ] || { echo 'ERROR empty assertion run' >&2; exit 2; }
    echo "$checks checks, 0 failures"
}

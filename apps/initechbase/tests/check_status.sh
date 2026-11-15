# Sourced after factory/testlib/shell-checks.sh (initech-zw6sl).
# Only a completed comparison that reports a difference (exit status 1) is a
# semantic failure. A crash (status 128+signal), a missing tool (126/127) or
# a setup refusal (2) is a test error: the suite exits 2 without a FAIL line,
# so it can never count as a mutant kill (factory/testlib/README.md).
setup_error() {
    echo "ERROR $*" >&2
    exit 2
}
# check LABEL COMMAND...: ok on 0, fail LABEL on 1, setup_error otherwise.
check() {
    check_label=$1; shift
    check_rc=0; "$@" || check_rc=$?
    case $check_rc in
    0) ok;;
    1) fail "$check_label";;
    *) setup_error "$check_label: exit $check_rc (crash, missing tool or setup failure)";;
    esac
}
# need FILE...: every named input exists before anything compares it.
need() {
    for need_file in "$@"; do
        [ -f "$need_file" ] || setup_error "missing input $need_file"
    done
}
# need_manifest DIR MANIFEST: every file a checksum manifest names exists.
need_manifest() {
    need "$1/$2"
    while read -r need_hash need_name; do
        need "$1/${need_name#\*}"
    done < "$1/$2"
}
# The mapping itself is graded: a signalled helper must be an error.
status_self_test() {
    status_rc=0
    (check "status self-test" sh -c 'kill -SEGV $$') > /dev/null 2>&1 || status_rc=$?
    [ "$status_rc" = 2 ] || fail "status self-test: a crashed helper exited $status_rc, not 2"
    ok
    status_rc=0
    (check "status self-test" sh -c 'exit 1') > /dev/null 2>&1 || status_rc=$?
    [ "$status_rc" = 1 ] || fail "status self-test: a reported difference exited $status_rc, not 1"
    ok
}

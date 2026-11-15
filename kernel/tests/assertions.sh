# Explicit shell properties only. Tool/setup failures retain exit2; neither
# arbitrary helper failures nor a trap manufacture completed assertions.
checks=0
assert() {
 checks=$((checks+1))
 rc=0; "$@" || rc=$?
 case $rc in
  0) ;;
  1) printf 'FAIL: %s\nINITECH_CHECKS %s failures=1\n' "$*" "$checks" >&2; exit 1;;
  *) printf 'ERROR: %s (exit %s)\n' "$*" "$rc" >&2; exit 2;;
 esac
}
finish() { printf 'INITECH_CHECKS %s failures=0\n' "$checks"; }
assert_not() {
 checks=$((checks+1))
 rc=0; "$@" || rc=$?
 case $rc in
  1) ;;
  0) printf 'FAIL: unexpected match %s\nINITECH_CHECKS %s failures=1\n' "$*" "$checks" >&2; exit 1;;
  *) printf 'ERROR: %s (exit %s)\n' "$*" "$rc" >&2; exit 2;;
 esac
}

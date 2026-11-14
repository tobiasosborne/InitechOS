# Scenario protocol: 0 + PASS, 42 + explicit SEMANTIC assertion, 2 + ERROR.
# Only an independently completed, healthy emulator run permits assertions.
error() { echo "ERROR DOS scenario: $*" >&2; exit 2; }
transport_pass() {
    grep -Fx '[harness] OK=1' "$1" >/dev/null || error 'transport did not complete'
    grep -F 'timed_out=0' "$1" >/dev/null || error 'transport timed out'
    grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$1" >/dev/null || error 'CPU/startup fault'
    realpath "$1" > "$out/transport.ok"
}
semantic() {
    [ -s "$out/transport.ok" ] || error 'assertion before completed transport'
    printf 'SEMANTIC %s\n' "$*" > "$out/assertion"
    echo "ASSERTION-FAILED: $*" >&2
    exit 42
}
require() {
    message=$1;shift
    if "$@"; then return; else rc=$?;fi
    [ "$rc" -eq 1 ] || error "assertion command failed ($rc): $message"
    semantic "$message"
}
forbid() {
    message=$1;shift
    if "$@"; then semantic "$message"; else rc=$?;fi
    [ "$rc" -eq 1 ] || error "assertion command failed ($rc): $message"
}
finish_scenario() {
    rc=$1
    case $rc in
      0) [ -s "$out/transport.ok" ] || { echo 'ERROR no completed transport' > "$out/result";exit 2; }
         printf 'PASS\n' > "$out/result";;
      42) [ -s "$out/assertion" ] || { echo 'ERROR missing assertion' > "$out/result";exit 2; }
          cp "$out/assertion" "$out/result";;
      *) printf 'ERROR exit=%s\n' "$rc" > "$out/result";exit 2;;
    esac
}
trap 'finish_scenario "$?"' 0

set -eu
check_suite=runtime-verdict
. dos/tests/assertions.sh
mkdir -p build/protocol
out=$PWD/build/protocol
verdict_case() {
 sh dos/tests/runtime/classify.sh "$out" "$1" > "$out/actual"
 printf '%s\n' "$2" > "$out/expected"
 check "exit $1 expected $2" cmp "$out/actual" "$out/expected"
}
verdict_case 42 'ERROR exit=42'
printf '[harness] OK=1\n[harness] timed_out=0\n[harness] triple_fault=0 cpu_reset=2 guest_errors=0\n' > "$out/report"
printf '%s\n' "$out/report" > "$out/transport.ok"
printf 'SEMANTIC independent assertion\n' > "$out/assertion"
cp "$out/assertion" "$out/result"
verdict_case 42 'SEMANTIC independent assertion'
verdict_case 124 'ERROR exit=124'
verdict_case 127 'ERROR exit=127'
verdict_case 1 'ERROR exit=1'
printf 'PASS\n' > "$out/result"
verdict_case 0 PASS
printf '[harness] OK=0\n[harness] timed_out=1\n' > "$out/report"
verdict_case 0 'ERROR exit=0'
printf '[bochs] launched=1 rfb_unblocked=1 timed_out=0 exit=0 signal=0\n[bochs] triple_fault=0\n[bochs] OK=1\n' > "$out/report"
verdict_case 0 PASS
cp "$out/assertion" "$out/result"
verdict_case 42 'SEMANTIC independent assertion'
printf '[bochs] launched=1 rfb_unblocked=1 timed_out=1 exit=0 signal=0\n[bochs] triple_fault=0\n[bochs] OK=1\n' > "$out/report"
verdict_case 42 'ERROR exit=42'
echo 'GREEN runtime verdict: explicit completed assertions only; timeout/startup/setup failures are errors'
complete

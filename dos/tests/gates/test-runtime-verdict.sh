set -eu
mkdir -p build/protocol
out=$PWD/build/protocol
check() {
 sh dos/tests/runtime/classify.sh "$out" "$1" > "$out/actual"
 printf '%s\n' "$2" > "$out/expected"
 cmp "$out/actual" "$out/expected"
}
check 42 'ERROR exit=42'
printf '[harness] OK=1\n[harness] timed_out=0\n[harness] triple_fault=0 cpu_reset=2 guest_errors=0\n' > "$out/report"
printf '%s\n' "$out/report" > "$out/transport.ok"
printf 'SEMANTIC independent assertion\n' > "$out/assertion"
cp "$out/assertion" "$out/result"
check 42 'SEMANTIC independent assertion'
check 124 'ERROR exit=124'
check 127 'ERROR exit=127'
check 1 'ERROR exit=1'
printf 'PASS\n' > "$out/result"
check 0 PASS
printf '[harness] OK=0\n[harness] timed_out=1\n' > "$out/report"
check 0 'ERROR exit=0'
echo 'GREEN runtime verdict: explicit completed assertions only; timeout/startup/setup failures are errors'

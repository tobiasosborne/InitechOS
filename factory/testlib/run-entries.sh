#!/bin/sh
# Shell/Pascal companion to mk/host-test.mk's status protocol.
set -eu
component=$1 out=$2 runner=$3
shift 3
mkdir -p "$out/logs"
start=$(date +%s)
n=0 failed=0
for entry do
    n=$((n+1))
    if timeout -k 1 "${TEST_TIMEOUT_S:-20}" sh "$runner" "$out" "$entry" > "$out/logs/$entry.log" 2>&1; then
        echo "  ok    $entry"
    else
        echo "  FAIL  $entry (log: $out/logs/$entry.log)"
        tail -n 15 "$out/logs/$entry.log"
        failed=$((failed+1))
    fi
done
elapsed=$(($(date +%s)-start))
echo "$component: $n tests, $failed failures (${elapsed}s; budget ${TEST_BUDGET_S:-30}s)"
if [ "$elapsed" -gt "${TEST_BUDGET_S:-30}" ]; then echo "FAIL budget: $component exceeded TEST_BUDGET_S"; failed=$((failed+1)); fi
test "$failed" -eq 0

#!/bin/sh
# Shell/Pascal companion to mk/host-test.mk's status protocol.
set -eu
component=$1 out=$2 runner=$3
shift 3
mkdir -p "$out/logs"
n=0 failed=0
for entry do
    n=$((n+1))
    if sh "$runner" "$out" "$entry" > "$out/logs/$entry.log" 2>&1; then
        echo "  ok    $entry"
    else
        echo "  FAIL  $entry (log: $out/logs/$entry.log)"
        tail -n 15 "$out/logs/$entry.log"
        failed=$((failed+1))
    fi
done
echo "$component: $n tests, $failed failures"
test "$failed" -eq 0

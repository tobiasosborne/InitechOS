#!/bin/sh
# Wrap even inherited multi-command runners; logs stay private to the program.
set -eu
name=$1 seconds=$2 pattern=$3 bin=$4
shift 4
rm -f "$bin.result" "$bin.log" "$bin.assertion"
start=$(date +%s)
rc=0
timeout -k 1 "$seconds" "$@" > "$bin.run.log" 2>&1 || rc=$?
# A dependency on the canonical header opts in even if no summary was emitted.
if [ -z "$pattern" ] && find "$bin.objs" -name '*.d' -exec grep -El '/factory/(testlib|include)/test_assert.h' {} + 2>/dev/null | grep -q .; then
    pattern='^INITECH_CHECKS ([0-9]+) failures=[0-9]+$'
fi
if [ "$rc" -eq 0 ] && [ -n "$pattern" ]; then
    count=$(sed -nE "s/$pattern/\1/p" "$bin.log" | tail -n 1)
    case $count in ''|*[!0-9]*) count=0;; esac
    if ! [ "$count" -gt 0 ] 2>/dev/null; then echo "FAIL count: $name reported no positive check count" >> "$bin.run.log"; rc=1; fi
fi
state=ERROR
if [ "$rc" -eq 0 ]; then
    state=PASS
elif [ "$rc" -eq 1 ] &&
     awk -f "$(dirname "$0")/host-verdict.awk" "$bin.log" > "$bin.assertion"; then
    state="SEMANTIC $(sha256sum "$bin.assertion" | cut -d " " -f1)"
fi
printf "%s\n" "$state" > "$bin.result"
elapsed=$(($(date +%s)-start))
if [ "$rc" -eq 0 ]; then
    echo "  ok    $name (${elapsed}s)"
else
    echo "  FAIL  $name (${elapsed}s, exit $rc; timeout ${seconds}s; log: $bin.log)"
    tail -n 12 "$bin.log" "$bin.run.log" 2>/dev/null || :
fi
exit "$rc"

#!/bin/sh
# Classify an entire make test-run. Unknown/partial evidence is never a kill.
set -eu
build=$1 log=$2 rc=$3
bad() { echo ERROR; exit 0; }
case $rc in 0|2) :;; *) bad;; esac
count=$(sed -n 's/^INITECH_TEST_RUN \([0-9][0-9]*\)$/\1/p' "$log")
case $count in ''|*[!0-9]*|0) bad;; esac
grep -q '^FAIL budget:' "$log" && bad
n=0 semantic=0
for result in "$build"/host/*.result; do
    [ -f "$result" ] || bad
    n=$((n+1))
    state=$(cat "$result")
    case $state in
      PASS) :;;
      SEMANTIC\ *)
        test -s "${result%.result}.assertion" || bad
        test "$state" = "SEMANTIC $(sha256sum "${result%.result}.assertion" | cut -d ' ' -f1)" || bad
        semantic=$((semantic+1));;
      *) bad;;
    esac
done
[ "$n" -eq "$count" ] || bad
if [ "$rc" -eq 0 ]; then
    [ "$semantic" -eq 0 ] || bad
    echo PASS
else
    [ "$semantic" -gt 0 ] || bad
    for result in "$build"/host/*.result; do
        state=$(cat "$result")
        case $state in SEMANTIC\ *) printf '%s %s\n' "$(basename "$result")" "$state";; esac
    done
fi

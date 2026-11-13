#!/bin/sh
set -eu
runner=$1
root=$(mktemp -d build/scratch-lock-test.XXXXXX)
trap 'rm -rf "$root"' EXIT
for mode in open fcntl; do
 rc=0; "$runner" "$mode" "$root/new/parent/lock" > "$root/log" 2>&1 || rc=$?
 [ "$rc" = 2 ] && grep -q '^RED scratch lock ' "$root/log" || { cat "$root/log"; exit 1; }
 if grep -q 'critical section' "$root/log"; then echo 'RED lock failed open'; exit 1; fi
done
"$runner" eintr "$root/another/parent/lock" > "$root/log"
grep -qx 'critical section entered' "$root/log"
echo 'GREEN test-samir-scratch-lock: open/ENOLCK fail closed, EINTR retries, parents created'

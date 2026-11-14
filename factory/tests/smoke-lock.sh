#!/bin/sh
# Independent critical-section invariant: two cooperating shells cannot
# create the same directory at once. Test never touches the host smoke lock.
set -eu
cd "$(dirname "$0")/../.."
root=$PWD
work=$(mktemp -d "$root/build/factory/smoke-lock.XXXXXX")
trap 'rm -f "$work/lock" "$work/entered" "$work/first.log" "$work/second.log"; rmdir "$work"' EXIT HUP INT TERM
cat > "$work/worker" <<'EOF'
set -eu
. "$1/factory/testlib/smoke-lock.sh"
smoke_lock "$2/lock"
# Child reuses the inherited description rather than deadlocking itself.
sh -c '. "$1/factory/testlib/smoke-lock.sh"; smoke_lock "$2/lock"' sh "$1" "$2"
mkdir "$2/critical" || { echo 'FAIL smoke isolation: critical sections overlap'; exit 1; }
: > "$2/entered"
sleep 1
rmdir "$2/critical"
EOF
sh "$work/worker" "$root" "$work" > "$work/first.log" 2>&1 &
first=$!
while [ ! -f "$work/entered" ]; do
 kill -0 "$first" 2>/dev/null || { cat "$work/first.log"; exit 1; }
 sleep 0.01
done
sh "$work/worker" "$root" "$work" > "$work/second.log" 2>&1 &
second=$!
rc=0
wait "$first" || rc=1
wait "$second" || rc=1
cat "$work/first.log" "$work/second.log"
rm "$work/worker"
[ "$rc" -eq 0 ] || exit "$rc"
grep -F 'waiting for host lock' "$work/second.log" >/dev/null || {
 echo 'FAIL smoke isolation: second run did not wait'; exit 1;
}
echo 'ok smoke isolation: two runs exclude each other and nested drivers reuse the lock'

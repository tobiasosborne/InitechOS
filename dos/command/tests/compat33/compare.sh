#!/bin/sh
# Independent captured expectations, deliberately red until product fixes land.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
# Scratch is external to the frozen fixtures.
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT HUP INT TERM
red=0
check() {
 if cmp -s "$1" "$2"; then echo "GREEN $3"; else echo "RED $3"; red=$((red+1)); fi
}
select_case() {
 tr -d '\r' < "$1" | sed -n "$2" > "$3"
}
for control in LIMIT TCASE; do
 check "$src/expected/$control/files/TRACE.TXT" "$src/observed/$control/files/TRACE.TXT" "$control captured limit control"
done
for control in R.OUT 'PIPE1.$$$'; do
 check "$src/expected/REDIR/files/$control" "$src/observed/REDIR/files/$control" "$control redirected/append/preserved bytes"
done
for control in IN.OUT PIPE.OUT; do
 check "$src/expected/STREAM/files/$control" "$src/observed/STREAM/files/$control" "$control input redirect/pipe bytes"
done
for pattern in '/^SHIFT=/p' '/^MISSING=/p' '/^TOKEN=/p' '/^WILDCARD-EXISTS/p' '/^PARAMS=/p' '/^PERCENT=/p'; do
 select_case "$src/expected/BATCH/files/TRACE.TXT" "$pattern" "$out/expected"
 select_case "$src/observed/BATCH/files/TRACE.TXT" "$pattern" "$out/actual"
 check "$out/expected" "$out/actual" "BATCH $pattern"
done
check "$src/expected/DEVICES/files/TRACE.TXT" "$src/observed/DEVICES/files/TRACE.TXT" 'device path validation'
for mode in expected observed; do
 awk '$1=="DEL" && $2=="TXT" {print $1,$2,$3}' "$src/$mode/SWITCH/after.mdir" > "$out/$mode"
done
check "$out/expected" "$out/observed" 'invalid DEL switch preserves file'
check "$src/expected/U08/files/UTIL.OUT" "$src/observed/U08/files/UTIL.OUT" 'SYS invalid switch control'
echo "$red captured incompatibilities in selected comparisons"
[ "$red" = 0 ]

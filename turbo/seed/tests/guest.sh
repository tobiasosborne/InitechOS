#!/bin/sh
# Validate factory execution evidence before any semantic comparison. This
# wrapper also redoes the comparisons explicitly, because the inherited
# factory script can stop at a diff or lose a missing-serial pipeline error.
set -eu
root=$1 out=$2 name=$3
. "$root/turbo/tests/checks.sh"
work=$out/runs/guest-$name
setup rm -rf "$work"
setup mkdir -p "$work"
rc=0
sh "$root/factory/seed-guest.sh" "$root" "$out" "$name" > "$work/factory.log" 2>&1 || rc=$?
cat "$work/factory.log"
setup test -f "$work/expected"
setup test -s "$work/run.serial"
setup grep -Fq 'launched=1 timed_out=0 exit=0 signal=0' "$work/report"
setup grep -Fq 'triple_fault=0 cpu_reset=2 guest_errors=0' "$work/report"
setup grep -Fq 'quit_after_found=1' "$work/report"
setup tr -d '\r' < "$work/run.serial" > "$work/serial.txt"
setup test "$(grep -c '^seed-output-start$' "$work/serial.txt")" -eq 1
setup test "$(grep -c '^seed-output-end$' "$work/serial.txt")" -eq 1
setup sed -n '/^seed-output-start$/,/^seed-output-end$/p' "$work/serial.txt" > "$work/bracket"
setup sed '1d;$d' "$work/bracket" > "$work/actual"
assert "guest-$name:FPC stdout" diff -u "$work/expected" "$work/actual"
# File absence is judged from a successful independent directory listing;
# mtools startup/read errors remain infrastructure errors.
setup mdir -b -i "$work/test.img" :: > "$work/list"
for file in "$work"/*.BIN; do
    test -f "$file" || continue
    leaf=$(basename "$file")
    assert "guest-$name:$leaf exists" grep -iEq "(^|/)$leaf\$" "$work/list"
    setup mcopy -o -i "$work/test.img" ::"$leaf" "$file.guest"
    assert "guest-$name:$leaf FPC bytes" cmp "$file" "$file.guest"
done
# A valid comparison must not launder an unexplained producer failure.
setup test "$rc" -eq 0
finish

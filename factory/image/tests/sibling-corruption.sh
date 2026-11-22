#!/bin/sh
# Both copies of HELLO.TXT can agree even when both disagree with the manifest.
set -eu
cd "$(dirname "$0")/../../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
cp build/image/c.img build/image/data.img build/image/a-data.img "$work/"
printf 'incorrect shared payload\r\n' > "$work/wrong"
for volume in "$work/data.img" "$work/c.img@@32256"; do
 mcopy -o -i "$volume" "$work/wrong" ::HELLO.TXT
done
rc=0
sh factory/image/c-disk-check.sh "$work" > "$work/verdict" || rc=$?
[ "$rc" -eq 1 ] && grep -Fq 'FAIL c-disk: C:HELLO.TXT differs from build/system/installed/HELLO.TXT' "$work/verdict" ||
 { cat "$work/verdict"; echo 'Expected manifest payload rejection' >&2; exit 1; }
echo 'ok C: independent source rejects matching corrupt siblings'

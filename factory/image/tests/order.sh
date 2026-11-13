#!/bin/sh
# Constitution: Reproducible builds. Directory insertion order is observable
# by the Finder, and must not depend on host readdir order inside mcopy -s.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
export TZ=UTC LC_ALL=C SOURCE_DATE_EPOCH=315532800 MTOOLS_SKIP_CHECK=1
for pass in a b; do
 mkdir -p "$work/$pass/APPS"
 if [ "$pass" = a ]; then names='123.EXE 123.MNU WORD.EXE'; else names='WORD.EXE 123.MNU 123.EXE'; fi
 for name in $names; do printf '%s\n' "$name" > "$work/$pass/APPS/$name"; done
 find "$work/$pass" -exec touch -d '@315532800' {} +
 dd if=/dev/zero of="$work/$pass.img" bs=512 count=2880 status=none
 mformat -i "$work/$pass.img" -f 1440 -N 0x494e4933 -v INITECH ::
 sh "$root/factory/image/copy-tree.sh" "$work/$pass" "$work/$pass.img"
 mdir -b -i "$work/$pass.img" ::APPS/ > "$work/$pass.order"
 printf '::/APPS/123.EXE\n::/APPS/123.MNU\n::/APPS/WORD.EXE\n' > "$work/expected"
 diff -u "$work/expected" "$work/$pass.order"
done
cmp "$work/a.img" "$work/b.img"
echo 'image order: lexical FAT entries and identical bytes across opposite host insertion orders'

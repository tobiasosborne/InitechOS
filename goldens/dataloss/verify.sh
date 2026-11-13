#!/bin/sh
# Verify retained reference bytes, disk readback, and independent reader output.
set -eu
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
cd "$here"
sha256sum -c SHA256SUMS > /dev/null
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT HUP INT TERM
for d in dos/* wordperfect/*; do
 [ -f "$d/data.img.gz" ] || continue
 gzip -dc "$d/data.img.gz" > "$tmp/disk.img"
 case "$d" in wordperfect/*) mdir -a -i "$tmp/disk.img" :: > "$tmp/dir";; *) mdir -i "$tmp/disk.img" :: > "$tmp/dir";; esac
 cmp "$d/after.mdir" "$tmp/dir"
 case "$d" in
  dos/*) if mdir -i "$tmp/disk.img" ::TARGET.BIN >/dev/null 2>&1; then echo "unexpected target: $d" >&2; exit 1; fi;;
  wordperfect/full) if mdir -i "$tmp/disk.img" ::NAME.WP5 >/dev/null 2>&1; then echo 'unexpected failed document' >&2; exit 1; fi;;
 esac
 case "$d" in wordperfect/*)
  for f in NAME.WP5 'NAME.BK!'; do
   if [ -f "$d/after.$f" ]; then mcopy -o -i "$tmp/disk.img" "::$f" "$tmp/extracted"; cmp "$d/after.$f" "$tmp/extracted"; fi
  done;;
 esac
done
cmp wordperfect/collision/before.NAME.WP5 'wordperfect/collision/after.NAME.BK!'
cmp wordperfect/full/before.NAME.WP5 'wordperfect/full/after.NAME.BK!'
cmp wordperfect/readonly/before.NAME.WP5 wordperfect/readonly/after.NAME.WP5
cmp 'wordperfect/readonly/before.NAME.BK!' 'wordperfect/readonly/after.NAME.BK!'
for d in allprotected firstprotected; do cmp "lotus/$d/BEFORE.WK1" "lotus/$d/AFTER.WK1"; done
for f in lotus/*/*.WK1 wordperfect/*/before.NAME* wordperfect/*/after.NAME*; do
 case "$f" in *.dump) continue;; esac
 case "$f" in *.WK1) reader=/home/tobias/Projects/lotus123-decomp/tools/wk1_ref.py;; *) reader=/home/tobias/Projects/wordperfect51-decomp/tools/wp5_ref.py;; esac
 python3 "$reader" "$f" > "$tmp/read"
 sed '1d' "$tmp/read" > "$tmp/actual"
 sed '1d' "${f%.WK1}.dump" > "$tmp/expected" 2>/dev/null || sed '1d' "$f.dump" > "$tmp/expected"
 cmp "$tmp/actual" "$tmp/expected"
done
echo 'Capture hashes, mtools readback and independent reader dumps: GREEN'

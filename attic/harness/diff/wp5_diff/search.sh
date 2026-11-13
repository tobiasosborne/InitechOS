#!/bin/sh
# Read-only FAT differential: identical images, MILTON --cat versus mtools
# mtype, then independent code-aware match oracle on the real WP bodies.
# Ref: ADR-0003 Appendix A (READ); WP Reference pp. 571-573; real MINT corpus.
set -eu
export SOURCE_DATE_EPOCH=315532800 TZ=UTC
root=$1
out=$2
dump=$3
oracle=$4
mkdir -p "$out"
image="$out/search.img"
ref="$out/search.ref.img"
dd if=/dev/zero of="$image" bs=512 count=2880 status=none
mformat -i "$image" -f 1440 -N 12345678 ::
count=0
for mint in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 24A 25 26 27 28 30 31 32; do
    path="$root/goldens/minted/MINT$mint.WP5"
    if [ ! -f "$path" ]; then path="$root/mint/work/WP51/MINT$mint"; fi
    test -f "$path" || { echo "missing real MINT$mint" >&2; exit 1; }
    mcopy -o -i "$image" "$path" "::M$mint.WP5"
done
cp -f "$image" "$ref"
for mint in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 24A 25 26 27 28 30 31 32; do
    "$dump" "$image" --cat "M$mint.WP5" > "$out/milton.wp5"
    mtype -i "$ref" "::M$mint.WP5" > "$out/mtools.wp5"
    cmp "$out/milton.wp5" "$out/mtools.wp5"
    "$oracle" "$out/milton.wp5" > "$out/M$mint.log"
    count=$((count+1))
done
cmp "$image" "$ref"
fsck.fat -n "$image"
printf '>>> test-iword-search FAT/corpus: %s real documents; MILTON == mtools; images identical; searches preserve bytes\n' "$count"

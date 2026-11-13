#!/bin/sh
# Real WP inputs must survive independent FAT transport byte for byte.
# InitechDOS's own FAT-reader differential belongs to the kernel importer.
# Ref: WP Reference pp. 571-573; real MINT corpus and the legacy search gate.
set -eu
export SOURCE_DATE_EPOCH=315532800 TZ=UTC
root=$1
out=$2
oracle=$3
mode=${4:-search}
mkdir -p "$out"
image="$out/search.img"
ref="$out/search.ref.img"
dd if=/dev/zero of="$image" bs=512 count=2880 status=none
mformat -i "$image" -f 1440 -N 12345678 ::
for mint in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 24A 25 26 27 28 30 31 32; do
    path="$root/goldens/minted/MINT$mint.WP5"
    if [ ! -f "$path" ]; then path="$root/mint/work/WP51/MINT$mint"; fi
    test -f "$path" || { echo "missing real MINT$mint" >&2; exit 1; }
    mcopy -o -i "$image" "$path" "::M$mint.WP5"
done
cp -f "$image" "$ref"
count=0
for mint in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 24A 25 26 27 28 30 31 32; do
    path="$root/goldens/minted/MINT$mint.WP5"
    if [ ! -f "$path" ]; then path="$root/mint/work/WP51/MINT$mint"; fi
    mtype -i "$ref" "::M$mint.WP5" > "$out/input.wp5"
    cmp "$path" "$out/input.wp5"
    if [ "$mode" = search ]; then
        "$oracle" "$out/input.wp5" > "$out/M$mint.log"
    else
        "$oracle" "$mode" "$out/input.wp5" > "$out/M$mint.log"
    fi
    count=$((count+1))
done
cmp "$image" "$ref"
fsck.fat -n "$image"
printf 'corpus (%s): %s real documents, original == mtools; images identical\n' "$mode" "$count"

#!/bin/sh
# Captured settings bytes survive independent FAT transport unchanged.
# Source: legacy setup gate; IBM DOS 3.30 6-137..6-140.
set -eu
export SOURCE_DATE_EPOCH=315532800 TZ=UTC
out=$1
dd if=/dev/zero of="$out/settings.img" bs=512 count=2880 status=none
mformat -i "$out/settings.img" -f 1440 -N 12345678 ::
mcopy -o -i "$out/settings.img" "$out/IWORD.CAP" ::IWORD.SET
cp -f "$out/settings.img" "$out/settings.ref.img"
mtype -i "$out/settings.ref.img" ::IWORD.SET > "$out/mtools.set"
cmp "$out/IWORD.CAP" "$out/mtools.set"
cmp "$out/settings.img" "$out/settings.ref.img"
fsck.fat -n "$out/settings.img"
printf 'setup FAT: captured settings == mtools; identical images\n'

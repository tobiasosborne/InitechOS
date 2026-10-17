#!/bin/sh
# ADR-0003 Appendix A READ; IBM DOS 3.30 6-137..6-140. Captured tenant
# settings bytes, identical FAT images, independent MILTON and mtools reads.
set -eu
export SOURCE_DATE_EPOCH=315532800 TZ=UTC
out=$1
dump=$2
dd if=/dev/zero of="$out/settings.img" bs=512 count=2880 status=none
mformat -i "$out/settings.img" -f 1440 -N 12345678 ::
mcopy -o -i "$out/settings.img" "$out/IWORD.CAP" ::IWORD.SET
cp -f "$out/settings.img" "$out/settings.ref.img"
"$dump" "$out/settings.img" --cat IWORD.SET > "$out/milton.set"
mtype -i "$out/settings.ref.img" ::IWORD.SET > "$out/mtools.set"
cmp "$out/IWORD.CAP" "$out/milton.set"
cmp "$out/milton.set" "$out/mtools.set"
cmp "$out/settings.img" "$out/settings.ref.img"
fsck.fat -n "$out/settings.img"
printf '>>> test-iword-setup FAT: captured settings == MILTON == mtools; identical images\n'

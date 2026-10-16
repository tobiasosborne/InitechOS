#!/bin/sh
# Deterministic source image, cloned before every differential. Factory only.
set -eu
export SOURCE_DATE_EPOCH=315532800 TZ=UTC
img=$1
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 -N 0x12345678 -v ORIGINAL ::
printf 'unchanged bytes\r\n' > "${img}.txt"
mmd -i "$img" ::SUB ::SUB/DEEP ::OTHER
for p in ONE.TXT TWO.TXT OTHER.BIN SUB/ONE.TXT SUB/DEEP/ONE.TXT OTHER/ONE.TXT; do
    mcopy -i "$img" "${img}.txt" "::$p"
done
mattrib -i "$img" +h +s ::TWO.TXT

#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
${HOSTCC:-cc} -std=c11 -O1 -ffunction-sections -fdata-sections -Wl,--gc-sections \
 -Iapps/initech123/include factory/tests/worksheet-write.c \
 apps/initech123/fs/wk1.c apps/initech123/core/sheet.c -o "$work/write"
"$work/write" "$work/SLICE.WK1"
dd if=/dev/zero of="$work/disk.img" bs=512 count=2880 2>/dev/null
mformat -i "$work/disk.img" -f 1440 ::
mcopy -i "$work/disk.img" "$work/SLICE.WK1" ::
sh apps/initech123/tests/smoke/verify.sh "$work/disk.img" "$work/result"
echo 'ok worksheet: literal A1=123 B1=1.5 C1=@SUM(A1..B1), cached=124.5'

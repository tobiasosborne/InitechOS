#!/bin/sh
# Raw ATA oracle vs independent mtools-created MBR and host readback.
set -eu
export INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
build=$1; boot=$2
work=$build/block-device
mkdir -p "$work"
red() { echo "RED block-device $*"; exit 1; }
sh harness/diff/fat_diff/partition.sh mint "$build" > "$work/mint.log"
cp -f "$build/kzfs/partitioned.img" "$work/scratch.img"
spec="$work/scratch.img@@32256"
printf '@ECHO OFF\r\nBDPROBE\r\n' > "$work/autoexec.bat"
mcopy -o -i "$spec" "$work/autoexec.bat" ::AUTOEXEC.BAT
mcopy -i "$spec" "$build/block_device_probe.bin" ::BDPROBE.COM
dd if="$work/scratch.img" of="$work/before.img" bs=512 status=none
"$build/qemu_harness" --disk "$boot" --disk2 "$work/scratch.img" --name raw --out "$work" \
 --timeout-ms 18000 --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE \
 > "$work/raw.stdout" 2> "$work/raw.report" || true
! grep -q 'triple_fault=1' "$work/raw.report" || red TRIPLE-FAULT
tr -d '\r' < "$work/raw.serial" > "$work/serial.txt"
grep -qx 'BLOCK-PARAMS-RESET-STATUS-OK' "$work/serial.txt" || red 'geometry/reset/error registers'
grep -qx 'BLOCK-MBR signature=55AA type=01' "$work/serial.txt" || red 'MBR read: signature/type or INT13 gate'
grep -qx 'BLOCK-WRITE-OK' "$work/serial.txt" || red 'write call failed'
dd if="$work/scratch.img" of="$work/marker.bin" bs=512 skip=32 count=1 status=none
printf 'INITECH INT13 DEVICE LBA 32\r\n' > "$work/expected.txt"
dd if="$work/marker.bin" of="$work/marker.txt" bs=1 count="$(wc -c < "$work/expected.txt")" status=none
cmp "$work/expected.txt" "$work/marker.txt" || red 'write device LBA 32 (volume-relative mutant)'
dd if="$work/scratch.img" of="$work/after-prefix" bs=512 count=32 status=none
dd if="$work/before.img" of="$work/before-prefix" bs=512 count=32 status=none
cmp "$work/before-prefix" "$work/after-prefix" || red 'MBR/prefix damaged'
dd if="$work/scratch.img" of="$work/after-tail" bs=512 skip=33 status=none
dd if="$work/before.img" of="$work/before-tail" bs=512 skip=33 status=none
cmp "$work/before-tail" "$work/after-tail" || red 'non-marker sector changed'
echo 'GREEN test-block-device-emu: QEMU INT13 MBR 55AA/type01, scratch LBA32 host readback, every other sector intact'

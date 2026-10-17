#!/bin/sh
# Injected shell trace + independent mtools readback, same crafted layout.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800
build=$1; boot=$2
work=$build/khygiene-emu${3:-}
mkdir -p "$work"
cp -f "$build/khygiene-grow/seed.img" "$work/data.img"
printf '@ECHO OFF\r\nKHYPROBE\r\n' > "$work/autoexec.bat"
mcopy -i "$work/data.img" "$build/hygiene_probe.bin" ::KHYPROBE.COM
mcopy -i "$work/data.img" "$work/autoexec.bat" ::AUTOEXEC.BAT
"$build/qemu_harness" --disk "$boot" --disk2 "$work/data.img" --name probe --out "$work" \
    --timeout-ms 18000 --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE \
    > "$work/stdout" 2> "$work/report" || true
! grep -q 'triple_fault=1' "$work/report"
tr -d '\r' < "$work/probe.serial" > "$work/serial.txt"
for marker in KHY-RENAME-OK KHY-GROW-OK SHELL-DONE; do
    grep -qx "$marker" "$work/serial.txt" || { echo "RED FAT hygiene emu: missing $marker"; exit 1; }
done
mdir -a -b -i "$work/data.img" ::GROW | sort > "$work/list"
[ "$(wc -l < "$work/list")" -eq 15 ]
mcopy -i "$work/data.img" ::NEW/CHILD/NOTE.TXT "$work/note"
cmp "$work/note" "$build/khygiene-grow/note.txt"
fsck.fat -n "$work/data.img" > "$work/fsck"
echo 'GREEN test-fat-hygiene-emu: shell EXEC directory rename + fragmented growth + mtools/fsck readback'

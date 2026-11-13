#!/bin/sh
# Independent literal + rendered console, not strings/image-presence testing.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
boot=$1; tag=${2:-green}; out=build/dos-not-ready-$tag
mkdir -p "$out"
red() { echo "RED dos-not-ready-emu: $*"; exit 1; }
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 102 ::
printf 'DOSMSG-FAULT\r\n' > "$out/fault"
printf '@echo off\r\ncls\r\ntype fault.txt\r\necho MSG-DONE\r\n' > "$out/autoexec"
mcopy -i "$out/data.img" "$out/fault" ::FAULT.TXT
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
cp -f "$out/data.img" "$out/reference.img"
build/qemu_harness --disk "$boot" --disk2 "$out/data.img" --out "$out" --name run \
 --keys 'f,e,x,i,t,ret' --keys-after 'Abort, Retry, Fail?' --quit-after SHELL-DONE \
 --screendump --screendump-after SHELL-DONE --timeout-ms 12000 > "$out/stdout" 2> "$out/report" || true
tr -d '\r\f' < "$out/run.serial" > "$out/text"
grep -qx 'Not ready reading drive A:' "$out/text" || red 'MSG-DOS-0006 missing'
grep -q 'OK=1' "$out/report" || red 'emulator failed'
build/console_text "${VGABIOS:-/usr/share/seabios/vgabios-stdvga.bin}" "$out/run.ppm" > "$out/screen"
grep -qx 'Not ready reading drive A:' "$out/screen" || red 'MSG-DOS-0006 missing from decoded console'
grep -q '^Abort, Retry, Fail?' "$out/screen" || red 'A/R/F missing from console'
grep -qx MSG-DONE "$out/screen" || red 'Fail did not return to batch'
cmp "$out/reference.img" "$out/data.img" || red 'failed read changed disk'
mcopy -i "$out/data.img" ::FAULT.TXT - > "$out/got"
cmp "$out/fault" "$out/got" || red 'mtools bytes changed'
fsck.fat -n "$out/data.img" > "$out/fsck" 2>&1 || red 'fsck damage'
echo 'GREEN test-dos-not-ready-emu: MSG-DOS-0006 decoded verbatim; injected Fail resumes; identical mtools image + fsck.fat'

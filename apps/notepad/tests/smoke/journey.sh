#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/notepad/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
cp build/image/data.img "$out/data.img"
note=57; scrap=73
if mdir -i "$out/data.img" ::APPS/CPANELS.EXE >/dev/null 2>&1; then note=73; scrap=89; fi
trace="WFLAIR-CLOCK 9:00 am,m-100:-77,m-100:-77,m-100:-77,l1,w200,m40:$note,l0,WFLAIR-DISPATCH app=NOTEPAD,ka,kl,kp,kh,ka,kctrl-a,kctrl-c,D$out/notepad.ppm,kctrl-w,WTENANT-EXIT,m-40:-$note,l1,w200,m40:$scrap,l0,WFLAIR-DISPATCH app=SCRAPBK,kctrl-v,D$out/scrapbook.ppm,kctrl-w"
build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect 'FLAIR-DISPATCH app=SCRAPBK' \
 --quit-after 'TENANT-EXIT rc=0 via=exit' --quit-after-count 2 --timeout-ms 25000 \
 --name qemu --out "$out" > "$out/stdout" 2> "$out/report"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
for pair in 'NOTEPAD NPAD' 'SCRAPBK SBK1'; do
 set -- $pair
 mcopy -o -i "$out/data.img" "::$1.DAT" "$out/$1.DAT"
 build/apps/notepad/host/readback "$out/$1.DAT" "$2" alpha
done
fsck.fat -n "$out/data.img" > "$out/fsck.txt"

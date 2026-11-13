#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/initechtext/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
entry='ka,kl,kp,kh,ka,kret,kb,kr,ka,kv,ko,kctrl-s,w600,kret,w500,kctrl-q'
for emu in qemu bochs; do
 cp build/image/data.img "$out/$emu-data.img"
 if [ "$emu" = qemu ]; then
  trace="WFLAIR-CLOCK 9:00 am,$open_disk,WFINDER-OPEN-VOLUME,$open_apps,WFINDER-OPEN-FOLDER,kt,ke,kx,kt,kctrl-o,WFLAIR-DISPATCH app=TEXT,ka,kl,kp,kh,ka,kret,kb,kr,ka,kv,ko,D$out/text.ppm,kctrl-s,w600,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,kbspc,km,ke,km,ko,kdot,kt,kx,kt,kret,w500,kctrl-q"
  set -- --mouse "$trace"
 else
  set -- --keys "$open_disk,w300,$open_apps,w300,kt,ke,kx,kt,kctrl-o,w500,$entry"
 fi
 build/factory/${emu}_harness --disk build/image/boot.img --disk2 "$out/$emu-data.img" \
  --keys-after FLAIR-LIVE-READY "$@" --expect 'TENANT-EXIT rc=0 via=exit' \
  --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 30000 --name "$emu" --out "$out" \
  > "$out/$emu.stdout" 2> "$out/$emu.report"
 grep -F 'FLAIR-DISPATCH app=TEXT' "$out/$emu.serial"
 grep -F 'triple_fault=0' "$out/$emu.report"
 name=MEMO.TXT; [ "$emu" != bochs ] || name=UNTITLED
 mcopy -o -i "$out/$emu-data.img" "::$name" "$out/$emu.txt"
 printf 'alpha\rbravo' > "$out/expected.txt"
 cmp "$out/expected.txt" "$out/$emu.txt"
 fsck.fat -n "$out/$emu-data.img" > "$out/$emu.fsck"
 echo "ok $emu: InitechText saved exact CR-delimited TEXT bytes"
done

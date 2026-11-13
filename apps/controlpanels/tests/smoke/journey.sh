#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/controlpanels/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
cp build/image/data.img "$out/data.img"
trace="WFLAIR-CLOCK 9:00 am,m-100:-77,m-100:-77,m-100:-77,l1,w200,m40:57,l0,WFLAIR-DISPATCH app=CPANELS,m100:67,m80:67,l1,l0,w200,m-34:12,l1,l0,w200,D$out/patterns.ppm,kret,w300,kctrl-q"
build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect 'TENANT-EXIT rc=0 via=exit' \
 --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 25000 --name qemu --out "$out" \
 > "$out/stdout" 2> "$out/report"
grep -F 'FLAIR-DISPATCH app=CPANELS' "$out/qemu.serial"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
mcopy -o -i "$out/data.img" ::CPANELS.DAT "$out/record.dat"
# record.json: CP01, v1, pattern3, normal mouse, beep/full, clock; additive sum253.
printf '\103\120\060\061\001\003\001\001\000\002\001\000\000\000\375\000' > "$out/expected.dat"
cmp "$out/expected.dat" "$out/record.dat"
fsck.fat -n "$out/data.img" > "$out/fsck.txt"
echo 'ok QEMU: Control Panels saved exact independently authored settings record'

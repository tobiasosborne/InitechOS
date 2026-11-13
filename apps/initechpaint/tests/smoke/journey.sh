#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/initechpaint/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
cp build/image/data.img "$out/data.img"
trace="WFLAIR-CLOCK 9:00 am,$open_disk,WFINDER-OPEN-VOLUME,$open_apps,WFINDER-OPEN-FOLDER,kp,ka,ki,kn,kt,kctrl-o,WFLAIR-DISPATCH app=PAINT,w300,D$out/canvas.ppm,kctrl-q"
build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect 'TENANT-EXIT rc=0 via=exit' \
 --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 25000 --name qemu --out "$out" \
 > "$out/stdout" 2> "$out/report"
grep -F 'FLAIR-DISPATCH app=PAINT' "$out/qemu.serial"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
build/apps/initechpaint/host/pixels canvas "$out/canvas.ppm"

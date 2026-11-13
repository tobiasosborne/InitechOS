#!/bin/sh
# Standalone QEMU journey; capture checked against the real Mac Calculator.
set -eu
cd "$(dirname "$0")/../../../.."
. factory/desktop-trace.sh
out=build/apps/calculator/smoke
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
cp build/image/data.img "$out/data.img"
trace="WFLAIR-CLOCK 9:00 am,m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:41,l0,WFLAIR-DISPATCH app=CALCULAT,w300,D$out/window.ppm,kctrl-q"
build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect 'TENANT-EXIT rc=0 via=exit' \
 --quit-after 'TENANT-EXIT rc=0 via=exit' --timeout-ms 20000 --name qemu --out "$out" \
 > "$out/stdout" 2> "$out/report"
grep -F 'FLAIR-DISPATCH app=CALCULAT' "$out/qemu.serial"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
build/apps/calculator/host/pixels window "$out/window.ppm" "${SYSTEM7_DECOMP:-/home/tobiasosborne/Projects/system7-decomp}/goldens/captures/s8_calc_idle.png"

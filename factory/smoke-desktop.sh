#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out=${DESKTOP_SMOKE_OUT:-build/smoke-desktop}
mkdir -p "$out"
cp build/image/data.img "$out/qemu-data.img"
printf '@ECHO OFF\r\nFLAIR\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/qemu-data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
trace="D$out/desktop.ppm,m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-30:-2,l1,m0:100,m0:33,l0,WFINDER-EXIT-DOS-ALERT,kret,WFLAIR-EXIT-TO-DOS,w300,D$out/dos.ppm,kf,kl,ka,ki,kr,kret"
build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/qemu-data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect FLAIR-EXIT-TO-DOS \
 --quit-after FLAIR-LIVE-READY --quit-after-count 2 --screendump \
 --screendump-after FLAIR-LIVE-READY --screendump-after-count 2 \
 --timeout-ms 30000 --name qemu --out "$out" > "$out/qemu.stdout" 2> "$out/qemu.report"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null
[ "$(grep -c 'FLAIR-LIVE-READY' "$out/qemu.serial")" -eq 2 ]
grep -F 'A:\>' "$out/qemu.serial" >/dev/null
echo 'ok QEMU: Finder desktop, mouse, Exit to InitechDOS, prompt and FLAIR again'
echo 'DESKTOP S2: 1 check, 0 failures (8 MiB)'

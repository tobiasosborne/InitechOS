#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out=${DESKTOP_SMOKE_OUT:-build/smoke-desktop}
mkdir -p "$out"
fail() { echo "FAIL desktop: $*" >&2; exit 1; }
. factory/desktop-trace.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
cp build/image/data.img "$out/qemu-data.img"
trace="D$out/desktop.ppm,$open_disk,WFINDER-OPEN-VOLUME,D$out/disk.ppm,$open_apps,WFINDER-OPEN-FOLDER,D$out/apps.ppm,$launch_123,WFLAIR-DISPATCH app=123,$enter_sum,D$out/sum.ppm,$save_123,w300,kctrl-q,WTENANT-EXIT,$exit_desktop,WFLAIR-EXIT-TO-DOS,w300,D$out/dos.ppm,kf,kl,ka,ki,kr,kret"
if ! build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/qemu-data.img" \
 --keys-after FLAIR-LIVE-READY --mouse "$trace" --expect FLAIR-EXIT-TO-DOS \
 --quit-after FLAIR-LIVE-READY --quit-after-count 2 --screendump \
 --screendump-after FLAIR-LIVE-READY --screendump-after-count 2 \
 --timeout-ms 45000 --name qemu --out "$out" > "$out/qemu.stdout" 2> "$out/qemu.report"; then
 cat "$out/qemu.report"; fail 'QEMU session';
fi
for marker in 'FINDER-OPEN-VOLUME' 'FINDER-OPEN-FOLDER name=APPS' 'FLAIR-DISPATCH app=123' 'TENANT-EXIT rc=0 via=exit' 'FLAIR-EXIT-TO-DOS' 'A:\>'; do
 grep -F "$marker" "$out/qemu.serial" >/dev/null || fail "QEMU missing $marker"
done
[ "$(grep -c 'FLAIR-LIVE-READY' "$out/qemu.serial")" -eq 2 ] || fail 'QEMU FLAIR restart'
echo 'ok QEMU: Finder disk/APPS, 123 entry/save/quit, DOS prompt and FLAIR again'
sh factory/readback-worksheet.sh "$out/qemu-data.img" "$out/qemu" || fail 'QEMU worksheet content'
echo 'ok QEMU mtools + independent WK1 reader: A1=123 B1=1.5 C1=@SUM(A1..B1), cached=124.5'
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/qemu.report" >/dev/null || fail 'QEMU fault verdict'
[ "$(grep -c 'CPU Reset' "$out/qemu.qemu.log")" -eq 2 ] || fail 'QEMU extra reset'
if grep -Ei 'triple fault|check_exception old: 0x8' "$out/qemu.qemu.log" >/dev/null; then fail 'QEMU fault log'; fi
echo 'ok QEMU: no CPU fault, triple fault, guest error or unexpected reset'
# S5 adds the equivalent Bochs session and legacy pixel comparison here.
echo 'DESKTOP S4: 3 checks, 0 failures (QEMU, 8 MiB)'

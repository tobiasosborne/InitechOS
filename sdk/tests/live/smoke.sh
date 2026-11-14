#!/bin/sh
# Boot the ordinary product; add only a test tenant and fixtures on a copy of
# its writable disk. The kernel is byte-identical to make image's product.
set -eu
cd "$(dirname "$0")/../../.."
out=${SDK_SMOKE_OUT:-build/sdk/smoke}
mkdir -p "$out"
cp build/image/data.img "$out/data.img"
mcopy -o -i "$out/data.img" "${SDK_TENANT:-build/sdk/live/SDKCHECK.EXE}" ::APPS/SDKCHECK.EXE
printf abcdefghij > "$out/input.dat"
mcopy -o -i "$out/data.img" "$out/input.dat" ::SDK.DAT
mmd -i "$out/data.img" ::SUB
. factory/desktop-trace.sh
emulator=qemu
markers=$out/markers
: > "$markers"
trace=$(
 inject "$open_disk"; await_marker 'FINDER-OPEN-VOLUME'
 inject "$open_apps"; await_marker 'FINDER-OPEN-FOLDER name=APPS'
 # finder_windows.c:fw_key selects by name and reveals an offscreen icon.
 inject 'kup,ks,kd,kk,kc,kh,ke,kc,kk,kdot,ke,kx,ke,kctrl-o'; await_marker 'FLAIR-DISPATCH app=SDKCHECK'
)
trace=${trace%,}
trace=${trace%,WFLAIR-DISPATCH app=SDKCHECK}
INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 build/factory/qemu_harness \
 --disk build/image/boot.img --disk2 "$out/data.img" --keys-after FLAIR-LIVE-READY \
 --mouse "$trace" --expect 'TENANT-EXIT rc=0 via=exit' --quit-after 'TENANT-EXIT rc=0 via=exit' \
 --timeout-ms 90000 --out "$out" --name gate ${SDK_GDB:+--gdb} > "$out/stdout" 2> "$out/report"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
mcopy -o -i "$out/data.img" ::SDKRES.BIN "$out/results.bin"
"${SDK_HOST:-build/sdk/tests/conformance}" "$out/results.bin"
cmp sdk/tests/live/gate-results.bin "$out/results.bin"

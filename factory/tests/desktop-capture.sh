#!/bin/sh
# A normal product boot; the independent capture mask is the visual verdict.
set -eu
cd "$(dirname "$0")/../.."
out=build/desktop-capture
mkdir -p "$out"
cp build/image/data.img "$out/data.img"
INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 build/factory/qemu_harness \
 --disk build/image/boot.img --disk2 "$out/data.img" --expect FLAIR-LIVE-READY \
 --quit-after 'FLAIR-CLOCK 9:00 am' --screendump --screendump-after 'FLAIR-CLOCK 9:00 am' \
 --timeout-ms 60000 --name desktop --out "$out" > "$out/stdout" 2> "$out/report"
sh factory/pixels-check.sh "$out" desktop

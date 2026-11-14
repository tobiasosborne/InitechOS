#!/bin/sh
set -eu
out=$2
 mcopy -o -i "$1" ::NOTEPAD.DAT "$out-notepad.dat"
 # Independent fixed-record reader: page 2 length=2 and TEXT=20, per the
 # authored NPAD store contract (16-byte header, 1026-byte records).
 dd if="$out-notepad.dat" of="$out-copy.bin" bs=1 skip=1042 count=4 2>/dev/null
 printf '\002\00020' > "$out-copy.expected"
 cmp "$out-copy.expected" "$out-copy.bin"
 make -s -C apps/calculator "$PWD/build/apps/calculator/host/test_calcda_route"
 build/apps/calculator/host/test_calcda_route "$out-notepad.dat"
# Capture is QEMU-only; both emulators must dispatch and exit in the runner.
case $out in *qemu-*)
 make -s -C apps/calculator "$PWD/build/apps/calculator/host/pixels"
 build/apps/calculator/host/pixels window "${out%qemu-*}calculator.ppm" "${SYSTEM7_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/system7-decomp}/goldens/captures/s8_calc_idle.png"
 ;; esac

#!/bin/sh
set -eu
out=$2
# Capture is QEMU-only; both emulators must dispatch and exit in the runner.
case $out in *qemu-*)
 make -s -C apps/calculator "$PWD/build/apps/calculator/host/pixels"
 build/apps/calculator/host/pixels window "${out%qemu-*}calculator.ppm" "${SYSTEM7_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/system7-decomp}/goldens/captures/s8_calc_idle.png"
 ;; esac

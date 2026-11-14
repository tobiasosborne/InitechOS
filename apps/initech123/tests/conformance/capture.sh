#!/bin/sh
# Generic independent reference-load capture; same recipe works for every L.
# runtime.sh OUT must have been run first. Does not modify the base runtime.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
runtime=${1:?runtime directory}; casefile=${2:?absolute fixture}; out=${3:?new output directory}
mkdir -p "$out"
cc -std=c11 -O2 "$top/goldens/dataloss/drive.c" -o "$out/drive"
cp "$runtime/runtime.img" "$out/runtime.img"
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 -N 19920302 ::
mcopy -i "$out/data.img" "$casefile" ::CASE.WK1
mcopy -o -i "$out/runtime.img@@32256" "$casefile" ::LOTUS/CASE.WK1
cp "$top/apps/initech123/tests/conformance/capture.steps" "$out/input.steps"
timeout -k 2 60 "$out/drive" "$out" "$runtime/lotus-boot.img" "$out/data.img" "$out/runtime.img" "$out/input.steps"
mcopy -i "$out/data.img" ::CASE.WK1 "$out/CASE.WK1"
cmp "$casefile" "$out/CASE.WK1"

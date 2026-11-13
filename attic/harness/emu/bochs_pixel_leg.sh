#!/bin/sh
# WL-0170 (initech-u9gf): the Bochs PIXEL leg. Boots the golden image on QEMU
# (an independent emulator; the QMP screendump is the golden) and the leg image
# on Bochs, both with the guest RTC pinned to the same instant and the same
# data volume bytes, captures each at the SAME complete serial line, then
# grades Bochs against QEMU with ppm_bochs_check: EXACT RGB on Bochs's own
# snapshot (indices + emulated DAC) and BGR233 on what the RFB display served.
# Ref: PRD Sec 8 / CLAUDE Rule 5; harness/emu/bochs.h (screendump_after);
# tools/ppm_bochs_check.c (the Bochs 2.7 sources the BGR233 rule is read from).
#
# usage: bochs_pixel_leg.sh BUILD TAG BOCHS_IMG GOLDEN_IMG DATA|- MARKER-LINE
#   GOLDEN_IMG is what QEMU boots: the leg image itself for a differential,
#   the CLEAN image for a mutant (a mutant golden would agree by construction).
# Prints "GREEN bochs-pixel TAG ..." (exit 0), "RED bochs-pixel TAG ..." on a
# pixel disagreement (exit 1), "BROKEN bochs-pixel TAG ..." when no gradable
# capture exists (exit 2: a boot/capture failure can never count as a RED).
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; tag=$2; image=$3; golden=$4; data=$5; marker=$6
work=$build/bochs-pixel
mkdir -p "$work"
broken() { echo "BROKEN bochs-pixel $tag: $*"; exit 2; }
T0_ISO=1993-03-01T09:00:00     # vesa8.sh / qemu rtc_base: UTC
T0_UNIX=730976400              # date -u -d 1993-03-01T09:00:00 +%s
qd=; bd=
if [ "$data" != - ]; then
    cp -f "$data" "$work/$tag-qemu-data.img"; qd="--disk2 $work/$tag-qemu-data.img"
    cp -f "$data" "$work/$tag-bochs-data.img"; bd="--disk2 $work/$tag-bochs-data.img"
fi
# shellcheck disable=SC2086
"$build/qemu_harness" --disk "$golden" $qd --name "$tag-qemu" --out "$work" \
    --timeout-ms 20000 --screendump --rtc-base "$T0_ISO" \
    --screendump-after "$marker" > "$work/$tag-qemu.stdout" 2> "$work/$tag-qemu.report" || true
grep -q 'triple_fault=0' "$work/$tag-qemu.report" || broken "QEMU golden boot faulted"
test -s "$work/$tag-qemu.ppm" || broken "no QEMU golden capture"
grep -qxF "$marker" "$work/$tag-qemu.serial" || broken "QEMU never printed the capture line"
# shellcheck disable=SC2086
"$build/bochs_harness" --disk "$image" $bd --name "$tag-bochs" --out "$work" \
    --timeout-ms 120000 --rtc-time0 "$T0_UNIX" \
    --screendump-after "$marker" 2> "$work/$tag-bochs.report" || true
grep -q 'triple_fault=0' "$work/$tag-bochs.report" || broken "Bochs boot faulted"
grep -q 'screendump_ok=1' "$work/$tag-bochs.report" || broken "no stable Bochs RFB frame"
grep -q 'snapshot_ok=1' "$work/$tag-bochs.report" || broken "no Bochs snapshot"
if grep -q '^PANIC' "$work/$tag-bochs.serial"; then broken "Bochs guest PANICked"; fi
set +e
"$build/ppm_bochs_check" "$work/$tag-qemu.ppm" "$work/$tag-bochs.snap.ppm" \
    "$work/$tag-bochs.rfb.pgm" > "$work/$tag.grade" 2>&1
rc=$?
set -e
cat "$work/$tag.grade"
case $rc in
 0) echo "GREEN bochs-pixel $tag: Bochs == QEMU at '$marker' (exact + displayed)" ;;
 1) echo "RED bochs-pixel $tag: Bochs != QEMU at '$marker'"; exit 1 ;;
 *) broken "checker could not read the captures" ;;
esac

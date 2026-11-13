#!/bin/sh
# initech-2gva; oracle written before the VBE/DAC changes. Reuse the SAME
# canonical-scene checker and locked app-switch trace as the existing gates.
# A capture/boot failure cannot count as a killed pixel mutant.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; kind=$2; image=$3; tag=$4; data=${5:-}; trace=${6:-}
work=$build/vesa8
mkdir -p "$work"
red() { echo "RED vesa8 $tag $*"; exit 1; }
name=vesa8-$tag
set -- --disk "$image" --name "$name" --out "$work" --timeout-ms 15000 \
    --screendump --rtc-base 1993-03-01T09:00:00
case $kind in
 scene) set -- "$@" --screendump-after FLAIR-DESKTOP ;;
 mode) set -- "$@" --screendump-after BI-OK ;;
 pre|post)
    cp -f "$data" "$work/$name-data.img"
    set -- "$@" --disk2 "$work/$name-data.img"
    if [ "$kind" = post ]; then
        set -- "$@" --mouse "$trace" --keys-after FLAIR-LIVE-READY \
            --screendump-after 'FLAIR-DISPATCH app=NOTES'
    else
        set -- "$@" --screendump-after FLAIR-TENANTS-READY
    fi ;;
 *) red kind ;;
esac
"$build/qemu_harness" "$@" > "$work/$name.stdout" 2> "$work/$name.report" || true
grep -q 'triple_fault=0' "$work/$name.report" || red triple-fault
grep -q 'guest_errors=0' "$work/$name.report" || red guest-errors
test -s "$work/$name.ppm" || red missing-capture
case $tag in
 *32*) grep -q '^VIDEO 640x480 bpp=32 ' "$work/$name.serial" || red expected-32bpp ;;
 *) grep -q '^VIDEO 640x480 bpp=8 ' "$work/$name.serial" || red expected-8bpp
    grep -qx 'VBE-MODE=0101' "$work/$name.serial" || red expected-mode-0101 ;;
esac
if [ "$kind" != mode ] && grep -Eq '^PANIC|^BI-BAD|^HALTED' "$work/$name.serial"; then
    red panic
fi
if [ "$kind" = scene ]; then
    "$build/ppm_flair_check" "$work/$name.ppm" || red scene-pixels
elif [ "$kind" = pre ] || [ "$kind" = post ]; then
    grep -qx FLAIR-FAT-MOUNT-OK "$work/$name.serial" || red mount
    if [ "$kind" = post ]; then
        grep -qx 'FLAIR-DISPATCH app=NOTES' "$work/$name.serial" || red dispatch
    fi
fi
echo "GREEN vesa8 $tag: real $kind desktop captured, mode verified, no triple fault"

#!/bin/sh
# flair_capture.sh -- capture FLAIR screen figures of the Initech 123 books
# from the running system (bead initech-ipu5.9; PUB-REF-0001 NT-20..23,
# GC-04; the pixel route of NT-21, the FLAIR counterpart of capture.sh).
#
# Every figure is a QEMU screendump of InitechOS: the boot image of the
# FLAIR desktop with its data volume as the IDE primary slave, driven by the
# qemu_harness --mouse trace that launches 123.EXE from the Finder exactly
# as spec/flair_i123_traces.mk does (waypoints A, B, C123, double-click),
# then types the figure's keystrokes, then parks the pointer on the menu
# bar (m0:-100 from the 123.EXE icon: x 211, y 22) so that no pointer is
# drawn over the window.  The dump is taken after FLAIR-LIVE-OK (the image
# halts; a stable frame, Rule 11) and cut, greyed and replicated by
# harness/pubs/flair_shot (NT-21/NT-22).
#
# Usage: flair_capture.sh <qemu_harness> <boot short> <boot long> <data.img>
#                         <i123_face> <flair_shot> <FLAIR list> <out dir>
#                         <clut.json> <color_canon.json>
# FLAIR list, one figure per line ('#' comments):
#   name|x0 y0 x1 y1|scale|life
# The keystrokes are <list dir>/name.k123 (i123_face key-script syntax; the
# ONLY keyboard source).  life is short (the 500-tick gate image) or long
# (the publications image, 6000 ticks) -- the image must stay alive until
# every key is in.  A capture is refused unless the tenant received exactly
# one keyDown per typed key (TENANT-EVT what=3), the pointer was parked
# (FLAIR-CURSOR x=211 y=22) and the image ended normally (FLAIR-LIVE-OK, no
# PANIC, no triple fault).  Thin shell (Law 3).
set -eu
runner=$1; short=$2; long=$3; data=$4; face=$5; shot=$6; list=$7; out=$8; clut=$9; canon=${10}
dir=$(dirname "$list")
mkdir -p "$out"
launch='m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0'
n=0
while IFS='|' read -r name rect scale life; do
    case $name in ''|'#'*) continue ;; esac
    keys=$dir/$name.k123
    [ -f "$keys" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s missing\n' "$keys"; exit 1; }
    case $life in short) img=$short ;; long) img=$long ;; *)
        printf '!!! test-pubs RED SCREEN-KEYS: %s: life "%s" is not short or long\n' "$name" "$life"; exit 1 ;; esac
    tokens=$("$face" tokens "$keys")
    want=$(printf '%s' "$tokens" | tr ',' '\n' | grep -c '^k' || true)
    spec="$launch,$tokens,m0:-100"
    [ ${#spec} -lt 4095 ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s: %s bytes of trace (harness max 4095)\n' "$name" "${#spec}"; exit 1; }
    cp -f "$data" "$out/$name.data.img"
    rm -f "$out/$name.ppm" "$out/$name.serial"
    "$runner" --disk "$img" --disk2 "$out/$name.data.img" --name "$name" --out "$out" \
        --mouse "$spec" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK \
        --timeout-ms 120000 --screendump --screendump-after FLAIR-LIVE-OK \
        < /dev/null 2> "$out/$name.report" || true
    if grep -q 'triple_fault=1' "$out/$name.report"; then printf '!!! test-pubs RED SCREEN-CAPTURE: %s: triple fault\n' "$name"; exit 1; fi
    s=$out/$name.serial
    [ -s "$out/$name.ppm" ] && grep -qx 'FLAIR-LIVE-OK' "$s" && ! grep -q '^PANIC' "$s" || {
        cat "$out/$name.report"; printf '!!! test-pubs RED SCREEN-CAPTURE: %s: no settled screendump\n' "$name"; exit 1; }
    got=$(grep -c '^TENANT-EVT what=3$' "$s" || true)
    [ "$got" = "$want" ] || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: 123 received %s of %s keystrokes (the image life ran out, or a key was lost)\n' "$name" "$got" "$want"; exit 1; }
    grep -q '^FLAIR-CURSOR x=211 y=22$' "$s" || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: the pointer was not parked on the menu bar\n' "$name"; exit 1; }
    ! grep -q 'FLAIR-TRACK-EXPIRED' "$s" || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: a gesture outlived the image\n' "$name"; exit 1; }
    # shellcheck disable=SC2086
    "$shot" png "$out/$name.ppm" $rect "$scale" "$out/$name.png" "$clut" "$canon"
    rm -f "$out/$name.data.img" "$out/$name.qemu.log"
    n=$((n + 1))
    printf '>>> flair_capture: %s (%s keystrokes, crop %s, scale %s)\n' "$name" "$want" "$rect" "$scale"
done < "$list"
[ "$n" -gt 0 ] || { printf '!!! flair_capture: %s defines no figures\n' "$list"; exit 1; }
printf '>>> flair_capture: %d FLAIR figures captured from the running system\n' "$n"

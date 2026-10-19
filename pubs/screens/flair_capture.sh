#!/bin/sh
# flair_capture.sh -- capture FLAIR screen figures of the Initech 123 books
# from the running system (bead initech-ipu5.9; PUB-REF-0001 NT-20..23,
# GC-04; the pixel route of NT-21, the FLAIR counterpart of capture.sh).
#
# Every figure is a QEMU screendump of InitechOS: the boot image of the
# FLAIR desktop with its data volume as the IDE primary slave, driven by the
# qemu_harness --mouse trace that launches 123.EXE from the Finder exactly
# as spec/flair_i123_traces.mk does (waypoints A, B, C123, double-click),
# then types the figure's keystrokes, parking the pointer on the menu bar
# before the last one (m0:-100 from the 123.EXE icon: x 211, y 22) so that
# no pointer is drawn over the window.  The dump is taken after FLAIR-LIVE-OK (the image
# halts; a stable frame, Rule 11) and cut, greyed and replicated by
# harness/pubs/flair_shot (NT-21/NT-22) into <out>/<name>.png, and written
# in its committed ASCII form <out>/<name>.b64 (pubs/ is ASCII-only, GC-05).
#
# Usage: flair_capture.sh <qemu_harness> <boot short> <boot long> <boot longer> <disk dir>
#                         <i123_face> <flair_shot> <FLAIR list> <out dir>
#                         <clut.json> <color_canon.json>
# FLAIR list, one figure per line ('#' comments):
#   name|x0 y0 x1 y1|scale|life[|marker[|stage]]
# The keystrokes are <list dir>/name.k123 (i123_face key-script syntax; the
# ONLY keyboard source).  life is short (the 500-tick gate image), long or
# longer (the publications images, 2000 and 12000 ticks) -- the image must
# stay alive until
# every key is in.  marker (default FLAIR-LIVE-OK, the end of the image's
# life) is the serial line after which the dump is taken: a modal alert is
# dumped at "TENANT-ALERT up", while it is on the screen.  stage (default:
# the last) picks the data volume <disk dir>/s<stage>.img: the shipped
# volume plus the sample worksheets of stages 1..stage (pubs/pubs.mk).  A capture is refused unless the tenant received exactly
# one keyDown per typed key (TENANT-EVT what=3), the pointer was parked
# (FLAIR-CURSOR x=211 y=22) and the image ended normally (FLAIR-LIVE-OK, no
# PANIC, no triple fault).  Thin shell (Law 3).
set -eu
runner=$1; short=$2; long=$3; longer=$4; disks=$5; face=$6; shot=$7; list=$8; out=$9; clut=${10}; canon=${11}
dir=$(dirname "$list")
mkdir -p "$out"
launch='m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0'
n=0
while IFS='|' read -r name rect scale life marker stage; do
    case $name in ''|'#'*) continue ;; esac
    keys=$dir/$name.k123
    [ -f "$keys" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s missing\n' "$keys"; exit 1; }
    case $life in short) img=$short ;; long) img=$long ;; longer) img=$longer ;; *)
        printf '!!! test-pubs RED SCREEN-KEYS: %s: life "%s" is not short, long or longer\n' "$name" "$life"; exit 1 ;; esac
    # A 50 ms hold after every keystroke: the PS/2 controller holds one
    # scancode, and a host under load can otherwise overrun it (a lost key
    # is caught below, but a figure must not depend on host load).
    tokens=$("$face" tokens "$keys" | sed 's/,k/,w50,k/g')
    # A script whose first line is "#!select" shows the Finder with
    # 123.EXE selected and NOT launched (spec/flair_i123_traces.mk
    # FLAIR_I123_PRE_SPEC: one click); it types nothing.
    pre=
    if head -n 1 "$keys" | grep -qx '#!select'; then
        [ -z "$tokens" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s: a #!select figure types nothing\n' "$keys"; exit 1; }
        pre=${launch%,l1,l0}
    fi
    want=$(printf '%s' "$tokens" | tr ',' '\n' | grep -c '^k' || true)
    # Park the pointer before the LAST keystroke, so that a screen the last
    # key opens (an alert box) is never drawn under the pointer.
    last=${tokens##*,}; head=${tokens%,*}
    if [ -n "$pre" ]; then spec="$pre,m0:-100"
    elif [ "$head" != "$tokens" ]; then spec="$launch,$head,m0:-100,$last"
    else spec="$launch,m0:-100,$tokens"; fi
    marker=${marker:-FLAIR-LIVE-OK}
    [ ${#spec} -lt 4095 ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s: %s bytes of trace (harness max 4095)\n' "$name" "${#spec}"; exit 1; }
    stage=${stage:-$(cat "$disks/top")}
    [ -f "$disks/s$stage.img" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s: no data volume for stage "%s"\n' "$name" "$stage"; exit 1; }
    cp -f "$disks/s$stage.img" "$out/$name.data.img"
    rm -f "$out/$name.ppm" "$out/$name.serial"
    "$runner" --disk "$img" --disk2 "$out/$name.data.img" --name "$name" --out "$out" \
        --mouse "$spec" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK \
        --timeout-ms 240000 --screendump --screendump-after "$marker" \
        < /dev/null 2> "$out/$name.report" || true
    if grep -q 'triple_fault=1' "$out/$name.report"; then printf '!!! test-pubs RED SCREEN-CAPTURE: %s: triple fault\n' "$name"; exit 1; fi
    s=$out/$name.serial
    [ -s "$out/$name.ppm" ] && grep -qx 'FLAIR-LIVE-OK' "$s" && ! grep -q '^PANIC' "$s" || {
        cat "$out/$name.report"; printf '!!! test-pubs RED SCREEN-CAPTURE: %s: no settled screendump\n' "$name"; exit 1; }
    got=$(grep -c '^TENANT-EVT what=3$' "$s" || true)
    [ "$got" = "$want" ] || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: 123 received %s of %s keystrokes (the image life ran out, or a key was lost)\n' "$name" "$got" "$want"; exit 1; }
    grep -q '^FLAIR-CURSOR x=211 y=22$' "$s" || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: the pointer was not parked on the menu bar\n' "$name"; exit 1; }
    # A figure taken at its own marker (a modal alert) may leave the alert
    # open when the image ends; any other figure must end with no gesture.
    [ "$marker" != FLAIR-LIVE-OK ] || ! grep -q 'FLAIR-TRACK-EXPIRED' "$s" || {
        printf '!!! test-pubs RED SCREEN-CAPTURE: %s: a gesture outlived the image\n' "$name"; exit 1; }
    grep -qF "$marker" "$s" || { printf '!!! test-pubs RED SCREEN-CAPTURE: %s: marker "%s" never appeared\n' "$name" "$marker"; exit 1; }
    # shellcheck disable=SC2086
    "$shot" png "$out/$name.ppm" $rect "$scale" "$out/$name.png" "$clut" "$canon"
    "$shot" b64 "$out/$name.png" "$out/$name.b64"
    rm -f "$out/$name.data.img" "$out/$name.qemu.log"
    n=$((n + 1))
    printf '>>> flair_capture: %s (%s keystrokes, crop %s, scale %s)\n' "$name" "$want" "$rect" "$scale"
done < "$list"
[ "$n" -gt 0 ] || { printf '!!! flair_capture: %s defines no figures\n' "$list"; exit 1; }
printf '>>> flair_capture: %d FLAIR figures captured from the running system\n' "$n"

#!/bin/sh
# flair_capture.sh -- FLAIR screen figures for the manuals, captured from the
# BOOTED system (PUB-REF-0001 NT-20, NT-21 (b), NT-22; bead initech-ipu5.10).
#
# The text route (capture.sh + console_text) reads the 80 x 25 console back
# into characters; a FLAIR window is pixels, so this is the pixel route: boot
# the flagship desktop image with a copy of the shipped data volume under
# the QEMU harness, drive it with a LOCKED injected trace, take the QMP
# screendump when the desktop's pump life ends (FLAIR-LIVE-OK), crop the
# window, convert it to grey through the fixed palette-to-grey table
# (harness/pubs/ppm_grey_png.c) and write a 1:1 PNG.  The book places the PNG
# at 3 x 3 on a 600 dpi grid, interpolation off (initech-wpc.sty \FlairShot).
#
# usage: flair_capture.sh <qemu_harness> <boot.img> <data.img> <ppm_grey_png>
#                         <FLAIRSHOTS> <out dir> <launch spec file> <launch var>
# FLAIRSHOTS rows ('#' comments): name|x0 y0 x1 y1 -- the crop on the 640 x
# 480 screen.  The trace of figure NAME is <FLAIRSHOTS dir>/NAME.trace: one
# line of harness tokens ("kf6", "kshift-h", "kalt-f3"; the conventions of
# spec/flair_app_launch_traces.mk) typed after InitechWord has opened.  The
# opening -- the moves and double-clicks that open the data volume, APPS and
# WORD.EXE -- is NOT repeated in each trace: it is read from the locked
# spec trace <launch var> in <launch spec file> (spec/flair_iword_traces.mk
# FLAIR_IWORD_TYPE_SPEC: every token up to and including the third click
# pair, the double-click on WORD.EXE), so a figure opens InitechWord exactly
# as the locked gate does.
# Determinism (Rule 11): the trace and the pump life (ticks, never wall
# clock) fix what is on the screen; the crop leaves out the menu bar's clock
# (the guest clock is not pinned: --rtc-deterministic ties the PIT to retired
# instructions and the desktop's life then ends before the trace is read).  Fails loud on a triple fault, a missing screendump, a
# desktop that never reported FLAIR-LIVE-OK, or WORD not running.
# Thin shell (Law 3).
set -eu
runner=$1; boot=$2; data=$3; grey=$4; defs=$5; out=$6; specf=$7; var=$8
dir=$(dirname "$defs")
mkdir -p "$out"
full=$(sed -n "s/^$var := //p" "$specf")
[ -n "$full" ] || { printf '!!! test-pubs RED FLAIR-TRACE: %s has no %s\n' "$specf" "$var"; exit 1; }
# the opening: tokens up to the 6th button release (A, B, CWORD double-clicks)
launch=$(printf '%s\n' "$full" | tr ',' '\n' | awk '{ print } /^l0$/ { if (++n == 6) exit }' | paste -sd, -)
case $launch in *,l1,l0,l1,l0) ;; *) printf '!!! test-pubs RED FLAIR-TRACE: the opening of %s does not end in a double-click\n' "$var"; exit 1 ;; esac
n=0
while IFS='|' read -r name crop; do
    case $name in ''|'#'*) continue ;; esac
    tr=$dir/$name.trace
    [ -f "$tr" ] || { printf '!!! test-pubs RED FLAIR-TRACE: %s missing\n' "$tr"; exit 1; }
    keys=$(sed '/^#/d' "$tr" | tr -d ' \n')
    [ -n "$keys" ] || { printf '!!! test-pubs RED FLAIR-TRACE: %s is empty\n' "$tr"; exit 1; }
    set -- $crop
    [ $# -eq 4 ] || { printf '!!! test-pubs RED FLAIR-TRACE: %s: crop "%s" is not x0 y0 x1 y1\n' "$name" "$crop"; exit 1; }
    cp -f "$data" "$out/$name.data.img"
    rm -f "$out/$name.ppm" "$out/$name.serial"
    "$runner" --disk "$boot" --disk2 "$out/$name.data.img" --name "$name" --out "$out" \
        --mouse "$launch,$keys" --keys-after FLAIR-LIVE-READY --quit-after FLAIR-LIVE-OK \
        --timeout-ms 60000 --screendump --screendump-after FLAIR-LIVE-OK \
        < /dev/null 2> "$out/$name.report" || true
    if grep -q 'triple_fault=1' "$out/$name.report"; then printf '!!! test-pubs RED FLAIR-CAPTURE: %s: triple fault\n' "$name"; exit 1; fi
    grep -qx 'FLAIR-LIVE-OK' "$out/$name.serial" && ! grep -q '^PANIC' "$out/$name.serial" || {
        printf '!!! test-pubs RED FLAIR-CAPTURE: %s: the desktop did not finish its life (FLAIR-LIVE-OK)\n' "$name"; exit 1; }
    grep -qx 'TENANT-REGISTER ok app=WORD' "$out/$name.serial" || {
        printf '!!! test-pubs RED FLAIR-CAPTURE: %s: InitechWord never registered\n' "$name"; exit 1; }
    if grep -q '^TENANT-EXIT' "$out/$name.serial"; then printf '!!! test-pubs RED FLAIR-CAPTURE: %s: InitechWord exited before the screendump\n' "$name"; exit 1; fi
    [ -s "$out/$name.ppm" ] || { printf '!!! test-pubs RED FLAIR-CAPTURE: %s: no screendump\n' "$name"; exit 1; }
    "$grey" spec/assets/clut.json spec/assets/color_canon.json "$out/$name.ppm" "$1" "$2" "$3" "$4" "$out/$name.png"
    n=$((n + 1))
done < "$defs"
[ "$n" -gt 0 ] || { printf '!!! test-pubs RED FLAIR-TRACE: %s defines no figures\n' "$defs"; exit 1; }
printf '>>> flair_capture.sh: %d FLAIR figures captured from the booted system\n' "$n"

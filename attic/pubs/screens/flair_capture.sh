#!/bin/sh
# flair_capture.sh -- FLAIR screen figures for the manuals, captured from the
# BOOTED system.  ONE script, TWO routes, chosen by argument count (merge of the
# InitechWord lane 3c13e9c1 / bead initech-ipu5.10 and the FLAIR lane /
# bead initech-ipu5.11, which each wrote a flair_capture.sh):
#   8 arguments   -> the InitechWord route (iword_main): FLAIRSHOTS rows
#                    name|x0 y0 x1 y1, NAME.trace, keys typed after WORD.EXE opens.
#   10..12 args   -> the FLAIR desktop route (flair_main): FLAIR rows
#                    name|marker|crop[|proves[|image]], NAME.mouse, .grey output.
# The two headers follow, unchanged.
#
# ===== InitechWord route =====
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

# ===== FLAIR desktop route =====
#
# Bead initech-ipu5.11 (the FLAIR manuals).  The FLAIR counterpart of
# capture.sh: every FLAIR figure in an Initech manual is real FLAIR output
# (CLAUDE.md Law 2, Rule 14; PUB-REF-0001 NT-20..22, GC-04).  For each row of
# the manual's screens/FLAIR file this script boots the FLAIR desktop in QEMU
# through the factory harness, attaches a fresh copy of the shipped data
# volume (build/flair_data.img, the INITECH volume), injects the row's LOCKED
# mouse/keyboard trace (screens/<name>.mouse, the qemu_harness --mouse token
# language: m<dx>:<dy> move, l1/l0 button, k<chord> one keystroke, w<ms>
# hold), takes the QMP screendump once the row's serial marker has appeared
# (marker#N: its N-th occurrence; a FLAIR-MENU-DROP / -XDROP marker is taken
# while the button is still held, so a pulled-down menu is captured open),
# and converts the screendump through harness/pubs/flair_grey (the fixed
# palette-to-grey table of NT-22, the crop of NT-21) into <out>/<name>.grey,
# the ASCII grey listing the manual commits (the build expands it into the
# PNG it prints; harness/pubs/flair_grey.c documents the format).
#
# The image (pubs/flair.mk PUBS_FLAIR_IMG, build/flair_tenants_pubs.img) is
# the Rule-14 EXTRA-LONG RECORD kernel of the shipped desktop -- the SAME
# flair_tenants objects as build/initech.img with only the record knobs:
# a 12000-tick pump life, the record double-click bounds, the steady record
# console cursor, and the FILE COPY bar stepped per injected event
# (FILECOPY_RECORD_STEP) -- plus the transient COMMAND.COM, so Exit to
# InitechDOS works.  The shipped image's 250-tick pump ends before a
# manual-length trace has been injected.

# Determinism (Rule 11): the guest RTC is pinned to 1992-03-02 09:30:00 (the
# record-flair pattern, RECORD_RTC_DEFAULT: wall-paced, not icount -- under
# --rtc-deterministic an application launch spends the pump's 30-second life
# in virtual time before a trace is half injected), so the menu-bar clock
# reads 9:30 am in every figure (each capture runs well inside the minute,
# and no figure shows seconds); the data volume is the deterministic FAT12
# image the Makefile mints; the listing depends on the pixels only.
# Synchronisation is by serial marker, never by delay alone: a trace may
# hold with W<marker> (qemu_harness) until the guest has done what the next
# token needs (an application registered, a window drawn).
#
# Usage: flair_capture.sh <qemu_harness> <boot.img> <data.img> <flair_grey>
#                         <clut.json> <color_canon.json> <console_text>
#                         <vgabios.bin> <FLAIR file> <out dir> [<step image>
#                         [<locked data image>]]
# FLAIR file format, one screen per line ('#' comments allowed):
#   name|marker|crop[|proves[|image]]
# crop is `full` or x,y,w,h in screen pixels (640 x 480), or `text`: the
# screen is the InitechDOS console FLAIR hands back on Exit to InitechDOS,
# decoded character for character by harness/console_text (NT-21 text
# route) into <out>/<name>.console (25 lines, the .txt
# format of the DOS books; a different suffix keeps check_screens.sh for
# the DOS route) instead of a grey listing.  The trace is
# <FLAIR dir>/name.mouse: tokens separated by commas and/or newlines; lines
# starting with '#' are comments (the derivation of each move).  The pointer
# starts at the screen centre (320,240).  proves (optional) is a comma list
# of the serial markers of the desktop operations the trace performs
# (FINDER-OPEN-VOLUME, FLAIR-ZOOM ...): each must be on the serial capture,
# or the row is RED -- a documented operation is replayed, not asserted.
# image (optional) is `step`: the row is captured on <step image>, the
# record image whose FILE COPY bar advances a fixed step per injected event
# (FLAIRTENANTS_RECORDXLONGDBL_IMG, kmain.c FILECOPY_RECORD_STEP) instead of
# by the clock, so a figure of the bar in mid-copy is the same every run;
# or `nodisk`: the row is captured with NO data disk attached (the
# troubleshooting figure of a computer started without its data disk);
# or `locked`: the row is captured on <locked data image>, the shipped data
# volume with README.TXT made read-only (build/record_locked_trash.img, the
# test-flair-locked-trash fixture), for the figure of the Locked notice --
# no file of the volume as delivered is locked, so the notice cannot be
# reached on it (a row claiming the notice on the stock volume is RED:
# OPERATION-UNREPLAYED, the LOCKED_ON_STOCK_VOLUME mutant).
# Thin shell (Law 3).  Fails loud on a missing marker, a triple fault, a
# guest whose pump ended before the capture, or a pixel outside the palette.

set -eu
iword_main() {
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
}
flair_main() {
    runner=$1; boot=$2; data=$3; grey=$4; clut=$5; canon=$6; decode=$7; rom=$8; defs=$9; out=${10}; stepimg=${11:-}; lockedimg=${12:-}
    mkdir -p "$out"
    [ -r "$boot" ] || { printf '!!! flair_capture.sh FAIL: boot image %s not readable\n' "$boot"; exit 1; }
    [ -r "$data" ] || { printf '!!! flair_capture.sh FAIL: data volume %s not readable\n' "$data"; exit 1; }
    n=0
    while IFS='|' read -r name marker crop proves image; do
        case $name in ''|'#'*) continue ;; esac
        trace=$(dirname "$defs")/$name.mouse
        [ -f "$trace" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s missing\n' "$trace"; exit 1; }
        spec=$(sed -e '/^#/d' -e 's/[[:space:]]//g' "$trace" | tr '\n' ',' | sed -e 's/,,*/,/g' -e 's/^,//' -e 's/,$//')
        [ -n "$spec" ] || { printf '!!! test-pubs RED SCREEN-KEYS: %s is empty\n' "$trace"; exit 1; }
        [ ${#spec} -le 4095 ] || { printf '!!! flair_capture.sh FAIL: %s: trace is %d bytes (harness max 4095)\n' "$name" "${#spec}"; exit 1; }
        case $crop in
        full|text) set -- 0 0 0 0 ;;
        *,*,*,*) set -- $(printf '%s' "$crop" | tr ',' ' ') ;;
        *) printf '!!! test-pubs RED SCREEN-KEYS: %s: crop "%s" is not full or x,y,w,h\n' "$name" "$crop"; exit 1 ;;
        esac
        count=1
        case $marker in *'#'[0-9]*) count=${marker##*#}; marker=${marker%#*} ;; esac
        n=$((n + 1))
        work=$out/$name.data.img
        src=$data
        rm -f "$out/$name.ppm" "$out/$name.serial"
        img=$boot
        d2="--disk2 $work"
        case $image in
        '') ;;
        nodisk) d2= ;;
        step) [ -n "$stepimg" ] && [ -r "$stepimg" ] || { printf '!!! flair_capture.sh FAIL: %s: no step image given\n' "$name"; exit 1; }
              img=$stepimg ;;
        locked) [ -n "$lockedimg" ] && [ -r "$lockedimg" ] || { printf '!!! flair_capture.sh FAIL: %s: no locked data image given\n' "$name"; exit 1; }
              src=$lockedimg ;;
        *) printf '!!! test-pubs RED SCREEN-KEYS: %s: image "%s" is not empty, step, nodisk or locked\n' "$name" "$image"; exit 1 ;;
        esac
        cp -f "$src" "$work"
        # A run whose marker never reaches serial is tried once more (a PS/2
        # packet lost by a loaded host leaves the trace short); whatever the
        # attempt, the capture is still compared byte for byte by the gate.
        for attempt in 1 2; do
            rm -f "$out/$name.ppm" "$out/$name.serial"
            cp -f "$src" "$work"
            # A private copy of the boot image too: QEMU write-locks its disks,
            # so two captures (or a capture and a gate) never share one.
            cp -f "$img" "$out/$name.boot.img"
            "$runner" --disk "$out/$name.boot.img" $d2 --name "$name" --out "$out" \
                --mouse "$spec" --keys-after FLAIR-LIVE-READY --timeout-ms 90000 \
                --screendump --screendump-after "$marker" --screendump-after-count "$count" \
                --rtc-base 1992-03-02T09:30:00 \
                < /dev/null 2> "$out/$name.report" || true
            [ -s "$out/$name.ppm" ] && break
            grep -q 'triple_fault=1' "$out/$name.report" && break
            [ "$attempt" = 1 ] && printf '>>> flair_capture.sh: %s: marker not reached, capturing again\n' "$name"
        done
        if grep -q 'triple_fault=1' "$out/$name.report"; then
            printf '!!! flair_capture.sh FAIL: %s: triple fault\n' "$name"; exit 1
        fi
        [ -s "$out/$name.ppm" ] || {
            tr -d '\r' < "$out/$name.serial" 2>/dev/null | grep -v 'FLAIR-TICK\|FLAIR-CURSOR' | tail -15 || true
            printf '!!! flair_capture.sh FAIL: %s: no screendump (marker "%s" x%s never reached serial)\n' "$name" "$marker" "$count"; exit 1; }
        # The capture must be of a LIVE desktop: a pump that ended (FLAIR-LIVE-OK)
        # before the marker would show a frozen frame.
        if tr -d '\r' < "$out/$name.serial" | sed -n "/$(printf '%s' "$marker" | sed 's/[]\/.*[^$]/\\&/g')/q;p" | grep -q 'FLAIR-LIVE-OK'; then
            printf '!!! flair_capture.sh FAIL: %s: the desktop pump ended before the marker\n' "$name"; exit 1
        fi
        for op in $(printf '%s' "$proves" | tr ',' ' '); do
            tr -d '\r' < "$out/$name.serial" | grep -qF -- "$op" || {
                printf '!!! test-pubs RED OPERATION-UNREPLAYED: %s: %s never reached serial\n' "$name" "$op"; exit 1; }
        done
        if [ "$crop" = text ]; then
            "$decode" "$rom" "$out/$name.ppm" > "$out/$name.console" || {
                printf '!!! flair_capture.sh FAIL: %s: console_text could not decode the screendump\n' "$name"; exit 1; }
        else
            "$grey" capture "$clut" "$canon" "$out/$name.ppm" "$@" "$out/$name.grey" || {
                printf '!!! flair_capture.sh FAIL: %s: flair_grey refused the screendump\n' "$name"; exit 1; }
        fi
        rm -f "$work" "$out/$name.boot.img"
        printf '>>> flair_capture.sh: %s (%s, %s)\n' "$name" "$marker" "$crop"
    done < "$defs"
    [ "$n" -gt 0 ] || { printf '!!! flair_capture.sh FAIL: %s defines no screens\n' "$defs"; exit 1; }
    printf '>>> flair_capture.sh: %d FLAIR screens captured from the running OS\n' "$n"
}
case $# in
8) iword_main "$@" ;;
10|11|12) flair_main "$@" ;;
*) printf '!!! flair_capture.sh FAIL: %d arguments (8 = InitechWord route, 10..12 = FLAIR route)\n' "$#"; exit 1 ;;
esac

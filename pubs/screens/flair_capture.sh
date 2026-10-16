#!/bin/sh
# flair_capture.sh -- capture FLAIR screen reproductions from the running OS.
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
# The image is the Rule-14 RECORD image of the shipped desktop
# (FLAIRTENANTS_RECORDLONGDBL_IMG): the SAME flair_tenants kernel as
# build/initech.img with only the live pump's life widened (6000 ticks) and
# the double-click bounds of the record machinery -- every record-flair clip
# is captured on it.  The shipped image's 250-tick pump ends before a
# manual-length trace has been injected.
#
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
#                         <vgabios.bin> <FLAIR file> <out dir>
# FLAIR file format, one screen per line ('#' comments allowed):
#   name|marker|crop[|proves]
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
# Thin shell (Law 3).  Fails loud on a missing marker, a triple fault, a
# guest whose pump ended before the capture, or a pixel outside the palette.
set -eu
runner=$1; boot=$2; data=$3; grey=$4; clut=$5; canon=$6; decode=$7; rom=$8; defs=$9; out=${10}
mkdir -p "$out"
[ -r "$boot" ] || { printf '!!! flair_capture.sh FAIL: boot image %s not readable\n' "$boot"; exit 1; }
[ -r "$data" ] || { printf '!!! flair_capture.sh FAIL: data volume %s not readable\n' "$data"; exit 1; }
n=0
while IFS='|' read -r name marker crop proves; do
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
    cp -f "$data" "$work"
    rm -f "$out/$name.ppm" "$out/$name.serial"
    "$runner" --disk "$boot" --disk2 "$work" --name "$name" --out "$out" \
        --mouse "$spec" --keys-after FLAIR-LIVE-READY --timeout-ms 90000 \
        --screendump --screendump-after "$marker" --screendump-after-count "$count" \
        --rtc-base 1992-03-02T09:30:00 \
        < /dev/null 2> "$out/$name.report" || true
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
    rm -f "$work"
    printf '>>> flair_capture.sh: %s (%s, %s)\n' "$name" "$marker" "$crop"
done < "$defs"
[ "$n" -gt 0 ] || { printf '!!! flair_capture.sh FAIL: %s defines no screens\n' "$defs"; exit 1; }
printf '>>> flair_capture.sh: %d FLAIR screens captured from the running OS\n' "$n"

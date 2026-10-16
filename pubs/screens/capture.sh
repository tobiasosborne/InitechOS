#!/bin/sh
# capture.sh -- capture manual screen reproductions from the running OS.
#
# Bead initech-ipu5.5.  Every \screen in an Initech manual is real InitechDOS
# output (CLAUDE.md Law 2; DESIGN-SYSTEM.md Sec 5 "screen captures from the
# emulator"): this script boots the default COMMAND.COM image in QEMU through
# the factory harness, types the keystrokes named in the manual's SCREENS file,
# takes the QMP screendump once the named marker text is on the serial mirror,
# and reads the 80x25 console back out of the pixels with harness/console_text.
#
# Determinism (Rule 11): the practice diskette is minted from pubs/screens/disk
# with fixed timestamps, label and serial number; the guest RTC is pinned to
# 1992-03-02 09:30:00 and tied to instructions retired (--rtc-deterministic).
# The diskette layout is pubs/screens/disk/MANIFEST (bead initech-ipu5.7):
#   path|YYYY-MM-DD HH:MM|source
# in directory-entry order.  `path/` makes a subdirectory; a source `@NAME`
# is the shipped utility NAME from <utilities dir> (the very binaries the
# Makefile plants in APPS on the shipped data volume, build/flair_data.img);
# any other source is a text file in <disk dir>, written with CR LF endings.
#
# Usage: capture.sh <qemu_harness> <boot.img> <console_text> <vgabios.bin>
#                   <SCREENS file> <disk dir> <out dir> <utilities dir>
# SCREENS format, one screen per line ('#' comments allowed):
#   name|marker|keystrokes
# marker may end in #N: the capture is taken once the marker has appeared N
# times on the serial mirror (typed keys echo there too), so a screen can end
# on output whose text was already seen earlier in the session.
# keystrokes are literal text read with printf %b (\n = Enter, \\ = backslash).
# Thin shell (Law 3).  Fails loud on any capture that is not a settled prompt.
set -eu
runner=$1; boot=$2; decode=$3; rom=$4; defs=$5; disk=$6; out=$7; utils=$8
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$out"

for t in mformat mcopy; do
    command -v $t >/dev/null 2>&1 || { printf '!!! capture.sh FAIL: %s (mtools) not found\n' "$t"; exit 1; }
done
[ -r "$rom" ] || { printf '!!! capture.sh FAIL: VGA BIOS %s not readable\n' "$rom"; exit 1; }

# Mint the practice diskette once: 1.44 MB FAT12, label INITECH, CRLF text.
img=$out/practice.img
stage=$out/practice.d
rm -rf "$stage"; mkdir -p "$stage"
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
# The label entry carries a date too (CHKDSK prints it): pin it.
SOURCE_DATE_EPOCH=$(TZ=UTC date -d "1992-03-01 08:00" +%s) TZ=UTC \
    mformat -i "$img" -f 1440 -N 19920302 -v INITECH :: < /dev/null
[ -r "$disk/MANIFEST" ] || { printf '!!! capture.sh FAIL: %s/MANIFEST missing\n' "$disk"; exit 1; }
while IFS='|' read -r path stamp src; do
    case $path in ''|'#'*) continue ;; esac
    epoch=$(TZ=UTC date -d "$stamp" +%s)
    case $path in
    */)
        SOURCE_DATE_EPOCH=$epoch TZ=UTC mmd -i "$img" "::${path%/}" < /dev/null ;;
    *)
        f=$stage/$(printf '%s' "$path" | tr '/' '_')
        case $src in
        @*) [ -r "$utils/${src#@}" ] || { printf '!!! capture.sh FAIL: utility %s not in %s\n' "${src#@}" "$utils"; exit 1; }
            cp "$utils/${src#@}" "$f" ;;
        *)  [ -r "$disk/$src" ] || { printf '!!! capture.sh FAIL: %s not in %s\n' "$src" "$disk"; exit 1; }
            sed 's/$/\r/' "$disk/$src" > "$f" ;;
        esac
        TZ=UTC touch -d "$stamp" "$f"
        TZ=UTC mcopy -m -i "$img" "$f" "::$path" < /dev/null ;;
    esac
done < "$disk/MANIFEST"

n=0
while IFS='|' read -r name marker keys; do
    case $name in ''|'#'*) continue ;; esac
    n=$((n + 1))
    work=$out/$name.img
    cp "$img" "$work"
    # An empty keystroke field captures the startup screen itself.
    if [ -n "$keys" ]; then
        tokens=$(sh "$here/keys.sh" "$keys")
        # qemu_harness truncates a --keys spec past 4095 bytes; never let a
        # screen silently lose its last keystrokes.
        [ ${#tokens} -le 4095 ] || {
            printf '!!! capture.sh FAIL: %s: %d bytes of key tokens (harness max 4095)\n' "$name" "${#tokens}"; exit 1; }
        set -- --keys "$tokens" --keys-after SHELL-READY --keys-after-ms 30000
    else
        set --
    fi
    count=1
    case $marker in *'#'[0-9]*) count=${marker##*#}; marker=${marker%#*} ;; esac
    rm -f "$out/$name.ppm" "$out/$name.serial"
    "$runner" --disk "$boot" --disk2 "$work" --name "$name" --out "$out" \
        --timeout-ms 60000 "$@" \
        --screendump --screendump-after "$marker" --screendump-after-count "$count" \
        --rtc-base 1992-03-02T09:30:00 --rtc-deterministic \
        < /dev/null 2> "$out/$name.report" || true
    if grep -q 'triple_fault=1' "$out/$name.report"; then
        printf '!!! capture.sh FAIL: %s: triple fault\n' "$name"; exit 1
    fi
    [ -s "$out/$name.ppm" ] || {
        cat "$out/$name.report"
        printf '!!! capture.sh FAIL: %s: no screendump (marker "%s" never reached serial)\n' "$name" "$marker"; exit 1; }
    "$decode" "$rom" "$out/$name.ppm" > "$out/$name.txt" || {
        printf '!!! capture.sh FAIL: %s: console_text could not decode the screendump\n' "$name"; exit 1; }
    # A settled capture ends on an empty command prompt (nothing typed after it).
    last=$(sed '/^$/d' "$out/$name.txt" | tail -n 1)
    printf '%s\n' "$last" | grep -Eq '^A:\\[A-Z0-9.\\]*>$' || {
        cat "$out/$name.txt"
        printf '!!! capture.sh FAIL: %s: screen did not settle on a prompt (last line "%s")\n' "$name" "$last"; exit 1; }
    printf '>>> capture.sh: %s (%s lines of text)\n' "$name" "$(sed '/^$/d' "$out/$name.txt" | wc -l)"
done < "$defs"
[ "$n" -gt 0 ] || { printf '!!! capture.sh FAIL: %s defines no screens\n' "$defs"; exit 1; }
printf '>>> capture.sh: %d screens captured from the running OS\n' "$n"

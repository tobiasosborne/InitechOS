#!/bin/sh
# ug8l: compare classification with genuine DOS 3.30, not artifact output.
# Golden provenance/re-mint command: chkdsk-label-golden/README.md.
# DOS omits the empty category; Initech's existing report prints a zero row.
# Only hidden allocation/count are compared here; the existing CHKDSK oracle
# separately checks exact report columns, all categories, memory and repairs.
set -eu
work=$1; candidate=$2
golden=harness/diff/fat_diff/chkdsk-label-golden
export SOURCE_DATE_EPOCH=699408000 TZ=UTC
mkdir -p "$work"
printf 'fixture\r\n' > "$work/HIDDEN.TXT"
for name in none label hidden both; do
    image=$work/$name.img
    dd if=/dev/zero of="$image" bs=512 count=2880 status=none
    mformat -i "$image" -f 1440 -N 19920302 ::
    case $name in label|both) mlabel -i "$image" ::CHKTEST ;; esac
    case $name in hidden|both)
        mcopy -i "$image" "$work/HIDDEN.TXT" ::HIDDEN.TXT
        mattrib -i "$image" +h ::HIDDEN.TXT ;;
    esac
    cp -f "$image" "$image.before"
    "$candidate" "$image" 'A:' < /dev/null > "$work/$name.raw"
    tr -d '\r' < "$work/$name.raw" > "$work/$name.txt"
    cmp -s "$image.before" "$image" || { echo 'RED CHKDSK-LABEL-READONLY'; exit 1; }
    # Read only the independent mint, never the candidate, for expected values.
    expected=$(awk '/bytes in .* hidden files/{print $1, $4; found=1} END{if(!found)print "0 0"}' "$golden/$name.txt")
    set -- $expected
    row=$(printf '%9u bytes in %u hidden files' "$1" "$2")
    grep -qxF "$row" "$work/$name.txt" || {
        cat "$work/$name.txt"; echo "RED CHKDSK-LABEL-COUNT: $name expected $row"; exit 1; }
    free=$(mdir -i "$image" :: | sed -n '/bytes free/s/[^0-9]//gp')
    grep -qxF "$(printf '%9u bytes available on disk' "$free")" "$work/$name.txt" || {
        echo "RED CHKDSK-LABEL-FREE: $name"; exit 1; }
    echo "GREEN CHKDSK label classification $name: $expected (DOS 3.30 mint)"
done

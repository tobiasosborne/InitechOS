#!/bin/sh
# Independent period-file reader: also used by the component differential suite.
set -eu
disk=$1
out=$2
reader=${WORDPERFECT51_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/wordperfect51-decomp}/tools/wp5_ref.py
mcopy -o -i "$disk" ::WORD.WP5 "$out.WP5"
python3 -I "$reader" "$out.WP5" > "$out.readback"
grep -Fx "  TEXT: 'office reports are ready.'" "$out.readback" >/dev/null
grep -F "T'office ' [AttrOn Bold] T'reports' [AttrOff Bold] T' are ready.'" "$out.readback" >/dev/null
grep 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$out.readback" >/dev/null

#!/bin/sh
# Independent period-file reader: also used by the component differential suite.
set -eu
disk=$1
out=$2
reader=${WORDPERFECT51_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/wordperfect51-decomp}/tools/wp5_ref.py
mcopy -o -i "$disk" ::WORD.WP5 "$out.WP5"
python3 -I "$reader" "$out.WP5" > "$out.readback"
grep -Fx "  TEXT: 'office reports are ready. rev1 rev2'" "$out.readback" >/dev/null
grep -F "T'office ' [AttrOn Bold] T'reports' [AttrOff Bold] T' are ready. rev1 rev2'" "$out.readback" >/dev/null
grep 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$out.readback" >/dev/null
mcopy -o -i "$disk" '::WORD.BK!' "$out.BK!"
python3 -I "$reader" "$out.BK!" > "$out.backup-readback"
grep -Fx "  TEXT: 'office reports are ready. rev1'" "$out.backup-readback" >/dev/null
grep -F "T'office ' [AttrOn Bold] T'reports' [AttrOff Bold] T' are ready. rev1'" "$out.backup-readback" >/dev/null
grep 'END offset=\([0-9]*\) filesize=\1 trailer-errors=0$' "$out.backup-readback" >/dev/null

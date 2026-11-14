#!/bin/sh
# The independent Lotus corpus reader is also the Initech 123 host oracle.
set -eu
disk=$1
out=$2
reader=${LOTUS123_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/lotus123-decomp}/tools/wk1_ref.py
mcopy -o -i "$disk" ::SLICE.WK1 "$out.WK1"
python3 "$reader" "$out.WK1" > "$out.readback"
sh "$(dirname "$0")/../../../../factory/worksheet-check.sh" "$out.WK1" "$out.readback"

#!/bin/sh
# The independent Lotus corpus reader is also the Initech 123 host oracle.
set -eu
disk=$1
out=$2
reader=${LOTUS123_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/lotus123-decomp}/tools/wk1_ref.py
mcopy -o -i "$disk" ::SLICE.WK1 "$out.WK1"
python3 "$reader" "$out.WK1" > "$out.readback"
grep -F 'P:Default c0 r0 int=123' "$out.readback" >/dev/null
grep -F 'P:Default c1 r0 num=1.5' "$out.readback" >/dev/null
grep -F 'P:Default c2 r0 cached=124.5 len=12 rpn: RANGE(Cr-2,Rr+0..Cr-1,Rr+0) @SUM/1 RETURN' "$out.readback" >/dev/null
[ "$(grep -Ec '^  (INTEGER|NUMBER|FORMULA) ' "$out.readback")" -eq 3 ]
grep -E '^  EOF +len=0' "$out.readback" >/dev/null

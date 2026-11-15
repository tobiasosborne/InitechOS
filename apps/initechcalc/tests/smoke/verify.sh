#!/bin/sh
# Independent readback of the booted save: mtools extracts the files; the
# expected TABLES.TXT is the shipped ledger with the typed row inserted by awk
# (the first cell, 73, a general number), never Calc's serializer. The
# windows were not moved, so the saved positions are the shipped ones.
set -eu
disk=$1 out=$2
mcopy -o -i "$disk" ::APPS/TABLES.TXT "$out.txt"
awk '{ print } index($0,"ROWS\t")==1 { printf "1\t(G) 73\r\n" }' apps/initechcalc/data/LEDGER.TXT > "$out.expected"
cmp "$out.expected" "$out.txt"
mcopy -i "$disk" ::APPS/LEDGER.TXT - | cmp - apps/initechcalc/data/LEDGER.TXT
if mdir -b -i "$disk" ::APPS | grep -E 'TABLE.*\.TMP$'; then exit 1; fi
fsck.fat -n "$disk" > "$out.fsck"
# The booted scene against goldens/frame (tests/frame-pixels.sh): the chart
# before Save, then FILE COPY in front of it while the copy runs. The journey
# framework captures on QEMU only, so Bochs makes no pixel claim here.
case $out in *qemu-*)
 make -s -C apps/initechcalc "$PWD/build/apps/initechcalc/host/frame_pixels"
 sh apps/initechcalc/tests/frame-pixels.sh build/apps/initechcalc/host/frame_pixels "${out%qemu-*}calc.ppm" goldens/frame chart
 sh apps/initechcalc/tests/frame-pixels.sh build/apps/initechcalc/host/frame_pixels "${out%qemu-*}calc-copy.ppm" goldens/frame copy
 # The still beside the copy moment, for the operator's eye (not graded),
 # when the pinned still is provisioned (goldens/CORPORA.lock FRAME_STILL).
 still=${FRAME_STILL:-$HOME/Projects/initech-frame}/preview.ppm
 if [ -f "$still" ] && grep -q "^$(sha256sum < "$still" | cut -d' ' -f1)  preview.ppm" goldens/frame/SHA256SUMS; then
  make -s -C apps/initechcalc "$PWD/build/apps/initechcalc/host/frame_compare"
  # shellcheck disable=SC2046
  build/apps/initechcalc/host/frame_compare "$still" $(awk -F'\t' '$1=="SCREEN" { print $3, $4, $5, $6 }' goldens/frame/geometry.tsv) "${out%qemu-*}calc-copy.ppm" "${out%qemu-*}frame-compare.ppm"
 else echo "frame-compare: the pinned still is not provisioned (FRAME_STILL); no comparison written"
 fi
 ;; esac

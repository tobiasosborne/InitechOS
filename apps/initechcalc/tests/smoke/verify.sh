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
# The booted chart against goldens/frame (tests/frame-pixels.sh). The journey
# framework captures on QEMU only, so Bochs makes no pixel claim here.
case $out in *qemu-*)
 make -s -C apps/initechcalc "$PWD/build/apps/initechcalc/host/frame_pixels"
 sh apps/initechcalc/tests/frame-pixels.sh build/apps/initechcalc/host/frame_pixels "${out%qemu-*}calc.ppm" goldens/frame
 ;; esac

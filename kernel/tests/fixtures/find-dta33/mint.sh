#!/bin/sh
# Mint the B: data disk: root A1 B1 A2 BSUB(dir) SUB(dir) A3 B2, SUB holds S1 S2.
set -eu
img=$1
export SOURCE_DATE_EPOCH=699525000 TZ=UTC LC_ALL=C
truncate -s 1474560 "$img"
mformat -i "$img" -f 1440 -N 12345678 ::
t=$(mktemp -d)
for f in A1 B1 A2 A3 B2; do printf '%s\r\n' "$f" > "$t/$f.TXT"; done
printf 'S1\r\n' > "$t/S1.DAT"; printf 'S2\r\n' > "$t/S2.DAT"
touch -d '@699525000' "$t"/*
mcopy -m -i "$img" "$t/A1.TXT" "$t/B1.TXT" "$t/A2.TXT" ::
mmd -i "$img" ::BSUB ::SUB
mcopy -m -i "$img" "$t/A3.TXT" "$t/B2.TXT" ::
mcopy -m -i "$img" "$t/S1.DAT" "$t/S2.DAT" ::SUB
rm -rf "$t"

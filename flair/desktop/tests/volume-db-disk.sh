#!/bin/sh
# Mint two FAT12 roots with mtools: an installed-C geometry (eight-sector
# clusters) and a 1.44 MB DATA A:. Each root gets SAME.TXT first, so both
# occupy the same physical cluster, then an independently authored 56-byte
# IDB1 DESKTOP.DB (header, a kind-4 root view, a kind-5 Trash origin).
# After the Finder run, C must be byte-for-byte unchanged and A must hold
# exactly the authored record with only its view x moved to 100.
set -eu
export SOURCE_DATE_EPOCH=699350400 TZ=UTC
bin=$1 w=$2
mkdir -p "$w"
bytes() { for b; do printf "\\$(printf %03o "$b")"; done; }
zeros() { n=$1; while [ "$n" -gt 0 ]; do printf '\000'; n=$((n-1)); done; }
# IDB1, version 1, two records; view kind 4 at root (x and y little-endian
# 16-bit at record bytes 17 and 19); origin kind 5 at root named SAME.TXT.
db() {
 bytes 73 68 66 49 1 0 2 0
 bytes 4; zeros 15; bytes 0 "$1" 0 "$2"; zeros 4
 bytes 5 0 0 0; printf 'SAME.TXT'; zeros 12
}
db 70 90 > "$w/c.db"; db 46 115 > "$w/a.db"; db 100 115 > "$w/a-moved.db"
[ "$(wc -c < "$w/c.db")" -eq 56 ] || exit 1
printf 'Same name, different volume\r\n' > "$w/same.txt"
for v in c a; do
 image=$w/$v.img
 if [ "$v" = c ]; then sectors=16384 spc=8 label=INITECH; else sectors=2880 spc=1 label=DATA; fi
 truncate -s 0 "$image"; truncate -s $((sectors*512)) "$image"
 mformat -i "$image" -T "$sectors" -h 16 -s 63 -c "$spc" -v "$label" -N 0x19910330 ::
 mcopy -i "$image" "$w/same.txt" ::SAME.TXT
 mcopy -i "$image" "$w/$v.db" ::DESKTOP.DB
 mattrib -i "$image" +h ::DESKTOP.DB
 cp "$image" "$w/$v.before"
done
"$bin" "$w/c.img" "$w/a.img"
cmp "$w/c.before" "$w/c.img"
mtype -i "$w/a.img" ::DESKTOP.DB > "$w/a.after"
cmp "$w/a-moved.db" "$w/a.after"
mtype -i "$w/a.img" ::SAME.TXT | cmp "$w/same.txt" -
echo "ok volume DB disk: C unchanged, A moved view only"

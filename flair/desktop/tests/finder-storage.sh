#!/bin/sh
# Independent mtools mint/readback; no expected filesystem bytes from the driver.
set -eu
export SOURCE_DATE_EPOCH=699350400 TZ=UTC
bin=$1 w=$2
mkdir -p "$w"
for spec in 12 16 12c8; do
 kind=$spec; spc=1
 [ "$spec" != 12c8 ] || { kind=12; spc=8; }
 sectors=2880
 [ "$spec" = 12 ] || sectors=16384
 image=$w/fat$spec.img
 truncate -s 0 "$image"
 truncate -s $((sectors*512)) "$image"
 mformat -i "$image" -T "$sectors" -h 16 -s 63 -c "$spc" -N 0x19910330 ::
 printf 'Finder source bytes\r\n' > "$w/source.txt"
 mmd -i "$image" ::FOLDER ::EMPTY ::TRASH
 mcopy -i "$image" "$w/source.txt" ::SOURCE.TXT
 mcopy -i "$image" "$w/source.txt" ::LOCKED.TXT
 mattrib -i "$image" +r ::LOCKED.TXT
 awk 'BEGIN {for(i=0;i<100;i++) printf "Finder source bytes\r\n"}' > "$w/chain.txt"
 mcopy -i "$image" "$w/chain.txt" ::FOLDER/CHAIN.TXT
 mcopy -i "$image" "$w/source.txt" ::TRASH/GONE.TXT
 cp "$image" "$w/fat$spec.before"
 if [ "$kind" = 16 ]; then
  "$bin" "$image" "$kind" "$w/fat12.img"
 else
  "$bin" "$image" "$kind"
 fi
 if [ "$kind" = 16 ]; then
  cmp "$w/fat$spec.before" "$image"
  mtype -i "$w/fat12.img" ::OUT.TXT > "$w/out"
  cmp "$w/source.txt" "$w/out"
 else
  mtype -i "$image" ::COPY.TXT > "$w/copied"
  cmp "$w/source.txt" "$w/copied"
  mdir -i "$image" ::NEWFOLD > "$w/newfolder"
  printf 'IDB1\001\000\000\000' > "$w/expected-db"
  mtype -i "$image" ::DESKTOP.DB > "$w/desktop-db"
  cmp "$w/expected-db" "$w/desktop-db"
 fi
done

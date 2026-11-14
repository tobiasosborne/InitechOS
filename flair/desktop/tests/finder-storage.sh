#!/bin/sh
# Independent mtools mint/readback; no expected filesystem bytes from the driver.
set -eu
bin=$1 w=$2
mkdir -p "$w"
for kind in 12 16; do
 sectors=2880
 [ "$kind" = 12 ] || sectors=16384
 truncate -s 0 "$w/fat$kind.img"
 truncate -s $((sectors*512)) "$w/fat$kind.img"
 mformat -i "$w/fat$kind.img" -T "$sectors" -h 16 -s 63 -c 1 -N 0x19910330 ::
 printf 'Finder source bytes\r\n' > "$w/source.txt"
 mmd -i "$w/fat$kind.img" ::FOLDER ::EMPTY ::TRASH
 mcopy -i "$w/fat$kind.img" "$w/source.txt" ::SOURCE.TXT
 mcopy -i "$w/fat$kind.img" "$w/source.txt" ::TRASH/GONE.TXT
 cp "$w/fat$kind.img" "$w/fat$kind.before"
 "$bin" "$w/fat$kind.img" "$kind"
 if [ "$kind" = 16 ]; then
  cmp "$w/fat$kind.before" "$w/fat$kind.img"
 else
  mtype -i "$w/fat$kind.img" ::COPY.TXT > "$w/copied"
  cmp "$w/source.txt" "$w/copied"
  mdir -i "$w/fat$kind.img" ::NEWFOLD > "$w/newfolder"
 fi
done

#!/bin/sh
# PRD 6.1 differential: real mformat -c images, mtools + independent Python.
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800
tool=$1; tag=${2:-green}
w=build/fat-spc/$tag
mkdir -p "$w"
red() { echo "RED fat-spc $*"; exit 1; }
for spc in 2 4 8 16 32 64; do
 bpc=$((spc*512)); size=$((3*bpc+137))
 # Vary by byte AND sector: repeated sector patterns could hide wrong LBAs.
 awk -v n="$size" 'BEGIN { for(i=0;i<n;i++) printf "%c", (i*37+int(i/251)+int(i/512)*13)%256 }' > "$w/input"
 for type in ${FAT_SPC_TYPES:-12 16}; do
  img=$w/fat$type-c$spc.img
  sectors=2880; [ "$type" = 12 ] || sectors=$((8192*spc))
  truncate -s 0 "$img"; truncate -s $((sectors*512)) "$img"
  mformat -i "$img" -T "$sectors" -h 16 -s 63 -c "$spc" -N 0x19870330 ::
  mcopy -i "$img" "$w/input" ::CHAIN.BIN
  mcopy -i "$img" ::CHAIN.BIN - > "$w/mcopy"
  "$tool" "$img" "$w/mcopy" "$spc" "$type" || red "FAT$type spc=$spc driver"
  for name in CHAIN.BIN; do
   build/fat_dump "$img" --cat "$name" > "$w/ours"
   python3 harness/diff/fat_diff/fat${type}_ref.py "$img" --cat "$name" > "$w/python"
   cmp "$w/ours" "$w/mcopy" || red 'read vs mcopy'
   cmp "$w/ours" "$w/python" || red 'read vs python'
  done
  if [ "$type" = 12 ]; then
   cp -f "$w/input" "$w/expected"
   dd if="$w/input" of="$w/expected" bs=1 skip=11 seek=$((bpc-7)) count=1031 conv=notrunc status=none
   truncate -s $((bpc+13)) "$w/expected"
   dd if="$w/input" of="$w/expected" bs=1 seek=$((2*bpc+29)) count=777 conv=notrunc status=none
   for name in WRITE.BIN PART.BIN; do
    ref=$w/input; [ "$name" = WRITE.BIN ] || ref=$w/expected
    mcopy -i "$img" "::$name" - > "$w/mcopy"
    python3 harness/diff/fat_diff/fat12_ref.py "$img" --cat "$name" > "$w/python"
    cmp "$w/mcopy" "$ref" || red "$name vs mcopy"
    cmp "$w/python" "$ref" || red "$name vs python"
   done
   mdir -i "$img" ::GROW > "$w/dir"
   grep -q CHILD "$w/dir" || red 'grown directory CHILD vs mdir'
   sort "$img.list" > "$w/ours-dir"
   python3 harness/diff/fat_diff/fat12_ref.py "$img" --list-path GROW > "$w/python-dir"
   mdir -i "$img" ::GROW | awk '$1 ~ /^F[0-9]+$/ {print $1 " " $2}' | sort > "$w/mtools-dir"
   cmp "$w/ours-dir" "$w/python-dir" || red 'grown directory vs python'
   cmp "$w/ours-dir" "$w/mtools-dir" || red 'grown directory vs mdir'
   # mtools traverses the new directory and its parent links independently.
   mdir -i "$img" ::GROW/CHILD/.. > "$w/parent"
   grep -q CHILD "$w/parent" || red 'nested parent vs mdir'
  fi
  fsck.fat -n "$img" > "$w/fsck" 2>&1 || red "FAT$type spc=$spc fsck"
  echo ">>> test-fat-spc: green (FAT$type spc=$spc: mcopy/python bytes, fsck clean)"
 done
done

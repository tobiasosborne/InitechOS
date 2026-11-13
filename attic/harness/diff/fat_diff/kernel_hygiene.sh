#!/bin/sh
# Law 1: independent mtools operations on IDENTICAL starting images.
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1
mode=$1; build=$2; binary=$3
work=$build/khygiene-$mode
mkdir -p "$work"
red() { echo "RED kernel-hygiene $*"; exit 1; }
dd if=/dev/zero of="$work/seed.img" bs=512 count=2880 status=none
mformat -i "$work/seed.img" -f 1440 -N 12345678 -v KHYGIENE ::
mmd -i "$work/seed.img" ::OLD ::OLD/CHILD
printf 'preserved\r\n' > "$work/note.txt"
mcopy -i "$work/seed.img" "$work/note.txt" ::OLD/CHILD/NOTE.TXT
: > "$work/empty"
if [ "$mode" = grow ]; then
    # Leave exactly one cluster for GROW at the end of the volume.
    free=$(mdir -i "$work/seed.img" :: | awk '/bytes free/{gsub(/[^0-9]/,"",$0); print $0}')
    dd if=/dev/zero of="$work/fill" bs=512 count=$((free/512-1)) status=none
    mcopy -i "$work/seed.img" "$work/fill" ::FILL.BIN
    mmd -i "$work/seed.img" ::GROW
    mdel -i "$work/seed.img" ::FILL.BIN
fi
cp -f "$work/seed.img" "$work/art.img"
cp -f "$work/seed.img" "$work/ref.img"
case $mode in
rename) mren -i "$work/ref.img" ::OLD ::NEW;;
hidden) mmd -i "$work/ref.img" ::TRASH; mattrib -i "$work/ref.img" +h ::TRASH;;
grow) i=0; while [ "$i" -lt 15 ]; do
    name=$(printf 'F%02d.TXT' "$i")
    mcopy -i "$work/ref.img" "$work/empty" "::GROW/$name"
    i=$((i+1))
done;;
*) exit 2;;
esac
"$binary" "$mode" "$work/art.img" || red "$mode artifact assertions"
for side in art ref; do
    mdir -a -b -s -i "$work/$side.img" :: | sort > "$work/$side.list"
    fsck.fat -n "$work/$side.img" > "$work/$side.fsck" || red "$mode fsck $side"
done
cmp "$work/art.list" "$work/ref.list" || red "$mode mdir differential"
if [ "$mode" = hidden ]; then
    for side in art ref; do
        mattrib -i "$work/$side.img" ::TRASH | sed 's/::.*//' > "$work/$side.attr"
    done
    cmp "$work/art.attr" "$work/ref.attr" || red 'hidden mattrib differential'
fi
if [ "$mode" = rename ]; then
    for side in art ref; do
        mcopy -i "$work/$side.img" ::NEW/CHILD/NOTE.TXT "$work/$side.note"
        python3 harness/diff/fat_diff/fat12_ref.py "$work/$side.img" --dotdot 'NEW\CHILD' > "$work/$side.dotdot"
    done
    cmp "$work/art.note" "$work/ref.note" || red 'renamed child bytes'
    cmp "$work/art.dotdot" "$work/ref.dotdot" || red 'renamed child parent'
fi
echo "GREEN test-fat-hygiene-$mode: mtools identical-image differential + fsck.fat"

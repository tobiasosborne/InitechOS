#!/bin/sh
# initech-e9sx independent mtools / dosfstools differential on ONE image.
set -eu
image=$1; binary=$2; build=$3; tag=$4
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
work="$build/command-file/$tag"
mkdir -p "$work"
"$build/command_file" "$image" "$binary" check
mdir -i "$image@@32256" :: > "$work/mdir.txt"
grep -Eq '^COMMAND +COM ' "$work/mdir.txt" || { echo 'RED command-file MDIR visible COMMAND.COM'; exit 1; }
mattrib -i "$image@@32256" ::COMMAND.COM > "$work/mattrib.txt"
grep -Eq '^ *A +::/COMMAND.COM *$' "$work/mattrib.txt" || { cat "$work/mattrib.txt"; echo 'RED command-file MATTRIB'; exit 1; }
mtype -i "$image@@32256" ::COMMAND.COM > "$work/mtype.bin"
cmp "$binary" "$work/mtype.bin"
dd if="$image" of="$work/volume.img" bs=512 skip=63 count=2880 status=none
fsck.fat -n "$work/volume.img" > "$work/fsck.log" 2>&1
echo "GREEN test-command-file $tag: mdir + mattrib archive-only + mtype/cmp + artifact FAT reader + fsck.fat clean"

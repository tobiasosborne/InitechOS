#!/bin/sh
# The C: system disk (delivery owner, initech-tlhqy): the installed system and
# working volume on one fixed disk. 20160 sectors = 20 complete 16x63 CHS
# cylinders; MBR + stage2; one FAT12 partition at LBA 63, 16384 sectors
# (8 MiB), 8 sectors per cluster; IO.SYS and INITDOS.SYS are root entries 0
# and 1 (HSR, as SYS copies them: MS-DOS 3.3 User's Guide p119), then the
# label, then the working volume from TREE (a staged, date-normalised tree).
# Usage: mk_system_c.sh OUT MBR STAGE2 IO.SYS INITDOS.SYS LAYOUT TREE
set -eu
out=$1; mbr=$2; loader=$3; io=$4; kernel=$5; layout=$6; tree=$7
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
work=$(mktemp -d "${out}.mint.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
image=$work/image
dd if=/dev/zero of="$image" bs=512 count=20160 status=none
dd if="$mbr" of="$image" conv=notrunc status=none
dd if="$loader" of="$image" bs=512 seek=1 conv=notrunc status=none
"$layout" "$image" partition-c
volume="$image@@32256"
mformat -i "$volume" -T 16384 -h 16 -s 63 -c 8 -H 63 -N 0x494e4943 ::
cp -f "$io" "$work/IO.SYS"
cp -f "$kernel" "$work/INITDOS.SYS"
touch -d '@315532800' "$work/IO.SYS" "$work/INITDOS.SYS"
mcopy -m -i "$volume" "$work/IO.SYS" ::IO.SYS
mcopy -m -i "$volume" "$work/INITDOS.SYS" ::INITDOS.SYS
mattrib -i "$volume" +h +s +r -a ::IO.SYS ::INITDOS.SYS
mlabel -i "$volume" ::INITECH
sh "$(dirname "$0")/copy-tree.sh" "$tree" "$volume"
"$layout" "$image" check >/dev/null
mv -f "$image" "$out"
printf '>>> C: system disk: %s (FAT12 8 MiB spc8 @63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)\n' "$out"

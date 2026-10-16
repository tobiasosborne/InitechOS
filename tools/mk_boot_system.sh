#!/bin/sh
# Reproducible two-file FAT12 BOOT volume, initech-kod.
# Ref: ADR-0003 DEC-01/07/08; IBM DOS 3.30 TR p1-7,5-4,5-11;
# MS-DOS 3.3 User's Guide p119 SYS: copy IO first, then the DOS kernel.
set -eu
out=$1; mbr=$2; loader=$3; io=$4; kernel=$5; layout=$6; command=${7:-}
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
work=$(mktemp -d "${out}.mint.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
for t in mformat mcopy mattrib mlabel; do command -v "$t" >/dev/null; done
# 46 complete 2x32 CHS cylinders; partition [63,2943), one trailing zero.
dd if=/dev/zero of="$out" bs=512 count=2944 status=none
dd if="$mbr" of="$out" conv=notrunc status=none
dd if="$loader" of="$out" bs=512 seek=1 conv=notrunc status=none
"$layout" "$out" partition
volume="$out@@32256"
mformat -i "$volume" -T 2880 -h 2 -s 32 -c 1 -H 63 -N 0x4b4f4433 ::
cp -f "$io" "$work/IO.SYS"
cp -f "$kernel" "$work/INITDOS.SYS"
touch -d '@315532800' "$work/IO.SYS" "$work/INITDOS.SYS"
mcopy -m -i "$volume" "$work/IO.SYS" ::IO.SYS
mcopy -m -i "$volume" "$work/INITDOS.SYS" ::INITDOS.SYS
mattrib -i "$volume" +h +s +r -a ::IO.SYS ::INITDOS.SYS
# initech-e9sx, MS-DOS 3.3 User's Guide pp28/119 (PDF41/204): COMMAND.COM
# is an ordinary visible file, copied separately from the two hidden SYS files.
# -m preserves the fixed epoch rather than the linker's host modification time.
if [ -n "$command" ]; then
 cp -f "$command" "$work/COMMAND.COM"
 touch -d '@315532800' "$work/COMMAND.COM"
 mcopy -m -i "$volume" "$work/COMMAND.COM" ::COMMAND.COM
 mattrib -i "$volume" +a -h -s -r ::COMMAND.COM
fi
# A label consumes a root entry: add it only AFTER both system files.
mlabel -i "$volume" ::BOOT
printf '>>> boot-system image: %s (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)\n' "$out"

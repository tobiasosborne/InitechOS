#!/bin/sh
# Reproducible two-file FAT12 BOOT volume, initech-kod.
# Ref: ADR-0003 DEC-01/07/08; IBM DOS 3.30 TR p1-7,5-4,5-11;
# MS-DOS 3.3 User's Guide p119 SYS: copy IO first, then the DOS kernel.
set -eu
out=$1; mbr=$2; loader=$3; io=$4; kernel=$5; layout=$6; command=${7:-}; overlay=${8:-}
# ADR-0014 DEC-14-4: a FLAIR tenant kernel (it carries the FLAIROVL magic it
# checks) boots only beside C:\FLAIR.OVL linked against it, and only after
# tools/flairovl.c patched its expect slot (the "OVLCRC??" placeholder gone).
# Refuse either mismatch here, at image build, not as a boot-time MISSING.
if LC_ALL=C grep -qa 'OVLCRC??' "$kernel"; then
 echo "!!! mk_boot_system: $kernel is an UNPATCHED overlay kernel -- install build/flairovl_<kernel>.sys with its .ovl" >&2; exit 1
fi
if LC_ALL=C grep -qa 'FLAIROVL' "$kernel"; then
 [ -n "$overlay" ] || { echo "!!! mk_boot_system: $kernel loads C:\\FLAIR.OVL but no overlay was given" >&2; exit 1; }
elif [ -n "$overlay" ]; then
 echo "!!! mk_boot_system: an overlay was given for $kernel, which has no overlay manager" >&2; exit 1
fi
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
# ADR-0014 DEC-14-4: C:\FLAIR.OVL beside COMMAND.COM -- visible 8.3 file,
# archive attribute only, directory date 1980-01-01 00:00 (Rule 11), copied
# separately from SYS's hidden files (SYS / FORMAT /S never copy it, DEC-14-7).
if [ -n "$overlay" ]; then
 cp -f "$overlay" "$work/FLAIR.OVL"
 touch -d '@315532800' "$work/FLAIR.OVL"
 mcopy -m -i "$volume" "$work/FLAIR.OVL" ::FLAIR.OVL
 mattrib -i "$volume" +a -h -s -r ::FLAIR.OVL
fi
# A label consumes a root entry: add it only AFTER both system files.
mlabel -i "$volume" ::BOOT
printf '>>> boot-system image: %s (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)\n' "$out"

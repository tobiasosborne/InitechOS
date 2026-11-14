#!/bin/sh
# GM2 (operator 2026-11-14): immutable FAT16 import media. mtools creates
# the foreign filesystem; a writable adapter exposes any bypass attempt.
set -eu
w=build/fat16-utilities
mkdir -p "$w"
truncate -s $((16384*512)) "$w/fat16.img"
mformat -i "$w/fat16.img" -T 16384 -c 1 -h 2 -s 32 -N 0x494e4954 -v IMPORT ::
printf 'FAT16 import bytes\r\n' > "$w/keep"
mcopy -i "$w/fat16.img" "$w/keep" ::KEEP.TXT
# Usable FAT12 system source with independently supplied system contents.
truncate -s $((2943*512)) "$w/master.img"
printf '\200\000\001\000\001\377\377\377\077\000\000\000\100\013\000\000' | dd of="$w/master.img" bs=1 seek=446 conv=notrunc status=none
printf '\125\252' | dd of="$w/master.img" bs=1 seek=510 conv=notrunc status=none
mformat -i "$w/master.img@@32256" -f 1440 -H 63 -N 0x494e4954 ::
for f in IO.SYS INITDOS.SYS COMMAND.COM; do mcopy -i "$w/master.img@@32256" "$w/keep" "::$f"; done
bad=0
for kind in 0 1 2 3; do
 cp "$w/fat16.img" "$w/target.img"
 DISKUTIL_REPORT_ONLY=1 DU_CURRENT_C=1 build/formatsys_host "$w/master.img" "$w/target.img" "$kind" 'A:' '
N' 0 > "$w/$kind.out" 2> "$w/$kind.status"
 if ! grep -qFx 'Write protect error writing drive A:' "$w/$kind.out" || ! cmp -s "$w/fat16.img" "$w/target.img" || ! grep -q 'writes=0' "$w/$kind.status"; then
  echo "FAIL FAT16 utility kind=$kind: refusal, zero write calls, whole-image identity required"; bad=$((bad+1))
 else echo "GREEN FAT16 utility kind=$kind: refusal, zero writes, whole-image identity"; fi
done
# Blank large fixed media must not be turned into FAT16 by FORMAT.
truncate -s $((40000*512)) "$w/master.img"
printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=16384,type=4\n' | sfdisk --no-reread --no-tell-kernel "$w/master.img" >/dev/null 2>&1
cp "$w/master.img" "$w/before.img"
DISKUTIL_REPORT_ONLY=1 DU_CURRENT_C=1 build/formatsys_host "$w/master.img" "$w/target.img" 0 'D:' 'Y
' 0 > "$w/layout.out" 2> "$w/layout.status"
if ! grep -qFx 'Write protect error writing drive D:' "$w/layout.out" || ! cmp -s "$w/before.img" "$w/master.img" || ! grep -q 'writes=0' "$w/layout.status"; then
 echo 'FAIL FAT16 proposed layout: refusal before writes required'; bad=$((bad+1))
else echo 'GREEN FAT16 proposed layout: refused before writes'; fi
for spec in 'label|A: CHANGED' 'chkdsk|A: /F'; do
 tool=${spec%%|*};tail=${spec#*|}
 cp "$w/fat16.img" "$w/target.img"
 build/fat16_host "$w/target.img" "$tool" "$tail" > "$w/$tool.out" 2> "$w/$tool.status"
 if ! grep -qFx "$(printf 'Write protect error writing drive A:\r')" "$w/$tool.out" || ! cmp -s "$w/fat16.img" "$w/target.img" || ! grep -q 'writes=0' "$w/$tool.status"; then
  echo "FAIL FAT16 $tool: refusal, zero writes, whole-image identity required";bad=$((bad+1))
 else echo "GREEN FAT16 $tool: refused before writes";fi
done
# FDISK removes a mounted FAT16 entry, then reassigns its active status.
cp "$w/master.img" "$w/partition.img"
dd if="$w/fat16.img" of="$w/partition.img" bs=512 seek=63 conv=notrunc status=none
# The fixture maps primary 1 as D:, irrespective of the on-disk type byte.
for keys in "$(printf '3\r1\rY\r\033\033')" "$(printf '2\r2\r\033\033')"; do
 cp "$w/partition.img" "$w/target.img"
 build/fat16_host "$w/target.img" fdisk "$keys" > "$w/fdisk.out" 2> "$w/fdisk.status"
 if ! grep -qFx "$(printf 'Write protect error writing drive D:\r')" "$w/fdisk.out" || ! cmp -s "$w/partition.img" "$w/target.img" || ! grep -q 'writes=0' "$w/fdisk.status"; then
  echo 'FAIL FAT16 FDISK: mounted extent changed or missing refusal';bad=$((bad+1))
 else echo 'GREEN FAT16 FDISK: mounted extent protected before MBR write';fi
done
echo "INITECH_CHECKS 9 failures=$bad"
[ "$bad" -eq 0 ]

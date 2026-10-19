#!/bin/sh
# Identical-image MBR differential, mpartition plus util-linux sfdisk.
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1
build=$1; tool=$2
w=$build/fdisk
mkdir -p "$w"
red() { echo "RED fdisk $*"; exit 1; }
for t in mpartition sfdisk mformat fsck.fat; do command -v "$t" >/dev/null; done
for cyl in 4 8 65 66 513; do
 n=$((cyl*1008))
 truncate -s 0 "$w/ref.img"; truncate -s $((n*512)) "$w/ref.img"
 cp -f "$w/ref.img" "$w/ours.img"
 printf 'drive z: file="%s/%s/ref.img" partition=1\n' "$PWD" "$w" > "$w/mtools.conf"
 MTOOLSRC="$PWD/$w/mtools.conf" mpartition -I z:
 MTOOLSRC="$PWD/$w/mtools.conf" mpartition -c -a -b 63 -l $((n-63)) -s 63 -h 16 -f z:
 "$tool" create "$w/ours.img" || red CREATE
 dd if="$w/ref.img" of="$w/ref.table" bs=1 skip=446 count=66 status=none
 dd if="$w/ours.img" of="$w/ours.table" bs=1 skip=446 count=66 status=none
 cmp "$w/ref.table" "$w/ours.table" || red 'CHS/relative-sector/type/signature vs mpartition'
 sfdisk --dump "$w/ref.img" | sed "s,$w/ref.img,DISK,g" > "$w/ref.dump"
 sfdisk --dump "$w/ours.img" | sed "s,$w/ours.img,DISK,g" > "$w/ours.dump"
 cmp "$w/ref.dump" "$w/ours.dump" || red 'sfdisk dump'
 echo "GREEN fdisk create $cyl cylinders: MBR table+55AA == mpartition, sfdisk dump equal"
 # Format the full independently partitioned extent with mtools' native
 # allocation-unit choice (IBM DOS 3.30 TR pp2-31,4-17,5-8).
 volume_sectors=$((n-63))
 printf 'FDISK partition mount oracle\r\n' > "$w/input.txt"
 for image in ref ours; do
  mformat -i "$w/$image.img@@32256" -T "$volume_sectors" -h 16 -s 63 -r 32 -H 63 -N 0x46444953 ::
  spc=$(od -An -tu1 -j 32269 -N 1 "$w/$image.img" | tr -d '[:space:]')
  mcopy -i "$w/$image.img@@32256" "$w/input.txt" ::INPUT.TXT
 done
 "$build/fat_dump" "$w/ours.img" --cat INPUT.TXT > "$w/kernel.txt" || red 'kernel partition mount'
 mcopy -i "$w/ref.img@@32256" ::INPUT.TXT - > "$w/reference.txt"
 cmp "$w/kernel.txt" "$w/reference.txt" || red 'kernel bytes vs mtools'
 dd if="$w/ours.img" of="$w/volume.img" bs=512 skip=63 count="$volume_sectors" status=none
 fsck.fat -n "$w/volume.img" > "$w/fsck.log" 2>&1 || red 'dosfstools volume validation'
 echo "GREEN fdisk FAT on $cyl-cylinder disk ($volume_sectors sectors, spc=$spc): kernel file bytes == mcopy; fsck.fat clean"
done
# Independent two-entry non-DOS + DOS layout exercises active entry selection.
truncate -s 0 "$w/two.img"; truncate -s $((32*1008*512)) "$w/two.img"
printf 'label: dos\nunit: sectors\nstart=63,size=8001,type=1,bootable\nstart=8064,size=8064,type=4\n' | sfdisk --no-reread --no-tell-kernel "$w/two.img" > "$w/sfdisk.log" 2>&1
cp -f "$w/two.img" "$w/ref.img";cp -f "$w/two.img" "$w/ours.img"
sfdisk --activate "$w/ref.img" 2 > "$w/active.log" 2>&1
"$tool" active "$w/ours.img" || red ACTIVE
cmp "$w/ref.img" "$w/ours.img" || red 'active flag on wrong entry'
cp -f "$w/ours.img" "$w/before.img"
"$tool" delete-N "$w/ours.img" || red DELETE
cmp "$w/before.img" "$w/ours.img" || red 'delete without confirmation'
"$tool" delete-Y "$w/ours.img" || red DELETE
sfdisk --delete "$w/ref.img" 1 > "$w/delete.log" 2>&1
cmp "$w/ref.img" "$w/ours.img" || red 'confirmed delete table vs sfdisk'
echo '>>> test-fdisk: green (create/type/CHS/signature, native cluster sizes, active entry, default-No and confirmed deletion differential)'

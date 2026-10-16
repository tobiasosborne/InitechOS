#!/bin/sh
# Menus driven by injected keys; pixels decoded by harness/console_text.c.
# Only SCRATCH copies of both disks. The flagship layout is never altered.
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1
build=$1; boot=$2; com=$3; only=${4:-all}
w=$build/fdisk-emu
mkdir -p "$w"
red() { echo "RED fdisk-emu $*"; exit 1; }
prepare() {
 cp -f "$boot" "$w/boot.img"
 truncate -s $((8064*512)) "$w/boot.img"
 cp -f "$w/boot.img" "$w/before.img"
 dd if=/dev/zero of="$w/before.img" bs=1 seek=446 count=64 conv=notrunc status=none
 printf 'drive z: file="%s/%s/before.img" partition=1\n' "$PWD" "$w" > "$w/mtools.conf"
 if [ "$1" != create ]; then
  MTOOLSRC="$PWD/$w/mtools.conf" mpartition -c -a -b 63 -l 8001 -s 63 -h 16 -f z:
 fi
 dd if=/dev/zero of="$w/data.img" bs=512 count=2880 status=none
 mformat -i "$w/data.img" -f 1440 -N 0x46444953 ::
 printf '@ECHO OFF\r\nFDSETUP\r\nFDISK\r\nECHO FDISK-RETURN\r\n' > "$w/autoexec.bat"
 mcopy -i "$w/data.img" "$w/autoexec.bat" ::AUTOEXEC.BAT
 mcopy -i "$w/data.img" "$com" ::FDISK.COM
 mcopy -i "$w/data.img" "$build/fdisk_fixture.bin" ::FDSETUP.COM
}
run() {
 name=$1;keys=$2;mark=$3
 dd if="$w/before.img" of="$w/prep.bin" bs=512 count=1 status=none
 mcopy -o -i "$w/data.img" "$w/prep.bin" ::PREP.BIN
 cp -f "$w/data.img" "$w/data-before.img"
 "$build/qemu_harness" --disk "$w/boot.img" --disk2 "$w/data.img" --name "$name" --out "$w" \
  --timeout-ms 20000 --keys "$keys" --keys-after 'Enter choice:' \
  --quit-after "$mark" --screendump --screendump-after "$mark" \
  > "$w/$name.stdout" 2> "$w/$name.report" || true
 ! grep -q 'triple_fault=1' "$w/$name.report" || red TRIPLE-FAULT
 if [ "$name" = cancel ]; then
  cmp "$w/before.img" "$w/boot.img" || red 'delete without confirmation'
 fi
 if [ "$name" = loaded ]; then
  cmp "$w/data-before.img" "$w/data.img" || red 'loaded-volume disk changed'
  grep -q 'quit_after_found=1' "$w/$name.report" || red 'loaded volume was not refused'
 fi
 grep -q 'quit_after_found=1' "$w/$name.report" || red "$name did not reach manual prompt"
 "$build/console_text" /usr/share/seabios/vgabios-stdvga.bin "$w/$name.ppm" > "$w/$name.screen" || red "$name console pixels"
}
if [ "$only" = all ] || [ "$only" = SIGNATURE ] || [ "$only" = CHS ]; then
 prepare create
 run create '1,ret,1,ret,ret' 'Press any key when ready . . .'
 cp -f "$w/before.img" "$w/ref.img"
 printf 'drive z: file="%s/%s/ref.img" partition=1\n' "$PWD" "$w" > "$w/mtools.conf"
 MTOOLSRC="$PWD/$w/mtools.conf" mpartition -c -a -b 63 -l 8001 -s 63 -h 16 -f z:
 dd if="$w/ref.img" of="$w/ref.table" bs=1 skip=446 count=66 status=none
 dd if="$w/boot.img" of="$w/ours.table" bs=1 skip=446 count=66 status=none
 cmp "$w/ref.table" "$w/ours.table" || red 'CHS/signature create MBR vs mpartition'
 "$build/fat_dump" "$w/boot.img" --list > "$w/mounted.list" || red 'kernel partition mount rejected created table'
 grep -q '^IO.SYS ' "$w/mounted.list" || red 'kernel mount did not read IO.SYS'
 echo 'GREEN fdisk QEMU create: injected menus, MBR == mpartition, kernel FAT mount accepts result'
fi
if [ "$only" = all ]; then
 prepare create
 run custom '1,ret,1,ret,n,ret,2,ret,esc,esc' 'Press any key when ready . . .'
 cp -f "$w/before.img" "$w/ref.img"
 printf 'drive z: file="%s/%s/ref.img" partition=1\n' "$PWD" "$w" > "$w/mtools.conf"
 MTOOLSRC="$PWD/$w/mtools.conf" mpartition -c -b 63 -l 1953 -s 63 -h 16 -f z:
 # MS-DOS UG p289: custom size does not select the maximum-size-and-active
 # choice. mpartition auto-activates a first primary, so request inactive
 # explicitly rather than treating its default as the DOS menu golden.
 MTOOLSRC="$PWD/$w/mtools.conf" mpartition -d z:
 dd if="$w/ref.img" of="$w/ref.table" bs=1 skip=446 count=66 status=none
 dd if="$w/boot.img" of="$w/ours.table" bs=1 skip=446 count=66 status=none
 cmp "$w/ref.table" "$w/ours.table" || red 'custom cylinder size/default inactive'
 echo 'GREEN fdisk QEMU custom: N + 2 cylinders == mpartition, inactive until selected'
 prepare display
 run display '4,ret' 'Press ESC to return to Fdisk Options'
 sed -n '/^Display Partition Information$/,/^Press ESC to return to Fdisk Options$/p' "$w/display.screen" > "$w/display.txt"
 cmp harness/diff/fat_diff/fixtures/fdisk_display.txt "$w/display.txt" || red 'display manual columns/text'
 echo 'GREEN fdisk QEMU display: VGA screendump text/columns == manual p295 transcription'
fi
if [ "$only" = all ] || [ "$only" = ACTIVE ]; then
 prepare active
 printf 'label: dos\nunit: sectors\nstart=63,size=1953,type=1,bootable\nstart=2016,size=1008,type=4\n' | sfdisk --no-reread --no-tell-kernel "$w/before.img" > "$w/sfdisk.log" 2>&1
 cp -f "$w/before.img" "$w/ref.img"
 sfdisk --activate "$w/ref.img" 2 > "$w/active.log" 2>&1
 run active '2,ret,2,ret,esc,esc' 'Press any key when ready . . .'
 cmp "$w/ref.img" "$w/boot.img" || red 'active flag on wrong entry'
 echo 'GREEN fdisk QEMU active: injected partition 2 == sfdisk --activate 2'
fi
if [ "$only" = all ] || [ "$only" = CONFIRM ]; then
 prepare cancel
 run cancel '3,ret,1,ret,ret,esc,esc' FDISK-RETURN
 cmp "$w/before.img" "$w/boot.img" || red 'delete without confirmation'
 echo 'GREEN fdisk QEMU delete: default-No leaves every disk byte intact'
 if [ "$only" = all ]; then
  prepare delete
  cp -f "$w/before.img" "$w/ref.img"
  sfdisk --delete "$w/ref.img" 1 > "$w/delete.log" 2>&1
  run delete '3,ret,1,ret,y,ret,esc,esc' 'Press any key when ready . . .'
  cmp "$w/ref.img" "$w/boot.img" || red 'confirmed delete'
  echo 'GREEN fdisk QEMU delete: explicit Y/Return == sfdisk --delete 1'
 fi
fi
if [ "$only" = all ] || [ "$only" = LOADED ]; then
 prepare loaded
 run loaded '5,ret' FDISK-RETURN
 echo 'GREEN fdisk QEMU loaded-volume refusal: option 5 exits, data volume byte-identical'
fi
echo 'GREEN test-fdisk-emu'

#!/bin/sh
# Injected manual prompts + console_text pixels and mtools on scratch disks.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800
b=$1; boot=$2; format=$3; only=${4:-all}; w=$b/formatsys-emu
mkdir -p "$w"
red(){ echo "RED formatsys-emu $*"; exit 1; }
prepare(){
 cp -f "$boot" "$w/master.img"; truncate -s $((8064*512)) "$w/master.img"
 mcopy -o -i "$w/master.img@@32256" "$b/transient_flairtenants.bin" ::COMMAND.COM
 printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=2880,type=1\n' | sfdisk --no-reread --no-tell-kernel "$w/master.img" > "$w/sfdisk.log" 2>&1
 dd if=/dev/zero of="$w/data.img" bs=512 count=2880 status=none
 mformat -i "$w/data.img" -f 1440 -N 0x494e4954 ::
 for pair in "$format:FORMAT" "$b/sys_program.bin:SYS" "$b/diskcopy_program.bin:DISKCOPY" "$b/diskcomp_program.bin:DISKCOMP"; do
  mcopy -i "$w/data.img" "${pair%:*}" "::${pair#*:}.COM"
 done
 printf '@ECHO OFF\r\n%s\r\nECHO DU-RETURN\r\n' "$1" > "$w/autoexec.bat"
 mcopy -i "$w/data.img" "$w/autoexec.bat" ::AUTOEXEC.BAT
 cp -f "$w/data.img" "$w/data-before.img"
}
run(){
 "$b/qemu_harness" --disk "$w/master.img" --disk2 "$w/data.img" --out "$w" --name "$1" \
 --mouse "$2" --keys-after "$3" --quit-after "$4" --screendump --screendump-after "$4" --timeout-ms 25000 > "$w/$1.stdout" 2> "$w/$1.report" || true
 ! grep -q 'triple_fault=1' "$w/$1.report" || red triple-fault
 grep -q 'quit_after_found=1' "$w/$1.report" || red "$1 prompt"
 "$b/console_text" /usr/share/seabios/vgabios-stdvga.bin "$w/$1.ppm" > "$w/$1.screen" || red pixels
}
if [ "$only" != RUNNING ]; then
 prepare 'FORMAT D: /S /V'
 run format 'ky,kret,WVolume label,kshift-o,kshift-f,kshift-f,kshift-i,kshift-c,kshift-e,kret' 'Proceed with Format' DU-RETURN
 dd if="$w/master.img" of="$w/formatted.img" bs=512 skip=4032 count=2880 status=none
 cp -f "$w/formatted.img" "$w/system.img"
 # Independent reference boot/BPB produced by mformat, not our formatter.
 dd if=/dev/zero of="$w/ref.img" bs=512 count=2880 status=none
 mformat -i "$w/ref.img" -T 2880 -h 16 -s 63 -c 1 -r 14 -H 4032 -N 0x4b4f4433 ::
 printf '\370' | dd of="$w/ref.img" bs=1 seek=21 conv=notrunc status=none
 for off in 512 5120; do printf '\370' | dd of="$w/ref.img" bs=1 seek=$off conv=notrunc status=none; done
 "$b/formatsys_check" "$w/formatted.img" "$w/ref.img" system "$boot"
 for f in IO.SYS INITDOS.SYS COMMAND.COM; do
  mcopy -i "$w/master.img@@32256" "::$f" - > "$w/ref-$f"
  mcopy -i "$w/formatted.img" "::$f" - > "$w/ours-$f"
  cmp "$w/ref-$f" "$w/ours-$f" || red 'system bytes'
 done
 fsck.fat -n "$w/formatted.img" > "$w/fsck" 2>&1 || red filesystem
 # Whole console body, not just substring presence: the words/layout are a
 # manual transcription; numeric totals come from mtools and source sizes.
 total=$(mdir -i "$w/ref.img" :: | sed -n '/bytes free/s/[^0-9]//gp')
 used=0
 for f in IO.SYS INITDOS.SYS COMMAND.COM; do
  bytes=$(wc -c < "$w/ref-$f"); used=$((used+(bytes+511)/512*512))
 done
 cat > "$w/format.want" <<END
WARNING, ALL DATA ON NON-REMOVABLE DISK
DRIVE D: WILL BE LOST!
Proceed with Format (Y/N)?y
Formatting...
Format complete
System transferred
Volume label (11 characters, ENTER for none)?OFFICE
$total bytes total disk space
$used bytes used by system
$((total-used)) bytes available on disk
END
 awk '/^WARNING,/{p=1} /^DU-RETURN$/{exit} p{print}' "$w/format.screen" > "$w/format.got"
 cmp "$w/format.want" "$w/format.got" || red 'manual console layout'
 for text in 'Format complete' 'System transferred' 'Volume label (11 characters, ENTER for none)?' 'bytes total disk space' 'bytes used by system' 'bytes available on disk'; do
  grep -qF "$text" "$w/format.screen" || red "pixels $text"
 done
 if [ "$only" = boot ]; then
  # Existing stage2 boots first FAT partition only. Make the just-formatted
  # scratch D: the first entry, retaining its base/BPB and every volume byte.
  dd if="$w/master.img" of="$w/booted.img" bs=512 count=8064 status=none
  dd if="$w/master.img" of="$w/entry" bs=1 skip=462 count=16 status=none
  printf '\200' | dd of="$w/entry" bs=1 seek=0 conv=notrunc status=none
  dd if="$w/entry" of="$w/booted.img" bs=1 seek=446 conv=notrunc status=none
  dd if=/dev/zero of="$w/booted.img" bs=1 seek=462 count=48 conv=notrunc status=none
  printf '@ECHO OFF\r\n' | mcopy -o -i "$w/data.img" - ::AUTOEXEC.BAT
  "$b/qemu_harness" --disk "$w/booted.img" --disk2 "$w/data.img" --out "$w" --name boot \
   --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE --timeout-ms 20000 > "$w/boot.stdout" 2> "$w/boot.report" || true
  grep -q 'quit_after_found=1' "$w/boot.report" || red '/S did not boot'
  grep -qF 'A:\>' "$w/boot.serial" || red '/S missing A prompt'
  for marker in IOSYS-FOUND INITDOS-FOUND KERNEL SHELL-READY SHELL-EXIT; do
   grep -qFx "$marker" "$w/boot.serial" || red "/S missing $marker"
  done
  echo 'GREEN test-format-system-boot: FORMAT /S output boots existing MBR/stage2 to A:\> in QEMU'; exit 0
 fi
fi
if [ "$only" = all ]; then
 prepare 'FORMAT D:'
 run empty 'ky,kret' 'Proceed with Format' DU-RETURN
 printf '@ECHO OFF\r\nSYS D:\r\nECHO DU-RETURN\r\n' | mcopy -o -i "$w/data.img" - ::AUTOEXEC.BAT
 run sys '' 'System transferred' DU-RETURN
 dd if="$w/master.img" of="$w/sys.img" bs=512 skip=4032 count=2880 status=none
 "$b/formatsys_check" "$w/sys.img" "$w/ref.img" sys "$boot"
 fsck.fat -n "$w/sys.img" > "$w/sys-fsck" 2>&1 || red 'SYS filesystem'
 grep -qFx 'System transferred' "$w/sys.screen" || red 'SYS message pixels'
 prepare 'SYS D:'
 mformat -i "$w/master.img@@2064384" -T 2880 -c 1 -H 4032 ::
 printf occupied > "$w/occupied"
 mcopy -i "$w/master.img@@2064384" "$w/occupied" ::FIRST.TXT
 cp -f "$w/master.img" "$w/occupied-before.img"
 run occupied '' 'No room for system' DU-RETURN
 cmp "$w/master.img" "$w/occupied-before.img" || red 'SYS occupied disk changed'
 grep -qFx 'No room for system on destination disk' "$w/occupied.screen" || red 'SYS occupied pixels'
 prepare 'SYS B:'; run invalid '' 'Invalid drive specification' DU-RETURN
 grep -qFx 'Invalid drive specification' "$w/invalid.screen" || red 'invalid pixels'
 for cmd in DISKCOPY DISKCOMP; do
  prepare "$cmd D: A:"
  cp -f "$w/master.img" "$w/fixed-before.img"
  run "$cmd-fixed" '' 'Specified drive does not exist' DU-RETURN
  cmp "$w/fixed-before.img" "$w/master.img" || red "$cmd fixed write"
  grep -qFx 'Specified drive does not exist,' "$w/$cmd-fixed.screen" || red "$cmd fixed pixels"
  grep -qFx 'or is non-removable' "$w/$cmd-fixed.screen" || red "$cmd fixed pixels"
  prepare "$cmd B:"
  run "$cmd-invalid" '' 'Invalid drive specification' DU-RETURN
  grep -qFx 'Invalid drive specification' "$w/$cmd-invalid.screen" || red "$cmd invalid pixels"
 done
fi
if [ "$only" = all ] || [ "$only" = RUNNING ]; then
 prepare 'FORMAT A:'
 run running 'kret,kn' 'Format not supported' DU-RETURN
 cmp -s "$w/data-before.img" "$w/data.img" || red 'running volume not refused'
 grep -qFx 'Format not supported on drive A:' "$w/running.screen" || red 'running volume not refused'
fi
echo 'GREEN test-formatsys-emu: injected FORMAT /S /V and SYS, manual text decoded from pixels, images independently checked, running-volume refusal'

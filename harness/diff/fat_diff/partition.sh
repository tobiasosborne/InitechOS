#!/bin/sh
# Identical-image differential: real kernel FAT mount vs mtools/dosfstools.
# Ref: IBM DOS 3.30 TR pp2-31,6-24..25,9-3..9; ADR-0003 DEC-07a/DEC-15.
set -eu
kind=$1; build=$2; boot=${3:-}; mutant=${4:-}
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C INITECH_QMP_STDIO=1
work="$build/kzfs"
mkdir -p "$work"
for t in mpartition mformat mcopy mdir fsck.fat; do command -v "$t" >/dev/null; done
red() { echo "RED partition $*"; exit 1; }
printf 'Partition sector oracle\r\nSame DOS file bytes\r\n' > "$work/input.txt"
printf '@ECHO OFF\r\nECHO PART-DIR-BEGIN\r\nDIR\r\nECHO PART-DIR-END\r\nECHO PART-TYPE-BEGIN\r\nTYPE INPUT.TXT\r\nECHO PART-TYPE-END\r\nCOPY INPUT.TXT COPY.TXT\r\nPBASE\r\nCHKDSK /F\r\n' > "$work/autoexec.bat"
mint() {
 image=$1; base=$2; sectors=$3; bits=$4
 dd if=/dev/zero of="$image" bs=512 count=$((sectors+base)) status=none
 if [ "$base" != 0 ]; then
  printf 'drive z: file="%s/%s" partition=1 fat_bits=%s\n' "$PWD" "$image" "$bits" > "$work/mtools.conf"
  MTOOLSRC="$PWD/$work/mtools.conf" mpartition -I z:
  MTOOLSRC="$PWD/$work/mtools.conf" mpartition -c -a -b "$base" -l "$sectors" -s 63 -h 16 -f z:
 fi
 spec="$image@@$((base*512))"
 mformat -i "$spec" -T "$sectors" -h 16 -s 63 -c 1 -H "$base" -N 0x4b5a4653 -v KZFS ::
 mcopy -i "$spec" "$work/input.txt" ::INPUT.TXT
 # A second file and a multi-cluster read keep directory/content coverage real.
 dd if=/dev/zero of="$work/chain.bin" bs=512 count=5 status=none
 mcopy -i "$spec" "$work/chain.bin" ::CHAIN.BIN
}
mint "$work/super.img" 0 2880 12
mint "$work/partitioned.img" 63 2880 12
if [ "$kind" = mint ]; then echo 'GREEN partitioned-data: MBR + FAT12 at LBA 63'; exit 0; fi
extract() { dd if="$1" of="$2" bs=512 skip="$3" count=2880 status=none; }
check_host() {
 image=$1; base=$2; sectors=$3
 spec="$image@@$((base*512))"
 "$build/fat_dump" "$image" --list > "$work/ours.list" || red "MOUNT base=$base"
 mdir -b -i "$spec" :: | sed 's,::/,,' | sort > "$work/ref.names"
 awk '{print $1}' "$work/ours.list" > "$work/our.names"
 cmp "$work/our.names" "$work/ref.names" || red LIST
 while read -r name size; do
  "$build/fat_dump" "$image" --cat "$name" > "$work/ours.bin"
  mcopy -i "$spec" "::$name" - > "$work/ref.bin"
  cmp "$work/ours.bin" "$work/ref.bin" || red "READ $name"
  test "$(wc -c < "$work/ref.bin")" = "$size" || red "SIZE $name"
 done < "$work/ours.list"
 dd if="$image" of="$work/fsck.img" bs=512 skip="$base" count="$sectors" status=none
 fsck.fat -n "$work/fsck.img" > "$work/fsck.log" 2>&1 || { cat "$work/fsck.log"; red FSCK; }
 echo "GREEN partition host base=$base: directory names/sizes + every file byte == mtools; fsck.fat -n clean"
}
if [ "$kind" = host ]; then
 check_host "$work/super.img" 0 2880
 check_host "$work/partitioned.img" 63 2880
 "$build/test_partition" "$work/partitioned.img"
 mint "$work/partition16.img" 63 16384 16
 check_host "$work/partition16.img" 63 16384
 echo 'GREEN test-fat-partition'; exit 0
fi
run_emu() {
 image=$1; base=$2; name=$3
 spec="$image@@$((base*512))"
 mcopy -i "$spec" "$work/autoexec.bat" ::AUTOEXEC.BAT
 mcopy -i "$spec" "$build/partition_probe.bin" ::PBASE.COM
 mcopy -i "$spec" "$build/chkdsk_program.bin" ::CHKDSK.COM
 # A bad chain terminator must be repaired through INT 26h at the same base.
 # Both FAT copies carry the identical fault; dosfstools judges afterwards.
 "$build/test_partition" "$image" corrupt
 dd if="$image" of="$work/$name.prefix" bs=512 count="$base" status=none
 "$build/qemu_harness" --disk "$boot" --disk2 "$image" --name "$name" --out "$work" \
  --timeout-ms 25000 --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE \
  > "$work/$name.stdout" 2> "$work/$name.report" || true
 test -s "$work/$name.serial" || red SERIAL
 ! grep -q 'triple_fault=1' "$work/$name.report" || red TRIPLE-FAULT
 grep -qx 'SHELL-EXIT' "$work/$name.serial" || red SHELL
 grep -qx 'FAT-MOUNT-OK' "$work/$name.serial" || red MOUNT
 tr -d '\r' < "$work/$name.serial" > "$work/serial.txt"
 grep -qx 'PART-ABS-OK' "$work/serial.txt" || red ABS-BASE
 grep -qx 'Has invalid cluster, file truncated' "$work/serial.txt" || red CHKDSK-NOT-DRIVEN
 grep -q '^INPUT.*TXT' "$work/serial.txt" || red DIR
 awk '/^PART-TYPE-BEGIN$/{p=1;next}/^PART-TYPE-END$/{p=0}p' "$work/serial.txt" > "$work/type.txt"
 tr -d '\r' < "$work/input.txt" > "$work/expected.txt"
 # TYPE appends a newline after the file in this existing shell.
 sed '/^$/d' "$work/type.txt" > "$work/type-lines.txt"
 cmp "$work/expected.txt" "$work/type-lines.txt" || red TYPE
 mcopy -i "$spec" ::COPY.TXT - > "$work/copy.txt"
 cmp "$work/input.txt" "$work/copy.txt" || red COPY
 dd if="$image" of="$work/$name.after" bs=512 count="$base" status=none
 cmp "$work/$name.prefix" "$work/$name.after" || red PREFIX-CORRUPTION
 extract "$image" "$work/fsck.img" "$base"
 fsck.fat -n "$work/fsck.img" > "$work/fsck.log" 2>&1 || { cat "$work/fsck.log"; red CHKDSK-REPAIR; }
 echo "GREEN partition QEMU base=$base: DIR TYPE COPY INT25/26 CHKDSK /F, mcopy bytes, fsck.fat clean, prefix intact"
}
reject_emu() {
 rule=$1
 "$build/test_partition" "$work/partitioned.img" "$rule"
 "$build/qemu_harness" --disk "$boot" --disk2 "$work/partitioned.img" --name "reject-$rule" --out "$work" \
  --timeout-ms 15000 --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE \
  > "$work/reject-$rule.stdout" 2> "$work/reject-$rule.report" || true
 ! grep -q 'triple_fault=1' "$work/reject-$rule.report" || exit 2
 grep -qx 'SHELL-EXIT' "$work/reject-$rule.serial" || exit 2
 case "$rule" in
  EXTENDED) expected='FAT-MOUNT-FAIL rc=-20 base_lba=0 hidden_sectors=0' ;;
  SIGNATURE) expected='FAT-MOUNT-FAIL rc=-2 base_lba=0 hidden_sectors=0' ;;
  HIDDEN) expected='FAT-MOUNT-FAIL rc=-20 base_lba=63 hidden_sectors=0' ;;
 esac
 grep -qx "$expected" "$work/reject-$rule.serial" || red "$rule accepted invalid MBR"
 echo "GREEN partition QEMU rejection $rule: $expected"
}
if [ "$kind" = emu-mut ]; then
 case "$mutant" in
  IGNORE_BASE) run_emu "$work/partitioned.img" 63 "mut-$mutant" ;;
  EXTENDED|SIGNATURE) reject_emu "$mutant" ;;
 esac
 exit 0
fi
run_emu "$work/partitioned.img" 63 partitioned
run_emu "$work/super.img" 0 super
for rule in EXTENDED SIGNATURE HIDDEN; do
 mint "$work/partitioned.img" 63 2880 12
 reject_emu "$rule"
done
echo 'GREEN test-fat-partition-emu'

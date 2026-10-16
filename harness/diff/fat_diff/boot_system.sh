#!/bin/sh
# Oracle first, initech-kod. Manual sources: boot_system.c, boot_system.mk.
set -eu
kind=$1; build=$2; image=$3; tag=${4:-normal}; kernel=${5:-$build/kernel_shell_secondary.bin}
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C INITECH_QMP_STDIO=1
work="$build/boot-system"
mkdir -p "$work"
red() { echo "RED boot-system $*"; exit 1; }
if [ "$kind" = host ]; then
 "$build/boot_system" "$image" check
 mdir -a -i "$image@@32256" :: > "$work/root.txt"
 awk '$1=="IO" || $1=="INITDOS" {print $1}' "$work/root.txt" > "$work/names.txt"
 printf 'IO\nINITDOS\n' > "$work/names.expected"
 cmp "$work/names.expected" "$work/names.txt" || red MDIR
 for file in IO.SYS INITDOS.SYS; do
  mattrib -i "$image@@32256" "::$file" > "$work/$file.attrs"
  grep -Eq 'R.*S.*H|H.*S.*R|S.*H.*R' "$work/$file.attrs" || red "MTOOLS-HSR $file"
  mcopy -i "$image@@32256" "::$file" - > "$work/$file"
  expected="$build/$file"
  [ "$file" != INITDOS.SYS ] || expected="$kernel"
  cmp "$expected" "$work/$file" || red "BYTES $file"
 done
 dd if="$image" of="$work/volume.img" bs=512 skip=63 count=2880 status=none
 fsck.fat -n "$work/volume.img" > "$work/fsck.log" 2>&1 || { cat "$work/fsck.log"; red FSCK; }
 echo 'GREEN test-boot-system: mdir order + mattrib HSR + mcopy/cmp both files + fsck.fat clean'
 exit 0
fi
name="boot-system-$tag"
if [ "$kind" = emu ]; then
 "$build/qemu_harness" --disk "$image" --name "$name" --out "$work" --timeout-ms 12000 \
  --keys 'e,x,i,t,ret' --keys-after SHELL-READY --quit-after SHELL-DONE \
  > "$work/$name.stdout" 2> "$work/$name.report" || true
else
 # Observe the halt for the entire interval, with the error emitted once.
 "$build/qemu_harness" --disk "$image" --name "$name" --out "$work" --timeout-ms 2500 \
  > "$work/$name.stdout" 2> "$work/$name.report" || true
fi
grep -q 'triple_fault=0' "$work/$name.report" || red "$tag TRIPLE-FAULT/report"
grep -q 'launched=1' "$work/$name.report" || red "$tag not launched"
grep -q 'guest_errors=0' "$work/$name.report" || red "$tag guest errors"
serial="$work/$name.serial"
test -s "$serial" || red "$tag no serial"
if [ "$kind" = emu ]; then
 for mark in S1 JMP2 IOSYS-FOUND INITDOS-FOUND S2 KLOAD A20 GDT PM LFB OK KHI KERNEL SHELL-READY SHELL-EXIT; do
  grep -qx "$mark" "$serial" || red "$tag missing $mark"
 done
 grep -q '^IO-CLUSTERS=[0-9A-F]\{4\}$' "$serial" || red "$tag IO count"
 grep -qx 'INITDOS-CLUSTERS=0180' "$serial" || red "$tag INITDOS count"
 grep -q 'keys_sent=5' "$work/$name.report" || red "$tag no injected trace"
 grep -q 'quit_after_found=1' "$work/$name.report" || red "$tag shell did not complete"
 echo "GREEN test-boot-system-emu $tag: FAT-loaded IO.SYS + INITDOS.SYS -> KERNEL + SHELL-READY + injected EXIT; no triple fault"
else
 grep -q 'timed_out=1' "$work/$name.report" || red "$tag did not remain halted"
 tr -d '\r' < "$serial" > "$work/$name.error.txt"
 [ "$(grep -xc 'Non-System disk or disk error' "$work/$name.error.txt")" = 1 ] || red "$tag missing Non-System message"
 grep -qx 'Replace and strike any key when ready' "$work/$name.error.txt" || red "$tag replacement message"
 ! grep -Eq '^(PM|KLOAD|KERNEL|SHELL-READY)$' "$serial" || red "$tag entered kernel"
 echo "GREEN test-boot-system-negative $tag: exact Non-System text once, halted before PM/KERNEL, no triple fault"
fi

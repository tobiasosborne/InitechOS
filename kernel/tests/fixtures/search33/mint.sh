#!/bin/sh
# Writes disposable copies only. Media provenance/hash: DOS33 row in legacy
# docs/research/sidekick/PROVENANCE.md (Packard Bell MS-DOS 3.30 disk 1).
set -eu
media=$1
out=$2
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir -p "$out"
cp "$media" "$out/boot.img"
mdel -i "$out/boot.img" ::BACKUP.COM
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 -N 19920302 -v A5 ::
printf 'literal file\r\n' > "$out/file"
for n in 0 1 2 3; do mcopy -i "$out/data.img" "$out/file" "::A$n"; done
mattrib -i "$out/data.img" +h ::A1
mattrib -i "$out/data.img" +s ::A2
mattrib -i "$out/data.img" +h +s ::A3
mmd -i "$out/data.img" ::A4 ::A6
mattrib -i "$out/data.img" +h +s ::A6
nasm -f bin kernel/tests/fixtures/search33/probe.asm -o "$out/PROBE.COM"
printf '@ECHO OFF\r\nVER > A:\\VERSION.TXT\r\nPROBE\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/boot.img" "$out/PROBE.COM" "$out/AUTOEXEC.BAT" ::
rc=0
timeout 20 qemu-system-i386 -display none -monitor none -serial none -cpu 486 -m 8 -no-reboot -rtc base=1992-03-02T09:30:00 -drive "file=$out/boot.img,format=raw,if=floppy,index=0" -drive "file=$out/data.img,format=raw,if=floppy,index=1" -boot a -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$out/qemu.log" 2>&1 || rc=$?
test "$rc" = 33 || { cat "$out/qemu.log"; exit 1; }
mcopy -i "$out/boot.img" ::CAPTURE.BIN "$out/matrix.bin"
mcopy -i "$out/boot.img" ::VERSION.TXT "$out/version.txt"
sha256sum "$out/matrix.bin" "$out/version.txt"

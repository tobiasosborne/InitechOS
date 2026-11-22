#!/bin/sh
# Run creat33.asm on the hash-pinned Packard Bell MS-DOS 3.30 disk with a
# blank mtools 1.44 MB B:. Leaves RESULT.BIN, the B: image and listings.
set -eu
src=$(cd "$(dirname "$0")" && pwd)
out=${1:?absolute new capture directory required}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir "$out"
cd "$out"
nasm -f bin "$src/creat33.asm" -o CREAT33.COM
printf 'ECHO OFF\r\nCREAT33\r\n' > AUTOEXEC.BAT
cp "$media" boot.img
mdel -i boot.img ::BACKUP.COM
mcopy -o -i boot.img AUTOEXEC.BAT CREAT33.COM ::
truncate -s 1474560 b.img
mformat -i b.img -f 1440 -N 12345678 ::
rc=0
timeout 25 qemu-system-i386 -display none -monitor none -serial none -net none -cpu 486 -m 8 -no-reboot \
    -rtc base=1992-03-02T09:30:00 -drive file=boot.img,format=raw,if=floppy,index=0 \
    -drive file=b.img,format=raw,if=floppy,index=1 -boot a \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 > qemu.log 2>&1 || rc=$?
test "$rc" = 33 || { echo "DOS probe exited $rc" >&2; exit 1; }
mcopy -i b.img ::RESULT.BIN RESULT.BIN
mdir -a -i b.img :: > dir.txt
mattrib -i b.img -/ :: | grep -v RESULT.BIN > attrib.txt
for f in F00 F01 F02 F04 F07 F20 F27 FN3 FH1; do printf '%s ' "$f"; mtype -i b.img "::$f" | od -An -tx1 | xargs; done > files.txt
od -An -v -tx1 RESULT.BIN > result.hex

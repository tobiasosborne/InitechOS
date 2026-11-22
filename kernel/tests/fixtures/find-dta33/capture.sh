#!/bin/sh
# Run findta33.asm on the hash-pinned Packard Bell MS-DOS 3.30 disk with the
# B: disk from mint.sh. Leaves RESULT.BIN and the B: image.
set -eu
src=$(cd "$(dirname "$0")" && pwd)
out=${1:?absolute new capture directory required}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir "$out"
cd "$out"
nasm -f bin "$src/findta33.asm" -o FINDTA33.COM
printf 'ECHO OFF\r\nFINDTA33\r\n' > AUTOEXEC.BAT
cp "$media" boot.img
mdel -i boot.img ::BACKUP.COM
mcopy -o -i boot.img AUTOEXEC.BAT FINDTA33.COM ::
sh "$src/mint.sh" b.img
rc=0
timeout 25 qemu-system-i386 -display none -monitor none -serial none -net none -cpu 486 -m 8 -no-reboot \
    -rtc base=1992-03-02T09:30:00 -drive file=boot.img,format=raw,if=floppy,index=0 \
    -drive file=b.img,format=raw,if=floppy,index=1 -boot a \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 > qemu.log 2>&1 || rc=$?
test "$rc" = 33 || { echo "DOS probe exited $rc" >&2; exit 1; }
mcopy -i b.img ::RESULT.BIN RESULT.BIN

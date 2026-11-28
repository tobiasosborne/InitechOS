#!/bin/sh
set -eu
source_dir=$(cd "$(dirname "$0")" && pwd)
out=${1:?new capture directory required}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir "$out"
cd "$out"
cp "$media" boot.img
mdel -i boot.img ::BACKUP.COM
printf 'ECHO OFF\r\nRESIDUAL\r\n' > AUTOEXEC.BAT
nasm -f bin "$source_dir/residual.asm" -o RESIDUAL.COM
nasm -f bin "$source_dir/child.asm" -o CHILD.COM
mcopy -o -i boot.img AUTOEXEC.BAT RESIDUAL.COM CHILD.COM ::
rc=0
timeout 25 qemu-system-i386 -display none -monitor none -serial none -net none -cpu 486 -m 8 -no-reboot \
    -rtc base=1992-03-02T09:30:00 -drive file=boot.img,format=raw,if=floppy,index=0 \
    -boot a -device isa-debug-exit,iobase=0xf4,iosize=0x04 || rc=$?
test "$rc" = 33
mcopy -o -i boot.img ::RESULT.BIN .
od -Ax -v -t x2 RESULT.BIN

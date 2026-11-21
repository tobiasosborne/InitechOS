#!/bin/sh
set -eu
src=$(cd "$(dirname "$0")" && pwd)
out=${1:?absolute new capture directory required}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir "$out"
cd "$out"
nasm -f bin "$src/append.asm" -o APPEND33.COM
printf 'ECHO OFF\r\nAPPEND33\r\n' > AUTOEXEC.BAT
for kind in tail middle blocked; do
    cp "$media" boot.img
    mdel -i boot.img ::BACKUP.COM
    mcopy -o -i boot.img AUTOEXEC.BAT APPEND33.COM ::
    sh "$src/mint.sh" "$kind.img" "$kind"
    mshowfat -i "$kind.img" ::TARGET.BIN > "$kind.before"
    rc=0
    timeout 25 qemu-system-i386 -display none -monitor none -serial none -net none -cpu 486 -m 8 -no-reboot \
        -rtc base=1992-03-02T09:30:00 -drive file=boot.img,format=raw,if=floppy,index=0 \
        -drive file="$kind.img",format=raw,if=floppy,index=1 -boot a \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$kind.qemu.log" 2>&1 || rc=$?
    test "$rc" = 33 || { echo "DOS $kind probe exited $rc" >&2; exit 1; }
    mshowfat -i "$kind.img" ::TARGET.BIN > "$kind.after"
    mcopy -i "$kind.img" ::TARGET.BIN "$kind.bin"
    fsck.fat -n "$kind.img"
done

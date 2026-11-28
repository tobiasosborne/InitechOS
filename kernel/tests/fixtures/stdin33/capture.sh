#!/bin/sh
# Run stdin33.asm on the hash-pinned Packard Bell MS-DOS 3.30 disk. B: holds
# IN.TXT; RES.BIN and ECHO.TXT are read back with mtools.
set -eu
src=$(cd "$(dirname "$0")" && pwd)
out=${1:?absolute new capture directory required}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
mkdir "$out"
cd "$out"
prog=${PROGRAM:-stdin33}
nasm -f bin ${EOF:+-DEOF=$EOF} ${CALLS:+-DCALLS=$CALLS} "$src/$prog.asm" -o PROBE.COM
printf 'ECHO OFF\r\nPROBE\r\n' > AUTOEXEC.BAT
cp "$media" boot.img
mdel -i boot.img ::BACKUP.COM
mcopy -o -i boot.img AUTOEXEC.BAT PROBE.COM ::
sh "$src/mint.sh" b.img
rc=0
timeout "${CAPTURE_TIMEOUT:-25}" qemu-system-i386 -display none -monitor none -serial none -net none -cpu 486 -m 8 -no-reboot \
    -rtc base=1992-03-02T09:30:00 -drive file=boot.img,format=raw,if=floppy,index=0 \
    -drive file=b.img,format=raw,if=floppy,index=1 -boot a \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 > qemu.log 2>&1 || rc=$?
echo "qemu exit $rc" > exit.txt
if [ "$prog" = status33 ]; then
    mcopy -i b.img ::RES0C.BIN RES0C.BIN
    mcopy -i b.img ::ECHO0C.TXT ECHO0C.TXT || true
elif [ "$prog" = buffered33 ]; then
    mcopy -i b.img ::RES0A.BIN RES0A.BIN
    mcopy -i b.img ::ECHO0A.TXT ECHO0A.TXT || true
else
    mcopy -i b.img ::RES.BIN RES.BIN
    mcopy -i b.img ::ECHO.TXT ECHO.TXT
fi

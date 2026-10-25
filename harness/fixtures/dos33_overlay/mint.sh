#!/bin/sh
# Independent DOS33 mint; never changes the reference disk or saved captures.
# Ref: README.txt, and harness/diff/fat_diff/chkdsk_label_mint.sh.
set -eu
media=$1
out=${2:-build/overlay-dos33-mint}
src=harness/fixtures/dos33_overlay
export SOURCE_DATE_EPOCH=699408000 TZ=UTC
mkdir -p "$out"
sha256sum "$media" > "$out/media.sha256"
test "$(cut -d ' ' -f 1 "$out/media.sha256")" = \
    3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
${CC:-cc} -std=c11 -Wall -Wextra -Werror "$src/check.c" -o "$out/check"
nasm -f bin "$src/overlay.asm" -o "$out/PROBE.OVL"
nasm -f bin "$src/exit.asm" -o "$out/EXITEMU.COM"
for mode in different equal; do
    boot=$out/$mode-boot.img
    data=$out/$mode-data.img
    cp -f "$media" "$boot"
    mdel -i "$boot" ::BACKUP.COM
    truncate -s 1474560 "$data"
    mformat -i "$data" -f 1440 -N 19920302 ::
    if [ "$mode" = equal ]; then
        nasm -f bin -DEQUAL_FACTOR "$src/probe.asm" -o "$out/PROBE.COM"
    else
        nasm -f bin "$src/probe.asm" -o "$out/PROBE.COM"
    fi
    printf '@ECHO OFF\r\nVER > A:\\VERSION.TXT\r\nPROBE\r\nEXITEMU\r\n' > "$out/AUTOEXEC.BAT"
    for f in PROBE.COM EXITEMU.COM AUTOEXEC.BAT; do
        mcopy -o -i "$boot" "$out/$f" ::"$f"
    done
    mcopy -o -i "$data" "$out/PROBE.OVL" ::PROBE.OVL
    rc=0
    timeout 20 qemu-system-i386 -display none -monitor none -serial none \
        -cpu 486 -m 16 -no-reboot -rtc base=1992-03-02T09:30:00 \
        -drive file="$boot",format=raw,if=floppy,index=0 \
        -drive file="$data",format=raw,if=floppy,index=1 -boot a \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$out/$mode.qemu.log" 2>&1 || rc=$?
    test "$rc" = 33 || { cat "$out/$mode.qemu.log"; exit 1; }
    mtype -i "$boot" ::VERSION.TXT > "$out/$mode.version.raw"
    grep -q '3.30' "$out/$mode.version.raw"
    mtype -i "$boot" ::RESULT.BIN > "$out/factor-$mode.bin"
done
"$out/check" "$out/factor-different.bin" "$out/factor-equal.bin"

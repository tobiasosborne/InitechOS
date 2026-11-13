#!/bin/sh
# Mint with genuine DOS 3.3, never InitechDOS. Reference media stays read-only.
# Ref: MS-DOS 3.3 User's Reference p.43; IBM DOS Reference p.7-50.
# Usage: chkdsk_label_mint.sh <DOS 3.3 boot disk> <output directory>
set -eu
media=$1; out=$2
export SOURCE_DATE_EPOCH=699408000 TZ=UTC
mkdir -p "$out"
sha256sum "$media" > "$out/media.sha256"
mcopy -i "$media" ::CHKDSK.COM "$out/CHKDSK.COM"
sha256sum "$out/CHKDSK.COM" > "$out/chkdsk.sha256"
printf 'bits 16\norg 0x100\nmov ax,0x10\nout 0xf4,ax\nhlt\n' > "$out/exit.asm"
nasm -f bin "$out/exit.asm" -o "$out/EXITEMU.COM"
printf 'fixture\r\n' > "$out/HIDDEN.TXT"
for name in none label hidden both; do
    boot=$out/$name-boot.img; data=$out/$name-data.img
    cp -f "$media" "$boot"
    # Make room for AUTOEXEC and its reports on the disposable 360K copy.
    mdel -i "$boot" ::BACKUP.COM
    dd if=/dev/zero of="$data" bs=512 count=2880 status=none
    mformat -i "$data" -f 1440 -N 19920302 ::
    case $name in label|both) mlabel -i "$data" ::CHKTEST ;; esac
    case $name in hidden|both)
        mcopy -i "$data" "$out/HIDDEN.TXT" ::HIDDEN.TXT
        mattrib -i "$data" +h ::HIDDEN.TXT ;;
    esac
    printf '@ECHO OFF\r\nVER > A:\\VERSION.TXT\r\nCHKDSK B: > A:\\REPORT.TXT\r\nEXITEMU\r\n' > "$out/AUTOEXEC.BAT"
    mcopy -o -i "$boot" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
    mcopy -o -i "$boot" "$out/EXITEMU.COM" ::EXITEMU.COM
    rc=0
    timeout 20 qemu-system-i386 -display none -monitor none -serial none \
        -cpu 486 -m 16 -no-reboot -rtc base=1992-03-02T09:30:00 \
        -drive file="$boot",format=raw,if=floppy,index=0 \
        -drive file="$data",format=raw,if=floppy,index=1 -boot a \
        -device isa-debug-exit,iobase=0xf4,iosize=0x04 > "$out/$name.qemu.log" 2>&1 || rc=$?
    [ "$rc" = 33 ] || { cat "$out/$name.qemu.log"; echo "RED DOS33 mint $name: exit $rc"; exit 1; }
    mtype -i "$boot" ::VERSION.TXT > "$out/$name.version.raw"
    mtype -i "$boot" ::REPORT.TXT > "$out/$name.raw"
    tr -d '\r' < "$out/$name.version.raw" > "$out/$name.version.txt"
    tr -d '\r' < "$out/$name.raw" > "$out/$name.txt"
    grep '3.30' "$out/$name.version.txt"
    cat "$out/$name.txt"
done

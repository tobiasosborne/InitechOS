#!/bin/sh
# Preserve the legacy shell's master BOOT partition and slave FAT12 A:.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/image
sh factory/image/mk_boot_system.sh build/image/boot.img build/boot/mbr.bin build/boot/stage2.bin build/boot/IO.SYS "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" build/factory/boot_system
work=$(mktemp -d build/image/mint.XXXXXX)
trap 'rm -rf "$work" "$work.c"; rm -f "$work.img" "$work.a"' EXIT HUP INT TERM
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
dd if=/dev/zero of="$work/data.img" bs=512 count=2880 status=none
mformat -i "$work/data.img" -f 1440 -N 0x494e4933 -v INITECH ::
for f in build/dos/*.COM; do
 cp "$f" "$work/$(basename "$f")"
done
printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\nDEVICE=ANSI.SYS\r\n' > "$work/CONFIG.SYS"
# COMMAND.COM remains resident; this ordinary executable forwards COMMAND /C
# to the resident interpreter through the real INT21 EXEC route.
cp build/dos/COMMAND.COM "$work/COMMAND.COM"
printf 'Hello from InitechDOS.\r\n' > "$work/HELLO.TXT"
printf '@ECHO OFF\r\nFLAIR\r\n' > "$work/AUTOEXEC.BAT"
# Product disk contents are component artifacts listed by system/.
while read -r source destination rest; do
 case "$source" in ""|\#*) continue;; esac
 test -z "$rest"
 mkdir -p "$work/$(dirname "$destination")"
 cp "$source" "$work/$destination"
done < system/working-volume.list
# The C: system disk carries the same working volume (staged before data.img
# is copied in), its AUTOEXEC putting C: and C:\APPS on the PATH. It is not
# yet the booted product image: delivery switches the product to it once its
# C: startup is on main (initech-tlhqy); until then both layouts are minted.
mkdir "$work.c"
for entry in "$work"/*; do [ "$entry" = "$work/data.img" ] || cp -R "$entry" "$work.c/"; done
printf '@ECHO OFF\r\nPATH C:\\;C:\\APPS\r\nFLAIR\r\n' > "$work.c/AUTOEXEC.BAT"
find "$work" "$work.c" -exec touch -d '@315532800' {} +
mv "$work/data.img" "$work.img"
sh factory/image/copy-tree.sh "$work" "$work.img"
mv "$work.img" build/image/data.img
build/factory/boot_system build/image/boot.img check >/dev/null
sh factory/image/mk_system_c.sh build/image/c.img build/boot/mbr.bin build/boot/stage2.bin build/boot/IO.SYS "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" build/factory/boot_system "$work.c"
rm -rf "$work.c"
# The optional A: data volume (ATA, never a diskette): system/'s manifest and
# installer, then this factory's fixed shipped timestamps.
dd if=/dev/zero of="$work.a" bs=512 count=2880 status=none
mformat -i "$work.a" -f 1440 -N 0x44415441 -v DATA ::
sh system/install-data.sh "$work.a"
mv "$work.a" build/image/a-data.img

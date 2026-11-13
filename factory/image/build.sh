#!/bin/sh
# Preserve the legacy shell's master BOOT partition and slave FAT12 A:.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/image
sh factory/image/mk_boot_system.sh build/image/boot.img build/boot/mbr.bin build/boot/stage2.bin build/boot/IO.SYS build/kernel/INITDOS.SYS build/factory/boot_system
work=$(mktemp -d build/image/mint.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
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
for f in "$work"/*; do
 [ "$f" = "$work/data.img" ] && continue
 touch -d '@315532800' "$f"
 mcopy -m -i "$work/data.img" "$f" ::
done
mv "$work/data.img" build/image/data.img

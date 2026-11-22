#!/bin/sh
# Preserve the legacy shell's master BOOT partition and slave FAT12 A:.
set -eu
cd "$(dirname "$0")/../.."
mkdir -p build/image
sh factory/image/mk_boot_system.sh build/image/boot.img build/boot/mbr.bin build/boot/stage2.bin build/boot/IO.SYS "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" build/factory/boot_system
work=$(mktemp -d build/image/mint.XXXXXX)
trap 'rm -rf "$work"; rm -f "$work.img" "$work.a"' EXIT HUP INT TERM
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
dd if=/dev/zero of="$work/data.img" bs=512 count=2880 status=none
mformat -i "$work/data.img" -f 1440 -N 0x494e4933 -v INITECH ::
# Product disk contents are component artifacts listed by system/, the DOS
# utilities included (a closed inventory, no wildcard): a missing input
# stops the build (delivery owner, initech-tlhqy).
while read -r source destination rest; do
 case "$source" in ""|\#*) continue;; esac
 test -z "$rest"
 test -f "$source" || { echo "image: $source (for $destination) is missing" >&2; exit 1; }
 mkdir -p "$work/$(dirname "$destination")"
 cp "$source" "$work/$destination"
done < system/working-volume.list
find "$work" -exec touch -d '@315532800' {} +
mv "$work/data.img" "$work.img"
sh factory/image/copy-tree.sh "$work" "$work.img"
mv "$work.img" build/image/data.img
build/factory/boot_system build/image/boot.img check >/dev/null
# The A: pair above stays canonical until activation step 20. system/
# supplies the corresponding prompt-start boot files for the installed C:.
cp build/system/installed/c-boot/CONFIG.SYS build/system/installed/c-boot/AUTOEXEC.BAT "$work/"
touch -d '@315532800' "$work/CONFIG.SYS" "$work/AUTOEXEC.BAT"
sh factory/image/mk_system_c.sh build/image/c.img build/boot/mbr.bin build/boot/stage2.bin build/boot/IO.SYS "${SYSTEM_IMAGE:-build/system/INITDOS.SYS}" build/factory/boot_system "$work"
# The optional A: data volume (ATA, never a diskette): system/'s manifest and
# installer, then this factory's fixed shipped timestamps.
dd if=/dev/zero of="$work.a" bs=512 count=2880 status=none
mformat -i "$work.a" -f 1440 -N 0x44415441 -v DATA ::
sh system/install-data.sh "$work.a"
mv "$work.a" build/image/a-data.img

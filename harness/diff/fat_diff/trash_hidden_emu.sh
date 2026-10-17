#!/bin/sh
# Runtime service attribute, independently read back by mtools.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; boot=$2; seed=$3; name=$4
work=$build/khygiene-trash-$name
mkdir -p "$work"
cp -f "$seed" "$work/data.img"
"$build/qemu_harness" --disk "$boot" --disk2 "$work/data.img" --name trash --out "$work" \
    --expect FLAIR-FAT-MOUNT-OK --timeout-ms 12000 --keys-after FLAIR-LIVE-READY \
    --mouse 'm100:-88,m100:-88,m80:0,l1,l0,l1,l0' --quit-after FINDER-OPEN-VOLUME \
    > "$work/stdout" 2> "$work/report" || true
! grep -q 'triple_fault=1' "$work/report"
grep -q '^FINDER-OPEN-VOLUME' "$work/trash.serial" || { echo 'RED TRASH trace incomplete'; exit 1; }
sh harness/verify_fixture_volume.sh harness "$work/data.img" TRASH=H DESKTOP.DB=H \
    || { echo 'RED TRASH hidden attribute: runtime service directory must have H'; exit 1; }
mdir -a -i "$work/data.img" :: | grep -q '^TRASH .*<DIR>'
echo 'GREEN test-trash-hidden-emu: injected Finder trace; mtools TRASH=H DESKTOP.DB=H'

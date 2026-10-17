#!/bin/sh
# bcg.12: real port-I/O failure without a primary slave; shell stays alive.
set -eu
export INITECH_QMP_STDIO=1
build=$1; boot=$2; name=$3
work=$build/khygiene-ata-$name
mkdir -p "$work"
"$build/qemu_harness" --disk "$boot" --name nodrive --out "$work" \
    --timeout-ms 12000 --expect SHELL-READY --keys 'e,x,i,t,ret' \
    --keys-after SHELL-READY --quit-after SHELL-DONE > "$work/stdout" 2> "$work/report" || true
! grep -q 'triple_fault=1' "$work/report"
grep -qx 'FAT-MOUNT-FAIL rc=-1 base_lba=0 hidden_sectors=0' "$work/nodrive.serial" \
    || { echo 'RED ATA no-drive: mount must report READ failure, not success or bad BPB'; exit 1; }
grep -qx SHELL-DONE "$work/nodrive.serial"
echo 'GREEN test-ata-no-drive-emu: missing slave reports READ failure; injected shell EXIT completes'

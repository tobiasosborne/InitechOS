#!/bin/sh
# ADR-0003 DEC-08a.2: reject entry at module end through the real FAT loader.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; boot=$2; seed=$3; name=$4
work=$build/khygiene-mz-$name
mkdir -p "$work"
cp -f "$seed" "$work/data.img"
"$build/test_mzload" "$work/entry.exe" > "$work/host.log"
mcopy -o -i "$work/data.img" "$work/entry.exe" ::MZEXEC.EXE
"$build/qemu_harness" --disk "$boot" --disk2 "$work/data.img" --name entry --out "$work" \
    --expect MZEXEC-BEGIN --timeout-ms 6000 > "$work/stdout" 2> "$work/report" || true
grep -qx MZEXEC-BEGIN "$work/entry.serial" \
    || { echo 'RED MZ fixture: kernel never reached the FAT load'; exit 1; }
grep -qx 'MZEXEC-FAIL st=11' "$work/entry.serial" \
    || { echo 'RED MZ entry at end: missing BAD_FORMAT rejection'; exit 1; }
grep -qx MZEXEC-END "$work/entry.serial"
! grep -q 'triple_fault=1' "$work/report"
echo 'GREEN test-mzload-entry-emu: FAT MZ entry-at-end rejected BAD_FORMAT; kernel resumes'

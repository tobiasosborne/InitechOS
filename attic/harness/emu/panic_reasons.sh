#!/bin/sh
# CLAUDE Rule 2; ADR-0003 DEC-08a.5; DOS L030 / DAS L001 audit evidence.
# Literal text is independent of the artifact's enum/message table. The font
# comes from the pre-fault ROM dump in test-panic-screen, never screen pixels.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C
build=$1; image=$2; base=$3; tag=$4; kind=$5
work=$build/panic-reasons
mkdir -p "$work"
set --
case "$kind" in
foreign)
    data=$work/$tag-data.img
    dd if=/dev/zero of="$data" bs=512 count=2880 status=none
    mformat -i "$data" -f 1440 ::
    fixture=docs/audits/2026-10-23-dos/evidence/special-fixtures/T_DOS16.EXE
    mcopy -o -i "$data" "$fixture" ::DOS16.EXE
    mtype -i "$data" ::DOS16.EXE | cmp - "$fixture"
    printf '@ECHO OFF\r\n' > "$work/$tag.bat"
    mcopy -o -i "$data" "$work/$tag.bat" ::AUTOEXEC.BAT
    set -- --disk2 "$data" --keys d,o,s,1,6,ret --keys-after SHELL-READY
    reason='UNSUPPORTED 16-BIT MZ'; subject=DOS16.EXE
    ;;
nodata) reason='REQUIRED DATA DISK UNAVAILABLE'; subject='DATA DISK' ;;
noapps|nocalc|badcalc)
    data=$work/$tag-data.img
    cp -f "$base" "$data"
    case "$kind" in
    noapps) mdeltree -i "$data" ::APPS; reason='REQUIRED APPS FOLDER UNAVAILABLE'; subject=APPS ;;
    nocalc) mdel -i "$data" ::APPS/CALC.EXE; reason='CANNOT LOAD REQUIRED CALC.EXE'; subject=CALC.EXE ;;
    badcalc)
        printf 'MZ' > "$work/bad-calc.exe"
        mcopy -o -i "$data" "$work/bad-calc.exe" ::APPS/CALC.EXE
        reason='CANNOT LOAD REQUIRED CALC.EXE'; subject=CALC.EXE ;;
    esac
    set -- --disk2 "$data"
    ;;
*) exit 2 ;;
esac
"$build/qemu_harness" --disk "$image" "$@" --out "$work" --name "$tag" \
    --timeout-ms 20000 --expect HALTED --quit-after HALTED \
    --screendump --screendump-after HALTED > "$work/$tag.stdout" 2> "$work/$tag.report"
grep -q 'triple_fault=0' "$work/$tag.report"
grep -q 'guest_errors=0' "$work/$tag.report"
tr -d '\r' < "$work/$tag.serial" > "$work/$tag.norm"
grep -qx HALTED "$work/$tag.norm"
test "$(grep -cx KERNEL "$work/$tag.norm")" -eq 1
if [ "$kind" = foreign ]; then
    grep -q '^PANIC foreign-mz:' "$work/$tag.norm"
    grep -qx '  name=DOS16.EXE' "$work/$tag.norm"
else
    grep -q '^PANIC ' "$work/$tag.norm"
fi
echo "GREEN panic-reason-serial $tag: PANIC + HALTED, no reboot"
"$build/ppm_panic_check" "$work/$tag.ppm" "$build/panic-screen/ud32.font" 640 480 "$reason" "$subject"

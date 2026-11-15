#!/bin/sh
set -eu
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$(pwd)/build/dos-batch-transfer-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
truncate -s 368640 "$d/data.img"
mformat -i "$d/data.img" -f 360 -N 19920302 ::
printf 'keep these bytes\r\n' > "$d/KEEP.TXT"
mcopy -i "$d/data.img" "$d/KEEP.TXT" ::KEEP.TXT
for mode in PLAIN TCALL TIF TFOR; do
    case $mode in
    PLAIN) invoke='CHILD arg';;
    TCALL) invoke='CALL CHILD arg';;
    TIF) invoke='IF x==x CHILD arg';;
    TFOR) invoke='FOR %%i IN (one two) DO CHILD arg';;
    esac
    printf '@echo off\r\n%s\r\necho PARENT-RETURN\r\n' "$invoke" > "$d/$mode.BAT"
    mcopy -i "$d/data.img" "$d/$mode.BAT" "::$mode.BAT"
done
printf '@echo off\r\nrem unndel *.txt\r\necho CHILD=%%1\r\nGRAND\r\necho CHILD-RETURN\r\n' > "$d/CHILD.BAT"
printf '@echo off\r\necho GRAND\r\n' > "$d/GRAND.BAT"
mcopy -i "$d/data.img" "$d/CHILD.BAT" ::CHILD.BAT
mcopy -i "$d/data.img" "$d/GRAND.BAT" ::GRAND.BAT
printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c PLAIN.BAT >PLAIN.OUT\r\ncommand /c TCALL.BAT >TCALL.OUT\r\ncommand /c TIF.BAT >TIF.OUT\r\ncommand /c TFOR.BAT >TFOR.OUT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
for mode in PLAIN TCALL TIF TFOR; do
    mtype -i "$d/data.img" "::$mode.OUT" > "$d/$mode.OUT"
done
mtype -i "$d/data.img" ::KEEP.TXT > "$d/AFTER.TXT"
cmp "$d/KEEP.TXT" "$d/AFTER.TXT"
gzip -n -c "$d/data.img" > "$d/data.img.gz"

#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-attrib-error-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
gzip -dc dos/command/tests/compat33/expected/U13/data.img.gz > "$d/data.img"
awk '{printf "%s\r\n",$0}' dos/command/tests/dos33/attrib-error-input.txt > "$d/ATTERR.BAT"
mcopy -o -i "$d/data.img" "$d/ATTERR.BAT" ::ATTERR.BAT
printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c ATTERR.BAT >TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
# The product reads AUTOEXEC from its data drive. Retain the same directory
# member in the reference data disk so the wide inventory is comparable.
mcopy -o -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
mtype -i "$d/data.img" ::TRACE.TXT > "$d/TRACE.TXT"
gzip -n -c "$d/data.img" > "$d/data.img.gz"
mtype -i "$d/data.img" ::UTIL.OUT > "$d/UTIL.OUT"

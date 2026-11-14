#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-prompt-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
printf '@echo off\r\nver\r\nprompt [$v][$d][$t][$p]$g\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf '@echo off\r\nprompt $v$g\r\n' > "$d/PCASE.BAT"
mcopy -o -i "$d/boot.img" "$d/PCASE.BAT" ::PCASE.BAT
printf 'wait CAPTURE-DONE\nshot combined\ntype pcase\nkey ret\nsleep 1\nshot vonly\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" - - "$d/steps" > "$d/driver.log" 2>&1
cat "$d/combined.txt"
echo ===VONLY===
cat "$d/vonly.txt"

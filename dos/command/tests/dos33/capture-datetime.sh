#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-datetime-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
# Fresh empty data disk: batch inputs must not inherit other cases' files.
truncate -s 0 "$d/data.img"
truncate -s 1474560 "$d/data.img"
mformat -i "$d/data.img" -f 1440 -N 19920302 ::
awk '{printf "%s\r\n",$0}' dos/command/tests/dos33/datetime-input.txt > "$d/DTCASE.BAT"
nasm -f bin dos/command/tests/dos33/clock.asm -o "$d/CLOCK.COM"
mcopy -o -i "$d/data.img" "$d/DTCASE.BAT" ::DTCASE.BAT
mcopy -o -i "$d/data.img" "$d/CLOCK.COM" ::CLOCK.COM
# Genuine DATE/TIME prompt for entry after an invalid argument; blank
# answers keep the clock and let the batch continue.
printf '\r\n%.0s' 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 > "$d/ANSWERS.TXT"
mcopy -o -i "$d/data.img" "$d/ANSWERS.TXT" ::ANSWERS.TXT
printf '@echo off\r\nver > A:\\VERSION.TXT\r\npath A:\\\r\nB:\r\ncommand /c DTCASE.BAT <ANSWERS.TXT >TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
mcopy -o -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
mtype -i "$d/boot.img" ::VERSION.TXT > "$d/VERSION.TXT"
mtype -i "$d/data.img" ::TRACE.TXT > "$d/TRACE.TXT"
gzip -n -c "$d/data.img" > "$d/data.img.gz"

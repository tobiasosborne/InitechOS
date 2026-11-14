#!/bin/sh
set -eu
root=$(pwd)
d=$root/build/dos-compat33/break-reference
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$d"
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
cp "$media" "$d/boot.img"
printf '@echo off\r\nver > VERSION.TXT\r\npause\r\necho AFTER > AFTER.OUT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'wait Strike a key\nkey ctrl-pause\nsleep 1\nshot question\nkey y\nsleep 1\nshot after\nend\n' > "$d/steps"
"$d/drive" "$d" "$d/boot.img" - - "$d/steps"
mcopy -o -i "$d/boot.img" ::VERSION.TXT "$d/VERSION.TXT"
if mdir -i "$d/boot.img" ::AFTER.OUT >/dev/null 2>&1; then
 mcopy -o -i "$d/boot.img" ::AFTER.OUT "$d/AFTER.OUT"
fi

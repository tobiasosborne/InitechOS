#!/bin/sh
set -eu
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$(pwd)/build/dos-self-alias-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
truncate -s 368640 "$d/data.img"
mformat -i "$d/data.img" -f 360 -N 19920302 ::
printf 'original source bytes\r\n' > "$d/SOURCE.TXT"
mcopy -i "$d/data.img" "$d/SOURCE.TXT" ::LONGNAME.TXT
printf '@echo off\r\nver\r\ncopy LONGNAME.TXT LONGNAMEX.TXT\r\necho SELF-DONE\r\n' > "$d/SELF.BAT"
mcopy -i "$d/data.img" "$d/SELF.BAT" ::SELF.BAT
printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c SELF.BAT >A:\\TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
mtype -i "$d/boot.img" ::TRACE.TXT > "$d/TRACE.TXT"
mtype -i "$d/data.img" ::LONGNAME.TXT > "$d/AFTER.TXT"
cmp "$d/SOURCE.TXT" "$d/AFTER.TXT"
gzip -n -c "$d/data.img" > "$d/data.img.gz"
cat "$d/TRACE.TXT"

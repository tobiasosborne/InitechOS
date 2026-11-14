#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-diskcopy-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
# B: a 360K target holding precious content; bytes are checked after the run.
truncate -s 0 "$d/data.img"
truncate -s 368640 "$d/data.img"
mformat -i "$d/data.img" -f 360 -N 19920302 -v PRECIOUS ::
printf 'precious target content\r\n' > "$d/P.TXT"
mcopy -i "$d/data.img" "$d/P.TXT" ::P.TXT
mattrib -i "$d/data.img" +r ::P.TXT
printf '@echo off\r\nver > A:\\VERSION.TXT\r\na:\\diskcopy b: b:\r\necho AFTER-DISKCOPY\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait SOURCE\nshot source\nkey spc\nwait TARGET\nshot target\nkey ctrl-c\nsleep 2\nshot afterc\nkey y\nsleep 2\nshot final\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
mdir -a -i "$d/data.img" :: > "$d/after.mdir"
gzip -n -c "$d/data.img" > "$d/data.img.gz"
gzip -dc "$d/before-data.img.gz" > "$d/before-data.img"
if cmp -s "$d/before-data.img" "$d/data.img"; then echo IDENTICAL > "$d/IDENTITY"; else echo CHANGED > "$d/IDENTITY"; fi
cat "$d/IDENTITY"

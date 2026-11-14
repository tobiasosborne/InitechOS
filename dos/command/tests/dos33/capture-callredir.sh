#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
d=$root/build/dos-callredir-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
truncate -s 0 "$d/data.img"; truncate -s 368640 "$d/data.img"
mformat -i "$d/data.img" -f 360 -N 19920302 ::
printf '@echo off\r\necho CHILD-RAN\r\n' > "$d/CHILD.BAT"
mcopy -o -i "$d/data.img" "$d/CHILD.BAT" ::CHILD.BAT
printf '@echo off\r\npath A:\\\r\nB:\r\ncall CHILD.BAT > TRACE.TXT\r\ncall NOPE.BAT > NOPE.OUT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$d/drive"
"$d/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
mtype -i "$d/data.img" ::TRACE.TXT > "$d/TRACE.TXT" 2>/dev/null || true
mtype -i "$d/data.img" ::NOPE.OUT > "$d/NOPE-OUT.TXT" 2>/dev/null || echo ABSENT > "$d/NOPE-OUT.TXT"
mdir -a -i "$d/data.img" :: > "$d/after.mdir"
gzip -n -c "$d/data.img" > "$d/data.img.gz"

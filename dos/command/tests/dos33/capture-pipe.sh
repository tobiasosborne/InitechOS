#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
d=$root/build/dos-pipe-capture
mkdir -p "$d"
cp "$media" "$d/boot.img"
gzip -dc dos/command/tests/compat33/expected/REDIR/data.img.gz > "$d/data.img"
printf '@echo off\r\ntime 12:34:10\r\necho hello|dir >ONE.OUT\r\necho hello|dir >TWO.OUT\r\ntime 23:58:10\r\necho hello|dir >THREE.OUT\r\necho hello|type A.TXT|dir >FOUR.OUT\r\necho a-long-line|echo middle|echo x|SUB\\PASS >FIVE.OUT\r\necho PIPE-DONE\r\n' > "$d/PIPECASE.BAT"
nasm -DREAL_DOS -f bin dos/command/tests/compat33/stream.asm -o "$d/PASS.COM"
mcopy -o -i "$d/data.img" "$d/PASS.COM" ::SUB/PASS.COM
mcopy -o -i "$d/data.img" "$d/PIPECASE.BAT" ::PIPECASE.BAT
printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c PIPECASE.BAT >TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
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
for name in ONE TWO THREE FOUR FIVE;do
 mtype -i "$d/data.img" "::$name.OUT" > "$d/$name.OUT"
done

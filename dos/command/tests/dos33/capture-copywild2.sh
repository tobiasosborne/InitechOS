#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
base=$root/build/dos-copywild2-capture
mkdir -p "$base"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$base/drive"
printf 'AA\032tail\r\n' > "$base/A.TXT"
printf 'BB\r\n' > "$base/B.TXT"
run_case() {
 case_name=$1; copycmd=$2; outname=$3
 d=$base/$case_name
 mkdir -p "$d"
 cp "$media" "$d/boot.img"
 truncate -s 0 "$d/data.img"; truncate -s 368640 "$d/data.img"
 mformat -i "$d/data.img" -f 360 -N 19920302 ::
 mcopy -i "$d/data.img" "$base/A.TXT" ::A.TXT
 mcopy -i "$d/data.img" "$base/B.TXT" ::B.TXT
 printf '@echo off\r\n%s\r\necho COPYWILD-DONE\r\n' "$copycmd" > "$d/CPYCASE.BAT"
 mcopy -o -i "$d/data.img" "$d/CPYCASE.BAT" ::CPYCASE.BAT
 printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c CPYCASE.BAT >A:\\TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
 mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 mdir -a -i "$d/data.img" :: > "$d/before.mdir"
 gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
 printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 "$base/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
 mtype -i "$d/boot.img" ::TRACE.TXT > "$d/TRACE.TXT"
 mtype -i "$d/data.img" "::$outname" > "$d/ALL-OUT.TXT" 2>/dev/null || echo ABSENT > "$d/ALL-OUT.TXT"
 mdir -a -i "$d/data.img" :: > "$d/after.mdir"
 gzip -n -c "$d/data.img" > "$d/data.img.gz"
 echo "=== $case_name"; cat -v "$d/TRACE.TXT"; echo "-- OUT:"; xxd "$d/ALL-OUT.TXT"
}
run_case outside 'copy *.txt all.dat' ALL.DAT
run_case inside 'copy *.txt all.txt' ALL.TXT
run_case plus 'copy a.txt+b.txt all.txt' ALL.TXT

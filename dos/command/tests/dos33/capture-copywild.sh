#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
base=$root/build/dos-copywild-capture
mkdir -p "$base"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$base/drive"
printf 'AAAA\r\n' > "$base/A.TXT"
printf 'BBBB\r\n' > "$base/B.TXT"
printf 'XXXX\r\n' > "$base/X.TXT"
printf 'OLD!\r\n' > "$base/ALL.TXT"
run_case() {
 case_name=$1
 d=$base/$case_name
 mkdir -p "$d"
 cp "$media" "$d/boot.img"
 truncate -s 0 "$d/data.img"; truncate -s 368640 "$d/data.img"
 mformat -i "$d/data.img" -f 360 -N 19920302 ::
 printf '@echo off\r\ncopy *.txt all.txt\r\necho COPYWILD-DONE\r\n' > "$d/CPYCASE.BAT"
 mcopy -o -i "$d/data.img" "$d/CPYCASE.BAT" ::CPYCASE.BAT
 case "$case_name" in
  deleted)
   mcopy -i "$d/data.img" "$base/X.TXT" ::X.TXT
   mcopy -i "$d/data.img" "$base/A.TXT" ::A.TXT
   mcopy -i "$d/data.img" "$base/B.TXT" ::B.TXT
   mdel -i "$d/data.img" ::X.TXT ;;
  plain)
   mcopy -i "$d/data.img" "$base/A.TXT" ::A.TXT
   mcopy -i "$d/data.img" "$base/B.TXT" ::B.TXT ;;
  existing)
   mcopy -i "$d/data.img" "$base/ALL.TXT" ::ALL.TXT
   mcopy -i "$d/data.img" "$base/A.TXT" ::A.TXT
   mcopy -i "$d/data.img" "$base/B.TXT" ::B.TXT ;;
 esac
 printf '@echo off\r\npath A:\\\r\nB:\r\ncommand /c CPYCASE.BAT >A:\\TRACE.TXT\r\necho CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
 mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 if [ "$case_name" = deleted ]; then
  # mformat -f 360: root starts at sector 5; CPYCASE occupies slot 0.
  [ "$(od -An -tu1 -j 2592 -N 1 "$d/data.img" | tr -d ' ')" = 229 ]
 fi
 mdir -a -i "$d/data.img" :: > "$d/before.mdir"
 gzip -n -c "$d/data.img" > "$d/before-data.img.gz"
 printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 "$base/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
 mtype -i "$d/boot.img" ::TRACE.TXT > "$d/TRACE.TXT"
 mtype -i "$d/data.img" ::ALL.TXT > "$d/ALL-OUT.TXT" 2>/dev/null || echo ABSENT > "$d/ALL-OUT.TXT"
 mdir -a -i "$d/data.img" :: > "$d/after.mdir"
 gzip -n -c "$d/data.img" > "$d/data.img.gz"
 echo "=== $case_name"; cat -v "$d/TRACE.TXT"; echo "-- ALL:"; xxd "$d/ALL-OUT.TXT"
}
run_case deleted
run_case plain
run_case existing

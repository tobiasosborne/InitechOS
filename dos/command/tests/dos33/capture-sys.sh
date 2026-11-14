#!/bin/sh
set -eu
root=$(pwd)
media=$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img
[ "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30 ]
base=$root/build/dos-sys-capture
mkdir -p "$base"
cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$base/drive"
printf 'dummy one\r\n' > "$base/one.txt"
printf 'user ibmbio content\r\n' > "$base/ubio.txt"
printf 'user ibmdos content\r\n' > "$base/udos.txt"
run_case() {
 case_name=$1; sysarg=$2
 d=$base/$case_name
 mkdir -p "$d"
 cp "$media" "$d/boot.img"
 truncate -s 0 "$d/data.img"; truncate -s 368640 "$d/data.img"
 mformat -i "$d/data.img" -f 360 -N 19920302 ::
 case "$case_name" in
  elsewheresys)
   mcopy -i "$d/data.img" "$base/one.txt" ::FIRST.TXT
   mcopy -i "$d/data.img" "$base/one.txt" ::SECOND.TXT
   mcopy -i "$d/data.img" "$base/ubio.txt" ::IBMBIO.COM
   mcopy -i "$d/data.img" "$base/udos.txt" ::IBMDOS.COM
   mdel -i "$d/data.img" ::FIRST.TXT ::SECOND.TXT ;;
  occupied)
   mcopy -i "$d/data.img" "$base/one.txt" ::FIRST.TXT
   mcopy -i "$d/data.img" "$base/one.txt" ::SECOND.TXT ;;
  clean|self) ;;
 esac
 printf '@echo off\r\nsys %s > A:\\TRACE.TXT\r\necho SYS-EXIT-%%ERRORLEVEL%% >> A:\\TRACE.TXT\r\necho CAPTURE-DONE\r\n' "$sysarg" > "$d/AUTOEXEC.BAT"
 mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 mdir -a -i "$d/data.img" :: > "$d/before.mdir"
 cp "$d/data.img" "$d/before-data.img"
 cp "$d/boot.img" "$d/before-boot.img"
 "$base/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1
 mtype -i "$d/boot.img" ::TRACE.TXT > "$d/TRACE.TXT" 2>/dev/null || true
 mdir -a -i "$d/data.img" :: > "$d/after.mdir"
 fsck.fat -n "$d/data.img" > "$d/fsck.log" 2>&1 || true
 if cmp -s "$d/before-data.img" "$d/data.img"; then echo IDENTICAL > "$d/IDENTITY"; else echo CHANGED > "$d/IDENTITY"; fi
 if cmp -s "$d/before-boot.img" "$d/boot.img"; then echo IDENTICAL > "$d/BOOT-IDENTITY"; else echo CHANGED > "$d/BOOT-IDENTITY"; fi
 gzip -n -c "$d/before-data.img" > "$d/before-data.img.gz"; rm "$d/before-data.img"
 gzip -n -c "$d/data.img" > "$d/data.img.gz"
 echo "$case_name: $(cat "$d/IDENTITY") trace:"; cat "$d/TRACE.TXT" 2>/dev/null || true
}
run_case elsewheresys 'b:'
run_case occupied 'b:'
run_case clean 'b:'
run_case self 'a:'

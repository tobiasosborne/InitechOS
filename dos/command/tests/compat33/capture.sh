#!/bin/sh
# Run from repository root. Captures observations; never synthesizes expectations.
set -eu
export LC_ALL=C TZ=UTC
export INITECH_TEST_RAM_MB=8
mode=${1:?reference or product}
shift
case "$mode" in reference|product) ;; *) exit 2;; esac
root=$(pwd)
src=$root/dos/command/tests/compat33
out=$root/build/dos-compat33
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
rm -rf "$out/inputs-$mode"
mkdir -p "$out/inputs-$mode" "$out/$mode"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/generate.c" -o "$out/generate"
"$out/generate" "$out/inputs-$mode"
if [ "$mode" = reference ]; then
 test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
 cc -std=c11 -O2 -Wall -Wextra -Werror goldens/dataloss/drive.c -o "$out/drive"
 nasm -DREAL_DOS -f bin "$src/tail.asm" -o "$out/inputs-$mode/TAIL.COM"
 nasm -DREAL_DOS -f bin "$src/devices.asm" -o "$out/inputs-$mode/DEVPROBE.COM"
 nasm -DREAL_DOS -f bin "$src/stream.asm" -o "$out/inputs-$mode/PASS.COM"
else
 test "$mode" = product
 nasm -f bin "$src/tail.asm" -o "$out/inputs-$mode/TAIL.COM"
 nasm -f bin "$src/devices.asm" -o "$out/inputs-$mode/DEVPROBE.COM"
 nasm -f bin "$src/stream.asm" -o "$out/inputs-$mode/PASS.COM"
fi
[ "$#" -gt 0 ] || set -- BATCH LIMIT EXPAND PARAMS ENV SWITCH NAMES REDIR TCASE DEVICES CFG1 CFG2 CFG3 CFG4 FULL
pending=0
for case in "$@"; do
 case "$case" in ''|*[!A-Z0-9]*) echo 'Invalid capture case name' >&2; exit 2;; esac
 d=$out/$mode/$case
 capture_rc=0
 rm -rf "$d/files"
 mkdir -p "$d/files"
 truncate -s 0 "$d/data.img"
 truncate -s 1474560 "$d/data.img"
 mformat -i "$d/data.img" -f 1440 -N 19920302 ::
 mmd -i "$d/data.img" ::SUB
 for f in "$out/inputs-$mode"/*; do mcopy -o -i "$d/data.img" "$f" ::; done
 batch=$case
 case "$case" in CFG*) batch=ENV;; esac
 if [ "$mode" = reference ]; then
  cp "$media" "$d/boot.img"
  if [ -n "${DOS33_UTIL_DIR:-}" ]; then
   for name in MORE FIND SORT EDLIN FDISK FORMAT XCOPY TREE LABEL; do
    for ext in COM EXE; do
     f=$DOS33_UTIL_DIR/$name.$ext
     if [ -f "$f" ]; then
      sha256sum "$f" >> "$d/utility.sha256"
      mcopy -o -i "$d/data.img" "$f" ::
     fi
    done
   done
  fi
  required=
  case "$case" in
   FILTER) required='FIND SORT MORE';; MORECASE|U01) required=MORE;;
   U02) required=FIND;; U03) required=SORT;; U05) required=EDLIN;;
   U06) required=FDISK;; U07) required=FORMAT;; U11) required=XCOPY;;
   U12) required=TREE;; U14) required=LABEL;;
  esac
  missing=0
  for name in $required; do
   found=0
   for image in "$d/boot.img" "$d/data.img"; do
    for ext in COM EXE; do
     if mdir -i "$image" "::$name.$ext" >/dev/null 2>&1; then found=1; fi
    done
   done
   if [ "$found" = 0 ]; then printf 'PENDING: genuine 3.30 %s missing\n' "$name" >> "$d/PENDING"; missing=1; fi
  done
  if [ "$missing" = 1 ]; then cat "$d/PENDING"; pending=2; continue; fi
  printf '@echo off\r\nver > A:\\VERSION.TXT\r\npath A:\\\r\nB:\r\ncommand /c %s.BAT one two three four five six seven eight nine ten > TRACE.TXT\r\necho CAPTURE-DONE\r\n' "$batch" > "$d/AUTOEXEC.BAT"
  mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  case "$case" in CFG*)
   printf '@echo off\r\nver > A:\\VERSION.TXT\r\npath A:\\\r\nB:\r\n' > "$d/CONFIG-AUTO.BAT"
   if [ "$case" = CFG2 ]; then
    awk '{sub(/\r$/, ""); if($0=="set")$0="set > TRACE.TXT"; printf "%s\r\n",$0}' "$out/inputs-$mode/ENV.BAT" >> "$d/CONFIG-AUTO.BAT"
   fi
   printf 'echo CAPTURE-DONE\r\n' >> "$d/CONFIG-AUTO.BAT"
   mcopy -o -i "$d/boot.img" "$d/CONFIG-AUTO.BAT" ::AUTOEXEC.BAT
   mcopy -o -i "$d/boot.img" "$out/inputs-$mode/$case.CFG" ::CONFIG.SYS;; esac
  if [ "$case" = FULL ]; then
   free=$(mdir -i "$d/data.img" :: | sed -n 's/^ *\([0-9 ][0-9 ]*\) bytes free$/\1/p' | tr -d ' ')
   truncate -s "$((free - 4096))" "$d/FILL.BIN"
   mcopy -i "$d/data.img" "$d/FILL.BIN" ::FILL.BIN
  fi
  mdir -a -i "$d/data.img" :: > "$d/before.mdir"
  printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
  "$out/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps" > "$d/driver.log" 2>&1 || capture_rc=$?
  mcopy -o -i "$d/boot.img" ::VERSION.TXT "$d/VERSION.TXT" 2>/dev/null || true
 else
  for f in "$root/build/dos/"*.COM; do mcopy -o -i "$d/data.img" "$f" ::; done
  printf '@echo off\r\ncall %s.BAT one two three four five six seven eight nine ten > TRACE.TXT\r\necho CAPTURE-DONE\r\n' "$batch" > "$d/AUTOEXEC.BAT"
  case "$case" in CFG*)
   printf '@echo off\r\n' > "$d/AUTOEXEC.BAT"
   if [ "$case" = CFG2 ]; then
    awk '{sub(/\r$/, ""); if($0=="set")$0="set > TRACE.TXT"; printf "%s\r\n",$0}' "$out/inputs-$mode/ENV.BAT" >> "$d/AUTOEXEC.BAT"
   fi
   printf 'echo CAPTURE-DONE\r\n' >> "$d/AUTOEXEC.BAT"
   mcopy -o -i "$d/data.img" "$out/inputs-$mode/$case.CFG" ::CONFIG.SYS;; esac
  mcopy -o -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  if [ "$case" = FULL ]; then
   free=$(mdir -i "$d/data.img" :: | sed -n 's/^ *\([0-9 ][0-9 ]*\) bytes free$/\1/p' | tr -d ' ')
   truncate -s "$((free - 4096))" "$d/FILL.BIN"
   mcopy -i "$d/data.img" "$d/FILL.BIN" ::FILL.BIN
  fi
  mdir -a -i "$d/data.img" :: > "$d/before.mdir"
  INITECH_QMP_STDIO=1 "$root/build/factory/qemu_harness" --disk "$root/build/image/boot.img" --disk2 "$d/data.img" --out "$d" --name result --timeout-ms "${DOS33_TIMEOUT_MS:-60000}" --expect CAPTURE-DONE --quit-after "$(printf '\nCAPTURE-DONE\r\n')" --rtc-base 1992-03-02T09:30:00 --rtc-deterministic > "$d/driver.log" 2>&1 || capture_rc=$?
 fi
 printf '%s\n' "$capture_rc" > "$d/driver.rc"
 mdir -a -i "$d/data.img" :: > "$d/after.mdir"
 for pattern in '*.OUT' TRACE.TXT LONGFILE.LON LONGFILENAME.LONG 'PIPE*.$$$' 'SUB/NUL*'; do
  mcopy -o -i "$d/data.img" "::$pattern" "$d/files/" 2>/dev/null || true
 done
 fsck_rc=0
 fsck.fat -n "$d/data.img" > "$d/fsck.log" 2>&1 || fsck_rc=$?
 printf '%s\n' "$fsck_rc" > "$d/fsck.rc"
 echo "$mode $case: $(cat "$d/driver.log" | tail -c 160)"
done
exit "$pending"

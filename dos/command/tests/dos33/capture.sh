#!/bin/sh
# Real Packard Bell MS-DOS 3.30; disposable media, existing capture driver.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
out=${1:?absolute capture output directory}
media=${DOS33_BOOT:-"$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img"}
test "$(sha256sum "$media" | cut -d ' ' -f1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
if [ -e "$out" ]; then echo "capture output must be a new directory" >&2;exit 2;fi
mkdir -p "$out"
cc -O2 "$root/goldens/dataloss/drive.c" -o "$out/drive"
cc -O2 "$root/goldens/dataloss/fixtures.c" -o "$out/fixtures"
cc -O2 "$root/dos/command/tests/dos33/batch-fixture.c" -o "$out/batch-fixture"
for scenario in ${DOS33_SCENARIOS:-batch concat append appendpartial final self}; do
 d=$out/$scenario; mkdir -p "$d"
 cp "$media" "$d/boot.img"
 for f in BACKUP.COM RESTORE.COM EGA.CPI DEBUG.COM FC.EXE; do mdel -i "$d/boot.img" "::$f"; done
 truncate -s 1474560 "$d/data.img"
 mformat -i "$d/data.img" -f 1440 -N 19920302 ::
 printf 'bits 16\norg 0x100\nmov ax,0x4c07\nint 0x21\n' > "$out/seven.asm"
 nasm -f bin -o "$out/SEVEN.COM" "$out/seven.asm"
 mcopy -i "$d/boot.img" "$out/SEVEN.COM" ::SEVEN.COM
 "$out/fixtures" "$out/SOURCE.BIN" 131072 SOURCE-0123456789
 "$out/fixtures" "$out/OLD.BIN" 4096 OLD-CONTENT-9876543210
 if [ "$scenario" = batch ]; then
  "$out/batch-fixture" > "$d/AUTOEXEC.BAT"
  printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
  printf 'keep\r\n' > "$d/input-KEEP.DAT"
  printf 'temp\r\n' > "$d/input-JUNK.TMP"
  for spaces in 122 123; do
   mmd -i "$d/data.img" "::MASK$spaces"
   mcopy -i "$d/data.img" "$d/input-KEEP.DAT" "::MASK$spaces/KEEP.DAT"
   mcopy -i "$d/data.img" "$d/input-JUNK.TMP" "::MASK$spaces/JUNK.TMP"
  done
  cp "$d/data.img" "$d/before-data.img"
  gzip -n "$d/before-data.img"
 else
  mcopy -i "$d/data.img" "$out/SOURCE.BIN" ::SOURCE.BIN
  mcopy -i "$d/data.img" "$out/OLD.BIN" ::OLD.BIN
  mcopy -i "$d/data.img" "$out/OLD.BIN" ::TARGET.BIN
  fill=1310208
  case $scenario in final) fill=1314304;; appendpartial) fill=1252864;; esac
  "$out/fixtures" "$out/FILLER.BIN" "$fill" FILLER
  mcopy -i "$d/data.img" "$out/FILLER.BIN" ::FILLER.BIN
  case $scenario in
   concat) cmd='COPY /B OLD.BIN+SOURCE.BIN TARGET.BIN /B';;
   append|appendpartial) cmd='COPY /B TARGET.BIN+SOURCE.BIN /B';;
   final) cmd='COPY OLD.BIN+OLD.BIN TARGET.BIN /A';;
   self) cmd='COPY SOURCE.BIN .\source.bin /B';;
  esac
  printf '%s\r\n' "$cmd" > "$d/COMMAND.TXT"
  printf '@ECHO OFF\r\nVER>A:\\VERSION.TXT\r\nB:\r\nA:\\SEVEN\r\n%s >A:\\COPY.OUT\r\nIF ERRORLEVEL 8 GOTO HIGH\r\nIF ERRORLEVEL 7 GOTO SEVEN\r\nECHO LOW>A:\\STATE.OUT\r\nGOTO SHOW\r\n:HIGH\r\nECHO HIGH>A:\\STATE.OUT\r\nGOTO SHOW\r\n:SEVEN\r\nECHO SEVEN>A:\\STATE.OUT\r\n:SHOW\r\nTYPE A:\\COPY.OUT\r\nECHO CAPTURE-DONE\r\n' "$cmd" > "$d/AUTOEXEC.BAT"
  printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 fi
 mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 mdir -i "$d/data.img" :: > "$d/before.mdir"
 "$out/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps"
 mdir -i "$d/data.img" :: > "$d/after.mdir"
 mkdir -p "$d/files"
 if [ "$scenario" = batch ]; then
  mcopy -i "$d/data.img" '::*.OUT' "$d/files"
  grep -F CAPTURE-DONE "$d/result.txt" >/dev/null
  for spaces in 122 123; do
   mask=MASK$spaces;mkdir -p "$d/$mask"
   mdir -i "$d/data.img" "::$mask" > "$d/$mask/after.mdir"
   mdir -b -i "$d/data.img" "::$mask" | sed 's@.*/@@' | LC_ALL=C sort > "$d/$mask/names"
   while IFS= read -r leaf; do mcopy -i "$d/data.img" "::$mask/$leaf" "$d/$mask/$leaf";done < "$d/$mask/names"
   sed -n "/MASK-$spaces-BEGIN/,/MASK-$spaces-END/p" "$d/result.txt" |
    sed '1d;$d;s/ *$//' | awk 'length {printf "%s\r\n",$0}' > "$d/$mask/DEL.OUT"
  done
 else
  for f in COPY.OUT STATE.OUT VERSION.TXT; do mcopy -i "$d/boot.img" "::$f" "$d/$f"; done
  for f in SOURCE.BIN OLD.BIN TARGET.BIN; do
   if mdir -i "$d/data.img" "::$f" >/dev/null 2>&1; then mcopy -i "$d/data.img" "::$f" "$d/files/$f"; fi
  done
 fi
 gzip -n "$d/data.img"
 gzip -n "$d/qemu.log"
 if [ -f "$d/before.ppm" ]; then gzip -n "$d/before.ppm";fi
 rm "$d/boot.img" "$d/result.ppm" "$d/wait.vga" "$d/wait.txt" 2>/dev/null || :
done

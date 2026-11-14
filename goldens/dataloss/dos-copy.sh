#!/bin/sh
# Real DOS 3.30 COPY, using disposable copies of the pinned OEM media.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
out=${1:?absolute output directory}
media=${DOS33_BOOT:-/home/tobias/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$out"
test "$(sha256sum "$media" | cut -d ' ' -f 1)" = 3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30
sha256sum "$media" > "$out/media.sha256"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/drive.c" -o "$out/drive"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/fixtures.c" -o "$out/fixtures"
"$out/fixtures" "$out/SOURCE.BIN" 131072 SOURCE-0123456789
"$out/fixtures" "$out/OLD.BIN" 4096 OLD-CONTENT-9876543210
touch -t 199203020930 "$out/SOURCE.BIN" "$out/OLD.BIN"
# A 1.44 MB FAT12 has 2847 usable 512-byte clusters.
"$out/fixtures" "$out/FILLER.BIN" 1449472 FILLER
touch -t 199203020930 "$out/FILLER.BIN"
for scenario in new overwrite newbinary overwritebinary newzero overwritezero self selfalias ascii asciifinal; do
 d=$out/$scenario; mkdir -p "$d"
 cp "$media" "$d/boot.img"
 for f in BACKUP.COM RESTORE.COM EGA.CPI DEBUG.COM FC.EXE; do mdel -i "$d/boot.img" "::$f"; done
 mcopy -i "$d/boot.img" "$out/SOURCE.BIN" ::SOURCE.BIN
 truncate -s 1474560 "$d/data.img"
 mformat -i "$d/data.img" -f 1440 -N 19920302 ::
 free=8192; old=0
 case "$scenario" in overwrite*) old=4096; mcopy -i "$d/data.img" "$out/OLD.BIN" ::TARGET.BIN;; esac
 case "$scenario" in *zero) free=0;; esac
 "$out/fixtures" "$d/FILL.BIN" "$((1457664-free-old))" FILLER
 touch -t 199203020930 "$d/FILL.BIN"
 mcopy -i "$d/data.img" "$d/FILL.BIN" ::FILLER.BIN
 case "$scenario" in
  newbinary|overwritebinary) copy='COPY A:\SOURCE.BIN B:\TARGET.BIN /B' ;;
  new|overwrite|newzero|overwritezero) copy='COPY A:\SOURCE.BIN B:\TARGET.BIN' ;;
  self) copy='COPY A:\SOURCE.BIN A:\SOURCE.BIN /B' ;;
  selfalias) copy='COPY A:\SOURCE.BIN A:\.\source.bin /B' ;;
  ascii) copy='COPY A:\SOURCE.BIN B:\TARGET.BIN /A' ;;
  asciifinal) "$out/fixtures" "$d/SHORT.BIN" 8192 SHORT-0123456789; mcopy -i "$d/boot.img" "$d/SHORT.BIN" ::SHORT.BIN; copy='COPY A:\SHORT.BIN B:\TARGET.BIN /A' ;;
 esac
 mdir -i "$d/data.img" :: > "$d/before.mdir"
 printf '@ECHO OFF\r\nVER > A:\\VERSION.TXT\r\n%s > A:\\COPY.OUT\r\nTYPE A:\\COPY.OUT\r\nDIR B:\r\nECHO CAPTURE-DONE\r\n' "$copy" > "$d/AUTOEXEC.BAT"
 mcopy -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 printf 'wait CAPTURE-DONE\nshot result\nend\n' > "$d/steps"
 "$out/drive" "$d" "$d/boot.img" "$d/data.img" - "$d/steps"
 mcopy -i "$d/boot.img" ::COPY.OUT "$d/COPY.OUT"
 mcopy -i "$d/boot.img" ::VERSION.TXT "$d/VERSION.TXT"
 mcopy -i "$d/boot.img" ::SOURCE.BIN "$d/SOURCE.BIN"
 mdir -i "$d/data.img" :: > "$d/after.mdir"
 if mdir -i "$d/data.img" ::TARGET.BIN >/dev/null 2>&1; then mcopy -i "$d/data.img" ::TARGET.BIN "$d/TARGET.BIN"; fi
 convert "$d/result.ppm" "$d/result.png"
done

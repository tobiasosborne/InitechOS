#!/bin/sh
# Real III+ under the pinned DOS33 runtime; goldens/dataloss supplies the driver.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
out=${1:?absolute fresh output directory}
ref=${DBASE3_DECOMP:-$HOME/Projects/dbase3-decomp}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$out"
cc -O2 -std=c11 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
truncate -s 33554432 "$out/runtime.img"
printf 'label: dos\nlabel-id: 0x19920302\nunit: sectors\n\n63,65457,4,*\n' | sfdisk "$out/runtime.img" > "$out/partition.log"
mformat -i "$out/runtime.img@@32256" -T 65457 -h 16 -s 63 -c 4 -r 32 -H 63 -N 19920302 ::
for f in DBASE.EXE DBASE.OVL DBASE.MSG DBASEINL.OVL; do
 mcopy -i "$out/runtime.img@@32256" "$ref/mint/work/$f" ::
 sha256sum "$ref/mint/work/$f" >> "$out/reference.sha256"
done
printf 'STATUS=OFF\r\nBELL=OFF\r\n' > "$out/CONFIG.DB"
mcopy -i "$out/runtime.img@@32256" "$out/CONFIG.DB" ::
mcopy -i "$out/runtime.img@@32256" "$ref/mint/work/CLIENTS.DBF" ::
for scenario in ${SCENARIOS:-control full copy save index alias}; do
 d=$out/$scenario; mkdir -p "$d"; cp "$media" "$d/boot.img"; cp "$out/runtime.img" "$d/runtime.img"
 truncate -s 1474560 "$d/data.img"; mformat -i "$d/data.img" -f 1440 -N 19920302 ::
 mcopy -i "$d/data.img" "$ref/mint/work/CLIENTS.DBF" ::BASE.DBF
 printf 'PREVIOUS-FILE\r\n' > "$d/OLD.TXT"
 mcopy -i "$d/data.img" "$d/OLD.TXT" ::TARGET.TXT
 mcopy -i "$d/data.img" "$d/OLD.TXT" ::TARGET.MEM
 if [ "$scenario" != control ]; then
  free=$(mdir -i "$d/data.img" :: | sed -n 's/ //g; s/bytesfree//p')
  truncate -s "$free" "$d/FILLER.BIN"; mcopy -i "$d/data.img" "$d/FILLER.BIN" ::FILLER.BIN
 fi
 cat > "$d/TEST.PRG" <<'PRG'
SET TALK OFF
SET SAFETY OFF
SET ALTERNATE TO TRACE.TXT
SET ALTERNATE ON
ON ERROR ? 'ERR',ERROR()
USE B:BASE
? 'BEFORE',RECCOUNT(),FIRSTNAME
REPLACE FIRSTNAME WITH 'PRESERVED'
DO WHILE RECCOUNT()<200
APPEND BLANK
ENDDO
USE
USE B:BASE
? 'REOPEN',RECCOUNT(),FIRSTNAME
USE
COPY FILE CLIENTS.DBF TO B:TARGET.TXT
STORE 'NEW-VALUE' TO SENTINEL
STORE SPACE(254) TO SECOND
STORE SPACE(254) TO THIRD
STORE SPACE(254) TO FOURTH
SAVE TO B:TARGET
? 'DONE'
SET ALTERNATE TO
QUIT
PRG
 case "$scenario" in
 copy) sed -i "/REPLACE FIRSTNAME/,/USE$/d; s/COPY FILE CLIENTS.DBF TO B:TARGET.TXT/COPY FILE CLIENTS.DBF TO B:TARGET.TXT/" "$d/TEST.PRG";;
 save) sed -i "/REPLACE FIRSTNAME/,/COPY FILE/d" "$d/TEST.PRG";;
 index) sed -i "/REPLACE FIRSTNAME/,$ d" "$d/TEST.PRG"; printf "INDEX ON FIRSTNAME TO B:OLD\nREINDEX\nUSE\nQUIT\n" >> "$d/TEST.PRG";;
 alias) sed -i "/REPLACE FIRSTNAME/,$ d" "$d/TEST.PRG"; printf "USE\nCOPY FILE B:TARGET.TXT TO B:TARGET.TXT\nQUIT\n" >> "$d/TEST.PRG";;
 esac
 mcopy -i "$d/runtime.img@@32256" "$d/TEST.PRG" ::
 printf '@ECHO OFF\r\nC:\r\nDBASE TEST\r\nECHO CAPTURE-DONE\r\n' > "$d/AUTOEXEC.BAT"
 mcopy -o -i "$d/boot.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 printf 'FILES=40\r\nBUFFERS=20\r\n' > "$d/CONFIG.SYS"; mcopy -o -i "$d/boot.img" "$d/CONFIG.SYS" ::CONFIG.SYS
 printf 'sleep 3\nkey ret\nsleep 8\nshot running\n' > "$d/steps"
 if [ "$scenario" != control ]; then
  printf "type A\nkey ret\nsleep 1\ntype use\nkey ret\ntype use b:base\nkey ret\ntype ? reccount()\nkey ret\nsleep 1\nshot reopened\n" >> "$d/steps"
 fi
 printf 'shot result\nend\n' >> "$d/steps"
 "$out/drive" "$d" "$d/boot.img" "$d/data.img" "$d/runtime.img" "$d/steps"
 mdir -i "$d/data.img" :: > "$d/after.mdir"
 mcopy -i "$d/runtime.img@@32256" ::TRACE.TXT "$d/TRACE.TXT" || :
 for f in BASE.DBF TARGET.TXT TARGET.MEM; do mcopy -i "$d/data.img" "::$f" "$d/$f" || :; done
 for f in "$d"/*.ppm; do convert "$f" "${f%.ppm}.png"; done
done

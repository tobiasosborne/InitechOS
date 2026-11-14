#!/bin/sh
# Real III+ reopens output from the booted product, plus the host memo codec.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
out=${1:?absolute output directory}; product=${2:?smoke output directory}; core=${3:?host core fixture directory}
ref=${DBASE3_DECOMP:-$HOME/Projects/dbase3-decomp}
media=${DOS33_BOOT:-$HOME/Projects/initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img}
mkdir -p "$out"
cc -O2 -std=c11 -Wall -Wextra -Werror "$root/goldens/dataloss/drive.c" -o "$out/drive"
truncate -s 33554432 "$out/runtime.img"
printf 'label: dos\nlabel-id: 0x19920302\nunit: sectors\n\n63,65457,4,*\n' | sfdisk "$out/runtime.img" > "$out/partition.log"
mformat -i "$out/runtime.img@@32256" -T 65457 -h 16 -s 63 -c 4 -r 32 -H 63 -N 19920302 ::
for f in DBASE.EXE DBASE.OVL DBASE.MSG DBASEINL.OVL; do mcopy -i "$out/runtime.img@@32256" "$ref/mint/work/$f" ::; done
truncate -s 1474560 "$out/data.img"; mformat -i "$out/data.img" -f 1440 -N 19920302 ::
for f in BASE.DBF RESULT.DBF NAMES.NDX REPAIRED.DBF STALE.NDX; do mcopy -i "$out/data.img" "$product/$f" ::; done
mcopy -i "$out/data.img" "$core/build/core-memo.dbf" ::MEMO.DBF
mcopy -i "$out/data.img" "$core/build/core-memo.dbt" ::MEMO.DBT
mcopy -i "$out/data.img" "$core/build/FLUSHERR.DBF" ::FLUSHERR.DBF
cat > "$out/TEST.PRG" <<'PRG'
SET TALK OFF
SET SAFETY OFF
SET ALTERNATE TO TRACE.TXT
SET ALTERNATE ON
ON ERROR ? 'REOPEN-ERROR',ERROR()
USE B:RESULT
GO TOP
IF RECCOUNT()=5 .AND. CODE='ZZ' .AND. QTY=100 .AND. OK
GO 2
IF DELETED() .AND. CODE='BBB' .AND. QTY=200 .AND. .NOT. OK
GO 3
IF CODE='CCC' .AND. QTY=100 .AND. OK
GO 4
IF CODE='DDD' .AND. QTY=300 .AND. OK
GO 5
IF CODE='XX' .AND. QTY=999
? 'RECORDS-OK'
ENDIF
ENDIF
ENDIF
ENDIF
ENDIF
USE B:BASE INDEX B:NAMES
SEEK 'LUMBERGH'
IF FOUND() .AND. RECNO()=3 .AND. NAME='LUMBERGH'
SEEK 'PESTON'
IF FOUND() .AND. RECNO()=1 .AND. NAME='PESTON'
? 'INDEX-OK'
ENDIF
ENDIF
USE B:FLUSHERR
IF RECCOUNT()=9 .AND. TITLE="Business Expenses"
? "PRESERVED-OK"
ENDIF
USE B:REPAIRED
GO 1
IF RECCOUNT()=2 .AND. KEY='AAAAAAAA' .AND. VALUE=1
GO 2
IF KEY='BBBBBBBB' .AND. VALUE=9
? 'C21-RECORDS-OK'
ENDIF
ENDIF
USE B:REPAIRED INDEX B:STALE
SEEK 'AAAAAAAA'
IF FOUND() .AND. RECNO()=1 .AND. KEY='AAAAAAAA' .AND. VALUE=1
SEEK 'BBBBBBBB'
IF FOUND() .AND. RECNO()=2 .AND. KEY='BBBBBBBB' .AND. VALUE=9
? 'C21-INDEX-OK'
ENDIF
ENDIF
USE B:MEMO
IF RECCOUNT()=1
? NOTE
? 'MEMO-OK'
ENDIF
USE
SET ALTERNATE TO
QUIT
PRG
mcopy -i "$out/runtime.img@@32256" "$out/TEST.PRG" ::
printf 'STATUS=OFF\r\nBELL=OFF\r\n' > "$out/CONFIG.DB"; mcopy -i "$out/runtime.img@@32256" "$out/CONFIG.DB" ::
cp "$media" "$out/boot.img"
printf '@ECHO OFF\r\nC:\r\nDBASE TEST\r\nECHO REOPEN-DONE\r\n' > "$out/AUTOEXEC.BAT"; mcopy -o -i "$out/boot.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'FILES=40\r\nBUFFERS=20\r\n' > "$out/CONFIG.SYS"; mcopy -o -i "$out/boot.img" "$out/CONFIG.SYS" ::CONFIG.SYS
printf 'sleep 3\nkey ret\nwait REOPEN-DONE\nshot result\nend\n' > "$out/steps"
"$out/drive" "$out" "$out/boot.img" "$out/data.img" "$out/runtime.img" "$out/steps"
mcopy -i "$out/runtime.img@@32256" ::TRACE.TXT "$out/TRACE.TXT"
tr -d '\r\032' < "$out/TRACE.TXT" > "$out/trace.txt"
for marker in RECORDS-OK INDEX-OK MEMO-OK PRESERVED-OK C21-RECORDS-OK C21-INDEX-OK; do grep -Fx "$marker" "$out/trace.txt"; done
memo_line=$(printf 'hello%45s' '')
grep -Fx "$memo_line" "$out/trace.txt"
if grep -F REOPEN-ERROR "$out/trace.txt"; then exit 1; fi
echo 'GREEN: real III+ reopened target DBF/NDX, repaired C21 pair and host DBF/DBT with exact values'

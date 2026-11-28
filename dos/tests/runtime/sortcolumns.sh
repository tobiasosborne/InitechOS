#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
. "$root/dos/tests/runtime/utility-session.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" "$programs/SORT.COM" ::
printf 'b2\r\na3\r\nc1\r\nq\r\n' > "$out/input"
printf 'q\r\nc1\r\nb2\r\na3\r\n' > "$out/column"
printf 'a3\r\nb2\r\nc1\r\nq\r\n' > "$out/reverse"
mcopy -i "$out/data.img" "$out/input" ::INPUT.TXT
printf 'Z\r\n\032a\r\nx\r\n' > "$out/EOF.TXT"
printf 'Z\r\n' > "$out/eof-expected"
mcopy -i "$out/data.img" "$out/EOF.TXT" ::
printf '%s\r\n' '@echo off' 'SORT <INPUT.TXT >DEFAULT.OUT' 'SORT /+1 <INPUT.TXT >FIRST.OUT' \
 'SORT /+2 <INPUT.TXT >COLUMN.OUT' 'SORT /R /+2 <INPUT.TXT >REVERSE.OUT' \
 'SORT /Z <INPUT.TXT >BAD.OUT' 'if errorlevel 1 echo REFUSED >BAD.RC' \
 'SORT <EOF.TXT >EOF.OUT' DONE > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
utility_session "$boot" "$out/data.img" "$out" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE \
 --timeout-ms 25000 --name sortcolumns > "$out/driver.log" 2> "$out/report" || error 'SORT columns transport failed'
utility_transport "$out/report"
for spec in 'DEFAULT.OUT reverse' 'FIRST.OUT reverse' 'COLUMN.OUT column' 'REVERSE.OUT reverse'; do
 mtype -i "$out/data.img" "::${spec%% *}" > "$out/actual" || error 'SORT output missing'
 require 'SORT uses the documented starting column' cmp "$out/${spec##* }" "$out/actual"
done
# DOS 3.30 SORTEOF keeps only the line before Ctrl-Z.
mtype -i "$out/data.img" ::EOF.OUT > "$out/actual" || error 'SORT EOF output missing'
require 'SORT stops at Ctrl-Z' cmp "$out/eof-expected" "$out/actual"
mtype -i "$out/data.img" ::BAD.OUT > "$out/actual" || error 'SORT diagnostic missing'
tr -d '\r' < "$out/actual" > "$out/text"
require 'SORT refuses unsupported switch' grep -Fx 'Invalid parameter' "$out/text"
mtype -i "$out/data.img" ::BAD.RC > "$out/status" || error 'SORT status missing'
tr -d '\r' < "$out/status" > "$out/text"
require 'SORT unsupported switch sets failure status' grep -Fx 'REFUSED' "$out/text"
echo 'GREEN SORT: manual column examples and explicit unsupported-switch refusal'

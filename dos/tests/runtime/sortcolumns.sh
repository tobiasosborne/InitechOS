#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" "$programs/SORT.COM" ::
printf 'b2\r\na3\r\nc1\r\nq\r\n' > "$out/input"
printf 'q\r\nc1\r\nb2\r\na3\r\n' > "$out/column"
printf 'a3\r\nb2\r\nc1\r\nq\r\n' > "$out/reverse"
mcopy -i "$out/data.img" "$out/input" ::INPUT.TXT
printf '@echo off\r\nSORT <INPUT.TXT >DEFAULT.OUT\r\nSORT /+1 <INPUT.TXT >FIRST.OUT\r\nSORT /+2 <INPUT.TXT >COLUMN.OUT\r\nSORT /R /+2 <INPUT.TXT >REVERSE.OUT\r\nSORT /Z <INPUT.TXT >BAD.OUT\r\nif errorlevel 1 echo REFUSED >BAD.RC\r\nDONE\r\n' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
if ! timeout 45 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 25000 --out "$out" --name sortcolumns > "$out/driver.log" 2> "$out/report"; then error 'pipe CWD transport failed'; fi
transport_pass "$out/report"
for spec in 'DEFAULT.OUT reverse' 'FIRST.OUT reverse' 'COLUMN.OUT column' 'REVERSE.OUT reverse'; do
 mtype -i "$out/data.img" "::${spec%% *}" > "$out/actual" || error 'SORT output missing'
 require 'SORT uses the documented starting column' cmp "$out/${spec##* }" "$out/actual"
done
mtype -i "$out/data.img" ::BAD.OUT > "$out/actual" || error 'SORT diagnostic missing'
tr -d '\r' < "$out/actual" > "$out/text"
require 'SORT refuses unsupported switch' grep -Fx 'Invalid parameter' "$out/text"
mtype -i "$out/data.img" ::BAD.RC > "$out/status" || error 'SORT status missing'
tr -d '\r' < "$out/status" > "$out/text"
require 'SORT unsupported switch sets failure status' grep -Fx 'REFUSED' "$out/text"
echo 'GREEN SORT: manual column examples and explicit unsupported-switch refusal'

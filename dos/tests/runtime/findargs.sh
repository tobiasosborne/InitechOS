#!/bin/sh
# initech-iuuni: FIND accepts nothing silently. Unknown switches, file
# operands and empty strings refuse with the period parameter diagnostic
# and nonzero ERRORLEVEL, as SORT refuses unsupported switches
# (GOLDEN-MASTER item 2). Genuine FIND media are missing (initech-0e5gu):
# the diagnostic (MSG-DOS-0059, MS-DOS 3.3 User's Guide p.341, already
# FIND's missing-string message) and status 1 are stated assumptions.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?output}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/DONE.COM" "$programs/FIND.COM" ::
printf 'apple\r\nbanana\r\nAPPLE pie\r\n' > "$out/words"
printf 'apple\r\n' > "$out/ok"
printf '2\r\n' > "$out/count"
mcopy -i "$out/data.img" "$out/words" ::WORDS.TXT
printf '@echo off\r\nFIND "apple" <WORDS.TXT >OK.OUT\r\nFIND /C /I "APPLE" <WORDS.TXT >CNT.OUT\r\nFIND /Z "apple" <WORDS.TXT >BAD1.OUT\r\nif errorlevel 1 echo REFUSED >BAD1.RC\r\nFIND "apple" WORDS.TXT <WORDS.TXT >BAD2.OUT\r\nif errorlevel 1 echo REFUSED >BAD2.RC\r\nFIND "" <WORDS.TXT >BAD3.OUT\r\nif errorlevel 1 echo REFUSED >BAD3.RC\r\nDONE\r\n' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
if ! timeout 45 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 25000 --out "$out" --name findargs > "$out/driver.log" 2> "$out/report"; then error 'FIND args transport failed'; fi
transport_pass "$out/report"
mtype -i "$out/data.img" ::OK.OUT > "$out/actual" || error 'FIND valid output missing'
require 'FIND valid stdin form still filters' cmp "$out/ok" "$out/actual"
mtype -i "$out/data.img" ::CNT.OUT > "$out/actual" || error 'FIND count output missing'
require 'FIND /C /I still counts case-insensitively' cmp "$out/count" "$out/actual"
for n in 1 2 3; do
 mtype -i "$out/data.img" ::BAD$n.OUT > "$out/actual" || error "FIND BAD$n diagnostic missing"
 tr -d '\r' < "$out/actual" > "$out/text"
 require "FIND refuses bad argument $n with period diagnostic" grep -Fx 'Invalid number of parameters' "$out/text"
 mtype -i "$out/data.img" ::BAD$n.RC > "$out/status" || error "FIND BAD$n status missing"
 tr -d '\r' < "$out/status" > "$out/text"
 require "FIND bad argument $n sets failure status" grep -Fx 'REFUSED' "$out/text"
done
echo 'GREEN FIND: valid forms work; unknown switch, file operand and empty string refused with nonzero status'

#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${4:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
printf 'CHILD-CONTENT\r\n' > "$out/A.TXT"
mcopy -o -i "$out/data.img" "$out/A.TXT" ::A.TXT
printf '@echo off\r\ntype A.TXT\r\n' > "$out/CHILD.BAT"
mcopy -o -i "$out/data.img" "$out/CHILD.BAT" ::CHILD.BAT
printf '@echo off\r\ncall CHILD.BAT > TRACE.TXT\r\ncall NOPE.BAT > NOPE.OUT\r\necho END > END.TXT\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::DONE.COM
printf 'done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --out "$out" --name callredir > "$out/driver.log" 2> "$out/report"; then error 'callredir transport did not complete'; fi
transport_pass "$out/report"
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace" 2>/dev/null || semantic 'CALL with redirection ran the child into TRACE'
require 'CALL with redirection ran the child into TRACE' cmp "$out/A.TXT" "$out/trace"
mtype -i "$out/data.img" ::NOPE.OUT > "$out/nope" 2>/dev/null || semantic 'missing CALL created its redirect target'
require 'missing CALL target is created empty' test ! -s "$out/nope"
tr -d '\r' < "$out/callredir.serial" > "$out/serial-text"
require 'only the missing CALL reports Bad command' \
  test "$(grep -c 'Bad command or file name' "$out/serial-text")" -eq 1
echo 'GREEN callredir: CALL honors redirection; missing CALL creates empty target and reports once'

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
printf '@echo off\r\nbreak off\r\necho BEGIN > TRACE.TXT\r\nbreak >> TRACE.TXT\r\nbreak on >> TRACE.TXT\r\nbreak >> TRACE.TXT\r\nbreak maybe >> TRACE.TXT\r\nbreak >> TRACE.TXT\r\nbreak off extra >> TRACE.TXT\r\nbreak >> TRACE.TXT\r\nbreak off >> TRACE.TXT\r\nbreak >> TRACE.TXT\r\necho END >> TRACE.TXT\r\n' > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::DONE.COM
printf 'done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --out "$out" --name break > "$out/driver.log" 2> "$out/report"; then error 'break transport did not complete'; fi
transport_pass "$out/report"
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace" || error 'break trace missing'
tr -d '\r' < "$out/trace" | grep -v '^$' > "$out/text"
printf 'BEGIN\nBREAK is off\nBREAK is on\nBREAK is on\nBREAK is on\nBREAK is off\nEND\n' > "$out/expected"
require 'captured BREAK state sequence on stdout' cmp "$out/expected" "$out/text"
tr -d '\r' < "$out/break.serial" > "$out/serial-text"
require 'invalid BREAK operands report Must specify ON or OFF on CON twice' \
  test "$(grep -c 'Must specify ON or OFF' "$out/serial-text")" -eq 2
forbid 'invalid BREAK diagnostic stays off stdout' grep -F 'Must specify' "$out/text"
echo 'GREEN break: captured BREAK operand grammar, CON diagnostic, silent sets'

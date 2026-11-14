#!/bin/sh
set -eu
root=${1:?root}
out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/compat33/expected/REDIR
gzip -dc "$ref/data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
if ! timeout 40 build/factory/qemu_harness --disk build/image/boot.img --disk2 "$out/data.img" --keys-after SHELL-READY --keys "$(cat "$out/keys")" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 15000 --rtc-base 1987-07-06T13:30:58 --out "$out" --name unknown > "$out/driver.log" 2> "$out/report";then error 'unknown-command transport did not complete';fi
transport_pass "$out/report"
mdir -a -b -i "$out/data.img" :: > "$out/names"
forbid 'unknown command created BAD.OUT' grep -F /BAD.OUT "$out/names"
tr -d '\r' < "$out/unknown.serial" > "$out/text"
grep -F 'Bad command or file name' "$ref/result.txt" >/dev/null
require 'unknown command diagnostic on CON' grep -Fx 'Bad command or file name' "$out/text"
echo 'GREEN unknown command: refusal on CON, no redirection file'

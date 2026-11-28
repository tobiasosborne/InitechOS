#!/bin/sh
set -eu
root=${1:?root} out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/dos33/reference/syntax
gzip -dc "$ref/before-data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::SUB/DONE.COM
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
printf 'sub\\done\n' | build/factory/keys > "$out/keys"
DOS_SESSION_RTC=1987-07-06T13:30:58 sh dos/tests/runtime/execute.sh "$root" "${DOS_SESSION_BOOT:-build/image/boot.img}" \
    "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'control-syntax transport did not complete'
pattern='^(C[0-9]+|Syntax error|Bad command or file name)[[:space:]]*$'
grep -E "$pattern" "$ref/result.txt" | sed 's/ *$//' > "$out/expected-console"
grep -E "$pattern" "$out/text" > "$out/actual-console"
require 'captured IF/FOR diagnostics and silent controls on CON' cmp "$out/expected-console" "$out/actual-console"
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace"
require 'captured IF/FOR body decisions and clean stdout' cmp "$ref/TRACE.TXT" "$out/trace"
echo 'GREEN IF/FOR syntax: captured CON diagnostics and body/skip decisions'

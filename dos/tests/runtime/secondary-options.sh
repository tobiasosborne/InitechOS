#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/secondary-options
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
sed 's/$/\r/' "$ref/environment.txt" > "$out/TESTE.BAT"
sed 's/$/\r/' "$ref/environment-large.txt" > "$out/TESTF.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/TESTE.BAT" "$out/TESTF.BAT" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'secondary COMMAND option transport failed'
awk '/^(SMALL|LARGE|HUGE|HUGE-ACCEPTED|HUGE-REFUSED|Out of environment space|COMPACT|SEPARATE|OPTIONS-DONE|Invalid number of parameters)$/ { print }' "$out/text" > "$out/actual"
require 'COMMAND /E controls capacity and /C accepts a compact tail' cmp "$ref/result.txt" "$out/actual"
echo 'GREEN secondary COMMAND environment size and compact /C'

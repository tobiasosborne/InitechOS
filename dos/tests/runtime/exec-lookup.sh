#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/exec-lookup
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
mmd -i "$out/data.img" ::BIN ::FALL
: > "$out/BROKEN.EXE"
sed 's/$/\r/' "$ref/batch.txt" > "$out/BATCH.BAT"
mcopy -i "$out/data.img" "$out/BATCH.BAT" "$out/BROKEN.EXE" ::BIN
mcopy -i "$out/data.img" "$out/BATCH.BAT" ::FALL/BROKEN.BAT
printf '%s\r\n' '@echo off' 'PATH A:\BIN;A:\FALL' 'ECHO LOOKUP-READY' > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
printf '%s\n' 'a:\bin\batch' 'a:\bin\broken' 'broken' 'done' | build/factory/keys > "$out/keys"
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'executable lookup transport failed'
awk '/^(QUALIFIED-BATCH|Error in EXE file|Bad command or file name)$/ { print }' "$out/text" > "$out/actual"
require 'qualified suffix search and load error precedence match DOS 3.30' cmp "$ref/lookup.txt" "$out/actual"
echo 'GREEN COMMAND finds qualified batches and reports the first EXEC failure'

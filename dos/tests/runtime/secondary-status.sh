#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/secondary-status
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
for status in 0 7; do
    nasm -f bin -DTARGET_BITS=32 -DEXIT_STATUS="$status" "$ref/exit.asm" -o "$out/ERR$status.COM"
done
sed 's/$/\r/' "$ref/batch.txt" > "$out/STATUS.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'ECHO STATUS-READY' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" "$out/STATUS.BAT" "$out/ERR0.COM" "$out/ERR7.COM" ::
printf 'status\ndone\n' | build/factory/keys > "$out/keys"
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'secondary COMMAND status transport failed'
awk '/^(DIRECT7|(CHILD|PRIOR|BUILTIN|NEST)[07]|CHILD-INHERITED)$/ { print }' "$out/text" > "$out/actual"
require 'secondary COMMAND returns DOS 3.30 status 0' cmp "$ref/status.txt" "$out/actual"
echo 'GREEN COMMAND /C starts and returns with status 0'

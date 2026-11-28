#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/ctty
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
printf 'Ordinary file\r\n' > "$out/FILE.TXT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/FILE.TXT" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'CTTY transport failed'
awk '/^(CTTY-(BEFORE|AFTER|HIDDEN|DONE)|Invalid device|Bad command or file name)$/ { print }' "$out/text" > "$out/actual"
require 'CTTY changes the terminal and refuses non-devices' cmp "$ref/ctty.txt" "$out/actual"
echo 'GREEN CTTY changes device and restores the console'

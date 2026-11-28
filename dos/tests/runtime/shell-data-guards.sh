#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/data-guards
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'shell data guard transport failed'
awk '/^(File creation error|OLD-BYTES|REPLACED .*|Bad command or file name)$/ { print }' "$out/text" > "$out/actual"
require 'repeated output redirection refuses before overwriting' cmp "$ref/console.txt" "$out/actual"
mtype -i "$out/data.img" ::TOUT > "$out/tout" || error 'cannot read TOUT'
printf 'OLD-BYTES\r\n' > "$out/expected-tout"
require 'TOUT retained the original bytes' cmp "$out/expected-tout" "$out/tout"
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read disk inventory'
forbid 'repeated output redirection created MISS' grep -Fx '::/MISS' "$out/names"
echo 'GREEN shell data guards preserve DOS files'

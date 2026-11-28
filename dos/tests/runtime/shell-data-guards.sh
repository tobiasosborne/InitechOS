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
mmd -i "$out/data.img" ::EMPTY
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'shell data guard transport failed'
mtype -i "$out/data.img" ::TOUT > "$out/tout" || error 'cannot read TOUT'
printf 'OLD-BYTES\r\n' > "$out/expected-tout"
require 'TOUT retained the original bytes' cmp "$out/expected-tout" "$out/tout"
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read disk inventory'
forbid 'repeated output redirection created MISS' grep -Fx '::/MISS' "$out/names"
for name in KILL.ONE KILL.TWO; do
    require "DEL with two operands preserved $name" grep -Fx "::/$name" "$out/names"
done
mtype -i "$out/data.img" ::SAFE.OUT > "$out/safe" || error 'cannot read SAFE.OUT'
printf 'XX\r\n' > "$out/expected-safe"
require 'redirected pipe producer preserved SAFE.OUT' cmp "$out/expected-safe" "$out/safe"
forbid 'redirected pipe producer allowed the next stage to run' grep -Fx END "$out/text"
forbid 'MD with an extra operand created NEW' grep -Fx '::/NEW/' "$out/names"
require 'RD with an extra operand preserved EMPTY' grep -Fxq '::/EMPTY/' "$out/names"
awk '/^(File creation error|OLD-BYTES|REPLACED .*|Bad command or file name|Invalid number of parameters|XX)$/ { print }' "$out/text" > "$out/actual"
require 'shell refusals match DOS 3.30' cmp "$ref/console.txt" "$out/actual"
echo 'GREEN shell data guards preserve DOS files'

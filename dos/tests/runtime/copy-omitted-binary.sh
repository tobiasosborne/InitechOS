#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/copy-omitted-binary
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
for file in FIRST SECOND THIRD FOURTH; do
    printf 'AB\032C' > "$out/$file.BIN"
    mcopy -i "$out/data.img" "$out/$file.BIN" ::
done
printf x > "$out/ONE.BIN"
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" "$out/ONE.BIN" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'omitted-target binary COPY transport failed'
require 'COPY batch reached its end' grep -Fx BINARY-DONE "$out/text" >/dev/null
for file in FIRST SECOND THIRD FOURTH; do
    mtype -i "$out/data.img" "::$file.BIN" > "$out/bytes" || error "cannot read $file.BIN"
    printf '%s: ' "$file"
    od -An -v -tx1 "$out/bytes" | sed 's/^ *//'
done > "$out/actual"
require 'omitted-target COPY preserves binary bytes and the captured EOF rule' cmp "$ref/outputs.txt" "$out/actual"
echo 'GREEN omitted-target COPY bytes match DOS 3.30'

#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/verify
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin -DTARGET_BITS=32 "$ref/probe.asm" -o "$out/VSTATE.COM"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/VSTATE.COM" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'VERIFY transport failed'
awk '/^(VERIFY is (off|on)|VERIFY-(GATE-[A-Z-]+|DONE)|Must specify ON or OFF|Invalid parameter|Invalid function|Bad command or file name)$/ { print }' "$out/text" > "$out/actual"
require 'VERIFY sets and queries the kernel flag' cmp "$ref/result.txt" "$out/actual"
mtype -i "$out/data.img" ::CHECK.TXT > "$out/written"
printf 'checked\r\n' > "$out/expected"
require 'write with VERIFY ON persisted the captured bytes' cmp "$out/expected" "$out/written"
echo 'GREEN VERIFY kernel state and verified write'

#!/bin/sh
# Drive the captured SHIFT and nested CALL session through resident COMMAND.
set -eu
root=${1:?root} boot=${2:?boot} out=${4:?output}
mkdir -p "$out"
out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
cd "$root"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/dos33/reference/shift-window
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
for name in PARENT CHILD; do
    awk '{printf "%s\r\n",$0}' "$ref/$name.txt" > "$out/$name.BAT"
    mcopy -i "$out/data.img" "$out/$name.BAT" ::
done
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'echo SHIFT-BEGIN' "call PARENT.BAT$(cat "$ref/tail.txt")" \
    'echo SHIFT-END' DONE > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/DONE.COM" "$out/AUTOEXEC.BAT" ::
timeout 55 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" \
    --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 40000 \
    --out "$out" --name shift > "$out/driver.log" 2> "$out/report" ||
    error 'SHIFT transport failed'
transport_pass "$out/report"
tr -d '\r' < "$out/shift.serial" > "$out/text"
require 'SHIFT reached the start marker' grep -Fxq SHIFT-BEGIN "$out/text"
require 'SHIFT reached the end marker' grep -Fxq SHIFT-END "$out/text"
sed -n '/^SHIFT-BEGIN$/,/^SHIFT-END$/{ /^SHIFT-/d; /^$/d; p; }' "$out/text" > "$out/trace.txt"
require 'SHIFT positions match the captured parent and nested child windows' cmp "$ref/trace.txt" "$out/trace.txt"
echo 'GREEN SHIFT: late operands, nested CALL and exhaustion match DOS 3.30'

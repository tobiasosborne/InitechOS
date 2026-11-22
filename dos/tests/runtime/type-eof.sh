#!/bin/sh
# Compare console and redirected TYPE with the pinned DOS 3.30 capture.
set -eu
root=${1:?root} boot=${2:?boot} out=${4:?output}
mkdir -p "$out"
out=$(realpath "$out")
. "$root/dos/tests/runtime/verdict.sh"
cd "$root"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/dos33/reference/type-eof
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
mcopy -i "$out/data.img" "$ref/input.bin" ::PADDED.TXT
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'echo TYPE-SCREEN-BEGIN' 'TYPE PADDED.TXT' \
    'echo TYPE-SCREEN-END' 'TYPE PADDED.TXT>PADOUT.TXT' 'DONE' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/DONE.COM" "$out/AUTOEXEC.BAT" ::
timeout 55 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" \
    --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 40000 \
    --out "$out" --name type > "$out/driver.log" 2> "$out/report" ||
    error 'TYPE transport failed'
transport_pass "$out/report"
tr -d '\r' < "$out/type.serial" > "$out/text"
require 'TYPE reached the start marker' grep -Fxq TYPE-SCREEN-BEGIN "$out/text"
require 'TYPE reached the end marker' grep -Fxq TYPE-SCREEN-END "$out/text"
sed -n '/^TYPE-SCREEN-BEGIN$/,/^TYPE-SCREEN-END$/{ /^TYPE-SCREEN-/d; /^$/d; p; }' \
    "$out/text" > "$out/display.txt"
require 'TYPE console stops before Ctrl-Z and later bytes' cmp "$ref/display.txt" "$out/display.txt"
mtype -i "$out/data.img" ::PADOUT.TXT > "$out/redirected.bin" || error 'read PADOUT.TXT'
require 'redirected TYPE stops before Ctrl-Z and later bytes' cmp "$ref/redirected.bin" "$out/redirected.bin"
mtype -i "$out/data.img" ::PADDED.TXT > "$out/retained.bin" || error 'read PADDED.TXT'
require 'TYPE leaves the padded report unchanged' cmp "$ref/input.bin" "$out/retained.bin"
echo 'GREEN TYPE: captured console and redirected bytes, source preserved'

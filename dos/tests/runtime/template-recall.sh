#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'PROMPT P$G' 'ECHO RECALL-READY' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
keys() {
    printf '%s\n' "$1" | build/factory/keys | sed 's/^/k/; s/,/,k/g'
}
first=$(keys 'echo first')
cancel=$(keys 'echo other' | sed 's/,kret$//')
spec="$first,Wfirst,$cancel,kctrl-c,w250,kf3,kret,w250,$(keys done)"
timeout 40 build/factory/qemu_harness --disk "$boot" --disk2 "$out/data.img" \
    --keys-after RECALL-READY --mouse "$spec" --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE \
    --timeout-ms 25000 --out "$out" --name recall > "$out/driver.log" 2> "$out/report" ||
    error 'template recall transport failed'
transport_pass "$out/report"
grep -x FIRST dos/command/tests/dos33/reference/template-recall/recall.txt > "$out/expected"
tr -d '\r' < "$out/recall.serial" | tr '[:lower:]' '[:upper:]' > "$out/console"
require 'the unfinished command was cancelled' grep -Fx 'P>ECHO OTHER^C' "$out/console" >/dev/null
awk '/^(FIRST|OTHER)$/ { print }' "$out/console" > "$out/actual"
require 'F3 runs the previously completed line after Ctrl-C' cmp "$out/expected" "$out/actual"
echo 'GREEN Ctrl-C preserves the completed COMMAND template'

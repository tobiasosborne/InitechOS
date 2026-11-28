#!/bin/sh
set -eu
root=${1:?root}
out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
rm -f "$out/assertion" "$out/result" "$out/transport.ok"
. "$root/dos/tests/runtime/verdict.sh"
trap 'rc=$?; finish_scenario "$rc"' 0
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
ref=$root/dos/command/tests/compat33/expected/BATCH
gzip -dc "$ref/data.img.gz" > "$out/data.img"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -o -i "$out/data.img" "$out/DONE.COM" ::DONE.COM
# The captured outer COMMAND /C invocation must return to its AUTOEXEC parent.
sed 's/^B:/A:/' "$ref/AUTOEXEC.BAT" > "$out/AUTOEXEC.BAT"
mcopy -o -i "$out/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
DOS_SESSION_RTC=1987-07-06T13:30:58 sh dos/tests/runtime/execute.sh "$root" \
    "${DOS_SESSION_BOOT:-build/image/boot.img}" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'nest transport did not complete'
require 'outer batch resumes after secondary COMMAND' grep -Fx CAPTURE-DONE "$out/text"
mtype -i "$out/data.img" ::TRACE.TXT > "$out/trace"
grep -F AFTER-COMMAND "$ref/files/TRACE.TXT" >/dev/null
require 'inner batch resumes after secondary COMMAND' grep -F AFTER-COMMAND "$out/trace"
echo 'GREEN secondary COMMAND: inner and outer batch resume'

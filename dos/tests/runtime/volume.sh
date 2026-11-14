#!/bin/sh
set -eu
root=$1 boot=$2 programs=$3 out=$4
. "$root/dos/tests/runtime/verdict.sh"
mkdir -p "$out";truncate -s 1474560 "$out/data.img";mformat -i "$out/data.img" -f 1440 ::
printf '@ECHO OFF\r\nC:\r\nPROMPT $P$G\r\nECHO VOLUME-READY\r\n' > "$out/autoexec"
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
printf 'echo volume-done\n' | "$root/build/factory/keys" > "$out/keys"
sh "$root/dos/tests/runtime/execute.sh" "$root" "$boot" "$out" "$(cat "$out/keys")" volume-done
require 'default C prompt' grep -F 'C:\>echo volume-done' "$out/text" >/dev/null
echo 'GREEN prompt: mounted C is the default drive displayed by the REPL'

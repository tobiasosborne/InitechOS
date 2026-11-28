#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/secondary-prompt
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
printf '%s\n' 'echo secondary-prompt' 'exit' 'exit' 'echo secondary-permanent' 'done' |
    build/factory/keys > "$out/keys"
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'secondary COMMAND prompt transport failed'
tr '[:lower:]' '[:upper:]' < "$out/text" |
    awk '/^(MAIN-START|SECONDARY-PROMPT|BACK-FROM-CHILD|STARTUP-AGAIN|AFTER-EXIT|STARTUP-END|SECONDARY-PERMANENT|BACK-FROM-P|INVALID NUMBER OF PARAMETERS)$/ { print }' > "$out/actual"
require 'bare COMMAND returns on EXIT and COMMAND /P runs startup and keeps its prompt' cmp "$ref/result.txt" "$out/actual"
echo 'GREEN interactive and permanent secondary COMMAND prompts'

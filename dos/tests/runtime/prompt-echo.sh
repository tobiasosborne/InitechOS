#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/prompt-echo
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/query.txt" > "$out/QUERY.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'PROMPT P$G' 'ECHO ECHO-READY' > "$out/AUTOEXEC.BAT"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/QUERY.BAT" "$out/DONE.COM" ::
printf '%s\n' 'echo off' 'echo' 'echo off extra' 'command /c echo' 'query' \
    'echo on' 'echo' 'done' | build/factory/keys > "$out/keys"
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'prompt ECHO transport failed'
pattern='^(P>)?ECHO( |$)|^(OFF EXTRA|COMMAND /C ECHO|QUERY)$'
tr '[:lower:]' '[:upper:]' < "$out/text" | sed -n '/^ECHO-READY$/,$p' |
    awk -v pattern="$pattern" '$0 ~ pattern { print }' > "$out/actual"
tr '[:lower:]' '[:upper:]' < "$ref/echo.txt" > "$out/expected"
require 'ECHO controls the prompt and reports the DOS 3.30 state' cmp "$out/expected" "$out/actual"
echo 'GREEN prompt ECHO state, message text and secondary isolation'

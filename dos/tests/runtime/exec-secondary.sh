#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/exec-secondary
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
printf '@echo off\r\nPATH A:\\\r\nPROMPT P$G\r\n' > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
nasm -f bin dos/tests/runtime/execshell.asm -o "$out/EXECSHEL.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" "$out/EXECSHEL.COM" ::
{
    printf '%s\n' execshel 'set zz=child' | build/factory/keys
    echo ctrl-c
    printf '%s\n' 'echo inner' exit set 'echo parent-done' done | build/factory/keys
} | paste -sd , > "$out/keys"
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" "$(cat "$out/keys")" TRANSPORT-DONE ||
    error 'EXEC secondary COMMAND transport failed'
tr '[:lower:]' '[:upper:]' < "$out/text" > "$out/upper"
awk '/^(INNER|PARENT-DONE|COMSPEC=.*|PATH=.*|PROMPT=.*|ZZ=.*)$/ { print }' "$out/upper" > "$out/actual"
require 'Ctrl-C reprompts the EXEC secondary and EXIT leaves the parent environment' cmp "$ref/result.txt" "$out/actual"
require 'EXIT returns to the program that started the secondary' grep -qx PARENT-BACK "$out/upper"
echo 'GREEN EXEC secondary COMMAND: Ctrl-C reprompts, EXIT returns, SET stays private'

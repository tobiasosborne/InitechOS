#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/quoted-redirection
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/keep.txt" > "$out/KEEP.TXT"
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
printf 'plain\r\nflag>KEEP.TXT \r\n' > "$out/TEST.BIN"
printf 'before\r\nmiddle>arrow\r\nafter\r\n' > "$out/SRCH.TXT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/KEEP.TXT" "$out/AUTOEXEC.BAT" "$out/TEST.BIN" \
    "$out/SRCH.TXT" "$out/DONE.COM" build/dos/FIND.COM ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'quoted FIND transport failed'
require 'quoted FIND batch finishes' grep -Fxq QUOTES-DONE "$out/text"
mtype -i "$out/data.img" ::KEEP.TXT > "$out/retained" || error 'cannot read KEEP.TXT'
require 'the quoted greater-than sign leaves KEEP.TXT intact' cmp "$out/KEEP.TXT" "$out/retained"
require 'FIND receives and matches the quoted search text' grep -Fq 'flag>KEEP.TXT' "$out/text"
mdir -a -b -i "$out/data.img" :: > "$out/inventory"
require 'the unquoted redirect creates MATCHES' grep -Fxq '::/MATCHES' "$out/inventory"
mtype -i "$out/data.img" ::MATCHES > "$out/matches" || error 'cannot read MATCHES'
sed 's/$/\r/' "$ref/matches.txt" > "$out/expected"
require 'redirected FIND output matches the DOS 3.30 readback' cmp "$out/expected" "$out/matches"
echo 'GREEN quoted FIND preserves KEEP.TXT and writes captured MATCHES bytes'

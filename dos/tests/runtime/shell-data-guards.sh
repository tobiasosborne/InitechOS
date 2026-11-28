#!/bin/sh
set -eu
root=${1:?root} boot=${2:?boot} out=${3:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. dos/tests/runtime/verdict.sh
ref=dos/command/tests/dos33/reference/data-guards
truncate -s 1474560 "$out/data.img"
mformat -i "$out/data.img" -f 1440 ::
sed 's/$/\r/' "$ref/batch.txt" > "$out/AUTOEXEC.BAT"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
mcopy -i "$out/data.img" "$out/AUTOEXEC.BAT" "$out/DONE.COM" ::
sh dos/tests/runtime/execute.sh "$root" "$boot" "$out" d,o,n,e,ret TRANSPORT-DONE ||
    error 'shell data guard transport failed'
mtype -i "$out/data.img" ::TOUT > "$out/tout" || error 'cannot read TOUT'
printf 'OLD-BYTES\r\n' > "$out/expected-tout"
require 'TOUT retained the original bytes' cmp "$out/expected-tout" "$out/tout"
mdir -a -b -i "$out/data.img" :: > "$out/names" || error 'cannot read disk inventory'
forbid 'repeated output redirection created MISS' grep -Fx '::/MISS' "$out/names"
for name in KILL.ONE KILL.TWO; do
    require "DEL with two operands preserved $name" grep -Fx "::/$name" "$out/names"
done
forbid 'redirected producer created SAFE.OUT' grep -Fx '::/SAFE.OUT' "$out/names"
forbid 'redirected producer allowed the next stage to run' grep -Fx END "$out/text"
for name in OLD NEW ONLY; do
    require "COPY created or replaced $name.TXT" grep -Fx "::/$name.TXT" "$out/names"
    mtype -i "$out/data.img" "::$name.TXT" > "$out/copied" || error "cannot read $name.TXT"
    {
        printf 'ONE\r\n'
        if [ "$name" = OLD ]; then printf 'TWO\r\n'; fi
        printf '\032'
    } > "$out/expected-copy"
    require "COPY retained the available source bytes in $name.TXT" cmp "$out/expected-copy" "$out/copied"
done
sed 's/^ *//' "$out/text" |
    awk '/^(File creation error|OLD-BYTES|REPLACED .*|Bad command or file name|Invalid number of parameters|File not found|ONE\.TXT|TWO\.TXT|1 File\(s\) copied)$/ { print }' > "$out/actual"
require 'shell refusals match DOS 3.30' cmp "$ref/console.txt" "$out/actual"
echo 'GREEN shell data guards preserve DOS files'

#!/bin/sh
set -eu
root=$1 boot=$2 programs=$3 out=$4
mkdir -p "$out";truncate -s 1474560 "$out/data.img";mformat -i "$out/data.img" -f 1440 ::
printf 'This byte must never be emitted after a failed read\r\n' > "$out/file"
mcopy -i "$out/data.img" "$out/file" ::BROKEN.TXT
cc -std=c11 -O2 -Wall -Wextra "$root/dos/tests/runtime/breakfile.c" -o "$out/breakfile"
"$out/breakfile" "$out/data.img"
printf '@ECHO OFF\r\nCOPY BROKEN.TXT OUT.TXT\r\nECHO read-done\r\n' > "$out/autoexec"
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
free=$(mdir -i "$out/data.img" :: | awk '/bytes free/{gsub(/ /, "");sub(/bytesfree.*/, "");print}')
truncate -s "$((free-512))" "$out/filler"
mcopy -i "$out/data.img" "$out/filler" ::FILL.BIN
sh "$root/dos/tests/runtime/execute.sh" "$root" "$boot" "$out" '' read-done
grep -Fx 'Bad command or file name' "$out/text" >/dev/null
if grep -F 'This byte' "$out/text" >/dev/null; then echo 'RED read: corrupt-file data emitted' >&2;exit 1;fi
echo 'GREEN read error: CF failure is not a byte count and COPY returns to its batch'

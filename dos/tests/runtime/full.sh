#!/bin/sh
# AH40 short writes and refused redirection must produce visible errors.
set -eu
root=$1 boot=$2 programs=$3 out=$4
. "$root/dos/tests/runtime/verdict.sh"
mkdir -p "$out"
img=$out/data.img
truncate -s 1474560 "$img";mformat -i "$img" -f 1440 ::
mcopy -i "$img" "$programs/MORE.COM" ::MORE.COM
printf 'locked\r\n' > "$out/locked";mcopy -i "$img" "$out/locked" ::LOCK.TXT
mattrib -i "$img" +r ::LOCK.TXT
printf '@ECHO OFF\r\nECHO refused >LOCK.TXT\r\nECHO short >NEW.TXT\r\nECHO short | MORE\r\nECHO full-done\r\n' > "$out/autoexec"
mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
free=$(mdir -i "$img" :: | awk '/bytes free/{gsub(/ /,"");sub(/bytesfree.*/,"");print}')
truncate -s "$free" "$out/filler";mcopy -i "$img" "$out/filler" ::FILL.BIN
sh "$root/dos/tests/runtime/execute.sh" "$root" "$boot" "$out" '' full-done
tr -d '\r' < "$out/run.serial" > "$out/text"
require 'file creation diagnostic' grep -Fx 'File creation error' "$out/text" >/dev/null
require 'two full-disk diagnostics' test "$(grep -Ec '^(Insufficient disk space|Intermediate file error during pipe)$' "$out/text")" -eq 2
require 'redirected-write full diagnostic' grep -Fx 'Insufficient disk space' "$out/text" >/dev/null
require 'captured pipe diagnostic' grep -Fx 'Intermediate file error during pipe' "$out/text" >/dev/null
mtype -i "$img" ::LOCK.TXT > "$out/got";require 'locked bytes' cmp "$out/locked" "$out/got"
echo 'GREEN full disk: refused create, redirected short write, failed pipe; locked bytes intact'

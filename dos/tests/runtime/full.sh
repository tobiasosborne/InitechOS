#!/bin/sh
# AH40 short writes and refused redirection must produce visible errors.
set -eu
root=$1 boot=$2 programs=$3 out=$4
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
grep -Fx 'File creation error' "$out/text" >/dev/null
[ "$(grep -c '^Insufficient disk space$' "$out/text")" -eq 2 ]
mtype -i "$img" ::LOCK.TXT > "$out/got";cmp "$out/locked" "$out/got"
echo 'GREEN full disk: refused create, redirected short write, failed pipe; locked bytes intact'

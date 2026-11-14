#!/bin/sh
set -eu
root=$1 boot=$2 programs=$3 out=$4
mkdir -p "$out";truncate -s 1474560 "$out/data.img";mformat -i "$out/data.img" -f 1440 ::
printf '\n\necho startup-done\n' | "$root/build/factory/keys" > "$out/keys"
sh "$root/dos/tests/runtime/execute.sh" "$root" "$boot" "$out" "$(cat "$out/keys")" startup-done
grep -F 'Enter new date (mm-dd-yy): ' "$out/text" >/dev/null
grep -F 'Enter new time: ' "$out/text" >/dev/null
echo 'GREEN startup: no AUTOEXEC requests date and time before the prompt'

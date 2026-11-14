#!/bin/sh
# Bochs-only complete journey/readback proof; the full smoke also runs QEMU.
set -eu
cd "$(dirname "$0")/../.."
out=$1
mkdir -p "$out"
out=$(cd "$out" && pwd)
make -s -j2 -f factory/desktop-tools.mk test
journeys=$(sed '/^[[:space:]]*#/d; /^[[:space:]]*$/d' factory/desktop-journeys.list)
uptime > "$out/load-before.txt"
ps -eo pid,ni,pcpu,comm,args --sort=-pcpu > "$out/processes-before.txt"
sha256sum build/image/boot.img build/image/data.img build/factory/desktop-bochs > "$out/inputs.sha256"
sha256sum factory/desktop-bochs-sessions.sh factory/desktop-sessions.sh > "$out/scripts.sha256"
INITECH_FULL_SMOKE=1 sh factory/desktop-bochs-sessions.sh build/image/boot.img build/image/data.img "$out" $journeys
for dir in $journeys; do
 app=$(basename "$(dirname "$(dirname "$dir")")")
 sh "$dir/verify.sh" "$out/bochs-data.img" "$out/bochs-$app"
 echo "ok Bochs independent verifier $app"
done
uptime > "$out/load-after.txt"
echo 'Bochs desktop grouped sessions and all independent readbacks GREEN'

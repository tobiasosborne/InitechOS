#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
for pass in 1 2; do
 make clean > "$work/clean-$pass.log"
 make image > "$work/build-$pass.log" 2>&1
 sha256sum build/image/boot.img build/image/data.img build/kernel/INITDOS.SYS > "$work/hash-$pass"
done
cmp "$work/hash-1" "$work/hash-2"
mkdir -p build/repro
cp "$work"/* build/repro/
cat build/repro/hash-2
echo 'REPRO: identical across two clean builds'

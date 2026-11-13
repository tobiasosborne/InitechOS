#!/bin/sh
# No host compiler/assembler/linker participates after K1 is on the TEST disk.
set -eu
cd "$(dirname "$0")/../../.."
out=$PWD/build/turbo/selfhost
mkdir -p "$out"
sha256sum turbo/compiler/turbo.pas > "$out/source.sha256"
sh turbo/tests/smoke/compile.sh build/turbo/compiler/TURBO.COM turbo/compiler/turbo.pas "$out/K2"
cp "$out/K2/program.COM" "$out/K2.COM"
sh turbo/tests/smoke/compile.sh "$out/K2.COM" turbo/compiler/turbo.pas "$out/K3"
cp "$out/K3/program.COM" "$out/K3.COM"
build/turbo/smoke/compare "$out/K2.COM" "$out/K3.COM"
sha256sum "$out/K2.COM" "$out/K3.COM" | tee "$out/fixedpoint.sha256"
# Demonstrate that the exact comparator rejects contamination at a real byte.
cp "$out/K3.COM" "$out/corrupt.COM"
printf '\000' | dd of="$out/corrupt.COM" bs=1 seek=0 conv=notrunc 2>/dev/null
if build/turbo/smoke/compare "$out/K2.COM" "$out/corrupt.COM" > "$out/red-control.log" 2>&1; then exit 1; fi
cat "$out/red-control.log"
echo 'SELFHOST: in-OS K2 == K3, whole containers; corruption control red'

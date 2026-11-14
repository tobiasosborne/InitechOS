#!/bin/sh
# Diverse compiler D1 is made by FPC; Bochs performs the complete D1(S)->D2.
set -eu
cd "$(dirname "$0")/../../.."
out=$PWD/build/turbo/ddc/bochs
mkdir -p "$out"
sha256sum turbo/compiler/turbo.pas > "$out/source.sha256"
cmp build/turbo/ddc/source.sha256 "$out/source.sha256"
cmp build/turbo/selfhost/bochs/source.sha256 "$out/source.sha256"
sh turbo/tests/smoke/compile.sh build/turbo/ddc/D1.COM turbo/compiler/turbo.pas "$out/D2" bochs
cp "$out/D2/program.COM" "$out/D2.COM"
build/turbo/smoke/compare build/turbo/selfhost/bochs/K2.COM "$out/D2.COM"
build/turbo/smoke/compare build/turbo/selfhost/bochs/K3.COM "$out/D2.COM"
build/turbo/smoke/compare build/turbo/ddc/D2.COM "$out/D2.COM"
sha256sum "$out/D2.COM" | tee "$out/diverse.sha256"
cp build/turbo/selfhost/bochs/K3.COM "$out/contaminated.COM"
printf '\000' | dd of="$out/contaminated.COM" bs=1 seek=0 conv=notrunc 2>/dev/null
if build/turbo/smoke/compare "$out/contaminated.COM" "$out/D2.COM" > "$out/contaminated.log" 2>&1; then
 exit 1
else
 test "$?" -eq 1
fi
cat "$out/contaminated.log"
echo 'DDC Bochs: FPC bootstrap D2 == Bochs K2 == K3 == QEMU D2; contamination control red'

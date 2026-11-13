#!/bin/sh
# Diverse bootstrap: the independent FPC-built host Pascal compiler emits D1
# directly; D1 compiles the same source in the OS to D2. Compare to seed path.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
out=$root/build/turbo/ddc
mkdir -p "$out/host" "$out/changed"
sha256sum turbo/compiler/turbo.pas > "$out/source.sha256"
cmp "$out/source.sha256" build/turbo/selfhost/source.sha256
fpc -iV > "$out/fpc-version.txt"
sha256sum build/turbo/compiler/turbo_fpc build/turbo/compiler/TURBO.COM > "$out/bootstraps.sha256"
cp turbo/compiler/turbo.pas "$out/host/TURBOIN.PAS"
(cd "$out/host" && "$root/build/turbo/compiler/turbo_fpc" > transcript)
tr -d '\r' < "$out/host/transcript" > "$out/host/normalized"
grep -E '^TURBO-NATIVE-OK form=1 ' "$out/host/normalized"
if grep -E '^TURBO-.*(ERROR|SKIP)' "$out/host/normalized"; then exit 1; fi
cp "$out/host/TURBOOUT.EXE" "$out/D1.COM"
build/factory/mz/mzcheck "$out/D1.COM" > "$out/D1.mzcheck"
sh turbo/tests/smoke/compile.sh "$out/D1.COM" turbo/compiler/turbo.pas "$out/D2"
cp "$out/D2/program.COM" "$out/D2.COM"
build/turbo/smoke/compare build/turbo/selfhost/K2.COM "$out/D2.COM"
build/turbo/smoke/compare build/turbo/selfhost/K3.COM "$out/D2.COM"
sha256sum "$out/D1.COM" "$out/D2.COM" | tee "$out/diverse.sha256"
# Comparator sensitivity to contamination in the seed path's candidate.
cp build/turbo/selfhost/K3.COM "$out/contaminated.COM"
printf '\000' | dd of="$out/contaminated.COM" bs=1 seek=0 conv=notrunc 2>/dev/null
if build/turbo/smoke/compare "$out/contaminated.COM" "$out/D2.COM" > "$out/contaminated.log" 2>&1; then exit 1; fi
# A real source change must alter the translated compiler: change a used bound,
# keeping this program and compiler source within both capacities.
sed 's/SourceMax = 262144;/SourceMax = 262143;/' turbo/compiler/turbo.pas > "$out/changed/TURBOIN.PAS"
if cmp -s turbo/compiler/turbo.pas "$out/changed/TURBOIN.PAS"; then exit 1; fi
(cd "$out/changed" && "$root/build/turbo/compiler/turbo_fpc" > transcript)
grep -E '^TURBO-NATIVE-OK form=1 ' "$out/changed/transcript"
build/factory/mz/mzcheck "$out/changed/TURBOOUT.EXE" > "$out/changed/mzcheck.log"
if build/turbo/smoke/compare "$out/D1.COM" "$out/changed/TURBOOUT.EXE" > "$out/changed-source.log" 2>&1; then exit 1; fi
cat "$out/contaminated.log" "$out/changed-source.log"
echo 'DDC: FPC bootstrap D2 == seed bootstrap K2 == K3; two negative controls red'

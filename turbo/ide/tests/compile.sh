#!/bin/sh
set -eu
out=$1 root=${TOP:?} corpus=${TURBOPASCAL_DECOMP:?}
main=$(awk '/^begin$/ {n=NR} END {print n}' "$root/turbo/compiler/turbo.pas")
head -n $((main-1)) "$root/turbo/compiler/turbo.pas" > "$out/compile-host.pas"
main=$(awk '/^begin$/ {n=NR} END {print n}' tests/host.pas)
head -n $((main-1)) tests/host.pas >> "$out/compile-host.pas"
cat tests/compile-main.pas >> "$out/compile-host.pas"
fpc -B -O- -v0 -FaHostRTL -Fu"$root/turbo/compiler" -FU"$out" -FE"$out" -o"$out/compile-host" "$out/compile-host.pas" > "$out/compile-host-build.log" 2>&1 || { cat "$out/compile-host-build.log"; exit 2; }
scratch=$out/runs/compile
rm -rf "$scratch"; mkdir -p "$scratch"
cp "$corpus/goldens/minted/MINT06/disk/DEMO.BAK" "$scratch/DEMO.PAS"
(cd "$scratch" && "$out/compile-host") || exit 1
printf 'error85\ncompile\nafter_compile\n' > "$scratch/shots.list"
"$out/screens" "$corpus/goldens/minted/MINT06" "$scratch" tests/compile.expect
cmp "$scratch/BEFORE.COM" "$scratch/DEMO.COM"
echo '1 checks, 0 failures'

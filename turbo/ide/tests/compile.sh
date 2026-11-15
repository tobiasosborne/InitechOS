#!/bin/sh
set -eu
out=$1 root=${TOP:?} corpus=${TURBOPASCAL_DECOMP:?}
. "$root/turbo/tests/checks.sh"
setup cp "$root/turbo/compiler/turbo.pas" "$out/compile-model.pas"
# Mutants live in isolated component copies; execute those exact fragments.
# The independent expectations remain the real TP captures and file bytes.
setup sh "$root/turbo/compiler/tests/ide-source.sh" update "$out/compile-model.pas" "$PWD"
main=$(awk '/^begin$/ {n=NR} END {print n}' "$out/compile-model.pas")
setup head -n $((main-1)) "$out/compile-model.pas" > "$out/compile-host.pas"
main=$(awk '/^begin$/ {n=NR} END {print n}' tests/host.pas)
setup head -n $((main-1)) tests/host.pas >> "$out/compile-host.pas"
setup cat tests/compile-main.pas >> "$out/compile-host.pas"
fpc -B -O- -v0 -FaHostRTL -Fu"$root/turbo/compiler" -FU"$out" -FE"$out" -o"$out/compile-host" "$out/compile-host.pas" > "$out/compile-host-build.log" 2>&1 || { cat "$out/compile-host-build.log"; exit 2; }
scratch=$out/runs/compile
setup rm -rf "$scratch"
setup mkdir -p "$scratch"
setup cp "$corpus/goldens/minted/MINT06/disk/DEMO.BAK" "$scratch/DEMO.PAS"
compile_model() {
  model_rc=0
  (cd "$scratch" && "$out/compile-host") > "$scratch/stdout" 2>&1 || model_rc=$?
  cat "$scratch/stdout" || return 2
  return "$model_rc"
}
assert 'compiler record, Compile/Make and stale-output refusal' compile_model
printf 'error85\ncompile\nafter_compile\n' > "$scratch/shots.list"
assert 'real TP Error85 and compile-box screens' "$out/screens" "$corpus/goldens/minted/MINT06" "$scratch" tests/compile.expect
assert 'saved old executable exists' test -s "$scratch/BEFORE.COM"
assert 'old executable retained after error' test -s "$scratch/DEMO.COM"
assert 'compiler error leaves previous executable bytes' cmp "$scratch/BEFORE.COM" "$scratch/DEMO.COM"
alias=$(sed -n 's/^OWNERSHIP //p' "$scratch/stdout")
assert 'saved document survives private-file cleanup' test -f "$scratch/$alias"
assert 'saved document keeps TP text bytes' cmp "$corpus/goldens/minted/MINT06/disk/DEMO.PAS" "$scratch/$alias"
finish

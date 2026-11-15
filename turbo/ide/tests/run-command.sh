#!/bin/sh
set -eu
out=$1 root=${TOP:?} corpus=${TURBOPASCAL_DECOMP:?}
. "$root/turbo/tests/checks.sh"
setup cp "$root/turbo/compiler/turbo.pas" "$out/run-model.pas"
setup sh "$root/turbo/compiler/tests/ide-source.sh" update "$out/run-model.pas" "$PWD"
main=$(awk '/^begin$/ {n=NR} END {print n}' "$out/run-model.pas")
setup head -n $((main-1)) "$out/run-model.pas" > "$out/run-host.pas"
main=$(awk '/^begin$/ {n=NR} END {print n}' tests/host.pas)
setup head -n $((main-1)) tests/host.pas >> "$out/run-host.pas"
setup cat tests/run-main.pas >> "$out/run-host.pas"
setup cp tests/runrtl.pas "$out/runrtl.pas"
setup fpc -B -O- -v0 -FaRunRTL -Fu"$root/turbo/compiler" -Fu"$out" -FU"$out" -FE"$out" -o"$out/run-host" "$out/run-host.pas" > "$out/run-build.log" 2>&1
scratch=$out/runs/run-command
setup rm -rf "$scratch"
setup mkdir -p "$scratch"
setup cp "$corpus/mint/src/MINT07/EXIT7.PAS" "$scratch/EXIT7.PAS"
run_model() { (cd "$scratch" && "$out/run-host"); }
assert 'MINT07: Run and User screen DOS boundary ordering' run_model
printf 'after_run\nafter_user\n' > "$scratch/shots.list"
assert 'MINT07: editor after Run and after User screen' "$out/screens" "$corpus/goldens/minted/MINT07" "$scratch" tests/run-command.expect
finish

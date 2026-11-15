#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
setup cat tests/head-fpc.pas tests/faults.pas screen.pas buffer.pas editor.pas tests/first-save.pas > "$out/first-save.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/first-save" "$out/first-save.pas" > "$out/first-save-build.log" 2>&1 || { cat "$out/first-save-build.log"; exit 2; }
scratch=$out/runs/first-save
setup rm -rf "$scratch"
setup mkdir -p "$scratch"
for answer in esc no yes open; do
  setup mkdir "$scratch/$answer"
  printf OLD > "$scratch/$answer/DOC.PAS"
  cd "$scratch/$answer"
  assert "first save:$answer conservation" "$out/first-save" "$answer"
done
finish

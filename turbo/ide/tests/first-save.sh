#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas tests/faults.pas screen.pas buffer.pas editor.pas tests/first-save.pas > "$out/first-save.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/first-save" "$out/first-save.pas" > "$out/first-save-build.log" 2>&1 || { cat "$out/first-save-build.log"; exit 2; }
scratch=$out/runs/first-save
rm -rf "$scratch"
mkdir -p "$scratch"
for answer in esc no yes open; do
  mkdir "$scratch/$answer"
  printf OLD > "$scratch/$answer/DOC.PAS"
  (cd "$scratch/$answer" && "$out/first-save" "$answer") || exit 1
done
echo '1 checks, 0 failures'

#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/save-path.pas > "$out/save-path.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-path" "$out/save-path.pas" > "$out/save-path-build.log" 2>&1 || { cat "$out/save-path-build.log"; exit 2; }
scratch=$out/runs/save-path
rm -rf "$scratch"; mkdir -p "$scratch"
for prefix in 'SUB\' 'B:\'; do
  printf OLD > "$scratch/${prefix}DOC.PAS"
  printf ROOT-SENTINEL > "$scratch/DOC.PAS"
  (cd "$scratch" && "$out/save-path" "${prefix}DOC.PAS" "${prefix}NEW.PAS") || exit 1
done
echo '1 checks, 0 failures'

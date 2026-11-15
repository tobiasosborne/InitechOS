#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
setup cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/save-path.pas > "$out/save-path.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-path" "$out/save-path.pas" > "$out/save-path-build.log" 2>&1 || { cat "$out/save-path-build.log"; exit 2; }
scratch=$out/runs/save-path
setup rm -rf "$scratch"; setup mkdir -p "$scratch"
for prefix in 'SUB\' 'B:\'; do
  printf OLD > "$scratch/${prefix}DOC.PAS"
  printf ROOT-SENTINEL > "$scratch/DOC.PAS"
  cd "$scratch"
  assert "path:$prefix identity" "$out/save-path" "${prefix}DOC.PAS" "${prefix}NEW.PAS"
done
finish

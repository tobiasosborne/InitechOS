#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
setup cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/save-identity.pas > "$out/save-identity.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-identity" "$out/save-identity.pas" > "$out/save-identity-build.log" 2>&1 || { cat "$out/save-identity-build.log"; exit 2; }
scratch=$out/runs/save-identity
setup rm -rf "$scratch"
setup mkdir -p "$scratch"
i=0
for name in DOC.BAK DOC.bak 'DOC.$$$' DOC; do
  i=$((i+1)); setup mkdir "$scratch/$i"
  printf OLD > "$scratch/$i/$name"
  cd "$scratch/$i"
  assert "source:$name identity" "$out/save-identity" "$name"
  case $name in DOC.BAK|DOC.bak) backup=DOC.BK1;; *) backup=DOC.BAK;; esac
  printf OLD > "$scratch/old"
  assert "source:$name previous bytes" cmp "$scratch/old" "$scratch/$i/$backup"
done
for mode in dirty flush; do
 for name in TURBOWK0.PAS TURBOWK1.PAS; do
  i=$((i+1)); setup mkdir "$scratch/$i"
  printf OLD > "$scratch/$i/$name"
  cd "$scratch/$i"
  assert "work alias:$name:$mode conserves bytes" "$out/save-identity" "$name" "$mode"
 done
done
finish

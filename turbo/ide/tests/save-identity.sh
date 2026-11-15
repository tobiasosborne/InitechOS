#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/save-identity.pas > "$out/save-identity.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-identity" "$out/save-identity.pas" > "$out/save-identity-build.log" 2>&1 || { cat "$out/save-identity-build.log"; exit 2; }
scratch=$out/runs/save-identity
rm -rf "$scratch"
mkdir -p "$scratch"
i=0
for name in DOC.BAK DOC.bak 'DOC.$$$' DOC; do
  i=$((i+1)); mkdir "$scratch/$i"
  printf OLD > "$scratch/$i/$name"
  (cd "$scratch/$i" && "$out/save-identity" "$name") || exit 1
  case $name in DOC.BAK|DOC.bak) backup=DOC.BK1;; *) backup=DOC.BAK;; esac
  printf OLD > "$scratch/old"
  cmp "$scratch/old" "$scratch/$i/$backup" || exit 1
done
echo '1 checks, 0 failures'

#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas tests/faults.pas screen.pas buffer.pas editor.pas tests/save-failure.pas > "$out/save-failure.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-failure" "$out/save-failure.pas" > "$out/save-failure-build.log" 2>&1 || { cat "$out/save-failure-build.log"; exit 2; }
scratch=$out/runs/save-failure
mkdir -p "$scratch"
for kind in target write close rename erase rename-double; do
  # Page flush then DOS-text staging: a fault in either must be harmless.
  case $kind in target|erase|rename-double) count=1;; rename) count=2;; write) count=2;; close) count=4;; esac
  i=1
  while [ "$i" -le "$count" ]; do
    printf OLD > "$scratch/DOC.PAS"
    rm -f "$scratch/DOC.BAK" "$scratch/DOC."*'$$$'
    (cd "$scratch" && "$out/save-failure" "$kind" "$i") || exit 1
    [ "$kind" = rename-double ] || test ! -e "$scratch/DOC.\$\$\$" || { echo 'FAIL stage remains after a recoverable failure'; exit 1; }
    i=$((i+1))
  done
done
echo '1 checks, 0 failures'

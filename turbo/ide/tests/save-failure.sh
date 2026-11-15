#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
setup cat tests/head-fpc.pas tests/faults.pas screen.pas buffer.pas editor.pas tests/save-failure.pas > "$out/save-failure.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/save-failure" "$out/save-failure.pas" > "$out/save-failure-build.log" 2>&1 || { cat "$out/save-failure-build.log"; exit 2; }
scratch=$out/runs/save-failure
setup mkdir -p "$scratch"
for kind in target write close rename erase rename-double; do
  # Page flush then DOS-text staging: a fault in either must be harmless.
  case $kind in target|erase|rename-double) count=1;; rename) count=2;; write) count=2;; close) count=4;; esac
  i=1
  while [ "$i" -le "$count" ]; do
    printf OLD > "$scratch/DOC.PAS"
    rm -f "$scratch/DOC.BAK" "$scratch/DOC."*'$$$'
    cd "$scratch"
    assert "save:$kind:$i conserves previous bytes" "$out/save-failure" "$kind" "$i"
    if [ "$kind" != rename-double ]; then
      assert 'failed save removes only its stage' test ! -e "$scratch/DOC.\$\$\$"
    fi
    i=$((i+1))
  done
done
finish

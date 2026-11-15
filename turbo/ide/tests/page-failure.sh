#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas tests/faults.pas screen.pas buffer.pas editor.pas tests/page-failure.pas > "$out/page-failure.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/page-failure" "$out/page-failure.pas" > "$out/page-failure-build.log" 2>&1 || { cat "$out/page-failure-build.log"; exit 2; }
scratch=$out/runs/page-failure
mkdir -p "$scratch"
awk 'BEGIN {for(i=1;i<=5000;i++)printf "OLD\r\n"}' > "$scratch/DOC.PAS"
awk 'BEGIN {for(i=1;i<=5000;i++)printf "%sOLD\r\n",i==2500?"X":""; printf "\032"}' > "$scratch/expected"
for mode in page save; do
  for kind in write read close; do
    case $kind in write|read) count=79;; close) count=2;; esac
    i=1
    while [ "$i" -le "$count" ]; do
      (cd "$scratch" && "$out/page-failure" "$kind" "$i" "$mode") > "$scratch/log" 2>&1 || { cat "$scratch/log"; echo "FAIL $mode $kind $i"; exit 1; }
      cmp "$scratch/expected" "$scratch/RESULT.PAS" || exit 1
      i=$((i+1))
    done
  done
done
echo 'PASS failed page transfers and closes preserve the dirty page; retry matches authored bytes'
echo '1 checks, 0 failures'

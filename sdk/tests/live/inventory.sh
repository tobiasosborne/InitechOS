#!/bin/sh
set -eu
out=$1
awk '!/^#/ && $3 >= 0 {print tolower(substr($2,3)), $3}' tests/traps.tsv | sort > "$out/declared-abi.tsv"
awk '!/^#/ && NF {print tolower($1), $2}' tests/live/abi.tsv | sort > "$out/live-abi.tsv"
cmp "$out/declared-abi.tsv" "$out/live-abi.tsv"

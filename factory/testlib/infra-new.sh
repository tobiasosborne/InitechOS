#!/bin/sh
# Kill contract item 4 (testlib/README.md): print the infrastructure lines a
# mutant's run recorded that its baseline did not. Usage: infra-new.sh BASE RUN
set -eu
base=$1 run=$2
[ -f "$base" ] || : > "$base"
[ -f "$run" ] || : > "$run"
sorted=$(mktemp)
LC_ALL=C sort "$base" > "$sorted"
LC_ALL=C sort "$run" | LC_ALL=C comm -13 "$sorted" -
rm -f "$sorted"

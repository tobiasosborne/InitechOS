#!/bin/sh
# Both a valid product and an ordinary compiler error must earn zero kills.
set -eu
top=$1 out=$2 make_cmd=$3
mkdir -p "$out"
for control in no-fault compile-error; do
 log=$out/$control.log
 if sh tools/run-target-mutants.sh "$top" "$out/$control" "$make_cmd" "mutants/controls/target-$control.patch" > "$log" 2>&1; then
  echo "FAIL target grader accepted $control control ($log)"; exit 1
 fi
 grep -Fx 'InitechBase target mutations: 1 patches, 0 killed, 1 errors' "$log" >/dev/null || { cat "$log";exit 1; }
 echo "ok target grader refuses $control control"
done

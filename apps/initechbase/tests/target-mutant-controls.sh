#!/bin/sh
# Negative controls for the target grader. They are not mutants: they live
# outside mutants/, so the sweep's inventory never counts them, and their
# receipts stay in their own logs. Each must earn zero kills, and each must
# fail in its own way, so a stale control cannot pass by failing to apply:
#  no-fault         applies, links without rejection twice: one survivor
#  native-no-fault  applies, the native oracle is green twice: one survivor
#  compile-error    applies, then fails on its own undefined call: one error
set -eu
top=$1 out=$2 make_cmd=$3
mkdir -p "$out"
for control in no-fault compile-error native-no-fault; do
 log=$out/$control.log
 if sh tools/run-target-mutants.sh "$top" "$out/$control" "$make_cmd" "tests/target-controls/target-$control.patch" > "$log" 2>&1; then
  echo "FAIL target grader accepted $control control ($log)"; exit 1
 fi
 case $control in
  compile-error) want='apps/initechbase: 1 mutants, 0 killed, 0 survived, 1 errors;'
   build=$(sed -n 's/^ERROR target-compile-error (\(.*\))$/\1/p' "$log")
   [ -n "$build" ] && grep -F undefined_mutation_control "$build" >/dev/null || {
    echo "FAIL $control control did not fail on its own compiler error ($log)"; exit 1; } ;;
  *) want='apps/initechbase: 1 mutants, 0 killed, 1 survived, 0 errors;'
   grep "^SURVIVED target-$control " "$log" >/dev/null || {
    echo "FAIL $control control was not graded a survivor ($log)"; exit 1; } ;;
 esac
 grep -F "$want" "$log" >/dev/null || { echo "FAIL $control control receipt ($log)"; exit 1; }
 echo "ok target grader refuses $control control"
done

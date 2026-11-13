#!/bin/sh
# The canonical public assertion header must retain the positive-count guard.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
runner=${ASSERTION_RUNNER:-$root/factory/testlib/run-test.sh}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
mkdir "$work/test.objs"
failed=0
for location in include testlib; do
 printf '%s/factory/%s/test_assert.h\n' "$root" "$location" > "$work/test.objs/test.d"
 for count in 0 missing 1; do
  if [ "$count" = missing ]; then line='no summary'; else line="INITECH_CHECKS $count failures=0"; fi
  rc=0
  sh "$runner" assertions 20 '' "$work/test" sh -c 'printf "%s\n" "$2" > "$1.log"' sh "$work/test" "$line" > "$work/result" 2>&1 || rc=$?
  if [ "$count" = 1 ]; then expected=0; else expected=1; fi
  if [ "$rc" -eq "$expected" ]; then echo "ok assertions $location/$count"
  else echo "FAIL assertions $location/$count: exit $rc expected $expected"; cat "$work/result"; failed=1; fi
 done
done
exit "$failed"

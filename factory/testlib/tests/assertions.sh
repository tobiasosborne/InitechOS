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
# Kill contract, tier 0: a passing test with no completed summary, or with
# one that counts failures, fails loudly; a ledgered test passes with a NOTE.
lib=$work/lib
mkdir -p "$lib"
cp "$(dirname "$runner")"/run-test.sh "$(dirname "$runner")"/host-verdict.awk "$(dirname "$runner")"/summary-contract.awk "$lib/"
printf 'fixture listed initech-fixture\n' > "$lib/summary-contract.known"
rm -rf "$work/test.objs"
contract() { # label, component, test name, log text, expected exit, expected output
 rc=0
 INITECH_COMPONENT=$2 sh "$lib/run-test.sh" "$3" 20 '' "$work/$3" sh -c 'printf "%s\n" "$2" > "$1.log"' sh "$work/$3" "$4" > "$work/result" 2>&1 || rc=$?
 if [ "$rc" -eq "$5" ] && grep -q "$6" "$work/result" "$work/$3.run.log" 2>/dev/null; then echo "ok assertions contract/$1"
 else echo "FAIL assertions contract/$1: exit $rc expected $5 with '$6'"; cat "$work/result"; failed=1; fi
}
contract no-summary fixture unlisted 'GREEN everything' 1 'FAIL contract: unlisted printed no completed check summary'
contract zero-checks fixture unlisted '0 checks, 0 failures' 1 'FAIL contract'
contract counted-failures fixture unlisted 'INITECH_CHECKS 3 failures=2' 1 'FAIL count: unlisted exited 0'
contract legacy-summary fixture unlisted 'suite: 4 checks, 0 failures' 0 'ok    unlisted'
contract ledgered fixture listed 'GREEN everything' 0 'NOTE summary contract: listed'
contract other-component other listed 'GREEN everything' 1 'FAIL contract'
contract stale-row fixture listed 'INITECH_CHECKS 2 failures=0' 0 'NOTE stale summary-contract row'
exit "$failed"

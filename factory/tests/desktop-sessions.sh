#!/bin/sh
# Literal plans/coverage, independent of the production grouping implementation.
set -eu
cd "$(dirname "$0")/../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
if sh factory/desktop-sessions.sh > "$work/empty" 2>&1; then echo 'FAIL desktop plan: empty accepted'; exit 1; fi
sh factory/desktop-sessions.sh a b c > "$work/actual"
printf 'a\nb\nc\n' > "$work/expected"
cmp "$work/expected" "$work/actual" || { echo 'FAIL desktop plan: generic split'; exit 1; }
sh factory/desktop-sessions.sh apps/calculator/tests/smoke apps/notepad/tests/smoke other > "$work/actual"
printf 'apps/calculator/tests/smoke apps/notepad/tests/smoke\nother\n' > "$work/expected"
cmp "$work/expected" "$work/actual" || { echo 'FAIL desktop plan: clipboard group split'; exit 1; }
journeys=$(sed '/^[[:space:]]*#/d; /^[[:space:]]*$/d' factory/desktop-journeys.list)
sh factory/desktop-sessions.sh $journeys > "$work/actual"
[ "$(wc -l < "$work/actual")" -eq 7 ] || { echo 'FAIL desktop plan: expected seven bounded sessions'; exit 1; }
printf '%s\n' $journeys > "$work/selected"
awk '{for(i=1;i<=NF;i++)print $i}' "$work/actual" > "$work/planned"
cmp "$work/selected" "$work/planned" || { echo 'FAIL desktop plan: missing/repeated/reordered journey'; exit 1; }
echo 'ok desktop plan: seven sessions, full ordered coverage, clipboard group retained, empty rejected'
for limit in 0 00 -1 2147483648 bogus; do
 if INITECH_FULL_SMOKE=0 DESKTOP_SMOKE_TIMEOUT_MS=$limit sh factory/desktop-bochs-sessions.sh missing missing "$work/budget" apps/initech123/tests/smoke > "$work/budget.log" 2>&1; then
  echo "FAIL desktop plan: invalid budget $limit accepted"; exit 1
 fi
 grep -Eq 'invalid session timeout|session timeout overflow' "$work/budget.log"
done
echo 'ok desktop plan: invalid/zero/overflow deadlines rejected before boot'

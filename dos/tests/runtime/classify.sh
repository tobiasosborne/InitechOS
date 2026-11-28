#!/bin/sh
# Validate evidence as well as process status. No implicit nonzero kill.
set -eu
d=$1 rc=$2
bad() { printf 'ERROR exit=%s\n' "$rc";exit 0; }
[ -r "$d/result" ] && [ -r "$d/transport.ok" ] || bad
report=$(cat "$d/transport.ok")
[ -r "$report" ] || bad
if grep -Fx '[harness] OK=1' "$report" >/dev/null; then
 grep -F 'timed_out=0' "$report" >/dev/null || bad
 grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$report" >/dev/null || bad
elif grep -Fx '[bochs] OK=1' "$report" >/dev/null; then
 grep -E '^\[bochs\] launched=1 rfb_unblocked=1 timed_out=0( |$)' "$report" >/dev/null || bad
 grep -Fx '[bochs] triple_fault=0' "$report" >/dev/null || bad
else
 bad
fi
case $rc in
 0) grep -Fx PASS "$d/result" >/dev/null || bad;echo PASS;;
 42) [ -r "$d/assertion" ] || bad
     [ "$(wc -l < "$d/assertion")" -eq 1 ] || bad
     grep '^SEMANTIC .' "$d/assertion" >/dev/null || bad
     cmp -s "$d/assertion" "$d/result" || bad
     cat "$d/assertion";;
 *) bad;;
esac

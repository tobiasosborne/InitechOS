#!/bin/sh
# DEC-03a.2-.5: same independent oracle for production and runtime mutants.
set -eu
stem=$1
tr -d '\r' < "$stem.serial" > "$stem.repl"
fail() { tail -35 "$stem.repl"; printf '!!! test-exec-high RED: %s\n' "$1"; exit 1; }
! grep -q 'triple_fault=1' "$stem.report" || fail 'triple fault'
! grep -q '^PANIC' "$stem.repl" || fail 'guest panic'
grep -qx 'SHELL-READY' "$stem.repl" || fail 'no shell'
[ "$(grep -c '^EXEC-HIGH-OK$' "$stem.repl")" -eq 2 ] || fail 'high base/allocation/refusals/reclaim marker missing'
grep -qx 'MZEXEC-OK' "$stem.repl" || fail 'legacy MZ no longer runs low'
grep -qxF 'A:\BIG.EXE: Insufficient memory' "$stem.repl" || fail 'oversize catalogue refusal missing'
grep -qx 'SHELL-DONE' "$stem.repl" || fail 'no return to shell'
printf 'exec-high-check: green\n'

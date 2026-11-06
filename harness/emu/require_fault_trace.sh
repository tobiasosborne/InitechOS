#!/bin/sh
# Explicit deliberate-fault oracle, NOT a successful guest completion.
# CLAUDE Law 2 / Rule 6: no-Escape must hang; recursion mutants must halt at
# the stack guard. Requiring SHELL-DONE would delete these regression probes.
# Keep the original 89000 ms observation, then take a live QMP snapshot and
# quit the emulator explicitly. OK=1 describes that transport, not guest
# completion; an infrastructure timeout is never evidence of the mutation.
set -eu
report=$1; serial=$2; expected=$3; mode=$4
red() { echo "RED deliberate-fault trace: $*" >&2; exit 1; }
[ -s "$serial" ] || red 'missing serial'
! grep -q 'unknown token' "$report" || red 'fault observation token was not executed'
[ "$(cat "$report.status")" = 0 ] || red 'fault observation transport failed'
grep -qx '\[harness\] launched=1 timed_out=0 exit=0 signal=0' "$report" || red 'QEMU failed'
grep -qx '\[harness\] triple_fault=0 cpu_reset=2 guest_errors=0' "$report" || red 'unexpected guest fault'
grep -q "^\[harness\] keys_sent=$expected " "$report" || red 'partial injected trace'
grep -qx '\[harness\] OK=1' "$report" || red 'fault observation capture failed'
grep -q 'screendump_taken=1' "$report" || red 'live fault snapshot missing'
! grep -qx SHELL-DONE "$serial" || red 'unexpected shell completion'
grep -qx SHELL-READY "$serial" || red 'shell never booted'
grep -Fq '. do loop' "$serial" || red 'SAMIR never began the probe'
case $mode in
 escape)
  [ "$(cat "$report.elapsed")" -ge 89 ] || red 'incomplete original 89000ms idle observation'
  ;;
 cap|stack)
  grep -Fq 'STACK OVERFLOW -- InitechBase halted' "$serial" || red 'expected stack guard halt absent'
  ;;
 *) red 'unknown negative trace' ;;
esac

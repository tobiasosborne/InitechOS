#!/bin/sh
# PRD 8 / Rule 6: absence means regression only AFTER a real completed trace.
set -eu
report=$1; serial=$2; expected=$3
red() { echo "RED emulator trace: $*" >&2; exit 1; }
[ -s "$serial" ] || red "missing/nonempty serial: $serial"
grep -qx '\[harness\] OK=1' "$report" || red "harness did not return success: $report"
grep -qx '\[harness\] launched=1 timed_out=0 exit=0 signal=0' "$report" || red 'QEMU did not exit cleanly'
grep -q "^\[harness\] keys_sent=$expected " "$report" || red "incomplete keys (expected $expected)"
[ "$expected" -gt 0 ] || red 'no injected keys'

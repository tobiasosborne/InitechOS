#!/bin/sh
# initech-tfn6r; PRD Sec 8 / Rule 6. Real QEMU fixtures, not a fake logger.
set -eu
runner=$1; good=$2; bad=$3; build=$4
d=$(mktemp -d "$build/qlog-XXXXXX")
red() { printf 'RED qemu-log: %s (%s)\n' "$1" "$d" >&2; exit 1; }
export INITECH_QEMU_LOG_CAP_BYTES=67108864
"$runner" --kernel "$good" --expect HARNESS-OK --out "$d" --name good > "$d/good.out" 2> "$d/good.report" || red GOOD-FAILED
grep -qx '\[harness\] OK=1' "$d/good.report" || red GOOD-REPORT
test -s "$d/good.serial" || red SERIAL-LOST
test ! -s "$d/good.qemu.log" || red GREEN-RETAINED
if "$runner" --kernel "$bad" --expect HARNESS-OK --out "$d" --name bad > "$d/bad.out" 2> "$d/bad.report"; then red BAD-PASSED; fi
grep -q 'triple_fault=1' "$d/bad.report" || red FAULT-REPORT
grep -qx '\[harness\] OK=0' "$d/bad.report" || red BAD-REPORT
grep -q 'Triple fault\|check_exception old: 0x8' "$d/bad.qemu.log" || red RED-LOG-LOST
# A restart loop must be capped even while the parent waits in QMP for a
# marker that will never arrive. Keep every byte written before termination.
if INITECH_QEMU_LOG_CAP_BYTES=16384 "$runner" --kernel "$bad" --allow-reboot \
    --screendump --screendump-after NEVER --timeout-ms 4000 --out "$d" --name cap \
    > "$d/cap.out" 2> "$d/cap.report"; then red CAP-PASSED; fi
grep -q 'RED QEMU-LOG-CAP:' "$d/cap.report" || red CAP-MISSING
test "$(wc -c < "$d/cap.qemu.log")" -gt 16384 || red CAP-LOG-LOST
grep -qx '\[harness\] OK=0' "$d/cap.report" || red CAP-REPORT
grep -q 'timed_out=0' "$d/cap.report" || red CAP-WAITED-FOR-TIMEOUT
# Catch short-lived writers as well as the live monitor, and validate the
# override (zero is an error, never an opt-out).
if INITECH_QEMU_LOG_CAP_BYTES=1 "$runner" --kernel "$good" --out "$d" --name short \
    > "$d/short.out" 2> "$d/short.report"; then red SHORT-CAP-PASSED; fi
grep -q 'RED QEMU-LOG-CAP:' "$d/short.report" || red SHORT-CAP-MISSING
test -s "$d/short.qemu.log" || red SHORT-LOG-LOST
if INITECH_QEMU_LOG_CAP_BYTES=0 "$runner" --kernel "$good" --out "$d" --name zero \
    > "$d/zero.out" 2> "$d/zero.report"; then red ZERO-CAP-PASSED; fi
grep -q 'QEMU-LOG-CAP.*positive decimal' "$d/zero.report" || red ZERO-CAP-DIAGNOSTIC
printf '>>> test-harness-qemu-log: green (GREEN log removed; RED log intact; live QMP and short-run caps; positive override; %s)\n' "$d"

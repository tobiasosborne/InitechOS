#!/bin/sh
# initech-e9sx negative oracle: initial interpreter load (manual printed p310),
# not the post-EXEC overwrite/retry prompt documented on printed p338.
set -eu
build=$1; image=$2; data=$3; trace=$4; tag=$5; reason=$6
export INITECH_QMP_STDIO=1 LC_ALL=C
work="$build/command-file"
mkdir -p "$work"
cp -f "$data" "$work/$tag-data.img"
started=$(date +%s)
"$build/qemu_harness" --disk "$image" --disk2 "$work/$tag-data.img" \
 --name "$tag" --out "$work" --mouse "$trace" --keys-after FLAIR-LIVE-READY \
 --screendump --screendump-after HALTED \
 --quit-after COMMAND-FILE-MUST-NOT-RETURN --timeout-ms 12000 \
 > "$work/$tag.stdout" 2> "$work/$tag.report" || true
elapsed=$(( $(date +%s) - started ))
red() { echo "RED command-file-emu $tag $*"; exit 1; }
grep -q 'launched=1' "$work/$tag.report" || red 'not launched'
grep -q 'triple_fault=0' "$work/$tag.report" || red 'triple fault/report'
grep -q 'guest_errors=0' "$work/$tag.report" || red 'guest errors'
tr -d '\r' < "$work/$tag.serial" | sed -n '/^FLAIR-EXIT-TO-DOS$/,$p' > "$work/$tag.txt"
s="$work/$tag.txt"
grep -qx 'FLAIR-EXIT-TO-DOS' "$s" || red 'no injected exit'
[ "$(grep -xc 'Bad or missing Command Interpreter' "$s")" = 1 ] || red 'missing exact MSG-DOS-0014'
grep -q "^PANIC exit-to-dos: $reason" "$s" || red 'wrong panic reason'
grep -qx HALTED "$s" || red 'no HALTED'
! grep -Eq '^(SHELL-READY|COMMAND-TRANSIENT-OK|BANNER-BEGIN)' "$s" || red 'entered interpreter'
# The QMP input driver waits for the impossible marker for the timeout budget,
# then sends QMP quit; this is NOT the harness's no-input timed_out=1 path.
# Measure the observation and require the missing marker, rather than accepting
# an early QEMU exit as a halt. These host times never enter a shipped image.
[ "$elapsed" -ge 12 ] || red 'did not stay halted'
grep -q 'quit_after_found=0' "$work/$tag.report" || red 'returned from halt'
echo "GREEN test-command-file-negative $tag: MSG-DOS-0014 once + $reason + HALTED; no interpreter, no triple fault"

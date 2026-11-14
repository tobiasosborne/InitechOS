#!/bin/sh
# A real halted guest must fail the progress-aware wait after a healthy baseline.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/factory/desktop-wait-mutants
work=$(mktemp -d "$root/build/factory/desktop-wait-mutants/run.XXXXXX")
error() { echo "FAIL desktop wait setup: $* ($work)"; echo 'factory: 1 mutants, 0 killed, 0 survived, 1 errors'; exit 1; }
. factory/desktop-trace.sh
nice make -j2 -f factory/desktop-tools.mk all > "$work/tools.log" 2>&1 || error "frontend build"
probe() {
 boot=$1 data=$2 dir=$3
 INITECH_FULL_SMOKE=0 sh factory/desktop-bochs-sessions.sh "$boot" "$data" "$dir" apps/initech123/tests/smoke > "$dir.stdout" 2> "$dir.stderr"
}
unset DESKTOP_WAIT_MS
scratch=$work/source
mkdir -p "$scratch"
# Check acquisition before extraction; do not inherit POSIX pipeline status loss.
tar -cf "$work/source.tar" --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . || error snapshot
tar -xf "$work/source.tar" -C "$scratch" || error extraction
rm "$work/source.tar"
(cd "$scratch" && nice make -j2 image) > "$work/baseline-build.log" 2>&1 || error "baseline build"
probe "$scratch/build/image/boot.img" "$scratch/build/image/data.img" "$work/baseline" || error baseline
sh apps/initech123/tests/smoke/verify.sh "$work/baseline/bochs-data.img" "$work/baseline/worksheet" > "$work/baseline-verifier.log" 2>&1 || error "baseline readback"
patch --batch --fuzz=0 -p1 -d "$scratch" < factory/mutants/desktop-wait/stall.patch > "$work/patch.log" 2>&1 || error patch
(cd "$scratch" && nice make clean && nice make -j2 image) > "$work/build.log" 2>&1 || error build
for pass in 1 2; do
 rc=0
 probe "$scratch/build/image/boot.img" "$scratch/build/image/data.img" "$work/stall-$pass" || rc=$?
 [ "$rc" -eq 1 ] || error "stall exit $rc"
 grep -q 'no meaningful serial/input progress for 10000ms' "$work/stall-$pass/bochs-sessions/1/bochs.report" || error 'wrong failure reason'
 grep -Fx FLAIR-LIVE-READY "$work/stall-$pass/bochs-sessions/1/bochs.serial" >/dev/null || error 'stalled product never booted'
 grep -F 'rfb_unblocked=1 timed_out=0' "$work/stall-$pass/bochs-sessions/1/bochs.report" >/dev/null || error transport
 grep -Fx '[bochs] triple_fault=0' "$work/stall-$pass/bochs-sessions/1/bochs.report" >/dev/null || error fault
 grep -F "exit=-1 signal=15" "$work/stall-$pass/bochs-sessions/1/bochs.report" >/dev/null || error "unexpected emulator exit"
 grep -F "rfb_error=(none)" "$work/stall-$pass/bochs-sessions/1/bochs.report" >/dev/null || error "display transport"
 echo "killed desktop-stall (semantic missing Finder marker, pass $pass)"
done
echo "factory: 1 mutants, 1 killed, 0 survived, 0 errors; desktop wait evidence: $work"

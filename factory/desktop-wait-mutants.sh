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
 mkdir -p "$dir"
 cp "$data" "$dir/data.img"
 DESKTOP_SERIAL="$dir/run.serial" INITECH_TEST_RAM_MB=8 \
 "$root/build/factory/desktop-bochs" --disk "$boot" --disk2 "$dir/data.img" \
 --keys-after FLAIR-LIVE-READY --keys "$open_disk,WFINDER-OPEN-VOLUME" \
 --expect FLAIR-LIVE-READY --quit-after FLAIR-LIVE-READY \
 --timeout-ms 60000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"
}
unset DESKTOP_WAIT_MS
probe "$root/build/image/boot.img" "$root/build/image/data.img" "$work/baseline" || error baseline
scratch=$work/source
mkdir -p "$scratch"
# Check acquisition before extraction; do not inherit POSIX pipeline status loss.
tar -cf "$work/source.tar" --exclude=./build --exclude=./.git --exclude=./.beads --exclude=./attic . || error snapshot
tar -xf "$work/source.tar" -C "$scratch" || error extraction
rm "$work/source.tar"
patch --batch --fuzz=0 -p1 -d "$scratch" < factory/mutants/desktop-wait/stall.patch > "$work/patch.log" 2>&1 || error patch
(cd "$scratch" && nice make -j2 image) > "$work/build.log" 2>&1 || error build
for pass in 1 2; do
 rc=0
 probe "$scratch/build/image/boot.img" "$scratch/build/image/data.img" "$work/stall-$pass" || rc=$?
 [ "$rc" -eq 1 ] || error "stall exit $rc"
 grep -q 'no meaningful serial/input progress for 10000ms' "$work/stall-$pass/report" || error 'wrong failure reason'
 grep -Fx FLAIR-LIVE-READY "$work/stall-$pass/run.serial" >/dev/null || error 'stalled product never booted'
 grep -F 'rfb_unblocked=1 timed_out=0' "$work/stall-$pass/report" >/dev/null || error transport
 grep -Fx '[bochs] triple_fault=0' "$work/stall-$pass/report" >/dev/null || error fault
 grep -F "exit=-1 signal=15" "$work/stall-$pass/report" >/dev/null || error "unexpected emulator exit"
 grep -F "rfb_error=(none)" "$work/stall-$pass/report" >/dev/null || error "display transport"
 echo "killed desktop-stall (semantic missing Finder marker, pass $pass)"
done
echo "factory: 1 mutants, 1 killed, 0 survived, 0 errors; desktop wait evidence: $work"

#!/bin/sh
# Reject missing/failed/partial observations before any absence oracle.
set -eu
mkdir -p build
w=$(mktemp -d build/emu-trace-test.XXXXXX)
trap 'rm -rf "$w"' EXIT
printf 'SHELL-READY\nSHELL-DONE\n' > "$w/serial"
card() {
 printf '[harness] launched=1 timed_out=0 exit=%s signal=0\n' "$1"
 printf '[harness] keys_sent=%s mouse_events_sent=0 screendump_taken=0 path=(none)\n' "$2"
 printf '[harness] OK=%s\n' "$3"
}
card 0 9 1 > "$w/report"
sh harness/emu/require_trace.sh "$w/report" "$w/serial" 9
for case_ in missing failed short empty; do
 card 0 9 1 > "$w/report"
 printf 'SHELL-DONE\n' > "$w/serial"
 case $case_ in
 missing) rm -f "$w/serial" ;;
 failed) card 1 9 0 > "$w/report" ;;
 short) card 0 8 1 > "$w/report" ;;
 empty) : > "$w/serial" ;;
 esac
 if sh harness/emu/require_trace.sh "$w/report" "$w/serial" 9 > "$w/log" 2>&1; then
  echo "RED emulator trace accepted $case_"; exit 1
 fi
done
# End-to-end: a failed harness must never mutation-prove the boot oracle,
# even if a previous successful run left serial/report files behind.
card 0 9 1 > "$w/report"
printf 'SHELL-READY\nSHELL-DONE\n' > "$w/serial"
printf '#!/bin/sh\necho INJECTED-HARNESS-FAILURE >&2\nexit 1\n' > "$w/failed-harness"
chmod 755 "$w/failed-harness"
# initech-1x6x2.1: this nested make shares build/ with the parallel merge
# vector, so it must run ONLY the test-samir-boot-mutant recipe. Every file
# that recipe depends on is a prerequisite of test-samir-trace (built once, by
# the outer make) and is assumed old here (-o), so this make can never become
# a second producer of build/tracer_secondary.img or its kernel chain while
# other legs boot it (the cold-run triple faults). The Makefile passes the list.
: "${TRACE_ASSUME_OLD:?set by the test-samir-trace recipe (the boot-mutant inputs)}"
old="-o $PWD/$w/failed-harness"
for f in $TRACE_ASSUME_OLD; do
 [ -f "$f" ] || { echo "RED test-samir-trace: prerequisite $f missing (declare it on test-samir-trace)"; exit 1; }
 old="$old -o $f"
done
# Phony gates the boot mutant is ordered after (harness/gate_pairs.mk): done.
for g in ${TRACE_ASSUME_DONE:-}; do old="$old -o $g"; done
rc=0
# shellcheck disable=SC2086
${MAKE:-make} --no-print-directory $old test-samir-boot-mutant HARNESS_BIN="$PWD/$w/failed-harness" \
 SAMIRBOOT_MUT_NAME=trace_failed SAMIRBOOT_MUT_REPORT="$w/report" SAMIRBOOT_MUT_SERIAL="$w/serial" > "$w/boot.log" 2>&1 || rc=$?
[ "$rc" != 0 ] || { cat "$w/boot.log"; echo 'RED boot mutant accepted failed harness'; exit 1; }
grep -qx INJECTED-HARNESS-FAILURE "$w/report" || {
 cat "$w/boot.log"; echo 'RED test-samir-trace: boot mutant failed before running the injected harness'; exit 1; }
echo 'GREEN test-samir-trace: missing/empty/partial/failed captures rejected; boot mutant fails when harness fails'

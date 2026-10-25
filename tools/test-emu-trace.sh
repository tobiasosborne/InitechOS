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
rc=0
make --no-print-directory test-samir-boot-mutant HARNESS_BIN="$PWD/$w/failed-harness" \
 SAMIRBOOT_MUT_NAME=trace_failed SAMIRBOOT_MUT_REPORT="$w/report" SAMIRBOOT_MUT_SERIAL="$w/serial" > "$w/boot.log" 2>&1 || rc=$?
[ "$rc" != 0 ] || { cat "$w/boot.log"; echo 'RED boot mutant accepted failed harness'; exit 1; }
grep -qx INJECTED-HARNESS-FAILURE "$w/report"
echo 'GREEN test-samir-trace: missing/empty/partial/failed captures rejected; boot mutant fails when harness fails'

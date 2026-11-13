#!/bin/sh
# Independent serial oracle for the factory reset fixture, PRD Sec 8.
set -eu
serial=$1
count=$(grep -cxF BOCHS-REBOOT-READY "$serial" || true)
[ "$count" = 2 ] || { printf 'RED bochs-reboot: READY seen %s times, want 2 (must wait for reboot)\n' "$count"; exit 1; }
grep -qxF BOCHS-REBOOT-DONE "$serial" || { echo 'RED bochs-reboot: second boot did not finish'; exit 1; }
[ "$(grep -cxF BOCHS-REBOOT-PULSE "$serial")" = 1 ] || { echo 'RED bochs-reboot: want exactly one reset pulse'; exit 1; }
! grep -qxF BOCHS-REBOOT-FAIL "$serial" || { echo 'RED bochs-reboot: fixture failed'; exit 1; }
echo 'GREEN test-bochs-reboot: READY -> 8042 pulse -> DONE -> READY (second boot)'

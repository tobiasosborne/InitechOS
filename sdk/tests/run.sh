#!/bin/sh
# ABI/fake/container checks under the shared timeout runner.
set -eu
out=$1
case $2 in
 abi) "$out/tests/audit" tests/traps.tsv include/toolbox_gate.h include/cp_dialog.h rt/tbx_rt.c "$TOP/flair/tbxgate.c";;
 host) "$out/tests/host";;
 conformance) sh tests/live/inventory.sh "$out/tests"; "$out/tests/conformance" tests/live/gate-results.bin;;
 dispatch) "$out/tests/dispatch" "$TOP/flair/tbxgate.c" tests/traps.tsv;;
 search33) "$out/tests/search33" "$TOP/kernel/tests/fixtures/search33/matrix.bin";;
 host32) sh tests/host32.sh "${HOSTCC:-cc}" "$out/tests/host32";;
 mz) make --no-print-directory -C "$TOP/factory/mz" test;;
 sweep)
  # The live toy passes the sweep; the dead-Beep toy must fail it on that
  # row, or the engine proves nothing (GOLDEN-MASTER item 4).
  "$out/tests/sweep-live"
  rc=0; "$out/tests/sweep-dead" > "$out/tests/sweep-dead.log" 2>&1 || rc=$?
  if [ "$rc" -eq 1 ] && grep -q 'Beep' "$out/tests/sweep-dead.log"; then
   echo 'sdk sweep: dead Beep row found: 1 checks, 0 failures'
  else
   cat "$out/tests/sweep-dead.log"; echo "FAIL sweep: dead-Beep toy exited $rc"; echo 'sdk sweep: 1 checks, 1 failures'; exit 1
  fi;;
 *) exit 1;;
esac

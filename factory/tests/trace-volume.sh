#!/bin/sh
# Regression for the volume opener: between separate clicks the guest's
# double-click period can expire. Real input and Finder's serial receipts are
# transcribed from the plain-main Bochs Word/Text opener (initech-fvicp).
set -eu
cd "$(dirname "$0")/../.."
. factory/desktop-trace.sh
markers=/dev/null
app_entries='::/APPS/WORD.EXE'
trace=$(open_volume; inject "$open_apps"; launch_app WORD.EXE)
# Model only the delayed deliveries up to folder navigation. Every release
# produces a fresh selection. File Open works only on a selected volume.
replay() {
 printf '%s\n' "$trace" | tr ',' '\n' | awk -v deliver_open="$1" '
  /^m/ {next}
  $0=="l1" {down=1; next}
  $0=="l0" && down {selected=1; down=0; next}
  $0=="WFINDER-ICON-SELECT name=" && selected {ack=1; next}
  $0=="kctrl-o" && ack {opened=deliver_open; next}
  $0=="WFINDER-OPEN-VOLUME" {
   if (!opened) {print "stopped at volume wait"; exit 1}
   confirmed=1; next
  }
  $0=="kup" && confirmed {print "started folder navigation"; exit}
  {print "unexpected input: " $0; exit 1}
 '
}
if ! result=$(replay 1) || [ "$result" != 'started folder navigation' ]; then
 echo "FAIL delayed volume opener: $result" >&2
 exit 1
fi
rc=0
result=$(replay 0) || rc=$?
[ "$rc" -eq 1 ] && [ "$result" = 'stopped at volume wait' ] ||
 { echo "FAIL select without open: $result" >&2; exit 1; }
echo 'ok acknowledged volume selection, File Open, open receipt before navigation'

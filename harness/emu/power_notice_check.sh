#!/bin/sh
# B04 / IM VI p.345: cancelling a dirty document keeps it and the Finder alive.
set -eu
s=$1
red() { printf 'FAIL power-notice: %s\n' "$*"; exit 1; }
grep -qxF 'FINDER-POWER-CANCEL app=TEXT.EXE declined' "$s" || red 'dirty Text did not cancel shutdown'
grep -qxF 'TENANT-ALERT-HIT item=2' "$s" || red 'dirty Text save prompt was not cancelled'
grep -qxF 'FINDER-POWER-NOTICE' "$s" || red 'no visible explanation'
grep -qxF 'FINDER-ALERT-LINE Shut Down or Restart was cancelled.' "$s" || red 'missing operation'
grep -qxF 'FINDER-ALERT-LINE TEXT.EXE' "$s" || red 'missing app name'
grep -qxF 'FINDER-ALERT-LINE The application did not quit.' "$s" || red 'missing reason'
! grep -qE '^(TENANT-EXIT|TENANT-CRASH|PANIC|FLAIR-SHUTDOWN|FLAIR-RESTART)' "$s" || red 'cancelled app or session ended'
grep -q '^FINDER-NEW-FOLDER name=' "$s" || red 'Finder unusable after acknowledgement'
printf 'GREEN power-notice: dirty Text retained, visible reason acknowledged, Finder usable\n'

#!/bin/sh
# Independent lifecycle golden: guide p.125; IM VI p.345; B04 audit L022.
set -eu
serial=$1
action=$2
red() { printf 'FAIL calcda-power: %s\n' "$*"; exit 1; }
grep -qxF 'TENANT-REGISTER ok app=CALCULAT' "$serial" || red 'Calculator not registered'
grep -q "^FINDER-CMD .*name=$action " "$serial" || red "$action not requested"
grep -qxF 'TENANT-MENU menu=129 item=2 src=quit' "$serial" || red 'Calculator was not asked to quit'
seq=$(sed -n "/^FINDER-CMD .*name=$action /,\$p" "$serial" | sed -n '/^TENANT-MENU /p; /^TENANT-EXIT/p; /^DESKTOP-DB-SAVE/p; /^FLAIR-SHUTDOWN-SAFE/p; /^FLAIR-RESTART$/p; /^FINDER-POWER-CANCEL /p' | head -n 4 | tr '\n' '|')
case "$seq" in
    'TENANT-MENU menu=129 item=2 src=quit|TENANT-EXIT rc=0 via=exit|DESKTOP-DB-SAVE n='*"|FLAIR-$([ "$action" = SHUTDOWN ] && echo SHUTDOWN-SAFE || echo RESTART)|") ;;
    *) red "quit/exit/commit/power order: $seq" ;;
esac
! grep -qE '^(PANIC|TENANT-CRASH|FINDER-POWER-CANCEL)' "$serial" || red 'power request failed'
printf 'GREEN calcda-power %s: own Quit -> exit -> commit -> power\n' "$action"

#!/bin/sh
# initech-cqej: same locked FLAIR_SHUTDOWN_APP_SPEC as the consenting leg.
# Ref: kmain.c finder_power_ask_apps / tbxgate.c tbx_ask_quit, grounded there
# in Inside Macintosh VI PDF p.345 Listing 6-8 (userCanceledErr).
set -eu
serial=$1
red() { printf 'SDC: %s\n' "$*"; exit 1; }
grep -qxF 'FLAIR-DISPATCH app=CTENANT' "$serial" || red 'CTENANT was never launched'
grep -qxF 'FINDER-CMD id=13 name=SHUTDOWN src=mouse sel=1' "$serial" || red 'Shut Down never reached Finder'
# tbx_resident_name returns g_slot.file (CTENANT.EXE), not the app label.
grep -qxF 'FINDER-POWER-CANCEL app=CTENANT.EXE declined' "$serial" || red 'the application did not cancel Shut Down'
seq=$(sed -n '/^FINDER-CMD id=13 name=SHUTDOWN/,/^FINDER-POWER-CANCEL /p' "$serial" |
    sed -n '/^TENANT-MENU /p; /^TENANT-EXIT/p; /^DESKTOP-DB-SAVE/p; /^FLAIR-SHUTDOWN/p; /^FINDER-POWER-CANCEL /p' | tr '\n' '|')
[ "$seq" = 'TENANT-MENU menu=129 item=1 src=quit|FINDER-POWER-CANCEL app=CTENANT.EXE declined|' ] || red "quit/cancel order or premature put-away: $seq"
! grep -qE '^(PANIC|TENANT-EXIT|TENANT-CRASH|FLAIR-SHUTDOWN|FLAIR-RESTART)' "$serial" || red 'cancelled session halted, reset, crashed or killed the application'
sed -n '/^FINDER-POWER-CANCEL /,$p' "$serial" | grep -q '^FINDER-CMD id=2 name=NEW_FOLDER ' || red 'Finder did not answer Ctrl-N after the cancel'
printf 'GREEN shutdown-cancel: own File > Quit -> declined -> FINDER-POWER-CANCEL; tenant alive, Finder answers Ctrl-N\n'

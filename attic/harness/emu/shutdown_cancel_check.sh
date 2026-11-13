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
# initech-89sb: the Ctrl-N probe WRITES the volume (fat12_mkdir: both FAT
# copies, then the '.'/'..' sector, then the parent entry). The gate quits on
# FINDER-NEW-FOLDER, printed only after fat12_mkdir returned (kmain.c
# finder_report_outcome), so the probe's write must be complete AND on disk.
sed -n '/^FINDER-CMD id=2 name=NEW_FOLDER /,$p' "$serial" | grep -q '^FINDER-NEW-FOLDER name=' ||
    red 'the Ctrl-N folder write never completed before the harness quit'
if [ $# -ge 2 ]; then
    name=$(sed -n 's/^FINDER-NEW-FOLDER name=\([^ ]*\) parent=.*/\1/p' "$serial" | head -n 1)
    parent=$(sed -n 's/^FINDER-NEW-FOLDER name=[^ ]* parent=\([0-9]*\)$/\1/p' "$serial" | head -n 1)
    [ -n "$name" ] && [ -n "$parent" ] || red 'FINDER-NEW-FOLDER carries no name/parent'
    # The folder must be linked into the directory whose first cluster is the
    # reported parent (0 = the fixed root), read back by mtools.
    hit=0
    for path in $(mdir -i "$2" -/ -b :: 2>/dev/null | grep "/$name/\$"); do
        pdir=${path%/"$name"/}
        if [ "$pdir" = '::' ]; then pc=0
        else pc=$(mshowfat -i "$2" "$pdir" | sed -n 's/^[^<]*<\([0-9]*\).*/\1/p'); fi
        [ "$pc" = "$parent" ] && hit=1
    done
    [ "$hit" = 1 ] || red "folder $name (parent $parent) reported created but absent from the volume"
fi
printf 'GREEN shutdown-cancel: own File > Quit -> declined -> FINDER-POWER-CANCEL; tenant alive, Finder answers Ctrl-N with a folder on the volume\n'

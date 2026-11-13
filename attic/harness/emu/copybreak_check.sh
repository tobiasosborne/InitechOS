#!/bin/sh
# Audit DAS L002; Macintosh System Software User's Guide pp.125,152;
# spec/toolbox_gate.h Sec 12. Shared by clean, leak mutant and recorder.
set -eu
s=$1
fail() { echo "RED copybreak: $*"; exit 1; }
! grep -qE '^(INT23-BREAK|INT21-EXIT|PANIC|KBD-RING-DROP|FLAIR-EVT-DROP)' "$s" || fail 'GUI key reached DOS break/exit or input dropped'
[ "$(grep -c '^TENANT-REGISTER ok app=SCRAPBK$' "$s")" = 2 ] || fail 'Scrapbook did not relaunch'
grep -qx 'TENANT-SCRAP put n=5 count=1' "$s" || fail 'Copy did not publish alpha once'
[ "$(grep -c '^TENANT-SCRAP put ' "$s")" = 1 ] || fail 'Copy was delivered more than once'
grep -qx 'TENANT-MENU menu=130 item=5 src=key' "$s" || fail 'Paste chord missing'
awk '/^TENANT-SCRAP put n=5 count=1$/{copied=1}
     copied && /^FLAIR-CLOCK-POLL$/{polled=1}
     /^TENANT-MENU menu=130 item=5 src=key$/{pasted=1; if (!polled) bad=1}
     END{exit !(pasted && polled && !bad)}' "$s" || fail 'Paste did not follow a completed idle clock poll'
grep -q 's="2 of 2"' "$s" || fail 'Paste did not append a second clip'
grep -q '^FLAIR-DRAG ' "$s" || fail 'desktop did not drag after Copy'
grep -q '^FLAIR-CLOSE-ASK win ' "$s" && grep -qx 'TENANT-EXIT rc=0 via=exit' "$s" || fail 'close missing'
grep -q '^DA-LOADED SCRAPBK.DAT' "$s" || fail 'relaunch did not load saved clips'
echo 'GREEN copybreak: Copy, clock-poll idle, Paste, drag, close, relaunch'

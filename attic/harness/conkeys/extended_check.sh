#!/bin/sh
# initech-wuxo0; IBM PC AT TR Sep 1985, System BIOS pp.5-18--5-19.
# Expected bytes are independent literals; never read the artifact table.
set -eu
name=$1; serial=$2; report=$3
fail() { printf 'RED %s: %s\n' "$name" "$*"; exit 1; }
tr -d '\r' < "$serial" > "$serial.norm"
for m in IRQ-LIVE CONIN-LIVE KBD-ECHO-READY KBD-ECHO-BEGIN KBD-ECHO-END; do
    grep -qxF "$m" "$serial.norm" || fail "missing $m"
done
grep '^KBD-BYTES=' "$serial.norm" > "$serial.hex"
cmp -s harness/conkeys/extended.hex "$serial.hex" || fail 'IBM extended byte transcript differs'
grep -q 'triple_fault=0' "$report" || fail 'no clean triple-fault report'
count=$(tr ',' '\n' < harness/conkeys/extended.keys | wc -l)
grep -Eq "keys_sent=$count([^0-9]|$)" "$report" || fail "keys_sent != $count"
if grep -Eq '^PANIC|KBD-RING-DROP' "$serial.norm"; then fail 'panic or ring overflow'; fi
printf '>>> %s: green (%s keys, exact IBM NUL/scan bytes via INT 21h 07h/08h; triple_fault=0)\n' "$name" "$count"

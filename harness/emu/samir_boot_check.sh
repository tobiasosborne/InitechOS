#!/bin/sh
# ADR-0009 DEC-08 / PRD Sec 6.6; the SAME CLIENTS value fixture and M005U/M005N
# LIST layout asserted by test-samir-boot (initech-31d1, mint-results-005).+# Scope past EXEC so a boot directory listing cannot supply a record token.
set -eu
name=$1; serial=$2
fail() { printf 'RED %s serial: %s\n' "$name" "$*"; exit 1; }
body=$(tr -d '\r' < "$serial" | sed -n '/^A:\\>samir$/,$p')
has() { printf '%s\n' "$body" | grep -qxF "$1"; }
has '. use clients.dbf' || fail 'SAMIR dot prompt / USE echo missing'
has '. list' || fail 'SAMIR dot prompt / LIST echo missing'
for row in \
    'Record#  NAME       CITY              BAL' \
    '      1  PESTON     HONOLULU      1234.50' \
    '      2  WADDAMS    AUSTIN         -42.00' \
    '      3  LUMBERGH   DALLAS        7777.77'; do
    has "$row" || fail "LIST row missing: $row"
done
printf '>>> %s: serial green (SAMIR dot prompt + USE/LIST echoes + all 3 exact CLIENTS.DBF rows)\n' "$name"

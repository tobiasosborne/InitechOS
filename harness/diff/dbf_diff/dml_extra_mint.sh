#!/bin/sh
# Real III+ probe companion. See dml_probe.c and the DML evidence ledger.
set -eu
ref=${1:-../dbase3-decomp}
root=$(pwd)
work="$root/build/samirdml/extra-mint"
mkdir -p "$work"
cc -std=c11 -Wall -Wextra -Werror harness/diff/dbf_diff/dml_probe.c -o build/samirdml/dml_probe
build/samirdml/dml_probe "$work"
for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do cp -f "$ref/mint/work/$file" "$work/$file"; done
cp -f build/samirdml/golden/label/CLIENTS.DBF "$work/CLIENTS.DBF"
cp -f pubs/examples/samir/salary.prg "$work/SALARY.PRG"
cp -f build/samirdml/golden/label/CONFIG.DB "$work/CONFIG.DB"
printf 'SET PROCEDURE TO EXTRA\r\nDO EXTRA\r\nQUIT\r\n' > "$work/MINT.PRG"
sed 's@golden/label@extra-mint@g' build/samirdml/golden/label/mint.conf > "$work/mint.conf"
rm -f "$work"/LO*.TXT "$work"/OUT*.BIN "$work/SALARY.OUT" "$work"/SCOPE*.MEM
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -k 2 20 dosbox-x -defaultconf -nopromptfolder -fastlaunch -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1 || :
for i in 0 1 2 3 4; do test -s "$work/LO$i.TXT"; done
for i in 0 1 2 3 4 5; do test -f "$work/OUT$i.BIN"; done
test -s "$work/SALARY.OUT"
# ALTERNATE adds the DOS EOF byte; a terminal transcript does not contain it.
last=$(tail -c 1 "$work/SALARY.OUT" | od -An -tu1 | tr -d ' ')
test "$last" = 26
size=$(stat -c%s "$work/SALARY.OUT")
head -c "$((size-1))" "$work/SALARY.OUT" > pubs/examples/samir/salary.txt
(cd "$work" && sha256sum EXTRA.PRG LP*.LBL LO*.TXT IN*.BIN OUT*.BIN SALARY.PRG SALARY.OUT SCOPE0.MEM SCOPE1.MEM) > "$work/sha256.txt"
echo 'MINT EXTRA-01..05 labels; EXTRA-06..11 copy boundaries; EXTRA-12..13 MEM scope; PUBS-01 salary'

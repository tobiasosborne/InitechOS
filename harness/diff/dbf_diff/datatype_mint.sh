#!/bin/sh
# Real dBASE III PLUS mint for the SAMIR data-typing lane (initech-ylqs;
# audit docs/audits/2026-10-23-samir L001/L006/L007/L016). PRD 6.6: the
# goldens are real III+ bytes, never SAMIR output.
#
# Runs DT1..DT5.PRG in ONE fresh C: (they run in order; DT3-DT5 read the E5
# table DT2 wrote) with the audit's EDGE.DBF/.DBT fixture copied to E1..E5,
# then checks every output against the committed goldens in
# harness/diff/dbf_diff/xbase_prog_diff/datatype-mint/sha256.txt.
# `datatype_mint.sh <dbase3-decomp> update` rewrites the committed goldens.
set -eu
ref=${1:-../dbase3-decomp}
mode=${2:-verify}
root=$(pwd)
src="$root/harness/diff/dbf_diff/xbase_prog_diff/datatype-mint"
work="$root/build/samirdt/mint"
rm -rf "$work"
mkdir -p "$work"
for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
    cp -f "$ref/mint/work/$file" "$work/$file"
done
printf 'STATUS = OFF\r\nBELL = OFF\r\n' > "$work/CONFIG.DB"
for n in 1 2 3 4 5; do
    cp -f "$src/DT$n.PRG" "$work/DT$n.PRG"
    cp -f "$src/EDGE.DBF" "$work/E$n.DBF"
    cp -f "$src/EDGE.DBT" "$work/E$n.DBT"
done
{
    printf '[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=8\n'
    printf '[cpu]\ncore=auto\ncputype=386\ncycles=max\n[autoexec]\n'
    printf 'mount c "%s"\nc:\nDATE 07/24/1986\n' "$work"
    for n in 1 2 3 4 5; do printf 'DBASE DT%s\n' "$n"; done
    printf 'exit\n'
} > "$work/mint.conf"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout --foreground -k 2 \
    "${DT_MINT_TIMEOUT:-180}" dosbox-x -defaultconf -fastlaunch -nopromptfolder \
    -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1 || :
outs="DT1LOG.TXT DT2LOG.TXT DT3LOG.TXT DT4LOG.TXT DT5LOG.TXT BLANKDAT.NDX E1.DBF E2.DBF E4.DBF E5.DBF"
for f in $outs; do
    test -s "$work/$f" || { echo "RED: MINT impossible: $f missing; see $work/mint.log"; exit 1; }
done
(cd "$work" && sha256sum $outs) > "$work/sha256.txt"
if [ "$mode" = update ]; then
    for f in $outs; do cp -f "$work/$f" "$src/$f"; done
    cp -f "$work/sha256.txt" "$src/sha256.txt"
    echo "MINT datatype: goldens updated in $src"
else
    cmp "$work/sha256.txt" "$src/sha256.txt" || { echo "RED: real III+ no longer reproduces the committed datatype goldens"; exit 1; }
    echo "MINT datatype: real III+ reproduces the committed goldens"
fi

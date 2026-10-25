#!/bin/sh
# PRD 6.6: real III PLUS 1.1 only; independent bytes, never SAMIR output.
# Manual printed pages and capture hashes are recorded by the lane ledger.
set -eu
ref=${1:-../dbase3-decomp}
only=${2:-all}
root=$(pwd)
dest="$root/build/samirfns/golden"
mkdir -p "$dest"
for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
    cp -f "$ref/mint/work/$file" "$dest/$file"
done
printf 'STATUS=OFF\r\nTALK=OFF\r\nSAFETY=OFF\r\nBELL=OFF\r\n' > "$dest/CONFIG.DB"
printf 'PROCEDURE LIBONE\r\n? "LIBRARY ONE"\r\nRETURN\r\nPROCEDURE LIBTWO\r\n? "LIBRARY TWO"\r\nRETURN\r\n' > "$dest/LIBRARY.PRG"
printf 'SET TALK OFF\r\nSET SAFETY OFF\r\nON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()\r\n' > "$dest/MINT.PRG"
for input in "$root"/harness/diff/dbf_diff/xbase_prog_diff/corpus/fns/*.prg; do
    name=$(basename "$input" .prg)
    if [ "$only" != all ] && [ "$only" != "$name" ]; then continue; fi
    work="$dest/$name"
    mkdir -p "$work/DATA"
    cp -f "$input" "$work/FNSTEST.PRG"
    cp -f "$dest/LIBRARY.PRG" "$work/LIBRARY.PRG"
    # Optional external DO programs are inputs to both real III+ and SAMIR.
    for helper in "$root/harness/diff/dbf_diff/xbase_prog_diff/corpus/fns/$name/"*.PRG; do
        [ -f "$helper" ] || continue
        cp -f "$helper" "$work/$(basename "$helper")"
    done
    cp -f "$root/build/CLIENTS.DBF" "$work/BASE.DBF"
    cp -f "$root/build/CLIENTS.DBF" "$work/OTHER.DBF"
    cp -f "$root/build/CLIENTS.DBF" "$work/DATA/BASE.DBF"
    printf '? "PROGRAM IN DATA"\r\nRETURN\r\n' > "$work/DATA/SUBPROG.PRG"
    if [ "$name" = path ] || [ "$name" = default ]; then rm -f "$work/BASE.DBF"; fi
    # Each directory is a fresh mounted C: so state/file ownership cannot leak.
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG CONFIG.DB; do cp -f "$dest/$file" "$work/$file"; done
    printf 'SET TALK OFF\r\nSET SAFETY OFF\r\nON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()\r\n' > "$work/MINT.PRG"
    # III+ loads named procedures explicitly, as in prog_mint.sh.
    if grep -iq '^[[:space:]]*PROCEDURE[[:space:]]' "$input"; then
        printf 'SET PROCEDURE TO FNSTEST\r\n' >> "$work/MINT.PRG"
    fi
    printf 'SET ALTERNATE TO GOLD.OUT\r\nSET ALTERNATE ON\r\nDO FNSTEST\r\nSET ALTERNATE TO\r\nQUIT\r\n' >> "$work/MINT.PRG"
    MINT_PROBE=0 MINT_TIMEOUT="${FNS_MINT_TIMEOUT:-90}" sh "$root/harness/diff/dbf_diff/mint_capture.sh" "$work" "$input" '[FNS-END]'
    (cd "$work" && sha256sum FNSTEST.PRG GOLD.OUT
     for helper in "$root/harness/diff/dbf_diff/xbase_prog_diff/corpus/fns/$name/"*.PRG; do
         [ -f "$helper" ] || continue
         sha256sum "$(basename "$helper")"
     done) > "$work/sha256.txt"
    printf 'MINT SF-%s: %s/GOLD.OUT\n' "$name" "$work"
done

#!/bin/sh
# Independent replacement of the old authored Tier-0 transcripts.
# Real III+ 1.1 / re/harness-setup.md; no SAMIR output becomes an expectation.
set -eu
ref=${1:-../dbase3-decomp}
root=$(pwd)
for name in expr funcs exact flow proc query mutate; do
    work="$root/build/samirdml/prog-mint/$name"
    mkdir -p "$work"
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do cp -f "$ref/mint/work/$file" "$work/$file"; done
    awk '{ sub(/\r$/, ""); printf "%s\r\n", $0 }' "harness/diff/dbf_diff/xbase_prog_diff/corpus/$name.prg" > "$work/TEST.PRG"
    build/prog_diff --mint-parts "$work/PARTS.DBF"
    printf 'STATUS=OFF\r\nTALK=OFF\r\nSAFETY=OFF\r\nBELL=OFF\r\n' > "$work/CONFIG.DB"
    printf 'SET TALK OFF\r\nSET SAFETY OFF\r\nON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()\r\n' > "$work/MINT.PRG"
    if [ "$name" = proc ]; then printf 'SET PROCEDURE TO TEST\r\n' >> "$work/MINT.PRG"; fi
    if [ "$name" = mutate ]; then printf 'USE PARTS\r\n' >> "$work/MINT.PRG"; fi
    printf 'SET ALTERNATE TO REAL.OUT\r\nSET ALTERNATE ON\r\nDO TEST\r\nSET ALTERNATE TO\r\nQUIT\r\n' >> "$work/MINT.PRG"
    cat > "$work/mint.conf" <<CONF
[sdl]
output=surface
[dosbox]
machine=svga_s3
memsize=8
[cpu]
core=normal
cputype=386
cycles=60000
[autoexec]
mount c "$work"
c:
DATE 07/24/1986
DBASE MINT
exit
CONF
    rm -f "$work/REAL.OUT"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -k 2 20 dosbox-x -defaultconf -nopromptfolder -fastlaunch -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1 || :
    test -s "$work/REAL.OUT" || { echo "RED: mint $name missing output"; exit 1; }
    if grep -q MINT-ERROR "$work/REAL.OUT"; then cat "$work/REAL.OUT"; exit 1; fi
    # Copy bytes from the real process, including its DOS EOF. No hand edits.
    cp -f "$work/REAL.OUT" "harness/diff/dbf_diff/xbase_prog_diff/golden/$name.out"
    if [ "$name" = mutate ]; then
        python3 harness/diff/dbf_diff/dbf_ref.py --records "$work/PARTS.DBF" > "$work/PARTS.records"
        cp -f "$work/PARTS.records" harness/diff/dbf_diff/xbase_prog_diff/golden/mutate.dbf.out
    fi
    (cd "$work" && sha256sum TEST.PRG MINT.PRG REAL.OUT PARTS.DBF) > "$work/sha256.txt"
    echo "MINT PROG-$name: $work/REAL.OUT"
done

#!/bin/sh
# Real III+ only. Harness recipe: dbase3-decomp/re/harness-setup.md.
# Never read expected output from SAMIR; never modify the sister corpus.
set -eu
corpus=${1:-../dbase3-decomp}
if [ "$#" -gt 0 ]; then shift; fi
root=$(pwd)
dest=${SAMIR_DML_MINT_DEST:-"$root/build/samirdml/golden"}
source="$root/harness/diff/dbf_diff/xbase_prog_diff/corpus/dml"
command -v dosbox-x >/dev/null || { echo 'RED: MINT required: dosbox-x missing'; exit 1; }
mkdir -p "$dest"
for name in ${*:-clear macro close index reindex sort total append save restore copyfile type rename erase label run abbrev abbrfull abbrscope}; do
    work="$dest/$name"
    rm -rf "$work"
    mkdir -p "$work"
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
        cp -f "$corpus/mint/work/$file" "$work/$file"
    done
    cp -f build/CLIENTS.DBF "$work/BASE.DBF"
    printf 'ASCII first\r\nsecond line\r\n\032IGNORED\r\n' > "$work/INPUT.TXT"
    cp -f "$source/$name.prg" "$work/DMLTEST.PRG"
    printf 'STATUS=OFF\r\nTALK=OFF\r\nSAFETY=OFF\r\nBELL=OFF\r\n' > "$work/CONFIG.DB"
    if [ "$name" = reindex ]; then cp -f "$dest/index/NAMES.NDX" "$work/NAMES.NDX"; fi
    if [ "$name" = restore ]; then cp -f "$dest/save/VALUES.MEM" "$work/VALUES.MEM"; fi
    if [ "$name" = label ]; then
        for file in CLIENTS.DBF ADDRESS.LBL; do
            cp -f "$corpus/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities/$file" "$work/$file"
        done
    fi
    # Field-order mismatch for APPEND FROM: real III+ COPY creates the input.
    printf 'SET TALK OFF\r\nSET SAFETY OFF\r\nON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()\r\n' > "$work/MINT.PRG"
    if [ "$name" = append ]; then
        printf 'USE BASE\r\nCOPY TO IMPORT FIELDS CITY,BAL,NAME\r\nUSE\r\n' >> "$work/MINT.PRG"
    fi
    if [ "$name" = abbrscope ]; then
        # III+ creates the independent input too: five scope-like field names,
        # three blank records. INPUT.DBF is pinned and replayed by both PALs.
        cat >> "$work/MINT.PRG" <<'EOF'
USE BASE
COPY STRUCTURE EXTENDED TO SHAPE
USE SHAPE
GO 2
REPLACE FIELD_NAME WITH 'REST'
GO 3
REPLACE FIELD_NAME WITH 'NEXT', FIELD_TYPE WITH 'C', FIELD_LEN WITH 10, FIELD_DEC WITH 0
APPEND BLANK
REPLACE FIELD_NAME WITH 'ALL', FIELD_TYPE WITH 'C', FIELD_LEN WITH 10, FIELD_DEC WITH 0
APPEND BLANK
REPLACE FIELD_NAME WITH 'RECORD', FIELD_TYPE WITH 'C', FIELD_LEN WITH 10, FIELD_DEC WITH 0
USE
CREATE INPUT FROM SHAPE
APPEND BLANK
APPEND BLANK
APPEND BLANK
COPY TO BASE
USE
EOF
    fi
    case "$name" in clear|abbrev|abbrfull) printf 'SET PROCEDURE TO DMLTEST\r\n' >> "$work/MINT.PRG";; esac
    printf 'SET ALTERNATE TO GOLD.OUT\r\nSET ALTERNATE ON\r\nDO DMLTEST\r\nSET ALTERNATE TO\r\nQUIT\r\n' >> "$work/MINT.PRG"
    cat > "$work/mint.conf" <<EOF
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
EOF
    # dosbox-x may retain its window after QUIT. A flushed alternate with BOTH
    # markers, no trapped error, and ^Z is required even when timeout kills it.
    rm -f "$work/GOLD.OUT"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -k 2 20 dosbox-x -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1 || :
    test -s "$work/GOLD.OUT" || { echo "RED: MINT required: $name produced no GOLD.OUT ($work/mint.log)"; exit 1; }
    grep -q '\[DML-END\]' "$work/GOLD.OUT" || { echo "RED: MINT required: $name did not finish"; exit 1; }
    if grep -q 'MINT-ERROR' "$work/GOLD.OUT"; then cat "$work/GOLD.OUT"; exit 1; fi
    if [ "$name" = abbrscope ] && grep -q 'SCOPE-ERROR' "$work/GOLD.OUT"; then cat "$work/GOLD.OUT"; exit 1; fi
    (cd "$work" && sha256sum DMLTEST.PRG GOLD.OUT ./*.DBF ./*.NDX ./*.MEM ./*.LBL ./*.TXT 2>/dev/null || :) > "$work/sha256.txt"
    printf 'MINT samirdml/%s: %s/GOLD.OUT\n' "$name" "$work"
done

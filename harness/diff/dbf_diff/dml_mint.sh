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
for name in ${*:-clear macro close index reindex sort total append save restore copyfile type rename erase label run abbrev abbrfull abbrscope status memory statusinto}; do
    work="$dest/$name"
    rm -rf "$work"
    mkdir -p "$work"
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
        cp -f "$corpus/mint/work/$file" "$work/$file"
    done
    cp -f build/CLIENTS.DBF "$work/BASE.DBF"
    if [ "$name" = status ]; then
        cp -f build/CLIENTS.DBF "$work/SECOND.DBF"
        cp -f build/CLIENTS.DBF "$work/THIRD.DBF"
        cp -f "$root/docs/audits/2026-10-23-samir/evidence/fixtures/EDGE.DBF" "$root/docs/audits/2026-10-23-samir/evidence/fixtures/EDGE.DBT" "$work/"
    fi
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
    if [ "$name" = statusinto ]; then
        # Real III+ creates the PAINTO field; both PALs replay this input.
        cat >> "$work/MINT.PRG" <<'EOF'
USE BASE
COPY STRUCTURE EXTENDED TO SHAPE
USE SHAPE
GO TOP
REPLACE FIELD_NAME WITH 'PAINTO'
USE
CREATE INPUT FROM SHAPE
APPEND BLANK
REPLACE PAINTO WITH 'ALPHA'
USE
EOF
    fi
    case "$name" in clear|abbrev|abbrfull|memory) printf 'SET PROCEDURE TO DMLTEST\r\n' >> "$work/MINT.PRG";; esac
    case "$name" in
        status|memory|statusinto) printf 'DO DMLTEST\r\nQUIT\r\n' >> "$work/MINT.PRG";;
        *) printf 'SET ALTERNATE TO GOLD.OUT\r\nSET ALTERNATE ON\r\nDO DMLTEST\r\nSET ALTERNATE TO\r\nQUIT\r\n' >> "$work/MINT.PRG";;
    esac
    drive=c
    keys=
    mint_timeout=20
    mapper=
    case "$name" in status|memory|statusinto)
        # spec/drive_letters.json: the locked QEMU data disk is A:. Mint the
        # same drive identity, rather than normalizing filenames in a diff.
        drive=a
        mint_timeout=45
        mapper=-defaultmapper
        # Keep the DOS command below 127 bytes. Keys span the complete run;
        # sending them all before a later DISPLAY would lose type-ahead.
        keys="AUTOTYPE -w 2 -p 2$(printf ' space%.0s' $(seq 1 16))";;
    esac
    if [ "$name" = statusinto ]; then
        mapper="$mapper -fastlaunch"
        keys= # LIST-only boundary regression; DISPLAY's existing mint is separate.
    fi
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
mount $drive "$work"
$drive:
DATE 07/24/1986
$keys
DBASE MINT
exit
EOF
    # dosbox-x may retain its window after QUIT. A flushed alternate with BOTH
    # markers, no trapped error, and ^Z is required even when timeout kills it.
    rm -f "$work/GOLD.OUT"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout -k 2 "$mint_timeout" dosbox-x $mapper -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1 || :
    test -s "$work/GOLD.OUT" || { echo "RED: MINT required: $name produced no GOLD.OUT ($work/mint.log)"; exit 1; }
    grep -q '\[DML-END\]' "$work/GOLD.OUT" || { echo "RED: MINT required: $name did not finish"; exit 1; }
    case "$name" in status|memory|statusinto)
        test "$(tail -c 1 "$work/GOLD.OUT" | od -An -tu1 | tr -d ' ')" = 26 || {
            echo "RED: MINT required: $name alternate did not close with DOS EOF"; exit 1;
        };;
    esac
    # STATUS prints the installed ON ERROR source itself. Only a line starting
    # with the handler's output marker denotes a trapped error in that case.
    case "$name" in status|statusinto) error_pattern='^MINT-ERROR';; *) error_pattern='MINT-ERROR';; esac
    if grep -q "$error_pattern" "$work/GOLD.OUT"; then cat "$work/GOLD.OUT"; exit 1; fi
    if [ "$name" = abbrscope ] && grep -q 'SCOPE-ERROR' "$work/GOLD.OUT"; then cat "$work/GOLD.OUT"; exit 1; fi
    (cd "$work" && sha256sum DMLTEST.PRG GOLD.OUT ./*.DBF ./*.NDX ./*.MEM ./*.LBL ./*.TXT 2>/dev/null || :) > "$work/sha256.txt"
    printf 'MINT samirdml/%s: %s/GOLD.OUT\n' "$name" "$work"
done

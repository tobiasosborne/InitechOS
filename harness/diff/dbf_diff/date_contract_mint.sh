#!/bin/sh
# initech-bkmv: re-verify real III+ date evidence, never mint from SAMIR.
# Recipe: dbase3-decomp/re/harness-setup.md sections 1-3, 6.2.
set -eu
ref=${1:?usage: date_contract_mint.sh /absolute/path/to/dbase3-decomp}
root=$(pwd)
src="$root/harness/diff/dbf_diff/xbase_prog_diff/datatype-mint"
dest="$root/build/bkmv/remint"
(cd "$ref/mint/work" && sha256sum --quiet -c "$src/datecontract/reference.sha256")
for probe in DATES CMP CMP2 VERIFY DATECHK; do
    work="$dest/$probe"
    rm -rf "$work"
    mkdir -p "$work"
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
        cp -f "$ref/mint/work/$file" "$work/$file"
    done
    cp -f "$src/datecontract/$probe.PRG" "$work/$probe.PRG"
    cp -f "$src/EDGE.DBF" "$work/E7.DBF"
    cp -f "$src/EDGE.DBT" "$work/E7.DBT"
    printf 'STATUS=OFF\r\nSAFETY=OFF\r\nTALK=OFF\r\nBELL=OFF\r\n' > "$work/CONFIG.DB"
    core=normal; cycles=60000
    if [ "$probe" = CMP2 ]; then core=auto; cycles=max; fi
    {
        printf '[sdl]\noutput=surface\n[dosbox]\nmachine=svga_s3\nmemsize=8\n'
        printf '[cpu]\ncore=%s\ncputype=386\ncycles=%s\n[autoexec]\n' "$core" "$cycles"
        printf 'mount c "%s"\nc:\nDATE 07/24/1986\nTIME 00:00:00\nDBASE %s\nexit\n' "$work" "$probe"
    } > "$work/mint.conf"
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout --foreground -k 5 \
        "${DATE_MINT_TIMEOUT:-50}" dosbox-x -defaultconf -fastlaunch -nopromptfolder \
        -conf "$work/mint.conf" -exit > "$work/mint.log" 2>&1
    case "$probe" in
        DATES) actual=DATELOG.TXT; expected=DATELOG.TXT;;
        CMP) actual=CMPLOG.TXT; expected=CMPLOG.TXT;;
        CMP2) actual=CMP2LOG.TXT; expected=CMP2LOG.TXT;;
        VERIFY) actual=DATECHK.TXT; expected=VERIFY.TXT;;
        DATECHK) actual=DATECHK.TXT; expected=DATECHK.TXT;;
    esac
    test -s "$work/$actual"
    cmp "$work/$actual" "$src/datecontract/$expected"
    printf 'MINT datecontract: %s reproduces real III+ %s\n' "$probe" "$expected"
done

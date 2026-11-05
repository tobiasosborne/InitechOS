#!/bin/sh
# initech-104j3: real III+ 1.1 only; re/harness-setup.md and mint_capture.sh.
# Sources and flushed alternate bytes are pinned; never promote SAMIR output.
set -eu
ref=${1:?absolute dbase3-decomp directory required}
root=$(pwd)
base=harness/diff/dbf_diff/xbase_prog_diff/do-file-mint
for source in "$base"/*; do
    test -d "$source" || continue
    name=${source##*/}
    work="$root/build/do-file-mint/$name"
    rm -rf "$work"
    mkdir -p "$work"
    for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do
        cp -f "$ref/mint/work/$file" "$work/$file"
    done
    for file in "$source"/*.PRG; do
        cp -f "$file" "$work/${file##*/}"
    done
    printf 'STATUS=OFF\r\nTALK=OFF\r\nSAFETY=OFF\r\nBELL=OFF\r\n' > "$work/CONFIG.DB"
    MINT_TIMEOUT=30 sh harness/diff/dbf_diff/mint_capture.sh "$work" "$source/MINT.PRG" '[DO-END]'
    cp -f "$work/GOLD.OUT" "$source/GOLD.OUT"
    (cd "$source" && sha256sum ./*.PRG GOLD.OUT) > "$source/sha256.txt"
    printf 'MINT DO-%s: flushed real III+ GOLD.OUT\n' "$name"
done
(cd "$ref/mint/work" && sha256sum DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG) > "$base/reference.sha256"

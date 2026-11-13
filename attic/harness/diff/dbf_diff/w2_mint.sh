#!/bin/sh
# PRD 6.6: independent real III+ bytes. Shared runner: mint_capture.sh.
set -eu
ref=${1:-../dbase3-decomp}
only=${2:-all}
root=$(pwd)
base="$root/harness/diff/dbf_diff/xbase_prog_diff"
for kind in set verb; do
 for input in "$base"/corpus/w2/"$kind"/*.prg; do
  [ -f "$input" ] || continue
  name=$(basename "$input" .prg)
  case ",$only," in *,all,*|*,"$kind/$name",*) ;; *) continue;; esac
  work="$root/build/samirw2/$kind/$name"
  dest="$base/$kind-mint/$name"
  rm -rf "$work"
  mkdir -p "$work/DATA" "$work/DRIVE"
  for file in DBASE.EXE DBASE.OVL DBASEINL.OVL DBASE.MSG; do cp -f "$ref/mint/work/$file" "$work/$file"; done
  printf 'STATUS=OFF\r\nTALK=OFF\r\nSAFETY=OFF\r\nBELL=OFF\r\n' > "$work/CONFIG.DB"
  cp -f "$root/build/CLIENTS.DBF" "$work/BASE.DBF"
  "$root/build/w2_fixture" "$work"
  if [ "$name" = view ]; then
   mkdir -p "$work/VIEW"
   sample="$ref/goldens/dbase-iii-plus-1.1-pristine/files/Sample_Programs_and_Utilities"
   for file in RESERVE.VUE RESERVE.FMT TRAVEL.DBF TRAVEL.DBT TRIPS.NDX CLIENTS.DBF CNAMES.NDX; do cp -f "$sample/$file" "$work/VIEW/$file"; done
  fi
  printf 'PROCEDURE LIBONE\r\n? "LIBRARY ONE"\r\nRETURN\r\n' > "$work/LIBRARY.PRG"
  printf 'PROCEDURE LIBONE\r\n? "LIBRARY TWO"\r\nRETURN\r\n' > "$work/LIBRARY2.PRG"
  printf '? "SEARCH-PATH PROGRAM FOUND"\r\nRETURN\r\n' > "$work/DATA/ONLYDATA.PRG"
  printf 'STORE 101 TO OFFTRACE\r\nSTORE 102 TO OFFTRACE\r\nRETURN\r\n' > "$work/HISTOFF.PRG"
  printf 'STORE 201 TO ONTRACE\r\nSTORE 202 TO ONTRACE\r\nRETURN\r\n' > "$work/HISTON.PRG"
  for n in $(seq 1 24); do printf 'STORE %s TO HISTVALUE\r\n' "$n"; done > "$work/HISTLONG.PRG"
  printf 'RETURN\r\n' >> "$work/HISTLONG.PRG"
  cp -f "$input" "$work/FNSTEST.PRG"
  # Freeze replay inputs BEFORE III+ can INSERT, UPDATE, DELETE or REPLACE.
  mkdir -p "$work/INPUT/DATA" "$work/INPUT/DRIVE"
  for file in BASE.DBF OTHER.DBF MEMO.DBF MEMO.DBT LIBRARY.PRG LIBRARY2.PRG HISTOFF.PRG HISTON.PRG HISTLONG.PRG; do cp -f "$work/$file" "$work/INPUT/$file"; done
  cp -f "$work/DATA/TARGET.DBF" "$work/DATA/ONLYDATA.PRG" "$work/INPUT/DATA/"
  cp -f "$work/DRIVE/BASE.DBF" "$work/INPUT/DRIVE/"
  if [ -d "$work/VIEW" ]; then cp -rf "$work/VIEW" "$work/INPUT/VIEW"; fi
  (cd "$work/INPUT" && find . -type f | LC_ALL=C sort | xargs sha256sum) > "$work/input.sha256"
  cat > "$work/MINT.PRG" <<'PRG'
SET TALK OFF
SET SAFETY OFF
ON ERROR ? "MINT-ERROR", ERROR(), MESSAGE()
SET ALTERNATE TO C:\GOLD.OUT
SET ALTERNATE ON
DO FNSTEST
SET DEFAULT TO C
SET ALTERNATE TO
QUIT
PRG
  printer=0
  case "$kind/$name" in set/margin|set/print|verb/eject) printer=1;; esac
  MINT_PRINTER_CAPTURE=$printer MINT_FREESIZE=64 sh "$root/harness/diff/dbf_diff/mint_capture.sh" "$work" "$input" '[W2-END]'
  (cd "$work/INPUT" && sha256sum --quiet -c ../input.sha256)
  if [ "$printer" = 1 ]; then test -s "$work/PRINTER.OUT"; fi
  if [ "${MINT_PROBE:-0}" = 1 ]; then continue; fi
  # Promote only a complete flushed capture with reviewed error declarations.
  rm -rf "$dest"
  mkdir -p "$dest/INPUT"
  for file in FNSTEST.PRG GOLD.OUT MINT.PRG CONFIG.DB mint.conf mint.log; do cp -f "$work/$file" "$dest/$file"; done
  cp -rf "$work/INPUT/." "$dest/INPUT/"
  if [ "$printer" = 1 ]; then cp -f "$work/PRINTER.OUT" "$dest/PRINTER.OUT"; fi
  (cd "$dest" && { printf '%s\n' FNSTEST.PRG GOLD.OUT MINT.PRG CONFIG.DB mint.conf; find INPUT -type f; if [ -f PRINTER.OUT ]; then printf '%s\n' PRINTER.OUT; fi; } | LC_ALL=C sort | xargs sha256sum) > "$dest/sha256.txt"
  printf 'MINT W2-%s-%s: %s/GOLD.OUT\n' "$kind" "$name" "$dest"
 done
done

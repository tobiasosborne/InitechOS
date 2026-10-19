#!/bin/sh
# VO-30 (PUB-REF-0001 Sec 9.3; bead initech-ipu5.16): the class reports where
# a \Slip printed.  A proof book puts one slip in each place the register
# names and the written <job>.slips must equal the places listed below,
# which are written here by hand from the class contract (initech-syntax.sty
# \Slip), never read from the class.  Usage: slip_places.sh <out dir> [mutant]
# The mutant typesets the same book with the running heads' place switched
# off (\fancyhfinit{}) and must go RED: proof that the comparison bites.
set -eu
root=$(pwd); out=$1; mode=${2:-}
d=$out/slip-places${mode:+-$mode}
rm -rf "$d"; mkdir -p "$d"
pre=''
[ "$mode" = mutant ] && pre='\fancyhfinit{}'
{ printf '%s\n' '\documentclass[operations]{initech-manual}' \
    '\ProductName{Initech}\DocTitle{Slip Proof}' "$pre" '\begin{document}' \
    '\FrontMatterFolios' '\MakeTitlePage' '\tableofcontents' \
    '\chapter{\Slip{VO30-04}{A Chapter head}}' \
    '\RunningTitle{\Slip{VO30-04}{A Running title}}' \
    '\section{\Slip{VO30-04}{A Section head}}' \
    'Body \Slip{VO30-06}{it'"'"'s} and \doctitle{InitechDOS \Slip{VO30-01}{Users Guide}}.' \
    '\index{proof@\Slip{VO30-09}{Proof entry}}' \
    '\newpage Second page.' '\newpage Third page, a recto.' '\printindex' '\end{document}'; } > "$d/proof.tex"
( cd "$d" && SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
    TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
    TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" INDEXSTYLE="$root/pubs/class:" \
    "${LATEXMK:-latexmk}" -g -r "$root/pubs/latexmkrc" -interaction=nonstopmode -halt-on-error proof.tex ) \
    > "$d/build.log" 2>&1 || { tail -20 "$d/build.log"; printf '!!! test-pubs RED SLIP-PLACES: the proof book did not typeset\n'; exit 1; }
# Distinct ID|PLACE|text triples, as the class must report them.
cut -d'|' -f1,2,4 "$d/proof.slips" | LC_ALL=C sort -u > "$d/got"
LC_ALL=C sort > "$d/want" <<'WANT'
VO30-01|cross-reference|Users Guide
VO30-04|contents|A Chapter head
VO30-04|contents|A Section head
VO30-04|heading|A Chapter head
VO30-04|heading|A Section head
VO30-04|running-head|A Running title
VO30-06|body|it's
VO30-09|index|Proof entry
WANT
if ! cmp -s "$d/want" "$d/got"; then
    diff -u "$d/want" "$d/got" || true
    printf '!!! test-pubs RED SLIP-PLACES: the class reports slips in the wrong places (%s)\n' "$d/proof.slips"
    exit 1
fi
printf '>>> test-pubs-slip-places: green (%s places reported as the class contract says)\n' "$(wc -l < "$d/want")"

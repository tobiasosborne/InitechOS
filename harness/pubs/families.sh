#!/bin/sh
# Compile every PG-10..26 family branch, not just the IBM pilot branch.
set -eu
root=$(pwd); out=$root/build/pubs/families
mkdir -p "$out"
for family in ibm ashtontate lotus wordperfect borland apple operations; do
    d=$out/$family
    mkdir -p "$d"
    printf '\\documentclass[%s]{initech-manual}\n' "$family" > "$d/model.tex"
    cat >> "$d/model.tex" <<'TEX'
\ProductName{Initech}
\DocTitle{Family Proof}
\begin{document}
\FrontMatterFolios
\tableofcontents
\chapter{File Tasks}
\begin{ChapterCovers}
\item Working with files
\end{ChapterCovers}
\section{Working with Files}
This paragraph proves the family body face and fixed left side column.
\SideHead{Purpose}The operator reads this text.
\subsection{A Second Head}
The key name is \key{Enter}.
\newpage
This page proves the running head and foot.
\appendix
\chapter{Register}
The appendix proves its folio prefix.
\end{document}
TEX
    (cd "$d" && SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
        TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
        TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" INDEXSTYLE="$root/pubs/class:" \
        "${LATEXMK:-latexmk}" -g -r "$root/pubs/latexmkrc" model.tex) > "$d/build.log" 2>&1 || {
        tail -25 "$d/build.log"; exit 1; }
    sh harness/pubs/check_log.sh "$d/model.log" 0
    case $family in ibm|wordperfect|operations) face=Termes;; *) face=Pagella;; esac
    pdffonts "$d/model.pdf" | grep -q "$face" || {
        printf '!!! test-pubs RED FAMILY-FACE: %s lacks %s\n' "$family" "$face"; exit 1; }
    printf '>>> test-pubs-families: %s green (%s, opener, headings, appendix, running furniture)\n' "$family" "$face"
done
printf '>>> test-pubs-families: green (7 model families typeset with their required body faces)\n'

# ---------------------------------------------------------------------------
# Identity leg (PUB-REF-0001 D-8 as revised, PK-01, PK-02, PK-03, PK-05;
# worklog WL-0132): the cover furniture contract.  A proof cover (front and
# back) and spine are typeset with the house class, rendered by pdftoppm, and
# graded by cover_check against the INDEPENDENT measured Initech Cyan
# (#57B19F, in cover_check.c, never read from the class): the logo must
# stand in the publisher-mark cell of front, back and spine with all three
# inks, and the line band must run cyan along the foot.  Rule 6: a cover
# without the logo and a cover in the wrong cyan (the desktop teal's
# build) must each go RED for the right reason.
check=${COVER_CHECK:?COVER_CHECK must name the built cover_check}
d=$out/identity
mkdir -p "$d"
cover_doc() { # $1 name, $2 class options, $3 extra preamble, $4 body
    { printf '\\documentclass[%s]{initech-manual}\n' "$2"
      printf '%s\n' '\ProductName{InitechDOS}\ProductVersion{Version 3.30}' \
        '\DocTitle{Reference}\PartNumber{1024-3300-10}\PrintCode{A1-3/92}' \
        '\FamilyName{Systems Family}\FamilyColor{FamilySystems}' "$3" \
        '\begin{document}' "$4" '\end{document}'; } > "$d/$1.tex"
    (cd "$d" && SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
        TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
        TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" \
        "${LUALATEX:-lualatex}" -interaction=nonstopmode -halt-on-error "$1.tex") > "$d/$1.build.log" 2>&1 || {
        tail -25 "$d/$1.build.log"; printf '!!! test-pubs RED COVER-BUILD: %s did not typeset\n' "$1"; exit 1; }
    sh harness/pubs/check_log.sh "$d/$1.log" 0
    pdftoppm -r 150 "$d/$1.pdf" "$d/$1"
}
grade() { # $1 tag, then cover_check arguments
    tag=$1; shift
    "$check" "$@" || { printf '!!! test-pubs-families FAIL: identity %s\n' "$tag"; exit 1; }
}
cover_doc cover ibm,cover '' '\MakeCover\MakeBackCover'
cover_doc spine ibm,spine '' '\MakeSpine'
grade front-logo "$d/cover-1.ppm" 150 logo 23 183 30 20.5
grade front-band "$d/cover-1.ppm" 150 band 2 223.5 173 4.5
grade back-logo  "$d/cover-2.ppm" 150 logo 23 19 30 20.5
grade back-band  "$d/cover-2.ppm" 150 band 2 223.5 173 4.5
grade spine-logo "$d/spine-1.ppm" 150 logo 11 6 22 15
grade spine-band "$d/spine-1.ppm" 150 band 1 223.5 42 4.5
red_cover() { # $1 name, $2 expected tag, $3 extra preamble
    cover_doc "$1" ibm,cover "$3" '\MakeCover'
    if "$check" "$d/$1-1.ppm" 150 logo 23 183 30 20.5 > "$d/$1.check.log" 2>&1; then
        cat "$d/$1.check.log"; printf '!!! test-pubs-families FAIL: mutant %s survived the identity leg\n' "$1"; exit 1; fi
    grep -F "test-pubs RED $2:" "$d/$1.check.log" || {
        cat "$d/$1.check.log"; printf '!!! test-pubs-families FAIL: mutant %s failed for the wrong reason\n' "$1"; exit 1; }
    printf '>>> test-pubs-families: mutant %s rejected (%s)\n' "$1" "$2"
}
red_cover mutant-nologo COVER-LOGO '\makeatletter\renewcommand\InitechLogo[1][]{}\makeatother'
red_cover mutant-wrongcyan COVER-CYAN '\definecolor{InitechCyan}{cmyk}{0.36,0,0,0.14}'
printf '>>> test-pubs-families: identity green (logo in the cell on front, back and spine in Initech Cyan #57B19F; cyan line band; 2 mutants RED)\n'

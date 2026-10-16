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
        TEXINPUTS="$root/pubs/class//:" INDEXSTYLE="$root/pubs/class:" \
        "${LATEXMK:-latexmk}" -g -r "$root/pubs/latexmkrc" model.tex) > "$d/build.log" 2>&1 || {
        tail -25 "$d/build.log"; exit 1; }
    sh harness/pubs/check_log.sh "$d/model.log" 0
    case $family in ibm|wordperfect|operations) face=Termes;; *) face=Pagella;; esac
    pdffonts "$d/model.pdf" | grep -q "$face" || {
        printf '!!! test-pubs RED FAMILY-FACE: %s lacks %s\n' "$family" "$face"; exit 1; }
    printf '>>> test-pubs-families: %s green (%s, opener, headings, appendix, running furniture)\n' "$family" "$face"
done
printf '>>> test-pubs-families: green (7 model families typeset with their required body faces)\n'

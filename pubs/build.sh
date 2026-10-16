#!/bin/sh
# build.sh -- typeset every Initech manual (bead initech-ipu5.5).
# Usage: build.sh <out root> <generated-tex dir> [mutant]
# For each pubs/manuals/<m>/{main,cover,spine}.tex present, runs latexmk with
# LuaLaTeX (D-11) into <out root>/<m>/<doc>.pdf.  Reproducible (Rule 11):
# SOURCE_DATE_EPOCH is pinned to the edition date (1992-03-01) and
# FORCE_SOURCE_DATE=1 makes \today and friends obey it; the class suppresses
# the PDF /ID and date fields.  The third argument `mutant` injects the build
# time into every page (make test-pubs-repro-mutant: proves the cmp bites).
# Thin shell (Law 3); TeX is an external tool like nasm.
set -eu
out=$1; gen=$2; mode=${3:-}
root=$(cd "$(dirname "$0")" && pwd)
LATEXMK=${LATEXMK:-latexmk}
mkdir -p "$out"
out=$(cd "$out" && pwd); gen=$(cd "$gen" && pwd)
pretex=
[ "$mode" = mutant ] && pretex='-usepretex=\def\initechpubsmutant{}'
n=0
for dir in "$root"/manuals/*/; do
    m=$(basename "$dir")
    for doc in main cover spine; do
        [ -f "$dir/$doc.tex" ] || continue
        n=$((n + 1))
        mkdir -p "$out/$m"
        log=$out/$m/$doc.latexmk.out
        if ! ( cd "$dir" && \
               SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
               TEXINPUTS="$root/class//:$gen//:" INDEXSTYLE="$root/class:" \
               "$LATEXMK" -r "$root/latexmkrc" -outdir="$out/$m" $pretex "$doc.tex" \
             ) > "$log" 2>&1; then
            grep -E '^[^ ]*:[0-9]+: |^! ' "$out/$m/$doc.log" 2>/dev/null | head -20 || true
            tail -20 "$log"
            printf '!!! pubs FAIL: %s/%s.tex did not build (log %s)\n' "$m" "$doc" "$log"
            exit 1
        fi
        printf '>>> pubs: %s/%s.pdf\n' "$m" "$doc"
    done
done
[ "$n" -gt 0 ] || { printf '!!! pubs FAIL: no pubs/manuals/*/main.tex found\n'; exit 1; }
printf '>>> pubs: %d documents built into %s\n' "$n" "$out"

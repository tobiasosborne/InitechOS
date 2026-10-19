#!/bin/sh
# build.sh -- typeset every Initech manual (bead initech-ipu5.5).
# Usage: build.sh <out root> <generated-tex dir> [mutant] [manual]
# For each pubs/manuals/<m>/{main,cover,spine,strip}.tex present (strip: a
# keyboard template, PK-14; bead initech-ipu5.10), runs latexmk with
# LuaLaTeX (D-11) into <out root>/<m>/<doc>.pdf.  Reproducible (Rule 11):
# SOURCE_DATE_EPOCH is pinned to the edition date (1992-03-01) and
# FORCE_SOURCE_DATE=1 makes \today and friends obey it; the class suppresses
# the PDF /ID and date fields.  The third argument `mutant` injects the build
# time into every page (make test-pubs-repro-mutant: proves the cmp bites).
# -interaction=nonstopmode -halt-on-error go to latexmk itself as well: with
# -usepretex (the mutant) latexmk builds its own command line, and a LaTeX
# error there used to wait for terminal input forever (ipu5.7).
# Thin shell (Law 3); TeX is an external tool like nasm.
set -eu
out=$1; gen=$2; mode=${3:-}; manual=${4:-}
root=$(cd "$(dirname "$0")" && pwd)
cache=$root/../build/pubs-tex-cache
mkdir -p "$cache"
export TEXMFVAR="$cache" TEXMFCACHE="$cache"
LATEXMK=${LATEXMK:-latexmk}
mkdir -p "$out"
out=$(cd "$out" && pwd); gen=$(cd "$gen" && pwd)
pretex=
[ "$mode" = mutant ] && pretex='-usepretex=\def\initechpubsmutant{}'
# The InitechBase volumes share one master index (bead initech-ipu5.8,
# AN-24/CP-13): initech-xbase.sty merges the partner volume's own index
# entries from $INITECH_PUBS_OUT/<partner>/main.own.idx, and a manual with a
# MASTERINDEX file is typeset again after every manual has been (pass 2), so
# each volume sees the other's entries whatever the build order.
export INITECH_PUBS_OUT="$out" INITECH_PUBS_PASS=1
n=0
# Optional single-manual route for the initech-46u4 stale-aux/repro oracle;
# the default still builds the complete set (PUB-REF-0001 GC-07).
for dir in "$root"/manuals/${manual:-*}/; do
    m=$(basename "$dir")
    for doc in main cover spine strip; do
        [ -f "$dir/$doc.tex" ] || continue
        n=$((n + 1))
        mkdir -p "$out/$m"
        log=$out/$m/$doc.latexmk.out
        if ! ( cd "$dir" && \
               SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
               TEXINPUTS="$root/class//:$root/common//:$gen//:" INDEXSTYLE="$root/class:" \
               "$LATEXMK" -g -r "$root/latexmkrc" -interaction=nonstopmode -halt-on-error -outdir="$out/$m" $pretex "$doc.tex" \
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
INITECH_PUBS_PASS=2
for dir in "$root"/manuals/${manual:-*}/; do
    m=$(basename "$dir")
    [ -f "$dir/MASTERINDEX" ] || continue
    log=$out/$m/main.latexmk.out
    if ! ( cd "$dir" && \
           SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
           TEXINPUTS="$root/class//:$root/common//:$gen//:" INDEXSTYLE="$root/class:" \
           "$LATEXMK" -g -r "$root/latexmkrc" -interaction=nonstopmode -halt-on-error -outdir="$out/$m" $pretex main.tex \
         ) > "$log" 2>&1; then
        grep -E '^[^ ]*:[0-9]+: |^! ' "$out/$m/main.log" 2>/dev/null | head -20 || true
        tail -20 "$log"
        printf '!!! pubs FAIL: %s/main.tex did not build on the master-index pass (log %s)\n' "$m" "$log"
        exit 1
    fi
    printf '>>> pubs: %s/main.pdf (master index)\n' "$m"
done
printf '>>> pubs: %d documents built into %s\n' "$n" "$out"

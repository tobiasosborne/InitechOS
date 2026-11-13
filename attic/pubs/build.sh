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
# Pass 1 runs the documents in PARALLEL (bead initech-7opiw, F6): up to
# PUBS_JOBS at once (default: nproc, at most 16; PUBS_JOBS=1 is the old serial
# order).  Every document has its own jobname inside a per-manual outdir, so
# the runs share no file -- except the InitechBase master index, where a
# volume reads its partner's main.own.idx (below).  The mains of the books
# that carry a MASTERINDEX file are therefore ONE sequential chain in glob
# order, exactly the order of the old serial build, so what each of them sees
# of its partner in pass 1 (and hence every byte of the result) does not
# depend on scheduling.  Pass 2 (the master-index re-typeset) runs after ALL
# of pass 1, as before.
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

# One document of pass 1: build_one <manual> <doc>
build_one() {
    m=$1; doc=$2; dir=$root/manuals/$m
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
        return 1
    fi
    printf '>>> pubs: %s/%s.pdf\n' "$m" "$doc"
}
# Worker mode (xargs -> this script): after the four ordinary arguments the
# words are manual/doc tokens, built one after the other.
if [ "${PUBS_WORKER:-}" = 1 ]; then
    shift 4
    for tok in "$@"; do build_one "${tok%%/*}" "${tok#*/}" || exit 1; done
    exit 0
fi
jobs=${PUBS_JOBS:-}
if [ -z "$jobs" ]; then
    jobs=$(nproc 2>/dev/null || echo 4)
    [ "$jobs" -le 16 ] || jobs=16
fi
# Job list, one line per unit: the MASTERINDEX mains chained first (glob
# order), then every other document on a line of its own.
n=0; chain=; units=$out/.pass1.units
: > "$units"
# Optional single-manual route for the initech-46u4 stale-aux/repro oracle;
# the default still builds the complete set (PUB-REF-0001 GC-07).
for dir in "$root"/manuals/${manual:-*}/; do
    m=$(basename "$dir")
    if [ -f "$dir/MASTERINDEX" ] && [ -f "$dir/main.tex" ]; then chain="$chain $m/main"; fi
done
if [ -n "$chain" ]; then printf '%s\n' "${chain# }" >> "$units"; fi
for dir in "$root"/manuals/${manual:-*}/; do
    m=$(basename "$dir")
    for doc in main cover spine strip; do
        [ -f "$dir/$doc.tex" ] || continue
        n=$((n + 1))
        if [ "$doc" = main ] && [ -f "$dir/MASTERINDEX" ]; then continue; fi
        printf '%s/%s\n' "$m" "$doc" >> "$units"
    done
done
[ "$n" -gt 0 ] || { printf '!!! pubs FAIL: no pubs/manuals/*/main.tex found\n'; exit 1; }
# xargs -L1 keeps a line (a chain) together; any failed unit makes xargs exit
# non-zero.
if ! PUBS_WORKER=1 xargs -L1 -P "$jobs" sh "$root/build.sh" "$out" "$gen" "$mode" "$manual" < "$units"; then
    printf '!!! pubs FAIL: pass 1 did not build every document (see the log above)\n'
    exit 1
fi
rm -f "$units"
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

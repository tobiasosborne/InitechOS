#!/bin/sh
# One local composed run. Scratch survives clean/repro/bigkernel's build removal.
set -u
cd "$(dirname "$0")/.." || exit 1
root=$PWD
id=$(date -u +%Y-%m-%dT%H%M%SZ)-$(git rev-parse --short HEAD)
work=$(mktemp -d)
start=$(date +%s)
failed=0
: > "$work/summary"
: > "$work/full.log"
finish() {
    elapsed=$(($(date +%s)-start))
    echo "total: ${elapsed}s; failed stages: $failed" >> "$work/summary"
    dest=$root/build/cert/$id
    mkdir -p "$dest"
    cp -R "$work"/. "$dest/"
    rm -rf "$work"
    cat "$dest/summary"
    echo "cert evidence: $dest"
}
trap 'failed=$((failed+1)); finish; exit 1' HUP INT TERM
stage() {
    name=$1; shift
    begun=$(date +%s)
    echo "cert: $name"
    rc=0
    "$@" > "$work/$name.log" 2>&1 || rc=$?
    took=$(($(date +%s)-begun))
    state=PASS
    if [ "$rc" -ne 0 ]; then state=FAIL; failed=$((failed+1)); fi
    echo "$name: $state (${took}s; exit $rc)" >> "$work/summary"
    printf '\n=== %s ===\n' "$name" >> "$work/full.log"
    cat "$work/$name.log" >> "$work/full.log"
    # Show the useful tail immediately; the complete log is retained.
    tail -n 8 "$work/$name.log"
}
clean_test() { make clean && make -j"${CERT_JOBS:-16}" test; }
image_smoke() { make image && make smoke; }
selfhost_ddc() { make -C turbo selfhost && make -C turbo ddc; }
board() {
    make status > "$work/board.txt" 2>&1 || return
    cat "$work/board.txt"
    if [ "${CERT_STRICT:-0}" = 1 ]; then
        awk '/^[a-z][a-z0-9\/]*: (host-green|red)$/ {bad=1} END {exit bad}' "$work/board.txt"
    fi
}
unchanged() {
    git status --porcelain > "$work/git-status.txt"
    cat "$work/git-status.txt"
    test ! -s "$work/git-status.txt"
}
stage clean-test clean_test
stage image-smoke image_smoke
stage repro make repro
stage mutants sh factory/mutants.sh "$work"
for log in "$work"/mutants/*.log; do [ ! -f "$log" ] || cat "$log" >> "$work/full.log"; done
stage selfhost-ddc selfhost_ddc
stage bigkernel make -C boot bigkernel-check
stage status board
stage tracked-files unchanged
finish
test "$failed" -eq 0

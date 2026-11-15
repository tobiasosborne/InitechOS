#!/bin/sh
# One local composed run. Scratch survives clean/repro/bigkernel's build removal.
set -u
cd "$(dirname "$0")/.." || exit 1
root=$PWD
# Budgets, timeouts and test selections are the components' own. A shell or
# make command line that loosens them would grade a weaker product. The
# defaults (mk/config.mk exports them into every recipe) are accepted.
refuse() { echo "FAIL cert: $1; certify with the components' own budgets" >&2; exit 1; }
[ "${TEST_BUDGET_S:-30}" = 30 ] || refuse "TEST_BUDGET_S=$TEST_BUDGET_S is set"
[ "${TEST_TIMEOUT_S:-20}" = 20 ] || refuse "TEST_TIMEOUT_S=$TEST_TIMEOUT_S is set"
[ -z "${TESTS:-}" ] || refuse "TESTS selects a subset"
unset TEST_BUDGET_S TEST_TIMEOUT_S TESTS
# Inherited make command-line settings ride in MAKEFLAGS in any assignment
# form (=, :=, ::=, +=, ?=); ask make itself rather than match spellings
# (initech-jgo5k). With the environment copies unset, only the command line
# can still define them.
origins=$(printf 'cert-origins:\n\t@echo $(origin TEST_BUDGET_S) $(origin TEST_TIMEOUT_S) $(origin TESTS)\n' |
          make -s --no-print-directory -f - cert-origins 2>/dev/null) || refuse "cannot read inherited make settings"
[ "$origins" = "undefined undefined undefined" ] ||
    refuse "TEST_BUDGET_S/TEST_TIMEOUT_S/TESTS origins '$origins' (set on the make command line)"
id=$(date -u +%Y-%m-%dT%H%M%SZ)-$(git rev-parse --short HEAD)
work=$(mktemp -d)
start=$(date +%s)
failed=0
: > "$work/summary"
: > "$work/full.log"
finish() {
    elapsed=$(($(date +%s)-start))
    echo "total: ${elapsed}s; failed stages: $failed" >> "$work/summary"
    if [ -f "$work/mutants/pending-count" ]; then
        echo "coverage: $(cat "$work/mutants/pending-count") pending mutation patches remain unproved" >> "$work/summary"
    fi
    if [ -f "$work/board.txt" ]; then
        awk '/^[a-z][a-z0-9\/]*: (host-green|red)$/ {n++} END {print "coverage: " n+0 " components have incomplete imports"}' "$work/board.txt" >> "$work/summary"
    fi
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
    if [ "$name" = mutants ] && [ "$rc" -eq 0 ]; then
        pending=$(cat "$work/mutants/pending-count" 2>/dev/null) || pending=missing
        case $pending in
          0) :;;
          *) state=INCOMPLETE; rc=1; failed=$((failed+1));;
        esac
    fi
    echo "$name: $state (${took}s; exit $rc)" >> "$work/summary"
    printf '\n=== %s ===\n' "$name" >> "$work/full.log"
    cat "$work/$name.log" >> "$work/full.log"
    # Show the useful tail immediately; the complete log is retained.
    tail -n 8 "$work/$name.log"
}
clean_test() {
    make clean || return
    make -s dependencies > "$work/host-registry.txt" || return
    rc=0
    make -j"${CERT_JOBS:-16}" test > "$work/host-tests.txt" 2>&1 || rc=$?
    cat "$work/host-tests.txt"
    [ "$rc" -eq 0 ] || return "$rc"
    # A successful labelled development subset is not full product evidence.
    awk -f factory/testlib/cert-host.awk "$work/host-registry.txt" "$work/host-tests.txt" || return
    while read -r component rest; do
        component=${component%:}
        awk -v component="$component" -f factory/testlib/component-result.awk "$work/host-tests.txt" || return
    done < "$work/host-registry.txt"
}
image_smoke() {
    rc=0
    make image && make smoke || rc=$?
    # Repro removes build/. Preserve the transcripts needed to diagnose even
    # an intermittent smoke failure before the next stage starts cleaning.
    mkdir -p "$work/smoke"
    for dir in build/smoke build/smoke-programs build/smoke-desktop build/turbo/smoke build/apps/initechbase/smoke; do
        [ ! -d "$dir" ] || cp -R "$dir" "$work/smoke/$(printf '%s' "$dir" | tr / _)"
    done
    return "$rc"
}
selfhost_ddc() { make -C turbo selfhost ddc; }
board() {
    make status > "$work/board.txt" 2>&1 || return
    cat "$work/board.txt"
    make -s dependencies > "$work/registry.txt" || return
    awk -f factory/testlib/cert-board.awk "$work/registry.txt" "$work/board.txt"
}
unchanged() {
    git status --porcelain > "$work/git-status.txt" || return
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

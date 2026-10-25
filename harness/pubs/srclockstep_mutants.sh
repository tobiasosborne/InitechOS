#!/bin/sh
# srclockstep_mutants.sh -- Rule 6 for `pubs_gate srclockstep` (bead
# initech-7opiw; make test-pubs-lockstep-src-mutant).  The baseline copy of
# the three InitechBase books must be GREEN; then each fault is injected into
# a fresh copy (no tracked file is touched), run through the SAME gate, and
# must fail with its specific diagnostic; restoring (a fresh copy) is GREEN.
# Usage: srclockstep_mutants.sh <pubs_gate> <shipped> <allow> <manuals dir> <work dir>
set -eu
gate=$1; shipped=$2; allow=$3; man=$4; w=$5
vo=initechbase-volume-one; qr=initechbase-quick-reference; v2=initechbase-volume-two
fresh() {
    rm -rf "$w"; mkdir -p "$w"
    for b in $vo $qr $v2; do cp -r "$man/$b" "$w/$b"; done
}
run() { "$gate" srclockstep "$shipped" "$allow" "$w/$vo" "$w/$qr" "$w/$v2"; }
red() {
    name=$1; tag=$2
    if run > "$w/$name.log" 2>&1; then
        printf '!!! test-pubs-lockstep-src-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -F "test-pubs RED $tag:" "$w/$name.log" > /dev/null || {
        cat "$w/$name.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-lockstep-src-mutant: %s rejected (%s)\n' "$name" "$tag"
}
fresh
run > "$w/baseline.log" 2>&1 || { cat "$w/baseline.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: baseline is not green\n'; exit 1; }
# 1. Volume One loses the whole ABS function entry (the name is still shipped).
fresh
awk '/^\\begin{XbaseEntry}{ABS}{function}/{skip=1} !skip{print} skip && /^\\end{XbaseEntry}/{skip=0}' "$man/$vo/u6-entries.tex" > "$w/$vo/u6-entries.tex"
cmp -s "$man/$vo/u6-entries.tex" "$w/$vo/u6-entries.tex" && { printf '!!! mutant 1 did not apply\n'; exit 1; }
red vo-entry-missing SHIPPED-NOT-DOCUMENTED
# 2. The Quick Reference loses the ABS line.
fresh
sed '/^\\QRFunction{ABS}/d' "$man/$qr/qr-functions.tex" > "$w/$qr/qr-functions.tex"
cmp -s "$man/$qr/qr-functions.tex" "$w/$qr/qr-functions.tex" && { printf '!!! mutant 2 did not apply\n'; exit 1; }
red qr-entry-missing SHIPPED-NOT-DOCUMENTED
# 3. An entry for a name the shipped inventory lacks (a non-DRAFT QR line).
fresh
printf '\\QRFunction{NOSUCHFN}{Not shipped}\n' >> "$w/$qr/qr-functions.tex"
red qr-entry-unshipped DOC-NOT-SHIPPED
# 4. A DotExample whose transcript is gone.
fresh
f=$(sed -n 's/.*\\DotExample{\(examples\/[^}]*\)}.*/\1/p' "$man/$vo/l1-quick-start.tex" | head -1)
rm -f "$w/$vo/$f.txt"
red example-txt-missing EXAMPLE-PAIR
# 5. A DotExample whose script is gone.
fresh
rm -f "$w/$vo/$f.dot"
red example-dot-missing EXAMPLE-PAIR
# 6. A duplicated entry in one book (the class's "documented twice" error).
fresh
printf '\\QRFunction{ABS}{Again}\n' >> "$w/$qr/qr-functions.tex"
red qr-entry-twice DOCUMENTED-TWICE
# Restore: a fresh copy is GREEN again.
fresh
run > "$w/restored.log" 2>&1 || { cat "$w/restored.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: restored copy is not green\n'; exit 1; }
rm -rf "$w"
printf '>>> test-pubs-lockstep-src-mutant: green (6 faults rejected with their diagnostics; restored copy green)\n'

#!/bin/sh
# Rule 6: isolated publication faults, judged by the same LABEL gate.
set -eu
gate=$1; entry=$2; keys=$3; screen=$4; out=$5
mkdir -p "$out"
red() {
    name=$1; tag=$2; shift 2
    if sh "$gate" "$@" > "$out/$name.log" 2>&1; then
        printf '!!! test-pubs-label-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -F "test-pubs RED LABEL-$tag:" "$out/$name.log" || {
        cat "$out/$name.log"; exit 1; }
    printf '>>> test-pubs-label-mutant: %s rejected\n' "$name"
}
sed 's/You can specify any mounted drive/Only the current drive can be labeled/' "$entry" > "$out/current-only.tex"
red CURRENT_ONLY DRIVE "$out/current-only.tex" "$keys" "$screen"
sed 's/C:/A:/g' "$keys" > "$out/current-only.keys"
red SAME_DRIVE_TRACE TRACE "$entry" "$out/current-only.keys" "$screen"
sed 's/ Volume in drive A is REPORTS/Invalid drive specification/' "$screen" > "$out/rejected.txt"
red REJECTED_DRIVE SCREEN "$entry" "$keys" "$out/rejected.txt"
printf '>>> test-pubs-label-mutant: green (3 named mutants rejected)\n'

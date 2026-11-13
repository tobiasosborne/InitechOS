#!/bin/sh
# Independent fresh DOS drives; compare screens and both provenance manifests.
set -eu
base=${1:-harness/diff/dbf_diff/xbase_prog_diff/screen-mint}
mkdir -p build/screen-mint
mkdir -p build/screen-mint
work=$(mktemp -d build/screen-mint/repro.XXXXXX)
printf 'Repro evidence: %s\n' "$work"
for case_dir in "$base"/*; do
    [ -f "$case_dir/KEYS" ] || continue
    c=$(basename "$case_dir")
    sh harness/diff/dbf_diff/screen_mint_oracle.sh "$case_dir"
    for run in 1 2; do
        sh harness/diff/dbf_diff/screen_mint.sh "$case_dir" "$work/run$run/$c"
    done
    for manifest in SHA256SUMS INPUTS.sha256 ORACLE.sha256; do
        cmp "$work/run1/$c/$manifest" "$work/run2/$c/$manifest"
        cmp "$case_dir/$manifest" "$work/run1/$c/$manifest"
    done
    while IFS= read -r name; do
        case $name in ''|'#'*) continue ;; esac
        cmp "$work/run1/$c/$name.screen" "$work/run2/$c/$name.screen"
        cmp "$case_dir/$name.screen" "$work/run1/$c/$name.screen"
    done < "$case_dir/DUMPS"
    printf 'RUN 1 %s\n' "$c"
    cat "$work/run1/$c/SHA256SUMS"
    printf 'RUN 2 %s\n' "$c"
    cat "$work/run2/$c/SHA256SUMS"
    printf 'GREEN: reproducible real III+ %s (two fresh runs byte-identical; stored goldens match)\n' "$c"
done

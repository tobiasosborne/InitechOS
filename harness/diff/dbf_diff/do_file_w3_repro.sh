#!/bin/sh
# initech-104j3: restored DO P/FILL/EH sources must reproduce every existing
# real III+ W3 dump. Never replace a screen golden to accept a DO change.
set -eu
base=harness/diff/dbf_diff/xbase_prog_diff/screen-mint
for name in at-to-a at-to-b at-to-c at-large at-clear-a at-clear-b at-clear-d; do
    out="build/do-file-w3/$name"
    sh harness/diff/dbf_diff/screen_mint.sh "$base/$name" "$out"
    for screen in "$base/$name"/*.screen; do
        cmp "$screen" "$out/${screen##*/}"
    done
    cmp "$base/$name/SHA256SUMS" "$out/SHA256SUMS"
    cmp "$base/$name/ORACLE.sha256" "$out/ORACLE.sha256"
    # Only the input manifest changes; all independent output bytes stay pinned.
    cp -f "$out/INPUTS.sha256" "$base/$name/INPUTS.sha256"
    printf 'GREEN DO-FILE W3 %s: every real III+ dump unchanged\n' "$name"
done

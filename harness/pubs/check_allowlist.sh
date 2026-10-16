#!/bin/sh
# GC-01: compare both uncommitted changes and the previous committed state.
# Read-only git show does not alter the index or .git. Bootstrap is explicit.
set -eu
gate=$1; allow=$2; out=$3
found=0
for ref in HEAD 'HEAD^'; do
    if git show "$ref:$allow" > "$out/previous.allow" 2>/dev/null; then
        "$gate" monotonic "$allow" "$out/previous.allow"
        found=1
    fi
done
[ "$found" = 1 ] || printf '!!! test-pubs ALLOWLIST-BOOTSTRAP: %s is new; dated initial omissions require PM review\n' "$allow"

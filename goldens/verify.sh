#!/bin/sh
# Verify hashes once per metadata identity; never regenerate expected hashes.
set -eu
cd "$(dirname "$0")/.."
if [ "${1:-}" != --ready ]; then
    exec make --no-print-directory -f goldens/preflight.mk verify CORPORA="$*"
fi
cache=${CORPUS_CACHE:?}
work=$(mktemp -d "$cache/check.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
export CORPORA
# Resolve override roots in one process. Hash manifests retain full paths,
# so relocating an override invalidates the cache even with identical stats.
awk -v repo="$(pwd)" -v work="$work" '
/^(#|$)/ || $1=="ORIGIN" { next }
ENVIRON["CORPORA"]!="" && index(" " ENVIRON["CORPORA"] " ", " " $2 " ")==0 { next }
{ root=($2=="REPO" ? repo : ENVIRON[$2]); if(root=="") { print "FAIL corpus: unset " $2 > "/dev/stderr"; exit 1 }
  if($1=="FILE") print $3 "  " root "/" $4 > work "/files";
  else print $1 " " $2 " " $3 " " root " " $4 > work "/other";
}' goldens/CORPORA.lock
: >> "$work/files"
: >> "$work/other"
while read -r kind var expected root path; do
    case $kind in
      HEAD) actual=$(git -C "$root" rev-parse HEAD 2>/dev/null || :);;
      LIST) actual=$(cd "$root" && for f in $path; do printf '%s\n' "$f"; done | LC_ALL=C sort | sha256sum | cut -d ' ' -f 1);;
      *) echo "FAIL corpus: bad lock record $kind" >&2; exit 1;;
    esac
    if [ "$actual" != "$expected" ]; then
        echo "FAIL corpus: $var $path expected $expected, got ${actual:-missing}; set $var to the pinned corpus" >&2
        exit 1
    fi
done < "$work/other"
"$cache/stat-files" < "$work/files" > "$work/signature"
if ! cmp -s "$work/signature" "$cache/verified"; then
    if ! sha256sum --quiet -c "$work/files"; then
        echo 'FAIL corpus: changed pinned bytes; check the matching *_DECOMP override in CORPORA.lock' >&2
        exit 1
    fi
    # Reject a file changed while hashing; do not cache an unverified identity.
    "$cache/stat-files" < "$work/files" > "$work/after"
    cmp -s "$work/signature" "$work/after" || { echo 'FAIL corpus: inputs changed during verification' >&2; exit 1; }
    mv "$work/signature" "$cache/verified"
fi

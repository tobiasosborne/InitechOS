#!/bin/sh
# Hash only consumed files; no cache can hide same-mtime corruption.
set -eu
cd "$(dirname "$0")/.."
while read -r kind var expected path; do
    case $kind in ''|'#'*) continue;; esac
    [ "$kind" != ORIGIN ] || continue
    if [ "$#" -gt 0 ]; then case " $* " in *" $var "*) ;; *) continue;; esac; fi
    case $var in REPO) root=.;; *_DECOMP) eval 'root=${'"$var"':-}';; *) echo "bad corpus variable: $var" >&2; exit 1;; esac
    case $kind in
      HEAD) actual=$(git -C "$root" rev-parse HEAD 2>/dev/null || :);;
      FILE) actual=$(sha256sum "$root/$path" 2>/dev/null | cut -d ' ' -f 1);;
      LIST) actual=$(cd "$root" && for f in $path; do printf '%s\n' "$f"; done | LC_ALL=C sort | sha256sum | cut -d ' ' -f 1);;
      *) echo "bad lock record: $kind" >&2; exit 1;;
    esac
    if [ "$actual" != "$expected" ]; then
        echo "FAIL corpus: $var $path expected $expected, got ${actual:-missing}; set $var to the pinned corpus" >&2
        exit 1
    fi
done < goldens/CORPORA.lock

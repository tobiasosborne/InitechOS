#!/bin/sh
# fetch-siblings.sh -- clone the six ground-truth sibling repos next to this
# checkout and restore their gitignored goldens (real period software images,
# ROMs, minted fixtures, captures) from each repo's PRIVATE GitHub release.
# Ref: docs/BOOTSTRAP.md. Needs: git, gh (logged in as tobiasosborne), tar, sha256sum.
#
#   tools/bootstrap/fetch-siblings.sh                 # clone + restore all six
#   tools/bootstrap/fetch-siblings.sh system7-decomp  # just one
#   TAG=goldens-2026-10-24 tools/bootstrap/fetch-siblings.sh   # pin a snapshot
#
# Why a release asset and not git: the goldens are copyrighted period software
# (dBASE III+, Lotus 1-2-3, WordPerfect 5.1, Mac OS ROMs/images, Windows 3.1) and
# multi-hundred-MB disk images; they are gitignored in every sibling on purpose
# and travel only as private release assets. Every sibling's SOURCES.md also
# records the original archive.org / WinWorld / vendor URL + sha256 so the set
# can be re-acquired from scratch without this script.

set -e
here="$(cd "$(dirname "$0")/../.." && pwd)"
proj="$(dirname "$here")"
OWNER=tobiasosborne
TAG="${TAG:-}"           # empty = latest release of each repo
ALL="dbase3-decomp lotus123-decomp system7-decomp win31-decomp wordperfect51-decomp turbopascal-decomp"
want="${*:-$ALL}"

for d in $want; do
    echo "== $d"
    if [ ! -d "$proj/$d/.git" ]; then
        git clone -q "https://github.com/$OWNER/$d" "$proj/$d"
    else
        (cd "$proj/$d" && git fetch -q --all)
    fi
    tmp="$(mktemp -d)"
    if [ -n "$TAG" ]; then gh release download "$TAG" --repo "$OWNER/$d" --dir "$tmp" --pattern "$d-ignored.*"
    else gh release download --repo "$OWNER/$d" --dir "$tmp" --pattern "$d-ignored.*"; fi
    tar -xf "$tmp/$d-ignored.tar" -C "$proj/$d"
    (cd "$proj/$d" && sha256sum -c --quiet "$tmp/$d-ignored.sha256" && echo "   restored + verified $(wc -l < "$tmp/$d-ignored.sha256") files" \
        && grep -qx '.goldens-restored' .git/info/exclude 2>/dev/null || echo '.goldens-restored' >> "$proj/$d/.git/info/exclude"; \
        date -u +%Y-%m-%dT%H:%M:%SZ > "$proj/$d/.goldens-restored")
    rm -rf "$tmp"
done
echo "siblings ready: $want"

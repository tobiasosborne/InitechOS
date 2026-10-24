#!/bin/sh
# snapshot-sibling.sh -- refresh a sibling's private release asset: tar every
# gitignored path of ../<repo> (goldens, installed mint trees, ROMs, captures),
# write the sha256 manifest, upload both to the repo's GitHub release.
# This is the inverse of fetch-siblings.sh. Ref: docs/BOOTSTRAP.md Sec 3.
#
#   tools/bootstrap/snapshot-sibling.sh dbase3-decomp                 # refresh goldens-2026-10-24
#   TAG=goldens-2026-11-01 tools/bootstrap/snapshot-sibling.sh lotus123-decomp   # new tag (created if absent)
#
# REFUSES if the sibling has UNTRACKED (??) files: a path that is neither
# committed nor ignored is invisible to this snapshot and will be lost on a
# machine move (lesson 2026-10-24: dbase3-decomp/mint/work -- the installed
# DBASE.EXE the SAMIR gates read -- was untracked, so the first snapshot missed
# it; initech-e8s3). Commit it or add it to .gitignore first, then re-run.

set -e
here="$(cd "$(dirname "$0")/../.." && pwd)"
proj="$(dirname "$here")"
OWNER=tobiasosborne
TAG="${TAG:-goldens-2026-10-24}"
repo="${1:?usage: snapshot-sibling.sh <sibling-repo-name>}"
dir="$proj/$repo"
[ -d "$dir/.git" ] || { echo "no sibling at $dir" >&2; exit 1; }
cd "$dir"

untracked="$(git status --short | grep '^??' || true)"
if [ -n "$untracked" ]; then
    echo "REFUSING: $repo has untracked files (commit them or add them to .gitignore):" >&2
    echo "$untracked" >&2
    exit 1
fi
if [ -n "$(git status --short)" ]; then
    echo "REFUSING: $repo has uncommitted changes" >&2; git status --short >&2; exit 1
fi

tmp="$(mktemp -d)"
git status --short --ignored | grep '^!!' | cut -c4- | grep -vE '^(\.beads|\.claude|__pycache__|\.goldens-restored)' > "$tmp/list.txt"
[ -s "$tmp/list.txt" ] || { echo "nothing ignored in $repo?" >&2; exit 1; }
tar -cf "$tmp/$repo-ignored.tar" -T "$tmp/list.txt"
tr '\n' '\0' < "$tmp/list.txt" | xargs -0 -I{} find {} -type f -print0 | xargs -0 sha256sum > "$tmp/$repo-ignored.sha256"
sha256sum -c --quiet "$tmp/$repo-ignored.sha256"
echo "$repo: $(wc -l < "$tmp/list.txt") ignored paths, $(wc -l < "$tmp/$repo-ignored.sha256") files, $(du -h "$tmp/$repo-ignored.tar" | cut -f1)"

if ! gh release view "$TAG" --repo "$OWNER/$repo" >/dev/null 2>&1; then
    gh release create "$TAG" --repo "$OWNER/$repo" --title "$TAG" --notes "gitignored ground truth (goldens, mint trees, captures); restore with initech-os tools/bootstrap/fetch-siblings.sh"
fi
gh release upload "$TAG" --repo "$OWNER/$repo" --clobber "$tmp/$repo-ignored.tar" "$tmp/$repo-ignored.sha256"
gh release view "$TAG" --repo "$OWNER/$repo" --json assets -q '.assets[] | "   \(.name) \(.size) B"'
rm -rf "$tmp"
echo "snapshot of $repo uploaded to $OWNER/$repo release $TAG"

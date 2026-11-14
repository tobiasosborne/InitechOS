#!/bin/sh
# Patch only isolated manifest copies; product sources are never changed.
set -eu
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
sh tests/manifest.sh
for fault in mutants/*.patch; do
 cp working-volume.list "$work/working-volume.list"
 patch -s -d "$work" -p1 < "$fault"
 if sh tests/manifest.sh "$work/working-volume.list" > "$work/verdict" 2>&1; then
  echo "FAIL survived: $fault"; exit 1
 fi
 grep '^FAIL installed inventory:' "$work/verdict" >/dev/null
 echo "KILLED $fault: $(cat "$work/verdict")"
done

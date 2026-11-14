#!/bin/sh
# Explicitly retain completed independent captures; never run by check.sh.
# usage: retain.sh DEST CAPTURE_ROOT [CAPTURE_ROOT ...] (later roots override)
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
dest=${1:?destination};shift
mkdir -p "$dest"
for root do
 for d in "$root"/C*; do
  test -d "$d/before" || continue
  name=${d##*/}
  test "$(cat "$d/exit-status")" = 0
  grep -q "END $name" "$d/after/RESULT.OUT"
  for f in "$src/cases/$name"/*; do cmp "$f" "$d/before/${f##*/}"; done
  mkdir -p "$dest/$name"
  cp "$d/after"/* "$dest/$name/"
  cp "$d/exit-status" "$d/result.txt" "$d/result.vga" "$dest/$name/"
 done
done
(cd "$dest" && find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort | xargs sha256sum > SHA256SUMS)

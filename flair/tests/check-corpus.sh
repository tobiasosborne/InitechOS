#!/bin/sh
set -eu
while read -r hash file; do
 case "$file" in
 SYSTEM7_DECOMP/*) path=$SYSTEM7_DECOMP/${file#*/} ;;
 WIN31_DECOMP/*) path=$WIN31_DECOMP/${file#*/} ;;
 *) echo "FAIL: unknown reference authority: $file" >&2; exit 1 ;;
 esac
 if ! printf '%s  %s\n' "$hash" "$path" | sha256sum -c --status; then
  echo "FAIL: missing or changed reference input: $path" >&2
  exit 1
 fi
done < tests/REFERENCE-INPUTS.sha256

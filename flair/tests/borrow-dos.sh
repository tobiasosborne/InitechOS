#!/bin/sh
# Host-only legacy integration view. Product never sees these include paths.
set -eu
src=$1
out=$2
mkdir -p "$out"
for f in "$src"/*.[ch]; do
 name=${f##*/}
 case "$name" in
 native.c|mz.h) sed 's|../../spec/||g' "$f" > "$out/$name" ;;
 *) ln -sf "$f" "$out/$name" ;;
 esac
done
touch "$out/stamp"

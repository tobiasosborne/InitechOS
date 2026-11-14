#!/bin/sh
# Explicit real-runtime capture, never a tier-0 oracle generator.
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
out=${1:?absolute empty output directory}
inputs=$(mktemp -d)
trap 'rm -rf "$inputs"' EXIT HUP INT TERM
mkdir "$inputs/C33"
cp "$src/../cases/C33/"* "$inputs/C33/"
awk 'BEGIN{ORS="\r\n"} {sub(/\r$/,"");if($0 ~ /^ON ERROR/)$0="ON ERROR WAIT '\''FD-READY'\''";print}' \
 "$src/../cases/C33/MINT.PRG" > "$inputs/C33/MINT.PRG"
CASE_ROOT="$inputs" CASES=C33 CAPTURE_MEMORY_MARKER=FD-READY sh "$src/../capture.sh" "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/read.c" -o "$out/read-handles"
"$out/read-handles" "$out/C33/memory.bin" > "$out/C33/handles.txt"

#!/bin/sh
set -eu
out=$1
cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/tab-width.pas > "$out/tab-width.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/tab-width" "$out/tab-width.pas" > "$out/tab-width-build.log" 2>&1 || { cat "$out/tab-width-build.log"; exit 2; }
mkdir -p "$out/runs/tab-width"
(cd "$out/runs/tab-width" && "$out/tab-width")

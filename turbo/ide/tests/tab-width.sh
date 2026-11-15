#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
setup cat tests/head-fpc.pas screen.pas buffer.pas editor.pas tests/tab-width.pas > "$out/tab-width.pas"
fpc -B -O- -v0 -FU"$out" -FE"$out" -o"$out/tab-width" "$out/tab-width.pas" > "$out/tab-width-build.log" 2>&1 || { cat "$out/tab-width-build.log"; exit 2; }
setup mkdir -p "$out/runs/tab-width"
cd "$out/runs/tab-width"
assert 'tabs:authored display columns' "$out/tab-width"

finish

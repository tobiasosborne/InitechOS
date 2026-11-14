#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
out=${1:-../build/system/installed}
python3 tests/read-samples.py "$out"
cmp "$out/HELLO.PAS" "$out/TURBOIN.PAS"
cmp ../turbo/compiler/turbo.pas "$out/TURBO.PAS"
cmp samples/TPS.DBF "$out/TPS.DBF"
cmp samples/TPSPARM.DAT "$out/TPSPARM.DAT"
# The on-disk accounting body is the independently retained canon program,
# with an ordinary reporting-date assignment ahead of it. Compare the body,
# not a second output freshly minted by the current interpreter.
expected=$(mktemp)
actual=$(mktemp)
trap 'rm -f "$expected" "$actual"' EXIT HUP INT TERM
sed -n "/^? 'INITECH SYSTEMS CORP/,$ p" ../apps/initechbase/tests/canon/y2k_accounting.prg > "$expected"
tr -d '\r' < "$out/Y2KACCT.PRG" | sed -n "/^? 'INITECH SYSTEMS CORP/,$ p" > "$actual"
cmp "$expected" "$actual"
echo 'system: complete compiler source, initial Pascal input, recovered TPS inputs and canon accounting body green'

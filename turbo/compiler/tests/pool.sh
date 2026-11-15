#!/bin/sh
set -eu
source=$1 out=$2
. "$TOP/turbo/tests/checks.sh"
setup mkdir -p "$out"
cat > "$out/pool.pas" <<'EOF'
{$MODE DELPHI}{$B+}
program PoolSearch;
var PoolCount: integer;
    PoolOffset, PoolLength: array[1..8] of integer;
EOF
setup awk '/^function PoolLengthAt/ && !/forward/ {printing=1} printing {print} printing && /^end;$/ {exit}' "$source" >> "$out/pool.pas"
cat >> "$out/pool.pas" <<'EOF'
begin
  PoolCount := 4;
  PoolOffset[1] := 1; PoolLength[1] := 2;
  PoolOffset[2] := 3; PoolLength[2] := 0;
  PoolOffset[3] := 3; PoolLength[3] := 2;
  PoolOffset[4] := 5; PoolLength[4] := 4;
  Writeln(PoolLengthAt(0), ' ', PoolLengthAt(1), ' ', PoolLengthAt(2), ' ',
          PoolLengthAt(3), ' ', PoolLengthAt(4), ' ', PoolLengthAt(5), ' ',
          PoolLengthAt(100));
  PoolCount := 0; Writeln(PoolLengthAt(1))
end.
EOF
setup "$FPC" -B -v0 -FU"$out" -FE"$out" "$out/pool.pas" > "$out/build.log" 2>&1
setup "$out/pool" > "$out/actual"
printf '0 2 0 2 0 4 0\n0\n' > "$out/expected"
assert 'pooled names: boundaries, gaps, duplicate empty offset and empty table' cmp "$out/expected" "$out/actual"
finish

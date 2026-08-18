{$MODE DELPHI}{$B+}{$H-}
{ B9.4 hand-computed code-generation tooth. The two value parameters make
  [ebp+8]/[ebp+12] observable; the two while statements make duplicate-label
  emission observable under TPS_GEN_MUT_LABELS.

  Expected stdout: TINY=10 }
program GenTiny;
var
  n: integer;

function AddPair(a: integer; b: integer): integer;
var
  local: integer;
begin
  local := a + b;
  AddPair := local
end;

begin
  n := 0;
  while n < 2 do
    n := n + 1;
  while n < 4 do
    n := n + 1;
  if n = 4 then
    n := AddPair(n, 6)
  else
    n := 99;
  write('TINY='); writeln(n)
end.

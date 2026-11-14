program probe;
type Pair = record a,b: integer; end;
procedure Run(dummy: integer);
var sentinel: integer; r: array[1..2] of Pair;
begin
  sentinel := 777;
  r[1].a := 11; r[1].b := 12;
  r[2].a := 21; r[2].b := 22;
  writeln(r[1].a, ' ', r[1].b, ' ', r[2].a, ' ', r[2].b, ' ', sentinel)
end;
begin Run(0) end.

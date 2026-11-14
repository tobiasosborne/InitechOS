program probe;
type Pair = record a,b: integer; end;
var src,dst: array[1..4] of Pair; calls: integer;
function Index(dummy: integer): integer;
begin calls := calls + 1; Index := calls end;
begin
  src[1].a := 11; src[1].b := 12;
  src[2].a := 21; src[2].b := 22;
  calls := 0; dst[1] := src[Index(0)];
  writeln(calls, ' ', dst[1].a, ' ', dst[1].b)
end.

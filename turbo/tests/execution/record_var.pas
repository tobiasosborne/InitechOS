program probe;
type Pair = record a,b: integer; end;
procedure Change(var r: Pair);
begin r.b := 99 end;
procedure Run(dummy: integer);
var sentinel: integer; r: Pair;
begin
  sentinel := 777; r.a := 11; r.b := 22;
  Change(r);
  writeln(r.a, ' ', r.b, ' ', sentinel)
end;
begin Run(0) end.
